"""Convert production LVGL captures to review PNGs, GIFs and a contact sheet."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
p=ROOT/'docs/previews/sprite-v3'
for f in p.glob('*.ppm'):
    with Image.open(f) as im: im.save(f.with_suffix('.png'))
    f.unlink()
for scene in ['idle','apple','toast','cookie','bath','sleep','play','party','talk']:
    frames=[]
    for f in sorted((p/'frames').glob(scene+'-*.ppm')):
        with Image.open(f) as im: frames.append(im.convert('RGB'))
    if frames:
        frames[0].save(p/(scene+'-animation.gif'),save_all=True,append_images=frames[1:],duration=160,loop=0,optimize=False)
names=['home-idle','eat-apple-bite','eat-toast-bite','eat-cookie-bite','bath','sleep','play','party']
board=Image.new('RGB',(368*4,480*2),'#fff9ed');draw=ImageDraw.Draw(board)
for i,name in enumerate(names):
    with Image.open(p/(name+'.png')) as im:board.paste(im,((i%4)*368,(i//4)*480+28))
    draw.text(((i%4)*368+18,(i//4)*480+8),name,fill='#3d203d')
board.save(p/'overview.png')
# Keep frame captures local; the compact GIFs are the review deliverables.
(p/'frames'/'.gitignore').write_text('*\n!.gitignore\n')
