#import "SceneTransfer.h"
#import <math.h>

static BOOL HPString(id value, NSUInteger limit) {
    return [value isKindOfClass:NSString.class] && [value length] > 0 && [value length] <= limit;
}
static BOOL HPInteger(id value, long long lower, long long upper) {
    if (![value isKindOfClass:NSNumber.class]) return NO;
    double n = [value doubleValue];
    return isfinite(n) && floor(n) == n && n >= lower && n <= upper;
}
static NSDictionary *HPReject(NSError **error, NSString *message) {
    if (error) *error = [NSError errorWithDomain:@"SylvaniaPlus.SceneTransfer" code:1
                                      userInfo:@{NSLocalizedDescriptionKey:message}];
    return nil;
}
@implementation HPSceneTransfer
+ (NSDictionary *)validatedDocument:(id)document meshUUID:(NSString *)meshUUID error:(NSError **)error {
    if (![document isKindOfClass:NSDictionary.class] ||
        ![document[@"format"] isEqual:@"sylvania-plus-scenes"] ||
        ![document[@"version"] isEqual:@1]) return HPReject(error, @"This is not a SYLVANIA Plus scene file.");
    if (!HPString(meshUUID,128) || ![document[@"meshUUID"] isEqual:meshUUID])
        return HPReject(error, @"These scenes belong to a different light network.");
    id routines = document[@"routines"];
    if (![routines isKindOfClass:NSArray.class]) return HPReject(error, @"The scene list is invalid.");
    NSMutableSet *identifiers = [NSMutableSet new];
    NSMutableArray *clean = [NSMutableArray new];
    for (id scene in routines) {
        if (![scene isKindOfClass:NSDictionary.class] || !HPString(scene[@"uuid"],128) ||
            !HPString(scene[@"routineName"],256) || !HPInteger(scene[@"color"],0,65535) ||
            ![scene[@"routineList"] isKindOfClass:NSArray.class]) return HPReject(error, @"A scene has invalid fields.");
        if ([identifiers containsObject:scene[@"uuid"]]) return HPReject(error, @"The file contains duplicate scene identifiers.");
        [identifiers addObject:scene[@"uuid"]];
        NSMutableArray *commands = [NSMutableArray new];
        for (id command in scene[@"routineList"]) {
            if (![command isKindOfClass:NSDictionary.class] || !HPString(command[@"uuid"],128) ||
                !HPString(command[@"deviceName"],256)) return HPReject(error, @"A scene contains an invalid light.");
            for (NSString *key in @[@"color",@"saturation",@"lightness",@"cct",@"deviceType"])
                if (!HPInteger(command[key],0,65535)) return HPReject(error, @"A scene contains an invalid light setting.");
            for (NSString *key in @[@"onOff",@"isGroup"])
                if (!HPInteger(command[key],0,1)) return HPReject(error, @"A scene contains an invalid switch setting.");
            NSMutableDictionary *entry = [NSMutableDictionary new];
            for (NSString *key in @[@"uuid",@"deviceName",@"color",@"saturation",@"lightness",@"onOff",@"cct",@"isGroup",@"deviceType"])
                entry[key] = command[key];
            [commands addObject:entry];
        }
        [clean addObject:@{@"uuid":scene[@"uuid"],@"routineName":scene[@"routineName"],
                          @"color":scene[@"color"],@"routineList":commands}];
    }
    return @{@"format":@"sylvania-plus-scenes",@"version":@1,@"meshUUID":meshUUID,@"routines":clean};
}
@end
