.global fast_copy_neon
.type fast_copy_neon, %function

fast_copy_neon:
    // x0 = dst (целевой буфер)
    // x1 = src (исходный буфер)
    // x2 = width
    // x3 = height
    // x4 = src_stride
    // x5 = dst_stride

    cmp x3, #0                  // Если height == 0, выходим
    ble end_copy_neon

loop_y:
    mov x6, x2                  // x6 = width (копируем, т.к. будем уменьшать)
    mov x7, x1                  // x7 = src_ptr
    mov x8, x0                  // x8 = dst_ptr

loop_x:
    cmp x6, #0                  // Если ширина <= 0, идём на следующую строку
    ble next_row

    ld1 {v0.16b}, [x7], #16      // Загружаем 16 байт (5 пикселей RGB)
    st1 {v0.16b}, [x8], #16      // Записываем 16 байт в dst

    sub x6, x6, #16              // Уменьшаем счётчик ширины
    b loop_x                     // Повторяем

next_row:
    add x1, x1, x4               // src_ptr += src_stride
    add x0, x0, x5               // dst_ptr += dst_stride
    sub x3, x3, #1               // Уменьшаем счётчик строк
    bne loop_y                   // Повторяем для следующей строки

end_copy_neon:
    ret
