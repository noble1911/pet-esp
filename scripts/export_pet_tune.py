"""Export a pet note pattern as a standard MIDI file (one monophonic instrument).
Usage: python3 scripts/export_pet_tune.py score.json output.mid
The ESP uses synthesized PCM; this export is for auditioning scores elsewhere.
"""
import json
import struct
import sys
from pathlib import Path

def vlq(n):
    result=bytearray([n&127]);n>>=7
    while n:result.insert(0,128|(n&127));n>>=7
    return bytes(result)

def to_midi(score):
    tempo=score['tempo'];notes=score['notes'];instrument=score['instrument']
    if type(tempo) is not int or not 60<=tempo<=150 or instrument not in ('bell','pluck','flute') or not 4<=len(notes)<=24:raise ValueError('Invalid score')
    total=0
    for pitch,ticks in notes:
        if type(pitch) is not int or type(ticks) is not int or (pitch!=0 and not 48<=pitch<=84) or not 1<=ticks<=8:raise ValueError('Invalid note')
        total+=ticks
    if total*15/tempo>12 or not any(p for p,_ in notes):raise ValueError('Invalid duration or silent tune')
    track=bytearray(b'\x00\xff\x51\x03'+round(60000000/tempo).to_bytes(3,'big'))
    track+=bytes([0,0xc0,{'bell':9,'pluck':24,'flute':73}[instrument]])
    pending=0
    for pitch,ticks in notes:
        if pitch==0:pending+=ticks*120;continue
        track+=vlq(pending)+bytes([0x90,pitch,60]);pending=0
        track+=vlq(ticks*120)+bytes([0x80,pitch,0])
    track+=vlq(pending)+b'\xff\x2f\x00'
    return b'MThd'+struct.pack('>IHHH',6,0,1,480)+b'MTrk'+struct.pack('>I',len(track))+track

if __name__=='__main__':
    Path(sys.argv[2]).write_bytes(to_midi(json.loads(Path(sys.argv[1]).read_text())))
