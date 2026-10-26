from pathlib import Path
import plistlib,json,uuid,secrets,copy
root=Path(__file__).resolve().parent
prefs=plistlib.loads((root.parent/'installed-data/Library/Preferences/com.ledvance.smart.plist').read_bytes())
mesh=json.loads(prefs['mesh.json']);controller_uuid=uuid.uuid4().hex.upper();address=0x7f00
for node in mesh['nodes']:
 lo=int(node['unicastAddress'],16);hi=lo+len(node.get('elements',[]))-1
 assert not lo<=address<=hi
for provider in mesh['provisioners']:
 for allocated in provider['allocatedUnicastRange']:assert not int(allocated['lowAddress'],16)<=address<=int(allocated['highAddress'],16)
local=copy.deepcopy(mesh['nodes'][0]);local.update(UUID=controller_uuid,unicastAddress=f'{address:04X}',name='Light Show independent controller',deviceKey=secrets.token_hex(16).upper())
mesh['nodes'].append(local)
mesh['provisioners'].append({'UUID':controller_uuid,'provisionerName':'Light Show','allocatedUnicastRange':[{'lowAddress':'7F00','highAddress':'7FFF'}],'allocatedGroupRange':[{'lowAddress':'D000','highAddress':'D0FF'}],'allocatedSceneRange':[{'firstScene':'7F00','lastScene':'7FFF'}]})
# Device keys are not required to send normal bound light-control messages.
for node in mesh['nodes']:
 if node['UUID']!=controller_uuid:node['deviceKey']='0'*32
seed={'mesh.json':json.dumps(mesh,separators=(',',':')).encode(),'currenProvisionerUUID_key':controller_uuid,'currenProvisionerSno_key':0,'currentMeshProvisionAddress_key':{controller_uuid:address+1}}
p=root/'LightShowSeed.plist';p.write_bytes(plistlib.dumps(seed,fmt=plistlib.FMT_BINARY));p.chmod(0o600)
assert mesh['provisioners'][0]['UUID']!=controller_uuid
print('Private seed created: original pairing keys retained; distinct controller at 7F00; original files unchanged. Keys not printed.')
