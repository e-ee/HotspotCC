"""Audit privately packaged controllers against a retained seed baseline and CI module."""
import argparse, hashlib, json, plistlib, struct, tempfile, zipfile
from pathlib import Path
import make_homeplus as build

def sections(data):
    result = {}; offset = 32
    for _ in range(struct.unpack_from('<I', data, 16)[0]):
        command, size = struct.unpack_from('<II', data, offset)
        if command == 0x19:
            for index in range(struct.unpack_from('<I', data, offset + 64)[0]):
                at = offset + 72 + index * 80
                name = data[at:at+16].split(b'\0')[0].decode()
                segment = data[at+16:at+32].split(b'\0')[0].decode()
                length = struct.unpack_from('<Q', data, at+40)[0]
                start = struct.unpack_from('<I', data, at+48)[0]
                flags = struct.unpack_from('<I', data, at+64)[0]
                if flags & 0xff not in (1, 12, 18):
                    result[segment+','+name] = data[start:start+length]
        offset += size
    return result

def verify(root, baseline, module_dir, version, build_number, launch_name):
    baseline = json.loads(baseline.read_text())
    assert baseline['profiles_sha256'] == hashlib.sha256((root/'private-profiles.plist').read_bytes()).hexdigest()
    compiled = sections((module_dir/'HomePlus.dylib').read_bytes())
    assert '__TEXT,__text' in compiled
    rows = []
    for number in range(1, 4):
        path = root/'release'/f'SYLVANIA_Plus_Device_{number}_iOS14plus.ipa'
        with zipfile.ZipFile(path) as archive:
            assert archive.testzip() is None
            prefix = 'Payload/SmartHome Prd.app/'
            info = plistlib.loads(archive.read(prefix+'Info.plist'))
            assert info['CFBundleVersion'] == build_number and info['CFBundleShortVersionString'] == version
            assert info['CFBundleIdentifier'] == build.BUNDLE_ID and info['MinimumOSVersion'] == '14.0'
            assert info['UILaunchStoryboardName'] == launch_name and info['UILaunchStoryboardName~ipad'] == launch_name
            seed = archive.read(prefix+'HomePlusSeed.plist')
            assert hashlib.sha256(seed).hexdigest() == baseline['previous_ipa_seeds'][path.name]
            module = archive.read(prefix+'Frameworks/HomePlus.dylib')
            assert sections(module) == compiled
            assert b'mock-lamp-' not in module and b'HPMockReadHandle' not in module
            for file in (module_dir/'HomePlusLaunch.storyboardc').iterdir():
                assert archive.read(prefix+launch_name+'.storyboardc/'+file.name) == file.read_bytes()
            with tempfile.TemporaryDirectory(prefix='homeplus-check-') as temporary:
                archive.extractall(temporary)
                app = Path(temporary)/prefix
                count = sum(build.verify_signatures(p) for p in app.rglob('*') if p.is_file())
                eligible = build.verify_ios14_deployment(app)
                assert count == 20 and eligible == 18
            rows.append({'device':number, 'version':version, 'seed_byte_identical_to_previous_release':True,
                         'verified_code_directories':count, 'ios14_eligible_machos':eligible,
                         'minimum_ios':'14.0', 'bundle_identity_retained':True,
                         'production_module_sections_match_ci_build':True, 'test_doubles_excluded':True,
                         'blank_launch_resource_matches_ci':True,
                         'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    return {'private_profiles_unchanged':True, 'packages':rows}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--module-dir', type=Path, required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--build-number', required=True)
    parser.add_argument('--launch-name', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = verify(Path(__file__).resolve().parent, args.baseline, args.module_dir,
                    args.version, args.build_number, args.launch_name)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
