/*
 * Correlation Interface
 * 
 * This header provides a unified interface for executing batch correlation
 * for a satellite block
 */

#ifndef CORRELATOR_INTERFACE_H
#define CORRELATOR_INTERFACE_H

/*
 * Process every satellite channel block in a receiver
 * Dispatches batch correlation for each block using that block's correlator configuration
 *
 * Parameters:
 *   rcv - Receiver context (blocks and RF channel)
 */
int process_receiver(
    GNSSReceiver_t *rcv
);

/*
 * Run batch correlation for one satellite channel block
 * Uses the GPU path when blk->config.device requests it; otherwise runs the CPU
 * correlator (sequential, parallel frequency, or parallel code) with non-coherent accumulation
 *
 * Parameters:
 *   blk         - Block with RF samples, satellite channels, and correlator configuration
 *   num_periods - Number of code periods to process (GPU batch size; CPU path uses cfg in blk)
 */
int process_block(
    satellite_channel_block_t *blk,
    int num_periods
);

#endif /* CORRELATOR_INTERFACE_H */
