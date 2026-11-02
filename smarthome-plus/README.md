# SYLVANIA Plus 3.7.0 Universal

One private iOS 14+ IPA replaces the three numbered packages. Install it over existing Plus without uninstalling. Existing installations keep their controller identity, sequence counter, Home and scenes. The original SYLVANIA app remains separate.

## Add another device

1. Update the original Device 1 installation first. It is the main controller that issues invitations.
2. Install the same universal IPA on the new device. In Add this device, use Share setup request and transfer Device_Setup.sylrequest to your main controller.
3. On the main controller, open PLUS > Create device invitation, select the request, confirm, and transfer the returned Device_Invitation.sylhome to the new device.
4. On the new device, choose Import invitation, confirm, then close Plus from the app switcher and reopen it.

No bulbs are re-paired. Each invitation is bound to the requesting installation. Make a separate request for every new installation; an invitation is not a reusable family Home file. A reinstall needs a new request. Keep the main controller installed and backed up: its private reservation ledger prevents address reuse. Only the original Device 1 can issue invitations in this release; transferring that authority is not implemented.

Fresh installations use a randomly generated isolated mesh until enrollment. They cannot control the existing lights before importing their invitation. Current users of Device 2 and Device 3 can update directly and keep working without enrollment.

## Home updates

Export Home and Import Home still update already enrolled devices, preserving the receiver's identity and advancing its own counter. Home exports from the main controller include reserved controller records. Invitations and Home files contain private pairing information. Share them only with your own devices.

## Limits and verification

There is no three-device packaging cap. Capacity is bounded by the mesh's available address ranges, rather than an arbitrary device count. New controllers reserve separate 16-address unicast, group and scene ranges. Reservations are kept even if sharing is cancelled; repeating the same request returns its existing reservation. One controller should run continuous Light Studio effects at a time to avoid competing animations.

Automated enrollment tests exercise 21 unique reservations, isolated networks, receiver binding, repeat requests, replay rejection and journaled adoption. Existing Home/Realm and fourteen-lamp UIKit regressions also run for this identity change. Physical Bluetooth and actual iOS 14 installation must still be confirmed on the user's hardware.

The 32 moods, six journeys, colour shuffle, apartment map, paused scene capture, unlimited local scene count and header cleanup remain. No microphone or firmware flashing is used.
