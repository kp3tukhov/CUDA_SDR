/*
 * Correlation Interface Implementation
 * 
 * This module provides a unified interface for executing batch correlation
 * across multiple satellites
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "core/params.h"
#include "core/types.h"
#include "correlator/correlator.h"
#include "correlator/corr_accumulator.h"
#include "correlator/corr_interface.h"


int batch_corr_execute(
    const float complex *signal,
    const int8_t *local_code,
    const correlator_config_t *config,
    const receiver_t *recv,
    double **corr_maps
) {
    if (!signal || !local_code || !config || !corr_maps) {
        return -1;
    }

    if (config->num_prns <= 0 || config->num_prns > MAX_SATS) {
        return -1;
    }

    // Select correlation function based on method
    void (*corr_func)(
        const float complex *signal,
        const int8_t *local_code,
        const receiver_t *recv,
        const correlator_config_t *cfg,
        double *corr_map
    );

    switch (config->method) {
        case METHOD_TIME_DOMAIN:
            corr_func = corr_sequential;
            break;

        case METHOD_PARALLEL_FREQ:
            corr_func = corr_parallel_freq;
            break;

        case METHOD_PARALLEL_CODE:
        default:
            corr_func = corr_parallel_code;
            break;
    }

    // Execute correlation for each satellite
    for (int sat_idx = 0; sat_idx < config->num_prns; sat_idx++) {

        const int8_t *sat_code = local_code + (sat_idx * GPS_CODE_LEN);
        double *sat_corr_map = corr_maps[sat_idx];

        // Call accumulator for each satellite
        corr_accumulate_noncoherent(signal, sat_code, recv, config, sat_corr_map, corr_func);
    }

    return 0;
}
