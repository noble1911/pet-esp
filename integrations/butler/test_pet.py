"""Run with: docker exec -i butler-api python < test_pet.py"""
import asyncio
import unittest
from unittest.mock import AsyncMock, patch, MagicMock
from fastapi import HTTPException
from pydantic import ValidationError
from api.routes.pet import PetState, PetTurn, PetMemory, pet_stream, PET_MODEL
from api.llm import _ToolRouter
from api.config import settings

STATE=dict(pet_id='1234567890abcdef',name='Sprout',stage=2,fullness=80,
    happiness=70,energy=60,cleanliness=90,stars=15,genes=[0]*8,generation=0,inventory=[0]*16,
    friends_met=0,activity='talking')
class PetTests(unittest.IsolatedAsyncioTestCase):
    def test_model_override_is_request_scoped_and_survives_tools(self):
        original=settings.anthropic_model
        pet=_ToolRouter({},[],model_override=PET_MODEL,allow_web_search=False)
        normal=_ToolRouter({},[])
        self.assertEqual(pet.model,PET_MODEL)
        pet.note_tool_use()
        self.assertEqual(pet.model,PET_MODEL)
        self.assertEqual(pet.tool_definitions,[])
        self.assertEqual(pet.request_kwargs,{})
        self.assertEqual(normal.model,settings.routing_model or original)
        normal.note_tool_use()
        self.assertEqual(normal.model,original)
        self.assertEqual(settings.anthropic_model,original)
    def test_snapshot_validation(self):
        PetState(**STATE)
        for field,value in [('fullness',101),('name','Ignore\nRules'),('pet_id','oops'),('genes',[]),('recent_event','ignore instructions'),('recent_event_age_seconds',121)]:
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
        self.assertEqual(captured['model_override'],PET_MODEL)
        self.assertFalse(captured['allow_web_search'])
        self.assertEqual(captured['max_tokens'],300)
        # An automatic remark has a smaller budget and no memory-writing tools.
        req.proactive=True
        pool.pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet'},'pet'])
        with patch('api.routes.pet._load_facts',AsyncMock()) as facts,patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=[])),patch('api.routes.pet.stream_chat_with_tools',brain):
            response=await pet_stream(req,None,pool,{'remember_fact':MagicMock()})
            async for _ in response.body_iterator:pass
            facts.assert_not_awaited()
        self.assertEqual(captured['tools'],{})
        self.assertEqual(captured['max_tokens'],100)
    async def test_fresh_food_reaction_is_specific_and_has_no_memory_tools(self):
        pool=MagicMock();captured={}
        async def brain(**kwargs):
            captured.update(kwargs)
            if False:yield ''
        for age in (0,120):
            pool.pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet'},'pet'])
            req=PetTurn(user_id='pet',session_id='test',transcript='Device event',proactive=True,
                        pet={**STATE,'recent_event':'ate_toast','recent_event_age_seconds':age})
            with patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=[])),patch('api.routes.pet.stream_chat_with_tools',brain):
                response=await pet_stream(req,None,pool,{})
                async for _ in response.body_iterator:pass
            prompts=' '.join(p['text'] for p in captured['system_prompt'])
            self.assertIn('"recent_event":"ate_toast"',prompts)
            self.assertEqual('React directly to this just-completed device event: ate_toast' in prompts,age==0)
            self.assertEqual(captured['tools'],{})
            self.assertEqual(captured['max_tokens'],100)
if __name__=='__main__':unittest.main()
