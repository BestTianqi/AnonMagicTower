"""Export portrait and 60x60 model candidates from transparent two-up boards."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def alpha_bbox(image: Image.Image, threshold: int = 12) -> tuple[int, int, int, int]:
    alpha = image.getchannel("A").point(lambda value: 255 if value > threshold else 0)
    bbox = alpha.getbbox()
    if bbox is None:
        raise ValueError("image has no visible pixels")
    return bbox


def padded_bbox(
    bbox: tuple[int, int, int, int], width: int, height: int, padding: int
) -> tuple[int, int, int, int]:
    left, top, right, bottom = bbox
    return (
        max(0, left - padding),
        max(0, top - padding),
        min(width, right + padding),
        min(height, bottom + padding),
    )


def render_model(model: Image.Image, size: int) -> Image.Image:
    scale_factor = size / 60
    usable_width = round(54 * scale_factor)
    usable_height = round(56 * scale_factor)
    bottom_guard = round(2 * scale_factor)
    scale = min(usable_width / model.width, usable_height / model.height)
    output_size = (
        max(1, round(model.width * scale)),
        max(1, round(model.height * scale)),
    )
    model = model.resize(output_size, Image.Resampling.LANCZOS)
    tile = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    tile.alpha_composite(
        model,
        ((size - output_size[0]) // 2, size - bottom_guard - output_size[1]),
    )
    return tile


def export_board(
    board: Path, portrait_dir: Path, model_dir: Path, model_hd_dir: Path
) -> None:
    with Image.open(board) as source:
        source = source.convert("RGBA")
        midpoint = source.width // 2
        portrait_half = source.crop((0, 0, midpoint, source.height))
        model_half = source.crop((midpoint, 0, source.width, source.height))

        portrait_box = padded_bbox(
            alpha_bbox(portrait_half), portrait_half.width, portrait_half.height, 8
        )
        model_box = padded_bbox(alpha_bbox(model_half), model_half.width, model_half.height, 5)
        portrait = portrait_half.crop(portrait_box)
        model = model_half.crop(model_box)

        stem = board.stem.removesuffix("_board")
        portrait.save(portrait_dir / f"{stem}_portrait.png")

        render_model(model, 60).save(model_dir / f"{stem}_model_60.png")
        render_model(model, 240).save(model_hd_dir / f"{stem}_model_240.png")


def contact_sheet(
    images: list[Path], out: Path, *, thumb: tuple[int, int], columns: int
) -> None:
    label_height = 38
    rows = (len(images) + columns - 1) // columns
    cell_width, cell_height = thumb[0] + 16, thumb[1] + label_height + 16
    sheet = Image.new("RGB", (cell_width * columns, cell_height * rows), "#20242c")
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default()
    for index, path in enumerate(images):
        row, column = divmod(index, columns)
        x, y = column * cell_width + 8, row * cell_height + 8
        with Image.open(path) as image:
            image = image.convert("RGBA")
            image.thumbnail(thumb, Image.Resampling.LANCZOS)
            preview = Image.new("RGBA", thumb, (236, 238, 244, 255))
            preview.alpha_composite(
                image, ((thumb[0] - image.width) // 2, thumb[1] - image.height)
            )
            sheet.paste(preview.convert("RGB"), (x, y))
        label = path.stem.replace("_portrait", "").replace("_model_60", "")
        draw.text((x, y + thumb[1] + 4), label[:36], fill="white", font=font)
    sheet.save(out, quality=95)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    args = parser.parse_args()
    root = Path(args.root)
    board_dir = root / "transparent_boards"
    portrait_dir = root / "portraits"
    model_dir = root / "models_60"
    model_hd_dir = root / "models_240"
    portrait_dir.mkdir(parents=True, exist_ok=True)
    model_dir.mkdir(parents=True, exist_ok=True)
    model_hd_dir.mkdir(parents=True, exist_ok=True)

    boards = sorted(board_dir.glob("*_board.png"))
    if not boards:
        raise SystemExit("no transparent boards found")
    for board in boards:
        export_board(board, portrait_dir, model_dir, model_hd_dir)

    portraits = sorted(portrait_dir.glob("*_portrait.png"))
    models = sorted(model_dir.glob("*_model_60.png"))
    contact_sheet(
        portraits,
        root / "comparison_portraits.png",
        thumb=(220, 300),
        columns=5,
    )
    contact_sheet(
        models,
        root / "comparison_models_60.png",
        thumb=(120, 120),
        columns=6,
    )
    hd_models = sorted(model_hd_dir.glob("*_model_240.png"))
    print(
        f"boards={len(boards)} portraits={len(portraits)} "
        f"models60={len(models)} models240={len(hd_models)}"
    )


if __name__ == "__main__":
    main()
