# SYLVANIA Plus 3.6 / Colour Play

An update to the user's modified SYLVANIA Smart Home 3.0.2 app. Original controls, paired bulbs, scenes and the separate original app remain.

## Light Studio

32 generative moods. Aurora remains first. New modes are Emerald Sky, Fjord, Cherry Blossom, Solstice, Nebula, Sea Glass, Candle Garden and Kaleidoscope, alongside the existing natural palettes. Soft gradient cards preview each palette. Two overlapping 2D fields keep adjacent bulbs related while colour slowly moves through the apartment.

Playback no longer waits for an HSL Get reply before writing. That mandatory read gate in 3.4 could leave every effect inert when bulbs failed to answer; the former identification pulse shared the same dependency. Studio now starts from the SDK's last known raw HSL values or the last Studio command. An empty cache uses the target and an initial native four-second fade. The cache is not a fresh physical measurement. Original command fields stay degrees/percent; SDK node cache fields are normalized 16-bit values. Timed-out capture/confirmation reads cancel only their own message handle, preventing a stale request from holding the command queue.

Explicit mode choices and Brightness/Colour depth adjustments now send the latest target on the next visit to each bulb, with a four-second native fade. They no longer pass through the very slow ambient limiter: an Aurora-to-Embers choice previously needed approximately 235 seconds to get within two hue degrees. For fourteen bulbs, each destination is visited roughly once per 1.4 seconds before radio delays. Repeated input coalesces to the newest settings instead of building a queue of obsolete slider values. The ambient animation between user changes retains its exponential filter and strict limits: 1 degree hue, 1.2 percentage points saturation and 0.8 points brightness per second. Physical interpolation for deliberate choices relies on the advertised native transition field.

Colour depth now spans a pale quarter of each palette's native saturation at the low end to full saturation at the top. Midrange settings are substantially richer than the old multiplier. Hue and brightness are unaffected by this control. Aurora restores the first documented Light Studio colour scheme from commit 2860c3118166d0623f741b9021564a0f514e32b7: jade, blue and violet, using the original hue and base-saturation equations. Its current slow timing and low brightness contrast remain. Emerald Sky provides a separate deeper-green variation.

Motion now has a displayed 0.2x to 10.0x travel-speed range. A cubic curve keeps the lower settings gentle. Pause/Resume preserves phase and journey progress. Brightness and Colour depth also adjust an already paused look with a single bounded-rate sender pass, then leave it paused; Motion prepares its speed for Resume. Capture waits for the most recent native fade. Reconnect clears stale local targets and resumes from the SDK's current cache. Slider values are not overwritten by the UI refresh timer, horizontal slider drags stay with the slider, while vertical drags on cards, captions and empty content scroll the page, the active mode has a visible outline, and mood cards use a plain touch control with a gradient backing layer and a separate caption, avoiding button presentation updates that could cover their text after scrolling. Studio controls are excluded from the original app's legacy recolouring hooks.

Six repeatable journeys unfold through related palettes: Northern skies, Coastal drift, Warm evening, Forest after rain, Velvet night and Golden hour. Each chapter lasts six active minutes: four to settle and two to dissolve into the next. Journeys last 30 or 36 minutes per cycle, then loop continuously. Forest after rain explores green/jade/water, Velvet night moves through blossom/violet/indigo, and Golden hour stays in amber, gold and candlelight.

Shuffle colours sits directly above the mood/journey grid. Prism and Kaleidoscope receive a fresh colour family; other modes can recolour, rotate or mirror their flow. Candidates are checked for a meaningful hue change. The selected mode remains active and each bulb fades toward its new target. On a paused look, shuffle preserves brightness and saturation and leaves it paused. More > Restore original colours returns the stock palette and orientation. My moods stores named recipes including the shuffle seed, phase, room, brightness, motion and colour depth; recipes travel in Home exports.

Studio uses a mapped preview of last commanded colours. The preview is approximate, not a live measurement. It redraws at five frames per second while visible and opens the apartment map on tap.

Pause, then Save as scene, reads selected bulbs after their fades settle. No partial scene is written if a bulb fails to respond. New scenes use the original Realm schema: independent entry UUIDs, device names, HSL deviceType raw value 8, hue degrees, percentage saturation/lightness and cct=-1. Native scene controls can recall them. Saving also works on an already held look without starting animation. The ten-scene cap stays removed.

## Apartment map and controls

Map light travel path provides one grid for all colour bulbs. Drag numbered lights into their relative apartment positions. Saved normalized coordinates drive 2D effects and travel with exported Home files. Tap a pin or named lamp, or use Identify selected / Next light. The selected bulb is sent a bright contrasting amber/cyan colour marker for four seconds, then its last known HSL colour is restored. This uses the same HSL sender as working native colour control, rather than the old brief brightness-only pulse. Both writes are sent before any confirmation query. A missing confirmation is reported without holding the marker indefinitely. Restore values are last known app state, so an unknown change from another controller cannot be inferred. Studio pauses while mapping; return and Resume when finished. Selected/dragged pins come forward, and the named list provides access when pins overlap.

PLUS and STUDIO use matching translucent capsules with opaque white text. Drag each to move it. Per-device positions persist and remain inside safe screen/keyboard bounds. A non-key overlay accepts touches only on the capsules, keeping both present across original pages and modal tools.

Arrange scene shortcuts supports reordering and confirmed deletion by scene name. Entries referenced by another scene survive deletion; no bulb commands are sent. Default fade and power-on settings remain.

Persistent dark styling guards later black text and hamburger-image assignments, including attributed labels. The connection badge is dark with white text. The launch screen is now a plain dark background with no label or image. A new launch-resource name avoids reusing the earlier branded launch cache. The Sylvania logo and Network Analyse are hidden, together with retired smart-home experience and support/legal sections.

## Home files

Export Home and Import Home exchange one complete .sylhome: consistent native Realm/SQLite snapshots, pairings, groups, scenes, ordering, apartment map and saved moving moods. Imports preserve the receiving controller's UUID, allocation, profile and monotonic counter. Floating button positions stay local. Restoration runs on restart before native storage/mesh opens, with a journal covering interruption. Legacy .sylbackup import remains supported. Home files contain pairing keys.

## Hardware and compatibility

This vendor SDK uses hue degrees and percentage saturation/lightness despite its UInt16 fields. verify_native_color.py executes the exact original ARM64 conversion and scene-enum methods under Unicorn: blue reaches the SDK as 240/80/60; native HSL scenes use type 8. Normalized mesh units caused the static-red failure fixed in 3.3.

The add-on targets iOS 14 with compiler availability checks. The supplied executable and bundled frameworks have iOS 13-or-earlier deployment targets. Packaging audits them and sets the app minimum to 14. Install with the existing TrollStore route over the same numbered Plus app, without uninstalling. Each phone needs a different numbered controller; private-profiles.plist must persist across rebuilds.

Physical bulbs and an actual iOS 14 runtime are unavailable in this build environment. Simulators do not establish radio performance or perceived smoothness. Moods are generative ambient lighting, not beat analysis or audio/video capture. No microphone or firmware flashing is involved. Keep Plus active for reliable continuous effects and run Studio on one controller at a time.

## Design research

These are original palette formulas. Research informed the design, not copied scene data:

- [Philips Hue dynamic scenes](https://www.philips-hue.com/en-us/explore-hue/blog/things-to-do-with-hue-color-lights): coordinated natural palettes and unobtrusive movement informed the low-contrast gallery.
- [NASA auroras](https://science.nasa.gov/sun/auroras/) and [NOAA aurora tutorial](https://www.spaceweather.gov/content/aurora-tutorial): layered colour and curtain forms inspired broad overlapping fields.
- [Guggenheim Turrell interview](https://www.guggenheim.org/wp-content/uploads/2021/08/Aten_Reign_James_Turrell_Skyspaces_transcript.pdf): relationships between light, space and the viewer informed spatial continuity.
- [Nanoleaf Scenes](https://support.nanoleaf.me/hc/en-us/articles/36067632456084-FAQ-Feature-Scenes): separating a reusable moving palette from a static saved look informed My moods.
- [Apple scroll cancellation policy](https://developer.apple.com/documentation/uikit/uiscrollview/touchesshouldcancel(in:)): the gallery explicitly permits vertical pans to cancel card presses while preserving slider tracking.
- [Bluetooth Mesh Model specification](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/MMDL_v1.1/out/en/index-en.html): native transition semantics. The supplied vendor binary remains authoritative for numeric units.

## Build and validation

GitHub receives only source, synthetic tests and documentation. Binaries, real databases, pairing keys and controller profiles remain local. make_homeplus.py verifies the supplied executable, patches the scene-limit branch, injects the add-on/dark launch screen, signs private TrollStore packages and verifies deployment targets/CodeDirectories.

The workflow tests Home identity/counters/restoration, moving recipe validation, 32-palette and six-journey continuity, reproducible variations, colour depth, bounded steps, spatial persistence, real Realm 10.11 scene writes/deletion with shared-child preservation, and UIKit regressions with fourteen synthetic bulbs, including unanswered queries, timeout cancellation, all 32 actual mode cards while already running, fast per-bulb retargeting, all slider callbacks, brightness/depth adjustments while paused, 10x motion, card/slider cancellation policy, palette shuffling, held-level preservation, blank launch loading, slider hit testing and refresh stability, reconnect cache refresh, preserved pause/resume phase, moving mood persistence/deletion, and both map identification controls. Internal Studio/map/dark-theme screenshots support visual review. Rendered card text is checked by counting white pixels inside card snapshots after a complete gallery scroll and repeated selections. Test doubles are excluded from IPAs.
