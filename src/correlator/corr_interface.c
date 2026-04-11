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
#include "correlator/corr_interface.h"
#include "correlator/correlator.h"
#include "correlator/correlator_gpu.cuh"


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
static void corr_accumulate_noncoherent(
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
) {

    int N = (int)round(recv->f_adc * (CODE_PERIOD_MS / 1000.0));

    int n_rows = cfg->n_dop;
    int n_cols = (cfg->method == METHOD_PARALLEL_CODE) ? N : GPS_CODE_LEN;

    // Zero the correlation map
    memset(corr_map, 0, sizeof(double) * n_rows * n_cols);

    if (cfg->verbose) {
        const char *method_names[] = {
            [METHOD_TIME_DOMAIN] = "Time Domain",
            [METHOD_PARALLEL_FREQ] = "Parallel Frequency",
            [METHOD_PARALLEL_CODE] = "Parallel Code"
        };
        printf("Starting non-coherent accumulation (%s method, %d periods)...\n",
               method_names[cfg->method], cfg->num_periods);
    }

    // Main accumulation loop over code periods
    for (int p = 0; p < cfg->num_periods; p++) {

        const float complex *sig_ptr = signal + (p * N);
        corr_func(sig_ptr, local_code, recv, cfg, corr_map);

    }
}


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

    if (config->device == DEVICE_GPU) {
        if (config->verbose && config->method != 3) {
            fprintf(stderr, "Warning: GPU is only implemented for method 3 (Parallel Code Search).\n");
            fprintf(stderr, "         Switchig to method 3\n");
        }

        batch_corr_execute_cuda(
            (const cuFloatComplex*) signal,
            local_code,
            config,
            recv,
            corr_maps
        );

        return 0;
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
