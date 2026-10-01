import sys
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))


class StoryTtsPipelineTests(unittest.TestCase):
    def test_gptsovits_character_models_and_reference_audio_exist(self):
        from generate_story_tts import SPEAKERS, build_request

        for voice in ("aiyi", "sushi", "deng"):
            settings = SPEAKERS[voice]
            self.assertTrue(settings["gpt"].is_file())
            self.assertTrue(settings["sovits"].is_file())
            self.assertTrue(settings["reference"].is_file())
            request = build_request(settings, "素世，你在哪里？")
            self.assertEqual(request["text_lang"], "zh")
            self.assertEqual(request["prompt_lang"], "ja")
            self.assertEqual(request["text"], "素世，你在哪里？")
            self.assertTrue(request["prompt_text"])

    def test_rvc_keeps_original_pitch_and_uses_character_index(self):
        from generate_story_voices import MODELS, RVC_INDEX_RATE, RVC_PITCH

        self.assertEqual(RVC_PITCH, 0)
        self.assertGreater(RVC_INDEX_RATE, 0)
        self.assertTrue(MODELS["aiyi"][1].is_file())
        self.assertTrue(MODELS["sushi"][1].is_file())
        self.assertTrue(MODELS["deng"][0].is_file())
        self.assertTrue(MODELS["deng"][1].is_file())

    def test_narration_uses_tomori_voice_and_is_collected(self):
        from generate_story_voices import model_for, story_lines

        self.assertEqual(model_for("旁白"), "deng")
        narration = {text for speaker, text in story_lines() if speaker == "旁白"}
        self.assertGreaterEqual(len(narration), 30)
        self.assertIn("巫师的邻接魔法阵在脚下闭合，魔力领域对爱音造成了伤害。", narration)
        self.assertIn("快去找剑和盾吧。现在可以继续前进了。", narration)


if __name__ == "__main__":
    unittest.main()
