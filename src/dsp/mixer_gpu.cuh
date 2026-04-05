#ifndef DSP_MIXER_GPU_H
#define DSP_MIXER_GPU_H

#include <cuComplex.h>

#ifdef __cplusplus
extern "C" {
#endif

int cpx_vec_mul_cuda(
    int size,
    const cuFloatComplex *a,
    const cuFloatComplex *b,
    cuFloatComplex *c
);

#ifdef __cplusplus
}
#endif

#endif /* DSP_MIXER_GPU_H */
