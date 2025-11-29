#!/usr/bin/env python3
import sys
from PIL import Image, ImageFont, ImageDraw
import unicodedata

# ---------------- CONFIG ----------------
FONT_PATH = "Roboto-Regular.ttf"  # путь к TTF
FONT_SIZE = 32                   # высота
CHARS = [chr(c) for c in range(32, 127)]  # ASCII
# Для кириллицы можно так:
# CHARS = [chr(c) for c in range(0x20, 0x0500)]
# ----------------------------------------


def to_hex32(argb_tuple):
    a, r, g, b = argb_tuple
    return f"0x{a:02X}{r:02X}{g:02X}{b:02X}"


font = ImageFont.truetype(FONT_PATH, FONT_SIZE)

glyphs = []
glyph_data = []

for ch in CHARS:
    # Метрики
    (width, height), (offset_x, offset_y) = font.font.getsize(ch)
    advance = font.getlength(ch)

    # Создаем ARGB канвас
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), ch, font=font, fill=(255, 255, 255, 255))

    pixels = img.load()

    # Формируем C-массив
    flat = []
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            flat.append((a, r, g, b))

    array_name = f"glyph_{ord(ch):04X}_bitmap"
    glyphs.append((ch, width, height, offset_x, offset_y, int(advance), array_name))
    glyph_data.append((array_name, width * height, flat))


# ---------------- WRITE .h AND .c ----------------

with open("font32.h", "w") as h, open("font32.c", "w") as c:

    h.write("#pragma once\n#include <stdint.h>\n\n")
    h.write("typedef struct {\n"
            "    uint32_t codepoint;\n"
            "    uint16_t width;\n"
            "    uint16_t height;\n"
            "    int16_t  bearing_x;\n"
            "    int16_t  bearing_y;\n"
            "    int16_t  advance;\n"
            "    const uint32_t *bitmap;\n"
            "} Glyph;\n\n")

    for name, count, data in glyph_data:
        c.write(f"static const uint32_t {name}[{count}] = {{\n")
        for (a, r, g, b) in data:
            c.write(f"    0x{a:02X}{r:02X}{g:02X}{b:02X},\n")
        c.write("};\n\n")

    c.write("const Glyph font32[] = {\n")
    for ch, w, hgt, bx, by, adv, arr in glyphs:
        c.write(f"    {{ {ord(ch)}, {w}, {hgt}, {bx}, {by}, {adv}, {arr} }},\n")
    c.write("};\n\n")

    c.write("const uint32_t font32_count = ")
    c.write(f"{len(glyphs)};\n")

    h.write("extern const Glyph font32[];\n")
    h.write("extern const uint32_t font32_count;\n")


# import freetype
# import numpy as np

# font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
# face = freetype.Face(font_path)
# face.set_pixel_sizes(0, 20)

# chars = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" \
#         "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"


# W, H = 13, 26
# BASELINE = 22

# advances, lefts = [], []

# for ch in chars:
#     face.load_char(ch)
#     bmp = face.glyph.bitmap
#     buf = np.array(bmp.buffer, dtype=np.uint8).reshape(bmp.rows, bmp.width)

#     left = face.glyph.bitmap_left
#     top = face.glyph.bitmap_top
#     adv = face.glyph.advance.x >> 6

#     out = np.zeros((H, W), np.uint8)

#     # вычисляем вертикальное положение от baseline
#     y0 = BASELINE - top
#     x0 = max(0, left)

#     h, w = min(bmp.rows, H - y0), min(bmp.width, W - x0)
#     if h > 0 and w > 0 and y0 >= 0:
#         out[y0:y0 + h, x0:x0 + w] = buf[:h, :w]

#     print(f"    // '{ch}' (U+{ord(ch):04X}), left={left}, advance={adv}")
#     print("    {")
#     for y in range(H):
#         row = ", ".join(f"{v:3d}" for v in out[y])
#         print(f"        {row},")
#     print("    },")
#     advances.append(adv)
#     lefts.append(left)





# # from PIL import Image, ImageFont, ImageDraw
# # import os

# # # Настройки
# # font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)
# # chars = (
# #     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
# #     "abcdefghijklmnopqrstuvwxyz"
# #     "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
# #     "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
# #     "0123456789"
# #     ".,:;!?()[]{}+-=*/_<>\"'@#$%^&~ "
# # )

# # os.makedirs("font", exist_ok=True)

# # for ch in chars:
# #     if ch == " ":
# #         continue
# #     img = Image.new("L", (32, 32), 0)
# #     draw = ImageDraw.Draw(img)
# #     draw.text((0, 0), ch, 255, font=font)
# #     safe_name = f"{ord(ch)}.bmp"  # безопасное имя, только код символа
# #     img.save(os.path.join("font", safe_name))
