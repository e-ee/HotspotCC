#!/bin/sh
# Read-only troubleshooting for Sileo's "Uninstall Identifiers Mismatch".
# Does not install, remove or alter anything.
PKG="com.chatgpt.hotspotcc"
echo "=== dpkg installed package ==="
dpkg-query -W -f='\${Package} \${Version} \${Architecture} \${Status}\n' "$PKG" 2>&1 || true
echo "=== apt package recognition ==="
apt-cache policy "$PKG" 2>&1 || true
echo "=== simulated apt removal (does not remove anything) ==="
apt-get -s remove "$PKG" 2>&1 || true
echo "=== dpkg audits ==="
dpkg --audit 2>&1 || true
