/*
 * Frequency Mixing and Complex Vector Operations Implementation
 * 
 * This module provides functions for frequency mixing (down-conversion)
 * and complex vector operations used in GPS signal processing
 * Includes optimized phase lookup table (LUT) for carrier mixing
 */

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <complex.h>
#include "core/params.h"
#include "core/types.h"
#include "dsp/mixer.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define PHASE_LUT_SIZE 65536  // 2^16

static float complex phase_lut[PHASE_LUT_SIZE];
static int lut_initialized = 0;


// Initialize phase lookup table
static inline void init_phase_lut(void) {
    if (lut_initialized) return;

    for (int i = 0; i < PHASE_LUT_SIZE; i++) {
        double phase = 2.0 * M_PI * i / PHASE_LUT_SIZE;
        phase_lut[i] = cosf((float) phase) + I * sinf((float) phase);
    }
    lut_initialized = 1;
}


void mix_freq(
    const float complex *signal_in,
    float complex *signal_out,
    int size,
    double freq,
    const receiver_t *recv
) {
    init_phase_lut();

    // Phase increment per sample (scaled to LUT indices)
    uint16_t phase_step_lut = (uint16_t) round((freq / recv->f_adc) * PHASE_LUT_SIZE);
    uint16_t current_phase = 0;

    for (int i = 0; i < size; i++) {
        signal_out[i] = signal_in[i]*phase_lut[current_phase];

        current_phase += phase_step_lut; // Overflow wraps phase automatically

    }
}


void cpx_vec_mul(
    int size,
    const float complex *a,
    const float complex *b,
    float complex *c
) {
    for (int i = 0; i < size; i++) {
        c[i] = a[i] * b[i];
    }
}

float complex cpx_dot_product(
    int size,
    const float complex *a,
    const float complex *b
) {
    float complex s = 0;

    for (int i = 0; i < size; i++) {
        s += a[i]*b[i];
    }

    return s;
}