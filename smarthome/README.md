# SYLVANIA Light Show

Experimental microphone-driven visualizer module for the supplied SYLVANIA Smart Home 3.0.2 build. Bass, midrange and treble determine colour and brightness. A floating panel starts/stops analysis and adjusts sensitivity. The existing app owns pairing and mesh credentials. No user app binaries or mesh credentials are hosted in this repository.

Build: `.github/workflows/smarthome.yml` compiles an ARM64 dylib using the iOS SDK on macOS. The module itself targets iOS 12, but the supplied host application requires iOS 15. The packaged IPA therefore remains iOS 15 only. Older host-app compatibility is not established.

The Telink path sends unacknowledged broadcast HSL commands to the active mesh, subject to app-key bindings. WICED uses available root groups and targets observed from existing UI commands; if none are found, select the all-lights group in the app and change colour before starting. Brightness-only devices are supported by the Telink path when no HSL targets exist. Mixed RGB and white-only networks are not fully covered by the initial broadcast policy.

Audio comes from the microphone. This is not direct capture of another app's Now Playing audio. Keep the host app foregrounded. Interruption/background stops the visualizer. Lights remain at their last colour on stop.

The compilation workflow passed. No physical-device, Bluetooth, pairing, or installation tests were available. Cross-device credential sharing is not added by this module. Use the host's pairing/import flow. This branch is independent of HotspotCC main.
