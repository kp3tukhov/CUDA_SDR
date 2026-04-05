/*
 * Correlation Functions for GPS Signal Processing
 * 
 * This header provides three correlation methods:
 * - Sequential time-domain correlation
 * - Parallel frequency-domain (FFT-based) correlation
 * - Parallel code-domain (FFT-based) correlation
 */

#ifndef CORRELATOR_H
#define CORRELATOR_H

#include <complex.h>
#include <fftw3.h>
#include "core/types.h"


/*
 * Perform sequential time-domain correlation
 * Iterates through all code phases and Doppler frequencies sequentially
 *
 * Parameters:
 *   signal     - Input signal buffer (complex samples)
 *   local_code - Generated local PRN code (+1/-1 values)
 *   recv       - Receiver configuration
 *   cfg        - Correlator configuration
 *   corr_map   - Output correlation map (Doppler x Code)
 */
void corr_sequential(
    const float complex *signal,
    const int8_t *local_code,
    const receiver_t *recv,
    const correlator_config_t *cfg,
    double *corr_map
);


/*
 * Perform parallel frequency-domain correlation (XF)
 * Uses FFT to search all Doppler frequencies in parallel for each code phase
 *
 * Parameters:
 *   signal     - Input signal buffer (complex samples)
 *   local_code - Generated local PRN code (+1/-1 values)
 *   recv       - Receiver configuration
 *   cfg        - Correlator configuration
 *   corr_map   - Output correlation map (Doppler x Code)
 */
void corr_parallel_freq(
    const float complex *signal,
    const int8_t *local_code,
    const receiver_t *recv,
    const correlator_config_t *cfg,
    double *corr_map
);


/*
 * Perform parallel code-domain correlation (FX)
 * Uses FFT to search all code phases in parallel for each Doppler frequency
 *
 * Parameters:
 *   signal     - Input signal buffer (complex samples)
 *   local_code - Generated local PRN code (+1/-1 values)
 *   recv       - Receiver configuration
 *   cfg        - Correlator configuration
 *   corr_map   - Output correlation map (Doppler x Code)
 */
void corr_parallel_code(
    const float complex *signal,
    const int8_t *local_code,
    const receiver_t *recv,
    const correlator_config_t *cfg,
    double *corr_map
);

#endif /* CORRELATOR_H */
