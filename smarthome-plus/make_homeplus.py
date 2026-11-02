"""Build private, distinct SYLVANIA Plus controllers from the user's own backups.

Only source code goes to GitHub. App binaries, home databases and all mesh keys
are handled locally. Controller identities persist across rebuilds.
"""
from pathlib import Path
import argparse, copy, hashlib, io, json, plistlib, secrets, shutil, struct, subprocess, uuid, zipfile

ROOT = Path(__file__).resolve().parent
SOURCE_HASH = '6c74a0bf6609ccf3ce5a132318eaf31f411743d8a78ce2b760ed15c813788eb2'
GATE = 0x1A9198
EXPECTED_GATE = bytes.fromhex('ea030054')  # b.ge to the ten-routine rejection branch
NOP = bytes.fromhex('1f2003d5')
BUNDLE_ID = 'com.ledvance.smart.plus'

def executable_offset(binary, address):
    offset = 32
    for _ in range(struct.unpack_from('<I', binary, 16)[0]):
        command, size = struct.unpack_from('<II', binary, offset)
        if command == 0x19:
            va, _, file_offset, file_size = struct.unpack_from('<QQQQ', binary, offset + 24)
            if va <= address < va + file_size:
                return file_offset + address - va
        offset += size
    raise ValueError('Address not found')

def patch_executable(source):
    assert hashlib.sha256(source).hexdigest() == SOURCE_HASH, 'Exact supported app build required'
    b = bytearray(source)
    gate = executable_offset(b, 0x100000000 + GATE)
    assert b[gate:gate+4] == EXPECTED_GATE
    # Keep the compare intact; remove only the conditional branch into rejection.
    b[gate:gate+4] = NOP
    commands, size = struct.unpack_from('<II', b, 16)
    offset = 32
    for _ in range(commands):
        command, length = struct.unpack_from('<II', b, offset)
        if command == 0x2c: assert struct.unpack_from('<I',b,offset+16)[0] == 0
        offset += length
    name = b'@executable_path/Frameworks/HomePlus.dylib\0'
    length = (24 + len(name) + 7) & ~7
    assert not any(b[offset:offset+length]), 'Insufficient load-command padding'
    load = struct.pack('<IIIIII',12,length,24,0,0x10000,0x10000)+name
    b[offset:offset+length] = load + bytes(length-len(load))
    struct.pack_into('<II', b, 16, commands+1, size+length)
    return b

def profiles():
    path = ROOT/'private-profiles.plist'
    if path.exists(): return plistlib.loads(path.read_bytes())
    result = [dict(name=f'Device {index+1}', uuid=uuid.uuid4().hex.upper(),
                   deviceKey=secrets.token_hex(16).upper(), address=address)
              for index,address in enumerate([0x7e00,0x7d00,0x7c00])]
    path.write_bytes(plistlib.dumps(result));path.chmod(0o600)
    return result

def add_controllers(mesh, identities):
    mesh = copy.deepcopy(mesh)
    local = mesh['nodes'][0]
    for identity in identities:
        address=identity['address']
        for node in mesh['nodes']:
            start=int(node['unicastAddress'],16)
            assert not start<=address<=start+len(node.get('elements',[]))-1
        for controller in mesh['provisioners']:
            for allocated in controller['allocatedUnicastRange']:
                assert not int(allocated['lowAddress'],16)<=address+0xff or int(allocated['highAddress'],16)<address
        node = copy.deepcopy(local)
        node.update(UUID=identity['uuid'],deviceKey=identity['deviceKey'],
                    name='SYLVANIA Plus '+identity['name'],unicastAddress=f'{address:04X}')
        mesh['nodes'].append(node)
        mesh['provisioners'].append(dict(UUID=identity['uuid'],provisionerName='SYLVANIA Plus '+identity['name'],
            allocatedUnicastRange=[dict(lowAddress=f'{address:04X}',highAddress=f'{address+0xff:04X}')],
            allocatedGroupRange=[dict(lowAddress=f'{0xd000+(0x7e00-address):04X}',highAddress=f'{0xd0ff+(0x7e00-address):04X}')],
            allocatedSceneRange=[dict(firstScene=f'{address:04X}',lastScene=f'{address+0xff:04X}')]))
    return mesh

def verify_signatures(path):
    b=path.read_bytes()
    if b[:4] != bytes.fromhex('cffaedfe'): return 0
    offset=32; checked=0
    for _ in range(struct.unpack_from('<I',b,16)[0]):
        command,size=struct.unpack_from('<II',b,offset)
        if command==29:
            start,length=struct.unpack_from('<II',b,offset+8)
            for j in range(struct.unpack_from('>I',b,start+8)[0]):
                slot,relative=struct.unpack_from('>II',b,start+12+j*8);q=start+relative
                if struct.unpack_from('>I',b,q)[0]!=0xfade0c02:continue
                hash_offset=struct.unpack_from('>I',b,q+16)[0]
                count,limit=struct.unpack_from('>II',b,q+28)
                hash_size,hash_type,_,page_shift=struct.unpack_from('BBBB',b,q+36)
                algorithm={1:hashlib.sha1,2:hashlib.sha256,3:hashlib.sha256,4:hashlib.sha384}[hash_type]
                for page in range(count):
                    block=b[page*(1<<page_shift):min((page+1)*(1<<page_shift),limit)]
                    assert algorithm(block).digest()[:hash_size]==b[q+hash_offset+page*hash_size:q+hash_offset+(page+1)*hash_size],str(path)
                checked+=1
        offset+=size
    assert checked,str(path)
    return checked

def build(config_zip, module, ldid):
    config = zipfile.ZipFile(config_zip)
    original_ipa = config.read('Documents/decrypted-app.ipa')
    source = zipfile.ZipFile(io.BytesIO(original_ipa))
    app_path = 'Payload/SmartHome Prd.app/'
    original_binary = source.read(app_path+'SmartHome Prd')
    patched_binary = patch_executable(original_binary)
    prefs = plistlib.loads(config.read('Library/Preferences/com.ledvance.smart.plist'))
    identities = profiles()
    mesh = add_controllers(json.loads(prefs['mesh.json']),identities)
    # All current light pairings, device keys, group subscriptions and names remain intact.
    assert len(mesh['nodes'])==18 and len(mesh['groups'])==5
    network_name = next(n for n in config.namelist() if n.startswith('Documents/Network/network_'))
    network_mesh = add_controllers(json.loads(config.read(network_name)),identities)
    documents = {n.removeprefix('Documents/'):config.read(n) for n in
                 ['Documents/default.realm','Documents/UserModel.sqlite','Documents/networkCompareList.archiver','Documents/AppStatusAssistant.archiver']}
    news = plistlib.loads(documents['AppStatusAssistant.archiver'])
    for obj in news['$objects']:
        if isinstance(obj,dict) and 'isNotifyNewApp' in obj:
            # These point to archived NSNumber booleans, not arbitrary model bytes.
            for key in ['isNotifyNewApp','isForceUpdateNewApp']:
                news['$objects'][obj[key].data] = False
    documents['AppStatusAssistant.archiver']=plistlib.dumps(news,fmt=plistlib.FMT_BINARY)
    documents[network_name.removeprefix('Documents/')]=json.dumps(network_mesh,separators=(',',':')).encode()
    output = ROOT/'release';output.mkdir(exist_ok=True)
    report=[]
    for index,identity in enumerate(identities,1):
        # Work only in a dedicated, newly created tree. Existing releases stay available.
        staging=ROOT/('stage-'+identity['uuid'])
        if staging.exists():shutil.rmtree(staging)
        source.extractall(staging)
        app=staging/app_path
        for directory in ['_CodeSignature','SC_Info']:
            p=app/directory
            if p.exists():shutil.rmtree(p)
        for filename in ['embedded.mobileprovision']:
            p=app/filename
            if p.exists():p.unlink()
        binary=app/'SmartHome Prd';binary.write_bytes(patched_binary)
        shutil.copy2(module,app/'Frameworks/HomePlus.dylib')
        info=plistlib.loads((app/'Info.plist').read_bytes())
        info.update(CFBundleIdentifier=BUNDLE_ID,CFBundleDisplayName='SYLVANIA Plus',CFBundleName='SYLVANIA Plus',
                    CFBundleVersion='3300',CFBundleShortVersionString='3.3.0',UIUserInterfaceStyle='Dark',
                    FirebaseAnalyticsCollectionEnabled=False,FirebaseCrashlyticsCollectionEnabled=False)
        info.pop('CFBundleURLTypes',None)
        info['UTExportedTypeDeclarations']=[{'UTTypeIdentifier':'com.ledvance.smart.plus.home','UTTypeDescription':'SYLVANIA Plus Home',
            'UTTypeConformsTo':['public.data'],'UTTypeTagSpecification':{'public.filename-extension':['sylhome','sylbackup'],'public.mime-type':'application/x-sylvania-home'}}]
        (app/'Info.plist').write_bytes(plistlib.dumps(info,fmt=plistlib.FMT_BINARY))
        private_prefs=copy.deepcopy(prefs)
        private_prefs.update({'mesh.json':json.dumps(mesh,separators=(',',':')).encode(),
            'currenProvisionerUUID_key':identity['uuid'],'currenProvisionerSno_key':0,
            'currentMeshProvisionAddress_key':{identity['uuid']:identity['address']+1},
            'uniqueID':str(uuid.UUID(identity['uuid'])).upper(),'InfoAlertPresented':True,'SHOWALERT':False,'HP.Profile':identity['name']})
        seed={'Profile':identity['name'],'Summary':'14 lights / 5 groups','Preferences':private_prefs,'Documents':documents}
        (app/'HomePlusSeed.plist').write_bytes(plistlib.dumps(seed,fmt=plistlib.FMT_BINARY))
        # Separate container and application identity preserve the installed original app.
        entitlements={'application-identifier':'4657EYLGTH.'+BUNDLE_ID,
            'com.apple.developer.team-identifier':'4657EYLGTH','com.apple.developer.homekit':True,
            'com.apple.private.security.container-required':BUNDLE_ID,
            'com.apple.multitasking.unlimitedassertions':True,'com.apple.runningboard.assertions':True}
        entitlement_path=ROOT/'homeplus-entitlements.plist';entitlement_path.write_bytes(plistlib.dumps(entitlements))
        for path in app.rglob('*'):
            if not path.is_file():continue
            if path.read_bytes()[:4] in [bytes.fromhex('cffaedfe'),bytes.fromhex('cafebabe')]:
                path.chmod(0o755)
                subprocess.run([str(ldid),'-S'+str(entitlement_path) if path==binary else '-S',str(path)],check=True)
        verify_signatures(binary);verify_signatures(app/'Frameworks/HomePlus.dylib')
        out=output/f'SYLVANIA_Plus_Device_{index}_iOS15.ipa'
        with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(staging.rglob('*')):
                if path.is_file():archive.write(path,path.relative_to(staging))
        with zipfile.ZipFile(out) as archive:
            assert archive.testzip() is None
            verify=plistlib.loads(archive.read(app_path+'HomePlusSeed.plist'))
            assert verify['Preferences']['currenProvisionerUUID_key']==identity['uuid']
            assert archive.read(app_path+'SmartHome Prd')[GATE:GATE+4]==NOP
            assert verify['Documents']['default.realm']==config.read('Documents/default.realm')
        report.append({'filename':out.name,'bytes':out.stat().st_size,'sha256':hashlib.sha256(out.read_bytes()).hexdigest(),
                       'profile':identity['name'],'controller_address':f"{identity['address']:04X}"})
    (output/'SHA256.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'built':report,'original_unchanged':True,'scene_gate':'removed','private_configuration_uploaded':False}))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--config',type=Path,required=True)
    parser.add_argument('--module',type=Path,required=True);parser.add_argument('--ldid',type=Path,required=True)
    args=parser.parse_args();build(args.config,args.module,args.ldid)
