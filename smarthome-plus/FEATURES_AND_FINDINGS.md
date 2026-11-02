# SYLVANIA Plus 3.3.0

This update focuses on working colour motion, a full Home transfer and dark mode.

The original vendor colour path forwards hue degrees and saturation/brightness
percent directly to the low-level HSL command. Studio previously sent normalized
16-bit fields, which exceeded these bulbs' expected range. Its encoder now
matches the actual original ARM64 implementation. Blue, green and magenta inputs
were replayed through that original code to verify the contract.

Home export uses Realm's write-copy operation and SQLite's backup API. It keeps
all existing scene values intact, including legacy sentinels which the former
JSON scene validator rejected. Export Home and Import Home replace the old
scene-sharing options. Import validates the copied Realm, stages the Home and
applies it on restart. Each receiving device preserves its own controller identity
and monotonic packet counter. A restore journal handles interrupted replacement.

Dark mode includes the original app's hard-coded white backgrounds and black
text, Plus's panels, a dark launch screen, and preserved scene colour chips.
The retired help/support/legal sections are hidden from the old menu.

Eight spatial moods remain available: Aurora, Comet, Ocean, Embers, Neon, Prism,
Breathe and Velvet. They generate colour motion without a microphone. They are
not audio beat analysis or screen capture. Bluetooth updates are paced and lamps
interpolate targets; the simulator does not prove physical lamp response.

Firmware findings from the earlier release remain in FIRMWARE_FINDINGS.md. This
update uses supported existing light models and does not flash any firmware.

Install the matching numbered IPA over the existing Plus app in TrollStore.
Do not uninstall first. Your current saved scenes and pairing state are retained.
