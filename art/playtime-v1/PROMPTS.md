# Playtime artwork

Created with the built-in imagegen tool. Reference: `../sprite-v3/icons-source.png`.
Source images are preserved; `scripts/pack_playtime_art.py` splits the atlas, keys the background, and compiles nearest-neighbour device pixels.

## Sticker atlas

Create a new 4-column by 3-row atlas of twelve separate pixel-art sticker icons matching the supplied reference's chunky plum outlines, lovely shaded cheerful toy colors and crisp simple readable forms. EACH TILE SAME SIZE, lots of solid pure chroma magenta #FF00FF padding, no labels, no shadows outside shapes, no grid lines. EXACT row-major order: row1 a blue and yellow butterfly, pink daisy flower, white bunny head, pastel rainbow with little white clouds; row2 a turquoise diamond kite with tail, mint watering can, fluffy pale blue cloud, smiling yellow sun; row3 two joined musical notes in turquoise, golden crown with pink jewels, open blue storybook, pink wrapped present with yellow bow. Square-ish icons centered with equal scale each fitting within its tile. Background uniformly bright magenta, none of the icon interiors should be magenta. These are small 24 pixel game icons, use large chunky details, stepped pixel edges, no lettering, no gradients in background. Landscape 4:3 canvas.

## Room decoration atlas

Use case: stylized-concept. Create six charming illustrated pixel-art virtual pet room decoration sprites in a perfect 3-column by 2-row atlas. Match reference chunky dark plum outlines, shaded cheerful toy colors, and crisp stepped pixel edges. Row1: terracotta pot of pink daisies and green leaves; short curved string of five pastel triangular bunting flags; sitting honey-brown teddy bear with mint ribbon. Row2: small glowing crescent-moon bedside lamp on a blue base; plump rainbow-striped floor cushion; gold star-shaped trophy on a little plum base. Every object complete and isolated in its own equally sized tile, centered, generous solid pure chroma magenta #FF00FF padding. Uniform magenta background, no checkerboard, no labels, no grid, no text. Orthographic front view, whole objects no crops. Assets need to read at 40x40 pixels with few large details; suitable for cozy pixel art nursery room. No magenta inside objects. Landscape canvas ratio 3:2.
