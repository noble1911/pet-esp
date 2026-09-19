# Five complete characters

Sprout preserves the original approved complete sprites from `art/sprite-v3`
and `art/special-foods-v1`. Cloud bunny, Pebble penguin, Peach kitten and Tiny
dragon were each illustrated independently with the built-in imagegen tool.
The full source atlases and exact generation prompts are saved in each folder.
Bunny and dragon have replacement care sheets to keep quilts and tubs inside
the frame. No runtime facial parts or body compositing are used.

Each character has 26 complete 72 × 72 frames: twelve movement/care poses and
hold/bite poses for all seven foods. Food and props are painted into the frames.
`python3 scripts/pack_characters.py` removes the magenta key, uniformly scales
animation groups with nearest-neighbour sampling, and aligns whole frames to
a common baseline. It never moves or replaces individual features. Care sheets
are centred as whole images. The generated RGB565 RLE atlas is 635,520 bytes;
every frame is checked against its decoded pixels. `--check` verifies the header.

The renderer applies only a whole-frame baby size adjustment. Growth remains
in gameplay; later stages share the same character artwork. Original source
atlases and earlier art folders are preserved for edits and rollback.

Review actual firmware renderer captures in `docs/previews/five-characters-v1`.
Regenerate those with `PET_CAPTURE_CHARACTERS=1` in the host test directory,
then `python3 scripts/render_character_review.py /path/to/captures`.
