"""Live Haiku -> Kokoro -> two simulated pet sockets, with isolated temporary users.
Run from the Mac: python3 integrations/scripts/check_pet_playdate_chat.py
No real pet is contacted and no pet memories or conversation history are read.
"""
import json
from pathlib import Path
import subprocess
import uuid
HOST='ron@192.168.1.117'
PREFIX='export PATH=/opt/homebrew/bin:$HOME/.orbstack/bin:$PATH; '
USERS=['pet-chat-test-'+uuid.uuid4().hex for _ in range(2)]

def remote(container, source):
    result=subprocess.run(['ssh','-o','BatchMode=yes',HOST,PREFIX+'docker exec -i '+container+' python'],
                          input=source,text=True,capture_output=True,timeout=240)
    if result.returncode:raise RuntimeError(result.stderr[-3000:])
    return result.stdout

try:
    remote('butler-api',f'''
import asyncio
from tools import DatabasePool
async def main():
 db=await DatabasePool.create()
 try:
  for user in {USERS!r}:
   await db.pool.execute('INSERT INTO butler.users(id,name,soul,permissions,notification_prefs) VALUES ($1,$2,$3::jsonb,$4::jsonb,$5::jsonb)',user,'Temporary pet chat test',{{'profile':'virtual_pet'}},{{}},{{'enabled':False}})
 finally:await db.close()
asyncio.run(main())
''')
    source='''
import asyncio,json,hashlib
import websockets
from esp_gateway.server import build_deps
from esp_gateway.config import load_config
from esp_gateway.playdates import Playdates,play_connection
USERS=USER_LIST
async def main():
 cfg=load_config();cfg.device_tokens={'a':[USERS[0]],'b':[USERS[1]]};cfg.playdates_groups={u:'test-home' for u in USERS};cfg.playdates_db=':memory:'
 deps,client=build_deps(cfg);game=Playdates(cfg,deps=deps);result=[];done=[asyncio.Event(),asyncio.Event()];ready=asyncio.Event();seen=[0,0]
 async def receive(ws,index):
  audio=bytearray();seq=0;room='';last=''
  async for raw in ws:
   if isinstance(raw,bytes):audio.extend(raw);continue
   m=json.loads(raw)
   if m['type']=='play_state':
    if index==0 and m.get('peers'):ready.set()
    if m.get('phase')=='incoming':await ws.send(json.dumps(dict(type='accept',invite=m['invite'])))
    if m.get('text'):last=m['text']
    if m.get('phase')=='finished':
     assert not m.get('notice'),m
     done[index].set()
   elif m['type']=='chat_audio_start':
    seq=m['seq'];room=m['room'];audio.clear();assert (seq-1)%2==index
   elif m['type']=='chat_audio_end':
    assert len(audio)>8000
    result.append(dict(speaker=('Osono','Larry')[index],turn=seq,text=last,pcm_bytes=len(audio),sha256=hashlib.sha256(audio).hexdigest()))
    seen[index]+=1
    await ws.send(json.dumps(dict(type='heard',room=room,seq=seq)))
 async def ping(ws):
  while True:await asyncio.sleep(5);await ws.send(json.dumps(dict(type='ping')))
 try:
  async with websockets.serve(lambda ws,path:play_connection(ws,game),'127.0.0.1',0) as server:
   port=server.sockets[0].getsockname()[1]
   async with websockets.connect(f'ws://127.0.0.1:{port}') as a,websockets.connect(f'ws://127.0.0.1:{port}') as b:
    tasks=[]
    try:
     for index,ws in enumerate((a,b)):
      pet=dict(id=f'{index+1:016x}',name=('Osono','Larry')[index],character=(7,6)[index],stage=3,fullness=85,happiness=90,energy=80,cleanliness=90,stars=25,personality=0)
      await ws.send(json.dumps(dict(type='hello',proto=1,artwork=3,chat=1,user_id=USERS[index],device_token=('a','b')[index],pet=pet)))
      await ws.send(json.dumps(dict(type='join',pet=pet)))
      tasks.extend([asyncio.create_task(receive(ws,index)),asyncio.create_task(ping(ws))])
     await asyncio.wait_for(ready.wait(),10)
     await a.send(json.dumps(dict(type='invite',user=USERS[1],mode='chat')))
     finished=asyncio.ensure_future(asyncio.gather(*(event.wait() for event in done)))
     completed,_=await asyncio.wait([finished,*tasks],timeout=180,return_when=asyncio.FIRST_COMPLETED)
     if finished not in completed:
      finished.cancel()
      await asyncio.gather(finished,return_exceptions=True)
      for task in completed:task.result()
      raise TimeoutError('Pet chat did not finish')
     assert seen==[4,4],seen
     assert game.db.execute('SELECT count(*) FROM rewards').fetchone()[0]==0
     print(json.dumps(sorted(result,key=lambda r:r['turn']),indent=2))
    finally:
     for task in tasks:task.cancel()
     await asyncio.gather(*tasks,return_exceptions=True)
 finally:
  chats=[r.chat_task for r in game.rooms.values() if r.chat_task]
  for task in chats:task.cancel()
  await asyncio.gather(*chats,return_exceptions=True)
  game.db.close();await client.aclose()
asyncio.run(main())
'''.replace('USER_LIST',repr(USERS))
    data=remote('esp-gateway',source)
    output=Path(__file__).resolve().parents[2]/'docs/previews/pet-chat-v1/live-chat.json'
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(data)
    print(data)
finally:
    print(remote('butler-api',f'''
import asyncio
from tools import DatabasePool
async def main():
 db=await DatabasePool.create()
 try:
  for user in {USERS!r}:
   assert not await db.pool.fetchval('SELECT count(*) FROM butler.conversation_history WHERE user_id=$1',user)
   await db.pool.execute('DELETE FROM butler.users WHERE id=$1',user)
  print('Temporary pet accounts removed; no conversation history created.')
 finally:await db.close()
asyncio.run(main())
'''))
