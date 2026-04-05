/*
 * Data Type Definitions for GPS Signal Processing
 * 
 * This header defines core data structures used throughout the GPS
 * signal processing pipeline, including satellite channels, receiver
 * configuration, and correlator data types
 */

#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <fftw3.h>
#include <complex.h>
#include "params.h"

/* 
 * ============================================================================
 * SATELLITE
 * ============================================================================
 */

// Satellite channel parameters
typedef struct {
    int prn;                        // PRN number (1-37)
    double fdoppler;                // Doppler frequency shift (Hz)
    double code_phase_start_chips;  // Initial code phase (chips)
    double carrier_phase_start_rad; // Initial carrier phase (radians)
    double cn0_db_hz;               // Carrier-to-noise density ratio (dB-Hz)
    int active;                     // Active channel flag
} satellite_channel_t;


/* 
 * ============================================================================
 * RECEIVER FRONTEND
 * ============================================================================
 */

// Receiver configuration structure
typedef struct {
    double f_adc;   // Sampling rate (Hz)
    double f_bw;    // Filter bandwidth (Hz)
    double f_lo;    // Local oscillator frequency (Hz)
    double f_if;    // Intermediate frequency (Hz)
    int iq;         // IQ mode flag (1=IQ 0=I)
} receiver_t;


/* 
 * ============================================================================
 * CORRELATOR
 * ============================================================================
 */

// Correlator method enumeration
typedef enum {
    METHOD_TIME_DOMAIN = 1,     // Sequential time-domain correlation
    METHOD_PARALLEL_FREQ,       // Parallel frequency-domain (FFT-based)
    METHOD_PARALLEL_CODE        // Parallel code-domain (FFT-based)
} correlator_method_t;

// Correlator configuration parameters
typedef struct {
    double dop_min;                 // Minimum Doppler frequency (Hz)
    double dop_step;                // Doppler frequency step (Hz)
    int n_dop;                      // Number of Doppler bins
    int num_periods;                // Number of code periods for accumulation
    correlator_method_t method;     // Correlation method
    int prns[MAX_SATS];             // PRN numbers to process
    int num_prns;                   // Number of PRNs
    int verbose;                    // Verbose output flag
} correlator_config_t;

// Correlation peak information
typedef struct {
    int doppler_idx;            // Doppler frequency bin index
    int code_sample;            // Code phase sample index
    double power;               // Peak power
    double doppler_freq;        // Doppler frequency (Hz)
    double code_phase_chips;    // Code phase (chips)
} correlation_peak_t;


#endif /* CORE_TYPES_H */
