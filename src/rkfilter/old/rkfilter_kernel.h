#ifndef _RKFILTER_KERNEL_H
#define _RKFILTER_KERNEL_H

typedef enum gauss_kernel
{
    RK_GAUSS_3x3 = 3,
    RK_GAUSS_5x5 = 5
} gauss_kernel_t;

float **make_gaussian_kernel(int size, float sigma);

#endif