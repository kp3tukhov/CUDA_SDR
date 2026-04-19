/*
 * GPS L1 C/A spreading-code generation
 *
 * This header declares the LFSR-based C/A Gold code generator: standard G1/G2 registers,
 * per-PRN G2 delay, and bipolar (+1/-1) chip output for correlation and resampling pipelines
 */

#ifndef CODE_CODE_GPS_H
#define CODE_CODE_GPS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generate a contiguous segment of the GPS L1 C/A code for a specific PRN
 * Runs the G1/G2 LFSRs from the requested starting chip and writes n_samples chips as +1/-1
 *
 * Parameters:
 *   prn        - PRN number (1 through MAX_SATS for supported GPS C/A PRNs)
 *   buffer     - Output buffer for code chips (+1 / -1)
 *   n_samples  - Number of chips to write
 *   start_chip - Starting chip index within the C/A period (0-based, wraps modulo code length)
 */
void gen_code_L1CA(
    int prn,
    int8_t *buffer,
    int n_samples,
    int start_chip
);

#ifdef __cplusplus
}
#endif

#endif /* CODE_CODE_GPS_H */
