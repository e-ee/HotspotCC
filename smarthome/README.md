# Light Show v0.4: Room Flow

TrollStore IPA for the supplied SYLVANIA Smart Home 3.0.2 host, iOS 15. No microphone, no pairing/reset commands, separate install identity `com.ledvance.smart.lightshow`.

## Existing lights

Read the original on-device preferences file and `mesh.json`, restore the existing controller UUID through the stock Telink SDK, select app-key index 0, initialize SigMeshLib and explicitly start SigBearer's mesh connection. The uploaded data verified 14 bulbs in 5 groups. Network keys and user state are not embedded in source or published to GitHub. Original data is shared in place; close the original app before running this app so controller sequence state has only one writer.

## Spatial effects

Living Room's four fill bulbs are updated individually. Saved names containing TV and Corner are provisional spatial hints for a dimmer TV anchor and a wider corner wash. The photographs establish broad wall/ceiling washes and TV focus; they do not prove which physical fixture each saved node controls. No definitive TV-white or behind-TV mapping is claimed. The other 8 bulbs remain in their saved support groups. The Kitchen and single-bulb new group share an effect phase, without changing saved membership.

Use a bounded 8-slot schedule at 0.16 seconds per slot: fill, corner, fill, support, TV, corner, fill, fill. Avoid overlapping Living Room group broadcasts that would overwrite the TV/corner settings. Shared palette, slow offset waves, gradual artwork-hue changes, conservative TV brightness, no random black/white strobing. Decorative on-screen waves represent the generated light scene, not an audio spectrum.

## Now Playing

MediaRemote supplies title, artwork, play/pause state and playback position across sources where available. Album artwork drives colour; clock-driven curves animate the room. If the player publishes BPM metadata it can influence the pulse. Without BPM, use slow continuous movement; there is no invented 120 BPM default. This is playback-driven ambient lighting, not waveform analysis, measured beat sync, or movie screen-colour matching. There is no microphone fallback or permission.

## Interface

Full-screen dark native interface, real artwork, current title, saved group names, connection status, one Start/Pause button. Palette and brightness are optional. Starts automatically when Now Playing is active and the mesh is connected. Keep the app foregrounded while your player supplies playback in the background. Broad cross-app background execution is not implemented.

## Validation

Native ARM64 module compiled on GitHub macOS with the iOS SDK. Local checks verify all 14 saved bulbs are covered, TV/corner endpoints are excluded from fill updates, no user network keys appear in source, identifiers differ from the original app, microphone permission/capture symbols are absent, and executable signature page hashes match. No physical-device installation, original-state access, or Bluetooth tests are available. Compilation and package checks do not guarantee runtime success. Earlier v0.2 did not connect; v0.3+ adds the explicit saved-mesh initialization and connection steps.

Original host binaries stay private. fishhook dependency license is included. HotspotCC main is unaffected.
