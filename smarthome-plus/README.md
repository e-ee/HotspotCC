# SYLVANIA Plus 3.4 / Slow Light

An update to the user's working modified SYLVANIA Smart Home 3.0.2 app. Original controls, paired bulbs, scenes and the separate original app remain.

## Light Studio

24 generative moods. Aurora remains first, followed by new calm natural palettes: Forest Mist, Lagoon, Heather, Opal, Blue Hour, Coast, Rose Dusk and Polar Mist. Soft gradient cards preview each palette. Two overlapping 2D fields keep adjacent bulbs related while colour slowly moves through the apartment.

Every bulb's current HSL is read before animation. Mode changes preserve the current drift state. A shared exponential filter and strict rate limits (1 degree hue, 1.2 percentage points saturation and 0.8 points brightness per second) constrain changes. Elapsed time is capped after congestion/suspension. Four-second native transitions overlap the round-robin sender for fourteen bulbs. Palettes have low brightness contrast. Motion adjusts travel speed without relaxing these limits.

Pause, then Save as scene, reads selected bulbs after their fades settle. No partial scene is written if a bulb fails to respond. New scenes use the original Realm schema: independent entry UUIDs, device names, HSL deviceType raw value 8, hue degrees, percentage saturation/lightness and cct=-1. Native scene controls can recall them. Saving also works on an already held look without starting animation. The ten-scene cap stays removed.

## Apartment map and controls

Map light travel path provides one grid for all colour bulbs. Drag numbered lights into their relative apartment positions. Saved normalized coordinates drive 2D effects and travel with exported Home files. Tap a pin for one gentle physical brightness pulse and restoration of its read level. Colour/CCT is not rewritten. Studio remains paused while mapping and identifying.

PLUS and STUDIO use matching translucent capsules with opaque white text. Drag each to move it. Per-device positions persist and remain inside safe screen/keyboard bounds. A non-key overlay accepts touches only on the capsules, keeping both present across original pages and modal tools.

Arrange scene shortcuts supports reordering and confirmed deletion by scene name. Entries referenced by another scene survive deletion; no bulb commands are sent. Default fade and power-on settings remain.

Persistent dark styling guards later black text and hamburger-image assignments, including attributed labels. The connection badge is dark with white text. The Sylvania logo and Network Analyse are hidden, together with retired smart-home experience and support/legal sections.

## Home files

Export Home and Import Home exchange one complete .sylhome: consistent native Realm/SQLite snapshots, pairings, groups, scenes, ordering and apartment map. Imports preserve the receiving controller's UUID, allocation, profile and monotonic counter. Floating button positions stay local. Restoration runs on restart before native storage/mesh opens, with a journal covering interruption. Legacy .sylbackup import remains supported. Home files contain pairing keys.

## Hardware and compatibility

This vendor SDK uses hue degrees and percentage saturation/lightness despite its UInt16 fields. verify_native_color.py executes the exact original ARM64 conversion and scene-enum methods under Unicorn: blue reaches the SDK as 240/80/60; native HSL scenes use type 8. Normalized mesh units caused the static-red failure fixed in 3.3.

The add-on targets iOS 14 with compiler availability checks. The supplied executable and bundled frameworks have iOS 13-or-earlier deployment targets. Packaging audits them and sets the app minimum to 14. Install with the existing TrollStore route over the same numbered Plus app, without uninstalling. Each phone needs a different numbered controller; private-profiles.plist must persist across rebuilds.

Physical bulbs and an actual iOS 14 runtime are unavailable in this build environment. Simulators do not establish radio performance or perceived smoothness. Moods are generative ambient lighting, not beat analysis or audio/video capture. No microphone or firmware flashing is involved. Keep Plus active for reliable continuous effects and run Studio on one controller at a time.

## Design research

These are original palette formulas. Research informed the design, not copied scene data:

- [Philips Hue dynamic scenes](https://www.philips-hue.com/en-us/explore-hue/blog/things-to-do-with-hue-color-lights): coordinated natural palettes and unobtrusive movement informed the low-contrast gallery.
- [NASA auroras](https://science.nasa.gov/sun/auroras/) and [NOAA aurora tutorial](https://www.spaceweather.gov/content/aurora-tutorial): layered colour and curtain forms inspired broad overlapping fields.
- [Guggenheim Turrell interview](https://www.guggenheim.org/wp-content/uploads/2021/08/Aten_Reign_James_Turrell_Skyspaces_transcript.pdf): relationships between light, space and the viewer informed spatial continuity.
- [Bluetooth Mesh Model specification](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/MMDL_v1.1/out/en/index-en.html): native transition semantics. The supplied vendor binary remains authoritative for numeric units.

## Build and validation

GitHub receives only source, synthetic tests and documentation. Binaries, real databases, pairing keys and controller profiles remain local. make_homeplus.py verifies the supplied executable, patches the scene-limit branch, injects the add-on/dark launch screen, signs private TrollStore packages and verifies deployment targets/CodeDirectories.

The workflow tests Home identity/counters/restoration, 24-palette continuity and bounded steps, spatial persistence, real Realm 10.11 scene writes/deletion with shared-child preservation, and UIKit regressions with fourteen synthetic bulbs. Internal Studio/map/dark-theme screenshots support visual review. Test doubles are excluded from IPAs.
