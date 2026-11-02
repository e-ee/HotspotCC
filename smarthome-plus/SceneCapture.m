#import "SceneCapture.h"
#import <objc/message.h>
#import <dlfcn.h>
#import "EffectPatterns.h"
static id HPCaptureError(NSError **error,NSString *reason){if(error)*error=[NSError errorWithDomain:@"SceneCapture" code:1 userInfo:@{NSLocalizedDescriptionKey:reason}];return nil;}
static BOOL HPCaptureNumber(id n,NSInteger max){return [n isKindOfClass:NSNumber.class]&&isfinite([n doubleValue])&&floor([n doubleValue])==[n doubleValue]&&[n integerValue]>=0&&[n integerValue]<=max;}
@implementation HPSceneCapture
+ (NSDictionary *)sceneWithName:(NSString *)name readings:(NSArray *)readings error:(NSError **)error{
    name=[name stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if(!name.length||name.length>40)return HPCaptureError(error,@"Give the scene a name of 1 to 40 characters.");
    if(!readings.count)return HPCaptureError(error,@"No light values were received.");
    NSMutableSet *seen=[NSMutableSet new];NSMutableArray *devices=[NSMutableArray new];
    for(NSDictionary *r in readings){
        if(![r isKindOfClass:NSDictionary.class]||![r[@"id"]isKindOfClass:NSString.class]||![r[@"name"]isKindOfClass:NSString.class]||![r[@"name"]length]||[seen containsObject:r[@"id"]]||!HPCaptureNumber(r[@"hue"],360)||!HPCaptureNumber(r[@"sat"],100)||!HPCaptureNumber(r[@"light"],100))return HPCaptureError(error,@"A light did not return a complete colour reading. No scene was saved.");
        [seen addObject:r[@"id"]];
        // Native enum case 4 has raw value 8 and dispatches HSL. Entry uuid is
        // its PRIMARY KEY, independently generated for every scene and lamp.
        [devices addObject:@{@"uuid":NSUUID.UUID.UUIDString,@"deviceName":r[@"name"],@"color":r[@"hue"],@"saturation":r[@"sat"],@"lightness":r[@"light"],@"onOff":@([r[@"light"]integerValue]>0),@"cct":@-1,@"isGroup":@NO,@"deviceType":@8}];
    }
    NSDictionary *first=readings.firstObject;double h=[first[@"hue"]doubleValue]/60.,s=[first[@"sat"]doubleValue]/100.,v=.65;
    double c=v*s,x=c*(1-fabs(fmod(h,2)-1)),m=v-c,r=0,g=0,b=0;
    if(h<1){r=c;g=x;}else if(h<2){r=x;g=c;}else if(h<3){g=c;b=x;}else if(h<4){g=x;b=c;}else if(h<5){r=x;b=c;}else{r=c;b=x;}
    uint32_t argb=0xff000000|((uint32_t)lround((r+m)*255)<<16)|((uint32_t)lround((g+m)*255)<<8)|(uint32_t)lround((b+m)*255);
    return @{@"uuid":NSUUID.UUID.UUIDString,@"routineName":name,@"color":@(argb),@"routineList":devices};
}
+ (BOOL)writeScene:(NSDictionary *)scene realm:(id)realm error:(NSError **)error{
    if(!realm){HPCaptureError(error,@"The scene database is not ready.");return NO;}
    BOOL began=NO;
    @try{
        ((void(*)(id,SEL))objc_msgSend)(realm,NSSelectorFromString(@"beginWriteTransaction"));began=YES;
        ((id(*)(id,SEL,id,id))objc_msgSend)(realm,NSSelectorFromString(@"createObject:withValue:"),@"RoutinesInfo",scene);
        BOOL ok=((BOOL(*)(id,SEL,NSError **))objc_msgSend)(realm,NSSelectorFromString(@"commitWriteTransaction:"),error);if(ok)return YES;
    }@catch(NSException *exception){HPCaptureError(error,exception.reason?:@"The scene could not be saved.");}
    if(began&&[[realm valueForKey:@"inWriteTransaction"]boolValue])((void(*)(id,SEL))objc_msgSend)(realm,NSSelectorFromString(@"cancelWriteTransaction"));return NO;
}
+ (BOOL)deleteScene:(NSString *)identifier realm:(id)realm error:(NSError **)error{
    if(!identifier.length||!realm){HPCaptureError(error,@"The scene is unavailable.");return NO;}
    BOOL began=NO;
    @try{
        SEL query=NSSelectorFromString(@"objects:withPredicate:");
        id matches=((id(*)(id,SEL,id,id))objc_msgSend)(realm,query,@"RoutinesInfo",[NSPredicate predicateWithFormat:@"uuid == %@",identifier]);NSMutableArray *scenes=[NSMutableArray new];for(id scene in matches)[scenes addObject:scene];
        if(!scenes.count){HPCaptureError(error,@"This scene was already removed.");return NO;}
        ((void(*)(id,SEL))objc_msgSend)(realm,NSSelectorFromString(@"beginWriteTransaction"));began=YES;
        id(*field)(id,NSString *)=dlsym(RTLD_DEFAULT,"RLMObjectBaseObjectForKeyedSubscript");
        for(id scene in scenes){
            id list=field?field(scene,@"routineList"):[scene valueForKey:@"routineList"];NSMutableDictionary *children=[NSMutableDictionary new];for(id child in list)children[[child valueForKey:@"uuid"]]=child;
            for(NSString *uuid in children){id others=((id(*)(id,SEL,id,id))objc_msgSend)(realm,query,@"RoutinesInfo",[NSPredicate predicateWithFormat:@"uuid != %@ AND ANY routineList.uuid == %@",identifier,uuid]);if(![others count])((void(*)(id,SEL,id))objc_msgSend)(realm,NSSelectorFromString(@"deleteObject:"),children[uuid]);}
            ((void(*)(id,SEL,id))objc_msgSend)(realm,NSSelectorFromString(@"deleteObject:"),scene);
        }
        if(((BOOL(*)(id,SEL,NSError **))objc_msgSend)(realm,NSSelectorFromString(@"commitWriteTransaction:"),error))return YES;
    }@catch(NSException *exception){HPCaptureError(error,exception.reason?:@"The scene could not be deleted.");}
    if(began&&[[realm valueForKey:@"inWriteTransaction"]boolValue])((void(*)(id,SEL))objc_msgSend)(realm,NSSelectorFromString(@"cancelWriteTransaction"));return NO;
}
@end
