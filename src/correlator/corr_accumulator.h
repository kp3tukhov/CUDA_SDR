/*
 * Non-coherent Correlation Accumulation
 * 
 * This header provides functions for accumulating correlation results
 * across multiple code periods to improve detection sensitivity
 */

#ifndef CORR_ACCUMULATOR_H
#define CORR_ACCUMULATOR_H

#include "core/types.h"


/*
 * Dispatcher function for non-coherent correlation accumulation
 * Calls the correlation function for each code period and accumulates power
 *
 * Parameters:
 *   signal      - Input signal buffer (contains multiple periods)
 *   local_code  - Generated local PRN code (+1/-1 values)
 *   recv        - Receiver configuration
 *   cfg         - Correlator configuration (includes num_periods)
 *   corr_map    - Output correlation map (accumulated power)
 *   corr_func   - Pointer to correlation function to use
 */
void corr_accumulate_noncoherent(
    const float complex *signal,
    const int8_t *local_code,
    const receiver_t *recv,
    const correlator_config_t *cfg,
    double *corr_map,
    void (*corr_func)(
        const float complex *signal,
        const int8_t *local_code,
        const receiver_t *recv,
        const correlator_config_t *cfg,
        double *corr_map
    )
);

#endif /* CORR_ACCUMULATOR_H */
