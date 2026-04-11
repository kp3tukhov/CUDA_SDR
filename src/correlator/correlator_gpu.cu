/*
 * CUDA Batch Correlator Implementation (GPU)
 *
 * This module provides a CUDA-based batch correlator for GPS signal
 * processing on the GPU. It performs frequency-domain correlation across
 * multiple satellites, Doppler bins, and code periods with non-coherent
 * power accumulation
 */

#include <cuda_runtime.h>
#include <cuComplex.h>
#include <cufft.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/params.h"
#include "core/types.h"
#include "correlator/correlator_gpu.cuh"
#include "dsp/mixer_gpu.cuh"
#include "dsp/code_gpu.cuh"


// Number of code periods to process in one batch, higher = more parallelism, more memory
#define PERIODS_PER_BATCH 10


/*
 * PERSISTENT GPU RESOURCES                 TODO: make a module for complete host/device resources configuration
 */

// Device buffers
static cuFloatComplex *d_codes_sampled = NULL;  // [num_sats x N]
static cuFloatComplex *d_codes_fft = NULL;      // [num_sats x N]
static cuFloatComplex *d_mixed = NULL;          // [PERIODS_PER_BATCH x n_dop x N]
static cuFloatComplex *d_prod = NULL;           // [PERIODS_PER_BATCH x num_sats x n_dop x N]
static double *d_accum = NULL;                  // [num_satsxn_dop x N]
static uint16_t *d_phase_steps = NULL;          // [n_dop]

// cuFFT plans
static cufftHandle plan_code_fwd = 0;   // batch = num_sats
static cufftHandle plan_mixed_fwd = 0;  // batch = PERIODS_PER_BATCH x n_dop 
static cufftHandle plan_prod_inv = 0;   // batch = PERIODS_PER_BATCH x num_sats x n_dop

// State
static int gpu_resources_initialized = 0;
static int current_num_sats = 0;
static int current_N = 0;
static int current_n_dop = 0;


/*
 * GPU kernel for batched power accumulation across multiple periods, satellites, and Doppler bins
 * Each thread processes one (period, satellite, Doppler, sample) tuple,
 * performing atomic addition into the per-(satellite, Doppler, sample) accumulator
 *
 * Parameters:
 *   d_prod       - [batch_p × num_sats × n_dop × N] input product buffer
 *   d_accum      - [num_sats × n_dop × N] accumulator buffer
 *   batch_p      - Actual batch size (may be < PERIODS_PER_BATCH for last batch)
 *   num_sats     - Number of satellites being processed
 *   n_dop        - Number of Doppler bins
 *   N            - Number of samples per period
 *   fft_size     - FFT size for normalization
 */
__global__ void power_accum_batched_kernel(
    const cuFloatComplex *d_prod,
    double *d_accum,
    int batch_p,
    int num_sats,
    int n_dop,
    int N,
    int fft_size
) {
    long idx = (long)blockIdx.x * blockDim.x + threadIdx.x;
    long total_per_period = (long)num_sats * n_dop * N;
    long total_all = (long)batch_p * total_per_period;
    
    if (idx >= total_all) return;
    
    // Decompose index
    long local_idx = idx % total_per_period;
    
    cuFloatComplex val = d_prod[idx];
    double re = (double)val.x / fft_size;
    double im = (double)val.y / fft_size;
    double power = re * re + im * im;
    
    // Atomic add to shared accumulator
    atomicAdd(&d_accum[local_idx], power);
}


/*
 * GPU RESOURCE MANAGEMENT
 */


/*
 * Ensures all GPU resources needed for one batch processing are prepared
 */
static inline int ensure_batch_resources(int num_sats, int N, int n_dop) {
    if (num_sats < 1 || num_sats > MAX_SATS) return -1;

    // Already matching config, reuse everything
    if (gpu_resources_initialized &&
        current_num_sats == num_sats &&
        current_N == N &&
        current_n_dop == n_dop)
    {
        return 0;
    }

    // Free existing resources if config changed
    if (gpu_resources_initialized) {
        cudaFree(d_codes_sampled);
        cudaFree(d_codes_fft);
        cudaFree(d_mixed);
        cudaFree(d_prod);
        cudaFree(d_accum);
        cudaFree(d_phase_steps);
        cufftDestroy(plan_code_fwd);
        cufftDestroy(plan_mixed_fwd);
        cufftDestroy(plan_prod_inv);
        d_codes_sampled = d_codes_fft = d_mixed = d_prod = NULL;
        d_accum = NULL;
        d_phase_steps = NULL;
        plan_code_fwd = plan_mixed_fwd = plan_prod_inv = gpu_resources_initialized = 0;
    }

    // Prepare resources if config changed
    size_t cpx_bytes = (size_t)N * sizeof(cuFloatComplex);
    cudaError_t err;

    // Device buffers
    err = cudaMalloc((void **)&d_codes_sampled, cpx_bytes * num_sats);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_codes_sampled): %s\n", cudaGetErrorString(err)); return -1; }

    err = cudaMalloc((void **)&d_codes_fft, cpx_bytes * num_sats);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_codes_fft): %s\n", cudaGetErrorString(err)); return -1; }

    err = cudaMalloc((void **)&d_mixed, cpx_bytes * PERIODS_PER_BATCH * n_dop);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_mixed): %s\n", cudaGetErrorString(err)); return -1; }

    err = cudaMalloc((void **)&d_prod, cpx_bytes * PERIODS_PER_BATCH * num_sats * n_dop);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_prod): %s\n", cudaGetErrorString(err)); return -1; }

    size_t accum_bytes = (size_t)num_sats * n_dop * N * sizeof(double);
    err = cudaMalloc((void **)&d_accum, accum_bytes);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_accum): %s\n", cudaGetErrorString(err)); return -1; }

    err = cudaMalloc((void **)&d_phase_steps, n_dop * sizeof(uint16_t));
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_phase_steps): %s\n", cudaGetErrorString(err)); return -1; }

    // cuFFT plans
    if (cufftPlan1d(&plan_code_fwd,  N, CUFFT_C2C, num_sats) != CUFFT_SUCCESS) {
        fprintf(stderr, "cuFFT plan_code_fwd creation failed\n"); return -1;
    }
    
    if (cufftPlan1d(&plan_mixed_fwd, N, CUFFT_C2C, PERIODS_PER_BATCH * n_dop) != CUFFT_SUCCESS) {
        fprintf(stderr, "cuFFT plan_mixed_fwd creation failed\n"); return -1;
    }

    if (cufftPlan1d(&plan_prod_inv,  N, CUFFT_C2C, PERIODS_PER_BATCH * num_sats * n_dop) != CUFFT_SUCCESS) {
        fprintf(stderr, "cuFFT plan_prod_inv creation failed\n"); return -1;
    }

    current_num_sats = num_sats;
    current_N = N;
    current_n_dop = n_dop;
    gpu_resources_initialized = 1;

    return 0;
}

/*
 * Frees all GPU resources needed for one batch processing
 */
static inline void free_batch_resources(void) {
    if (!gpu_resources_initialized) return;
    cudaFree(d_codes_sampled);
    cudaFree(d_codes_fft);
    cudaFree(d_mixed);
    cudaFree(d_prod);
    cudaFree(d_accum);
    cudaFree(d_phase_steps);
    cufftDestroy(plan_code_fwd);
    cufftDestroy(plan_mixed_fwd);
    cufftDestroy(plan_prod_inv);
    d_codes_sampled = d_codes_fft = d_mixed = d_prod = NULL;
    d_accum = NULL;
    d_phase_steps = NULL;
    plan_code_fwd = plan_mixed_fwd = plan_prod_inv = 0;
    gpu_resources_initialized = current_num_sats = current_N = current_n_dop = 0;
}

/*
 * MAIN HOST INTERFACE
 */

int batch_corr_execute_cuda(
    const cuFloatComplex *signal,
    const int8_t *local_code,
    const correlator_config_t *config,
    const receiver_t *recv,
    double **corr_maps
) {
    if (!signal || !local_code || !config || !corr_maps) return -1;
    if (config->num_prns <= 0 || config->num_prns > MAX_SATS) return -1;

#ifdef CORR_GPU_PROFILING
    // Profiling events
    cudaEvent_t ev_start, ev_init, ev_lut, ev_sig_copy, ev_code_copy;
    cudaEvent_t ev_preprocess, ev_corr, ev_result;
    cudaEventCreate(&ev_start);
    cudaEventCreate(&ev_init);
    cudaEventCreate(&ev_lut);
    cudaEventCreate(&ev_sig_copy);
    cudaEventCreate(&ev_code_copy);
    cudaEventCreate(&ev_preprocess);
    cudaEventCreate(&ev_corr);
    cudaEventCreate(&ev_result);
    cudaEventRecord(ev_start);
#endif

    // INIT

    cudaError_t err = cudaFree(0);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA context init failed: %s\n", cudaGetErrorString(err));
        return -1;
    }
#ifdef CORR_GPU_PROFILING
    cudaEventRecord(ev_init);
#endif

    int num_sats = config->num_prns;
    int N = (int)round(recv->f_adc * (CODE_PERIOD_MS / 1000.0));
    int fft_size = N;
    int n_dop = config->n_dop;
    int num_periods = config->num_periods;

    init_phase_lut_gpu();
#ifdef CORR_GPU_PROFILING
    cudaEventRecord(ev_lut);
#endif

    if (ensure_batch_resources(num_sats, N, n_dop) != 0) return -1;

    // Precompute phase steps for all Doppler bins to use in batched frequency mixing

    uint16_t *h_steps = (uint16_t *)malloc(n_dop * sizeof(uint16_t));
    for (int d = 0; d < n_dop; d++) {
        double f_curr = recv->f_if + config->dop_min + d * config->dop_step;
        h_steps[d] = (uint16_t)round((f_curr / recv->f_adc) * (double)PHASE_LUT_SIZE);
    }
    cudaMemcpy(d_phase_steps, h_steps, n_dop * sizeof(uint16_t), cudaMemcpyHostToDevice);
    free(h_steps);



    // Copy signal buffer to GPU

    size_t signal_bytes = (size_t)N * num_periods * sizeof(cuFloatComplex);
    cuFloatComplex *d_signal = NULL;
    err = cudaMalloc((void **)&d_signal, signal_bytes);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA malloc (d_signal): %s\n", cudaGetErrorString(err));
        return -1;
    }
    err = cudaMemcpy(d_signal, signal, signal_bytes, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA memcpy (signal): %s\n", cudaGetErrorString(err));
        cudaFree(d_signal);
        return -1;
    }
#ifdef CORR_GPU_PROFILING
    cudaEventRecord(ev_sig_copy);
#endif

    // Copy all PRN codes to GPU

    size_t codes_bytes = (size_t)num_sats * GPS_CODE_LEN * sizeof(int8_t);
    int8_t *d_codes = NULL;
    err = cudaMalloc((void **)&d_codes, codes_bytes);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA malloc (d_codes): %s\n", cudaGetErrorString(err));
        cudaFree(d_signal);
        return -1;
    }
    err = cudaMemcpy(d_codes, local_code, codes_bytes, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA memcpy (codes): %s\n", cudaGetErrorString(err));
        cudaFree(d_codes);
        cudaFree(d_signal);
        return -1;
    }
#ifdef CORR_GPU_PROFILING
    cudaEventRecord(ev_code_copy);
#endif


    // PREPROCESSING (get conjugated FFT of sampled PRN codes)

    double chip_step = FS_GPS / recv->f_adc;

    // Sample all codes
    int sample_grid = (num_sats * fft_size + CUDA_BLOCK_SIZE - 1) / CUDA_BLOCK_SIZE;
    sample_code_batched_kernel<<<sample_grid, CUDA_BLOCK_SIZE>>>(
        d_codes, d_codes_sampled, num_sats, N, fft_size,
        chip_step, GPS_CODE_LEN
    );

    // FFT all codes (batched cuFFT)
    cufftResult cufft_err = cufftExecC2C(plan_code_fwd,
                        (cufftComplex*)d_codes_sampled,
                        (cufftComplex*)d_codes_fft,
                        CUFFT_FORWARD);
    if (cufft_err != CUFFT_SUCCESS) {
        fprintf(stderr, "cuFFT code forward failed (code %d)\n", cufft_err);
    }

    // Conjugate all codes
    int conj_grid = (num_sats * fft_size + CUDA_BLOCK_SIZE - 1) / CUDA_BLOCK_SIZE;
    conjugate_kernel<<<conj_grid, CUDA_BLOCK_SIZE>>>(
        d_codes_fft, num_sats * fft_size
    );

    // Check for kernel errors
    cudaError_t kerr = cudaGetLastError();
    if (kerr != cudaSuccess) {
        fprintf(stderr, "CUDA kernel error in preprocessing: %s\n", cudaGetErrorString(kerr));
    }


#ifdef CORR_GPU_PROFILING
    cudaEventRecord(ev_preprocess);
#endif


    // CORRELATION (all periods, all Dopplers, all sats in one batch)

    // Zero the accumulator
    size_t accum_bytes = (size_t)num_sats * n_dop * N * sizeof(double);
    cudaMemset(d_accum, 0, accum_bytes);


    // Calculate grid sizes for batch processing with all periods
    int mix_grid = ((long)PERIODS_PER_BATCH * n_dop * fft_size + CUDA_BLOCK_SIZE - 1) / CUDA_BLOCK_SIZE;
    int mul_grid = ((long)PERIODS_PER_BATCH * num_sats * n_dop * fft_size + CUDA_BLOCK_SIZE - 1) / CUDA_BLOCK_SIZE;
    int pow_batch_grid = ((long)PERIODS_PER_BATCH * num_sats * n_dop * N + CUDA_BLOCK_SIZE - 1) / CUDA_BLOCK_SIZE;

    // Calculate number of batches needed
    int num_batches = (num_periods + PERIODS_PER_BATCH - 1) / PERIODS_PER_BATCH;

    for (int b = 0; b < num_batches; b++) {
        int batch_start = b * PERIODS_PER_BATCH;
        int batch_size = (num_periods - batch_start < PERIODS_PER_BATCH) ?
                         (num_periods - batch_start) : PERIODS_PER_BATCH;

        // Start of signal for current batch
        const cuFloatComplex *sig_batch = d_signal + batch_start * N;

        // Mix signal for all Doppler bins and code periods in batch
        mix_batch_kernel<<<mix_grid, CUDA_BLOCK_SIZE>>>(
            sig_batch, d_mixed, N, fft_size, n_dop, batch_size, d_phase_steps
        );

        // FFT all mixed signals for current batch (batched cuFFT)
        cufftResult cufft_err2 = cufftExecC2C(plan_mixed_fwd,
                            (cufftComplex*)d_mixed,
                            (cufftComplex*)d_mixed,
                            CUFFT_FORWARD);
        if (cufft_err2 != CUFFT_SUCCESS) {
            fprintf(stderr, "cuFFT mixed forward failed, batch %d (code %d)\n", b, cufft_err2);
        }

        // Multiply all sats, Dopplers and periods
        multiply_batch_kernel<<<mul_grid, CUDA_BLOCK_SIZE>>>(
            d_mixed, d_codes_fft, d_prod,
            num_sats, n_dop, fft_size, batch_size
        );

        // IFFT all products for current batch
        cufft_err2 = cufftExecC2C(plan_prod_inv,
                            (cufftComplex*)d_prod,
                            (cufftComplex*)d_prod,
                            CUFFT_INVERSE);
        if (cufft_err2 != CUFFT_SUCCESS) {
            fprintf(stderr, "cuFFT product inverse failed, batch %d (code %d)\n", b, cufft_err2);
        }

        // Power accumulate for all periods, sats and dopplers
        power_accum_batched_kernel<<<pow_batch_grid, CUDA_BLOCK_SIZE>>>(
            d_prod, d_accum, batch_size, num_sats, n_dop, N, fft_size
        );

        // Check for kernel launch errors after each batch
        cudaError_t kerr = cudaGetLastError();
        if (kerr != cudaSuccess) {
            fprintf(stderr, "CUDA kernel error in batch %d: %s\n",
                    b, cudaGetErrorString(kerr));
        }
    }

#ifdef CORR_GPU_PROFILING
    cudaEventRecord(ev_corr);
#endif

    // Copy results back to host, one memcpy per satellite
        
    for (int s = 0; s < num_sats; s++) {
        size_t sat_map_bytes = (size_t)n_dop * N * sizeof(double);
        const double *d_sat_accum = d_accum + (size_t)s * n_dop * N;
        err = cudaMemcpy(corr_maps[s], d_sat_accum, sat_map_bytes,
                        cudaMemcpyDeviceToHost);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA memcpy (result, sat %d): %s\n",
                    s, cudaGetErrorString(err));
        }
    }

#ifdef CORR_GPU_PROFILING
    cudaEventRecord(ev_result);

    // Profiling report
    float ms;
    float t_init, t_lut, t_sig_copy, t_code_copy;
    float t_preprocess, t_corr, t_result;

    cudaEventElapsedTime(&ms, ev_start, ev_init); t_init = ms;
    cudaEventElapsedTime(&ms, ev_init, ev_lut); t_lut = ms;
    cudaEventElapsedTime(&ms, ev_lut, ev_sig_copy); t_sig_copy = ms;
    cudaEventElapsedTime(&ms, ev_sig_copy, ev_code_copy); t_code_copy = ms;
    cudaEventElapsedTime(&ms, ev_code_copy, ev_preprocess); t_preprocess = ms;
    cudaEventElapsedTime(&ms, ev_preprocess, ev_corr); t_corr = ms;
    cudaEventElapsedTime(&ms, ev_corr, ev_result); t_result = ms;

    float t_total = t_init + t_lut + t_sig_copy + t_code_copy
                   + t_preprocess + t_corr + t_result;

    fprintf(stderr, "\n=== GPU Batch Correlator Profiling Report ===\n");
    fprintf(stderr, "  Satellites: %-3d  Doppler bins: %-4d  Periods: %-3d  Samples/period: %-5d\n",
            num_sats, n_dop, num_periods, N);
    fprintf(stderr, "  CUDA Context Init:     %8.3f ms  (%5.1f%%)\n", t_init, 100.0f * t_init / t_total);
    fprintf(stderr, "  Phase LUT:             %8.3f ms  (%5.1f%%)\n", t_lut, 100.0f * t_lut / t_total);
    fprintf(stderr, "  Signal H2D Copy:       %8.3f ms  (%5.1f%%)\n", t_sig_copy, 100.0f * t_sig_copy / t_total);
    fprintf(stderr, "  Codes H2D Copy:        %8.3f ms  (%5.1f%%)\n", t_code_copy, 100.0f * t_code_copy / t_total);
    fprintf(stderr, "  Code Preprocess:       %8.3f ms  (%5.1f%%)\n", t_preprocess, 100.0f * t_preprocess / t_total);
    fprintf(stderr, "  Correlation (all):     %8.3f ms  (%5.1f%%)\n", t_corr, 100.0f * t_corr / t_total);
    fprintf(stderr, "  Result D2H Copy:       %8.3f ms  (%5.1f%%)\n", t_result, 100.0f * t_result / t_total);
    fprintf(stderr, "  =============================================\n");
    fprintf(stderr, "  TOTAL GPU TIME:        %8.3f ms\n", t_total);
    fprintf(stderr, "===============================================\n\n");

    cudaEventDestroy(ev_start);
    cudaEventDestroy(ev_init);
    cudaEventDestroy(ev_lut);
    cudaEventDestroy(ev_sig_copy);
    cudaEventDestroy(ev_code_copy);
    cudaEventDestroy(ev_preprocess);
    cudaEventDestroy(ev_corr);
    cudaEventDestroy(ev_result);
#endif


    cudaFree(d_signal);
    cudaFree(d_codes);

    free_batch_resources();

    return 0;
}
