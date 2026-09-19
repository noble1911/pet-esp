# Playtime art

Built-in imagegen created two matching chroma-key atlases using the current illustrated icon style as a reference. Exact prompts are preserved in [PROMPTS.md](PROMPTS.md).

- `stickers-source.png`: twelve extra 24×24 sticker assets, row-major butterfly, flower, bunny, rainbow, kite, watering can, cloud, sun, music, crown, book, present.
- `decorations-source.png`: six 40×40 gifts, row-major flowers, bunting, teddy, moon lamp, cushion, trophy.
- `collectible-*.png` and `decoration-*.png`: exact device-resolution RGBA outputs.

Run `python3 scripts/pack_playtime_art.py` from the repository to reproduce `firmware/components/ui/include/playtime_art.h`. It only extracts complete illustrations, removes the supplied magenta key and uses nearest-neighbour resampling. The six original stickers retain their artwork/order and thresholds. The extra 66,048 bytes of pixel data are flash-backed.
