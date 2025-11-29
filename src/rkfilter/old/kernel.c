#include "rkfilter.h"

float **make_gaussian_kernel(int size, float sigma)
{
    if (size % 2 == 0 || size < 1)
    {
        fprintf(stderr, "kernel size must be odd and > 0\n");
        return NULL;
    }

    int radius = size / 2;
    float **kernel = malloc(size * sizeof(float *));
    if (!kernel)
        return NULL;

    for (int i = 0; i < size; i++)
    {
        kernel[i] = malloc(size * sizeof(float));
        if (!kernel[i])
        {
            for (int j = 0; j < i; j++)
                free(kernel[j]);
            free(kernel);
            return NULL;
        }
    }

    float sum = 0.0f;
    float two_sigma_sq = 2.0f * sigma * sigma;

    // вычисляем значения
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            float value = expf(-(x * x + y * y) / two_sigma_sq);
            kernel[y + radius][x + radius] = value;
            sum += value;
        }
    }

    // нормализация
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            kernel[y][x] /= sum;

    return kernel;
}