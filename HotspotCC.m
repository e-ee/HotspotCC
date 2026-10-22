#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <ControlCenterUIKit/CCUIToggleModule.h>
#import <objc/message.h>
#import <objc/runtime.h>
#import <dlfcn.h>

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
static BOOL SetHotspotState(BOOL enabled) {
    id controller = SpringBoardTetheringController();
    for (NSString *name in @[@"setPersonalHotspotEnabled:", @"setTetheringEnabled:"]) {
        SEL sel = NSSelectorFromString(name);
        if ([controller respondsToSelector:sel]) {
            ((void (*)(id, SEL, BOOL))objc_msgSend)(controller, sel, enabled);
            return YES;
        }
    }
    id ct = TelephonyClient();
    SEL sel = NSSelectorFromString(@"setTetheringActive:");
    if ([ct respondsToSelector:sel]) {
        ((id (*)(id, SEL, BOOL))objc_msgSend)(ct, sel, enabled);
        return YES;
    }
    NSLog(@"[HotspotCC] No working private setter found");
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
    if (SetHotspotState(selected)) {
        [self performSelector:@selector(refreshHotspot) withObject:nil afterDelay:0.5];
        [self performSelector:@selector(refreshHotspot) withObject:nil afterDelay:1.5];
    }
    [self refreshState];
}
- (void)refreshHotspot { [self refreshState]; }
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
