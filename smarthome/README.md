# Beat Room v0.6

Actual system-audio feature client for a jailbroken iOS 15 phone. Reuses the private paired mesh state from v0.5. No microphone, saved-group UI, generated-beat fallback, stock startup screens or news popup.

## Audio path

Requires upstream AudioSnapshotServer 2 version 2.1 (rootless package on Dopamine). It runs in mediaserverd and publishes reduced system-mix features over UDP 127.0.0.1:44333. The app implements the canonical MSH2 protocol, session/server/epoch checks, sequence rejection, bounded nonblocking reads, 4-second subscription refresh and stale-data detection. Raw PCM never leaves the helper; no audio is saved or uploaded.

Sources: https://github.com/ryannair05/AudioSnapshotServer2 and https://github.com/ryannair05/libmitsuha6 . Protocol license is included. The original unmodified helper package is distributed only in the private install bundle; no tweak source is changed.

## Analysis and lights

Use 64 spectrum bands and RMS to derive low/mid/high energy, adaptive normalization and positive-flux onset detection with a 180 ms refractory period. Steady audio does not fabricate repeated beats. Silence settles the brightness. New onsets advance the colour palette. Three optional styles: Pulse, Spectrum and Prism. All 14 lights receive the same unacknowledged broadcast HSL frame through the established Telink path, gated by transport readiness and the SDK queue. No spatial grouping is applied.

Now Playing supplies title/artwork only; it does not pretend to supply waveform data. The audio feature stream drives the effect. If the helper is unavailable or audio is stale, the UI reports this rather than continuing clock effects. A guarded private process assertion with TrollStore entitlements attempts to preserve processing across player switches; it is released on Pause. Background behavior still requires device testing.

## Installation

1. Install the rootless AudioSnapshotServer 2 DEB through Sileo. Existing older libmitsuha6 installations may need updating because upstream declares a package conflict below version 2.0.
2. Perform a userspace reboot once so mediaserverd loads the audio helper, keeping the jailbreak active.
3. Install the supplied private IPA over Light Show in TrollStore. Close the original SYLVANIA app to release its Bluetooth connection.
4. Play music and open Beat Room. It connects the existing bulbs and subscribes to audio automatically. No re-pairing or microphone permission is used.

The paired IPA contains private network credentials and one extra controller identity at 7F00. Do not run this same seeded controller on multiple phones concurrently. Existing controller state and sequence counters persist across updates.

## Validation

The working light connection is confirmed by the user's v0.5 report. New v0.6 analysis and transport remain untested on the phone. Host tests detect exactly 20 synthetic kicks, do not retrigger on a steady tone, settle during silence and handle invalid numeric data. Upstream protocol, feature pacing, active-token and audio-activity-gate tests pass locally. GitHub macOS runs the beat test and compiles ARM64. The helper DEB matches its upstream git blob and contains the MSH2 wire magic. These checks do not verify physical capture routes, helper stability or actual bulb latency. Never claim universal source/route compatibility before device verification.
