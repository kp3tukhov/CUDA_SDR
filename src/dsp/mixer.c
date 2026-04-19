/*
 * Frequency Mixing and Complex Vector Operations Implementation
 * 
 * This module provides functions for frequency mixing (down-conversion)
 * and complex vector operations used in GPS signal processing
 * Includes cached lookup table (LUT) for carrier mixing
 */

#include <math.h>
#include <stdint.h>
#include <complex.h>

#include <stdio.h>
#include "core/params.h"
#include "core/types.h"
#include "dsp/mixer.h"


#define PHASE_LUT_SIZE 256  // 2^8
#define RAW_SIZE 256        // uint8_t max val


static float complex mix_lut[RAW_SIZE][PHASE_LUT_SIZE];   // Cached values for all possible raw values (2^8)
static int lut_initialized = 0;


// Unpack raw 8(4+4) to float complex
static inline float complex raw_unpack(uint8_t val) {
    return (float)(val & 0xF) + I * (float)(val >> 4);
}

// Initialize mixing lookup table
static inline void init_mix_lut(void) {
    if (lut_initialized) return;

    for (int i = 0; i < PHASE_LUT_SIZE; i++) {
        double phase = 2.0 * M_PI * i / PHASE_LUT_SIZE;
        for (int j = 0; j < RAW_SIZE; j++) {
            mix_lut[j][i] = raw_unpack((uint8_t)j)*(cosf((float)phase) + I * sinf((float)phase));
        }
    }

    lut_initialized = 1;
}


void mix_freq(
    const uint8_t *raw,
    float complex *mixed,
    int fft_size,
    int N,
    double phase_step
) {
    init_mix_lut();

    // Phase increment per sample in LUT indices (rescaled by 2^12)
    uint32_t phase_step_lut = (uint32_t)(int)(phase_step * (PHASE_LUT_SIZE << 12));

    int i = 0;
    for (; i < N; i++) {
        uint8_t current_phase = (uint8_t)((phase_step_lut * i) >> 12); // Overflow wraps phase automatically
        
        mixed[i] = mix_lut[raw[i]][current_phase];  // Get value from LUT
    }
    for(; i < fft_size; i++) {
        mixed[i] = 0.0f + I * 0.0f;
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