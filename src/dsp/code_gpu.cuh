/*
 * CUDA Code Sampling and FFT Preparation (GPU)
 *
 * This header provides CUDA kernel declarations for GPS code
 * sampling, FFT preparation, and conjugation used in GPS signal
 * processing on the GPU
 */

#ifndef DSP_CODE_GPU_CUH
#define DSP_CODE_GPU_CUH

#include <cuComplex.h>
#include <cufft.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CUDA Kernel: Sample code chips to ADC rate
 *
 * Parameters:
 *   code         - Generated code sequence (+1/-1 values)
 *   code_sampled - Output buffer for sampled code (complex)
 *   fft_size     - FFT size (may be larger than code length for zero-padding)
 *   chip_step    - Code phase increment per ADC sample
 *   code_len     - Length of the code sequence (chips)
 *   N            - Number of valid samples in one code period
 *   coff         - Code phase offset in chips
 */
__global__ void sample_code_kernel(
    const int8_t *code,
    cuFloatComplex *code_sampled,
    int fft_size,
    double chip_step,
    int code_len,
    int N,
    double coff
);

/*
 * CUDA Kernel: Complex conjugate (in-place)
 * Negates the imaginary part of each complex element
 *
 * Parameters:
 *   data - Input/output complex array (modified in-place)
 *   total_size - Number of elements to conjugate
 */
__global__ void conjugate_kernel(
    cuFloatComplex *data,
    int total_size
);

/*
 * CUDA Kernel: Sample codes for all satellites in one launch
 * Maps code chips (+1/-1) to ADC-rate complex samples.
 * Each thread processes one sample for one satellite.
 *
 * Parameters:
 *   d_codes    - [num_sats x GPS_CODE_LEN] input code sequences
 *   d_out      - [num_sats x fft_size] output sampled codes
 *   num_sats   - number of satellites
 *   N          - number of valid samples in one code period
 *   fft_size   - FFT size (with possible zero-padding)
 *   chip_step  - code phase increment per ADC sample
 *   code_len   - length of the code sequence (chips)
 */
__global__ void sample_code_batched_kernel(
    const int8_t *d_codes,
    cuFloatComplex *d_out,
    int num_sats,
    int N,
    int fft_size,
    double chip_step,
    int code_len
);

#ifdef __cplusplus
}
#endif

#endif /* DSP_CODE_GPU_CUH */
