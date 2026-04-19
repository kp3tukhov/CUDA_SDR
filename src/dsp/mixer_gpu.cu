/*
 * CUDA Frequency Mixing and Complex Vector Operations Implementation (GPU)
 *
 * This module provides CUDA kernel implementations for frequency
 * mixing (down-conversion) and complex vector operations used in
 * GPS signal processing on the GPU
 * Includes cached phase lookup table (LUT) for carrier mixing
 */

#include <cuda_runtime.h>
#include <cuComplex.h>

#include <stdio.h>
#include <stdint.h>

#include "core/params.h"
#include "core/types.h"
#include "dsp/mixer_gpu.cuh"


#define PHASE_LUT_SIZE 256  // 2^8
#define RAW_SIZE 256        // uint8_t max val


static __device__ cuFloatComplex d_mix_lut[RAW_SIZE][PHASE_LUT_SIZE];   // Cached values for all possible raw values (2^8)
static int lut_initialized = 0;


// Unpack raw 8(4+4) to float complex
static inline cuFloatComplex raw_unpack(uint8_t val) {
    return make_cuFloatComplex((float)(val & 0xF), (float)(val >> 4));
}


#ifdef __cplusplus
extern "C" {
#endif

// Initialize mixing lookup table
void init_mix_lut_gpu(void) {
    if (lut_initialized) return;

    cudaError_t err;
    cuFloatComplex host_lut[RAW_SIZE][PHASE_LUT_SIZE];

    for (int i = 0; i < PHASE_LUT_SIZE; i++) {
        double phase = 2.0 * M_PI * i / PHASE_LUT_SIZE;
        for (int j = 0; j < RAW_SIZE; j++) {
            host_lut[j][i] = cuCmulf(raw_unpack((uint8_t)j), make_cuFloatComplex((float)cos(phase), (float)sin(phase)));
        }
    }

    err = cudaMemcpyToSymbol(d_mix_lut, host_lut, sizeof(host_lut));
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA cudaMemcpyToSymbol error: %s\n", cudaGetErrorString(err));
        return;
    }

    lut_initialized = 1;
}


__global__ void mix_freq_kernel(
    int size,
    const uint8_t *signal_in,
    cuFloatComplex *signal_out,
    double phase_step
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < size) {
        uint32_t phase_step_lut = (uint32_t)(int)(phase_step * (PHASE_LUT_SIZE << 12));
        uint8_t current_phase = (uint8_t)((phase_step_lut * idx) >> 12); // Overflow wraps phase automatically

        signal_out[idx] = d_mix_lut[signal_in[idx]][current_phase];
    }
}


__global__ void cpx_vec_mul_kernel(
    int size,
    const cuFloatComplex *a,
    const cuFloatComplex *b,
    cuFloatComplex *c
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < size) {
        c[idx] = cuCmulf(a[idx], b[idx]);
    }
}


__global__ void cpx_sum_kernel(
    int size,
    const cuFloatComplex *input,
    cuFloatComplex *output
) {
    extern __shared__ cuFloatComplex sdata[];

    unsigned int tid = threadIdx.x;
    unsigned int idx = blockIdx.x * blockDim.x * 2 + threadIdx.x;

    // Load two elements per thread if available
    cuFloatComplex sum = make_cuFloatComplex(0.0f, 0.0f);
    if (idx < size) {
        sum = input[idx];
        if (idx + blockDim.x < size) {
            sum = cuCaddf(sum, input[idx + blockDim.x]);
        }
    }
    sdata[tid] = sum;
    __syncthreads();

    // Parallel reduction in shared memory
    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] = cuCaddf(sdata[tid], sdata[tid + s]);
        }
        __syncthreads();
    }

    // Each block writes its partial sum
    if (tid == 0) {
        atomicAdd(&output->x, sdata[0].x);
        atomicAdd(&output->y, sdata[0].y);
    }
}


__global__ void mix_batch_kernel(
    const uint8_t *d_raw,
    cuFloatComplex *d_mixed, 
    int N,
    int fft_size,
    int n_dop,
    int num_periods,
    const double *d_phase_steps
) {
    long idx = (long)blockIdx.x * blockDim.x + threadIdx.x;
    long total = (long)num_periods * n_dop * fft_size;
    if (idx >= total) return;

    long remaining = idx;
    int period = remaining / (n_dop * fft_size);
    remaining %= (n_dop * fft_size);
    int doppler = remaining / fft_size;
    int sample = remaining % fft_size;

    if (sample < N) {
        uint32_t phase_step_lut = (uint32_t)(int)(d_phase_steps[doppler] * (PHASE_LUT_SIZE << 12));

        uint8_t current_phase = (uint8_t)((phase_step_lut * sample) >> 12);

        const uint8_t *raw_period = d_raw + (long)period * N;
        d_mixed[idx] = d_mix_lut[raw_period[sample]][current_phase];
    } else {
        d_mixed[idx] = make_cuFloatComplex(0.0f, 0.0f);
    }
}


__global__ void multiply_batch_kernel(
    const cuFloatComplex *d_mixed_fft,
    const cuFloatComplex *d_codes_fft,
    cuFloatComplex *d_prod,
    int num_sats,
    int n_dop,
    int fft_size,
    int num_periods
) {
    long idx = (long)blockIdx.x * blockDim.x + threadIdx.x;
    long total = (long)num_periods * num_sats * n_dop * fft_size;
    if (idx >= total) return;

    long remaining = idx;
    int period = remaining / (num_sats * n_dop * fft_size);
    remaining %= (num_sats * n_dop * fft_size);
    int sat = remaining / (n_dop * fft_size);
    remaining %= (n_dop * fft_size);
    int doppler = remaining / fft_size;
    int sample = remaining % fft_size;

    long mixed_idx = (long)period * n_dop * fft_size + doppler * fft_size + sample;
    long code_idx = (long)sat * fft_size + sample;

    d_prod[idx] = cuCmulf(d_mixed_fft[mixed_idx], d_codes_fft[code_idx]);
}


#ifdef __cplusplus
}
#endif
