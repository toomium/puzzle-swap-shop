#!/usr/bin/env python3
"""Print the puzzle state of a real meet.dat (and optionally gamecoin.dat).

Used for hardware tests: shows each panel's raw bitfield and how it
decodes under both possible bit orders, so a known puzzle state from the
game can be matched to the right one. For unfinished panels it also lists
where the missing pieces would sit in the picture if pieces are numbered
row by row from the top left. Reads only; never writes.

The first argument may also be the system save image copied with GodMode9
(see tools/extract_disa.py); meet.dat is then taken from inside it.

    python3 tools/inspect_save.py path/to/meet.dat [path/to/gamecoin.dat]
"""

import struct
import sys

sys.path.insert(0, __file__.rsplit("/", 1)[0])
from make_fixture import MEET_SIZE, PANEL_SIZES, STANDARD_COUNT, panel_offset  # noqa: E402
import extract_disa  # noqa: E402

# Columns of the picture grid per piece count.
GRID_COLUMNS = {15: 5, 24: 6, 40: 8}


def decode(field, count, msb_first):
    owned = []
    for i in range(count):
        mask = (0x80 >> (i % 8)) if msb_first else (1 << (i % 8))
        if field[i // 8] & mask:
            owned.append(i)
    return owned


def unused_clear(field, count, msb_first):
    for i in range(count, len(field) * 8):
        mask = (0x80 >> (i % 8)) if msb_first else (1 << (i % 8))
        if field[i // 8] & mask:
            return False
    return True


def ranges(indices):
    if not indices:
        return "-"
    out = []
    start = prev = indices[0]
    for i in indices[1:]:
        if i == prev + 1:
            prev = i
            continue
        out.append(str(start) if start == prev else f"{start}-{prev}")
        start = prev = i
    out.append(str(start) if start == prev else f"{start}-{prev}")
    return ",".join(out)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    data = open(sys.argv[1], "rb").read()
    if extract_disa.is_disa(data):
        try:
            data = extract_disa.read_files(data)["meet.dat"]
        except (extract_disa.DisaError, KeyError) as e:
            print(f"system save image without a readable meet.dat: {e}")
            return 1
        print("system save image: meet.dat extracted, IVFC hashes ok")
    print(f"meet.dat: {len(data)} bytes (expected {MEET_SIZE})")
    if len(data) != MEET_SIZE:
        return 1

    hypothesis_ok = True
    unfinished = []
    print("panel kind      pieces raw              LSB-first ok  MSB-first ok  owned (hypothesis)")
    for panel, count in enumerate(PANEL_SIZES):
        off = panel_offset(panel)
        field = data[off:off + (count + 7) // 8]
        lsb_ok = unused_clear(field, count, False)
        msb_ok = unused_clear(field, count, True)
        msb = panel >= STANDARD_COUNT
        owned = decode(field, count, msb)
        if not (msb_ok if msb else lsb_ok):
            hypothesis_ok = False
        kind = "SpotPass" if msb else "Standard"
        if 0 < len(owned) < count:
            unfinished.append((panel, count, [i for i in range(count) if i not in owned]))
        print(f"{panel:5} {kind:9} {count:6} {field.hex(' '):16} {str(lsb_ok):13} {str(msb_ok):13} "
              f"{len(owned):2}/{count} {ranges(owned)}")
    print()
    print("unused bits consistent with hypothesis:", "yes" if hypothesis_ok else "NO")

    if unfinished:
        print()
        print("missing pieces as row/column, counted row by row from the top left (hypothesis):")
        for panel, count, missing in unfinished:
            cols = GRID_COLUMNS[count]
            where = ", ".join(f"{i // cols + 1}/{i % cols + 1}" for i in missing)
            print(f"{panel:5} ({cols}x{count // cols}): {where}")

    if len(sys.argv) > 2:
        coin = open(sys.argv[2], "rb").read()
        print(f"gamecoin.dat: {len(coin)} bytes, coins = {struct.unpack_from('<H', coin, 4)[0]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
