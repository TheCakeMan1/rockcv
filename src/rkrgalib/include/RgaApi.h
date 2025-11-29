#ifndef _RKRGALIB_H
#define _RKRGALIB_H
#include "NormalRga.h"

#define RK_TRANSFORM_FLIP_H 0x01
#define RK_TRANSFORM_FLIP_V 0x02
#define RK_TRANSFORM_ROT_90 0x04
#define RK_TRANSFORM_ROT_180 0x03
#define RK_TRANSFORM_ROT_270 0x07

/**
 * @brief Performs hardware-accelerated blitzing (copying, scaling, rotation,
 *        format transformations, alpha blending) using RGA.
 *
 * The function is the main API for the Rockchip RGA graphics accelerator
 * and allows you to perform operations on images with minimal CPU usage.
 *
 * @param[in]  src   The main data source.
 *                   The rga_info_t structure specifies the format, dimensions, sampling area,
 *                   conversion parameters, and buffer address.
 *
 * @param[out] dst   The destination buffer.
 *                   Description of the output image and its characteristics.
 *
 * @param[in]  src1  Additional source (used for blend/overlay).
 *                   It can be NULL if the operation does not require a second buffer.
 *
 * @return 0 if the operation is started successfully.
 * @retval -EINVAL   Incorrect arguments
 * @retval -ENODEV   RGA is unavailable
 * @retval -EFAULT   Invalid buffer addresses
 * @retval -EBUSY    The device is busy
 *
 * @note All rga_info_t fields must be filled in correctly before calling. The function is taken from the official librga repository.
 */
int RgaBlit(rga_info_t *src, rga_info_t *dst, rga_info_t *src1);

#endif