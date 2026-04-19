/*
 * GNSS receiver and channel-block construction
 *
 * This module implements allocation and teardown functions for the receiver context,
 * RF front-end channels, and satellite channel blocks. Block setup includes PRN code
 * generation, acquisition state, CUDA unified-memory buffers, cuFFT plans, and
 * precomputed code spectra used by the GPU correlator pipeline
 */

#include <cuda_runtime.h>
#include <cuComplex.h>
#include <cufft.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "core/params.h"
#include "core/types.h"
#include "core/rcv.cuh"
#include "code/getcode.h"
#include "dsp/code_gpu.cuh"
#include "dsp/mixer_gpu.cuh"


/*
 * ============================================================================
 * RECEIVER LIFECYCLE
 * ============================================================================
 */

GNSSReceiver_t *new_receiver(const RF_channel_t *rf_ch) {
    GNSSReceiver_t *rcv = (GNSSReceiver_t *)calloc(sizeof(GNSSReceiver_t), 1);
    if (!rcv) {
        fprintf(stderr, "Failed to allocate receiver\n");
        return NULL;
    }

    rcv->rf_ch = (RF_channel_t *) rf_ch;
    rcv->num_blocks = 0;

    return rcv;
}

void free_receiver(GNSSReceiver_t *rcv) {
    if (!rcv) return;

    for (int i = 0; i < rcv->num_blocks; i++) {
        free_block(rcv->blocks[i]);
    }

    free(rcv);
}


RF_channel_t *new_rf_ch(
    double f_adc,
    double f_bw,
    double f_lo,
    int iq,
    int buffsize
) {
    RF_channel_t *rf_ch = (RF_channel_t *)calloc(sizeof(RF_channel_t), 1);
    if (!rf_ch) {
        fprintf(stderr, "Failed to allocate RF channel\n");
        return NULL;
    }

    rf_ch->f_adc = f_adc;
    rf_ch->f_bw = f_bw;
    rf_ch->f_lo = f_lo;
    rf_ch->iq = iq;

    rf_ch->buffsize = buffsize;
    rf_ch->buffer = (uint8_t *)malloc(sizeof(uint8_t)*buffsize);
    if (!rf_ch->buffer) {
        fprintf(stderr, "Failed to allocate RF channel buffer\n");
        free(rf_ch);
        return NULL;
    }

    return rf_ch;
}

void free_rf_ch(RF_channel_t *rf_ch) {
    if (!rf_ch) return;

    if (rf_ch->buffer) free(rf_ch->buffer);

    free(rf_ch);
}

/*
 * ============================================================================
 * BLOCK MANAGEMENT
 * ============================================================================
 */

int add_block_to_receiver(GNSSReceiver_t *rcv, sys_t *syss, sig_t *sigs, int *prns, int num_prns, const correlator_config_t *cfg) {
    if (!rcv || rcv->num_blocks >= MAXBLOCKS) return -1;

    satellite_channel_block_t *blk = new_block(
        rcv->num_blocks,
        syss,
        sigs,
        prns,
        num_prns,
        cfg,
        rcv->rf_ch
    );
    if (!blk) return -1;

    rcv->blocks[rcv->num_blocks++] = blk;
    return 0;
}





/*
 * ============================================================================
 * SATELLITE & ACQUISITION
 * ============================================================================
 */

acquisition_context_t *new_acquisition(
    double dop_min,
    double dop_step,
    int n_dop,
    int n_samples,
    float *corr_map
) {
    acquisition_context_t *acq = (acquisition_context_t *) calloc(sizeof(acquisition_context_t), 1);

    acq->dop_min = dop_min;
    acq->dop_step = dop_step;
    acq->n_dop = n_dop;

    acq->n_samples = n_samples;
    acq->corr_map = corr_map;

    return acq;
}

void free_acquisition(acquisition_context_t *acq) {
    if (!acq) return;
    // corr_map — часть общего буфера блока, не освобождаем
    free(acq);
}



satellite_channel_t *new_satellite(
    sys_t sys,
    sig_t sig,
    int prn,
    double dop_min,
    double dop_step,
    int n_dop,
    int n_samples,
    float *corr_map
) {
    satellite_channel_t *sat = (satellite_channel_t *) calloc(sizeof(satellite_channel_t), 1);
    
    sat->sys = sys;
    sat->sig = sig;
    sat->prn = prn;

    sat->code_len = get_code_len(sys, sig);

    sat->code = (int8_t *)malloc(sat->code_len * sizeof(int8_t));

    get_code(sys, sig, prn, (int8_t *) sat->code);

    sat->acq = new_acquisition(
        dop_min,
        dop_step,
        n_dop,
        n_samples,
        corr_map
    );

    return sat;
}

void free_satellite(satellite_channel_t *sat) {
    if (!sat) return;
    free_acquisition(sat->acq);
    free((void *)sat->code);
    free(sat);
}


satellite_channel_block_t *new_block(
    int id,
    sys_t *syss,
    sig_t *sigs,
    int *prns,
    int num_prns,
    const correlator_config_t *cfg,
    const RF_channel_t *rf_ch
) {
    satellite_channel_block_t *blk = (satellite_channel_block_t *) calloc(sizeof(satellite_channel_block_t), 1);

    blk->id = id;
    blk->config = *cfg;
    blk->rf_ch = (RF_channel_t *)rf_ch;

    int num_sats = num_prns > MAXCHPERBLOCK ? MAXCHPERBLOCK : num_prns;
    blk->num_channels = num_sats;

    int n_samples = cfg->n_samples;
    int n_dop = cfg->n_dop;
    int num_periods = cfg->num_periods;

    
    cudaError_t err;
    size_t cpx_bytes = (size_t)n_samples * sizeof(cuFloatComplex);


    //cufftHandle plan_fwd_tmp, plan_inv_tmp;
    if (cufftPlan1d(&blk->plan_fwd, n_samples, CUFFT_C2C, num_periods * n_dop) != CUFFT_SUCCESS) {
        fprintf(stderr, "cuFFT plan_fwd creation failed\n"); return NULL;
    }
    if (cufftPlan1d(&blk->plan_inv, n_samples, CUFFT_C2C, num_periods * num_sats * n_dop) != CUFFT_SUCCESS) {
        fprintf(stderr, "cuFFT plan_inv creation failed\n"); return NULL;
    }

    err = cudaMallocManaged((void **)&blk->d_codes_fft, cpx_bytes * num_sats, cudaMemAttachGlobal);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_codes_fft): %s\n", cudaGetErrorString(err)); return NULL; }

    err = cudaMallocManaged((void **)&blk->d_mixed, cpx_bytes * num_periods * n_dop, cudaMemAttachGlobal);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_mixed): %s\n", cudaGetErrorString(err)); return NULL; }

    err = cudaMallocManaged((void **)&blk->d_prod, cpx_bytes * num_periods * num_sats * n_dop, cudaMemAttachGlobal);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_prod): %s\n", cudaGetErrorString(err)); return NULL; }

    size_t accum_bytes = (size_t)num_sats * n_dop * n_samples * sizeof(float);
    err = cudaMallocManaged((void **)&blk->d_corr_maps, accum_bytes, cudaMemAttachGlobal);
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_corr_maps): %s\n", cudaGetErrorString(err)); return NULL; }

    // Allocate device phase steps buffer

    err = cudaMalloc((void **)&blk->d_phase_steps, n_dop * sizeof(double));
    if (err != cudaSuccess) { fprintf(stderr, "CUDA malloc (d_phase_steps): %s\n", cudaGetErrorString(err)); return NULL; }

    // Precompute phase steps for all Doppler bins
    double *h_steps = (double *)malloc(n_dop * sizeof(double));
    if (!h_steps) { fprintf(stderr, "Failed to allocate host phase steps\n"); return NULL; }
    for (int d = 0; d < n_dop; d++) {
        double f_if = G_L1_CARR - blk->rf_ch->f_lo;
        double f_curr = f_if + cfg->dop_min + d * cfg->dop_step;
        h_steps[d] = f_curr / blk->rf_ch->f_adc;
    }
    cudaMemcpy(blk->d_phase_steps, h_steps, n_dop * sizeof(double), cudaMemcpyHostToDevice);
    free(h_steps);


    for (int i = 0; i < num_sats; i++) {
        blk->channels[i] = new_satellite(
            syss[i],
            sigs[i],
            prns[i],
            blk->config.dop_min,
            blk->config.dop_step,
            n_dop,
            n_samples,
            blk->d_corr_maps + i*n_dop*n_samples
        );

        blk->channels[i]->parent = blk;
    }

    // === PREPROCESSING: calculate codes_fft for all satellites ===

    // Temporary device buffer for raw codes
    int8_t *d_codes = NULL;
    size_t codes_raw_bytes = (size_t)num_sats * G_L1CA_CLEN * sizeof(int8_t);
    err = cudaMalloc((void **)&d_codes, codes_raw_bytes);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA malloc (d_codes): %s\n", cudaGetErrorString(err));
        return NULL;
    }

    // Copy all PRN codes from satellite channels to GPU
    for (int i = 0; i < num_sats; i++) {
        size_t sat_code_bytes = G_L1CA_CLEN * sizeof(int8_t);
        err = cudaMemcpy(d_codes + i * G_L1CA_CLEN, blk->channels[i]->code,
                         sat_code_bytes, cudaMemcpyHostToDevice);
        if (err != cudaSuccess) {
            fprintf(stderr, "CUDA memcpy (codes, sat %d): %s\n", i, cudaGetErrorString(err));
            cudaFree(d_codes);
            return NULL;
        }
    }

    // Temporary buffer for sampled codes
    cuFloatComplex *d_codes_sampled = NULL;
    err = cudaMalloc((void **)&d_codes_sampled, cpx_bytes * num_sats);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA malloc (d_codes_sampled): %s\n", cudaGetErrorString(err));
        cudaFree(d_codes);
        return NULL;
    }

    // Temporary cuFFT plan for codes
    cufftHandle plan_code;
    if (cufftPlan1d(&plan_code, n_samples, CUFFT_C2C, num_sats) != CUFFT_SUCCESS) {
        fprintf(stderr, "cuFFT plan_code creation failed\n");
        cudaFree(d_codes_sampled);
        cudaFree(d_codes);
        return NULL;
    }

    // Sample all codes at ADC rate
    double chip_step = G_L1CA_CRATE / blk->rf_ch->f_adc;
    int sample_grid = (num_sats * n_samples + CUDA_BLOCK_SIZE - 1) / CUDA_BLOCK_SIZE;
    sample_code_batched_kernel<<<sample_grid, CUDA_BLOCK_SIZE>>>(
        d_codes, d_codes_sampled, num_sats, n_samples, n_samples,
        chip_step, G_L1CA_CLEN
    );

    // FFT all codes (batched cuFFT)
    cufftResult cufft_err = cufftExecC2C(plan_code,
                        d_codes_sampled,
                        blk->d_codes_fft,
                        CUFFT_FORWARD);
    if (cufft_err != CUFFT_SUCCESS) {
        fprintf(stderr, "cuFFT code forward failed (code %d)\n", cufft_err);
        cufftDestroy(plan_code);
        cudaFree(d_codes_sampled);
        cudaFree(d_codes);
        return NULL;
    }

    // Conjugate all codes in-place
    int conj_grid = (num_sats * n_samples + CUDA_BLOCK_SIZE - 1) / CUDA_BLOCK_SIZE;
    conjugate_kernel<<<conj_grid, CUDA_BLOCK_SIZE>>>(
        blk->d_codes_fft, num_sats * n_samples
    );

    // Check for kernel errors
    cudaError_t kerr = cudaGetLastError();
    if (kerr != cudaSuccess) {
        fprintf(stderr, "CUDA kernel error in code preprocessing: %s\n", cudaGetErrorString(kerr));
        cufftDestroy(plan_code);
        cudaFree(d_codes_sampled);
        cudaFree(d_codes);
        return NULL;
    }

    // Initialize mixing LUT for mixer kernel
    init_mix_lut_gpu();

    // Free temporary resources
    cufftDestroy(plan_code);
    cudaFree(d_codes_sampled);
    cudaFree(d_codes);

    return blk;
}

void free_block(satellite_channel_block_t *blk) {
    if (!blk) return;

    for (int i = 0; i < blk->num_channels; i++) {
        free_satellite(blk->channels[i]);
    }

    cufftDestroy(blk->plan_fwd);
    cufftDestroy(blk->plan_inv);

    cudaFree(blk->d_codes_fft);
    cudaFree(blk->d_mixed);
    cudaFree(blk->d_prod);
    cudaFree(blk->d_phase_steps);
    cudaFree(blk->d_corr_maps);

    free(blk);
}
