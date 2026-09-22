"""Clean 60x60 fusion models by retaining and re-framing the main character."""

from __future__ import annotations

import argparse
from pathlib import Path

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont


def main_component_mask(alpha: np.ndarray) -> np.ndarray:
    height, width = alpha.shape
    binary = (alpha > 18).astype(np.uint8)
    count, labels, stats, centroids = cv2.connectedComponentsWithStats(binary, 8)
    if count <= 1:
        raise ValueError("sprite has no visible component")

    candidates: list[tuple[float, int]] = []
    for label in range(1, count):
        area = float(stats[label, cv2.CC_STAT_AREA])
        center_x, center_y = centroids[label]
        distance = abs(center_x - (width - 1) / 2) / width
        distance += 0.35 * abs(center_y - height * 0.57) / height
        score = area * max(0.1, 1.0 - 0.8 * distance)
        candidates.append((score, label))
    main_label = max(candidates)[1]

    core = np.where(labels == main_label, 255, 0).astype(np.uint8)
    # Recover the main component's antialiased fringe without bringing back
    # detached spell circles, clipped weapons, or other border fragments.
    neighborhood = cv2.dilate(core, np.ones((5, 5), np.uint8), iterations=1)
    return np.where((neighborhood > 0) & (alpha > 0), alpha, 0).astype(np.uint8)


LEFT_TRIM_BY_STEM = {
    # These two generated chibis crossed the board midpoint, leaving a large
    # partial effect on the left after extraction.  Trim only that fragment;
    # the character and its attached costume motif remain intact.
    "arale_casual_big_slime_model_60": 17,
    "yukina_stage_vampire_queen_sp_model_60": 27,
}


def clean_sprite(
    source: Image.Image,
    source_stem: str,
    canvas_size: int,
    guide: Image.Image | None = None,
) -> Image.Image:
    rgba = np.array(source.convert("RGBA"))
    scale_factor = canvas_size / 60
    if guide is not None:
        guide_alpha = np.array(
            guide.convert("RGBA")
            .getchannel("A")
            .resize((rgba.shape[1], rgba.shape[0]), Image.Resampling.NEAREST)
        )
        radius = max(1, round(scale_factor * 1.5))
        kernel_size = radius * 2 + 1
        guide_mask = cv2.dilate(
            (guide_alpha > 3).astype(np.uint8),
            np.ones((kernel_size, kernel_size), np.uint8),
            iterations=1,
        )
        clean_alpha = np.where(guide_mask > 0, rgba[:, :, 3], 0).astype(np.uint8)
    else:
        clean_alpha = main_component_mask(rgba[:, :, 3])
        normalized_stem = source_stem.replace(f"_model_{canvas_size}", "_model_60")
        left_trim = round(LEFT_TRIM_BY_STEM.get(normalized_stem, 0) * scale_factor)
        if left_trim:
            clean_alpha[:, :left_trim] = 0
    rgba[:, :, 3] = clean_alpha
    rgba[clean_alpha == 0, :3] = 0

    ys, xs = np.where(clean_alpha > 4)
    if xs.size == 0:
        raise ValueError("sprite became empty after cleaning")
    source_width, source_height = rgba.shape[1], rgba.shape[0]
    padding = max(1, round(scale_factor))
    left, right = max(0, xs.min() - padding), min(source_width, xs.max() + padding + 1)
    top, bottom = max(0, ys.min() - padding), min(source_height, ys.max() + padding + 1)
    subject = Image.fromarray(rgba, "RGBA").crop((left, top, right, bottom))

    usable_width = round(50 * scale_factor)
    usable_height = round(54 * scale_factor)
    bottom_guard = round(2 * scale_factor)
    target_root_y = canvas_size - bottom_guard - 1
    scale = min(usable_width / subject.width, usable_height / subject.height)
    size = (
        max(1, round(subject.width * scale)),
        max(1, round(subject.height * scale)),
    )
    subject = subject.resize(size, Image.Resampling.LANCZOS)
    tile = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    tile.alpha_composite(
        subject,
        ((canvas_size - size[0]) // 2, canvas_size - bottom_guard - size[1]),
    )

    result = np.array(tile)
    result[result[:, :, 3] <= 3] = 0
    visible_y = np.where(result[:, :, 3] > 3)[0]
    if visible_y.size and visible_y.max() < target_root_y:
        shift = int(target_root_y - visible_y.max())
        shifted = np.zeros_like(result)
        shifted[shift:] = result[:-shift]
        result = shifted
    return Image.fromarray(result, "RGBA")


def checkerboard(size: tuple[int, int], cell: int = 12) -> Image.Image:
    width, height = size
    board = Image.new("RGB", size, "#f1f3f7")
    draw = ImageDraw.Draw(board)
    for y in range(0, height, cell):
        for x in range(0, width, cell):
            if (x // cell + y // cell) % 2:
                draw.rectangle((x, y, x + cell - 1, y + cell - 1), fill="#d9dde5")
    return board


def comparison(images: list[Path], out: Path, canvas_size: int) -> None:
    columns = 5 if canvas_size >= 120 else 6
    preview_size = 240 if canvas_size >= 120 else 180
    label_height = 34
    rows = (len(images) + columns - 1) // columns
    sheet = Image.new(
        "RGB", (columns * preview_size, rows * (preview_size + label_height)), "#20242c"
    )
    font = ImageFont.load_default()
    draw = ImageDraw.Draw(sheet)
    for index, path in enumerate(images):
        row, column = divmod(index, columns)
        x, y = column * preview_size, row * (preview_size + label_height)
        preview = checkerboard((preview_size, preview_size))
        with Image.open(path) as sprite:
            sprite = sprite.convert("RGBA").resize(
                (preview_size, preview_size),
                Image.Resampling.LANCZOS if canvas_size >= 120 else Image.Resampling.NEAREST,
            )
            preview.paste(sprite, (0, 0), sprite)
        sheet.paste(preview, (x, y))
        label = path.stem.replace(f"_model_{canvas_size}_clean", "")
        draw.text((x + 4, y + preview_size + 5), label[:31], fill="white", font=font)
    sheet.save(out, quality=95)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-dir", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--comparison", required=True)
    parser.add_argument("--size", type=int, default=60)
    parser.add_argument("--guide-dir")
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)
    guide_dir = Path(args.guide_dir) if args.guide_dir else None
    output_dir.mkdir(parents=True, exist_ok=True)
    outputs: list[Path] = []
    for source_path in sorted(input_dir.glob(f"*_model_{args.size}.png")):
        with Image.open(source_path) as source:
            try:
                guide = None
                if guide_dir is not None:
                    guide_name = source_path.name.replace(
                        f"_model_{args.size}.png", "_model_60_clean.png"
                    )
                    guide_path = guide_dir / guide_name
                    if not guide_path.exists():
                        raise ValueError(f"missing guide {guide_path}")
                    guide = Image.open(guide_path)
                try:
                    result = clean_sprite(source, source_path.stem, args.size, guide)
                finally:
                    if guide is not None:
                        guide.close()
            except ValueError as error:
                raise ValueError(f"{source_path.name}: {error}") from error
        output_path = output_dir / source_path.name.replace(
            f"_model_{args.size}.png", f"_model_{args.size}_clean.png"
        )
        result.save(output_path)
        outputs.append(output_path)
    if not outputs:
        raise SystemExit("no 60x60 sprites found")
    comparison(outputs, Path(args.comparison), args.size)
    print(f"cleaned={len(outputs)}")


if __name__ == "__main__":
    main()
