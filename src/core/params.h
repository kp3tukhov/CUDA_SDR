#ifndef PARAMS_H
#define PARAMS_H

#define M_PI 3.14159265358979323846

#define CUDA_BLOCK_SIZE 512         // Default thread block size for GPU

#define MAXCHPERBLOCK 64
#define MAXBLOCKS 4
#define MAXCHANNELS (MAXCHPERBLOCK*MAXBLOCKS)





/* 
 * ============================================================================
 * GPS SATELLITES L1CA
 * ============================================================================
 */
#define G_L1CA_CLEN 1023      // Number of chips in GPS L1CA code
#define G_L1CA_CRATE 1.023e6  // Chip rate GPS L1CA (Hz)
#define G_L1_CARR 1575.420e6  // Carrier frequency GPS L1 (Hz)

#define MAX_SATS 37            // Maximum PRN number (1-37)


/*
 * ============================================================================
 * RECEIVER FRONTEND
 * ============================================================================
 */

// Default receiver settings configuration file
#define RECV_CONFIG_FILE "recv.cfg"

#define RECV_F_ADC 8.0e6       // ADC sampling rate (Hz)
#define RECV_F_BW  2.5e6       // RF filter bandwidth (Hz)
#define RECV_F_LO  1573.420e6  // Local oscillator frequency (Hz)


/*
 * ============================================================================
 * CORRELATOR
 * ============================================================================
 */

// Default Doppler search range (Hz)
#define DEFAULT_DOP_MIN -5000.0
#define DEFAULT_DOP_MAX 5000.0

// Default Doppler search step (Hz)
#define DEFAULT_DOP_STEP 500.0

// Default integration time (ms)
#define DEFAULT_T_INT_MS 10.0


#endif /* PARAMS_H */
