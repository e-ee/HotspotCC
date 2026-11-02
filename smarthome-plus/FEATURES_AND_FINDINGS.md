# SYLVANIA Plus: findings and next features

The current scope is the original lighting app, its ten-routine cap, and local
control from additional iOS devices. This is separate from the earlier Beat Room
experiment. It does not include playback capture or a music visualizer.

## What the supplied app actually does

The installed app is SYLVANIA Smart Home 3.0.2, bundle `com.ledvance.smart`.
Your current Documents backup includes the exact decrypted executable supplied
earlier, your mesh network, the Realm routine database and the saved groups.

The ten-routine restriction is in `RoutinesViewController`'s add action. Its
Swift implementation compares the current routine count with 10, then branches
to the localized `preset_allowed_count_limit` alert. The patch changes that
single conditional branch to a NOP. It leaves the scene editor, persistence,
existing scenes and recall implementation in place.

For this exact executable:

- Source SHA-256: `6c74a0bf6609ccf3ce5a132318eaf31f411743d8a78ce2b760ed15c813788eb2`
- Compare address: `0x1001A9194`, `cmp x21, #10`
- Rejection branch: `0x1001A9198`, bytes `EA 03 00 54`
- Replacement: `1F 20 03 D5`, ARM64 NOP
- Rejection target: `0x1001A9214`
- Ordinary editor path: `0x1001A919C`

This is a phone-side routine limit. All 14 bulb records advertise Generic OnOff,
Lightness, CTL and HSL models. None advertises Scene Server `0x1203` or Scheduler
Server `0x1206`. Their existing presets live in the phone database; this patch
does not attempt to increase firmware memory or add a bulb clock.

## Why additional devices need more than a cloned database

Mesh messages include the sender identity and a monotonically increasing
sequence number. Copying the original phone's identity onto another phone lets
their counters interfere. The three builds keep the same existing bulb network
and application keys, but each receives a separate provisioner UUID, local node,
sequence counter and non-overlapping address block. The original controller
and the older Light Show controller allocation are left alone.

| Install | Controller block |
| --- | --- |
| Original app | Existing allocation retained |
| Plus Device 1 | `7E00–7EFF` |
| Plus Device 2 | `7D00–7DFF` |
| Plus Device 3 | `7C00–7CFF` |

These are three independent local controllers, with manual scene sharing.
This is not an automatic multi-writer database synchronization system.

LEDVANCE's official notice confirms that cloud operation ended on December 1,
2023 and describes the replacement as a local app with data on one selected
phone. The current work addresses that local-data limitation; it does not
restore the discontinued cloud service.

## Features worth adding next

| Feature | Practical value | Implementation constraints |
| --- | --- | --- |
| Scene search, favourites and duplication | Makes a library larger than ten scenes usable | Native local database; low implementation risk |
| Fade duration and brightness ceiling per scene | Gentler transitions, bedtime and TV lighting | Use the existing SDK's HSL/CTL transition messages; verify lamp timing |
| Siri shortcuts and home-screen scene links | Run a scene without navigating the app | On iOS 15, widget taps generally open the app; avoid claiming newer interactive-widget behaviour |
| Encrypted network invitations and automatic scene sync | Lets controllers exchange new bulbs, renamed groups and later edits | Must allocate controller addresses centrally and reconcile changes without copying sequence state |
| Local web/API bridge on a spare device | Safari, desktop and other-platform control without installing an IPA | Bridge must remain available and connected to Bluetooth; authenticate local requests |
| Schedules and circadian colour temperature | Daily hands-off lighting | Current bulb model list lacks a Scheduler Server, so a running controller or bridge must execute schedules |
| Guided backup restoration | Recover from a lost phone without re-pairing | Restore network state with a fresh controller identity, or carefully preserve its counter |

Recommended next order: scene search/favourites, adjustable fades, then a local
bridge or encrypted controller invitations. Real audio visualisation needs its
own verified capture-to-light path and should not be inferred from Now Playing
metadata alone.

## Sources

- [LEDVANCE cloud shutdown notice](https://shop.ledvanceus.com/blogs/smart-support/articles/sylvania-smart-home-app-has-discontinued-cloud-server-operation-for-sylvania-smart-bluetooth-products/)
- [Bluetooth Mesh Model specification](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/MMDL_v1.1/out/en/index-en.html)
- [Realm 10.11.0 dynamic Realm interface](https://github.com/realm/realm-swift/blob/v10.11.0/Realm/RLMRealm_Dynamic.h)
- [Realm 10.11.0 dynamic object accessor](https://github.com/realm/realm-swift/blob/v10.11.0/Realm/RLMObjectBase.mm)

The exact branch, model coverage, database preservation and controller allocation
findings come from the supplied app and backups. Device behaviour remains to be
verified on your hardware.
