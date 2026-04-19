/*
 * Carrier-to-Noise Density Ratio (C/N0) Estimation Implementation
 * 
 * This module provides functions for estimating C/N0 from correlation
 * maps, including noise floor estimation and sample-to-chip conversion
 */

#include <math.h>

#include "core/params.h"
#include "core/types.h"
#include "acquisition/cn0_estimator.h"


double estimate_noise_floor(
    acquisition_context_t *acq
) {
    if (!acq || !acq->corr_map) {
        return 0.0;
    }

    int N = acq->n_dop * acq->n_samples;

    double sum = 0.0;
    for (int i = 0; i < N; i++) {
        sum += acq->corr_map[i];
    }

    return sum / N;
}


double estimate_cn0(
    double peak_power,
    double noise_floor,
    double t_int
) {
    if (peak_power <= 0 || noise_floor <= 0) {
        return 0.0;
    }

    double snr = peak_power / noise_floor;

    // Signal below noise floor
    if (snr <= 1.0) {
        return 0.0;
    }

    double cn0 = 10.0 * log10((snr - 1.0) / t_int);

    return cn0;
}
