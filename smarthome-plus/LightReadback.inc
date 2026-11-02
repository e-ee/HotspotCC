typedef void(^HPReadCompletion)(NSDictionary *reading,NSError *error);
static BOOL HPIdentifying;
static UIBackgroundTaskIdentifier HPIdentifyTask=UIBackgroundTaskInvalid;
static NSError *HPReadError(NSString *text){return [NSError errorWithDomain:@"LightReadback" code:1 userInfo:@{NSLocalizedDescriptionKey:text}];}
static BOOL HPTransportOpen(void){id bearer=HPGet(NSClassFromString(@"SigBearer"),@"share");SEL selector=NSSelectorFromString(@"isOpen");return [bearer respondsToSelector:selector]&&((BOOL(*)(id,SEL))objc_msgSend)(bearer,selector);}
static BOOL HPTransportBusy(void){return [HPGet(HPGet(NSClassFromString(@"SigMeshLib"),@"share"),@"commands")count]>0;}
static void HPReadColour(NSDictionary *light,HPReadCompletion completion){
    if(!HPTransportOpen()){completion(nil,HPReadError(@"Connect to your lights before reading this look."));return;}
    Class sdk=NSClassFromString(@"SDKLibCommand");SEL get=NSSelectorFromString(@"lightHSLGetWithDestination:retryCount:responseMaxCount:successCallback:resultCallback:");
    if(![sdk respondsToSelector:get]){completion(nil,HPReadError(@"The light colour reader is unavailable."));return;}
    __block BOOL finished=NO;
    void(^finish)(NSDictionary *,NSError *)=^(NSDictionary *value,NSError *error){dispatch_async(dispatch_get_main_queue(),^{if(finished)return;finished=YES;completion(value,error);});};
    @try{
        void(^success)(uint16_t,uint16_t,id)=^(uint16_t source,uint16_t destination,id status){
            if(source!=[light[@"address"]unsignedShortValue])return;
            SEL h=NSSelectorFromString(@"HSLHue"),s=NSSelectorFromString(@"HSLSaturation"),l=NSSelectorFromString(@"HSLLightness");
            if(![status respondsToSelector:h]||![status respondsToSelector:s]||![status respondsToSelector:l]){finish(nil,HPReadError(@"A light returned an incomplete colour."));return;}
            HPColor color=HPStatusColor(((uint16_t(*)(id,SEL))objc_msgSend)(status,h),((uint16_t(*)(id,SEL))objc_msgSend)(status,s),((uint16_t(*)(id,SEL))objc_msgSend)(status,l));
            HPWireColor wire=HPVendorColor(color);NSMutableDictionary *reading=[light mutableCopy];reading[@"hue"]=@(wire.hue);reading[@"sat"]=@(wire.saturation);reading[@"light"]=@(wire.lightness);finish(reading,nil);
        };
        void(^result)(BOOL,NSError *)=^(BOOL all,NSError *error){if(error)finish(nil,error);};
        ((id(*)(id,SEL,uint16_t,NSInteger,NSInteger,id,id))objc_msgSend)(sdk,get,[light[@"address"]unsignedShortValue],0,1,success,result);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,3*NSEC_PER_SEC),dispatch_get_main_queue(),^{finish(nil,HPReadError([NSString stringWithFormat:@"%@ did not return its colour. No scene was saved.",light[@"name"]?:@"A light"]));});
    }@catch(NSException *exception){finish(nil,HPReadError(exception.reason?:@"Could not read a light."));}
}
@interface HPSceneReadback : NSObject
@property(nonatomic,strong) NSArray *lights;
@property(nonatomic,strong) NSMutableArray *readings;
@property(nonatomic,copy) BOOL(^stillPaused)(void);
@property(nonatomic,copy) void(^progress)(NSUInteger,NSUInteger);
@property(nonatomic,copy) void(^completion)(NSArray *,NSError *);
@property(nonatomic) NSUInteger index;
@property(nonatomic) double queueWaitBegan;
- (void)next;
@end
@implementation HPSceneReadback
- (void)finish:(NSError *)error{if(!self.completion)return;void(^callback)(NSArray *,NSError *)=self.completion;self.completion=nil;self.progress=nil;callback(error?nil:self.readings,error);}
- (void)next{
    if(!self.completion)return;
    if(self.stillPaused&&!self.stillPaused()){[self finish:HPReadError(@"Pause the lighting before saving a scene.")];return;}
    if(self.index>=self.lights.count){[self finish:nil];return;}
    if(!HPTransportOpen()){[self finish:HPReadError(@"The lights disconnected. No scene was saved.")];return;}
    double now=NSProcessInfo.processInfo.systemUptime;
    if(HPTransportBusy()){
        if(!self.queueWaitBegan)self.queueWaitBegan=now;
        if(now-self.queueWaitBegan>6){[self finish:HPReadError(@"The light connection is busy. Please try saving again.")];return;}
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self next];});return;
    }
    self.queueWaitBegan=0;NSDictionary *light=self.lights[self.index];
    HPReadColour(light,^(NSDictionary *reading,NSError *error){
        if(!reading){[self finish:error];return;}
        if(!self.readings)self.readings=[NSMutableArray new];[self.readings addObject:reading];self.index++;
        if(self.progress)self.progress(self.index,self.lights.count);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self next];});
    });
}
@end
// A single brightness pulse preserves RGB/CCT mode and restores the read level.
static void HPIdentifyFinish(NSError *error,void(^completion)(NSError *)){
    HPIdentifying=NO;if(HPIdentifyTask!=UIBackgroundTaskInvalid){UIBackgroundTaskIdentifier task=HPIdentifyTask;HPIdentifyTask=UIBackgroundTaskInvalid;[UIApplication.sharedApplication endBackgroundTask:task];}completion(error);
}
static BOOL HPIdentifyLevel(NSDictionary *light,uint16_t level){
    if(!HPTransportOpen()||HPTransportBusy())return NO;
    Class sdk=NSClassFromString(@"SDKLibCommand");SEL set=NSSelectorFromString(@"lightLightnessSetWithDestination:lightness:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:");if(![sdk respondsToSelector:set])return NO;
    id transition=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"SigTransitionTime"),@selector(alloc));transition=((id(*)(id,SEL,uint8_t))objc_msgSend)(transition,NSSelectorFromString(@"initWithRawValue:"),6);
    ((id(*)(id,SEL,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,set,[light[@"address"]unsignedShortValue],level,transition,0,0,0,NO,nil,nil);return YES;
}
static void HPIdentifyRestore(NSDictionary *light,uint16_t level,NSUInteger attempt,void(^completion)(NSError *)){
    if(HPIdentifyLevel(light,level)){dispatch_after(dispatch_time(DISPATCH_TIME_NOW,700*NSEC_PER_MSEC),dispatch_get_main_queue(),^{HPIdentifyFinish(nil,completion);});return;}
    if(attempt>=50){HPIdentifyFinish(HPReadError(@"The connection dropped before brightness could be restored. Use the bulb's normal control to restore it."),completion);return;}
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{HPIdentifyRestore(light,level,attempt+1,completion);});
}
static void HPIdentifyPulse(NSDictionary *light,uint16_t original,NSUInteger attempt,void(^completion)(NSError *)){
    uint16_t pulse=original>65?MAX(20,original-35):MIN(85,MAX(35,original+30));
    if(HPIdentifyLevel(light,pulse)){dispatch_after(dispatch_time(DISPATCH_TIME_NOW,950*NSEC_PER_MSEC),dispatch_get_main_queue(),^{HPIdentifyRestore(light,original,0,completion);});return;}
    if(attempt>=20){HPIdentifyFinish(HPReadError(@"The light connection is busy. Tap the light again."),completion);return;}
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{HPIdentifyPulse(light,original,attempt+1,completion);});
}
static void HPIdentifyLight(NSDictionary *light,void(^completion)(NSError *)){
    if(HPIdentifying){completion(HPReadError(@"Wait for the current identification pulse to finish."));return;}
    HPIdentifying=YES;HPIdentifyTask=[UIApplication.sharedApplication beginBackgroundTaskWithName:@"Identify light" expirationHandler:^{if(HPIdentifyTask!=UIBackgroundTaskInvalid){UIBackgroundTaskIdentifier task=HPIdentifyTask;HPIdentifyTask=UIBackgroundTaskInvalid;[UIApplication.sharedApplication endBackgroundTask:task];}}];
    HPReadColour(light,^(NSDictionary *reading,NSError *error){if(!reading){HPIdentifyFinish(error,completion);return;}HPIdentifyPulse(light,[reading[@"light"]unsignedShortValue],0,completion);});
}
