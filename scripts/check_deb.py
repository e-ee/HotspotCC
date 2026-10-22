#!/usr/bin/env python3
"""Verify .deb content without dpkg (unavailable on GitHub macOS runners)."""
import io
import pathlib
import plistlib
import subprocess
import sys
import tarfile

package = pathlib.Path(sys.argv[1])
sections = subprocess.check_output(['ar', 't', str(package)], text=True).splitlines()
name = next((n for n in sections if n.startswith('data.tar.')), None)
if name is None:
    raise SystemExit(f'No data.tar payload: {sections}')
raw = subprocess.check_output(['ar', 'p', str(package), name])
with tarfile.open(fileobj=io.BytesIO(raw), mode='r:*') as t:
    entries = {m.name.lstrip('./'): m for m in t.getmembers()}
    prefix = 'var/jb/Library/ControlCenter/CCSupport_Providers/HotspotCCProvider.bundle/'
    for filename in ('HotspotCCProvider', 'Info.plist'):
        path = prefix + filename
        if path not in entries or entries[path].size <= 0:
            raise SystemExit('Missing or empty: ' + path)
        contents = t.extractfile(entries[path]).read()
        if filename == 'Info.plist':
            assert plistlib.loads(contents)['NSPrincipalClass'] == 'HotspotCCProvider'
        print(filename, len(contents), 'bytes')
for name in entries:
    if name.startswith(prefix) and name.endswith('.png'):
        raise SystemExit('Unexpected custom icon file: ' + name)
print('Package verified:', package)
