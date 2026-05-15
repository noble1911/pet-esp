# sprite_forge

Laptop-side tool that turns source PNG art into the on-device `.bin`
sprite sheets. **Not firmware** (architecture §8). Run it on every art
change before flashing — art iteration happens without a device in the
loop.

```
pip install -r requirements.txt

python build.py          # build all .bin assets into ../firmware/assets
python preview.py        # interactive preview window (composite a pet)
python random_grid.py    # save a 4x4 PNG of random pets for review
```

## Source layout

```
parts/{stage}/{layer}/{shape}/{animation}.png
parts/{stage}/{layer}/{shape}/anchors.json
palettes/*.png
```

- `stage`     — egg | baby | child | teen | adult | elder
- `layer`     — body | tail | ears | mouth | eyes | pattern | accessory
- Part PNGs are authored **grayscale + alpha**; colour comes from a
  gene-indexed palette at composite time (architecture §5.2).
- `anchors.json` gives upper-layer attach offsets (docs/sprite_format.md).

## Output

`.bin` files written to `../firmware/assets/` in the format defined in
[docs/sprite_format.md](../docs/sprite_format.md) (magic `PSPR`). That
directory is gitignored.
