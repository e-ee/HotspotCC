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

static BOOL SetHotspotState(BOOL enabled) {
    HDiag(@"requested hotspot=%d previous=%d", enabled, GetHotspotState());
    if (GetHotspotState() == enabled) {
        HDiag(@"already at requested state");
        return YES;
    }

    id native = NativeHotspotController();
    SEL toggle = NSSelectorFromString(@"_toggleState");
    if (native && [native respondsToSelector:toggle]) {
        @try {
            BOOL discoverable = NO;
            BOOL readable = HGetBool(native, @"isDiscoverable", &discoverable);
            HDiag(@"native current discoverable=%d readable=%d", discoverable, readable);
            if (!readable || discoverable != enabled) {
                BOOL accepted = ((BOOL (*)(id, SEL))objc_msgSend)(native, toggle);
                HDiag(@"native _toggleState returned %d", accepted);
                if (accepted) return YES;
            } else {
                HDiag(@"native discoverability already matches requested state");
                return YES;
            }
        } @catch (NSException *e) {
            HDiag(@"native toggle exception: %@", e);
        }
    }

    // Flipswitch's secondary route: MISManager controls the global sharing
    // service with the same state codes confirmed on this iPhone.
    dlopen("/System/Library/PrivateFrameworks/MobileInternetSharing.framework/MobileInternetSharing", RTLD_LAZY);
    Class misClass = NSClassFromString(@"MISManager");
    id manager = HGet((id)misClass, @"sharedManager");
    SEL setState = NSSelectorFromString(@"setState:");
    if (manager && [manager respondsToSelector:setState]) {
        @try {
            ((void (*)(id, SEL, int))objc_msgSend)(manager, setState, enabled ? 1023 : 1022);
            HDiag(@"MISManager setState:%d invoked", enabled ? 1023 : 1022);
            return YES;
        } @catch (NSException *e) {
            HDiag(@"MISManager exception: %@", e);
        }
    }
    HDiag(@"MISManager unavailable class=%@ manager=%@", misClass, manager);

    // Diagnostic fallback only. Calls to this CoreTelephony selector can be
    // ignored when the caller lacks telephony permissions.
    id ct = TelephonyClient();
    SEL sel = NSSelectorFromString(@"setTetheringActive:completion:");
    if ([ct respondsToSelector:sel]) {
        @try {
            ((void (*)(id, SEL, BOOL, id))objc_msgSend)(ct, sel, enabled,
                ^(id error) {
                    HDiag(@"CoreTelephony completion: %@", error ?: @"no error");
                });
            HDiag(@"CoreTelephony async request submitted");
            return YES;
        } @catch (NSException *e) {
            HDiag(@"CoreTelephony exception: %@", e);
        }
    }
    HDiag(@"all hotspot setters unavailable; CoreTelephony client=%@", ct);
    return NO;
}
@interface HotspotCCToggle : CCUIToggleModule
- (void)refreshHotspot;
@end
@implementation HotspotCCToggle
- (UIImage *)iconGlyph {
    UIImage *img = [UIImage imageNamed:@"HotspotGlyph" inBundle:[NSBundle bundleForClass:[self class]] compatibleWithTraitCollection:nil];
    return [img ?: [UIImage systemImageNamed:@"personalhotspot"] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
}
- (UIImage *)selectedIconGlyph { return [self iconGlyph]; }
- (UIColor *)selectedColor { return [UIColor systemGreenColor]; }
- (BOOL)isSelected { return GetHotspotState(); }
- (void)setSelected:(BOOL)selected {
    HDiag(@"CC button setSelected:%d", selected);
    if (SetHotspotState(selected)) {
        [self performSelector:@selector(refreshHotspot) withObject:nil afterDelay:0.5];
        [self performSelector:@selector(refreshHotspot) withObject:nil afterDelay:1.5];
    }
    [self refreshState];
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
    UIImage *img = [UIImage imageNamed:@"Icon" inBundle:[NSBundle bundleForClass:[self class]] compatibleWithTraitCollection:nil];
    return img ?: [UIImage systemImageNamed:@"personalhotspot"];
}
@end
