"""Generate the game's original, short 16-bit PCM sound effects.

Run from the repository root: python tools/generate_sfx.py
"""

from __future__ import annotations

import math
import random
import struct
import wave
from pathlib import Path


RATE = 22050
OUTPUT = Path(__file__).resolve().parents[1] / "Audio" / "sfx"
RNG = random.Random(81729)


def tone(notes, duration, *, shape="sine", noise=0.0, decay=5.0, attack=0.008):
    samples = []
    for index in range(int(RATE * duration)):
        t = index / RATE
        envelope = min(1.0, t / attack) * math.exp(-decay * t / duration)
        value = 0.0
        for frequency, gain, start in notes:
            if t < start:
                continue
            phase = 2 * math.pi * frequency * (t - start)
            if shape == "triangle":
                wave_value = 2 / math.pi * math.asin(math.sin(phase))
            else:
                wave_value = math.sin(phase)
            value += gain * wave_value
        value += noise * (RNG.random() * 2 - 1)
        samples.append(max(-1.0, min(1.0, value * envelope)))
    return samples


def write(name, samples):
    OUTPUT.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT / f"{name}.wav"), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(RATE)
        wav.writeframes(b"".join(struct.pack("<h", int(sample * 23000)) for sample in samples))


def main():
    # Musical palette: soft pentatonic chimes for UI/puzzles; muted percussion
    # for movement/combat. All sounds are synthesized here, without samples.
    cues = {
        "menu_select": ([(523, .38, 0), (784, .28, .035)], .17, "sine", .0, 5),
        "step": ([(175, .15, 0)], .075, "triangle", .10, 12),
        "door": ([(262, .35, 0), (392, .28, .08), (523, .22, .16)], .37, "triangle", .05, 4),
        "pickup": ([(659, .35, 0), (880, .26, .055), (1047, .2, .11)], .3, "sine", .0, 5),
        "battle": ([(196, .45, 0), (247, .23, .045)], .22, "triangle", .22, 6),
        "victory": ([(392, .3, 0), (523, .28, .09), (659, .28, .18), (784, .25, .27)], .53, "sine", .0, 3.4),
        "blocked": ([(220, .25, 0), (185, .23, .055)], .18, "triangle", .06, 7),
        "stairs": ([(330, .28, 0), (440, .25, .07), (660, .24, .14)], .37, "sine", .0, 4),
        "dialogue": ([(587, .17, 0), (659, .14, .035)], .14, "sine", .0, 6),
        "shop": ([(440, .3, 0), (554, .24, .075), (659, .22, .15)], .39, "sine", .0, 4),
        "puzzle_slide": ([(330, .26, 0), (392, .15, .025)], .13, "triangle", .045, 8),
        "undo": ([(494, .21, 0), (370, .2, .05)], .19, "sine", .0, 6),
        "puzzle_solved": ([(392, .25, 0), (494, .24, .11), (587, .23, .22), (784, .23, .33)], .72, "sine", .0, 2.5),
        "merge_move": ([(220, .18, 0)], .09, "triangle", .04, 9),
        "merge_small": ([(392, .26, 0), (587, .23, .045)], .21, "sine", .0, 6),
        "merge_large": ([(392, .3, 0), (587, .24, .07), (784, .22, .14)], .37, "sine", .0, 4),
        "merge_goal": ([(392, .28, 0), (494, .28, .11), (587, .25, .22), (784, .25, .33), (988, .2, .44)], .79, "sine", .0, 2.7),
    }
    for name, (notes, duration, shape, noise, decay) in cues.items():
        write(name, tone(notes, duration, shape=shape, noise=noise, decay=decay))
        print(name)


if __name__ == "__main__":
    main()
