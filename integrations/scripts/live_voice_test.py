"""Run inside esp-gateway with a separately provisioned test account, then delete it."""
import asyncio,json,wave
from esp_gateway.server import build_deps
from esp_gateway.config import load_config
from esp_gateway.session import Session
PET=dict(pet_id='1234567890abcdef',name='Sprout',stage=2,fullness=85,happiness=90,energy=80,cleanliness=95,stars=15,genes=[0]*8,generation=0,inventory=[0]*16,friends_met=0,activity='talking')
class Conn:
    def __init__(self):self.messages=[];self.audio=bytearray()
    async def send(self,m):
        if isinstance(m,bytes):self.audio.extend(m)
        else:self.messages.append(json.loads(m))
async def main():
    cfg=load_config();cfg.device_tokens={'test-local':['pet-meadow-integration-test']}
    deps,client=build_deps(cfg)
    async def session():
        conn=Conn();s=Session(conn,deps)
        await s.handle(json.dumps(dict(type='hello',device_token='test-local',user_id='pet-meadow-integration-test',surface='pet')))
        return s,conn
    s,c=await session()
    speech=await deps.tts.synthesize('Hello Sprout! What is your name, and are you feeling happy today?', 'bf_emma',16000)
    await s.handle(json.dumps(dict(type='audio_start',pet=PET)))
    for i in range(0,len(speech),640):await s.handle(speech[i:i+640])
    await s.handle(json.dumps(dict(type='audio_end',pet=PET)))
    await asyncio.wait_for(s._turn,120)
    assert not [m for m in c.messages if m['type']=='error'],c.messages
    transcript=' '.join(m.get('text','') for m in c.messages if m['type']=='stt')
    reply=''.join(m.get('text','') for m in c.messages if m['type']=='say')
    assert transcript and reply and len(c.audio)>32000,(transcript,reply,len(c.audio))
    print('LIVE AUDIO:',transcript,flush=True);print('PET REPLY:',reply,flush=True);print('AUDIO BYTES:',len(c.audio),flush=True)
    with wave.open('/tmp/pet-reply.wav','wb') as f:f.setnchannels(1);f.setsampwidth(2);f.setframerate(16000);f.writeframes(c.audio)
    # A durable, harmless fact; reconnect and ask without including it in prompt.
    await s.handle(json.dumps(dict(type='text',text='Please remember: our pretend garden has a blue butterfly named Pip.',pet=PET)))
    await asyncio.wait_for(s._turn,120);await s.close()
    s,c=await session()
    await s.handle(json.dumps(dict(type='text',text='What is the name of the butterfly in our pretend garden?',pet=PET)))
    await asyncio.wait_for(s._turn,120)
    reply=''.join(m.get('text','') for m in c.messages if m['type']=='say')
    assert 'Pip' in reply,reply
    print('RECONNECTED MEMORY:',reply,flush=True)
    # Fresh stats must override prior happy/full context.
    await s.handle(json.dumps(dict(type='text',text='Is your tummy full right now?',pet={**PET,'fullness':20})))
    await asyncio.wait_for(s._turn,120)
    print('FRESH LOW FULLNESS:',''.join(m.get('text','') for m in c.messages if m['type']=='say')[len(reply):],flush=True)
    await s.close();await client.aclose()
asyncio.run(main())
