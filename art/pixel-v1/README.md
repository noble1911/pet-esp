# Pixel-art v1 assets

`approved-concept.png` is the user-approved imagegen mockup. The room source files were generated using the built-in imagegen tool from that concept. The device PNGs and C header are deployment conversions: nearest-neighbour resize to 184 × 152, then RGB565 packing. No runtime PNG decoder is needed. Each background costs 55,936 bytes in flash. Generated header: `firmware/components/ui/include/pixel_rooms.h`.

The character and icons are native C pixel artwork in `pixel_pet.c`, drawn at 56 × 56 and 20 × 20. The pet is enlarged 3×, rooms 2×, with filtering disabled. Original pet identity, coat gene and growth progress drive the new artwork; NVS has no schema change.

Regenerate deployment assets with `python3 scripts/pack_pixel_rooms.py` (Pillow required). The source PNGs are retained, and the generated header is committed so firmware builds need no Python image dependencies.

## Built-in imagegen prompts

Day room (approved concept supplied as the reference image):

> Create one production game BACKGROUND asset based on the left AT HOME room of this reference. Only the empty room, absolutely NO character, NO pet, NO speech bubble, NO HUD, NO icons, NO buttons, NO lettering, NO border, NO collage. Single nearly square landscape image with aspect ratio 368:304. Composition: cozy apricot wall, central blue sky window with pink checked curtains in upper half, small framed flower at upper left, small leafy plant at left edge, low wooden cabinet with books at right edge, pale golden wooden plank floor occupying bottom 40%, large mint green oval rug centred in lower third. The centre of rug stays completely empty to receive an animated sprite later. Match the charming saturated warm Tamagotchi pixel art in the supplied reference. Strong consistent coarse pixel grid, clean stepped edges, limited flat shade ramps, crisp pixel illustration. Simplify little details for a 184x152 logical pixel screen. Straight on front view with slight floor perspective, room fills entire image edge to edge. No gradient, no blur, no texture noise. This is the background layer of a real small-screen game.

Night room (day source supplied as the edit target):

> Edit this game background into its NIGHT variant. Preserve EXACT layout, dimensions, positions of all room furniture, empty rug and pixel-art style. Change only lighting and the view through window: muted indigo/lavender room, dark blue night sky with tiny stars and crescent moon instead of sunlit landscape. Calm soft readable colours, not too dark. Keep the room completely EMPTY: no character, no bed, no text, no HUD, no buttons. Keep same 368:304 aspect ratio and hard pixel-art edges. This must align with the day image as a background swap for the same real game.
