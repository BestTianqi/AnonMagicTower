"""Regression checks for the score-derived chiptune asset."""

import json
import unittest
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class ScoreChiptuneTest(unittest.TestCase):
    def check_notes(self, score, eighths_per_bar):
        for bar in score["measures"]:
            for start, duration, midi, voice in bar["notes"]:
                self.assertGreater(duration, 0)
                self.assertGreaterEqual(start, 0)
                self.assertLessEqual(start + duration, eighths_per_bar)
                self.assertIn(voice, ("treble", "bass"))
                self.assertTrue(20 <= midi <= 110)

    def test_score_notes_and_wav(self):
        score = json.loads((ROOT / "Audio/music/haruhikage_score_notes.json")
                           .read_text(encoding="utf-8"))
        self.assertEqual(score["meter"], "6/8")
        self.assertEqual(score["eighth_bpm"], 194)
        self.assertEqual(len(score["measures"]), 135)
        self.assertEqual(sum(len(bar["notes"]) for bar in score["measures"]), 2068)
        # Opening measure: B5/D#6, C#6, B5, C#6 over B major.
        melody = [(note[0], note[2]) for note in score["measures"][0]["notes"]
                  if note[3] == "treble"]
        self.assertEqual(melody, [(0, 83), (0, 87), (2, 85), (3, 83), (5, 85)])
        self.check_notes(score, 6)
        with wave.open(str(ROOT / "Audio/music/haruhikage_8bit.wav"), "rb") as audio:
            self.assertEqual(audio.getnchannels(), 1)
            self.assertEqual(audio.getsampwidth(), 2)
            self.assertEqual(audio.getframerate(), 16000)
            self.assertAlmostEqual(audio.getnframes() / 16000, 250.6, delta=.2)

    def test_killkiss_score_and_wav(self):
        score = json.loads((ROOT / "Audio/music/killkiss_score_notes.json")
                           .read_text(encoding="utf-8"))
        self.assertEqual(score["meter"], "4/4")
        self.assertEqual(score["key_sharps"], "F")
        self.assertEqual(score["eighth_bpm"], 150)
        self.assertEqual(score["tempo_changes"][-1],
                         {"measure": 13, "eighth_bpm": 400})
        self.assertEqual(len(score["measures"]), 146)
        self.assertEqual(sum(len(bar["notes"]) for bar in score["measures"]), 2465)
        self.assertEqual([note[2] for note in score["measures"][0]["notes"]
                          if note[3] == "bass"], [28, 40])
        self.check_notes(score, 8)
        with wave.open(str(ROOT / "Audio/music/killkiss_8bit.wav"), "rb") as audio:
            self.assertEqual(audio.getnchannels(), 1)
            self.assertEqual(audio.getsampwidth(), 2)
            self.assertEqual(audio.getframerate(), 16000)
            self.assertAlmostEqual(audio.getnframes() / 16000, 188.1, delta=.2)


if __name__ == "__main__":
    unittest.main()
