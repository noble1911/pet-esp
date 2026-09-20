# Complete characters

Sprout preserves the original approved complete sprites from `art/sprite-v3`
and `art/special-foods-v1`. Cloud bunny, Pebble penguin, Peach kitten and Tiny
dragon, Rosy pig and Larry were each illustrated independently with the built-in imagegen tool.
The full source atlases and exact generation prompts are saved in each folder.
Bunny and dragon have replacement care sheets to keep quilts and tubs inside
the frame. No runtime facial parts or body compositing are used.

Each character has 26 complete 72 × 72 frames: twelve movement/care poses and
hold/bite poses for all seven foods. Food and props are painted into the frames.
`python3 scripts/pack_characters.py` removes the magenta key, uniformly scales
animation groups with nearest-neighbour sampling, and aligns whole frames to
a common baseline. It never moves or replaces individual features. Care sheets
are centred as whole images. The generated RGB565 RLE atlas is 875,409 bytes;
every frame is checked against its decoded pixels. `--check` verifies the header.

The renderer applies only a whole-frame baby size adjustment. Growth remains
in gameplay; later stages share the same character artwork. Original source
atlases and earlier art folders are preserved for edits and rollback.

Review actual firmware renderer captures in `docs/previews/five-characters-v1`.
Regenerate those with `PET_CAPTURE_CHARACTERS=1` in the host test directory,
then `python3 scripts/render_character_review.py /path/to/captures`.

## Rosy pig addition

`pig/` contains the Peppa-inspired pink pig in a red dress, generated with the
built-in imagegen tool. `motion-source.png` and `food-source.png` are untouched
outputs; `motion-prompt.txt` and `food-prompt.txt` contain the exact accepted
prompts. The first food candidate was rejected for clipped feet.
`food-source.json` records crop boundaries through the sheet's empty row gutters,
preventing neighbouring ears from leaking into another frame. Cropping, keying,
whole-image scaling and baseline registration preserve the complete artwork.
This addition brought the total to 156 frames across six characters. The earlier five are unchanged.

## Larry addition

`larry/` contains the Pokemon salaryman in a charcoal suit, cloud-pattern tie,
and tired expression. The built-in imagegen tool created two source sheets,
`motion-source.png` and `food-source.png`, with their exact prompts saved as
`motion-prompt.txt` and `food-prompt.txt`. The official visual reference is
https://scarletviolet.pokemon.com/en-gb/characters/larry/ ; the earlier pig sheet
provided only the existing pixel-art style and animation layout.
All 26 complete frames are compiled without separate facial parts. Equal-cell
crop boundaries were checked to fall in empty background. The seven-character
atlas now contains 182 frames. All previous six characters are unchanged.
