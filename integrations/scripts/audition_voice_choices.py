"""Audition the installed toy presets with Kokoro (no AI credits or pet history).
Run using claude-esp/gateway/.venv/bin/python and set PYTHONPATH to that gateway.
"""
import asyncio
import json
from pathlib import Path
import numpy as np
from esp_gateway.audio import pcm16_to_wav
from esp_gateway.pet_voices import PRESETS, PREVIEW_TEXT
from esp_gateway.tts import KokoroTTS

OUT=Path(__file__).resolve().parents[2]/'docs/previews/voice-choice-v1'
async def main():
    OUT.mkdir(parents=True,exist_ok=True)
    tts=KokoroTTS('http://192.168.1.117:8880')
    report=[]
    try:
        for name,v in PRESETS.items():
            pcm=await tts.synthesize(PREVIEW_TEXT,v.voice,16000,speed=v.speed,pitch_semitones=v.pitch)
            samples=np.frombuffer(pcm,dtype='<i2').astype(float)
            assert 16000<len(samples)<15*16000 and abs(samples).max()>100
            (OUT/(name+'.wav')).write_bytes(pcm16_to_wav(pcm,16000))
            row=dict(id=name,voice=v.voice,speed=v.speed,pitch=v.pitch,seconds=round(len(samples)/16000,2),peak=round(float(abs(samples).max())/32768,3))
            report.append(row);print(row,flush=True)
    finally:await tts.aclose()
    (OUT/'auditions.json').write_text(json.dumps(dict(text=PREVIEW_TEXT,presets=report),indent=2)+'\n')

if __name__=='__main__':asyncio.run(main())
