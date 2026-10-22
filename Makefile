# Build with the patched iOS 15.6 SDK containing ControlCenterUIKit.tbd.
TARGET = iphone:clang:15.6:15.0
ARCHS = arm64 arm64e
THEOS_PACKAGE_SCHEME = rootless
INSTALL_TARGET_PROCESSES = SpringBoard

include $(THEOS)/makefiles/common.mk

BUNDLE_NAME = HotspotCCProvider
HotspotCCProvider_BUNDLE_EXTENSION = bundle
HotspotCCProvider_FILES = HotspotCC.m
HotspotCCProvider_CFLAGS = -fobjc-arc
HotspotCCProvider_FRAMEWORKS = Foundation UIKit CoreTelephony SystemConfiguration
HotspotCCProvider_PRIVATE_FRAMEWORKS = ControlCenterUIKit
HotspotCCProvider_INSTALL_PATH = /Library/ControlCenter/CCSupport_Providers

include $(THEOS_MAKE_PATH)/bundle.mk
