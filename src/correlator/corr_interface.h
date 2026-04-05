/*
 * Correlation Interface
 * 
 * This header provides a unified interface for executing batch correlation
 * across multiple satellites
 */

#ifndef CORRELATOR_INTERFACE_H
#define CORRELATOR_INTERFACE_H

#include <complex.h>
#include "core/types.h"


/*
 * Execute batch correlation for multiple satellites
 * This is the main entry point for correlation processing
 *
 * Parameters:
 *   signal      - Input signal buffer (complex samples)
 *   local_code  - Concatenated PRN codes [num_prns * GPS_CODE_LEN]
 *   config      - Correlator configuration
 *   recv        - Receiver configuration
 *   corr_maps   - Output array of correlation maps [num_prns * nrows * ncols]
 *
 * Returns:
 *   0 on success, -1 on error
 */
int batch_corr_execute(
    const float complex *signal,
    const int8_t *local_code,
    const correlator_config_t *config,
    const receiver_t *recv,
    double **corr_maps
);

#endif /* CORRELATOR_INTERFACE_H */
