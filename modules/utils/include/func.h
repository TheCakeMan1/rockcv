#ifndef ROCKCV_FUNC_H
#define ROCKCV_FUNC_H


extern "C" void fast_copy_neon(uint8_t *dst, uint8_t *src, int width, int height, int src_stride, int dst_stride);

#endif