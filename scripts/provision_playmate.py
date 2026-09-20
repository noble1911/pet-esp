#!/usr/bin/env python3
"""Register one verified board, preserve its account, and write a private build header.
Does not flash, reset a pet, or restart services. Run with --mac from esptool.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--mac',required=True)
parser.add_argument('--host',default='ron@192.168.1.117')
args=parser.parse_args()
mac=args.mac.replace(':','').lower()
if not re.fullmatch('[0-9a-f]{12}',mac):parser.error('Expected the board MAC address')
user='pet-meadow-'+mac
account='''import asyncio
from tools import DatabasePool
async def main():
    db=await DatabasePool.create()
    soul={"profile":"virtual_pet","allow_new_pets":True,"butler_name":"Sprout","voice":"bf_emma","customInstructions":"Little Meadow virtual pet. Use the dedicated pet voice route."}
    await db.pool.execute("INSERT INTO butler.users (id,name,soul,permissions,notification_prefs) VALUES ($1,$2,$3::jsonb,$4::jsonb,$5::jsonb) ON CONFLICT (id) DO NOTHING", USER,"Little Meadow pet",soul,{}, {"enabled":False,"categories":[]})
    assert await db.pool.fetchval("SELECT soul->>'profile' FROM butler.users WHERE id=$1",USER)=="virtual_pet"
    await db.close()
asyncio.run(main())
'''.replace('USER',repr(user))
subprocess.run(['ssh','-o','BatchMode=yes',args.host,'export PATH=/opt/homebrew/bin:$HOME/.orbstack/bin:$PATH; docker exec -i butler-api python'],input=account,text=True,check=True,capture_output=True)
remote='''import json,secrets,shutil,time
from pathlib import Path
p=Path.home()/'esp-gateway/.env'
lines=p.read_text().splitlines()
def get(key,default):
    for line in lines:
        if line.startswith(key+'='):
            value=line.split('=',1)[1].strip()
            if len(value)>1 and value[0] in "\\\"'" and value[-1]==value[0]:value=value[1:-1]
            return json.loads(value)
    return default
tokens=get('GATEWAY_DEVICE_TOKENS',{})
# Reuse this board's scoped token; never reuse another board's token.
token=next((token for token,users in tokens.items() if users==[USER]),None)
if token is None:token=secrets.token_urlsafe(32);tokens[token]=[USER]
groups=get('PET_PLAYDATE_GROUPS',{})
groups[USER]='little-meadow-home'
updates={'GATEWAY_DEVICE_TOKENS':json.dumps(tokens,separators=(',',':')),'PET_PLAYDATE_GROUPS':json.dumps(groups,separators=(',',':'))}
shutil.copy2(p,p.with_name('.env.before-playdates-'+str(time.time_ns())))
lines=[line for line in lines if not any(line.startswith(key+'=') for key in updates)]
lines.extend(key+"='"+value+"'" for key,value in updates.items())
p.write_text('\\n'.join(lines)+'\\n');p.chmod(0o600)
print(json.dumps({'token':token}))
'''.replace('USER',repr(user))
result=subprocess.run(['ssh','-o','BatchMode=yes',args.host,'python3 -'],input=remote,text=True,check=True,capture_output=True)
token=json.loads(result.stdout)['token']
root=Path(__file__).resolve().parents[1]
private=root/'firmware/private';private.mkdir(exist_ok=True);private.chmod(0o700)
header=private/(mac+'.h')
fd=os.open(header,os.O_WRONLY|os.O_CREAT|os.O_TRUNC,0o600)
with os.fdopen(fd,'w') as f:
    f.write('// Private, board-bound credentials. Never commit.\n#define PET_DEVICE_MAC '+json.dumps(mac)+'\n#define PET_AUTH_TOKEN '+json.dumps(token)+'\n')
print(f'Registered {user}. Private build header: {header}')
print('Restart the gateway to load registration, then build in a separate directory with:')
print(f'./scripts/device.sh -B {root / "firmware" / ("build-" + mac)} -DPET_IDENTITY_HEADER={header} build')
