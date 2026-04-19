/*
 * Signal Acquisition Pipeline
 * 
 * This header provides the main acquisition processing function that
 * performs satellite detection from correlation map
 */

#ifndef ACQUISITION_ACQ_PROCESSOR_H
#define ACQUISITION_ACQ_PROCESSOR_H

/*
 * Perform complete signal acquisition for a single satellite
 * Includes peak detection, Doppler refinement, and C/N0 estimation
 *
 * Parameters:
 *   sat            - Satellite channel structure
 *   threshold_db   - Detection threshold (dB-Hz)
 */
void perform_acquisition(
    satellite_channel_t *sat,
    double threshold_db
);

#endif /* ACQUISITION_ACQ_PROCESSOR_H */
