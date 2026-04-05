/*
 * Signal Acquisition Pipeline
 * 
 * This header provides the main acquisition processing function that
 * performs satellite detection from correlation map
 */

#ifndef ACQUISITION_ACQ_PROCESSOR_H
#define ACQUISITION_ACQ_PROCESSOR_H

#include "core/types.h"

/*
 * Perform complete signal acquisition for a single satellite
 * Includes peak detection, Doppler refinement, and C/N0 estimation
 *
 * Parameters:
 *   corr_map       - Input correlation map (Doppler x Code)
 *   n_doppler      - Number of Doppler bins
 *   n_samples      - Number of code phase samples
 *   doppler_min    - Minimum Doppler frequency (Hz)
 *   doppler_step   - Doppler frequency step (Hz)
 *   prn            - PRN number being processed
 *   t_integration  - Integration time (seconds)
 *   threshold_db   - Detection threshold (dB-Hz)
 *   result         - Output satellite channel structure
 */
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
);

#endif /* ACQUISITION_ACQ_PROCESSOR_H */
