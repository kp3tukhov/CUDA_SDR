/*
 * Peak Detection and Refinement
 * 
 * This header provides functions for finding correlation peaks and
 * refining Doppler frequency estimates using parabolic interpolation.
 */

#ifndef ACQUISITION_PEAK_DETECTION_H
#define ACQUISITION_PEAK_DETECTION_H

#include "core/types.h"


/*
 * Find the maximum correlation peak in a correlation map
 *
 * Parameters:
 *   corr_map     - Input correlation map (Doppler x Code)
 *   n_doppler    - Number of Doppler bins
 *   n_samples    - Number of code phase samples
 *   doppler_min  - Minimum Doppler frequency (Hz)
 *   doppler_step - Doppler frequency step (Hz)
 *   peak         - Output peak structure
 */
void find_correlation_peak(
    const double *corr_map,
    int n_doppler,
    int n_samples,
    double doppler_min,
    double doppler_step,
    correlation_peak_t *peak
);

/*
 * Refine Doppler frequency estimate using parabolic interpolation
 * Uses three points around the peak to estimate true peak position
 * with sub-bin accuracy. Code phase is held fixed
 *
 * Parameters:
 *   corr_map        - Input correlation map (Doppler x Code)
 *   n_doppler       - Number of Doppler bins
 *   n_samples       - Number of code phase samples
 *   peak_idx        - Doppler bin index of peak
 *   sample_idx      - Code sample index of peak
 *   doppler_min     - Minimum Doppler frequency (Hz)
 *   doppler_step    - Doppler frequency step (Hz)
 *   refined_doppler - Output refined Doppler frequency (Hz)
 */
void fine_doppler(
    const double *corr_map,
    int n_doppler,
    int n_samples,
    int peak_idx,
    int sample_idx,
    double doppler_min,
    double doppler_step,
    double *refined_doppler
);

#endif /* ACQUISITION_PEAK_DETECTION_H */
