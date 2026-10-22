#!/usr/bin/env python3
"""Generate icons without external packages on macOS GitHub runners."""
import math
import pathlib
import struct
import zlib

OUT = pathlib.Path(__file__).resolve().parent.parent / "Resources"
OUT.mkdir(exist_ok=True)

def make_icon(scale, color):
    width = 48 * scale
    pixels = bytearray()
    for y in range(width):
        pixels.append(0)
        for x in range(width):
            hits = 0
            for sy in range(3):
                yy = (y + (sy + 0.5) / 3) / scale
                for sx in range(3):
                    xx = (x + (sx + 0.5) / 3) / scale
                    dx, dy = xx - 24, yy - 24
                    radius = math.hypot(dx, dy)
                    angle = math.degrees(math.atan2(dy, dx))
                    if radius < 3.7 or ((abs(angle) < 66 or abs(angle) > 114) and
                        (abs(radius - 12.7) < 2.1 or abs(radius - 21.0) < 2.1)):
                        hits += 1
            pixels.extend((*color, (hits * 255 + 4) // 9))
    def chunk(tag, data):
        return struct.pack("!I", len(data)) + tag + data + struct.pack("!I", zlib.crc32(tag + data) & 0xffffffff)
    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack("!IIBBBBB", width, width, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(bytes(pixels), 9)) +
            chunk(b"IEND", b""))

for scale in (2, 3):
    for name, color in (("HotspotGlyph", (255, 255, 255)), ("Icon", (31, 38, 48))):
        filename = OUT / f"{name}@{scale}x.png"
        filename.write_bytes(make_icon(scale, color))
        print(f"{filename}: {filename.stat().st_size} bytes")
