"""Transcribe a supplied MuseScore PDF and synthesize a new 8-bit-style WAV.

No recording is read or sampled. The intermediate JSON stores the extracted
note pitches, measure positions and durations so the WAV can be re-rendered
without keeping the user-supplied PDF in the repository.

Usage:
  python tools/render_score_chiptune.py --pdf "score.pdf" \
      --notes Audio/music/haruhikage_score_notes.json \
      --output Audio/music/haruhikage_8bit.wav
  python tools/render_score_chiptune.py \
      --notes Audio/music/haruhikage_score_notes.json \
      --output Audio/music/haruhikage_8bit.wav

The PDF was engraved by MuseScore Studio 4.4.4. Its embedded SMuFL notehead
positions supply pitches; note spacing, flags and augmentation dots supply
rhythm. The PDF is a piano arrangement, so the generated pulse/triangle
orchestration is intentionally different from the original recording.
"""

from __future__ import annotations

import argparse
import itertools
import json
import wave
from collections import Counter
from pathlib import Path

import numpy as np

SAMPLE_RATE = 16000
PRESETS = {
    "haruhikage": {
        "source": "HalcyonMusic CRYCHIC Haruhikage 6-page piano score",
        "meter": "6/8", "eighths_per_bar": 6, "eighth_bpm": 194,
        "key_sharps": "FCGDA", "measure_count": 135,
    },
    "killkiss": {
        "source": "HalcyonMusic Ave Mujica KiLLKiSS 6-page piano score",
        "meter": "4/4", "eighths_per_bar": 8, "eighth_bpm": 150,
        "key_sharps": "F", "measure_count": 146,
        # The prelude marks quarter ~75, then poco a poco accelerando;
        # rehearsal A (measure 13) explicitly marks quarter = 200.
        "tempo_changes": [
            {"measure": measure, "eighth_bpm": 2 * bpm}
            for measure, bpm in [(5, 90), (6, 105), (7, 120), (8, 135),
                                 (9, 150), (10, 165), (11, 180),
                                 (12, 190), (13, 200)]
        ],
    },
}
HEADS = {"\ue0a2", "\ue0a3", "\ue0a4", "\ue0a9"}
FLAGS = {"\ue240": 1, "\ue241": 1, "\ue242": 2, "\ue243": 2}
DOT = "\ue1e7"
ACCIDENTALS = {"\ue260": -1, "\ue261": 0, "\ue262": 1, "\ue263": 2}
NATURAL = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
LETTERS = "CDEFGAB"


def glyphs(page):
    result = []
    for block in page.get_text("rawdict")["blocks"]:
        for line in block.get("lines", []):
            for span in line["spans"]:
                for char in span["chars"]:
                    if char["c"] in HEADS | set(FLAGS) | {DOT} | set(ACCIDENTALS):
                        x, y = char["origin"]
                        result.append((char["c"], x, y))
    return result


def staff_pairs(page):
    # MuseScore draws five horizontal staff lines as vector paths. Pair each
    # treble staff with the bass staff immediately below it.
    drawings = page.get_drawings()
    counts = Counter(round(p["rect"].y0, 2) for p in drawings
                     if abs(p["rect"].y1 - p["rect"].y0) < .02
                     and p["rect"].width > 50)
    rows = sorted(y for y, count in counts.items() if count >= 3)
    groups = []
    for y in rows:
        if not groups or y - groups[-1][-1] > 5.2:
            groups.append([y])
        else:
            groups[-1].append(y)
    groups = [group for group in groups if len(group) == 5]
    if len(groups) % 2:
        raise ValueError("Could not pair treble/bass staves")
    for index in range(0, len(groups), 2):
        treble, bass = groups[index][0], groups[index + 1][0]
        segments = sorted({(round(p["rect"].x0, 2), round(p["rect"].x1, 2))
                           for p in drawings
                           if abs(p["rect"].y0 - treble) < .03
                           and abs(p["rect"].y1 - treble) < .03
                           and p["rect"].width > 50})
        if not segments:
            raise ValueError("Staff has no measure segments")
        yield treble, bass, segments


def pitch(y, staff_top, treble, sharp_key):
    # One diatonic step is half a staff-line separation (2.4803 PDF points).
    steps = round((staff_top - y) / 2.4803)
    base = 5 * 7 + LETTERS.index("F") if treble else 3 * 7 + LETTERS.index("A")
    diatonic = base + steps
    octave, letter_index = divmod(diatonic, 7)
    letter = LETTERS[letter_index]
    return 12 * (octave + 1) + NATURAL[letter] + (letter in sharp_key), letter, octave


def partitions(total, pieces):
    # Half-eighth-note slots; every printed onset advances by at least one.
    for cuts in itertools.combinations(range(1, total), pieces - 1):
        yield tuple(b - a for a, b in zip((0, *cuts), (*cuts, total)))


def onset_slots(groups, right_edge, eighths_per_bar):
    count = len(groups)
    if count == 1:
        return [0.0]
    if count > 2 * eighths_per_bar:
        # Rare ornament clusters cannot fit the usual half-eighth grid; retain
        # visual ordering at an evenly spaced sub-eighth resolution.
        return [eighths_per_bar * i / count for i in range(count)]
    xs = [group[0][1] for group in groups]
    gaps = np.diff([*xs, right_edge]).astype(np.float64)
    best = (float("inf"), None)
    for lengths in partitions(2 * eighths_per_bar, count):
        durations = np.asarray(lengths, dtype=np.float64) / 2
        root = np.sqrt(durations)
        spacing = float(np.dot(gaps, root) / np.dot(root, root))
        cost = float(np.mean(((gaps - spacing * root) / max(spacing, 1)) ** 2))
        # An isolated printed flag is an eighth (two flags: sixteenth).
        # Allow the next event to belong to a different piano voice.
        for index, group in enumerate(groups[:-1]):
            flags = [item[4] for item in group if item[4] is not None]
            if flags:
                expected = .5 if max(flags) == 2 else 1.0
                if durations[index] > expected:
                    cost += 1.8 * (durations[index] - expected) ** 2
        if cost < best[0]:
            best = cost, durations
    assert best[1] is not None
    return [sum(best[1][:index]) for index in range(count)]


def extract(pdf_path, preset):
    import fitz  # PyMuPDF is only needed for the one-time PDF transcription.

    config = PRESETS[preset]
    eighths_per_bar = config["eighths_per_bar"]
    sharp_key = set(config["key_sharps"])
    document = fitz.open(str(pdf_path))
    measures = []
    for page_number, page in enumerate(document, 1):
        symbols = glyphs(page)
        systems = list(staff_pairs(page))
        for system_index, (treble, bass, segments) in enumerate(systems):
            cutoff = (treble + 19.841 + bass) / 2
            upper_limit = ((systems[system_index - 1][1] + 19.841 + treble) / 2
                           if system_index else treble - 48)
            lower_limit = ((bass + 19.841 + systems[system_index + 1][0]) / 2
                           if system_index + 1 < len(systems) else bass + 80)
            for left, right in segments:
                heads = []
                for char, x, y in symbols:
                    if char not in HEADS or not left + 1.5 < x < right - .7:
                        continue
                    is_treble = y < cutoff
                    staff_y = treble if is_treble else bass
                    if not (upper_limit < y < lower_limit):
                        continue
                    midi, letter, octave = pitch(y, staff_y, is_treble, sharp_key)
                    nearby = [(g, gx, gy) for g, gx, gy in symbols
                              if x - 4 < gx < x + 13 and abs(gy - y) < 5.5]
                    dot = any(g == DOT and 6 < gx - x < 12 for g, gx, gy in nearby)
                    flags = [FLAGS[g] for g, gx, gy in nearby
                             if g in FLAGS and abs(gx - x) < 3]
                    accidental = [(abs(x - gx), ACCIDENTALS[g])
                                  for g, gx, gy in symbols
                                  if g in ACCIDENTALS and left < gx < x
                                  and x - gx < 14 and abs(gy - y) < 2.6]
                    if accidental:
                        midi += min(accidental)[1] - (letter in sharp_key)
                    heads.append((char, x, y, midi, max(flags) if flags else None,
                                  dot, is_treble))
                heads.sort(key=lambda item: item[1])
                groups = []
                for head in heads:
                    if groups and head[1] - groups[-1][0][1] < 6.0:
                        groups[-1].append(head)
                    else:
                        groups.append([head])
                if not groups:
                    measures.append({"page": page_number, "notes": []})
                    continue
                positions = onset_slots(groups, right, eighths_per_bar)
                notes = []
                for index, (group, start) in enumerate(zip(groups, positions)):
                    for char, x, y, midi, flag, dot, is_treble in group:
                        # The other hand can play between two notes of this
                        # hand; that must not shorten the held melody/chord.
                        next_start = next((positions[following]
                                           for following in range(index + 1, len(groups))
                                           if any(head[6] == is_treble
                                                  for head in groups[following])), eighths_per_bar)
                        if char in ("\ue0a2", "\ue0a9"):
                            duration = eighths_per_bar
                        elif char == "\ue0a3":
                            duration = 6 if dot else 4
                        elif flag:
                            duration = (.5 if flag == 2 else 1) * (1.5 if dot else 1)
                        elif dot:
                            duration = 3 if next_start - start >= 1.75 else 1.5
                        else:
                            duration = max(.5, min(next_start - start, 2))
                        duration = min(duration, eighths_per_bar - start)
                        notes.append([round(start, 2), round(duration, 2), midi,
                                      "treble" if is_treble else "bass"])
                measures.append({"page": page_number, "notes": notes})
    if len(measures) != config["measure_count"]:
        raise ValueError(f"Expected {config['measure_count']} printed measures, "
                         f"found {len(measures)}")
    return {key: value for key, value in config.items() if key != "measure_count"} | {
        "measures": measures}


def add_note(mix, start_seconds, duration_seconds, midi, voice, chord_index):
    start = round(start_seconds * SAMPLE_RATE)
    size = max(1, round(duration_seconds * SAMPLE_RATE))
    size = min(size, len(mix) - start)
    if size <= 0:
        return
    t = np.arange(size, dtype=np.float32) / SAMPLE_RATE
    frequency = 440.0 * 2 ** ((midi - 69) / 12)
    phase = np.mod(t * frequency, 1.0)
    if voice == "bass":
        # NES-style triangle channel: pure enough to hold the piano bass.
        oscillator = (4 * np.abs(phase - .5) - 1).astype(np.float32)
        gain = .085 if midi < 60 else .055
    else:
        duty = .25 if chord_index == 0 else .125
        oscillator = np.where(phase < duty, 1 - duty, -duty).astype(np.float32)
        gain = .13 if chord_index == 0 else .075
    # Very short envelope removes clicks while keeping a crisp chip attack.
    attack = np.minimum(1, t / .004)
    release = np.minimum(1, (duration_seconds - t) / .024)
    envelope = np.maximum(0, np.minimum(attack, release))
    mix[start:start + size] += oscillator * envelope * gain


def render(score):
    eighths_per_bar = score.get("eighths_per_bar", 6)
    changes = {change["measure"]: change["eighth_bpm"]
               for change in score.get("tempo_changes", [])}
    measure_times = []
    measure_seconds = []
    total = 0.0
    tempo = score["eighth_bpm"]
    for measure_number in range(1, len(score["measures"]) + 1):
        tempo = changes.get(measure_number, tempo)
        measure_times.append(total)
        measure_seconds.append(60 / tempo)
        total += eighths_per_bar * measure_seconds[-1]
    mix = np.zeros(round((total + .12) * SAMPLE_RATE), dtype=np.float32)
    for measure_index, measure in enumerate(score["measures"]):
        chords = {}
        for start, duration, midi, voice in measure["notes"]:
            chords.setdefault((start, voice), []).append((duration, midi))
        for (start, voice), chord in chords.items():
            # The upper note is the lead pulse; chord tones stay quieter.
            for chord_index, (duration, midi) in enumerate(
                    sorted(chord, key=lambda item: -item[1])):
                add_note(mix, measure_times[measure_index] + start * measure_seconds[measure_index],
                         duration * measure_seconds[measure_index] * .96,
                         midi, voice, chord_index)
    mix = np.tanh(mix * 1.2)
    mix *= .88 / max(float(np.max(np.abs(mix))), 1e-6)
    edge = round(.035 * SAMPLE_RATE)
    mix[:edge] *= np.linspace(0, 1, edge, dtype=np.float32)
    mix[-edge:] *= np.linspace(1, 0, edge, dtype=np.float32)
    # Small amplitude steps supply a chip texture, without using recorded audio.
    return np.round(mix * 112).astype(np.float32) / 112


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preset", choices=PRESETS, default="haruhikage")
    parser.add_argument("--pdf", type=Path, help="Supplied MuseScore-engraved PDF")
    parser.add_argument("--notes", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.pdf:
        score = extract(args.pdf, args.preset)
        args.notes.parent.mkdir(parents=True, exist_ok=True)
        args.notes.write_text(json.dumps(score, ensure_ascii=False,
                                        separators=(",", ":")), encoding="utf-8")
    else:
        score = json.loads(args.notes.read_text(encoding="utf-8"))
    samples = render(score)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(args.output), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes((samples * 32767).astype("<i2").tobytes())
    count = sum(len(measure["notes"]) for measure in score["measures"])
    print(f"Rendered {len(score['measures'])} measures, {count} score notes, "
          f"{len(samples) / SAMPLE_RATE:.1f}s -> {args.output}")


if __name__ == "__main__":
    main()
