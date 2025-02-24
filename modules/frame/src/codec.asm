.global av_to_rk_format
.type av_to_rk_format, %function

.global align_up
.type align_up, %function

align_up:
    // w0 num , w1 align
    ADD w0, w0, w1  // num + align
    SUB w0, w0, #1  // num + align - 1
    SUB w1, w1, #1  // num + align - 1
    MVN w2, w1      // w2 = ~align  (битовая инверсия)
    AND w0, w0, w2  // w0 = (num + align - 1) & ~align
    RET

av_to_rk_format:
    LDR x1, =table_av     // Загружаем адрес таблицы AV_PIX_FMT
    LDR x2, =table_rga    // Загружаем адрес таблицы RGA-форматов
    MOV x3, #11           // Количество элементов в таблице
    MOV x5, #0            // Индекс для table_rga

loop:
    CBZ x3, not_found      // Если x3 == 0, выходим (не найдено)
    LDR w4, [x1, x5, LSL #2]  // Загружаем текущий AV_PIX_FMT из таблицы
    CMP w0, w4            // Сравниваем входное значение с текущим
    B.EQ found            // Если совпадает, идем к `found`
    
    ADD x5, x5, #1        // Увеличиваем индекс для `table_rga`
    SUBS x3, x3, #1       // Уменьшаем счетчик оставшихся элементов
    B.NE loop             // Если не 0, продолжаем цикл

not_found:
    MOVZ w0, #65636 & 0xFFFF  // Загружаем младшие 16 бит
    MOVK w0, #65636 >> 16, LSL #16  // Если есть старшие 16 бит, загружаем их
    RET

found:
    LDR w0, [x2, x5, LSL #2]  // Загружаем соответствующее значение из `table_rga`
    RET
    
.align 4
table_av:
    .word 0 //AV_PIX_FMT_YUV420P
    .word 23 //AV_PIX_FMT_NV12
    .word 24 //AV_PIX_FMT_NV21
    .word 1 //AV_PIX_FMT_YUYV422
    .word 15 //AV_PIX_FMT_UYVY422
    .word 2 //AV_PIX_FMT_RGB24
    .word 3 //AV_PIX_FMT_BGR24
    .word 26 //AV_PIX_FMT_RGBA
    .word 28 //AV_PIX_FMT_BGRA
    .word 8 //AV_PIX_FMT_GRAY8
    .word 4 //AV_PIX_FMT_YUV422P

table_rga:
    .word 2560 //RK_FORMAT_YCbCr_420_SP
    .word 3584 //RK_FORMAT_YCrCb_420_SP
    .word 2560 //RK_FORMAT_YCbCr_420_SP
    .word 7168 //RK_FORMAT_YUYV_422
    .word 7680 //RK_FORMAT_UYVY_422
    .word 512 //RK_FORMAT_RGB_888
    .word 1792 //RK_FORMAT_BGR_888
    .word 0 //RK_FORMAT_RGBA_8888
    .word 768 //RK_FORMAT_BGRA_8888
    .word 5376 //RK_FORMAT_YCbCr_400
    .word 2048 //RK_FORMAT_YCbCr_422_SP
