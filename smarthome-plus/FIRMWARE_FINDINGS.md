# Firmware and lighting capabilities

The supplied network describes 14 colour bulbs with company ID `052E`, product
ID `0001` and version field `3233`. All 14 expose the same relevant standard
models, bound to application key index 0:

| Model | Function used in this update |
| --- | --- |
| `1307` Light HSL Server | Individual colour and brightness with timed transitions |
| `1303` Light CTL Server | Existing warm/cool white controls |
| `1004` Generic Default Transition Time Server | A default fade duration |
| `1006` / `1007` Generic Power OnOff Server / Setup Server | Behaviour after a power cycle |
| `0000052E` vendor model | Present, but no proprietary animation command was established |

The existing app already contains the extended HSL setter, including an explicit
transition and delay. Light Studio uses it to let each bulb interpolate locally
between colour targets. Commands are unacknowledged, paced at no more than one
per 100 milliseconds, and held while the SDK queue or BLE write buffer is busy.
With all 14 lamps selected, a complete update round takes roughly 1.4 seconds
before radio delays. The effect is a moving, blended apartment atmosphere, not
a claim of video-frame-rate output from every lamp.

The original SDK's short HSL convenience method explicitly sets a zero-duration
transition. The default fade setting applies to commands that omit an explicit
transition; native controls may supply their own timing. Light Studio supplies its
own transition duration. Version 3.5 retains bounded adjacent command changes and four-second fades for
14 bulbs, but removes 3.4's mandatory initial colour query. Some installations
can send working colour commands without answering that query. Effects now use
last known SDK/cache state rather than waiting indefinitely for a read reply.
Read timeouts explicitly cancel their own message handle. Cache values use raw
16-bit mesh units; outgoing vendor command values use degrees/percent.
The vendor's degree/percentage field units were verified in the supplied native
executable; upstream normalized mesh examples are not this app's contract.

## Manufacturer downloads

The firmware server remains reachable, independently of the discontinued cloud
control service. The app's US mesh catalog returned 104 product-name entries.
Some are aliases for the same image. Two representative A19 artifacts were
downloaded for local inspection:

| Catalog family | Version | Image bytes | Findings |
| --- | --- | ---: | --- |
| SYLVANIA A19 C | 2.0.05 | 139,628 | Clear `BRCMcfgD` header and BCM20739-family marker |
| SYLVANIA A19 T-C | 33.02.50 | 161,440 | Filename marks an AES artifact; sampled byte entropy is about 7.999 bits/byte |

SHA-256 hashes:

- A19 C: `36600de881c6fc5ea6bde5acf3ae737f0c4c3cf093e9c19d3e8e139c1b078c76`
- A19 T-C: `5add7d00aeb3c82a7ff2bf0e6f79cbef4812d54c467fbd47f90f6d77e0abeffa`

These findings establish that the catalog covers different firmware families.
The saved composition version field alone does not establish which downloadable
image is an exact match for each physical board. No firmware was flashed or
modified. No unverified vendor opcodes were sent. The new effects use existing,
advertised commands and preserve pairing.

## What this enables now

- 24 calm moods, including Aurora and eight new natural palette fields.
- An apartment grid with saved 2D lamp positions and tap-to-identify colour markers, explicit Identify/Next controls and HSL restoration.
- Room selection, brightness, gentle motion and colour-depth controls.
- Three slow palette journeys and saved animated mood recipes.
- Capture a paused look as a native scene using actual bulb readback.
- Normal scene/light controls pause the local effect generator.
- Ordinary fade timing and power-restore behaviour, through native bulb models.

These modes are generative lighting, without a microphone, screen capture or
measured audio beat tracking. There is no fake claim of Ambilight synchronisation.
The app requests a background assertion when available, but keeping Plus open
is the supported fallback for continuous effects in an unverified environment.

Further work with the most value: an authenticated LAN controller for a spare
always-on iOS device, automatic scene/network synchronisation that preserves
independent counters, and a separately verified playback/screen capture input.
Firmware replacement remains a separate project requiring exact board identity,
image format, authentication and recovery information.

Primary references:

- Manufacturer catalog: https://apps.ledvanceus.com/firmware/ble/pd/mesh.txt
- App configuration: https://apps.ledvanceus.com/firmware/ble/pd/smartHome_configs.txt
- Telink SIG Mesh source: https://github.com/telink-semi/tc_ble_mesh
- Bluetooth Mesh Model: https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/MMDL_v1.1/out/en/index-en.html

The per-bulb model coverage and setter ABIs were inspected in the user's supplied
network and executable. Image observations come from the downloaded manufacturer
artifacts, not from reading the bulbs' flash. Runtime radio latency and persistent
setting behaviour still require the user's physical lights.
