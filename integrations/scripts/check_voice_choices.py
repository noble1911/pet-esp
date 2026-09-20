"""Run inside the deployed gateway; audition via a separate authenticated socket.
No microphones, prompts, history writes, pet state changes or physical playback.
"""
import asyncio
import json
import websockets
from esp_gateway.config import load_config
from esp_gateway.pet_voices import PRESETS

async def main():
    cfg=load_config()
    token,user=next((token,user) for token,users in cfg.device_tokens.items()
                    for user in users if user.startswith('pet-meadow-'))
    report=[]
    async with websockets.connect('ws://127.0.0.1:8770/ws') as ws:
        await ws.send(json.dumps(dict(type='hello',proto=1,surface='pet',device_token=token,user_id=user,
                                      capture={'rate':16000},playback={'rate':16000})))
        ready=json.loads(await asyncio.wait_for(ws.recv(),10));assert ready['type']=='ready'
        for index,preset in enumerate(PRESETS,1):
            await ws.send(json.dumps(dict(type='voice_preview',voice_preset=preset,preview_id=index)))
            pcm=bytearray();started=False;ended=False
            while True:
                raw=await asyncio.wait_for(ws.recv(),40)
                if isinstance(raw,bytes):
                    assert started and not ended;pcm.extend(raw);continue
                msg=json.loads(raw)
                assert msg.get('preview_id')==index and msg['type']!='error',msg
                if msg['type']=='tts_start':started=True
                elif msg['type']=='tts_end':ended=True
                elif msg['type']=='state' and msg['value']=='idle':break
            assert started and ended and 32000<len(pcm)<480000 and len(pcm)%2==0
            report.append(dict(preset=preset,seconds=round(len(pcm)/32000,2),pcm_bytes=len(pcm)))
        await ws.send(json.dumps(dict(type='voice_preview',voice_preset='warm',preview_id=100)))
        while True:
            msg=json.loads(await asyncio.wait_for(ws.recv(),10))
            if msg.get('value')=='thinking':break
        await ws.send(json.dumps(dict(type='cancel')))
        while True:
            raw=await asyncio.wait_for(ws.recv(),10)
            if isinstance(raw,str) and json.loads(raw).get('value')=='idle':break
        await ws.send(json.dumps(dict(type='ping')))
        assert json.loads(await asyncio.wait_for(ws.recv(),5))['type']=='pong'
    print(json.dumps(dict(previews=report,cancellation='passed',brain_calls=0),indent=2))

if __name__=='__main__':asyncio.run(main())
