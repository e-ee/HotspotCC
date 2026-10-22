# HotspotCC for iOS 15 (Dopamine)

A standalone Personal Hotspot Control Center toggle based on the [CCSupport](https://github.com/opa334/CCSupport) module-provider API.

This is experimental jailbreak software. Its private iOS tethering APIs require device testing.

## Build

The GitHub Actions workflow runs on macOS and compiles with [Theos](https://theos.dev). It produces a rootless Debian package and checks for an arm64e slice.

For a local macOS build with Xcode and Theos installed:

```sh
make clean
make package FINALPACKAGE=1
```

Install the resulting `.deb` with a rootless package manager, then respring and add Personal Hotspot under Settings > Control Center. Do not install old manually linked HotspotCC releases alongside this one.

## Recovery

```sh
sudo dpkg --purge com.chatgpt.hotspotcc
sbreload
```

Provider is installed under `/var/jb/Library/ControlCenter/CCSupport_Providers/HotspotCCProvider.bundle`. The full bundle is required; the dylib alone is insufficient.
