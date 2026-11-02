# Build workflow

For small copy/layout changes: batch edits, run homeplus-quick.yml, then package and run verify_release.py. Check the requested text/layout change directly. Avoid a full simulator run for copy-only edits.

Run homeplus.yml manually for lighting engine, Bluetooth ABI, persistence, navigation, gesture, capture or controller-identity changes. It retains the full Realm and fourteen-lamp UIKit tests. Reuse the prior passing baseline for unrelated behavior and label that evidence accurately.

Keep private-profiles.plist stable. Never publish keys, real configuration, seeds or private IPAs to GitHub. Update the matching numbered app without uninstalling it. Publish archives atomically after integrity checks.

Collect failure artifacts before retrying. Rebuild only after a concrete code/test fix; avoid repeated status polling and speculative changes while waiting. Keep internal layer snapshots separate from operating-system screenshots.
