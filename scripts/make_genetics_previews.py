"""Lay out production-renderer captures (PET_CAPTURE_GENETICS=1) for visual QA."""
from pathlib import Path
from PIL import Image,ImageDraw
import argparse,json
p=argparse.ArgumentParser();p.add_argument('captures',type=Path);a=p.parse_args()
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'docs/previews/genetic-sprites-v1';OUT.mkdir(parents=True,exist_ok=True)
BG='#f6efe1';INK='#3d203d';traits=json.loads((ROOT/'data/pet_traits.json').read_text())
def sprite(name,scale=2):return Image.open(a.captures/(name+'.ppm')).resize((72*scale,72*scale),Image.Resampling.NEAREST)
out=Image.new('RGB',(8*144,9*174),BG);d=ImageDraw.Draw(out);row=0
for g,t in enumerate(traits[:7]):
 for v,name in enumerate(t['values']):
  x=v%8*144;y=(row+v//8)*174;out.paste(sprite('gene-%d-%02d'%(g,v)),(x,y+25));d.text((x+4,y+7),name,fill=INK)
 row+=(len(t['values'])+7)//8
out.save(OUT/'all-traits.png')
poses=[(0,1,0,'Idle'),(0,0,0,'Wave'),(2,0,0,'Happy'),(5,1,0,'Talk'),(6,0,0,'Listen'),(9,0,0,'Reach left'),(9,1,0,'Reach right'),(3,0,1,'Toast'),(3,1,3,'Cupcake'),(4,0,0,'Sleep'),(8,0,0,'Bath'),(1,0,0,'Blink')]
out=Image.new('RGB',(len(poses)*144,8*166),BG);d=ImageDraw.Draw(out)
for v in range(8):
 for x,(face,pose,food,label) in enumerate(poses):
  out.paste(sprite('family-%d-face-%d-pose-%d-food-%d'%(v,face,pose,food)),(x*144,v*166+20));d.text((x*144+4,v*166+4),label,fill=INK)
out.save(OUT/'pose-matrix.png')
frames=[]
for face,pose,food,label in poses:
 im=Image.new('RGB',(4*216,2*244+36),BG);d=ImageDraw.Draw(im);d.text((16,10),'Same saved traits, every activity: '+label,fill=INK)
 for v in range(8):
  x=v%4*216;y=v//4*244+36
  im.paste(sprite('family-%d-face-%d-pose-%d-food-%d'%(v,face,pose,food),3),(x,y))
  d.text((x+12,y+220),traits[0]['values'][v],fill=INK)
 frames.append(im)
frames[0].save(OUT/'pet-family.gif',save_all=True,append_images=frames[1:],duration=[850,650,650,500,750,450,450,800,650,1000,850,180],loop=0,disposal=2)
frames[0].save(OUT/'pet-family.png')
out=Image.new('RGB',(14*144,8*160),BG);d=ImageDraw.Draw(out)
for v in range(8):
 for food in range(7):
  for pose in range(2):
   x=(food*2+pose)*144;y=v*160
   out.paste(sprite('family-%d-face-3-pose-%d-food-%d'%(v,pose,food)),(x,y+16));d.text((x+2,y+2),['Apple','Toast','Cookie','Cupcake','Pancakes','Jelly','Cake'][food]+(' bite' if pose else ''),fill=INK)
out.save(OUT/'all-food-poses.png')
for name in ['profile-1','profile-2','trait-4-choice-3','trait-6-choice-5','home-idle']:
 f=a.captures/(name+'.ppm')
 if f.exists():Image.open(f).save(OUT/(name+'.png'))
print(OUT)
