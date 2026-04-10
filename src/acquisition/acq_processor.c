/*
 * Signal Acquisition Pipeline Implementation
 * 
 * This module provides the main acquisition processing function that
 * performs satellite detection from correlation map
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core/params.h"
#include "core/types.h"
#include "acquisition/acq_processor.h"
#include "acquisition/peak_detection.h"
#include "acquisition/cn0_estimator.h"


void perform_acquisition(
    const double *corr_map,
    int n_doppler,
    int n_samples,
    double doppler_min,
    double doppler_step,
    int prn,
    double t_integration,
    double threshold_db,
    satellite_channel_t *result
) {
    if (!corr_map || !result) {
        return;
    }

    // Initialize channel structure
    result->prn = prn;
    result->active = 0;
    result->fdoppler = 0.0;
    result->code_phase_start_chips = 0.0;
    result->carrier_phase_start_rad = 0.0;
    result->cn0_db_hz = 0.0;

    // Find correlation peak
    correlation_peak_t peak;
    find_correlation_peak(
        corr_map, n_doppler, n_samples,
        doppler_min, doppler_step,
        &peak
    );

    result->code_phase_start_chips = peak.code_phase_chips;
    result->fdoppler = peak.doppler_freq;

    // Refine Doppler frequency (parabolic interpolation)
    double refined_doppler;
    fine_doppler(
        corr_map, n_doppler, n_samples,
        peak.doppler_idx, peak.code_sample,
        doppler_min, doppler_step,
        &refined_doppler
    );

    result->fdoppler = refined_doppler;

    // Estimate noise floor
    double noise_floor = estimate_noise_floor(corr_map, n_doppler, n_samples);

    // Estimate C/N0
    double peak_power = corr_map[peak.doppler_idx * n_samples + peak.code_sample];
    result->cn0_db_hz = estimate_cn0(
        peak_power,
        noise_floor,
        t_integration
    );

    // Detect satellite aquisition status
    result->active = (result->cn0_db_hz >= threshold_db) ? 1 : 0;
}
