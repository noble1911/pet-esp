"""Exercise deployed pet brain -> gateway -> Kokoro with a temporary account.
Runs from this Mac. Deletes the test account/history even if the speech test fails.
No real pet/user memory is read, reset or changed; no messages go to the device.
"""
import json,subprocess,uuid
from pathlib import Path
HOST='ron@192.168.1.117'
USER='pet-meadow-audition-'+uuid.uuid4().hex
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'art/voice-v2';OUT.mkdir(parents=True,exist_ok=True)
PREFIX='export PATH=/opt/homebrew/bin:$HOME/.orbstack/bin:$PATH; '
def python_in(container,source):
 return subprocess.run(['ssh','-o','BatchMode=yes',HOST,PREFIX+f'docker exec -i {container} python'],input=source,text=True,check=True,capture_output=True).stdout
try:
 print(python_in('butler-api',f'''
import asyncio
from tools import DatabasePool
async def main():
 db=await DatabasePool.create()
 try:
  await db.pool.execute('INSERT INTO butler.users (id,name,soul,permissions,notification_prefs) VALUES ($1,$2,$3::jsonb,$4::jsonb,$5::jsonb)',{USER!r},'Temporary creature audition',{{'profile':'virtual_pet','voice':'bf_emma'}},{{}},{{'enabled':False,'categories':[]}})
 finally:await db.close()
asyncio.run(main())
'''),end='')
 script='''
import asyncio,json,wave
import numpy as np
from pathlib import Path
from esp_gateway.config import load_config
from esp_gateway.server import build_deps
from esp_gateway.session import Session
USER=TEST_USER
PET=dict(pet_id='1234567890abcdef',name='Sprout',stage=3,fullness=80,happiness=90,energy=80,cleanliness=90,stars=35,genes=[0,2,0,0,0,0,0,4],generation=0,inventory=[0]*16,friends_met=0,activity='home')
class Conn:
 def __init__(self):self.messages=[];self.audio=bytearray()
 async def send(self,m):
  if isinstance(m,bytes):self.audio.extend(m)
  else:self.messages.append(json.loads(m))
async def main():
 cfg=load_config();cfg.device_tokens={'audition-in-process':[USER]}
 assert (cfg.pet_voice,cfg.pet_speech_speed,cfg.pet_pitch_semitones)==('af_heart',1.12,6.5)
 deps,client=build_deps(cfg);conn=Conn();session=Session(conn,deps);results=[]
 try:
  await session.handle(json.dumps(dict(type='hello',device_token='audition-in-process',user_id=USER,surface='pet')))
  for name,text in [('hello','Hi Sprout!'),('feisty','You are a silly potato!'),('identity','Are you a real animal?')]:
   conn.messages.clear();conn.audio.clear()
   await session.handle(json.dumps(dict(type='text',text=text,pet=PET)))
   await asyncio.wait_for(session._turn,90)
   assert not [m for m in conn.messages if m['type']=='error'],conn.messages
   reply=''.join(m.get('text','') for m in conn.messages if m['type']=='say').strip()
   a=np.frombuffer(conn.audio,dtype='<i2').astype(float)
   assert reply and len(a)>8000 and abs(a).max()>1000
   assert np.mean(abs(a)>=32767)<.001
   with wave.open('/tmp/creature-voice-'+name+'.wav','wb') as f:
    f.setnchannels(1);f.setsampwidth(2);f.setframerate(16000);f.writeframes(conn.audio)
   result=dict(case=name,reply=reply,words=len(reply.split()),seconds=round(len(a)/16000,2),voice=cfg.pet_voice,speed=cfg.pet_speech_speed,pitch=cfg.pet_pitch_semitones)
   results.append(result);print(json.dumps(result),flush=True)
  Path('/tmp/creature-live-results.json').write_text(json.dumps(results,indent=2)+'\\n')
 finally:
  await session.close();await client.aclose()
asyncio.run(main())
'''.replace('TEST_USER',repr(USER))
 print(python_in('esp-gateway',script),end='')
 for name in ('creature-voice-hello.wav','creature-voice-feisty.wav','creature-voice-identity.wav','creature-live-results.json'):
  subprocess.run(['ssh','-o','BatchMode=yes',HOST,PREFIX+f'docker cp esp-gateway:/tmp/{name} /tmp/{name}'],check=True)
  subprocess.run(['scp',HOST+':/tmp/'+name,str(OUT/name)],check=True)
finally:
 print(python_in('butler-api',f'''
import asyncio
from tools import DatabasePool
async def main():
 db=await DatabasePool.create()
 try:
  await db.pool.execute('DELETE FROM butler.conversation_history WHERE user_id=$1',{USER!r})
  await db.pool.execute('DELETE FROM butler.users WHERE id=$1',{USER!r})
  print('Temporary audition account and conversation history removed')
 finally:await db.close()
asyncio.run(main())
'''),end='')
