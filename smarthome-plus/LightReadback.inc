typedef void(^HPReadCompletion)(NSDictionary *reading,NSError *error);
static BOOL HPIdentifying;
static UIBackgroundTaskIdentifier HPIdentifyTask;
static NSError *HPReadError(NSString *text){return [NSError errorWithDomain:@"LightReadback" code:1 userInfo:@{NSLocalizedDescriptionKey:text}];}
static BOOL HPTransportOpen(void){id bearer=HPGet(NSClassFromString(@"SigBearer"),@"share");SEL selector=NSSelectorFromString(@"isOpen");return [bearer respondsToSelector:selector]&&((BOOL(*)(id,SEL))objc_msgSend)(bearer,selector);}
static BOOL HPTransportBusy(void){return [HPGet(HPGet(NSClassFromString(@"SigMeshLib"),@"share"),@"commands")count]>0;}
#import "LightChannel.inc"
static NSMutableDictionary *HPCommandColours;
static void HPRememberColour(NSDictionary *light,HPWireColor value){if(!HPCommandColours)HPCommandColours=[NSMutableDictionary new];HPCommandColours[light[@"id"]]=@{@"hue":@(value.hue),@"sat":@(value.saturation),@"light":@(value.lightness)};}
static NSDictionary *HPCachedColour(NSDictionary *light){
    NSDictionary *sent=HPCommandColours[light[@"id"]];if(sent)return sent;
    // SigNodeModel stores raw Bluetooth Mesh 16-bit values, unlike the vendor's
    // command API. Its HSL_*100 getters call lightnessToLum in the original app.
    for(id node in HPGet(HPGet(NSClassFromString(@"SigDataSource"),@"share"),@"curNodes")){
        if(![HPGet(node,@"UUID")isEqual:light[@"id"]])continue;
        SEL h=NSSelectorFromString(@"HSL_Hue"),s=NSSelectorFromString(@"HSL_Saturation"),l=NSSelectorFromString(@"HSL_Lightness");
        if(![node respondsToSelector:h]||![node respondsToSelector:s]||![node respondsToSelector:l])return nil;
        uint16_t hue=((uint16_t(*)(id,SEL))objc_msgSend)(node,h),sat=((uint16_t(*)(id,SEL))objc_msgSend)(node,s),level=((uint16_t(*)(id,SEL))objc_msgSend)(node,l);
        HPWireColor value=HPVendorColor((HPColor){hue/65535.f,sat/65535.f,level/65535.f});return @{@"hue":@(value.hue),@"sat":@(value.saturation),@"light":@(value.lightness)};
    }return nil;
}
static void HPReadColour(NSDictionary *light,HPReadCompletion completion){
    if(!HPTransportOpen()){completion(nil,HPReadError(@"Connect to your lights before reading this look."));return;}
    Class sdk=NSClassFromString(@"SDKLibCommand");SEL get=NSSelectorFromString(@"lightHSLGetWithDestination:retryCount:responseMaxCount:successCallback:resultCallback:");
    if(![sdk respondsToSelector:get]){completion(nil,HPReadError(@"The light colour reader is unavailable."));return;}
    __block BOOL finished=NO;__block id handle=nil;
    void(^finish)(NSDictionary *,NSError *)=^(NSDictionary *value,NSError *error){dispatch_async(dispatch_get_main_queue(),^{if(finished)return;finished=YES;if(error&&[handle respondsToSelector:@selector(cancel)])HPVoid(handle,@"cancel");handle=nil;completion(value,error);});};
    @try{
        void(^success)(uint16_t,uint16_t,id)=^(uint16_t source,uint16_t destination,id status){
            if(source!=[light[@"address"]unsignedShortValue])return;
            SEL h=NSSelectorFromString(@"HSLHue"),s=NSSelectorFromString(@"HSLSaturation"),l=NSSelectorFromString(@"HSLLightness");
            if(![status respondsToSelector:h]||![status respondsToSelector:s]||![status respondsToSelector:l]){finish(nil,HPReadError(@"A light returned an incomplete colour."));return;}
            HPColor color=HPStatusColor(((uint16_t(*)(id,SEL))objc_msgSend)(status,h),((uint16_t(*)(id,SEL))objc_msgSend)(status,s),((uint16_t(*)(id,SEL))objc_msgSend)(status,l));
            HPWireColor wire=HPVendorColor(color);NSMutableDictionary *reading=[light mutableCopy];reading[@"hue"]=@(wire.hue);reading[@"sat"]=@(wire.saturation);reading[@"light"]=@(wire.lightness);finish(reading,nil);
        };
        void(^result)(BOOL,NSError *)=^(BOOL all,NSError *error){if(error)finish(nil,error);};
        handle=((id(*)(id,SEL,uint16_t,NSInteger,NSInteger,id,id))objc_msgSend)(sdk,get,[light[@"address"]unsignedShortValue],0,1,success,result);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,3*NSEC_PER_SEC),dispatch_get_main_queue(),^{finish(nil,HPReadError([NSString stringWithFormat:@"%@ did not return its colour.",light[@"name"]?:@"A light"]));});
    }@catch(NSException *exception){finish(nil,HPReadError(exception.reason?:@"Could not read a light."));}
}
// Read the currently selected native white channel, without falling back to an
// old HSL register when the white query times out.
static void HPReadWhiteTemperature(NSDictionary *light,NSDictionary *colour,HPReadCompletion completion){
 if(!HPTransportOpen()){completion(nil,HPReadError(@"Connect to your lights before reading this look."));return;}
 Class sdk=NSClassFromString(@"SDKLibCommand");SEL get=NSSelectorFromString(@"lightCTLGetWithDestination:retryCount:responseMaxCount:successCallback:resultCallback:");
 if(![sdk respondsToSelector:get]){completion(nil,HPReadError(@"The native white reader is unavailable."));return;}
 __block BOOL finished=NO;__block id handle=nil;
 void(^finish)(NSDictionary *,NSError *)=^(NSDictionary *value,NSError *error){dispatch_async(dispatch_get_main_queue(),^{if(finished)return;finished=YES;if(error&&[handle respondsToSelector:@selector(cancel)])HPVoid(handle,@"cancel");handle=nil;completion(value,error);});};
 @try{
 void(^success)(uint16_t,uint16_t,id)=^(uint16_t source,uint16_t destination,id status){
  if(source!=[light[@"address"]unsignedShortValue])return;
  SEL temperature=NSSelectorFromString(@"presentCTLTemperature");
  if(![status respondsToSelector:temperature]){finish(nil,HPReadError(@"A light returned incomplete white settings."));return;}
  uint16_t t=((uint16_t(*)(id,SEL))objc_msgSend)(status,temperature);
  NSMutableDictionary *reading=[colour mutableCopy];reading[@"kind"]=@"ctl";reading[@"cct"]=@(t);reading[@"hue"]=@0;reading[@"sat"]=@0;finish(reading,nil);
 };
 void(^result)(BOOL,NSError *)=^(BOOL all,NSError *error){if(error)finish(nil,error);};
 handle=((id(*)(id,SEL,uint16_t,NSInteger,NSInteger,id,id))objc_msgSend)(sdk,get,[light[@"address"]unsignedShortValue],0,1,success,result);
 dispatch_after(dispatch_time(DISPATCH_TIME_NOW,3*NSEC_PER_SEC),dispatch_get_main_queue(),^{finish(nil,HPReadError([NSString stringWithFormat:@"%@ did not return its white settings.",light[@"name"]?:@"A light"]));});
 }@catch(NSException *exception){finish(nil,HPReadError(exception.reason?:@"Could not read the white settings."));}
}
// Restore the v4.0.0 HSL brightness path for every bulb. The extra CTL query
// contributes only white temperature; it must not replace the bound HSL level.
static void HPReadSceneLight(NSDictionary *light,HPReadCompletion completion){
 HPReadColour(light,^(NSDictionary *colour,NSError *error){
  if(!colour||!HPUsesWhiteChannel(light)){completion(colour,error);return;}
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{HPReadWhiteTemperature(light,colour,completion);});
 });
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
- (void)finishAvailable;
@end
@implementation HPSceneReadback
- (void)finish:(NSError *)error{if(!self.completion)return;void(^callback)(NSArray *,NSError *)=self.completion;self.completion=nil;self.progress=nil;callback(error?nil:self.readings,error);}
- (void)finishAvailable{[self finish:self.readings.count?nil:HPReadError(@"None of the selected lights replied. No scene was saved.")];}
- (void)next{
    if(!self.completion)return;
    if(self.stillPaused&&!self.stillPaused()){[self finish:HPReadError(@"Pause the lighting before saving a scene.")];return;}
    if(self.index>=self.lights.count){[self finishAvailable];return;}
    if(!HPTransportOpen()){[self finishAvailable];return;}
    double now=NSProcessInfo.processInfo.systemUptime;
    if(HPTransportBusy()){
        if(!self.queueWaitBegan)self.queueWaitBegan=now;
        if(now-self.queueWaitBegan>6){[self finishAvailable];return;}
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self next];});return;
    }
    self.queueWaitBegan=0;NSDictionary *light=self.lights[self.index];
    HPReadSceneLight(light,^(NSDictionary *reading,NSError *error){
        // Keep actual replies only. A timeout must not discard other bulbs.
        if(reading){if(!self.readings)self.readings=[NSMutableArray new];[self.readings addObject:reading];}self.index++;
        if(self.progress)self.progress(self.index,self.lights.count);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self next];});
    });
}
@end
// Use the proven HSL sender, hold a contrasting marker, then verify restoration.
// The old short brightness-only pulse could be missed or ignored by these bulbs.
@interface HPLightIdentifier:NSObject
@property(nonatomic,strong) NSDictionary *light,*original;
@property(nonatomic,copy) void(^completion)(NSError *);
@property(nonatomic) BOOL finished,restoring;
- (void)finish:(NSError *)error;
- (void)begin:(NSUInteger)attempt;
- (void)mark:(NSUInteger)attempt;
- (void)restore:(NSUInteger)attempt;
@end
static BOOL HPIdentifyColour(NSDictionary *light,NSDictionary *value){
    if(!HPTransportOpen()||HPTransportBusy())return NO;
    Class sdk=NSClassFromString(@"SDKLibCommand");SEL set=NSSelectorFromString(@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:");if(![sdk respondsToSelector:set])return NO;
    id transition=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"SigTransitionTime"),@selector(alloc));transition=((id(*)(id,SEL,uint8_t))objc_msgSend)(transition,NSSelectorFromString(@"initWithRawValue:"),10);
    @try{((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,set,[light[@"address"]unsignedShortValue],[value[@"light"]unsignedShortValue],[value[@"hue"]unsignedShortValue],[value[@"sat"]unsignedShortValue],transition,0,0,0,NO,nil,nil);HPRememberChannel([light[@"address"]unsignedShortValue],@"hsl");return YES;}@catch(NSException *exception){return NO;}
}
static BOOL HPIdentifyMatches(NSDictionary *a,NSDictionary *b){if(!a||!b)return NO;for(NSString *key in @[@"hue",@"sat",@"light"])if(abs([a[key]intValue]-[b[key]intValue])>2)return NO;return YES;}
@implementation HPLightIdentifier
- (void)finish:(NSError *)error{
    if(self.finished)return;self.finished=YES;HPIdentifying=NO;
    if(HPIdentifyTask!=UIBackgroundTaskInvalid){UIBackgroundTaskIdentifier task=HPIdentifyTask;HPIdentifyTask=UIBackgroundTaskInvalid;[UIApplication.sharedApplication endBackgroundTask:task];}
    void(^callback)(NSError *)=self.completion;self.completion=nil;if(callback)callback(error);
}
- (void)begin:(NSUInteger)attempt{
    if(self.finished)return;
    if(!HPTransportOpen()){[self finish:HPReadError(@"Connect to your lights before identifying a bulb.")];return;}
    if(HPTransportBusy()){
        if(attempt>=60){[self finish:HPReadError(@"The connection is busy. Try Identify again.")];return;}
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self begin:attempt+1];});return;
    }
    self.original=HPCachedColour(self.light);
    if(self.original){[self mark:0];return;}
    HPReadColour(self.light,^(NSDictionary *reading,NSError *error){if(self.finished)return;if(!reading){[self finish:HPReadError(@"This light has no saved colour yet. Set it once with the normal colour control, then Identify it.")];return;}self.original=reading;[self mark:0];});
}
- (NSDictionary *)marker{
    int hue=[self.original[@"hue"]intValue];return @{@"hue":hue>=90&&hue<=260?@30:@190,@"sat":@90,@"light":@65};
}
- (void)mark:(NSUInteger)attempt{
    if(self.finished||self.restoring)return;
    if(attempt>=30){[self finish:HPReadError(@"The light connection is busy. Try Identify again.")];return;}
    if(HPIdentifyColour(self.light,[self marker])){dispatch_after(dispatch_time(DISPATCH_TIME_NOW,4*NSEC_PER_SEC),dispatch_get_main_queue(),^{[self restore:0];});return;}
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self mark:attempt+1];});
}
- (void)restore:(NSUInteger)attempt{
    if(self.finished)return;self.restoring=YES;
    if(attempt>=5){[self finish:HPReadError(@"The previous colour could not be confirmed. Reconnect and use the light's normal controls.")];return;}
    BOOL sent=HPIdentifyColour(self.light,self.original);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1400*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
        if(self.finished)return;if(!sent){[self restore:attempt+1];return;}
        HPReadColour(self.light,^(NSDictionary *reading,NSError *error){if(self.finished)return;if(!reading)[self finish:HPReadError(@"Previous colour sent. This light did not send a confirmation.")];else if(HPIdentifyMatches(reading,self.original))[self finish:nil];else[self restore:attempt+1];});
    });
}
@end
static void HPIdentifyLight(NSDictionary *light,void(^completion)(NSError *)){
    if(HPIdentifying){completion(HPReadError(@"Wait for this light to restore before identifying the next one."));return;}
    HPLightIdentifier *operation=[HPLightIdentifier new];operation.light=light;operation.completion=completion;HPIdentifying=YES;
    HPIdentifyTask=[UIApplication.sharedApplication beginBackgroundTaskWithName:@"Identify light" expirationHandler:^{
        if(operation.original)HPIdentifyColour(operation.light,operation.original);
        [operation finish:HPReadError(@"Keep the app open while identifying lights. Check the selected light's colour.")];
    }];[operation begin:0];
}


