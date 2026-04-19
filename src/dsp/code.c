/*
 * CDMA Code Generation and Processing Implementation
 * 
 * This module provides functions for generating GPS L1 C/A Gold codes
 * using G1/G2 linear feedback shift registers (LFSR), sampling codes
 * for FFT-based processing, and frequency-domain code preparation
 */

#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include <fftw3.h>
#include <complex.h>

#include "core/params.h"
#include "core/types.h"
#include "dsp/code.h"


void sample_code(
    const int8_t *code,
    fftwf_complex *code_sampled,
    int fft_size,
    double chip_step,
    int code_len,
    int N,
    double coff
) {

    double curr_chip = coff;

    // Fill buffer with sampled code values
    int i = 0;
    for (; i < N; i++) {
        // Write to I-channel, Q = 0
        code_sampled[i][0] = (float)code[(int)curr_chip];
        code_sampled[i][1] = 0.0f;

        curr_chip += chip_step;
        if (curr_chip >= code_len) curr_chip -= code_len;
    }
    for (; i < fft_size; i++) {
        // Zero padding to FFT size
        code_sampled[i][0] = 0.0f;
        code_sampled[i][1] = 0.0f;
    }
}

void gen_code_fft(
    fftwf_complex *code_sampled,
    fftwf_complex *code_fft,
    int fft_size
) {

    fftwf_plan plan = fftwf_plan_dft_1d(fft_size, code_sampled, code_fft, FFTW_FORWARD, FFTW_ESTIMATE);

    if (!plan) {
        fprintf(stderr, "FFT Plan creation failed in gen_code_fft\n");
        return;
    }

    fftwf_execute(plan);
    fftwf_destroy_plan(plan);

    // Complex conjugate of spectrum C*(f) for correlation
    for (int i = 0; i < fft_size; i++) {
        code_fft[i][1] = -code_fft[i][1];
    }
}
