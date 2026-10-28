#!/usr/bin/env python3
"""Synthesize the UI sound effects into romfs/sfx/.

Every sound is generated here from sine waves and seeded noise, so the
files are our own work and rebuild byte for byte:

    python3 tools/make_sfx.py            # writes romfs/sfx/*.wav
    python3 tools/make_sfx.py --check    # fails if the files are out of date

Format: 16-bit PCM, mono, 32000 Hz, each sound under 300 ms.
"""

import argparse
import io
import math
import os
import random
import struct
import sys
import wave

RATE = 32000
MAX_MS = 300


def n_samples(ms):
    return int(RATE * ms / 1000)


def env_ad(i, n, attack_ms=2.0, decay=6.0):
    """Short linear attack, exponential decay, and a fade to exactly 0 at the end."""
    t = i / RATE
    a = n_samples(attack_ms)
    attack = min(1.0, i / a) if a else 1.0
    tail = min(1.0, (n - i) / n_samples(4))  # 4 ms fade-out avoids a click
    return attack * math.exp(-decay * t / (n / RATE)) * tail


def tone(ms, f0, f1=None, decay=6.0, partials=((1.0, 1.0),), attack_ms=2.0):
    """Sine tone gliding exponentially from f0 to f1 with optional partials (ratio, gain)."""
    n = n_samples(ms)
    f1 = f1 or f0
    out = []
    phase = [0.0] * len(partials)
    for i in range(n):
        f = f0 * (f1 / f0) ** (i / n)
        s = 0.0
        for k, (ratio, gain) in enumerate(partials):
            phase[k] += 2 * math.pi * f * ratio / RATE
            s += gain * math.sin(phase[k])
        out.append(s * env_ad(i, n, attack_ms, decay))
    return out


def noise(ms, seed, cutoff0, cutoff1, decay=4.0, attack_ms=2.0, swell=False):
    """Seeded white noise through a one-pole low-pass that sweeps from cutoff0 to cutoff1 Hz."""
    rng = random.Random(seed)
    n = n_samples(ms)
    out = []
    y = 0.0
    for i in range(n):
        fc = cutoff0 * (cutoff1 / cutoff0) ** (i / n)
        alpha = 1.0 - math.exp(-2 * math.pi * fc / RATE)
        y += alpha * (rng.uniform(-1.0, 1.0) - y)
        if swell:
            # Rise and fall like a swipe: sine-shaped envelope.
            e = math.sin(math.pi * i / n) ** 1.5
        else:
            e = env_ad(i, n, attack_ms, decay)
        out.append(y * e)
    return out


def silence(ms):
    return [0.0] * n_samples(ms)


def mix(*tracks):
    n = max(len(t) for t in tracks)
    return [sum(t[i] for t in tracks if i < len(t)) for i in range(n)]


def at(ms, track):
    return silence(ms) + track


def normalize(samples, peak):
    m = max(abs(s) for s in samples) or 1.0
    return [s * peak / m for s in samples]


BELL = ((1.0, 1.0), (2.0, 0.35), (2.76, 0.18), (5.4, 0.06))
SOFT_SQUARE = ((1.0, 1.0), (3.0, 0.22), (5.0, 0.08))


def note(name):
    names = {"C": -9, "D": -7, "E": -5, "F": -4, "G": -2, "A": 0, "B": 2}
    semis = names[name[0]] + 12 * (int(name[-1]) - 4)
    return 440.0 * 2 ** (semis / 12)


def build():
    """Returns {file name: samples}. Peaks set the relative loudness of the set."""
    s = {}
    # Very short, quiet tick.
    s["move"] = normalize(tone(18, 2600, 2200, decay=7.0, partials=((1.0, 1.0), (2.0, 0.2))), 0.22)
    # Bright pop: fast upward glide.
    s["select"] = normalize(tone(70, 520, 1250, decay=5.0, partials=((1.0, 1.0), (2.0, 0.15))), 0.5)
    # Same pop, a fifth lower and gliding down.
    s["deselect"] = normalize(tone(70, 840, 350, decay=5.0, partials=((1.0, 1.0), (2.0, 0.15))), 0.45)
    # Soft click: short mid tone plus a breath of noise.
    s["confirm"] = normalize(
        mix(tone(45, 1150, 950, decay=6.0, partials=SOFT_SQUARE), [0.25 * v for v in noise(25, 1, 6000, 2500)]),
        0.5,
    )
    # Dull short click: low tone, filtered noise.
    s["back"] = normalize(
        mix(tone(55, 330, 200, decay=6.0), [0.35 * v for v in noise(30, 2, 1800, 600)]), 0.5
    )
    # Swipe: noise swelling up and down while the filter opens.
    s["page"] = normalize(noise(150, 3, 900, 5200, swell=True), 0.4)
    # Two rising bell tones.
    s["buy"] = normalize(
        mix(tone(150, note("E6"), decay=4.5, partials=BELL), at(110, tone(180, note("B6"), decay=4.0, partials=BELL))),
        0.55,
    )
    # Quiet clink: two inharmonic partials.
    s["coin"] = normalize(tone(60, 2350, decay=6.0, partials=((1.0, 1.0), (1.51, 0.6), (2.9, 0.15))), 0.28)
    # Short fanfare: three rising tones, the last one held.
    s["complete"] = normalize(
        mix(
            tone(90, note("C5"), decay=2.5, partials=SOFT_SQUARE),
            at(85, tone(90, note("E5"), decay=2.5, partials=SOFT_SQUARE)),
            at(170, tone(125, note("G5"), decay=3.0, partials=SOFT_SQUARE)),
        ),
        0.55,
    )
    # Two low, short tones.
    s["error"] = normalize(
        mix(
            tone(70, 196, decay=3.0, partials=SOFT_SQUARE),
            at(100, tone(80, 165, decay=3.0, partials=SOFT_SQUARE)),
        ),
        0.55,
    )
    return s


def encode(samples):
    assert len(samples) <= n_samples(MAX_MS), "sound longer than %d ms" % MAX_MS
    pcm = b"".join(struct.pack("<h", max(-32767, min(32767, round(v * 32767)))) for v in samples)
    buf = io.BytesIO()
    with wave.open(buf, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm)
    return buf.getvalue()


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", default=os.path.join(here, "..", "romfs", "sfx"))
    ap.add_argument("--check", action="store_true", help="compare instead of writing")
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)
    stale = []
    for name, samples in build().items():
        data = encode(samples)
        path = os.path.join(args.out, name + ".wav")
        if args.check:
            try:
                with open(path, "rb") as f:
                    if f.read() != data:
                        stale.append(path)
            except FileNotFoundError:
                stale.append(path)
            continue
        with open(path, "wb") as f:
            f.write(data)
        print("%-14s %4d ms" % (name + ".wav", len(samples) * 1000 // RATE))

    if stale:
        print("out of date, run tools/make_sfx.py:", *stale, sep="\n  ", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
