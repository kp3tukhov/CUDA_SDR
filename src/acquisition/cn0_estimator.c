/*
 * Carrier-to-Noise Density Ratio (C/N0) Estimation Implementation
 * 
 * This module provides functions for estimating C/N0 from correlation
 * maps, including noise floor estimation and sample-to-chip conversion
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core/params.h"
#include "core/types.h"
#include "acquisition/cn0_estimator.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif



double estimate_noise_floor(
    const double *corr_map,
    int n_doppler,
    int n_samples
) {
    if (!corr_map || n_doppler <= 0 || n_samples <= 0) {
        return 0.0;
    }

    int N = n_doppler * n_samples;

    double sum = 0.0;
    for (int i = 0; i < N; i++) {
        sum += corr_map[i];
    }

    return sum / N;
}


double estimate_cn0(
    double peak_power,
    double noise_floor,
    double t_integration
) {
    if (peak_power <= 0 || noise_floor <= 0 || t_integration <= 0) {
        return 0.0;
    }

    double snr = peak_power / noise_floor;

    // Signal below noise floor
    if (snr <= 1.0) {
        return 0.0;
    }

    double t_period = CODE_PERIOD_MS / 1000.0;

    double cn0 = 10.0 * log10((snr - 1.0) / t_period);

    return cn0;
}
