from PIL import Image, ImageFont, ImageDraw
import os

# Настройки
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)
chars = (
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
    "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
    "0123456789"
    ".,:;!?()[]{}+-=*/_<>\"'@#$%^&~ "
)

os.makedirs("font", exist_ok=True)

for ch in chars:
    if ch == " ":
        continue
    img = Image.new("L", (32, 32), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), ch, 255, font=font)
    safe_name = f"{ord(ch)}.bmp"  # безопасное имя, только код символа
    img.save(os.path.join("font", safe_name))
