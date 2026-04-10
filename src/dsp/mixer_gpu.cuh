/*
 * CUDA Frequency Mixing and Complex Vector Operations (GPU)
 *
 * This header provides CUDA kernel declarations for frequency
 * mixing (down-conversion) and complex vector operations used in
 * GPS signal processing on the GPU
 * Includes cached phase lookup table (LUT) for carrier mixing
 */

#ifndef DSP_MIXER_GPU_H
#define DSP_MIXER_GPU_H

#include <cuComplex.h>

#define PHASE_LUT_SIZE 65536  // 2^16

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize the phase lookup table on GPU
 * Must be called once before using mix_freq_kernel from other modules
 */
void init_phase_lut_gpu(void);

/*
 * CUDA Kernel: Frequency mixing with a complex exponential
 *
 * Parameters:
 *   size           - Number of samples to process
 *   signal_in      - Input signal buffer (complex samples)
 *   signal_out     - Output buffer for mixed signal
 *   phase_step_lut - Phase increment per sample (scaled to LUT indices)
 */
__global__ void mix_freq_kernel(
    int size,
    const cuFloatComplex *signal_in,
    cuFloatComplex *signal_out,
    uint16_t phase_step_lut
);

/*
 * CUDA Kernel: Element-wise complex vector multiplication
 * Computes c[i] = a[i] * b[i] for all elements
 *
 * Parameters:
 *   size - Number of elements
 *   a    - First input vector
 *   b    - Second input vector
 *   c    - Output vector
 */
__global__ void cpx_vec_mul_kernel(
    int size,
    const cuFloatComplex *a,
    const cuFloatComplex *b,
    cuFloatComplex *c
);

/*
 * CUDA Kernel: Parallel complex array reduction (sum)
 * Sums all complex elements using shared memory block-level reduction
 * followed by atomic add for the final result
 *
 * Parameters:
 *   size   - Number of elements in input array
 *   input  - Input complex array
 *   output - Output single complex value (accumulated sum)
 */
__global__ void cpx_sum_kernel(
    int size,
    const cuFloatComplex *input,
    cuFloatComplex *output
);

/*
 * CUDA Kernel: Frequency mixing for all Doppler bins across multiple periods
 * Each thread processes one (period, doppler_bin, sample) tuple
 *
 * Parameters:
 *   d_signal       - [num_periods × N] input signal for all periods
 *   d_mixed        - [num_periods × n_dop × fft_size] output mixed signal
 *   N              - number of samples in each period
 *   fft_size       - FFT size
 *   n_dop          - number of Doppler bins
 *   num_periods    - number of periods to process in one batch
 *   d_phase_steps  - [n_dop] phase steps for each Doppler bin
 */
__global__ void mix_batch_kernel(
    const cuFloatComplex *d_signal,
    cuFloatComplex *d_mixed,
    int N,
    int fft_size,
    int n_dop,
    int num_periods,
    const uint16_t *d_phase_steps
);

/*
 * CUDA Kernel: Multiply mixed-signal FFT with code FFT for all periods, satellites and Doppler bins
 * Each thread processes one (period, sat, doppler, sample) tuple
 *
 * Parameters:
 *   d_mixed_fft  - [num_periods × n_dop × fft_size] mixed signal FFT
 *   d_codes_fft  - [num_sats × fft_size] code FFTs for each satellite
 *   d_prod       - [num_periods × num_sats × n_dop × fft_size] output product
 *   num_sats     - number of satellites
 *   n_dop        - number of Doppler bins
 *   fft_size     - FFT size
 *   num_periods  - number of periods to process in one batch
 */
__global__ void multiply_batch_kernel(
    const cuFloatComplex *d_mixed_fft,
    const cuFloatComplex *d_codes_fft,
    cuFloatComplex *d_prod,
    int num_sats,
    int n_dop,
    int fft_size,
    int num_periods
);

#ifdef __cplusplus
}
#endif

#endif /* DSP_MIXER_GPU_H */
