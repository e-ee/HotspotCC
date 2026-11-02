# SYLVANIA Plus 3.2.0

The original controls, unlimited routine patch and separate controller profiles
are retained. New work fixes native ARGB scene sharing, removes the retired
smart-home-experiences shortcut, reduces PLUS opacity and adds shortcut ordering.

Light Studio provides eight moving colour patterns with room targeting, an
editable lamp path, brightness and motion control. The controls use the bulbs'
existing HSL transition model. A direct fade command allows local interpolation
without a separate packet for every animation frame. Ordinary controls pause
this controller's effect timer. Separate controllers should not run effects
simultaneously because they would send competing target states.

Additional native-model settings cover default fades and power-cut behaviour.
The HSL convenience method is adjusted to honour the configured bulb default
instead of always overriding it with an explicit zero-duration transition.

The share bug was an incorrect UInt16 range check on the routine-card colour.
The native renderer splits that value into four ARGB bytes, so the export format
now accepts unsigned 32-bit colours and their signed Int32 representation.

Tests cover 0/9/10/11/100/1000 routine files, packed 32-bit colours, wrong-home
and invalid records, stable custom ordering with newly added items, bounded
spatial and temporal output for every effect, and an actual UIKit/sender run
with fourteen simulated lamps. Physical radio and firmware-setting behaviour
have not been measured in this environment.

See INSTALL.txt for update steps, and FIRMWARE_FINDINGS.md for manufacturer
catalog/image inspection and the capabilities of the saved bulb network.
