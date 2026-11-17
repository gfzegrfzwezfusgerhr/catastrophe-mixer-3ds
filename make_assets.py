#!/usr/bin/env python3
"""Generates icon.png (48x48), banner.png (256x128) and audio.wav (3s) with the stdlib only."""
import zlib, struct, math, wave

def png(path, w, h, px):
    raw = b''.join(b'\x00' + bytes(sum((px(x, y) for x in range(w)), ())) for y in range(h))
    def ch(t, d): c = struct.pack('>I', len(d)) + t + d; return c + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n' + ch(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + ch(b'IDAT', zlib.compress(raw)) + ch(b'IEND', b''))

def scene(w, h):
    def px(x, y):
        u, v = x / w, y / h
        r, g, b = 40 + 120 * (1 - v), 20 + 40 * (1 - v), 60 + 40 * v            # stormy sky
        if (x // (w // 12)) % 2 == 0 and v > 0.55 + 0.25 * ((x // (w // 12) * 37) % 5) / 5: r, g, b = 30, 30, 45   # skyline
        tx = w * 0.62                                                           # tornado
        if 0.15 < v < 0.9 and abs(x - tx - 6 * math.sin(v * 9)) < 3 + (1 - v) * w * 0.1: r, g, b = 200, 200, 215
        d = math.hypot(x - w * 0.2, y - h * 0.3)                                # meteor glow
        if d < h * 0.12: r, g, b = 255, 150 + 100 * (1 - d / (h * .12)), 40
        if v > 0.93: r, g, b = 255, 90, 20                                      # lava line
        return (int(min(255, r)), int(min(255, g)), int(min(255, b)))
    return px

png('icon.png', 48, 48, scene(48, 48))
png('banner.png', 256, 128, scene(256, 128))
import os
if not os.path.exists('audio.wav'):   # keep your own audio.wav if present
    with wave.open('audio.wav', 'wb') as f:   # silent placeholder - replace audio.wav with your own (max 3s)
        f.setnchannels(2); f.setsampwidth(2); f.setframerate(32728)
        f.writeframes(b'\x00' * 4 * 32728 * 2)
