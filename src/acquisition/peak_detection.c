/*
 * Peak Detection and Refinement Implementation
 * 
 * This module provides functions for finding correlation peaks and
 * refining Doppler frequency estimates using parabolic interpolation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core/params.h"
#include "core/types.h"
#include "acquisition/peak_detection.h"
#include "acquisition/cn0_estimator.h"


/*
 * Converts correlation sample index to code phase in chips
 */
static inline double sample_to_chip_phase(int sample_idx, int n_samples) {
    if (n_samples <= 0) return 0.0;

    double chips_per_sample = (double)GPS_CODE_LEN / (double)n_samples;
    double phase = (double)sample_idx * chips_per_sample;

    while (phase >= GPS_CODE_LEN) {
        phase -= GPS_CODE_LEN;
    }

    return phase;
}


/*
 * Parabolic interpolation for one dimension
 * Fits a parabola through three points and finds the vertex
 */
static double parabolic_interp_1d(
    double y_minus,
    double y_center,
    double y_plus,
    double *offset
) {
    /* Parabola formula: y = a*x^2 + b*x + c
     * a = (y_minus − 2y_center + y_plus)/2
     * b = (y_plus − y_minus)/2
     * Vertex at x = -b/(2a) */
    double a2 = y_minus - 2.0 * y_center + y_plus;

    if (fabs(a2) < 1e-10) {
        // Degenerate parabola (points are collinear)
        *offset = 0.0;
        return y_center;
    }

    // Offset of vertex relative to center point
    *offset = 0.5 * (y_minus - y_plus) / a2;

    // Interpolated value at vertex
    double interp_value = y_center - 0.25 * (y_minus - y_plus) * (*offset);

    return interp_value;
}


void find_correlation_peak(
    const double *corr_map,
    int n_doppler,
    int n_samples,
    double doppler_min,
    double doppler_step,
    correlation_peak_t *peak
) {
    if (!corr_map || !peak || n_doppler <= 0 || n_samples <= 0) {
        return;
    }

    // Initialize peak structure
    peak->doppler_idx = 0;
    peak->code_sample = 0;
    peak->power = corr_map[0];
    peak->doppler_freq = doppler_min;
    peak->code_phase_chips = 0.0;

    // Global maximum search
    double max_power = 0.0;
    int max_doppler_idx = 0;
    int max_sample_idx = 0;

    for (int d = 0; d < n_doppler; d++) {
        for (int s = 0; s < n_samples; s++) {
            double power = corr_map[d * n_samples + s];
            if (power > max_power) {
                max_power = power;
                max_doppler_idx = d;
                max_sample_idx = s;
            }
        }
    }

    // Fill result structure
    peak->doppler_idx = max_doppler_idx;
    peak->code_sample = max_sample_idx;
    peak->power = max_power;

    // Compute Doppler frequency
    peak->doppler_freq = doppler_min + max_doppler_idx * doppler_step;

    // Compute code phase in chips
    peak->code_phase_chips = sample_to_chip_phase(max_sample_idx, n_samples);
}


void fine_doppler(
    const double *corr_map,
    int n_doppler,
    int n_samples,
    int peak_idx,
    int sample_idx,
    double doppler_min,
    double doppler_step,
    double *refined_doppler
) {
    if (!corr_map || !refined_doppler) {
        return;
    }

    // Base values
    double base_doppler = doppler_min + peak_idx * doppler_step;
    double base_power = corr_map[peak_idx * n_samples + sample_idx];

    // Doppler interpolation (3 points vertically)
    int doppler_prev = (peak_idx > 0) ? peak_idx - 1 : peak_idx;
    int doppler_next = (peak_idx < n_doppler - 1) ? peak_idx + 1 : peak_idx;

    double y_minus = corr_map[doppler_prev * n_samples + sample_idx];
    double y_center = base_power;
    double y_plus = corr_map[doppler_next * n_samples + sample_idx];

    double doppler_offset = 0.0;
    parabolic_interp_1d(y_minus, y_center, y_plus, &doppler_offset);

    // Refined Doppler frequency
    *refined_doppler = base_doppler + doppler_offset * doppler_step;
}
