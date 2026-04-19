/*
 * Correlation Functions for GPS Signal Processing Implementation
 * 
 * This module provides three correlation methods:
 * - Sequential time-domain correlation
 * - Parallel frequency-domain (FFT-based) correlation
 * - Parallel code-domain (FFT-based) correlation
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <fftw3.h>
#include <complex.h>

#include "core/params.h"
#include "core/types.h"
#include "correlator/correlator.h"
#include "dsp/code.h"
#include "dsp/mixer.h"


void corr_sequential(
    const uint8_t *raw,
    const int8_t *local_code,
    int code_len,
    int N,
    double chiprate,
    double carrier,
    const RF_channel_t *rf_ch,
    const correlator_config_t *cfg,
    float *corr_map
) {
    
    double chip_step = chiprate / rf_ch->f_adc;


    float complex *code_buf = (float complex *)malloc(sizeof(float complex) * N);
    float complex *sig_buf = (float complex *)malloc(sizeof(float complex) * N);

    // Search code start chip
    for (int c = 0; c < code_len; c++) {

        // Make ADC code representation (starting with chip c)
        sample_code(local_code, (fftwf_complex *)code_buf, N, chip_step, code_len, N, c);

        // Search Doppler offset
        for (int d = 0; d < cfg->n_dop; d++) {

            // Current frequency guess: IF + Doppler
            double f_if = carrier - rf_ch->f_lo;
            double f_curr = f_if + cfg->dop_min + d * cfg->dop_step;

            mix_freq(raw, sig_buf, N, N, f_curr/rf_ch->f_adc);

            float complex sum = cpx_dot_product(N, sig_buf, code_buf);

            float re = crealf(sum);
            float im = cimagf(sum);
            
            //int c_offset = c;                                     // If so, res is code start phase
            int c_offset = (code_len - c) % code_len;       // If so, res is code offset

            // Compute power (with accumulation support)
            corr_map[d * code_len + c_offset] += re * re + im * im;
        }
    }
    free(code_buf);
    free(sig_buf);
}



void corr_parallel_freq(
    const uint8_t *raw,
    const int8_t *local_code,
    int code_len,
    int N,
    double chiprate,
    double carrier,
    const RF_channel_t *rf_ch,
    const correlator_config_t *cfg,
    float *corr_map
) {

    double chip_step = chiprate / rf_ch->f_adc;

    int fft_size = N;  // TODO: zero padding

    // Buffers
    fftwf_complex *in = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fftwf_complex *out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    float complex *signal = (float complex *) malloc(sizeof(float complex) * fft_size);
    float complex *code_buf = (float complex *)malloc(sizeof(float complex) * fft_size);

    if (!in || !out || !signal || !code_buf) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    fftwf_plan plan = fftwf_plan_dft_1d(fft_size, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    if (!plan) {
        fprintf(stderr, "FFT Plan creation failed\n");
        return;
    }

    for (int i = 0; i < N; i++) {
        signal[i] = (float)(raw[i]&0xF) + I * (float)(raw[i]>>4);
    }
    for (int i = N; i < fft_size; i++) {
        signal[i] = 0.0f + I*0.0f;
    }

    int f_sign = rf_ch->iq ? -1 : 1;
    double df = rf_ch->f_adc / fft_size;

    // Search code start chip
    for (int c = 0; c < code_len; c++) {

        // Make ADC code representation (starting with chip c)
        sample_code(local_code, (fftwf_complex *)code_buf, fft_size, chip_step, code_len, N, c);

        cpx_vec_mul(N, signal, code_buf, (float complex *)in);

        for (int i = N; i < fft_size; i++) {
            in[i][0] = 0.0f;
            in[i][1] = 0.0f;
        }

        // Forward FFT of sig*code
        fftwf_execute(plan);

        double f_if = carrier - rf_ch->f_lo;

        // Search Doppler offset
        for (int d = 0; d < cfg->n_dop; d++) {

            // Current frequency guess: IF + Doppler
            double f_curr = f_sign*(f_if + cfg->dop_min + d * cfg->dop_step);

            int freq_bin = (int)round(f_curr / df);
            while (freq_bin < 0) freq_bin += fft_size;
            while (freq_bin >= fft_size) freq_bin -= fft_size;

            float re = out[freq_bin][0];
            float im = out[freq_bin][1];

            //int c_offset = c;                                     // If so, res is code start phase
            int c_offset = (code_len - c) % code_len;       // If so, res is code offset

            // Compute power (with accumulation support)
            corr_map[d * code_len + c_offset] += re * re + im * im;
        }
    }

    fftwf_destroy_plan(plan);

    fftwf_free(in);
    fftwf_free(out);
    free(code_buf);
}


void corr_parallel_code(
    const uint8_t *raw,
    const int8_t *local_code,
    int code_len,
    int N,
    double chiprate,
    double carrier,
    const RF_channel_t *rf_ch,
    const correlator_config_t *cfg,
    float *corr_map
) {

    double chip_step = chiprate / rf_ch->f_adc;

    int fft_size = N;  // TODO: zero padding
    
    // Buffers
    fftwf_complex *sig_buf = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fftwf_complex *code_buf = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fftwf_complex *code_fft = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fftwf_complex *prod_fft = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fftwf_complex *corr_ifft = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    
    if (!sig_buf || !code_buf || !code_fft || !prod_fft || !corr_ifft) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    fftwf_plan plan_fwd = fftwf_plan_dft_1d(fft_size, sig_buf, prod_fft, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_plan plan_inv = fftwf_plan_dft_1d(fft_size, prod_fft, corr_ifft, FFTW_BACKWARD, FFTW_ESTIMATE);

    if (!plan_fwd || !plan_inv) {
        fprintf(stderr, "FFT Plan creation failed\n");
        return;
    }
    
    // Get FFT of properly sampled code
    sample_code(local_code, code_buf, fft_size, chip_step, code_len, N, 0.0);
    gen_code_fft(code_buf, code_fft, fft_size);
    fftwf_free(code_buf);

    double f_if = carrier - rf_ch->f_lo;
    // Search Doppler offset
    for (int d = 0; d < cfg->n_dop; d++) {

        // Current frequency guess: IF + Doppler
        double f_curr = f_if + cfg->dop_min + d * cfg->dop_step;

        mix_freq(raw, (float complex *)sig_buf, fft_size, N, f_curr/rf_ch->f_adc);
    
        // Forward FFT of signal
        fftwf_execute(plan_fwd);
        
        // Frequency-domain multiplication
        cpx_vec_mul(fft_size, (float complex *)prod_fft, (float complex *)code_fft, (float complex *)prod_fft);
        
        // Inverse FFT
        fftwf_execute(plan_inv);
        

        // Search code offset (samples)
        for (int s = 0; s < N; s++) {
            float re = corr_ifft[s][0] / fft_size/fft_size;
            float im = corr_ifft[s][1] / fft_size/fft_size;

            int c_offset = s;               // If so, res is code offset
            //int c_offset = (N - s) % Т;       // If so, res is code start phase

            // Compute power (with accumulation support)
            corr_map[d * N + c_offset] += re * re + im * im;
            
        }
    }

    fftwf_destroy_plan(plan_fwd);
    fftwf_destroy_plan(plan_inv);

    fftwf_free(sig_buf);
    fftwf_free(code_fft);
    fftwf_free(prod_fft);
    fftwf_free(corr_ifft);
    
}
