/*
 * Correlation Interface Implementation
 * 
 * This module provides a unified interface for executing batch correlation
 * for a satellite block
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <complex.h>

#include "core/params.h"
#include "core/types.h"
#include "correlator/corr_interface.h"
#include "correlator/correlator.h"
#include "correlator/correlator_gpu.cuh"
#include "code/getcode.h"



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
    const uint8_t *raw,
    const int8_t *local_code,
    int code_len,
    int N,
    double chiprate,
    double carrier,
    const RF_channel_t *rf_ch,
    const correlator_config_t *cfg,
    float *corr_map,
    void (*corr_func)(
        const uint8_t *raw,
        const int8_t *local_code,
        int code_len,
        int N,
        double chiprate,
        double carrier,
        const RF_channel_t *rf_ch,
        const correlator_config_t *cfg,
        float *corr_map
    )
) {

    int n_rows = cfg->n_dop;
    int n_cols = (cfg->method == METHOD_PARALLEL_CODE) ? N : code_len;

    // Zero the correlation map
    memset(corr_map, 0, sizeof(float) * n_rows * n_cols);

    // Main accumulation loop over code periods
    for (int p = 0; p < cfg->num_periods; p++) {

        const uint8_t *raw_ptr = raw + (p * N);
        
        corr_func(raw_ptr, local_code, code_len, N, chiprate, carrier, rf_ch, cfg, corr_map);
    }    
}


int process_receiver(GNSSReceiver_t *rcv) {
    if (!rcv) return -1;
  
    for (int i = 0; i < rcv->num_blocks; i++) {
        int ret = process_block(rcv->blocks[i], rcv->blocks[i]->config.num_periods);
        if (ret != 0) return ret;
    }

    return 0;
}


int process_block(
    satellite_channel_block_t *blk,
    int num_periods
) {
    if (!blk) return -1;

    correlator_config_t config = blk->config;



    if (config.device == DEVICE_GPU) {
        if (config.method != 3) {
            fprintf(stderr, "Warning: GPU is only implemented for method 3 (Parallel Code Search).\n");
            fprintf(stderr, "         Switchig to method 3\n");
        }

        correlate_block_gpu(blk, num_periods);

        return 0;
    }



    // Select correlation function based on method
    void (*corr_func)(
        const uint8_t *raw,
        const int8_t *local_code,
        int code_len,
        int N,
        double chiprate,
        double carrier,
        const RF_channel_t *rf_ch,
        const correlator_config_t *cfg,
        float *corr_map
    );

    switch (config.method) {
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
    for (int i = 0; i < blk->num_channels; i++) {
        
        satellite_channel_t *sat = blk->channels[i];

        const int8_t *sat_code = sat->code;
        float *sat_corr_map = sat->acq->corr_map;

        int code_len = get_code_len(sat->sys, sat->sig);

        int N = (int)round(blk->rf_ch->f_adc * (get_period_ms(sat->sys, sat->sig) / 1000.0));

        double chiprate = get_chiprate(sat->sys, sat->sig);
        double carrier = get_carrier(sat->sys, sat->sig);
        
        // Call accumulator for each satellite
        corr_accumulate_noncoherent(blk->rf_ch->buffer, sat_code, code_len, N, chiprate, carrier, blk->rf_ch, &config, sat_corr_map, corr_func);
        
    }

    return 0;
}
