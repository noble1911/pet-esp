"""Run with: docker exec -i butler-api python < test_pet.py"""
import asyncio
import unittest
from unittest.mock import AsyncMock, patch, MagicMock
from fastapi import HTTPException
from pydantic import ValidationError
from api.routes.pet import PetState, PetTurn, PetMemory, pet_stream

STATE=dict(pet_id='1234567890abcdef',name='Sprout',stage=2,fullness=80,
    happiness=70,energy=60,cleanliness=90,stars=15,genes=[0]*8,generation=0,inventory=[0]*16,
    friends_met=0,activity='talking')
class PetTests(unittest.IsolatedAsyncioTestCase):
    def test_snapshot_validation(self):
        PetState(**STATE)
        for field,value in [('fullness',101),('name','Ignore\nRules'),('pet_id','oops'),('genes',[])]:
            with self.assertRaises(ValidationError):PetState(**{**STATE,field:value})
    async def test_memory_cannot_read_another_user(self):
        tool=MagicMock();tool.name='recall_facts';tool.description='Recall';tool.parameters={'properties':{'user_id':{'type':'string'},'query':{'type':'string'}},'required':['user_id']};tool.execute=AsyncMock(return_value='ok')
        bound=PetMemory(tool,'pet-only')
        self.assertNotIn('user_id',bound.parameters['properties'])
        await bound.execute(user_id='adult',query='hello')
        tool.execute.assert_awaited_once_with(user_id='pet-only',query='hello')
    async def test_adult_account_rejected(self):
        pool=MagicMock();pool.pool.fetchval=AsyncMock(return_value={'profile':'adult'})
        req=PetTurn(user_id='adult',session_id='test',transcript='hello',pet=STATE)
        with self.assertRaises(HTTPException) as c:await pet_stream(req,None,pool,{})
        self.assertEqual(c.exception.status_code,403)
    async def test_wrong_pet_rejected(self):
        pool=MagicMock();pool.pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet'},None])
        req=PetTurn(user_id='pet',session_id='test',transcript='hello',pet=STATE)
        with self.assertRaises(HTTPException) as c:await pet_stream(req,None,pool,{})
        self.assertEqual(c.exception.status_code,409)
    async def test_only_memory_tools_and_current_snapshot_reach_model(self):
        pool=MagicMock();pool.pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet'},'pet'])
        req=PetTurn(user_id='pet',session_id='test',transcript='hello',pet=STATE)
        captured={}
        async def brain(**kwargs):
            captured.update(kwargs)
            if False:yield ''
        with patch('api.routes.pet._load_facts',AsyncMock(return_value=[])),patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=[])),patch('api.routes.pet.get_embedding_service',return_value=None),patch('api.routes.pet.stream_chat_with_tools',brain):
            response=await pet_stream(req,None,pool,{'remember_fact':MagicMock(),'home_assistant':MagicMock(),'display_image':MagicMock()})
            async for _ in response.body_iterator:pass
        self.assertEqual(set(captured['tools']),{'remember_fact'})
        self.assertIn('"fullness":80',captured['system_prompt'][1]['text'])
        self.assertEqual(captured['user_id'],'pet')
if __name__=='__main__':unittest.main()
