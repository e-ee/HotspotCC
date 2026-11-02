"""Audit the universal IPA while preserving the numbered-install upgrade seed."""
from pathlib import Path
import hashlib,json,plistlib,tempfile,zipfile
import make_homeplus as build
from verify_release import sections
root=Path(__file__).resolve().parent
baseline=json.loads((root/'baseline-v3.6.json').read_text())
assert hashlib.sha256((root/'private-profiles.plist').read_bytes()).hexdigest()==baseline['profiles_sha256']
prefix='Payload/SmartHome Prd.app/'
path=root/'release/SYLVANIA_Plus_Universal_iOS14plus.ipa'
with zipfile.ZipFile(root/'release/SYLVANIA_Plus_Device_1_iOS14plus.ipa') as old:
 prior=plistlib.loads(old.read(prefix+'HomePlusSeed.plist'))
with zipfile.ZipFile(path) as z:
 assert z.testzip() is None
 info=plistlib.loads(z.read(prefix+'Info.plist'));seed=plistlib.loads(z.read(prefix+'HomePlusSeed.plist'))
 assert info['CFBundleIdentifier']==build.BUNDLE_ID and info['CFBundleVersion']=='3700' and info['CFBundleShortVersionString']=='3.7.0' and info['MinimumOSVersion']=='14.0'
 assert seed['Universal'] is True and seed['AuthorityUUID']==prior['Preferences']['currenProvisionerUUID_key']
 prefs=dict(seed['Preferences']);assert prefs.pop('HP.EnrollmentAuthority')==seed['AuthorityUUID'];assert prefs==prior['Preferences'] and seed['Documents']==prior['Documents']
 module=z.read(prefix+'Frameworks/HomePlus.dylib');assert sections(module)==sections((root/'build-v3.7/HomePlus.dylib').read_bytes())
 assert b'HPMockReadHandle' not in module and b'mock-lamp-' not in module
 assert b'Let the room exhale.' not in module and b'Give the light a place.' not in module
 for key in ['UILaunchStoryboardName','UILaunchStoryboardName~ipad']:assert info[key]=='HomePlusBlankLaunch360'
 with tempfile.TemporaryDirectory() as d:
  z.extractall(d);app=Path(d)/prefix
  signatures=sum(build.verify_signatures(p) for p in app.rglob('*') if p.is_file());deployment=build.verify_ios14_deployment(app);assert signatures==20 and deployment==18
result={'version':'3.7.0','sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'private_profiles_unchanged':True,'legacy_upgrade_seed_documents_and_preferences_preserved':True,'authority_matches_existing_device_1':True,'compiled_module_sections_match_ci':True,'verified_code_directories':signatures,'ios14_eligible_machos':deployment,'test_endpoints_excluded':True}
(root/'universal-package-audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
