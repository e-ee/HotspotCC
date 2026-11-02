# SYLVANIA Plus

Local improvements to the user's own SYLVANIA Smart Home 3.0.2 installation:

- Remove the verified ten-routine gate while retaining the original scene UI.
- Seed a separate app container from the user's current backup.
- Prepare three independent mesh controller identities for additional devices.
- Export/import routines using a key-free, network-specific JSON format.
- Export a recovery archive containing the private local home state.

No app binaries, Realm databases, pairing keys, controller UUIDs or private seeds
are committed here. The local packager reads the user's own `smarthome.zip`,
including `Documents/decrypted-app.ipa` and the current preferences.

Build `HomePlus.dylib` with `.github/workflows/homeplus.yml`. Tests validate the
actual Foundation scene transfer code at counts around and well above ten.
`make_homeplus.py` validates the exact input executable, changes the one branch,
adds the module, assigns separate controller allocations, signs the app for
TrollStore and checks the resulting IPA and CodeDirectory hashes.

```sh
python3 make_homeplus.py --config /private/path/smarthome.zip \
  --module /path/HomePlus.dylib --ldid /path/ldid
```

Keep the generated `private-profiles.plist` private and preserve it across
rebuilds. Do not distribute the same numbered controller profile to multiple
physical devices. Install updates over the corresponding Plus installation;
resetting its sequence counter can make existing bulbs reject its messages.

The native app remains iOS 15.0 minimum. The new package uses the separate bundle
ID `com.ledvance.smart.plus` so the original remains installed. Runtime calls use
selectors and ABI signatures checked against the original binary and Realm
10.11.0 sources. The Objective-C module is not a claim of on-device testing:
startup, native database import/export and radio behaviour need hardware checks.

See the install package's `INSTALL.txt` and `FEATURES_AND_FINDINGS.md` for the
current deliverable, limitations and grounded feature roadmap.
