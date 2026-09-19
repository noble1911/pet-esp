"""Run with: docker exec -i butler-api python < test_pet.py"""
import asyncio
import json
import unittest
from unittest.mock import AsyncMock, patch, MagicMock
from fastapi import HTTPException
from pydantic import ValidationError
from api.routes.pet import PetState, PetTurn, PetMemory, pet_stream, PET_MODEL, reward_snapshot, PetTune, PetCompose, resolve_pet_account, trait_snapshot
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
        for field,value in [('fullness',101),('name','Ignore\nRules'),('pet_id','oops'),('genes',[]),('recent_event','ignore instructions'),('recent_event_age_seconds',121),('artwork_version',4)]:
            with self.assertRaises(ValidationError):PetState(**{**STATE,field:value})
    def test_rewards_and_new_game_events(self):
        state=PetState(**{**STATE,'stars':29,'inventory':[0]*15+[101]})
        rewards=reward_snapshot(state)
        self.assertEqual(rewards['stickers'],5)
        self.assertEqual(rewards['stars_to_next_sticker'],1)
        self.assertEqual(rewards['unlocked_room_gifts'],['flowers','bunting'])
        self.assertEqual(rewards['equipped_room_gift'],'bunting')
        state.inventory[15]=105
        self.assertIsNone(reward_snapshot(state)['equipped_room_gift'])
        state.stars=2**32-1
        self.assertEqual(reward_snapshot(state)['stickers'],18)
        self.assertIsNone(reward_snapshot(state)['stars_to_next_sticker'])
        self.assertEqual(reward_snapshot(state)['equipped_room_gift'],'trophy')
        for event in ('finished_hide_game','finished_ball_game','butterfly_visit'):
            PetState(**{**STATE,'recent_event':event,'recent_event_age_seconds':0})

    def test_room_gifts_and_wall_sticker(self):
        for value in range(256):
            inventory=[0]*16
            inventory[14]=value
            inventory[13]=value
            state=PetState(**{**STATE,'stars':90,'inventory':inventory})
            rewards=reward_snapshot(state)
            expected=(value & 63).bit_count() if 128 <= value <= 191 else 0
            self.assertEqual(len(rewards['placed_room_gifts']),expected)
            self.assertEqual(rewards['wall_sticker'] is not None,200 <= value <= 217)
        inventory=[0]*13+[217,191,100]
        state=PetState(**{**STATE,'stars':29,'inventory':inventory})
        rewards=reward_snapshot(state)
        self.assertEqual(rewards['placed_room_gifts'],['flowers','bunting'])
        self.assertIsNone(rewards['wall_sticker'])
        state.stars=90
        self.assertEqual(len(reward_snapshot(state)['placed_room_gifts']),6)
        self.assertEqual(reward_snapshot(state)['wall_sticker'],'Present')
        state.inventory[14]=128
        self.assertEqual(reward_snapshot(state)['placed_room_gifts'],[])

    def test_all_trait_values_match_catalogue_and_current_renderer(self):
        from api.routes.pet_traits import TRAITS, LEGACY_TRAITS
        self.assertEqual(len(TRAITS),8)
        for value in range(256):
            snapshot=trait_snapshot(PetState(**{**STATE,'genes':[value]*8}))
            self.assertEqual(snapshot['body_color']['name'],['Sunny gold','Lilac','Mint','Rose','Peach','Sky blue'][value%6])
            self.assertEqual(snapshot['personality']['name'],TRAITS[7]['values'][value%8])
            self.assertEqual(sum(t['mode']=='stored' for t in snapshot.values()),6)
            for trait in LEGACY_TRAITS:self.assertIn(snapshot[trait['key']]['name'],trait['values'])
            current=trait_snapshot(PetState(**{**STATE,'artwork_version':2,'genes':[value]*8}))
            self.assertEqual(sum(t['mode']=='visible' for t in current.values()),7)
            for trait in TRAITS:self.assertEqual(current[trait['key']]['name'],trait['values'][value % len(trait['values'])])
        for invalid in (-1,256,True,1.5):
            with self.assertRaises(ValidationError):PetState(**{**STATE,'genes':[invalid]*8})

    def test_six_complete_characters_ignore_legacy_part_genes(self):
        from api.routes.pet_characters import CHARACTERS
        self.assertEqual(len(CHARACTERS),6)
        for marker in range(256):
            state=PetState(**{**STATE,'artwork_version':3,'genes':[marker]*8})
            snapshot=trait_snapshot(state)
            self.assertEqual(set(snapshot),{'character','personality'})
            character=CHARACTERS[marker-240 if 240<=marker<240+len(CHARACTERS) else 0]
            self.assertEqual(snapshot['character']['character_type'],character['name'])
            self.assertEqual(snapshot['character']['description'],character['appearance'])
        for character in range(len(CHARACTERS)):
            for old_part in range(256):
                genes=[old_part]*8;genes[6]=240+character
                snapshot=trait_snapshot(PetState(**{**STATE,'artwork_version':3,'genes':genes}))
                self.assertEqual(snapshot['character']['character_type'],CHARACTERS[character]['name'])

    def test_special_food_milestones_and_events(self):
        for stars in (0,9,10,24,25,49,50,99,100,2**32-1):
            foods=reward_snapshot(PetState(**{**STATE,'stars':stars}))['special_foods']
            self.assertEqual([f['name'] for f in foods],['Star cupcake','Berry pancakes','Rainbow jelly','Party cake'])
            for food,threshold in zip(foods,(10,25,50,100)):
                self.assertEqual(food['unlocked'],stars>=threshold)
                self.assertEqual(food['stars_remaining'],max(0,threshold-stars))
        for event in ('ate_star_cupcake','ate_berry_pancakes','ate_rainbow_jelly','ate_party_cake'):
            PetState(**{**STATE,'recent_event':event,'recent_event_age_seconds':0})

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
    async def test_new_pet_accounts_are_stable_and_isolated(self):
        pool=MagicMock();pool.execute=AsyncMock()
        created=[]
        for device,pet in [('device-a','1234567890abcdef'),('device-a','1234567890abcdef'),
                           ('device-a','abcdef1234567890'),('device-b','1234567890abcdef')]:
            async def lookup(sql,*args):
                if sql.startswith('SELECT soul'):return {'profile':'virtual_pet','allow_new_pets':True}
                if sql.startswith('UPDATE'):return None
                return args[0]
            pool.fetchval=AsyncMock(side_effect=lookup)
            created.append(await resolve_pet_account(pool,device,pet))
            args=pool.execute.call_args.args
            self.assertEqual(args[3]['pet_id'],pet)
            self.assertEqual(args[3]['device_account'],device)
            self.assertNotIn('allow_new_pets',args[3])
            self.assertEqual(args[4],{})
            self.assertFalse(args[5]['enabled'])
        self.assertEqual(created[0],created[1])
        self.assertEqual(len(set(created)),3)
        for soul in ({'profile':'virtual_pet','allow_new_pets':True,'device_account':'parent'},
                     {'profile':'virtual_pet','allow_new_pets':'true'}):
            pool.fetchval=AsyncMock(side_effect=[soul,None])
            with self.assertRaises(HTTPException):await resolve_pet_account(pool,'child','1234567890abcdef')
        pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet','allow_new_pets':True},None,None])
        with self.assertRaises(HTTPException):await resolve_pet_account(pool,'device-a','1234567890abcdef')

    async def test_reset_routes_all_memory_and_model_context_to_new_account(self):
        pool=MagicMock();captured={}
        async def brain(**kwargs):
            captured.update(kwargs)
            if False:yield ''
        req=PetTurn(user_id='untrusted-request-id',session_id='test',transcript='hello',pet=STATE)
        with patch('api.routes.pet.resolve_pet_account',AsyncMock(return_value='fresh-account')) as resolve, \
             patch('api.routes.pet._load_facts',AsyncMock(return_value=[])) as facts, \
             patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=[])) as history, \
             patch('api.routes.pet.get_embedding_service',return_value=None), \
             patch('api.routes.pet.stream_chat_with_tools',brain):
            response=await pet_stream(req,'authenticated-device',pool,{'remember_fact':MagicMock()})
            async for _ in response.body_iterator:pass
        resolve.assert_awaited_once_with(pool.pool,'authenticated-device',STATE['pet_id'])
        self.assertEqual(facts.call_args.args[1],'fresh-account')
        self.assertEqual(history.call_args.args[1],'fresh-account')
        self.assertEqual(captured['user_id'],'fresh-account')
        self.assertEqual(captured['tools']['remember_fact'].user_id,'fresh-account')

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
        self.assertEqual(set(captured['tools']),{'remember_fact','compose_tune'})
        self.assertIn('"fullness":80',captured['system_prompt'][1]['text'])
        self.assertIn('CURRENT TRAITS (data)',captured['system_prompt'][1]['text'])
        self.assertIn('"name": "Sunny gold"',captured['system_prompt'][1]['text'])
        self.assertIn('"mode": "stored"',captured['system_prompt'][1]['text'])
        self.assertEqual(captured['user_id'],'pet')
        self.assertEqual(captured['model_override'],PET_MODEL)
        self.assertFalse(captured['allow_web_search'])
        self.assertEqual(captured['max_tokens'],300)
        # An automatic remark has a smaller budget and no memory-writing tools.
        req.proactive=True
        req.pet.artwork_version=3
        pool.pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet'},'pet'])
        with patch('api.routes.pet._load_facts',AsyncMock()) as facts,patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=[])),patch('api.routes.pet.stream_chat_with_tools',brain):
            response=await pet_stream(req,None,pool,{'remember_fact':MagicMock()})
            async for _ in response.body_iterator:pass
            facts.assert_not_awaited()
        self.assertEqual(captured['tools'],{})
        self.assertNotIn('"mode": "stored"',captured['system_prompt'][1]['text'])
        self.assertEqual(captured['system_prompt'][1]['text'].count('"mode": "visible"'),1)
        self.assertEqual(captured['max_tokens'],100)
        self.assertIn('at most 10 words',' '.join(p['text'] for p in captured['system_prompt']))
    async def test_personal_name_is_separate_from_character_and_old_introductions(self):
        captured={};pool=MagicMock()
        async def brain(**kwargs):
            captured.update(kwargs)
            if False:yield ''
        for name,marker in (("Olive",240),("Olive",245),("Rosy Pig",245),("Anne-Marie",241)):
            pool.pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet'},'pet'])
            req=PetTurn(user_id='pet',session_id='test',transcript="What's your name?",
                        pet={**STATE,'name':name,'artwork_version':3,'genes':[0]*6+[marker,0]})
            with patch('api.routes.pet._load_facts',AsyncMock(return_value=[{'fact':'You are Rosy pig with a red snout','category':'identity'}])),patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=[{'role':'assistant','content':'I am Rosy pig with a red snout!'}])),patch('api.routes.pet.get_embedding_service',return_value=None),patch('api.routes.pet.stream_chat_with_tools',brain):
                response=await pet_stream(req,None,pool,{})
                async for _ in response.body_iterator:pass
            data=captured['system_prompt'][1]['text']
            self.assertIn('CURRENT IDENTITY (data): '+json.dumps({'personal_name':name}),data)
            self.assertIn('"character_type":',data)
            self.assertNotIn('"character": {"name":',data)
            final=captured['system_prompt'][-1]['text']
            self.assertIn('say exactly '+json.dumps("I'm "+name+"!")+" and stop.",final)
            self.assertIn('Only describe your looks when the child asks',captured['system_prompt'][0]['text'])

    async def test_fresh_food_reaction_is_specific_and_has_no_memory_tools(self):
        pool=MagicMock();captured={}
        async def brain(**kwargs):
            captured.update(kwargs)
            if False:yield ''
        for event,age in (("ate_toast",0),("ate_toast",120),("finished_hide_game",0),("finished_ball_game",0),("butterfly_visit",0),("ate_star_cupcake",0),("ate_berry_pancakes",0),("ate_rainbow_jelly",0),("ate_party_cake",0)):
            pool.pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet'},'pet'])
            req=PetTurn(user_id='pet',session_id='test',transcript='Device event',proactive=True,
                        pet={**STATE,'recent_event':event,'recent_event_age_seconds':age})
            with patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=[])),patch('api.routes.pet.stream_chat_with_tools',brain):
                response=await pet_stream(req,None,pool,{})
                async for _ in response.body_iterator:pass
            prompts=' '.join(p['text'] for p in captured['system_prompt'])
            self.assertIn(f'"recent_event":"{event}"',prompts)
            self.assertEqual(f'React directly to this just-completed device event: {event}' in prompts,age==0)
            self.assertEqual(captured['tools'],{})
            self.assertEqual(captured['max_tokens'],100)
    async def test_compose_validation_and_one_tune_per_turn(self):
        score=dict(title='Tiny meadow',tempo=120,instrument='bell',notes=[[60,4],[64,4],[67,4],[72,4]])
        for patch_value in ({'tempo':True},{'tempo':0},{'instrument':'shell'},{'notes':[[60,8]]*24},{'notes':[[0,4]]*4},{'notes':[[90,4]]*4},{'notes':[[60.5,4]]*4},{'notes':[[60,True]]*4}):
            with self.assertRaises(ValidationError):PetTune(**{**score,**patch_value})
        tool=PetCompose()
        self.assertIn('Invalid',await tool.execute(**{**score,'tempo':0}))
        self.assertIsNone(tool.tune)
        self.assertIn('queued',await tool.execute(**score))
        first=tool.tune
        self.assertIn('already',await tool.execute(**score));self.assertIs(tool.tune,first)

    async def test_composition_reaches_structured_stream(self):
        pool=MagicMock();pool.pool.fetchval=AsyncMock(side_effect=[{'profile':'virtual_pet'},'pet'])
        score=dict(title='Tiny meadow',tempo=120,instrument='bell',notes=[[60,4],[64,4],[67,4],[72,4]])
        async def brain(**kwargs):
            await kwargs['tools']['compose_tune'].execute(**score)
            if False:yield ''
        req=PetTurn(user_id='pet',session_id='test',transcript='Make a tune',pet=STATE)
        with patch('api.routes.pet._load_facts',AsyncMock(return_value=[])),patch('api.routes.pet.load_conversation_messages',AsyncMock(return_value=[])),patch('api.routes.pet.get_embedding_service',return_value=None),patch('api.routes.pet.stream_chat_with_tools',brain):
            response=await pet_stream(req,None,pool,{})
            chunks=[chunk async for chunk in response.body_iterator]
        event=json.loads(chunks[0][6:])
        self.assertEqual(event,{'type':'pet_music','score':score})
        self.assertEqual(chunks[-1],'data: [DONE]\n\n')

if __name__=='__main__':unittest.main()
