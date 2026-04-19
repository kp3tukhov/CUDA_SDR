/*
 * Signal Acquisition Pipeline Implementation
 * 
 * This module provides the main acquisition processing function that
 * performs satellite detection from correlation map
 */

#include "core/params.h"
#include "core/types.h"
#include "acquisition/acq_processor.h"
#include "acquisition/peak_detection.h"
#include "acquisition/cn0_estimator.h"
#include "code/getcode.h"


void perform_acquisition(
    satellite_channel_t *sat,
    double threshold_db
) {
    if (!sat || !sat->acq || !sat->acq->corr_map) {
        return;
    }

    acquisition_context_t *acq = sat->acq;

    // Find correlation peak
    find_correlation_peak(sat);

    // Refine Doppler frequency (parabolic interpolation)
    double refined_doppler = fine_doppler(acq);

    sat->fdoppler = refined_doppler;

    // Estimate noise floor
    double noise_floor = estimate_noise_floor(acq);

    // Estimate C/N0
    double peak_power = acq->corr_map[acq->dop_idx * acq->n_samples + acq->code_sample];
    double t_int = (double)get_period_ms(sat->sys, sat->sig) / 1000.0;
    sat->cn0_db_hz = estimate_cn0(
        peak_power,
        noise_floor,
        t_int
    );

    // Detect satellite aquisition status
    sat->active = (sat->cn0_db_hz >= threshold_db) ? 1 : 0;
}
