#ifndef ROCKCV_HELP_H
#define ROCKCV_HELP_H

extern "C" void calc_resize_neon(int orig_w, int orig_h, int new_width, int new_height, int *resized_w, int *resized_h, int *pad_x, int *pad_y);

#endif