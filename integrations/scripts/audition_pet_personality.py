"""Live Haiku evaluation using a temporary pet account, deleted in finally.
Run inside butler-api. Uses the current production route and speaking directions.
Writes /tmp/pet-personality-v2.json. Never touches the real pet's memory.
"""
import asyncio,json,uuid
from pathlib import Path
from tools import DatabasePool
from api.routes import pet as route

STATE=dict(pet_id='1234567890abcdef',name='Sprout',stage=3,fullness=80,happiness=90,
    energy=80,cleanliness=90,stars=35,genes=[0,2,0,0,0,0,0,7],generation=0,inventory=[0]*16,
    friends_met=0,activity='home')
CASES=[('hello','Hi Sprout!',{},False),
       ('play','What shall we play?',{},False),
       ('meal','Device event',{'recent_event':'ate_berry_pancakes','recent_event_age_seconds':0},True),
       ('hungry','Is your tummy full?',{'fullness':20},False),
       ('locked_food','Can I have party cake?',{},False),
       ('identity','Are you a real animal?',{},False),
       ('quiet','Quiet moment',{},True),
       ('feisty','You are a silly potato!',{'genes':[0,2,0,0,0,0,0,4]},False)]
async def main():
 db=await DatabasePool.create();user='pet-meadow-voice-eval-'+uuid.uuid4().hex
 original=route.PET_RULES;results=[]
 try:
  await db.pool.execute('INSERT INTO butler.users (id,name,soul,notification_prefs) VALUES ($1,$2,$3::jsonb,$4::jsonb)',user,'Temporary voice evaluation',{'profile':'virtual_pet'},{'enabled':False,'categories':[]})
  profiles=[('creature',original,CASES)]
  for profile,rules,cases in profiles:
   route.PET_RULES=rules
   for case,text,patch,proactive in cases:
    await db.pool.execute('DELETE FROM butler.conversation_history WHERE user_id=$1',user)
    req=route.PetTurn(user_id=user,session_id='voice-eval',transcript=text,pet={**STATE,**patch},proactive=proactive)
    response=await route.pet_stream(req,None,db,{})
    parts=[]
    async for raw in response.body_iterator:
     payload=raw[6:].strip()
     if payload!='[DONE]':parts.append(json.loads(payload).get('delta',''))
    reply=''.join(parts).strip();assert reply
    entry=dict(profile=profile,case=case,reply=reply,words=len(reply.split()))
    results.append(entry);print(json.dumps(entry),flush=True)
  Path('/tmp/pet-personality-v2.json').write_text(json.dumps(results,indent=2)+'\n')
 finally:
  route.PET_RULES=original
  await db.pool.execute('DELETE FROM butler.conversation_history WHERE user_id=$1',user)
  await db.pool.execute('DELETE FROM butler.users WHERE id=$1',user)
  await db.close()
  print('Temporary pet account and conversation history removed',flush=True)
asyncio.run(main())
