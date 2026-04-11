/*
 * CUDA Batch Correlator Interface (GPU)
 *
 * This header provides the host interface for the CUDA-based batch correlator
 * for GPS signal processing on the GPU. It performs frequency-domain correlation
 * across multiple satellites, Doppler bins, and code periods with non-coherent
 * power accumulation
 */

#ifndef CORR_INTERFACE_GPU_CUH
#define CORR_INTERFACE_GPU_CUH

#include <cuComplex.h>
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Execute batch correlation for multiple satellites on GPU
 *
 * This is the main entry point for GPU-based correlation processing
 * It manages all GPU resources, transfers data once, performs correlation
 * with non-coherent accumulation entirely on GPU, and copies results back
 *
 * Parameters:
 *   signal      - Input signal buffer
 *   local_code  - Concatenated PRN codes [num_prns * GPS_CODE_LEN]
 *   config      - Correlator configuration
 *   recv        - Receiver configuration
 *   corr_maps   - Output array of correlation maps [num_prns * n_dop * N]
 *
 * Returns:
 *   0 on success, -1 on error
 */
int batch_corr_execute_cuda(
    const cuFloatComplex *signal,
    const int8_t *local_code,
    const correlator_config_t *config,
    const receiver_t *recv,
    double **corr_maps
);

#ifdef __cplusplus
}
#endif

#endif /* CORR_INTERFACE_GPU_CUH */
