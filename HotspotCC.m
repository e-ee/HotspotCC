#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <ControlCenterUIKit/CCUIToggleModule.h>
#import <objc/message.h>
#import <objc/runtime.h>
#import <dlfcn.h>
#import <fcntl.h>
#import <unistd.h>

static NSString *const kModuleIdentifier = @"com.chatgpt.hotspotcc.toggle";
static NSString *const kStoreKey = @"com.apple.MobileInternetSharing";

static id HGet(id target, NSString *method) {
    SEL sel = NSSelectorFromString(method);
    if (!target || ![target respondsToSelector:sel]) return nil;
    return ((id (*)(id, SEL))objc_msgSend)(target, sel);
}
static BOOL HGetBool(id target, NSString *method, BOOL *value) {
    SEL sel = NSSelectorFromString(method);
    if (!target || ![target respondsToSelector:sel]) return NO;
    *value = ((BOOL (*)(id, SEL))objc_msgSend)(target, sel);
    return YES;
}
static id SpringBoardTetheringController(void) {
    for (NSString *className in @[@"SBTetheringController", @"SBPersonalHotspotController"]) {
        Class cls = NSClassFromString(className);
        if (!cls) continue;
        for (NSString *factory in @[@"sharedInstance", @"sharedController", @"sharedTetheringController"]) {
            id obj = HGet((id)cls, factory);
            if (obj) return obj;
        }
    }
    return nil;
}
static id TelephonyClient(void) {
    static id client;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        dlopen("/System/Library/Frameworks/CoreTelephony.framework/CoreTelephony", RTLD_LAZY);
        Class cls = NSClassFromString(@"CoreTelephonyClient");
        if (cls) client = [[cls alloc] init];
    });
    return client;
}
static BOOL ReadTetheringStore(BOOL *enabled) {
    void *sc = dlopen("/System/Library/Frameworks/SystemConfiguration.framework/SystemConfiguration", RTLD_LAZY);
    if (!sc) return NO;
    typedef const void *(*CreateStore)(const void *, const void *, void *, void *);
    typedef const void *(*CopyStoreValue)(const void *, const void *);
    CreateStore create = (CreateStore)dlsym(sc, "SCDynamicStoreCreate");
    CopyStoreValue copy = (CopyStoreValue)dlsym(sc, "SCDynamicStoreCopyValue");
    if (!create || !copy) return NO;
    const void *store = create(NULL, (__bridge const void *)@"HotspotCC", NULL, NULL);
    if (!store) return NO;
    const void *v = copy(store, (__bridge const void *)kStoreKey);
    BOOL known = NO;
    if (v) {
        NSDictionary *dict = (__bridge NSDictionary *)v;
        NSNumber *state = dict[@"State"];
        if ([state respondsToSelector:@selector(integerValue)] &&
            (state.integerValue == 1022 || state.integerValue == 1023)) {
            *enabled = state.integerValue == 1023;
            known = YES;
        }
        CFRelease(v);
    }
    CFRelease(store);
    return known;
}
static BOOL GetHotspotState(void) {
    BOOL state = NO;
    if (ReadTetheringStore(&state)) return state;
    id controller = SpringBoardTetheringController();
    for (NSString *getter in @[@"isPersonalHotspotEnabled", @"personalHotspotEnabled", @"tetheringEnabled"]) {
        if (HGetBool(controller, getter, &state)) return state;
    }
    id ct = TelephonyClient();
    SEL get = NSSelectorFromString(@"getTetheringStatusSync:");
    if (ct && [ct respondsToSelector:get]) {
        id status = ((id (*)(id, SEL, NSError **))objc_msgSend)(ct, get, NULL);
        NSNumber *asserted = HGet(status, @"asserted");
        if ([asserted respondsToSelector:@selector(boolValue)]) return asserted.boolValue;
    }
    return NO;
}
// Keep a diagnostic record in the mobile user's cache so a failed private-API
// call can be distinguished from Control Center not dispatching setSelected:.
static void HDiag(NSString *format, ...) {
    va_list args;
    va_start(args, format);
    NSString *message = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    NSLog(@"[HotspotCC] %@", message);
    NSString *line = [NSString stringWithFormat:@"%@ %@\n", [NSDate date], message];
    NSData *data = [line dataUsingEncoding:NSUTF8StringEncoding];
    int fd = open("/var/mobile/Library/Caches/HotspotCC.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd >= 0) {
        (void)write(fd, data.bytes, data.length);
        close(fd);
    }
}

// Apple's *actual* Control Center hotspot controller. This technique is also
// used by Flipswitch Switches/Hotspot.x; unlike the previous guessed
// SBTetheringController selectors, this calls the original CC implementation.
static id NativeHotspotController(void) {
    static id controller = nil;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSString *path = @"/System/Library/ControlCenter/Bundles/ConnectivityModule.bundle/ConnectivityModule";
        void *handle = dlopen(path.UTF8String, RTLD_LAZY | RTLD_GLOBAL);
        Class cls = NSClassFromString(@"CCUIConnectivityHotspotViewController");
        HDiag(@"connectivity loaded=%d class=%@", handle != NULL, cls);
        if (!cls) return;
        @try {
            controller = [[cls alloc] init];
            if ([controller respondsToSelector:@selector(startObservingStateChanges)]) {
                ((void (*)(id, SEL))objc_msgSend)(controller, @selector(startObservingStateChanges));
            }
            HDiag(@"native controller=%@ toggle=%d", controller,
                  [controller respondsToSelector:NSSelectorFromString(@"_toggleState")]);
        } @catch (NSException *e) {
            HDiag(@"native controller init exception: %@", e);
            controller = nil;
        }
    });
    return controller;
}


// Based on Flipswitch Switches/Hotspot.x: MISManager's explicit service
// state interface controls 1022 (off) / 1023 (on). A cached
// CCUIConnectivityHotspotViewController's "discoverable" flag is *not*
// equivalent to the actual service state, as confirmed on this iPhone.
static id FlipswitchMISManager(void) {
    static id manager = nil;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        void *prefHandle = dlopen("/System/Library/PreferenceBundles/WirelessModemSettings.bundle/WirelessModemSettings",
                                 RTLD_LAZY | RTLD_GLOBAL);
        void *frameworkHandle = dlopen("/System/Library/PrivateFrameworks/MobileInternetSharing.framework/MobileInternetSharing",
                                      RTLD_LAZY | RTLD_GLOBAL);
        Class cls = NSClassFromString(@"MISManager");
        manager = HGet((id)cls, @"sharedManager");
        HDiag(@"MISManager probes preference=%d framework=%d class=%@ manager=%@",
              prefHandle != NULL, frameworkHandle != NULL, cls, manager);
    });
    return manager;
}
static BOOL SetViaMIS(BOOL enabled) {
    id manager = FlipswitchMISManager();
    SEL set = NSSelectorFromString(@"setState:");
    if (![manager respondsToSelector:set]) {
        HDiag(@"MISManager setState: unavailable");
        return NO;
    }
    int state = enabled ? 1023 : 1022;
    @try {
        SEL get = NSSelectorFromString(@"getState:andReason:");
        if ([manager respondsToSelector:get]) {
            int current = 0;
            int reason = 0;
            ((void (*)(id, SEL, int *, int *))objc_msgSend)(manager, get, &current, &reason);
            HDiag(@"MISManager previous service state=%d reason=%d", current, reason);
        }
        ((void (*)(id, SEL, int))objc_msgSend)(manager, set, state);
        HDiag(@"MISManager explicitly requested state=%d", state);
        return YES;
    } @catch (NSException *e) {
        HDiag(@"MISManager exception: %@", e);
        return NO;
    }
}
static BOOL SetViaNativeGlobal(BOOL enabled) {
    id native = NativeHotspotController();
    SEL setter = NSSelectorFromString(@"_setGlobalServiceState:");
    if (![native respondsToSelector:setter]) {
        HDiag(@"native _setGlobalServiceState: unavailable");
        return NO;
    }
    @try {
        int state = enabled ? 1023 : 1022;
        ((void (*)(id, SEL, int))objc_msgSend)(native, setter, state);
        HDiag(@"native explicitly requested global service state=%d", state);
        return YES;
    } @catch (NSException *e) {
        HDiag(@"native global setter exception: %@", e);
        return NO;
    }
}
static BOOL SetViaNativeToggleIfSafe(BOOL enabled) {
    id native = NativeHotspotController();
    SEL toggle = NSSelectorFromString(@"_toggleState");
    BOOL discoverable = NO;
    BOOL valid = HGetBool(native, @"isDiscoverable", &discoverable);
    if (![native respondsToSelector:toggle] || !valid || discoverable == enabled) {
        HDiag(@"skipping native toggle: selector=%d discoverability readable=%d value=%d requested=%d",
              [native respondsToSelector:toggle], valid, discoverable, enabled);
        return NO;
    }
    @try {
        BOOL accepted = ((BOOL (*)(id, SEL))objc_msgSend)(native, toggle);
        HDiag(@"native _toggleState returned=%d; this is NOT proof service state changed", accepted);
        return accepted;
    } @catch (NSException *e) {
        HDiag(@"native _toggleState exception: %@", e);
        return NO;
    }
}
static BOOL SetViaCoreTelephony(BOOL enabled) {
    id client = TelephonyClient();
    SEL withCompletion = NSSelectorFromString(@"setTetheringActive:completion:");
    if (![client respondsToSelector:withCompletion]) {
        HDiag(@"CoreTelephony async setter unavailable");
        return NO;
    }
    @try {
        ((void (*)(id, SEL, BOOL, id))objc_msgSend)(client, withCompletion, enabled, ^(id result) {
            HDiag(@"CoreTelephony completion result=%@", result ?: @"nil");
        });
        HDiag(@"CoreTelephony requested enabled=%d", enabled);
        return YES;
    } @catch (NSException *e) {
        HDiag(@"CoreTelephony setter exception: %@", e);
        return NO;
    }
}
// Primary pathway uses an explicit 1022/1023 service state, not the
// controller's possibly stale discoverability. The verification phase
// can independently try the native global setter if MIS did not work.

// iOS treats hotspot service state and Wi-Fi discovery as independent.
// The private WiFiKit monitor is also used by Apple's connectivity tile.
static id DiscoveryMonitor(void) {
    static id monitor;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        monitor = HGet(NativeHotspotController(), @"phStateMonitor");
        if (!monitor) {
            dlopen("/System/Library/PrivateFrameworks/WiFiKit.framework/WiFiKit", RTLD_LAZY | RTLD_GLOBAL);
            Class cls = NSClassFromString(@"WFPersonalHotspotStateMonitor");
            if (cls) {
                @try { monitor = [[cls alloc] init]; }
                @catch (NSException *e) { HDiag(@"discovery monitor exception=%@", e); }
            }
        }
        HDiag(@"WiFiKit monitor=%@", monitor);
    });
    return monitor;
}
static void SetDiscovery(BOOL enabled) {
    id m = DiscoveryMonitor();
    SEL sel = NSSelectorFromString(@"setMISDiscoveryState:immediateDisable:");
    if (![m respondsToSelector:sel]) {
        HDiag(@"WiFiKit discovery setter unavailable");
        return;
    }
    @try {
        ((void (*)(id, SEL, BOOL, BOOL))objc_msgSend)(m, sel, enabled, !enabled);
        HDiag(@"WiFiKit requested discoverable=%d immediateDisable=%d", enabled, !enabled);
        SEL get = NSSelectorFromString(@"asyncMISDiscoveryState:");
        if ([m respondsToSelector:get]) {
            ((void (*)(id, SEL, id))objc_msgSend)(m, get, ^(BOOL current) {
                HDiag(@"WiFiKit reported discoverable=%d", current);
            });
        }
    } @catch (NSException *e) { HDiag(@"WiFiKit discovery exception=%@", e); }
}
// Prepare wireless transports as Settings does. Never power them off when
// sharing is disabled. USB and physical Ethernet depend on attached hardware.
static void PrepareWirelessTransports(void) {
    id wifi = HGet((id)NSClassFromString(@"SBWiFiManager"), @"sharedInstance");
    BOOL enabled = NO;
    BOOL readable = HGetBool(wifi, @"wiFiEnabled", &enabled);
    HDiag(@"Wi-Fi manager=%@ readable=%d enabled=%d", wifi, readable, enabled);
    SEL turnWifiOn = NSSelectorFromString(@"setWiFiEnabled:");
    if (wifi && [wifi respondsToSelector:turnWifiOn] && (!readable || !enabled)) {
        @try {
            ((void (*)(id, SEL, BOOL))objc_msgSend)(wifi, turnWifiOn, YES);
            HDiag(@"Wi-Fi power enabled");
        } @catch (NSException *e) { HDiag(@"Wi-Fi power exception=%@", e); }
    }
    dlopen("/System/Library/PrivateFrameworks/BluetoothManager.framework/BluetoothManager",
           RTLD_LAZY | RTLD_GLOBAL);
    id bt = HGet((id)NSClassFromString(@"BluetoothManager"), @"sharedInstance");
    enabled = NO;
    readable = HGetBool(bt, @"powered", &enabled);
    HDiag(@"Bluetooth manager=%@ readable=%d enabled=%d", bt, readable, enabled);
    SEL turnBluetoothOn = NSSelectorFromString(@"setPowered:");
    if (bt && [bt respondsToSelector:turnBluetoothOn] && (!readable || !enabled)) {
        @try {
            ((BOOL (*)(id, SEL, BOOL))objc_msgSend)(bt, turnBluetoothOn, YES);
            HDiag(@"Bluetooth power enabled");
        } @catch (NSException *e) { HDiag(@"Bluetooth power exception=%@", e); }
    }
}

static NSInteger SetHotspotState(BOOL enabled) {
    BOOL actual = NO;
    BOOL known = ReadTetheringStore(&actual);
    HDiag(@"requested hotspot=%d actualStateKnown=%d actual=%d", enabled, known, actual);
    if (known && actual == enabled) {
        HDiag(@"service state already matches requested state");
        return 0;
    }
    if (SetViaMIS(enabled)) return 1;
    if (SetViaNativeGlobal(enabled)) return 2;
    if (SetViaCoreTelephony(enabled)) return 3;
    // Last resort: only toggle when the controller's own state is
    // *opposite* the desired state. Never toggle it blindly.
    if (SetViaNativeToggleIfSafe(enabled)) return 4;
    HDiag(@"no usable hotspot setter");
    return -1;
}
@interface HotspotCCToggle : CCUIToggleModule {
    NSUInteger _toggleGeneration;
}
- (void)refreshHotspot;
@end
@implementation HotspotCCToggle
// Use Apple's native Personal Hotspot SF Symbol. No custom bitmap assets.
- (UIImage *)iconGlyph {
    return [[UIImage systemImageNamed:@"personalhotspot"] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
}
// CCUIToggleModule defaults to 1.0. Make the identical glyph 50% larger.
- (double)glyphScale { return 1.5; }
- (UIImage *)selectedIconGlyph { return [self iconGlyph]; }
- (UIColor *)selectedColor { return [UIColor systemGreenColor]; }
- (BOOL)isSelected { return GetHotspotState(); }
- (void)setSelected:(BOOL)selected {
    NSUInteger generation = ++_toggleGeneration;
    HDiag(@"CC button setSelected:%d request #%lu", selected, (unsigned long)generation);
    if (selected) PrepareWirelessTransports();
    else SetDiscovery(NO);

    NSInteger path = SetHotspotState(selected);
    [self refreshState];
    // Set discovery after service activation to avoid the "Not Discoverable"
    // state observed when only MISManager setState:1023 is called.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.65 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        if (generation != self->_toggleGeneration) return;
        SetDiscovery(selected);
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        if (generation != self->_toggleGeneration) return;
        BOOL actual = NO;
        BOOL known = ReadTetheringStore(&actual);
        HDiag(@"verify #1 path=%ld target=%d known=%d actual=%d", (long)path, selected, known, actual);
        if (known && actual == selected) { [self refreshState]; return; }
        if (path == 1) {
            if (!SetViaNativeGlobal(selected)) SetViaCoreTelephony(selected);
        } else if (path == 2) SetViaCoreTelephony(selected);
        else if (path == -1) HDiag(@"No usable hotspot service setter");
        [self refreshState];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            if (generation != self->_toggleGeneration) return;
            BOOL state = NO;
            BOOL valid = ReadTetheringStore(&state);
            HDiag(@"verify #2 target=%d known=%d actual=%d success=%d",
                  selected, valid, state, valid && state == selected);
            [self refreshState];
        });
    });
}
- (void)refreshHotspot {
    HDiag(@"refresh after toggle actual state=%d", GetHotspotState());
    [self refreshState];
}
@end
@interface HotspotCCProvider : NSObject {
    NSMutableDictionary *_instances;
}
@end
@implementation HotspotCCProvider
- (instancetype)init {
    self = [super init];
    if (self) _instances = [NSMutableDictionary new];
    return self;
}
- (NSUInteger)numberOfProvidedModules { return 1; }
- (NSString *)identifierForModuleAtIndex:(NSUInteger)index { return index == 0 ? kModuleIdentifier : nil; }
- (id)moduleInstanceForModuleIdentifier:(NSString *)identifier {
    if (![identifier isEqualToString:kModuleIdentifier]) return nil;
    id module = _instances[identifier];
    if (!module) {
        module = [HotspotCCToggle new];
        if (module) _instances[identifier] = module;
    }
    return module;
}
- (NSString *)displayNameForModuleIdentifier:(NSString *)identifier { return @"Personal Hotspot"; }
- (UIImage *)settingsIconForModuleIdentifier:(NSString *)identifier {
    return [UIImage systemImageNamed:@"personalhotspot"];
}
@end
