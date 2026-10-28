#!/usr/bin/env python3
"""Generate synthetic test files for the host tests.

Never use real saves for tests: they contain Miis and StreetPass data.
This script is an independent implementation of the formats in
the format notes, so the C tests cross-check against it.

    python3 tools/make_fixture.py --out tests/fixtures --defaults
    python3 tools/make_fixture.py --meet out.dat --own 9:0-6,8 --own 0:0-13
    python3 tools/make_fixture.py --coins out.dat --value 120
"""

import argparse
import os
import struct
import zlib

MEET_SIZE = 0x60000
STANDARD_PANELS = 0x044BDC
STANDARD_STRIDE = 0x02
SPOTPASS_PANELS = 0x045578
SPOTPASS_STRIDE = 0x44
STANDARD_COUNT = 7

# Piece count per panel id (from the Marc Robledo save editor).
PANEL_SIZES = [
    15, 15, 15, 15, 15, 15, 15, 24, 24, 40, 40, 24, 24, 24, 40, 40, 40, 40, 24, 15, 15,
    40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 24, 40, 40, 40, 40, 40, 24, 24, 15, 40,
    24, 40, 40, 15, 15, 15, 40, 40, 40, 40, 40, 24, 24, 15, 40, 15, 40, 40, 15, 15, 15,
]

COIN_SIZE = 0x14
COIN_OFFSET = 0x04

PNEL_HEADER = 0x14
PNEL_ENTRY = 0xC30
PNEL_NAME_STRIDE = 0xC0
PNEL_LANGS = 16
# CFG_Language -> name slot; languages without a slot are not written.
PNEL_SLOT = {0: 0, 1: 5, 2: 6, 3: 8, 4: 9, 5: 7, 9: 4}


def panel_offset(panel):
    if panel < STANDARD_COUNT:
        return STANDARD_PANELS + panel * STANDARD_STRIDE
    return SPOTPASS_PANELS + (panel - STANDARD_COUNT) * SPOTPASS_STRIDE


def encode_field(panel, owned):
    """Bitfield bytes for a set of owned piece indices (hypothesis)."""
    count = PANEL_SIZES[panel]
    field = bytearray((count + 7) // 8)
    for i in owned:
        if not 0 <= i < count:
            raise ValueError(f"piece {i} out of range for panel {panel}")
        if panel < STANDARD_COUNT:
            field[i // 8] |= 1 << (i % 8)          # LSB first
        else:
            field[i // 8] |= 0x80 >> (i % 8)       # MSB first
    return bytes(field)


def parse_ranges(text):
    out = set()
    for part in filter(None, text.split(",")):
        a, _, b = part.partition("-")
        out.update(range(int(a), int(b or a) + 1))
    return out


def noise(size, seed):
    return bytearray(((i * 7 + seed) & 0xFF) for i in range(size))


def make_meet(owned_by_panel):
    """meet.dat filled with noise, every panel bitfield written explicitly."""
    data = noise(MEET_SIZE, 3)
    for panel in range(len(PANEL_SIZES)):
        field = encode_field(panel, owned_by_panel.get(panel, ()))
        off = panel_offset(panel)
        data[off:off + len(field)] = field
    return bytes(data)


def make_coins(value):
    data = noise(COIN_SIZE, 11)
    data[COIN_OFFSET:COIN_OFFSET + 2] = struct.pack("<H", value)
    return bytes(data)


def crc_zeroed(block, field):
    tmp = bytearray(block)
    tmp[field:field + 4] = b"\0\0\0\0"
    return zlib.crc32(bytes(tmp)) & 0xFFFFFFFF


def make_pnel(entries):
    """entries: list of dicts with order, names {lang: str}, piece_id, pink, blobs (3 bytes objects)."""
    count = len(entries)
    data_start = PNEL_HEADER + count * PNEL_ENTRY
    blobs = bytearray()
    raw_entries = []
    for e in entries:
        offsets, sizes, crcs = [], [], []
        for blob in e["blobs"]:
            offsets.append(data_start + len(blobs))
            sizes.append(len(blob))
            crcs.append(zlib.crc32(blob) & 0xFFFFFFFF)
            blobs += blob
            blobs += b"\0" * (-len(blob) % 4)
        entry = bytearray(PNEL_ENTRY)
        struct.pack_into("<I", entry, 0x00, e["order"])
        struct.pack_into("<9I", entry, 0x08, *offsets, *sizes, *crcs)
        for lang, name in e["names"].items():
            raw = name.encode("utf-16-le")[:PNEL_NAME_STRIDE - 2]
            start = 0x2C + PNEL_SLOT[lang] * PNEL_NAME_STRIDE
            entry[start:start + len(raw)] = raw
        entry[0xC2C] = e["piece_id"]
        entry[0xC2D] = e["pink"]
        struct.pack_into("<I", entry, 0x04, crc_zeroed(entry, 0x04))
        raw_entries.append(bytes(entry))

    header = bytearray(PNEL_HEADER)
    header[0:4] = b"PNEL"
    header[4:8] = b"0800"
    struct.pack_into("<II", header, 0x08, count, 1)
    struct.pack_into("<I", header, 0x10, crc_zeroed(header, 0x10))
    return bytes(header) + b"".join(raw_entries) + bytes(blobs)


def tile_xy(i):
    """Position inside an 8x8 tile of the GPU: x and y bits alternate, x lowest."""
    x = (i & 1) | ((i >> 1) & 2) | ((i >> 2) & 4)
    y = ((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4)
    return x, y


def sample_rgb565(width, height):
    """RGB565 texture data of a small test picture as the PNEL files store it: top row first, 8x8 tiles, Z order inside a tile. Picture pixel (x, y),
    y counted from the top, has R5 = x % 32, G6 = y * 4 % 64, B5 = (x + y) % 32."""
    out = bytearray()
    for ty in range(height // 8):
        for tx in range(width // 8):
            for i in range(64):
                x, y = tile_xy(i)
                px = tx * 8 + x
                py = ty * 8 + y
                value = ((px % 32) << 11) | ((py * 4 % 64) << 5) | ((px + py) % 32)
                out += struct.pack("<H", value)
    return bytes(out)


def lz11_literals(data):
    """LZ11 stream that stores everything as literals (valid, just not smaller)."""
    out = bytearray([0x11]) + len(data).to_bytes(3, "little")
    for i in range(0, len(data), 8):
        out.append(0)
        out += data[i:i + 8]
    return bytes(out)


def make_cgfx(width, height, fmt, data):
    """Smallest CGFX that holds one texture object. The layout of the object is
    the CGFX hypothesis; a real file has to confirm it."""
    txob_at = 0x20
    data_at = txob_at + 0x50
    buf = bytearray(data_at)
    buf[0:4] = b"CGFX"
    struct.pack_into("<HH", buf, 4, 0xFEFF, 0x14)
    struct.pack_into("<I", buf, 0x0C, data_at + len(data))
    buf[txob_at + 4:txob_at + 8] = b"TXOB"
    struct.pack_into("<II", buf, txob_at + 0x18, height, width)
    struct.pack_into("<I", buf, txob_at + 0x34, fmt)
    struct.pack_into("<I", buf, txob_at + 0x44, len(data))
    struct.pack_into("<I", buf, txob_at + 0x48, data_at - (txob_at + 0x48))
    return bytes(buf) + data


# Owned pieces of the sample save; mirrors the C tests.
SAMPLE_OWNED = {
    0: set(range(14)),                       # standard 15, all but piece 14
    3: set(range(15)),                       # standard 15, complete
    7: set(range(24)),                       # SpotPass 24, complete
    9: parse_ranges("0-6,8-11,13,16-19,21,24-27,29,32-33,35,37-39"),  # SpotPass 40, 28 owned
}


def write(path, data):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def write_defaults(out):
    write(os.path.join(out, "meet_empty.dat"), bytes(MEET_SIZE))
    write(os.path.join(out, "meet_sample.dat"), make_meet(SAMPLE_OWNED))

    bad = bytearray(make_meet(SAMPLE_OWNED))
    bad[panel_offset(0) + 1] |= 0x80  # bit beyond piece 14 of a standard panel
    write(os.path.join(out, "meet_bad_unused.dat"), bytes(bad))

    write(os.path.join(out, "gamecoin_120.dat"), make_coins(120))
    write(os.path.join(out, "gamecoin_300.dat"), make_coins(300))
    write(os.path.join(out, "gamecoin_bad_301.dat"), make_coins(301))

    pnel = make_pnel([
        {"order": 7, "piece_id": 1, "pink": 0,
         "names": {1: "Summer Meadow", 3: "Sommerwiese", 0: "夏の草原", 9: "Star 🌟"},
         "blobs": [b"IMG" * 10, b"SETTINGS", b"MODEL" * 3]},
        {"order": 8, "piece_id": 2, "pink": 3,
         "names": {1: "Lighthouse", 3: "Leuchtturm"},
         "blobs": [b"I" * 5, b"S" * 7, b"M" * 9]},
    ])
    write(os.path.join(out, "pnel_sample.dat"), pnel)

    # A file as the console would hold it: panel ids and piece counts agree with the catalog.
    picture = make_cgfx(16, 8, 3, sample_rgb565(16, 8))
    spotpass = make_pnel([
        {"order": 7, "piece_id": 1, "pink": 0,
         "names": {1: "Summer Meadow", 3: "Sommerwiese"},
         "blobs": [picture, b"SETTINGS", b"MODEL"]},
        {"order": 9, "piece_id": 2, "pink": 5,
         "names": {1: "Lighthouse"},
         "blobs": [b"not a cgfx file", b"S", b"M"]},
        {"order": 11, "piece_id": 1, "pink": 0,     # downloaded, no piece yet in the sample save
         "names": {3: "Rhythmus"},
         "blobs": [lz11_literals(picture), b"S", b"M"]},   # picture packed, like the real files
        {"order": 8, "piece_id": 2, "pink": 0,      # panel 8 has 24 pieces: ignored
         "names": {1: "Wrong size"},
         "blobs": [b"I", b"S", b"M"]},
        {"order": 3, "piece_id": 0, "pink": 0,      # standard panel: ignored
         "names": {1: "Standard"},
         "blobs": [b"I", b"S", b"M"]},
    ])
    write(os.path.join(out, "pnel_spotpass.dat"), spotpass)
    write(os.path.join(out, "sample_rgb565_16x8.cgfx"), picture)

    corrupt = bytearray(pnel)
    corrupt[PNEL_HEADER + 0x2C + PNEL_SLOT[3] * PNEL_NAME_STRIDE] ^= 0xFF  # change a name byte
    write(os.path.join(out, "pnel_bad_entry_crc.dat"), bytes(corrupt))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    p.add_argument("--out", default="tests/fixtures", help="directory for --defaults")
    p.add_argument("--defaults", action="store_true", help="write the fixture set used by the tests")
    p.add_argument("--meet", help="write a meet.dat to this path")
    p.add_argument("--own", action="append", default=[], help="PANEL:RANGES, e.g. 9:0-6,8")
    p.add_argument("--coins", help="write a gamecoin.dat to this path")
    p.add_argument("--value", type=int, default=120, help="coin value for --coins")
    args = p.parse_args()

    if args.defaults:
        write_defaults(args.out)
    if args.meet:
        owned = {}
        for spec in args.own:
            panel, _, ranges = spec.partition(":")
            owned[int(panel)] = parse_ranges(ranges)
        write(args.meet, make_meet(owned))
    if args.coins:
        write(args.coins, make_coins(args.value))
    if not (args.defaults or args.meet or args.coins):
        p.print_help()


if __name__ == "__main__":
    main()
