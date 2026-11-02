# SYLVANIA Plus 3.3 / After Dark

An improvement to the user's own SYLVANIA Smart Home 3.0.2 installation, retaining
the original controls, current paired bulbs and separate original app.

- Remove the verified ten-scene branch without replacing the scene editor.
- Install as `com.ledvance.smart.plus` with three distinct controller identities.
- Export and import one complete Home with scenes, groups, pairings and ordering.
- Keep the receiving controller's identity and advance its existing sequence
  counter; apply Home restoration on restart before the native databases open.
- Preserve Realm database bytes instead of rejecting legacy scene sentinels.
- Restore an interrupted Home import using a file and preferences journal.
- Run eight generative lighting moods, with configurable brightness, speed,
  room scope and lamp order, using the original vendor colour units.
- Apply a dark theme to legacy fixed colours as well as the new panels.
- Hide the retired smart-home experience and support/legal menu sections.
- Retain scene shortcut ordering, default fades and power-on settings.

## Colour fix

`DeviceControlApi.setHSL:color:sat:lightness:onCompletion:` calls the original
`OpMessage.changeHSLWithAddress:saturation:brightness:address:isGet:...` method.
At unslid address `0x1002e4070`, that method forwards the supplied floats as
UInt16 values **without normalizing to 65,535**. The input is hue in degrees,
saturation and brightness in percent. The generic upstream Telink example uses
a different contract and was an incorrect reference for this vendor app.

`verify_native_color.py` executes that actual native method locally under
Unicorn against the exact supplied executable. Blue 240/80/60 reaches the SDK as
240/80/60, as do independent green and magenta cases. `HPVendorColor` now emits
that contract. The native convenience HSL method is left unmodified.

## Home files

`HomeArchive` writes a binary `.sylhome` file, version 2, containing preferences,
a consistent Realm snapshot, the SQLite backup and the saved Network documents.
Document checksums detect damaged copies. The importer also accepts version 1
`.sylbackup` files from earlier Plus releases. Per-scene JSON sharing is removed
from the UI. `SceneTransfer` remains only for stable ordering and legacy tests.

Import validates the native Realm copy with the running app's configuration,
then stages a full Home. On restart, before the original app opens Realm or its
mesh connection, the receiver keeps its controller UUID, local node, allocation,
profile and a monotonic sequence counter. A restore journal covers interrupted
file replacement. No other app container is read or changed.

## Build and privacy

No app binaries, Realm databases, pairing keys, real controller UUIDs or private
seeds are committed. `make_homeplus.py` reads the user's own backup locally,
checks the exact original executable, applies the single scene-limit patch,
injects the add-on and dark launch screen, signs for TrollStore, and validates
archive contents and CodeDirectory hashes. Preserve `private-profiles.plist`
across rebuilds; never share the same numbered identity between phones.

Build the module and launch storyboard with `.github/workflows/homeplus.yml`.
Tests cover Home restoration, native unit conversion, pattern motion and bounds,
legacy ordering, fourteen simulated send targets, queue backpressure, manual
cancellation, hidden menu sections and dark legacy text. UIKit screenshots are
captured for visual inspection. Test doubles are not linked into the release.

The original app requires iOS 15.0. Updates install over the corresponding Plus
number, preserving the existing container and any scenes created since the seed
backup. The original SYLVANIA app stays installed. No firmware is flashed.

Physical BLE behaviour, lamp interpolation and background operation require
validation on the user's device. Studio's moods are generative, not measured
music/audio or screen synchronization.
