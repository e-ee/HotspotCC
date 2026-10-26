# Light Show v0.5: Private Paired Build

Private TrollStore IPA for the supplied SYLVANIA 3.0.2 host on iOS 15. The user reported that v0.4 could not access the original app's data container; the stock account tutorial also flashed before the delayed overlay. This version removes both dependencies.

## Existing network, independent controller

The local packaging step embeds a private seed derived from the user's supplied mesh export. The seed is never committed or uploaded to GitHub. It retains the existing network/app keys, 14 bulbs and 5 groups, but adds a distinct controller UUID at unicast address 7F00, outside the original provisioner's 0001–0100 allocation. Its own sequence counter persists in a separate preferences suite. Remote device keys are replaced with placeholders; configuration/provisioning operations are not implemented. No reset or re-pairing is performed.

This personalized build assigns one additional controller identity for the user's current phone. Do not run copies of this same seeded controller on multiple phones concurrently; allocate a distinct controller/address for each. Original SYLVANIA remains separately installed and its data is neither read nor modified. Close the original app while testing to release its Bluetooth connection.

The native module loads its own persisted mesh seed through SigDataSource, selects app-key 0, initializes SigMeshLib and explicitly starts SigBearer. It uses standard Bluetooth permissions. The original cross-container/private-storage entitlements are removed. The installer container stays `com.ledvance.smart.lightshow`.

## Clean startup

Remove stock main and launch storyboards. Replace the stock app delegate's launch/lifecycle path with the Light Show interface and lighting engine. The empty-account tutorial and APP NEWS fetch/presentation path no longer run. Actual Bluetooth/system permissions are retained.

## Responsive effects

20 Hz latest-frame scheduler, conditional on the SDK queue and CoreBluetooth write-without-response readiness. Frames are generated at send time; obsolete frames are not accumulated. Whole-room broadcast updates take priority; spatial accents are secondary. This is a configured update cadence, not measured physical bulb latency.

Now Playing supplies track information, artwork and play/pause state. Generated lighting effects follow playback; they do not measure soundtrack beats or movie screen colours. Microphone capture is absent. Experimental ReplayKit work is not shipped in this build. Keep Light Show foregrounded while the player supplies background playback.

## Validation

GitHub macOS compiled the ARM64 module. Local checks verify: a separate local controller/address and sequence domain; all existing bulbs/groups retained; readable bundled seed; no uploaded keys in public source; separate install identity; no stock storyboard or microphone permission; and valid executable signature page hashes. Physical-device installation and Bluetooth behavior remain untested in this environment. Prior container-access failure is addressed architecturally, but runtime success is not yet established.

Host binaries and private mesh seed remain outside GitHub. fishhook license is retained for earlier source history. Main HotspotCC is unaffected.
