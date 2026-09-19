"""Render review deliverables from actual LVGL host-test captures."""
from pathlib import Path
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
p=ROOT/'docs/previews/playtime-v1'
names=['games','peekaboo','peekaboo-found','bouncy-ball','stickers-page-1','stickers-page-2','stickers-page-3','stickers-next','room-gifts','room-gifts-locked','room-flowers','room-trophy','new-room-gift','butterfly-visit','window-weather-0','window-weather-1','window-weather-2']
for name in names:
    with Image.open(p/(name+'.ppm')) as im:im.save(p/(name+'.png'))
for scene in ['ball','peekaboo','butterfly']:
    frames=[]
    for f in sorted((p/'frames').glob(scene+'-*.ppm')):
        with Image.open(f) as im:frames.append(im.convert('RGB'))
    if frames:frames[0].save(p/(scene+'-animation.gif'),save_all=True,append_images=frames[1:],duration=160,loop=0,optimize=False)
board_names=['games','peekaboo-found','bouncy-ball','stickers-page-2','room-gifts','butterfly-visit']
board=Image.new('RGB',(368*3,480*2),'#fff9ed');draw=ImageDraw.Draw(board)
for i,name in enumerate(board_names):
    with Image.open(p/(name+'.png')) as im:board.paste(im,((i%3)*368,(i//3)*480+28))
    draw.text(((i%3)*368+18,(i//3)*480+8),name.replace('-',' ').title(),fill='#3d203d')
board.save(p/'overview.png')
print('Saved 17 screen captures, 3 animations and overview.png')
