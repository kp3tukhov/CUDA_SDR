/*
 * CUDA GPU-Accelerated Complex Vector Multiplication
 * 
 * This module provides CUDA kernel implementation for complex vector
 * multiplication used in GPS signal mixing operations
 */

#include <cuda_runtime.h>
#include <cuComplex.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


// Complex vector multiplication kernel: c[i] = a[i] * b[i]
__global__ void cpx_vec_mul_kernel(
    int size,
    const cuFloatComplex *a,
    const cuFloatComplex *b,
    cuFloatComplex *c
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < size) {
        c[idx] = cuCmulf(a[idx], b[idx]);
    }
}


/* ============================================================================
 * HOST INTERFACE
 * ============================================================================
 */

 
/*
 * Perform complex vector multiplication on GPU
 * Allocates device memory, copies data, executes kernel, and copies result back
 *
 * Parameters:
 *   size - Number of complex elements
 *   a    - First input vector (host pointer)
 *   b    - Second input vector (host pointer)
 *   c    - Output vector (host pointer)
 *
 * Returns:
 *   0 on success, -1 on CUDA error
 */
int cpx_vec_mul_cuda(
    int size,
    const cuFloatComplex *a,
    const cuFloatComplex *b,
    cuFloatComplex *c
) {
    cudaError_t err;

    // CUDA kernel launch configuration
    const int block_size = 256;
    const int grid_size = (size + block_size - 1) / block_size;

    // Allocate device memory
    size_t mem_size = size * sizeof(cuFloatComplex);

    cuFloatComplex *d_a = NULL;
    cuFloatComplex *d_b = NULL;
    cuFloatComplex *d_c = NULL;

    err = cudaMalloc((void**)&d_a, mem_size);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA malloc error (d_a): %s\n", cudaGetErrorString(err));
        return -1;
    }

    err = cudaMalloc((void**)&d_b, mem_size);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA malloc error (d_b): %s\n", cudaGetErrorString(err));
        cudaFree(d_a);
        return -1;
    }

    err = cudaMalloc((void**)&d_c, mem_size);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA malloc error (d_c): %s\n", cudaGetErrorString(err));
        cudaFree(d_a);
        cudaFree(d_b);
        return -1;
    }

    // Copy input data to device
    err = cudaMemcpy(d_a, a, mem_size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA memcpy error (d_a): %s\n", cudaGetErrorString(err));
        cudaFree(d_a);
        cudaFree(d_b);
        cudaFree(d_c);
        return -1;
    }

    err = cudaMemcpy(d_b, b, mem_size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA memcpy error (d_b): %s\n", cudaGetErrorString(err));
        cudaFree(d_a);
        cudaFree(d_b);
        cudaFree(d_c);
        return -1;
    }

    // Launch CUDA kernel
    cpx_vec_mul_kernel<<<grid_size, block_size>>>(size, d_a, d_b, d_c);

    err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA kernel launch error: %s\n", cudaGetErrorString(err));
        cudaFree(d_a);
        cudaFree(d_b);
        cudaFree(d_c);
        return -1;
    }

    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA device sync error: %s\n", cudaGetErrorString(err));
        cudaFree(d_a);
        cudaFree(d_b);
        cudaFree(d_c);
        return -1;
    }

    // Copy result back to host
    err = cudaMemcpy(c, d_c, mem_size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        fprintf(stderr, "CUDA memcpy error (result): %s\n", cudaGetErrorString(err));
        cudaFree(d_a);
        cudaFree(d_b);
        cudaFree(d_c);
        return -1;
    }

    // Free device memory
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);

    return 0;
}


#ifdef __cplusplus
}
#endif
