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

/*
 * Perform sequential time-domain correlation
 * Iterates through all code phases and Doppler frequencies sequentially
 *
 * Parameters:
 *   signal     - Input signal buffer (complex samples)
 *   local_code - Generated local PRN code (+1/-1 values)
 *   code_len   - Length of the code sequence (chips)
 *   N          - Number of valid samples in one code period
 *   chiprate   - Chip rate in the GNSS signal
 *   carrier    - Carrier frequency of the GNSS signal
 *   rf_ch      - Receiver RF channel
 *   cfg        - Correlator configuration
 *   corr_map   - Output correlation map (Doppler x Code)
 */
void corr_sequential(
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

/*
 * Perform parallel frequency-domain correlation (XF)
 * Uses FFT to search all Doppler frequencies in parallel for each code phase
 *
 * Parameters:
 *   signal     - Input signal buffer (complex samples)
 *   local_code - Generated local PRN code (+1/-1 values)
 *   code_len   - Length of the code sequence (chips)
 *   N          - Number of valid samples in one code period
 *   chiprate   - Chip rate in the GNSS signal
 *   carrier    - Carrier frequency of the GNSS signal
 *   rf_ch      - Receiver RF channel
 *   cfg        - Correlator configuration
 *   corr_map   - Output correlation map (Doppler x Code)
 */
void corr_parallel_freq(
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

/*
 * Perform parallel code-domain correlation (FX)
 * Uses FFT to search all code phases in parallel for each Doppler frequency
 *
 * Parameters:
 *   signal     - Input signal buffer (complex samples)
 *   local_code - Generated local PRN code (+1/-1 values)
 *   code_len   - Length of the code sequence (chips)
 *   N          - Number of valid samples in one code period
 *   chiprate   - Chip rate in the GNSS signal
 *   carrier    - Carrier frequency of the GNSS signal
 *   rf_ch      - Receiver RF channel
 *   cfg        - Correlator configuration
 *   corr_map   - Output correlation map (Doppler x Code)
 */
void corr_parallel_code(
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

#endif /* CORRELATOR_H */
