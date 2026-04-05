#ifndef CORE_INIT_GPU_H
#define CORE_INIT_GPU_H

#ifdef __cplusplus
extern "C" {
#endif

int cuda_init(int device_id, int verbose);
void cuda_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_INIT_GPU_H */