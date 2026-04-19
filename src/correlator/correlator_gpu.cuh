/*
 * CUDA Batch Correlator Interface (GPU)
 *
 * This header provides the host interface for the CUDA-based batch correlator
 * for GPS signal processing on the GPU. It performs frequency-domain correlation
 * across multiple satellites, Doppler bins, and code periods with non-coherent
 * power accumulation
 */

#ifndef CORR_CORRELATOR_GPU_CUH
#define CORR_CORRELATOR_GPU_CUH

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Kernel: batched power accumulation across multiple periods, satellites, and Doppler bins
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
    float *d_accum,
    int batch_p,
    int num_sats,
    int n_dop,
    int N,
    int fft_size
);

/*
 * Run CUDA batch correlation for one satellite channel block
 * Parallel code-domain search: mixes the signal per Doppler bin and period, FFTs, multiplies
 * by cached code spectra, inverse FFTs, and accumulates non-coherent power into device maps
 *
 * Parameters:
 *   blk         - Block with RF buffer, per-satellite codes, cuFFT plans, and GPU scratch buffers
 *   num_periods - Number of code periods in the batch (same dimension as the mixing / FFT batch)
 */
int correlate_block_gpu(
    satellite_channel_block_t *blk,
    int num_periods
);

#ifdef __cplusplus
}
#endif

#endif /* CORR_CORRELATOR_GPU_CUH */
