"""Audition deployed character prompts with Haiku, without reading/writing pet data.
Run: ssh HOST 'docker exec -i butler-api python' < check_character_idle.py
Prints JSON; makes one real model call per character, then repeat checks for adults.
"""
import asyncio
import json
from unittest.mock import AsyncMock, MagicMock, patch
from api.routes.pet import PetTurn, pet_stream, PET_MODEL
from api.routes.pet_characters import CHARACTERS
from api.llm import stream_chat_with_tools

async def sample(index, history):
    captured = {}
    async def capture(**kwargs):
        captured.update(kwargs)
        if False:
            yield ''
    state = dict(pet_id='0123456789abcdef',name='Olive',stage=3,fullness=80,
                 happiness=85,energy=75,cleanliness=90,stars=35,genes=[0]*6+[240+index,0],
                 generation=0,inventory=[0]*16,friends_met=0,activity='home',artwork_version=3)
    request = PetTurn(user_id='audition-no-account',session_id='idle-audition',
                      transcript='A quiet thought',proactive=True,pet=state)
    with patch('api.routes.pet.resolve_pet_account',AsyncMock(return_value='audition-no-account')), \
         patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=history)), \
         patch('api.routes.pet.stream_chat_with_tools',capture):
        response = await pet_stream(request,None,MagicMock(),{})
        async for _ in response.body_iterator:
            pass
    captured['db_pool']=None
    assert captured['tools']=={} and captured['model_override']==PET_MODEL
    result=''
    async for chunk in stream_chat_with_tools(**captured):
        if isinstance(chunk,str): result+=chunk
    result=result.strip()
    assert result
    print(json.dumps(dict(character=CHARACTERS[index]['name'],reply=result,
                          words=len(result.split()),model=PET_MODEL)),flush=True)
    return dict(role='assistant',content=result)

async def main():
    # Old Larry history deliberately accompanies every different character.
    old=[dict(role='assistant',content='My next meeting is with a sandwich.')]
    for index in range(len(CHARACTERS)):
        reply=await sample(index,old)
        if index>=6:
            await sample(index,old+[reply])

asyncio.run(main())
