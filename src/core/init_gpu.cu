/*
 * CUDA GPU Initialization and Cleanup
 * 
 * This module provides GPU initialization and cleanup functions for
 * CUDA-accelerated GPS signal processing. It handles device selection,
 * capability querying, and resource deallocation
 */

#include <cuda_runtime.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize CUDA device and print device information
 *
 * Parameters:
 *   device_id - CUDA device ID to use (0-based index)
 *   verbose   - If non-zero, print detailed device information
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int cuda_init(int device_id, int verbose) {
    cudaDeviceProp prop;
    cudaError_t err;

    // Set the CUDA device
    err = cudaSetDevice(device_id);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to set CUDA device %d: %s\n",
                device_id, cudaGetErrorString(err));
        return -1;
    }

    if (verbose) {
        // Get and print device properties
        err = cudaGetDeviceProperties(&prop, device_id);
        if (err != cudaSuccess) {
            fprintf(stderr, "Failed to get device properties: %s\n",
                    cudaGetErrorString(err));
            return -1;
        }

        printf("CUDA Device: %s\n", prop.name);
        printf("  Compute Capability: %d.%d\n", prop.major, prop.minor);
        printf("  Global Memory: %zu MB\n", prop.totalGlobalMem / (1024 * 1024));
        printf("  Shared Memory per Block: %zu KB\n", prop.sharedMemPerBlock / 1024);
        printf("  Max Threads per Block: %d\n", prop.maxThreadsPerBlock);
    }

    return 0;
}


/*
 * Clean up CUDA resources by resetting the device
 * This should be called before program exit to release GPU resources
 */
void cuda_cleanup(void) {
    cudaDeviceReset();
}

#ifdef __cplusplus
}
#endif