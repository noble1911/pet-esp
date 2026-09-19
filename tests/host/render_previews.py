"""Convert the real LVGL test renders to PNG; requires Pillow."""
from pathlib import Path
from PIL import Image, ImageDraw
p = Path(__file__).resolve().parents[2] / 'docs/previews/little-meadow'
names = ['home', 'food', 'play', 'bath', 'sleep', 'party', 'album', 'settings']
board = Image.new('RGB', (368 * 4, 480 * 2), '#eef0ef')
draw = ImageDraw.Draw(board)
for f in p.glob('*.ppm'):
    with Image.open(f) as im:
        im.save(f.with_suffix('.png'))
for i, name in enumerate(names):
    with Image.open(p / (name + '.png')) as im:
        board.paste(im, ((i % 4) * 368, (i // 4) * 480 + 32))
    draw.text(((i % 4) * 368 + 16, (i // 4) * 480 + 10), name, fill='#39465c')
board.save(p / 'overview.png')
for f in p.glob('*.ppm'):
    f.unlink()

# Pet review boards: preserve the recorded baseline and regenerate current art.
review = p.parent / 'pet-refresh'
review.mkdir(exist_ok=True)
if (p / 'pet-coat-0.png').exists():
    variants = Image.new('RGB', (6 * 196, 2 * 228), '#fff9ed')
    draw = ImageDraw.Draw(variants)
    for row, names, prefix in [
        (0, ['Peach', 'Lilac', 'Mint', 'Rose', 'Honey', 'Sky'], 'pet-coat'),
        (1, ['Baby', 'Child', 'Teen', 'Adult', 'Elder'], 'pet-stage'),
    ]:
        for i, name in enumerate(names):
            with Image.open(p / f'{prefix}-{i}.png') as im:
                variants.paste(im.crop((88, 144, 284, 344)), (i * 196, row * 228 + 28))
            draw.text((i * 196 + 14, row * 228 + 8), name, fill='#39465c')
    variants.save(review / 'coats-and-growth.png')
if (review / 'before.png').exists():
    comparison = Image.new('RGB', (736, 480), '#fff9ed')
    draw = ImageDraw.Draw(comparison)
    for i, (name, path) in enumerate([('Before', review / 'before.png'), ('After', p / 'home.png')]):
        with Image.open(path) as im:
            comparison.paste(im, (i * 368, 32))
        draw.text((i * 368 + 24, 10), name, fill='#39465c')
    comparison.save(review / 'before-after.png')
