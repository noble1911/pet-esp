"""Run with the claude-esp gateway on PYTHONPATH; uses local Kokoro, no paid voice API."""
import asyncio,json
from pathlib import Path
from esp_gateway.tts import KokoroTTS
from esp_gateway.audio import pcm16_to_wav
import numpy as np
OUT=Path(__file__).resolve().parents[2]/'art/voice-v2'
OUT.mkdir(parents=True,exist_ok=True)
TEXT="Ooh, it's you! Little leaf wiggles! That cupcake looks yummy."
async def main():
 tts=KokoroTTS('http://192.168.1.117:8880')
 profiles=[('01-current','af_sky',1.05,3),('02-tiny-sprout','af_heart',1.12,6.5),('03-woodland-sprout','am_puck',1.10,8)]
 report=[]
 try:
  for name,voice,speed,pitch in profiles:
   pcm=await tts.synthesize(TEXT,voice,16000,speed=speed,pitch_semitones=pitch)
   (OUT/(name+'.wav')).write_bytes(pcm16_to_wav(pcm,16000))
   a=np.frombuffer(pcm,dtype='<i2').astype(float)
   data=dict(file=name+'.wav',voice=voice,speed=speed,pitch_semitones=pitch,seconds=round(len(a)/16000,2),peak=round(float(abs(a).max())/32768,3),rms=round(float(np.sqrt(np.mean(a*a)))/32768,3))
   report.append(data);print(data,flush=True)
 finally:await tts.aclose()
 (OUT/'auditions.json').write_text(json.dumps({'text':TEXT,'profiles':report},indent=2)+'\n')
asyncio.run(main())
