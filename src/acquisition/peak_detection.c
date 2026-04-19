/*
 * Peak Detection and Refinement Implementation
 * 
 * This module provides functions for finding correlation peaks and
 * refining Doppler frequency estimates using parabolic interpolation.
 */

#include <math.h>

#include "core/params.h"
#include "core/types.h"
#include "acquisition/peak_detection.h"
#include "code/getcode.h"


/*
 * Converts correlation sample index to code phase in chips
 */
static inline double sample_to_chip_phase(int sample_idx, int n_samples, int code_len) {
    if (n_samples <= 0) return 0.0;

    double chips_per_sample = (double)code_len / (double)n_samples;
    double phase = (double)sample_idx * chips_per_sample;

    while (phase >= code_len) {
        phase -= code_len;
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
    satellite_channel_t *sat
) {
    if (!sat || !sat->acq || !sat->acq->corr_map) {
        return;
    }

    acquisition_context_t *acq = sat->acq;

    // Global maximum search
    double max_power = 0.0;
    int max_doppler_idx = 0;
    int max_sample_idx = 0;

    for (int d = 0; d < acq->n_dop; d++) {
        for (int s = 0; s < acq->n_samples; s++) {
            double power = acq->corr_map[d * acq->n_samples + s];
            if (power > max_power) {
                max_power = power;
                max_doppler_idx = d;
                max_sample_idx = s;
            }
        }
    }

    // Fill result structure
    acq->dop_idx = max_doppler_idx;
    acq->code_sample = max_sample_idx;
    acq->power = max_power;

    // Compute Doppler frequency
    sat->fdoppler = acq->dop_min + acq->dop_idx * acq->dop_step;
 
    // Compute code phase in chips
    int code_len = get_code_len(sat->sys, sat->sig);
    sat->code_phase_start_chips = sample_to_chip_phase(max_sample_idx, acq->n_samples, code_len);
}


double fine_doppler(
    acquisition_context_t *acq
) {
    if (!acq || !acq->corr_map) {
        return 0;
    }

    // Base values
    double base_doppler = acq->dop_min + acq->dop_idx * acq->dop_step;
    double base_power = acq->corr_map[acq->dop_idx * acq->n_samples + acq->code_sample];

    // Doppler interpolation (3 points vertically)
    int doppler_prev = (acq->dop_idx > 0) ? acq->dop_idx - 1 : acq->dop_idx;
    int doppler_next = (acq->dop_idx < acq->n_dop - 1) ? acq->dop_idx + 1 : acq->dop_idx;

    double y_minus = acq->corr_map[doppler_prev * acq->n_samples + acq->code_sample];
    double y_center = base_power;
    double y_plus = acq->corr_map[doppler_next * acq->n_samples + acq->code_sample];

    double doppler_offset = 0.0;
    parabolic_interp_1d(y_minus, y_center, y_plus, &doppler_offset);

    // Refined Doppler frequency
    return base_doppler + doppler_offset * acq->dop_step;
}
