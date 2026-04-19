/*
 * Frequency Mixing and Complex Vector Operations
 * 
 * This header provides functions for frequency mixing (down-conversion)
 * and complex vector operations used in GPS signal processing
 * Includes cached lookup table (LUT) for fast carrier mixing
 */

#ifndef DSP_MIXER_H
#define DSP_MIXER_H

/*
 * Mix input signal with a complex exponential at specified frequency
 * Uses a precomputed mixing lookup table for efficient carrier generation
 *
 * Parameters:
 *   raw        - Input signal buffer, raw bit-packed 8(4+4)
 *   mixed      - Output buffer for mixed signal
 *   fft_size   - Total output size for FFT (zero-padded to this value)
 *   N          - Number of samples to process
 *   phase_step - Phase increment per sample (freq/f_adc)
 */
void mix_freq(
    const uint8_t *raw,
    float complex *mixed,
    int fft_size,
    int N,
    double phase_step
);

/*
 * Element-wise complex vector multiplication
 * Computes c[i] = a[i] * b[i] for all elements
 *
 * Parameters:
 *   size - Number of elements
 *   a    - First input vector
 *   b    - Second input vector
 *   c    - Output vector (can alias a or b)
 */
void cpx_vec_mul(
    int size,
    const float complex *a,
    const float complex *b,
    float complex *c
);

/*
 * Complex dot product of two vectors
 * Computes sum(a[i] * b[i]) for all elements
 *
 * Parameters:
 *   size - Number of elements
 *   a    - First input vector
 *   b    - Second input vector
 *
 * Returns:
 *   Complex dot product result
 */
float complex cpx_dot_product(
    int size,
    const float complex *a,
    const float complex *b
);

#endif /* DSP_MIXER_H */
