#import <Foundation/Foundation.h>
#import <Realm/Realm.h>
#import <Realm/RLMRealm_Dynamic.h>
#import "SceneCapture.h"
#include <assert.h>

// The exact native model fields/primary keys inspected in the supplied app.
@interface RoutineDevice : RLMObject
@property(nonatomic,strong) NSString *uuid,*deviceName;
@property NSInteger color,saturation,lightness,cct,deviceType;
@property BOOL onOff,isGroup;
@end
@implementation RoutineDevice
+ (NSString *)primaryKey{return @"uuid";}
@end
RLM_ARRAY_TYPE(RoutineDevice)
@interface RoutinesInfo : RLMObject
@property(nonatomic,strong) NSString *uuid,*routineName;
@property NSInteger color;
@property(nonatomic,strong) RLMArray<RoutineDevice *><RoutineDevice> *routineList;
@end
@implementation RoutinesInfo
+ (NSString *)primaryKey{return @"uuid";}
@end
int main(void){@autoreleasepool{
    NSError *error=nil;RLMRealmConfiguration *config=[RLMRealmConfiguration new];config.inMemoryIdentifier=NSUUID.UUID.UUIDString;config.objectClasses=@[RoutinesInfo.class,RoutineDevice.class];
    RLMRealm *realm=[RLMRealm realmWithConfiguration:config error:&error];assert(realm);
    NSMutableArray *readings=[NSMutableArray new];for(int i=0;i<14;i++)[readings addObject:@{@"id":[NSString stringWithFormat:@"fixture-%d",i],@"name":[NSString stringWithFormat:@"Lamp %d",i],@"hue":@(210+i),@"sat":@48,@"light":@(i?32:0)}];
    for(int i=0;i<11;i++){
        NSDictionary *old=[HPSceneCapture sceneWithName:[NSString stringWithFormat:@"Existing %d",i]readings:readings error:&error];assert([HPSceneCapture writeScene:old realm:realm error:&error]);
    }
    NSDictionary *scene=[HPSceneCapture sceneWithName:@"  Aurora Glow  "readings:readings error:&error];assert(scene);
    assert([HPSceneCapture writeScene:scene realm:realm error:&error]);
    RLMResults *all=[realm allObjects:@"RoutinesInfo"];assert(all.count==12);
    RoutinesInfo *saved=[RoutinesInfo objectInRealm:realm forPrimaryKey:scene[@"uuid"]];assert(saved&&[saved.routineName isEqual:@"Aurora Glow"]&&saved.routineList.count==14);
    NSMutableSet *ids=[NSMutableSet new];for(RoutineDevice *lamp in saved.routineList){assert(lamp.deviceType==8&&lamp.cct==-1&&!lamp.isGroup);[ids addObject:lamp.uuid];}assert(ids.count==14&&!saved.routineList[0].onOff&&saved.routineList[1].onOff);
    assert([realm allObjects:@"RoutineDevice"].count==12*14); // No primary-key reuse across scenes.
    NSURL *copy=[NSURL fileURLWithPath:[NSTemporaryDirectory()stringByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingString:@".realm"]]];
    assert([realm writeCopyToURL:copy encryptionKey:nil error:&error]);
    RLMRealmConfiguration *read=[RLMRealmConfiguration new];read.fileURL=copy;read.readOnly=YES;read.objectClasses=config.objectClasses;
    RLMRealm *restored=[RLMRealm realmWithConfiguration:read error:&error];assert(restored&&[restored allObjects:@"RoutinesInfo"].count==12);
    assert(![HPSceneCapture sceneWithName:@" "readings:readings error:&error]);
    NSMutableArray *duplicate=[readings mutableCopy];[duplicate addObject:readings[0]];assert(![HPSceneCapture sceneWithName:@"Bad"readings:duplicate error:&error]);
    // A legacy scene may share an entry with another scene. Deleting either
    // scene must not delete a child still referenced by its neighbour.
    NSString *sharedID=saved.routineList[0].uuid;
    [realm transactionWithBlock:^{RoutinesInfo *shared=[RoutinesInfo new];shared.uuid=@"shared-scene";shared.routineName=@"Keep shared entry";shared.color=0;[shared.routineList addObject:saved.routineList[0]];[realm addObject:shared];}];
    error=nil;assert([HPSceneCapture deleteScene:scene[@"uuid"]realm:realm error:&error]);assert(!error&&[realm allObjects:@"RoutinesInfo"].count==12&&[realm allObjects:@"RoutineDevice"].count==155);
    assert([RoutineDevice objectInRealm:realm forPrimaryKey:sharedID]);
    assert([HPSceneCapture deleteScene:@"shared-scene"realm:realm error:&error]);assert([realm allObjects:@"RoutinesInfo"].count==11&&[realm allObjects:@"RoutineDevice"].count==154);
    assert(![RoutineDevice objectInRealm:realm forPrimaryKey:sharedID]);
    assert(![HPSceneCapture deleteScene:@"missing"realm:realm error:&error]&&[realm allObjects:@"RoutinesInfo"].count==11);
    [NSFileManager.defaultManager removeItemAtURL:copy error:nil];
    puts("Real Realm 10.11: native scene schema, 14 actual readings, HSL type 8, independent entry keys, off bulbs, more than ten scenes, database-copy round trip and deletion with shared-child preservation passed.");
}}
