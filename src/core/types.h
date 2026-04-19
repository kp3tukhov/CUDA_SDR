/*
 * Data Type Definitions for GPS Signal Processing
 *
 * This header defines core data structures used throughout the GPS
 * signal processing pipeline, including satellite channels, receiver
 * configuration, and correlator data types
 * 
 */

#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <cuda_runtime.h>
#include <cuComplex.h>
#include <cufft.h>

#include "params.h"

/*
 * ============================================================================
 * GENERAL
 * ============================================================================
 */

// Correlation device
typedef enum {
    DEVICE_CPU = 1,          // CPU correlation pipeline
    DEVICE_GPU,              // GPU correlation pipeline
} device_t;

// Satellite system
typedef enum {
    SYS_GPS = 1,
} sys_t;

// Signal type
typedef enum {
    SIG_L1CA = 1,
} sig_t;



/*
 * ============================================================================
 * RECEIVER FRONTEND
 * ============================================================================
 */

// Receiver RF channel configuration structure
typedef struct {
    double f_adc;   // Sampling rate (Hz)
    double f_bw;    // Filter bandwidth (Hz)
    double f_lo;    // Local oscillator frequency (Hz)
    int iq;         // IQ mode flag (1=IQ 0=I)

    uint8_t *buffer;                        // Buffer for raw data
    int buffsize;                           // Size of buffer

} RF_channel_t;


/*
 * ============================================================================
 * CORRELATOR
 * ============================================================================
 */

// Correlator method enumeration
typedef enum {
    METHOD_TIME_DOMAIN = 1,     // Sequential time-domain correlation
    METHOD_PARALLEL_FREQ,       // Parallel frequency-domain (FFT-based)
    METHOD_PARALLEL_CODE,       // Parallel code-domain (FFT-based)
} correlator_method_t;

// Correlator configuration parameters
typedef struct {
    double dop_min;                 // Minimum Doppler frequency (Hz)
    double dop_step;                // Doppler frequency step (Hz)
    int n_dop;                      // Number of Doppler bins
    int n_samples;
    int num_periods;                // Number of code periods for accumulation

    correlator_method_t method;     // Correlation method
    device_t device;                // Device to run correlation on

} correlator_config_t;


/*
 * ============================================================================
 * ACQUISITION
 * ============================================================================
 */

// Total aquisition context for one sattelite channel
typedef struct {

    double dop_min;
    double dop_step;
    int n_dop;
    int n_samples;


    int dop_idx;                // Doppler frequency bin index
    int code_sample;            // Code phase sample index
    double power;               // Peak power

    float *corr_map;

} acquisition_context_t;


/*
 * ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

typedef struct satellite_channel_block satellite_channel_block_t;


/*
 * ============================================================================
 * SATELLITE
 * ============================================================================
 */

// Satellite channel parameters
typedef struct {
    sys_t sys;                      // Satellite system identifier
    sig_t sig;                      // Signal identifier
    int prn;                        // PRN number
    
    double fdoppler;                // Doppler frequency shift (Hz)
    double code_phase_start_chips;  // Initial code phase (chips)
    double carrier_phase_start_rad; // Initial carrier phase (radians)
    double cn0_db_hz;               // Carrier-to-noise density ratio (dB-Hz)
    int active;                     // Active channel flag


    const int8_t *code;
    int code_len;

    acquisition_context_t *acq;     // Aquisition context for this satellite

    satellite_channel_block_t *parent;  // Block with this satellite
} satellite_channel_t;


/*
 * ============================================================================
 * SATELLITE BLOCK
 * ============================================================================
 */

struct satellite_channel_block {
    int id;                                             // Block id
    int num_channels;                                   // Number of sat channels

    satellite_channel_t *channels[MAXCHPERBLOCK];       // Sat channels

    RF_channel_t *rf_ch;                                // Associated RF channel

    cudaStream_t stream;                                // Working stream
    cufftHandle plan_fwd;                               // Forward batched FFT plan
    cufftHandle plan_inv;                               // Inversed batched FFT plan

// All arrays (except phase steps) are in unifierd memory space

    cuFloatComplex *d_codes_fft;                        // [num_channels x fft_size] Code FFT cache
    cuFloatComplex *d_mixed;                            // [fft_size x n_periods x n_dop] Frequency-mixed signal buffer
    cuFloatComplex *d_prod;                             // [fft_size x n_periods x n_channels x n_dop] Signal-code frequency domain product buffer
    double *d_phase_steps;                              // [n_dop] Phase steps for LUT cache (Device Memory)

    float *d_corr_maps;                                // Correlation maps

    correlator_config_t config;
};


/*
 * ============================================================================
 * RECEIVER
 * ============================================================================
 */

// Total receiver context
typedef struct {
    satellite_channel_block_t *blocks[MAXBLOCKS];       // Blocks in running receiver
    int num_blocks;                                     // Number of active blocks

    RF_channel_t *rf_ch;                                // Associated RF channel

} GNSSReceiver_t;

#endif /* CORE_TYPES_H */
