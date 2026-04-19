/*
 * GPS L1 C/A spreading-code generation
 *
 * This module implements the 10-bit G1 and G2 LFSRs, the IS-GPS-200 G2 delay table per PRN,
 * XOR combination into the C/A sequence, and NRZ mapping to +1/-1 samples in gen_code_L1CA
 */

#include <stdint.h>

#include "core/params.h"
#include "code/code_gps.h"


// G2 delay values for PRN 1-37 (chips)
static const uint16_t L1CA_G2_delay[] = {
       5,   6,   7,   8,  17,  18, 139, 140, 141, 251, 252, 254, 255, 256, 257,
     258, 469, 470, 471, 472, 473, 474, 509, 512, 513, 514, 515, 516, 859, 860,
     861, 862, 863, 950, 947, 948, 950
};

/*
 * LFSR Register Layout (10 bits):
 * ==========================================================
 * new -> || 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 || -> out
 * ==========================================================
 * Bit Indexing:
 * ==========================================================
 * new -> || 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 |  0 || -> out
 * ==========================================================
 */

 /*
 * Performs one step of G1 linear feedback shift register
 */
static inline uint16_t lfsr_step_g1(uint16_t state) {
    // G1 polynomial: x^10 + x^3 + 1
    // Taps at bits 0 and 7
    uint16_t bit = (state ^ (state >> 7)) & 1;
    return (state >> 1) | (bit << 9);
}

/*
 * Performs one step of G2 linear feedback shift register
 */
static inline uint16_t lfsr_step_g2(uint16_t state) {
    // G2 polynomial: x^10 + x^9 + x^8 + x^6 + x^3 + x^2 + 1
    // Taps at bits: 0, 1, 2, 4, 7, 8
    uint16_t bit = (state ^ (state >> 1) ^ (state >> 2) ^ (state >> 4) ^ (state >> 7) ^ (state >> 8)) & 1;
    return (state >> 1) | (bit << 9);
}



void gen_code_L1CA(
    int prn,
    int8_t *buffer,
    int n_samples,
    int start_chip
) {

    if (!buffer || prn < 1 || prn > MAX_SATS) return;

    // Initialize registers to all-ones state
    uint16_t g1 = 0x3FF;
    uint16_t g2 = 0x3FF;

    // Apply G2 delay for specific PRN
    uint16_t shift = G_L1CA_CLEN - L1CA_G2_delay[prn - 1];
    for (int i = 0; i < shift; i++) {
        g2 = lfsr_step_g2(g2);
    }

    // Skip to starting chip position
    int total_skip = (start_chip % G_L1CA_CLEN);
    for (int i = 0; i < total_skip; i++) {
        g1 = lfsr_step_g1(g1);
        g2 = lfsr_step_g2(g2);
    }

    // Generate PRN sequence
    for (int i = 0; i < n_samples; i++) {

        // Extract output bits (LSB) from each register
        int bit1 = g1 & 1;
        int bit2 = g2 & 1;

        // Convert to polar NRZ: XOR becomes multiplication
        buffer[i] = (bit1 == bit2) ? 1 : -1;

        // Advance both registers
        g1 = lfsr_step_g1(g1);
        g2 = lfsr_step_g2(g2);
    }
}