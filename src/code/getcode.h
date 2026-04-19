/*
 * GNSS spreading-code access (dispatch layer)
 *
 * This header declares helpers to query nominal signal parameters (code length, chip rate,
 * carrier, code period) and to fill a buffer with one period of spreading code for a given
 * system, signal, and PRN. Concrete sequences are produced by signal-specific generators
 * (for example GPS L1 C/A in code_gps.h)
 */

#ifndef CODE_GETCODE_H
#define CODE_GETCODE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fill a buffer with one complete spreading-code period for the given system, signal, and PRN
 * Dispatches to the appropriate generator (e.g. GPS L1 C/A); buffer length must be at least get_code_len
 *
 * Parameters:
 *   sys  - Satellite system identifier
 *   sig  - Signal identifier
 *   prn  - PRN number (valid range depends on system/signal)
 *   buff - Output buffer for bipolar chips (+1 / -1), length >= get_code_len(sys, sig)
 */
void get_code(sys_t sys, sig_t sig, int prn, int8_t *buff);

/*
 * Return the spreading-code length in chips for the given system and signal
 *
 * Parameters:
 *   sys - Satellite system identifier
 *   sig - Signal identifier
 */
int get_code_len(sys_t sys, sig_t sig);

/*
 * Return the nominal carrier frequency (Hz) for the given system and signal
 *
 * Parameters:
 *   sys - Satellite system identifier
 *   sig - Signal identifier
 */
double get_carrier(sys_t sys, sig_t sig);

/*
 * Return the spreading-code chip rate (chips per second) for the given system and signal
 *
 * Parameters:
 *   sys - Satellite system identifier
 *   sig - Signal identifier
 */
double get_chiprate(sys_t sys, sig_t sig);

/*
 * Return the nominal full-code period duration in milliseconds
 * Derived from code length and chip rate for the given system and signal
 *
 * Parameters:
 *   sys - Satellite system identifier
 *   sig - Signal identifier
 */
int get_period_ms(sys_t sys, sig_t sig);

#ifdef __cplusplus
}
#endif

#endif /* CODE_GETCODE_H */
