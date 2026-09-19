"""Run inside butler-api; idempotently provision only the dedicated pet account."""
import asyncio
from tools import DatabasePool
async def main():
    db = await DatabasePool.create()
    soul = {"profile":"virtual_pet", "butler_name":"Sprout", "voice":"bf_emma",
            "customInstructions":"Little Meadow virtual pet. Use the dedicated pet voice route."}
    await db.pool.execute("INSERT INTO butler.users (id,name,soul,permissions,notification_prefs) "
        "VALUES ($1,$2,$3::jsonb,$4::jsonb,$5::jsonb) ON CONFLICT (id) DO NOTHING",
        "pet-meadow-3cdc756e3104", "Little Meadow pet", soul, {}, {"enabled":False,"categories":[]})
    assert await db.pool.fetchval("SELECT soul->>'profile' FROM butler.users WHERE id=$1", "pet-meadow-3cdc756e3104") == "virtual_pet"
    print("Dedicated pet account ready")
    await db.close()
asyncio.run(main())
