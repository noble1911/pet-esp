"""Review complete-character frames captured from the real firmware C renderer."""
from pathlib import Path
from PIL import Image,ImageDraw
import argparse,json
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('captures',type=Path);p.add_argument('--out',type=Path,default=ROOT/'docs/previews/pink-pig-v1');args=p.parse_args();OUT=args.out;OUT.mkdir(parents=True,exist_ok=True)
chars=json.loads((ROOT/'data/pet_characters.json').read_text());BG='#fff6e6';INK='#3d203d'
def frame(c,face,pose=0,food=0,scale=3):return Image.open(args.captures/f'character-{c}-face-{face}-pose-{pose}-food-{food}.ppm').resize((72*scale,72*scale),Image.Resampling.NEAREST)
scenes=[('Idle',0,1),('Wave',0,0),('Blink',1,0),('Talk',5,1),('Listen',6,0),('Happy',2,0),('Reach left',9,0),('Reach right',9,1),('Sleep',4,0),('Breathe',4,1),('Bath',8,0),('Splash',8,1)]
out=Image.new('RGB',(12*144,len(chars)*174),BG);d=ImageDraw.Draw(out)
for c in range(len(chars)):
 for col,(name,face,pose) in enumerate(scenes):
  x=col*144;y=c*174;d.text((x+4,y+5),name,fill=INK);out.paste(frame(c,face,pose,scale=2),(x,y+25))
out.save(OUT/'all-actions.png')
out=Image.new('RGB',(14*144,len(chars)*166),BG);d=ImageDraw.Draw(out)
for c in range(len(chars)):
 for food,name in enumerate(['Apple','Toast','Cookie','Cupcake','Pancakes','Jelly','Cake']):
  for pose in range(2):
   x=(food*2+pose)*144;y=c*166;d.text((x+4,y+3),name+(' bite' if pose else ''),fill=INK);out.paste(frame(c,3,pose,food,2),(x,y+20))
out.save(OUT/'all-foods.png')
# Hold/blink/talk loops show whether whole faces remain registered, without hiding jumps.
for c,char in enumerate(chars):
 seq=[(0,1,900),(1,0,150),(0,1,600),(5,1,180),(0,1,180),(5,1,180),(0,1,650),(0,0,650),(0,1,650)]
 ims=[frame(c,f,p) for f,p,_ in seq]
 ims[0].save(OUT/(char['key']+'-face-loop.gif'),save_all=True,append_images=ims[1:],duration=[ms for _,_,ms in seq],loop=0,disposal=2)
frames=[]
for name,face,pose in scenes:
 out=Image.new('RGB',(len(chars)*216,248),BG);d=ImageDraw.Draw(out)
 for c,char in enumerate(chars):
  out.paste(frame(c,face,pose),(c*216,24));d.text((c*216+10,5),char['name']+' / '+name,fill=INK)
 frames.append(out)
frames[0].save(OUT/'all-friends.png');frames[0].save(OUT/'all-friends.gif',save_all=True,append_images=frames[1:],duration=[900,650,160,650,650,650,400,400,800,800,600,600],loop=0,disposal=2)
for f in args.captures.glob('choose-character-*.ppm'):Image.open(f).save(OUT/(f.stem+'.png'))
for name in ['character-profile','home-idle']:
 f=args.captures/(name+'.ppm')
 if f.exists():Image.open(f).save(OUT/(name+'.png'))
print(OUT)
