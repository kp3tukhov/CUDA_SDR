/*
 * Frequency Mixing and Complex Vector Operations
 * 
 * This header provides functions for frequency mixing (down-conversion)
 * and complex vector operations used in GPS signal processing
 * Includes optimized phase lookup table (LUT) for carrier mixing
 */

#ifndef DSP_MIXER_H
#define DSP_MIXER_H

#include <fftw3.h>
#include <complex.h>
#include "core/types.h"


/*
 * Mix input signal with a complex exponential at specified frequency
 * Uses a precomputed phase lookup table for efficient carrier generation
 * Warning: quite low precision
 *
 * Parameters:
 *   signal_in  - Input signal buffer (complex samples)
 *   signal_out - Output buffer for mixed signal
 *   size       - Number of samples to process
 *   freq       - Mixing frequency (Hz)
 *   recv       - Receiver configuration (for sampling rate)
 */
void mix_freq(
    const float complex *signal_in,
    float complex *signal_out,
    int size,
    double freq,
    const receiver_t *recv
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
