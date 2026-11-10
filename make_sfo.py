#!/usr/bin/env python3
"""Gera o PARAM.SFO (formato binário PSF) usado pelo pacote do PS3.

Uso:
  python3 make_sfo.py <saida> <titulo> <TITLE_ID> <CONTENT_ID> [versao]

Regras que evitam o erro "pkg não instala":
  - TITLE_ID tem exatamente 9 caracteres.
  - CONTENT_ID tem exatamente 36 caracteres: UP0001-<TITLE_ID>_00-<16 chars>.
  - CONTENT_ID precisa começar com o mesmo TITLE_ID.
"""
import struct
import sys


def build_sfo(entries):
    """entries: dict nome -> ('s', valor_str, tamanho_max) ou ('i', valor_int)."""
    keys = sorted(entries.keys())
    key_table = bytearray()
    data_table = bytearray()
    index = bytearray()

    for key in keys:
        key_offset = len(key_table)
        key_table += key.encode("ascii") + b"\x00"

        kind = entries[key][0]
        if kind == "s":
            _, value, max_len = entries[key]
            raw = value.encode("utf-8") + b"\x00"
            if len(raw) > max_len:
                raise ValueError(f"{key} maior que {max_len} bytes")
            fmt = 0x0204  # string UTF-8 terminada em nulo
            data_len = len(raw)
            data_max = max_len
            blob = raw.ljust(max_len, b"\x00")
        else:
            _, value = entries[key]
            fmt = 0x0404  # inteiro de 32 bits
            data_len = 4
            data_max = 4
            blob = struct.pack("<I", value)

        data_offset = len(data_table)
        data_table += blob
        while len(data_table) % 4:
            data_table += b"\x00"

        index += struct.pack("<HHIII", key_offset, fmt, data_len, data_max, data_offset)

    while len(key_table) % 4:
        key_table += b"\x00"

    header_size = 20
    key_table_offset = header_size + len(index)
    data_table_offset = key_table_offset + len(key_table)
    header = struct.pack(
        "<4sIIII", b"\x00PSF", 0x0101, key_table_offset, data_table_offset, len(keys)
    )
    return header + index + key_table + data_table


def main():
    if len(sys.argv) < 5:
        print(__doc__)
        sys.exit(1)

    out, title, title_id, content_id = sys.argv[1:5]
    version = sys.argv[5] if len(sys.argv) > 5 else "01.00"

    if len(title_id) != 9:
        sys.exit(f"TITLE_ID precisa ter 9 caracteres (tem {len(title_id)})")
    if len(content_id) != 36:
        sys.exit(f"CONTENT_ID precisa ter 36 caracteres (tem {len(content_id)})")
    if not content_id.startswith("UP0001-" + title_id):
        sys.exit("CONTENT_ID precisa começar com UP0001-<TITLE_ID>")

    entries = {
        "APP_VER": ("s", version, 8),
        "ATTRIBUTE": ("i", 0),
        "CATEGORY": ("s", "HG", 4),
        "CONTENT_ID": ("s", content_id, 48),
        "PARENTAL_LEVEL": ("i", 1),
        "TITLE": ("s", title, 128),
        "TITLE_ID": ("s", title_id, 16),
        "VERSION": ("s", version, 8),
    }

    with open(out, "wb") as f:
        f.write(build_sfo(entries))
    print(f"PARAM.SFO gerado em {out}")


if __name__ == "__main__":
    main()
