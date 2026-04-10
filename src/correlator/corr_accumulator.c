/*
 * Non-coherent Correlation Accumulation Implementation
 * 
 * This module provides functions for accumulating correlation results
 * across multiple code periods to improve detection sensitivity
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "core/params.h"
#include "core/types.h"
#include "core/utils.h"
#include "correlator/corr_accumulator.h"
#include "correlator/correlator.h"


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
