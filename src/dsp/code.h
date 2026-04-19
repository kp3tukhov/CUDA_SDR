/*
 * CDMA Code Processing
 * 
 * This header provides functions for sampling codes
 * for FFT-based processing, and frequency-domain code preparation
 */

#ifndef DSP_CODE_H
#define DSP_CODE_H

/*
 * Sample the code for FFT processing
 * Maps code chips to ADC sample rate with proper interpolation
 *
 * Parameters:
 *   code         - Code sequence (+1/-1 values)
 *   code_sampled - Output buffer for sampled code (complex)
 *   fft_size     - FFT size (may be larger than code length for zero-padding)
 *   chip_step    - Code phase increment per ADC sample
 *   code_len     - Length of the code sequence (chips)
 *   N            - Number of valid samples in one code period
 *   coff         - Code phase offset in chips
 */
void sample_code(
    const int8_t *code,
    fftwf_complex *code_sampled,
    int fft_size,
    double chip_step,
    int code_len,
    int N,
    double coff
);

/*
 * Generate frequency-domain representation of the code
 * Computes FFT of the sampled code and applies complex conjugation
 * for correlation processing
 *
 * Parameters:
 *   code_sampled - Time-domain sampled code (complex)
 *   code_fft     - Output frequency-domain representation
 *   fft_size     - FFT size
 */
void gen_code_fft(
    fftwf_complex *code_sampled,
    fftwf_complex *code_fft,
    int fft_size
);

#endif /* DSP_CODE_H */
