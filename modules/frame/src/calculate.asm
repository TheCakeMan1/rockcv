.global calc_resize_neon
.type calc_resize_neon, %function

calc_resize_neon:
    // x0 = orig_w, x1 = orig_h, x2 = new_w, x3 = new_h
    // x4 = resized_w*, x5 = resized_h*, x6 = pad_x*, x7 = pad_y*

    scvtf s0, w0      // float orig_w = (float) orig_w
    scvtf s1, w1      // float orig_h = (float) orig_h
    scvtf s2, w2      // float new_width = (float) new_width
    scvtf s3, w3      // float new_height = (float) new_height

    fdiv s2, s2, s0   // scale_w = new_width / orig_w
    fdiv s3, s3, s1   // scale_h = new_height / orig_h
    fmin s4, s2, s3   // scale = min(scale_w, scale_h)

    fmul s5, s0, s4   // resized_w = orig_w * scale
    fmul s6, s1, s4   // resized_h = orig_h * scale

    fcvtzs w8, s5     // w8 = resized_w (int)
    fcvtzs w9, s6     // w9 = resized_h (int)

    sub w10, w2, w8   // new_width - resized_w
    asr w10, w10, 1   // pad_x = (new_width - resized_w) / 2

    sub w11, w3, w9   // new_height - resized_h
    asr w11, w11, 1   // pad_y = (new_height - resized_h) / 2

    // Запись результатов по указателям
    str w8, [x4]      // *resized_w = w8
    str w9, [x5]      // *resized_h = w9
    str w10, [x6]     // *pad_x = w10
    str w11, [x7]     // *pad_y = w11

    ret

