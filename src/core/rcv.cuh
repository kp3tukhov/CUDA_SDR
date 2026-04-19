/*
 * GNSS receiver and channel-block construction (host API)
 *
 * This header provides allocation and teardown functinos for the top-level receiver context,
 * RF front-end channels, and satellite channel blocks. Block setup includes PRN code
 * generation, acquisition state, CUDA unified-memory buffers, cuFFT plans, and
 * precomputed code spectra used by the GPU correlator pipeline
 */

#ifndef CORE_RCV_CUH
#define CORE_RCV_CUH

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Allocate a GNSS receiver context
 * Stores a pointer to the given RF channel (no copy); the channel must outlive the receiver
 *
 * Parameters:
 *   rf_ch - RF channel whose sample buffer downstream correlation will use
 */
GNSSReceiver_t *new_receiver(const RF_channel_t *rf_ch);

/*
 * Release a receiver and every satellite channel block attached to it
 * Does not free the RF channel that was passed to new_receiver
 *
 * Parameters:
 *   rcv - Receiver to free (no-op if NULL)
 */
void free_receiver(GNSSReceiver_t *rcv);

/*
 * Allocate an RF channel and its raw IQ sample buffer
 * Fills in front-end parameters; the buffer is sized for buffsize bytes of uint8_t samples
 *
 * Parameters:
 *   f_adc    - ADC sample rate (Hz)
 *   f_bw     - RF filter bandwidth (Hz)
 *   f_lo     - Local oscillator frequency (Hz)
 *   iq       - IQ mode flag (nonzero = IQ, zero = I-only)
 *   buffsize - Sample buffer length in bytes
 */
RF_channel_t *new_rf_ch(
    double f_adc,
    double f_bw,
    double f_lo,
    int iq,
    int buffsize
);

/*
 * Free an RF channel allocated by new_rf_ch
 * Releases the sample buffer and the channel structure
 *
 * Parameters:
 *   rf_ch - RF channel to free (no-op if NULL)
 */
void free_rf_ch(RF_channel_t *rf_ch);

/*
 * Allocate a satellite channel block with correlator resources
 * Copies correlator configuration, creates per-satellite channels (PRN codes, acquisition state),
 * allocates unified-memory GPU buffers and cuFFT plans, uploads Doppler phase steps, and
 * precomputes conjugated code FFTs for the GPU correlation path
 *
 * Parameters:
 *   id       - Block identifier stored in the new block
 *   syss     - Satellite system per channel (length num_prns)
 *   sigs     - Signal type per channel (length num_prns)
 *   prns     - PRN number per channel (length num_prns)
 *   num_prns - Number of satellites (clamped to MAXCHPERBLOCK)
 *   cfg      - Correlator configuration (Doppler grid, samples per period, periods, device/method)
 *   rf_ch    - RF channel (sample rate and LO used for Doppler phase steps and code resampling)
 */
satellite_channel_block_t *new_block(
    int id,
    sys_t *syss,
    sig_t *sigs,
    int *prns,
    int num_prns,
    const correlator_config_t *cfg,
    const RF_channel_t *rf_ch
);

/*
 * Destroy a block created by new_block
 * Frees satellite channels, cuFFT plans, and all CUDA allocations owned by the block
 *
 * Parameters:
 *   blk - Block to free (no-op if NULL)
 */
void free_block(satellite_channel_block_t *blk);

/*
 * Build a new satellite block and attach it to the receiver
 * Uses the receiver's RF channel pointer and assigns the new block id from the current block count
 * Returns 0 on success and -1 if the receiver is invalid, MAXBLOCKS would be exceeded, or allocation fails
 *
 * Parameters:
 *   rcv      - Receiver to append a block to
 *   syss     - Satellite systems for the new block (parallel with sigs and prns)
 *   sigs     - Signal identifiers for the new block
 *   prns     - PRN numbers for the new block
 *   num_prns - Length of syss, sigs, and prns
 *   cfg      - Correlator configuration for the new block
 */
int add_block_to_receiver(GNSSReceiver_t *rcv, sys_t *syss, sig_t *sigs, int *prns, int num_prns, const correlator_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RCV_CUH */
