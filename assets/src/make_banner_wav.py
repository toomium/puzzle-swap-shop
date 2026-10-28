#!/usr/bin/env python3
"""Synthesizes the HOME menu banner jingle (assets/banner.wav).

Original composition, generated from scratch so the repository contains no
sampled or third-party audio: a short marimba arpeggio in C major over a soft
bass, ending on a bell chord with a few glints. Deterministic output, standard
library only.

Usage: python3 assets/src/make_banner_wav.py [out.wav]
"""
import math
import struct
import sys
import wave
from pathlib import Path

RATE = 32000
LENGTH = 2.9  # seconds; the HOME menu cuts banner sounds after 3 s
FADE = 0.35   # fade out at the very end


def hz(name):
    """Frequency of a note such as 'C5' or 'F#3' (A4 = 440 Hz)."""
    steps = {"C": -9, "D": -7, "E": -5, "F": -4, "G": -2, "A": 0, "B": 2}
    semis = steps[name[0]] + (1 if name[1] == "#" else 0)
    octave = int(name[-1])
    return 440.0 * 2 ** ((semis + 12 * (octave - 4)) / 12)


def marimba(t, f):
    # Struck bar: fundamental plus the bright 4th partial that dies fast.
    return (math.sin(2 * math.pi * f * t) * math.exp(-t * 6.0)
            + 0.35 * math.sin(2 * math.pi * f * 3.93 * t) * math.exp(-t * 22.0))


def bell(t, f):
    return (math.sin(2 * math.pi * f * t) * math.exp(-t * 2.2)
            + 0.25 * math.sin(2 * math.pi * f * 2.0 * t) * math.exp(-t * 3.5)
            + 0.12 * math.sin(2 * math.pi * f * 3.0 * t) * math.exp(-t * 6.0))


def bass(t, f):
    return math.sin(2 * math.pi * f * t) * math.exp(-t * 3.0)


def glint(t, f):
    return math.sin(2 * math.pi * f * t) * math.exp(-t * 18.0)


# (start s, voice, note, gain, length s)
B = 0.18  # one step
SCORE = [
    (0 * B, marimba, "C5", 0.55, 0.6),
    (1 * B, marimba, "E5", 0.55, 0.6),
    (2 * B, marimba, "G5", 0.55, 0.6),
    (3 * B, marimba, "A5", 0.55, 0.6),
    (4 * B, marimba, "G5", 0.50, 0.6),
    (5 * B, marimba, "E5", 0.50, 0.6),
    (6 * B, marimba, "F5", 0.55, 0.6),
    (7 * B, marimba, "D5", 0.50, 0.6),
    (0 * B, bass, "C3", 0.45, 0.7),
    (4 * B, bass, "A2", 0.45, 0.7),
    (6 * B, bass, "G2", 0.45, 0.5),
    (8 * B, bass, "C3", 0.50, 1.4),
    (8 * B, bell, "C6", 0.45, 1.5),
    (8 * B, bell, "G5", 0.30, 1.5),
    (8 * B, bell, "E5", 0.30, 1.5),
    (8 * B, marimba, "C5", 0.40, 0.8),
    (8 * B + 0.12, glint, "E7", 0.10, 0.3),
    (8 * B + 0.21, glint, "G7", 0.09, 0.3),
    (8 * B + 0.30, glint, "C8", 0.08, 0.3),
]


def render():
    n = int(RATE * LENGTH)
    mix = [0.0] * n
    attack = int(RATE * 0.004)  # 4 ms ramp against clicks
    for start, voice, note, gain, dur in SCORE:
        f = hz(note)
        s0 = int(start * RATE)
        for i in range(min(int(dur * RATE), n - s0)):
            t = i / RATE
            env = min(1.0, i / attack) if attack else 1.0
            mix[s0 + i] += gain * env * voice(t, f)
    fade = int(FADE * RATE)
    for i in range(fade):
        mix[n - fade + i] *= (1 - i / fade) ** 2
    peak = max(abs(v) for v in mix)
    scale = 0.8 * 32767 / peak
    return [int(round(v * scale)) for v in mix]


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent / "banner.wav"
    samples = render()
    with wave.open(str(out), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(struct.pack("<%dh" % len(samples), *samples))
    print("wrote %s (%.2f s, %d Hz)" % (out, len(samples) / RATE, RATE))


if __name__ == "__main__":
    main()
