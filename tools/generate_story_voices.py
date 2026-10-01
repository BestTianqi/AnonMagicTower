"""Render Anon/Soyo/narration via GPT-SoVITS source WAVs -> existing RVC weights.

Run with the RVC bundled Python from the repository root. The script reads the
RVC installation and character weights without modifying them; output belongs
to this game's Audio/voice directory.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys


ROOT = Path(__file__).resolve().parents[1]
STORY_SOURCE = ROOT / "UI" / "StoryScript.h"
RVC_ROOT = Path(r"D:\RVC\RVC20240604Nvidia")
MODELS = {
    "aiyi": (Path(r"D:\RVC\爱音\爱音\aiyi.pth"), Path(r"D:\RVC\爱音\爱音\aiyi.index")),
    "sushi": (Path(r"D:\RVC\素世\素世\sushi.pth"), Path(r"D:\RVC\素世\素世\sushi.index")),
    "deng": (Path(r"D:\RVC\灯\灯\deng.pth"), Path(r"D:\RVC\灯\灯\deng.index")),
}
RVC_PITCH = 0
RVC_INDEX_RATE = 0.65
LINE_PATTERN = re.compile(r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*"((?:\\.|[^"\\])*)"\s*(?:,|\})')
EXTRA_NARRATION = (
    "五个身影已经显现：长崎素世与四名魔法警卫将你围住！\n点击地图画面继续，查看事件对话。",
    "快去找剑和盾吧。现在可以继续前进了。",
    "巫师的邻接魔法阵在脚下闭合，魔力领域对爱音造成了伤害。",
    "两名魔法警卫从相对方向同时发动夹击，爱音的生命值被削减一半。",
    "警卫摘下面具——藏在盔甲之下的人正是藤都子。",
)


def model_for(speaker: str) -> str | None:
    if "爱音" in speaker:
        return "aiyi"
    if "素世" in speaker:
        return "sushi"
    if speaker == "旁白":
        return "deng"
    return None


def clip_name(speaker: str, text: str) -> str:
    return hashlib.sha1((speaker + "\n" + text).encode("utf-8")).hexdigest() + ".wav"


def story_lines():
    source = STORY_SOURCE.read_text(encoding="utf-8")
    seen = set()
    for match in LINE_PATTERN.finditer(source):
        speaker, text = (json.loads('"' + part + '"') for part in match.groups())
        if not model_for(speaker) or (speaker, text) in seen:
            continue
        seen.add((speaker, text))
        yield speaker, text
    # These lines are assembled in MainWindow rather than StoryScript.h.
    for text in EXTRA_NARRATION:
        if ("旁白", text) not in seen:
            yield "旁白", text


def convert_lines(model: str, lines, output_dir: Path, base_dir: Path, force: bool = False):
    pth, index = MODELS[model]
    for required in (pth, index, RVC_ROOT / "assets" / "hubert" / "hubert_base.pt"):
        if not required.is_file():
            raise FileNotFoundError(required)

    scratch = ROOT / "build" / "story_voice_tmp"
    scratch.mkdir(parents=True, exist_ok=True)
    ascii_index = scratch / (model + ".index")
    if not ascii_index.is_file() or ascii_index.stat().st_size != index.stat().st_size:
        shutil.copyfile(index, ascii_index)  # FAISS on Windows cannot open non-ASCII paths.
    os.environ["weight_root"] = str(pth.parent)
    os.environ["index_root"] = str(scratch)
    os.environ["rmvpe_root"] = str(RVC_ROOT / "assets" / "rmvpe")
    os.environ["PYTHONDONTWRITEBYTECODE"] = "1"
    os.environ["PATH"] = str(RVC_ROOT) + os.pathsep + os.environ["PATH"]
    os.chdir(RVC_ROOT)
    sys.path.insert(0, str(RVC_ROOT))

    from configs.config import Config
    from infer.modules.vc.modules import VC
    import numpy as np
    import soundfile as sf

    sys.argv = sys.argv[:1]  # RVC's Config also parses argv.
    vc = VC(Config())
    vc.get_vc(pth.name)
    output_dir.mkdir(parents=True, exist_ok=True)

    for number, (speaker, text) in enumerate(lines, 1):
        output = output_dir / clip_name(speaker, text)
        if not force and output.is_file() and output.stat().st_size > 1000:
            print(f"[{number}/{len(lines)}] cached {speaker}: {output.name}", flush=True)
            continue
        base = base_dir / clip_name(speaker, text)
        if not base.is_file():
            raise FileNotFoundError(f"Generate GPT-SoVITS source speech first: {base}")
        info, audio = vc.vc_single(0, str(base), RVC_PITCH, None, "rmvpe", str(ascii_index),
                                   None, RVC_INDEX_RATE, 3, 0, 1.0, 0.33)
        if audio is None or audio[0] is None or audio[1] is None:
            raise RuntimeError(f"RVC conversion failed for {speaker}: {info}")
        rate, data = audio
        if rate < 16000 or len(data) < rate // 5 or not np.isfinite(data).all():
            raise RuntimeError(f"Invalid RVC output for {speaker}: rate={rate}, samples={len(data)}")
        sf.write(str(output), data, rate, subtype="PCM_16")
        print(f"[{number}/{len(lines)}] RVC {speaker}: {output.name} ({len(data)/rate:.1f}s)", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--speaker", choices=tuple(MODELS), required=True)
    parser.add_argument("--limit", type=int, default=0, help="Generate only this many clips (0=all)")
    parser.add_argument("--list", action="store_true", help="List clips without running RVC")
    parser.add_argument("--force", action="store_true", help="Replace existing output clips")
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--base-dir", type=Path)
    args = parser.parse_args()
    lines = [line for line in story_lines() if model_for(line[0]) == args.speaker]
    if args.limit > 0:
        lines = lines[:args.limit]
    if args.list:
        for speaker, text in lines:
            print(f"{speaker}\t{clip_name(speaker, text)}\t{text}")
        return
    base_dir = args.base_dir or ROOT / "build" / "story_voice_tmp" / "gpt_base" / args.speaker
    output_dir = args.output_dir or ROOT / "Audio" / "voice"
    convert_lines(args.speaker, lines, output_dir.resolve(), base_dir.resolve(), args.force)


if __name__ == "__main__":
    main()
