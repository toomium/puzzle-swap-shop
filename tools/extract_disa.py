#!/usr/bin/env python3
"""Extract the files from a 3DS system save image (DISA container).

GodMode9 can copy Mii Plaza's system save from
`1:/data/<ID0>/sysdata/00020228/00000000` (EUR; USA 00020218, JPN 00020208).
That file is a DISA image; meet.dat sits inside it. NAND system saves are not
encrypted, so no console keys are needed. The CMAC is not checked, the IVFC
hashes are: a damaged meet.dat is reported, not silently returned.

    python3 tools/extract_disa.py 00000000 out/

Reads only the input. Format: https://www.3dbrew.org/wiki/DISA_and_DIFF
"""

import hashlib
import os
import struct
import sys

DISA_MAGIC = b"DISA"
DIFI_MAGIC = 0x49464944
IVFC_MAGIC = 0x43465649
DPFS_MAGIC = 0x53465044
SAVE_MAGIC = 0x45564153


class DisaError(Exception):
    pass


def is_disa(image):
    return image[0x100:0x104] == DISA_MAGIC


def _dpfs_level(selector, pair, block_size, length):
    """Pick each block from copy 0 or 1, as the selector bitmap says."""
    out = bytearray()
    pos = 0
    word = 0
    while pos < length:
        (bits,) = struct.unpack_from("<I", selector, word * 4)
        for i in range(32):
            if pos >= length:
                break
            n = min(block_size, length - pos)
            out += pair[(bits >> (31 - i)) & 1][pos : pos + n]
            pos += n
        word += 1
    return bytes(out)


def _bad_blocks(hashes, data, block_size):
    """Block indices whose SHA-256 does not match the level above."""
    bad = set()
    for n in range(len(hashes) // 32):
        chunk = data[n * block_size : (n + 1) * block_size]
        if not chunk:
            break
        chunk = chunk.ljust(block_size, b"\0")
        if hashlib.sha256(chunk).digest() != hashes[n * 32 : n * 32 + 32]:
            bad.add(n)
    return bad


def _unwrap_partition(desc, part):
    """Return the IVFC level-4 payload and the set of its unverified blocks."""
    (magic, _ver, ivfc_off, _ivfc_size, dpfs_off, _dpfs_size, hash_off, hash_size,
     ext_l4, l1_sel, _l4_ext) = struct.unpack_from("<IIQQQQQQBB2xQ", desc, 0)
    if magic != DIFI_MAGIC:
        raise DisaError("bad DIFI magic")
    if ext_l4:
        raise DisaError("external IVFC level 4 is not supported")
    ivfc = struct.unpack_from("<IIQQQI4xQQI4xQQI4xQQI4xQ", desc, ivfc_off)
    if ivfc[0] != IVFC_MAGIC:
        raise DisaError("bad IVFC magic")
    levels = [(ivfc[3 + 3 * i], ivfc[4 + 3 * i], 1 << ivfc[5 + 3 * i]) for i in range(4)]
    dpfs = struct.unpack_from("<IIQQI4xQQI4xQQI4x", desc, dpfs_off)
    if dpfs[0] != DPFS_MAGIC:
        raise DisaError("bad DPFS magic")
    d = [(dpfs[2 + 3 * i], dpfs[3 + 3 * i], 1 << dpfs[4 + 3 * i]) for i in range(3)]

    def pair(off, size):
        return part[off : off + size], part[off + size : off + 2 * size]

    l1 = pair(d[0][0], d[0][1])[l1_sel]
    l2 = _dpfs_level(l1, pair(d[1][0], d[1][1]), d[1][2], d[1][1])
    active = _dpfs_level(l2, pair(d[2][0], d[2][1]), d[2][2], d[2][1])

    hashes = desc[hash_off : hash_off + hash_size]
    for level, (off, size, block_size) in enumerate(levels, 1):
        data = active[off : off + size]
        bad = _bad_blocks(hashes, data, block_size)
        if bad and level < 4:
            raise DisaError(f"IVFC level {level} hash mismatch")
        hashes = data
    return data, bad, levels[3][2]


def read_files(image):
    """Return {path: bytes} for every file in a single-partition DISA image."""
    if not is_disa(image):
        raise DisaError("not a DISA image")
    (_magic, _ver, count, sec_tab, pri_tab, tab_size, a_desc_off, a_desc_size,
     _b_desc_off, _b_desc_size, a_off, a_size, _b_off, _b_size, active,
     tab_hash) = struct.unpack_from("<4sI I4xQQQQQQQQQQQB3x32s", image, 0x100)
    if count != 1:
        raise DisaError("only single-partition images (system saves) are supported")
    tab_off = pri_tab if active == 0 else sec_tab
    table = image[tab_off : tab_off + tab_size]
    if hashlib.sha256(table).digest() != tab_hash:
        raise DisaError("partition table hash mismatch")
    inner, bad_l4, l4_block = _unwrap_partition(
        table[a_desc_off : a_desc_off + a_desc_size], image[a_off : a_off + a_size])

    magic, _ver, fs_off, _size, _bs, _zero = struct.unpack_from("<IIQQII", inner, 0)
    if magic != SAVE_MAGIC:
        raise DisaError("bad SAVE magic")
    (_z, block_size, _dh_off, _dh_n, _fh_off, _fh_n, fat_off, fat_n, data_off,
     data_n) = struct.unpack_from("<IIQI4xQI4xQI4xQI4x", inner, fs_off)
    dir_blk, dir_blocks, _dmax, file_blk, file_blocks, _fmax = struct.unpack_from(
        "<III4xIII4x", inner, fs_off + 0x48)
    fat = [struct.unpack_from("<II", inner, fat_off + i * 8) for i in range(fat_n + 1)]

    def chain(start):
        cur = start + 1
        seen = 0
        while cur != 0:
            _u, v = fat[cur]
            end = fat[cur + 1][1] & 0x7FFFFFFF if v & 0x80000000 else cur
            for node in range(cur, end + 1):
                yield node - 1
            seen += 1
            if seen > len(fat):
                raise DisaError("FAT chain loops")
            cur = v & 0x7FFFFFFF

    def read_chain(start, length):
        out = bytearray()
        for block in chain(start):
            if len(out) >= length:
                break
            pos = data_off + block * block_size
            if any(p // l4_block in bad_l4 for p in (pos, pos + block_size - 1)):
                raise DisaError("file data fails the IVFC hash check")
            out += inner[pos : pos + block_size]
        if len(out) < length:
            raise DisaError("FAT chain shorter than file")
        return bytes(out[:length])

    dirs = read_chain(dir_blk, dir_blocks * block_size)
    files = read_chain(file_blk, file_blocks * block_size)
    result = {}

    def walk(index, path):
        _parent, name, sibling, child, first_file, _u, _c = struct.unpack_from(
            "<I16sIIIII", dirs, index * 0x28)
        here = path if index == 1 else path + name.rstrip(b"\0").decode() + "/"
        f = first_file
        while f:
            _p, fname, fnext, blk, size, _u2, _c2 = struct.unpack_from(
                "<I16sI4xIQII", files, f * 0x30)
            result[here + fname.rstrip(b"\0").decode()] = read_chain(blk, size) if size else b""
            f = fnext
        if child:
            walk(child, here)
        if sibling and index != 1:
            walk(sibling, path)

    walk(1, "")
    return result


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    with open(sys.argv[1], "rb") as f:
        image = f.read()
    try:
        files = read_files(image)
    except DisaError as e:
        sys.exit(f"error: {e}")
    for path, content in files.items():
        target = os.path.join(sys.argv[2], path)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, "wb") as f:
            f.write(content)
        print(f"{path}: {len(content)} bytes")


if __name__ == "__main__":
    main()
