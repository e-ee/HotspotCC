from pathlib import Path
import zipfile,plistlib,subprocess,shutil,struct,hashlib,json
root=Path(__file__).resolve().parent
src=root/'SYLVANIA_Light_Show_v0.1_iOS15.ipa'
dst=root/'side-by-side'
if dst.exists():shutil.rmtree(dst)
with zipfile.ZipFile(src) as z:z.extractall(dst)
app=dst/'Payload/SmartHome Prd.app';info=app/'Info.plist'
d=plistlib.loads(info.read_bytes());old=d['CFBundleIdentifier'];new='com.ledvance.smart.lightshow'
d.update(CFBundleIdentifier=new,CFBundleDisplayName='Light Show',CFBundleName='Light Show',CFBundleVersion='200')
d.pop('NSMicrophoneUsageDescription',None)
# Prevent this separate app from taking over the original app's URL handlers.
d.pop('CFBundleURLTypes',None)
info.write_bytes(plistlib.dumps(d,fmt=plistlib.FMT_BINARY))
shutil.copy2(root/'compiled/LightShow.dylib',app/'Frameworks/LightShow.dylib')
exe=app/d['CFBundleExecutable']
ent=plistlib.loads(subprocess.check_output([str(root.parent/'ldid'),'-e',str(exe)]))
ent['application-identifier']='4657EYLGTH.'+new
ent.pop('aps-environment',None)
ent.pop('com.apple.security.application-groups',None)
ent['keychain-access-groups']=['4657EYLGTH.com.ledvance.smart']
ent['platform-application']=True
ent['com.apple.private.security.no-sandbox']=True
ent['com.apple.private.security.storage.AppDataContainers']=True
ent['com.apple.private.security.container-required']=new
e=root/'side_by_side_entitlements.plist';e.write_bytes(plistlib.dumps(ent))
for p in app.rglob('*'):
 if p.is_file() and p.read_bytes()[:4] in [bytes.fromhex('cffaedfe'),bytes.fromhex('cafebabe')]:p.chmod(0o755)
subprocess.run([str(root.parent/'ldid'),'-S'+str(e),str(exe)],check=True)
out=root/'Light_Show_v0.2_Playback_iOS15.ipa'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(dst.rglob('*')):
  if p.is_file():z.write(p,p.relative_to(dst))
with zipfile.ZipFile(out) as z:
 assert z.testzip() is None
 assert plistlib.loads(z.read('Payload/SmartHome Prd.app/Info.plist'))['CFBundleIdentifier']==new
 assert 'NSMicrophoneUsageDescription' not in plistlib.loads(z.read('Payload/SmartHome Prd.app/Info.plist'))
signed=plistlib.loads(subprocess.check_output([str(root.parent/'ldid'),'-e',str(exe)]))
assert signed['application-identifier'].endswith('.'+new)
# Verify every CodeDirectory page hash in the newly signed executable.
b=exe.read_bytes();o=32;checked=0
for _ in range(struct.unpack_from('<I',b,16)[0]):
 c,s=struct.unpack_from('<II',b,o)
 if c==29:
  start,length=struct.unpack_from('<II',b,o+8)
  for j in range(struct.unpack_from('>I',b,start+8)[0]):
   slot,rel=struct.unpack_from('>II',b,start+12+j*8);q=start+rel
   if struct.unpack_from('>I',b,q)[0]!=0xfade0c02:continue
   ho=struct.unpack_from('>I',b,q+16)[0];ns,limit=struct.unpack_from('>II',b,q+28);hs,ht,_,ps=struct.unpack_from('BBBB',b,q+36)
   alg={1:hashlib.sha1,2:hashlib.sha256,3:hashlib.sha256,4:hashlib.sha384}[ht]
   for k in range(ns):assert alg(b[k*(1<<ps):min((k+1)*(1<<ps),limit)]).digest()[:hs]==b[q+ho+k*hs:q+ho+(k+1)*hs]
   checked+=1
 o+=s
assert checked
print(json.dumps({'file':str(out),'old_id':old,'new_id':new,'signature_verified':True,'bytes':out.stat().st_size}))
