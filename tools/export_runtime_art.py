from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
TILES = ROOT / "images" / "tiles"
OUT_TILES = ROOT / "images" / "runtime" / "tiles"
OUT_ITEMS = ROOT / "images" / "runtime" / "items"
OUT_UI = ROOT / "images" / "runtime" / "ui"


def cell(atlas: str, column: int, row: int) -> Image.Image:
    image = Image.open(TILES / atlas).convert("RGBA")
    x0 = round(column * image.width / 4)
    y0 = round(row * image.height / 4)
    x1 = round((column + 1) * image.width / 4)
    y1 = round((row + 1) * image.height / 4)
    return image.crop((x0, y0, x1, y1))


def save_tile(name: str, atlas: str, column: int, row: int) -> None:
    image = cell(atlas, column, row)
    image = image.resize((60, 60), Image.Resampling.NEAREST)
    image.save(OUT_TILES / name)


def save_icon(name: str, atlas: str, column: int, row: int) -> None:
    image = cell(atlas, column, row)
    # Remove small fragments from neighbouring atlas cells before trimming.
    clean_alpha = image.getchannel("A")
    pixels = clean_alpha.load()
    border = 12
    for y in range(image.height):
        for x in range(image.width):
            if x < border or y < border or x >= image.width - border or y >= image.height - border:
                pixels[x, y] = 0
    image.putalpha(clean_alpha)
    # Atlas decorations occasionally cross a cell boundary. Keep the primary
    # connected subject so those neighbouring fragments never reach runtime.
    mask = image.getchannel("A").point(lambda value: 1 if value > 8 else 0)
    source = mask.load()
    seen = bytearray(image.width * image.height)
    components = []
    for seed_y in range(image.height):
        for seed_x in range(image.width):
            seed = seed_y * image.width + seed_x
            if not source[seed_x, seed_y] or seen[seed]:
                continue
            stack = [(seed_x, seed_y)]
            seen[seed] = 1
            component = []
            while stack:
                x, y = stack.pop()
                component.append((x, y))
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    if 0 <= nx < image.width and 0 <= ny < image.height:
                        index = ny * image.width + nx
                        if source[nx, ny] and not seen[index]:
                            seen[index] = 1
                            stack.append((nx, ny))
            components.append(component)
    if components:
        primary = set(max(components, key=len))
        alpha_pixels = clean_alpha.load()
        for y in range(image.height):
            for x in range(image.width):
                if (x, y) not in primary:
                    alpha_pixels[x, y] = 0
        image.putalpha(clean_alpha)
    alpha = image.getchannel("A")
    bbox = alpha.point(lambda value: 255 if value > 8 else 0).getbbox()
    if bbox:
        image = image.crop(bbox)
    image.thumbnail((56, 56), Image.Resampling.NEAREST)
    output = Image.new("RGBA", (60, 60), (0, 0, 0, 0))
    output.alpha_composite(image, ((60 - image.width) // 2, (60 - image.height) // 2))
    output.save(OUT_ITEMS / name)


for directory in (OUT_TILES, OUT_ITEMS, OUT_UI):
    directory.mkdir(parents=True, exist_ok=True)

tile_exports = {
    "floor.png": ("dungeon_tiles.png", 2, 0),
    "wall.png": ("dungeon_tiles.png", 1, 0),
    "dark_wall.png": ("dungeon_tiles.png", 1, 0),
    "dark_wall_revealed.png": ("dungeon_tiles.png", 3, 0),
    "door_red.png": ("doors_stairs.png", 0, 0),
    "door_blue.png": ("doors_stairs.png", 1, 0),
    "door_yellow.png": ("doors_stairs.png", 3, 0),
    "door_magic.png": ("doors_stairs.png", 1, 3),
    "door_iron.png": ("doors_stairs.png", 3, 2),
    "stairs_up.png": ("doors_stairs.png", 2, 1),
    "stairs_down.png": ("doors_stairs.png", 3, 1),
    "lava.png": ("effects.png", 3, 2),
    "star_river.png": ("effects.png", 0, 3),
    "shop.png": ("doors_stairs.png", 2, 2),
}
for filename, args in tile_exports.items():
    save_tile(filename, *args)

item_exports = {
    "key_red.png": ("effects.png", 1, 0),
    "key_blue.png": ("effects.png", 0, 0),
    "key_yellow.png": ("props.png", 2, 3),
    "key_magic.png": ("effects.png", 2, 0),
    "potion.png": ("props.png", 0, 1),
    "weapon.png": ("props.png", 2, 1),
    "armor.png": ("props.png", 3, 1),
    "treasure.png": ("props.png", 3, 3),
    "glasses.png": ("props.png", 1, 3),
    "wall_breaker.png": ("props.png", 1, 3),
    "stairs_up.png": ("doors_stairs.png", 0, 2),
    "stairs_down.png": ("doors_stairs.png", 1, 2),
    "temp_shield.png": ("effects.png", 3, 3),
    "penguin_doll.png": ("props.png", 1, 0),
    "matcha_parfait.png": ("props.png", 3, 0),
    "lucky_coin.png": ("props.png", 3, 3),
    "holy_water.png": ("effects.png", 0, 3),
    "artifact.png": ("props.png", 2, 0),
}
for filename, args in item_exports.items():
    save_icon(filename, *args)

panel = cell("dungeon_tiles.png", 1, 0).resize((256, 256), Image.Resampling.NEAREST)
panel.putalpha(panel.getchannel("A").point(lambda value: int(value * 0.38)))
panel.save(OUT_UI / "panel_texture.png")
cell("dungeon_tiles.png", 0, 0).resize((384, 96), Image.Resampling.NEAREST).save(
    OUT_UI / "button_texture.png"
)
cell("props.png", 0, 3).resize((384, 96), Image.Resampling.NEAREST).save(
    OUT_UI / "title_plaque.png"
)
backdrop = Image.open(ROOT / "images" / "backgrounds" / "tower_hub.png").convert("RGBA")
backdrop.putalpha(backdrop.getchannel("A").point(lambda value: int(value * 0.24)))
backdrop.save(OUT_UI / "menu_backdrop.png")

print(f"Exported {len(tile_exports)} tiles, {len(item_exports)} items, and 4 UI textures")
