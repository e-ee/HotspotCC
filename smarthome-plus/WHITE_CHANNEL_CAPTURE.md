# White-channel scene capture

Native bulbs keep separate HSL and CTL registers. A CTL white command does not imply that an HSL Get reports the visible white output. Saving only HSL can therefore reproduce a previous coloured mood for a bulb currently displaying white.

The capture path observes native SDK HSL/CTL setters, including convenience setters and white-temperature setters, and remembers the last selected channel per bulb UUID and mesh. Group commands expand through model subscriptions. No new control commands are introduced by the observer.

Scene capture queries the selected channel. CTL reads use `presentCTLLightness` and `presentCTLTemperature`; white read timeouts omit that bulb rather than falling back to stale HSL. Returned temperature units pass through unchanged to the native scene field. The capture reader still requires a real response.

Native reflection and the original enum raw-value table establish:

| Native enum case | Persisted value |
| --- | --- |
| DEVICE_TYPE_GENERIC_ON_OFF_SERVER | 3 |
| DEVICE_TYPE_GENERIC_LEVEL_SERVER | 4 |
| DEVICE_TYPE_LIGHT_DIMMABLE | 5 |
| DEVICE_TYPE_LIGHT_CCT | 7 |
| DEVICE_TYPE_LIGHT_COLOR | 8 |

White entries store type 7, cct and lightness, with colour/saturation sentinels -1. Colour entries retain type 8. The mixed scene is exercised against real Realm 10.11 and a simulated native shortcut which intentionally leaves the old HSL register untouched. The simulated CTL status differs from the command target to prove that capture uses readback.

Channel tracking records commands issued through this installation. It does not claim to discover the currently active channel after an unobserved command from a different controller. Existing incorrectly saved scenes are not retroactively rewritten.


## 4.0.2 brightness regression rollback

The user verified that a static shortcut captured on 4.0.0 retains brightness, while a new shortcut captured on 4.0.1 dims when selected in the original top shortcut row. Playback is unchanged. Capture now always uses the 4.0.0 HSL readback for brightness, including white bulbs; the additional CTL query supplies temperature only. The SDK class-method wrappers introduced in 4.0.1 have been removed. Existing DeviceControlApi UI hooks observe the chosen white/colour channel instead, and Studio marks its own HSL writes explicitly. No brightness multipliers, compensation offsets or native configuration changes are applied.

Regression fixtures deliberately return a CTL level lower than the HSL level. The saved native type-7 entry keeps the HSL brightness and returned CTL temperature. The static Home shortcut callback is exercised through its original handler, with distinct levels for all fourteen lights, including off/full brightness. Unreadable bulbs are still omitted.
