#!/usr/bin/env python3
"""Gera um ICON0.PNG 80x80 simples (sem dependências externas).

Um ICON0.PNG ausente ou inválido impede a instalação do pacote no XMB.
Uso: python3 make_icon.py <saida.png>
"""
import struct
import sys
import zlib


def png_chunk(tag, data):
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "ICON0.PNG"
    w = h = 80
    raw = bytearray()
    for y in range(h):
        raw.append(0)  # filtro "None" para a linha
        for x in range(w):
            border = x < 4 or y < 4 or x >= w - 4 or y >= h - 4
            # verde escuro com borda verde clara
            r, g, b = (120, 220, 120) if border else (20, 60, 20)
            raw += bytes((r, g, b))

    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += png_chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += png_chunk(b"IEND", b"")
    with open(out, "wb") as f:
        f.write(png)
    print(f"ICON0 gerado em {out}")


if __name__ == "__main__":
    main()
