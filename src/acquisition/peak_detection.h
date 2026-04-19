/*
 * Peak Detection and Refinement
 * 
 * This header provides functions for finding correlation peaks and
 * refining Doppler frequency estimates using parabolic interpolation.
 */

#ifndef ACQUISITION_PEAK_DETECTION_H
#define ACQUISITION_PEAK_DETECTION_H

/*
 * Find the maximum correlation peak in a correlation map of a satellite
 *
 * Parameters:
 *   sat     - Satellite Channel structure
 */
void find_correlation_peak(satellite_channel_t *sat);

/*
 * Refine Doppler frequency estimate using parabolic interpolation
 * Uses three points around the peak to estimate true peak position
 * with sub-bin accuracy. Code phase is held fixed
 *
 * Parameters:
 *   acq        - Acquisition Context structure
 */
double fine_doppler(
    acquisition_context_t *acq
);

#endif /* ACQUISITION_PEAK_DETECTION_H */
