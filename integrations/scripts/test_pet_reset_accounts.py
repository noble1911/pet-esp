"""Live DB check using temporary accounts inside an always-rolled-back transaction.
Run: docker exec -i butler-api python < test_pet_reset_accounts.py
No model calls and no changes to the real pet account.
"""
import asyncio
import uuid
from types import SimpleNamespace
from tools import DatabasePool
from api.routes.pet import resolve_pet_account
from api.context import _load_facts, load_conversation_messages

async def main():
    db = await DatabasePool.create()
    try:
        async with db.pool.acquire() as conn:
            tx = conn.transaction()
            await tx.start()
            try:
                device = 'pet-reset-test-' + uuid.uuid4().hex
                old_id, new_id = '1111111111111111', '2222222222222222'
                await conn.execute('INSERT INTO butler.users (id,name,soul) VALUES ($1,$2,$3::jsonb)',
                    device, 'Temporary reset test', {'profile':'virtual_pet','allow_new_pets':True,'pet_id':old_id})
                await conn.execute("INSERT INTO butler.user_facts (user_id,fact) VALUES ($1,'Original pet test memory')",device)
                await conn.execute("INSERT INTO butler.conversation_history (user_id,channel,role,content) VALUES ($1,'voice','user','Original pet test chat')",device)
                assert await resolve_pet_account(conn,device,old_id) == device
                fresh = await resolve_pet_account(conn,device,new_id)
                assert fresh != device
                assert await resolve_pet_account(conn,device,new_id) == fresh
                assert await _load_facts(conn,fresh) == []
                assert await load_conversation_messages(SimpleNamespace(pool=conn),fresh,channel='voice') == []
                assert len(await _load_facts(conn,device)) == 1
                assert len(await load_conversation_messages(SimpleNamespace(pool=conn),device,channel='voice')) == 1
                await conn.execute("INSERT INTO butler.user_facts (user_id,fact) VALUES ($1,'Fresh pet test memory')",fresh)
                assert (await _load_facts(conn,fresh))[0]['fact'] == 'Fresh pet test memory'
                next_pet = await resolve_pet_account(conn,device,'3333333333333333')
                assert next_pet not in (device,fresh)
                assert await _load_facts(conn,next_pet) == []
                assert await resolve_pet_account(conn,device,old_id) == device
                print('PASS: original memory retained; two fresh pets isolated; reconnect identity stable')
            finally:
                await tx.rollback()
                print('Temporary accounts and data rolled back')
    finally:
        await db.close()

asyncio.run(main())
