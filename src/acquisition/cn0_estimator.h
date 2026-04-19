/*
 * Carrier-to-Noise Density Ratio (C/N0) Estimation
 * 
 * This header provides functions for estimating C/N0 from correlation
 * maps, including noise floor estimation and sample-to-chip conversion
 */

#ifndef ACQUISITION_CN0_ESTIMATOR_H
#define ACQUISITION_CN0_ESTIMATOR_H

/*
 * Estimate noise floor from correlation map
 * Computes mean power across all points in the map
 *
 * Parameters:
 *   acq        - Acquisition Context structure
 *
 * Returns:
 *   Estimated noise floor (mean power)
 */
double estimate_noise_floor(
    acquisition_context_t *acq
);

/*
 * Estimate carrier-to-noise density ratio (C/N0)
 *
 * Parameters:
 *   peak_power    - Peak correlation power
 *   noise_floor   - Estimated noise floor (mean power)
 *   t_int         - Integration time (seconds)
 *
 * Returns:
 *   C/N0 estimate in dB-Hz
 */
double estimate_cn0(
    double peak_power,
    double noise_floor,
    double t_int
);

#endif /* ACQUISITION_CN0_ESTIMATOR_H */
