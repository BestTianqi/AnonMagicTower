"""Generate expressive source speech using the installed character GPT-SoVITS models.

Run this script with GPT-SoVITS's bundled Python. The WAV files are intermediate
inputs for generate_story_voices.py; they are not shipped with the game.
"""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from generate_story_voices import ROOT, clip_name, model_for, story_lines


GPT_ROOT = Path(r"D:\GPT_SOVITS\GPT-SoVITS-v3lora-20250228")
SPEAKERS = {
    "aiyi": {
        "gpt": GPT_ROOT / "GPT_weights_v2/Anon-e15.ckpt",
        "sovits": GPT_ROOT / "SoVITS_weights_v2/Anon_e8_s1736.pth",
        "reference": GPT_ROOT / "logs/Anon/5-wav32k/Anon.mp3_0000140480_0000291200.wav",
        "prompt_text": "いい衣装作っちゃいますから海外セレブ系はまた今度ってことで",
    },
    "sushi": {
        "gpt": GPT_ROOT / "GPT_weights_v2/Soyorin-e15.ckpt",
        "sovits": GPT_ROOT / "SoVITS_weights_v2/soyorin_e8_s600.pth",
        "reference": GPT_ROOT / "logs/Soyorin/5-wav32k/Soyojia.aac_0000123840_0000299200.wav",
        "prompt_text": "ずっと心配してたんだよ。学校休んでるし、返事も全然ないから。",
    },
    "deng": {
        "gpt": GPT_ROOT / "GPT_weights_v2/tomori-e15.ckpt",
        "sovits": GPT_ROOT / "SoVITS_weights_v2/tomori_e8_s272.pth",
        "reference": GPT_ROOT / "logs/tomori/5-wav32k/1.aac_0000032640_0000163520.wav",
        "prompt_text": "本当に、辞めちゃうの?でも…",
    },
}


def build_request(settings: dict, text: str) -> dict:
    return {
        "text": text,
        "text_lang": "zh",
        "ref_audio_path": str(settings["reference"]),
        "prompt_lang": "ja",
        "prompt_text": settings["prompt_text"],
        "text_split_method": "cut0",
        "top_k": 10,
        "top_p": 0.9,
        "temperature": 1.0,
        "repetition_penalty": 1.2,
        "batch_size": 1,
        "parallel_infer": False,
        "seed": int(hashlib.sha1(text.encode("utf-8")).hexdigest()[:8], 16),
    }


def generate(model: str, lines, output_dir: Path, force: bool = False):
    import numpy as np
    import soundfile as sf

    settings = SPEAKERS[model]
    for key in ("gpt", "sovits", "reference"):
        if not settings[key].is_file():
            raise FileNotFoundError(settings[key])
    if not 3 <= sf.info(str(settings["reference"])).duration <= 10:
        raise RuntimeError("GPT-SoVITS reference audio must be 3–10 seconds")

    output_dir.mkdir(parents=True, exist_ok=True)
    os.environ["PATH"] = str(GPT_ROOT) + os.pathsep + os.environ["PATH"]
    os.environ["TOKENIZERS_PARALLELISM"] = "false"
    sys.dont_write_bytecode = True
    os.chdir(GPT_ROOT)
    sys.path.insert(0, str(GPT_ROOT))

    from GPT_SoVITS.TTS_infer_pack.TTS import TTS, TTS_Config

    config = TTS_Config({
        "version": "v2",
        "custom": {
            "device": "cuda",
            "is_half": True,
            "version": "v2",
            "t2s_weights_path": str(settings["gpt"]),
            "vits_weights_path": str(settings["sovits"]),
            "bert_base_path": str(GPT_ROOT / "GPT_SoVITS/pretrained_models/chinese-roberta-wwm-ext-large"),
            "cnhuhbert_base_path": str(GPT_ROOT / "GPT_SoVITS/pretrained_models/chinese-hubert-base"),
        },
    })
    config.configs_path = str(ROOT / "build" / "story_voice_tmp" / f"gptsovits_{model}.yaml")
    tts = TTS(config)

    for number, (speaker, text) in enumerate(lines, 1):
        output = output_dir / clip_name(speaker, text)
        if not force and output.is_file() and output.stat().st_size > 1000:
            print(f"[{number}/{len(lines)}] cached {output.name}", flush=True)
            continue
        sample_rate, audio = next(tts.run(build_request(settings, text)))
        if sample_rate < 16000 or len(audio) < sample_rate // 2:
            raise RuntimeError(f"GPT-SoVITS produced an empty/short clip: {speaker} {text}")
        if not np.isfinite(audio).all() or float(np.sqrt(np.mean(audio.astype(np.float32)**2))) < 100:
            raise RuntimeError(f"GPT-SoVITS produced invalid/silent audio: {speaker} {text}")
        sf.write(str(output), audio, sample_rate, subtype="PCM_16")
        print(f"[{number}/{len(lines)}] GPT-SoVITS {speaker}: {output.name} ({len(audio)/sample_rate:.1f}s)", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--speaker", choices=tuple(SPEAKERS), required=True)
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--output-dir", type=Path)
    args = parser.parse_args()
    lines = [line for line in story_lines() if model_for(line[0]) == args.speaker]
    if args.limit > 0:
        lines = lines[:args.limit]
    output_dir = args.output_dir or ROOT / "build" / "story_voice_tmp" / "gpt_base" / args.speaker
    generate(args.speaker, lines, output_dir, args.force)


if __name__ == "__main__":
    main()
