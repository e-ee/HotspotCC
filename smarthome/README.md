# Light Show v0.2

Experimental playback-only lighting module for the supplied SYLVANIA Smart Home 3.0.2 build. No microphone code or permission is included.

The separate app identifier `com.ledvance.smart.lightshow` avoids replacing the original `com.ledvance.smart` installation. The IPA is intended for TrollStore on iOS 15; ordinary signing does not grant the required private entitlements. Older iOS versions are not established because the supplied host requires iOS 15.

## Playback behavior

MediaRemote supplies system Now Playing metadata and play/pause state. Album artwork determines colour. Playback position drives a pulse. BPM metadata is used when available; otherwise the animation uses an explicitly unmeasured 120 BPM pulse. This is not audio waveform capture, frequency analysis, or verified beat synchronization. App-to-app audio capture is not implemented.

The foreground app automatically attempts to start when playback is active and saved lights are available. The compact control expands for pause and palette options. Keep it foregrounded while another player supplies music.

## Existing bulbs

The module uses TrollStore entitlements to locate only the original SYLVANIA data container by its metadata identifier. It redirects the host executable's Foundation home/search paths, NSFileManager directory URLs, and standard defaults to that existing configuration. It does not copy the controller identity or reset/provision bulbs. Its own installer container identity remains distinct, to avoid tying uninstall to the original app's container.

Close the original app before launching Light Show. Both apps must not independently write mesh state or sequence numbers concurrently. Shared original state can be updated by the stock mesh SDK while controlling lights. This is not a read-only bridge. No pairing credentials are uploaded or published here.

The Telink path uses broadcast lighting commands within the current mesh/app-key binding. WICED attempts to enumerate groups and can also observe host UI targets. Not all bulb/backend combinations are verified. If the saved network cannot be loaded, the module waits rather than requesting re-pairing.

## Validation and limitations

GitHub macOS builds compile the native ARM64 module. Local packaging checks cover identifiers, absent microphone permission, library embedding, and executable code-signature page hashes. No physical-device installation, original-state routing, or Bluetooth tests are available in this environment. Successful package validation is not a guarantee of runtime success.

Source dependencies: fishhook from facebook/fishhook, with its license in this directory. The supplied host app is never uploaded to this repository. The main HotspotCC branch is unaffected.
