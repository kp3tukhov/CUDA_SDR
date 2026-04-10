/*
 * CUDA Code Sampling and FFT Preparation (GPU)
 *
 * This module provides CUDA kernel implementations for GPS code
 * sampling, FFT preparation, and conjugation used in GPS signal
 * processing on the GPU
 */

#include <cuda_runtime.h>
#include <cuComplex.h>
#include <cufft.h>
#include <math.h>
#include <stdio.h>

#include "core/params.h"
#include "core/types.h"


#ifdef __cplusplus
extern "C" {
#endif


__global__ void sample_code_kernel(
    const int8_t *code,
    cuFloatComplex *code_sampled,
    int fft_size,
    double chip_step,
    int code_len,
    int N,
    double coff
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= fft_size) return;

    double curr_chip = coff + fmod((double)i * chip_step, (double)code_len);
    int chip_idx = (int)curr_chip;

    // Fill buffer with sampled code values
    if (i < N) {
        // Write to I-channel, Q = 0
        code_sampled[i] = make_cuFloatComplex((float)code[chip_idx], 0.0f);
    } else {
        // Zero padding to FFT size
        code_sampled[i] = make_cuFloatComplex(0.0f, 0.0f);
    }
}


__global__ void conjugate_kernel(
    cuFloatComplex *data,
    int total_size
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < total_size) {
        data[idx].y = -data[idx].y;
    }
}


__global__ void sample_code_batched_kernel(
    const int8_t *d_codes,
    cuFloatComplex *d_out,
    int num_sats,
    int N,
    int fft_size,
    double chip_step,
    int code_len
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total = num_sats * fft_size;
    if (idx >= total) return;

    int sat = idx / fft_size;
    int i = idx % fft_size;

    // Fill buffer with sampled code values
    if (i < N) {
        // Write to I-channel, Q = 0
        double curr_chip = fmod((double)i * chip_step, (double)code_len);
        int chip_idx = (int)curr_chip;

        d_out[idx] = make_cuFloatComplex((float)d_codes[sat * code_len + chip_idx], 0.0f);
    } else {
        // Zero padding to FFT size
        d_out[idx] = make_cuFloatComplex(0.0f, 0.0f);
    }
}


#ifdef __cplusplus
}
#endif
