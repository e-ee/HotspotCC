// Included after the native scene/database helpers in HomePlus.m.
#import "EffectPatterns.h"
#import "StudioPresets.h"
#import "LightReadback.inc"
#import <unistd.h>

static NSArray *HPThemeNames(void){return @[@"Aurora",@"Comet",@"Ocean",@"Embers",@"Neon",@"Prism",@"Breathe",@"Velvet",@"Sunset",@"Moonlight",@"Fireflies",@"Tide",@"Silk",@"Rain",@"Dawn",@"Orbit",@"Forest Mist",@"Lagoon",@"Heather",@"Opal",@"Blue Hour",@"Coast",@"Rose Dusk",@"Polar Mist",@"Emerald Sky",@"Fjord",@"Cherry Blossom",@"Solstice",@"Nebula",@"Sea Glass",@"Candle Garden",@"Kaleidoscope"];}
static NSArray *HPThemeDetails(void){return @[@"Original jade, blue and violet ribbons",@"A soft glow drifting through home",@"Long, quiet teal swells",@"A steady, living amber glow",@"Soft rose and turquoise drift",@"A slow journey through colour",@"Gentle, warm breathing light",@"Quiet plum and indigo",@"Honey and peach across home",@"Soft silver-blue moonlight",@"Warm pools wandering nearby",@"Quiet rings through your space",@"Rose and lilac ribbons",@"Cool light drifting downward",@"A slow wash of warm morning",@"Colour circling the apartment",@"Sage and eucalyptus in the mist",@"Still turquoise, deepening blue",@"Lilac hills under a quiet sky",@"Pearl, sea glass and lavender",@"The sky just after sunset",@"Pale blue meeting soft green",@"Dusty rose fading into mauve",@"Jade and ice-blue veils",@"Deep greens in layered ribbons",@"Blue water flowing between cliffs",@"Rose and lilac petals drifting",@"Amber and gold flowing softly",@"Violet clouds in deep blue",@"Jade and turquoise crossing gently",@"Wandering pools of candlelight",@"Mirrored colour fields turning slowly"];}
static NSArray *HPThemeOrder(void){return @[@0,@24,@25,@26,@27,@28,@29,@30,@31,@16,@17,@18,@19,@20,@21,@22,@23,@2,@8,@9,@11,@12,@13,@14,@3,@6,@7,@1,@4,@5,@10,@15];}
static NSArray *HPJourneyNames(void){return @[@"Northern skies",@"Coastal drift",@"Warm evening",@"Forest after rain",@"Velvet night",@"Golden hour"];}
static NSArray *HPJourneyDetails(void){return @[@"Jade ribbons, lilac and blue hour",@"Sea glass, deep water and moonlight",@"Peach, embers and quiet fireflies",@"Emerald, sea glass and fjord blue",@"Blossom, violet clouds and moonlight",@"Sunrise, golden skies and candlelight"];}
static NSString *HPMoodStorageKey(void){return HPMeshUUID()?:@"current";}
static NSArray *HPSavedMoods(void){return [HPStudioPresets validRecipes:[NSUserDefaults.standardUserDefaults dictionaryForKey:@"HP.StudioMoods"][HPMoodStorageKey()]];}
static void HPStoreMoods(NSArray *moods){NSMutableDictionary *homes=[[NSUserDefaults.standardUserDefaults dictionaryForKey:@"HP.StudioMoods"]mutableCopy]?:[NSMutableDictionary new];homes[HPMoodStorageKey()]=moods;[NSUserDefaults.standardUserDefaults setObject:homes forKey:@"HP.StudioMoods"];}
static NSArray *HPOrdered(NSArray *items,NSString *key,NSString *identifier){
    NSArray *order=[NSUserDefaults.standardUserDefaults arrayForKey:key]?:@[];
    return [HPSceneTransfer orderedRecords:items identifiers:order key:identifier];
}
static NSArray *HPLightRecords(void){
    NSMutableArray *records=[NSMutableArray new];
    id source=HPGet(NSClassFromString(@"SigDataSource"),@"share");
    for(id node in HPGet(source,@"curNodes")){
        if(![HPGet(node,@"HSLAddresses") count])continue;
        uint16_t address=((uint16_t(*)(id,SEL))objc_msgSend)(node,NSSelectorFromString(@"address"));
        NSString *uuid=HPGet(node,@"UUID");if(!uuid)continue;
        [records addObject:@{@"id":uuid,@"name":HPGet(node,@"name")?:@"Light",@"address":@(address)}];
    }
    NSDictionary *layouts=[NSUserDefaults.standardUserDefaults dictionaryForKey:@"HP.LightLayout"];
    return [HPSpatialLayout positionedLights:HPOrdered(records,@"HP.LightOrder",@"id") layout:layouts[HPMeshUUID()?:@"current"]];
}
static NSDictionary *HPMesh(void){NSData *data=[NSUserDefaults.standardUserDefaults dataForKey:@"mesh.json"];return data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;}

@interface HPBulbMotion : NSObject
@property(nonatomic) HPColor color;
@property(nonatomic) double updatedAt;
@property(nonatomic) float referenceSaturation,referenceDepth;
@property(nonatomic) BOOL known;
@end
@implementation HPBulbMotion @end

@interface HPLightEngine : NSObject
@property(nonatomic,strong) NSTimer *timer;
@property(nonatomic,strong) NSArray *lights;
@property(nonatomic,strong) NSMutableDictionary *sent,*states,*heldTargets;
@property(nonatomic,strong) NSMutableSet *retargets;
@property(nonatomic,strong) id assertion;
@property(nonatomic) NSInteger mode,lastMode,cursor,journey;
@property(nonatomic) NSUInteger generation;
@property(nonatomic) float brightness,speed,depth;
@property(nonatomic) uint32_t variation;
@property(nonatomic) double lastSent,phase,lastTick,lastConnectAttempt,settleUntil,journeyElapsed;
@property(nonatomic) BOOL oldIdle,backgroundReady,connecting,needsRead,adjusting;
@property(nonatomic,strong) NSString *scope,*status;
+ (instancetype)shared;
- (void)start:(NSInteger)mode;
- (void)stop;
- (void)tick;
- (void)reloadLights;
- (void)startJourney:(NSInteger)journey;
- (void)resume;
- (void)activate:(NSInteger)mode journey:(NSInteger)journey reset:(BOOL)reset retarget:(BOOL)retarget;
- (void)changeBrightness:(float)value;
- (void)changeMotion:(float)value;

- (void)changeDepth:(float)value;
- (void)requestRetarget;
- (void)shuffleColours;
- (void)restoreColours;
- (void)applyColourVariation:(uint32_t)variation;
- (BOOL)prepareHeldAdjustment;
- (void)completeRetarget:(NSString *)key;
- (NSString *)moodName;
- (NSDictionary *)recipeNamed:(NSString *)name;
- (BOOL)applyRecipe:(NSDictionary *)recipe error:(NSError **)error;
- (HPColor)targetForLight:(NSDictionary *)light;
@end
@implementation HPLightEngine
+ (instancetype)shared {static HPLightEngine *engine;static dispatch_once_t once;dispatch_once(&once,^{
    engine=[HPLightEngine new];engine.mode=-1;engine.lastMode=0;engine.journey=-1;
    NSUserDefaults *prefs=NSUserDefaults.standardUserDefaults;
    engine.brightness=[prefs objectForKey:@"HP.StudioBrightness"]?HPClamp([prefs floatForKey:@"HP.StudioBrightness"],.08,1):.6;
    engine.speed=[prefs objectForKey:@"HP.StudioMotion"]?HPClamp([prefs floatForKey:@"HP.StudioMotion"],.15,1):.5;
    engine.depth=[prefs objectForKey:@"HP.StudioDepth"]?HPClamp([prefs floatForKey:@"HP.StudioDepth"],.25,1):1;
    engine.scope=@"All lights";engine.status=@"Choose a slow drift";
});return engine;}
- (void)reloadLights {
    NSArray *all=HPLightRecords();if([self.scope isEqual:@"All lights"]){self.lights=all;return;}
    NSDictionary *mesh=HPMesh();NSString *groupAddress;
    for(NSDictionary *group in mesh[@"groups"])if([group[@"name"]isEqual:self.scope]){groupAddress=[group[@"address"]uppercaseString];break;}
    NSMutableSet *members=[NSMutableSet new];
    for(NSDictionary *node in mesh[@"nodes"])for(NSDictionary *element in node[@"elements"])for(NSDictionary *model in element[@"models"])
        for(id subscription in model[@"subscribe"])if([subscription isKindOfClass:NSString.class]&&[[subscription uppercaseString]isEqual:groupAddress])[members addObject:node[@"UUID"]];
    NSMutableArray *selected=[NSMutableArray new];for(NSDictionary *light in all)if([members containsObject:light[@"id"]])[selected addObject:light];self.lights=selected;
}
- (NSString *)moodName{return self.journey>=0?HPJourneyNames()[self.journey]:HPThemeNames()[self.lastMode];}
- (HPColor)targetForLight:(NSDictionary *)light{return HPStudioTarget((int)self.lastMode,(int)self.journey,self.journeyElapsed,self.phase,[light[@"x"]doubleValue],[light[@"y"]doubleValue],self.brightness,self.depth,self.variation);}
- (NSDictionary *)recipeNamed:(NSString *)name{
    return @{@"id":NSUUID.UUID.UUIDString,@"name":[name stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet]?:@"",@"scope":self.scope,@"mode":@(self.lastMode),@"journey":@(self.journey),@"brightness":@(self.brightness),@"motion":@(self.speed),@"depth":@(self.depth),@"variation":@(self.variation),@"phase":@(self.phase),@"journeyElapsed":@(self.journeyElapsed)};
}
- (BOOL)applyRecipe:(NSDictionary *)recipe error:(NSError **)error{
    NSDictionary *valid=[HPStudioPresets validatedRecipe:recipe error:error];if(!valid)return NO;
    if(HPIdentifying){if(error)*error=HPReadError(@"Wait for the identification pulse to finish.");return NO;}
    BOOL found=[valid[@"scope"]isEqual:@"All lights"];for(NSDictionary *group in HPMesh()[@"groups"])if([group[@"name"]isEqual:valid[@"scope"]])found=YES;
    if(!found){if(error)*error=HPReadError(@"The room saved in this mood is not in this Home.");return NO;}
    [self stop];self.scope=valid[@"scope"];self.brightness=[valid[@"brightness"]floatValue];self.speed=[valid[@"motion"]floatValue];self.depth=[valid[@"depth"]floatValue];self.variation=[valid[@"variation"]unsignedIntValue];self.phase=[valid[@"phase"]doubleValue];self.journeyElapsed=[valid[@"journeyElapsed"]doubleValue];
    NSUserDefaults *prefs=NSUserDefaults.standardUserDefaults;[prefs setFloat:self.brightness forKey:@"HP.StudioBrightness"];[prefs setFloat:self.speed forKey:@"HP.StudioMotion"];[prefs setFloat:self.depth forKey:@"HP.StudioDepth"];
    [self activate:[valid[@"mode"]integerValue] journey:[valid[@"journey"]integerValue] reset:NO retarget:YES];
    if(self.mode<0){if(error)*error=HPReadError(self.status);return NO;}return YES;
}
- (void)start:(NSInteger)mode{[self activate:mode journey:-1 reset:self.lastMode!=mode||self.journey!=-1 retarget:YES];}
- (void)startJourney:(NSInteger)journey{if(journey<0||journey>=HP_JOURNEY_COUNT)return;[self activate:0 journey:journey reset:self.journey!=journey retarget:YES];}
- (void)resume{[self activate:self.lastMode journey:self.journey reset:NO retarget:NO];}
- (void)activate:(NSInteger)mode journey:(NSInteger)journey reset:(BOOL)reset retarget:(BOOL)retarget {
    if(mode<0||mode>=HP_MODE_COUNT)return;
    if(HPIdentifying){self.status=@"Finishing the light identification pulse";return;}
    [self reloadLights];if(!self.lights.count){[self stop];self.status=@"No colour lights in this room";return;}
    if(self.adjusting){[self stop];retarget=YES;}
    self.mode=mode;self.lastMode=mode;self.journey=journey;if(retarget)[self requestRetarget];if(reset){self.phase=0;self.journeyElapsed=0;}
    // Explicit choices get a native fade immediately; ambient evolution keeps its limiter.
    if(self.timer){self.status=[[self moodName]stringByAppendingString:@" · gently changing mood"];return;}
    self.generation++;self.lastTick=NSProcessInfo.processInfo.systemUptime;self.needsRead=NO;self.connecting=NO;self.cursor=0;
    self.sent=[NSMutableDictionary new];self.states=[NSMutableDictionary new];
    self.oldIdle=UIApplication.sharedApplication.idleTimerDisabled;UIApplication.sharedApplication.idleTimerDisabled=YES;
    dlopen("/System/Library/PrivateFrameworks/AssertionServices.framework/AssertionServices",RTLD_NOW);
    Class cls=NSClassFromString(@"BKSProcessAssertion");SEL init=NSSelectorFromString(@"initWithPID:flags:reason:name:withHandler:");
    if([cls instancesRespondToSelector:init]){
        id object=((id(*)(id,SEL))objc_msgSend)(cls,@selector(alloc));
        self.assertion=((id(*)(id,SEL,int,unsigned int,unsigned int,id,id))objc_msgSend)(object,init,getpid(),3,7,@"Slow light drift",nil);
        SEL valid=NSSelectorFromString(@"isValid");self.backgroundReady=[self.assertion respondsToSelector:valid]&&((BOOL(*)(id,SEL))objc_msgSend)(self.assertion,valid);
    }
    self.status=@"Starting a gentle fade";
    self.timer=[NSTimer timerWithTimeInterval:.10 target:self selector:@selector(tick) userInfo:nil repeats:YES];
    [[NSRunLoop mainRunLoop]addTimer:self.timer forMode:NSRunLoopCommonModes];[self tick];
}
- (void)requestRetarget{
    self.retargets=[NSMutableSet new];for(NSDictionary *light in self.lights)[self.retargets addObject:light[@"id"]];
}
- (BOOL)prepareHeldAdjustment{
    if(HPIdentifying){self.status=@"Wait for the selected light to restore";return NO;}
    if(self.mode>=0||self.adjusting)return YES;
    [self reloadLights];self.heldTargets=[NSMutableDictionary new];
    for(NSDictionary *light in self.lights){NSDictionary *cached=HPCachedColour(light);if(!cached)continue;HPBulbMotion *held=[HPBulbMotion new];held.color=HPStatusColor([cached[@"hue"]unsignedShortValue],[cached[@"sat"]unsignedShortValue],[cached[@"light"]unsignedShortValue]);held.referenceSaturation=held.color.saturation;held.referenceDepth=self.depth;self.heldTargets[light[@"id"]]=held;}
    if(!self.heldTargets.count){self.status=@"Choose a mood before shaping a paused look";return NO;}
    self.adjusting=YES;self.generation++;self.lastTick=NSProcessInfo.processInfo.systemUptime;self.oldIdle=UIApplication.sharedApplication.idleTimerDisabled;
    if(!self.states)self.states=[NSMutableDictionary new];if(!self.sent)self.sent=[NSMutableDictionary new];
    self.timer=[NSTimer timerWithTimeInterval:.10 target:self selector:@selector(tick)userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:self.timer forMode:NSRunLoopCommonModes];return YES;
}
- (void)completeRetarget:(NSString *)key{
    [self.retargets removeObject:key];
    if(self.adjusting&&!self.retargets.count){[self.timer invalidate];self.timer=nil;self.adjusting=NO;self.heldTargets=nil;self.status=@"Paused · look adjusted";}
}
- (void)changeBrightness:(float)value{
    if(!isfinite(value))return;value=HPClamp(value,.08,1);float old=self.brightness;if(fabs(value-old)<.00001)return;
    if(![self prepareHeldAdjustment])return;self.brightness=value;
    if(self.adjusting)for(HPBulbMotion *held in self.heldTargets.allValues){HPColor c=held.color;c.lightness=HPClamp(c.lightness*value/MAX(.08,old),0,1);held.color=c;}
    [self requestRetarget];[NSUserDefaults.standardUserDefaults setFloat:value forKey:@"HP.StudioBrightness"];self.status=@"Fading to your brightness";
}
- (void)changeDepth:(float)value{
    if(!isfinite(value))return;value=HPClamp(value,.25,1);float old=self.depth;if(fabs(value-old)<.00001)return;
    if(![self prepareHeldAdjustment])return;self.depth=value;
    if(self.adjusting)for(HPBulbMotion *held in self.heldTargets.allValues){HPColor c=held.color;c.saturation=HPHeldDepth(held.referenceSaturation,held.referenceDepth,value);held.color=c;}
    [self requestRetarget];[NSUserDefaults.standardUserDefaults setFloat:value forKey:@"HP.StudioDepth"];self.status=@"Fading to your colour depth";
}
- (void)changeMotion:(float)value{
    if(!isfinite(value))return;self.speed=HPClamp(value,.15,1);[NSUserDefaults.standardUserDefaults setFloat:self.speed forKey:@"HP.StudioMotion"];
    self.status=self.mode>=0?@"Motion updated":@"Motion set · tap Resume to see it";
}
- (void)applyColourVariation:(uint32_t)variation{
    if(![self prepareHeldAdjustment])return;self.variation=variation;
    if(self.adjusting)for(NSDictionary *light in self.lights){HPBulbMotion *held=self.heldTargets[light[@"id"]];if(!held)continue;HPColor c=held.color;c.hue=[self targetForLight:light].hue;held.color=c;}
    [self requestRetarget];self.status=variation?@"Fading into a fresh colour arrangement":@"Restoring the original palette";
}
- (void)shuffleColours{
    uint32_t value=self.variation;
    for(int attempt=0;attempt<8;attempt++){
        value=arc4random_uniform(1000000)+1;if(value==self.variation)continue;double difference=0;
        for(NSDictionary *light in self.lights){double x=[light[@"x"]doubleValue],y=[light[@"y"]doubleValue];HPColor a=[self targetForLight:light],b=HPStudioTarget((int)self.lastMode,(int)self.journey,self.journeyElapsed,self.phase,x,y,self.brightness,self.depth,value);double delta=fabs(a.hue-b.hue);difference+=MIN(delta,1-delta);}
        if(difference>self.lights.count*.025)break;
    }[self applyColourVariation:value];
}
- (void)restoreColours{[self applyColourVariation:0];}
- (void)stop {
    BOOL was=self.timer!=nil;[self.timer invalidate];self.timer=nil;self.mode=-1;self.adjusting=NO;self.heldTargets=nil;self.retargets=nil;self.generation++;
    if(self.assertion){HPVoid(self.assertion,@"invalidate");self.assertion=nil;}
    if(was)UIApplication.sharedApplication.idleTimerDisabled=self.oldIdle;
    self.status=@"Paused · save this look as a scene";
}
- (void)tick {
    if((self.mode<0&&!self.adjusting)||!self.lights.count)return;
    double now=NSProcessInfo.processInfo.systemUptime,gap=now-self.lastTick,elapsed=HPClamp(gap,0,.25);self.lastTick=now;
    if(gap>4){self.needsRead=YES;elapsed=0;}
    if(!HPTransportOpen()){
        self.needsRead=YES;
        self.status=@"Reconnecting to your lights";
        if(!self.connecting&&now-self.lastConnectAttempt>8){self.connecting=YES;self.lastConnectAttempt=now;
            id bearer=HPGet(NSClassFromString(@"SigBearer"),@"share");SEL connect=NSSelectorFromString(@"startMeshConnectWithComplete:");
            if([bearer respondsToSelector:connect])((void(*)(id,SEL,id))objc_msgSend)(bearer,connect,^(BOOL ok){dispatch_async(dispatch_get_main_queue(),^{self.connecting=NO;});});else self.connecting=NO;
        }return;
    }
    if(self.needsRead){self.generation++;[self.states removeAllObjects];[self.sent removeAllObjects];[HPCommandColours removeAllObjects];self.needsRead=NO;self.status=@"Resuming from the light controls";elapsed=0;}
    if(self.mode>=0){self.phase+=elapsed*HPMotionRate(self.speed);self.journeyElapsed+=elapsed;}
    id peripheral=HPGet(HPGet(NSClassFromString(@"SigBearer"),@"share"),@"getCurrentPeripheral");SEL ready=NSSelectorFromString(@"canSendWriteWithoutResponse");
    if([peripheral respondsToSelector:ready]&&!((BOOL(*)(id,SEL))objc_msgSend)(peripheral,ready))return;
    if(HPTransportBusy())return;
    NSUInteger index=(NSUInteger)self.cursor++%self.lights.count;NSDictionary *light=self.lights[index];NSString *key=light[@"id"];
    if(self.adjusting&&!self.heldTargets[key]){[self completeRetarget:key];return;}
    HPBulbMotion *state=self.states[key];if(!state){state=[HPBulbMotion new];self.states[key]=state;}
    if(!state.known){
        NSDictionary *cached=HPCachedColour(light);
        // Never hold the effect behind an HSL Get: some installed bulbs do not
        // answer it. Cache starts a bounded drift; a cacheless lamp uses its own
        // four-second native fade for the initial target, as the working 3.3 did.
        BOOL hasColour=cached&&([cached[@"hue"]intValue]||[cached[@"sat"]intValue]||[cached[@"light"]intValue]);
        state.color=hasColour?HPStatusColor([cached[@"hue"]unsignedShortValue],[cached[@"sat"]unsignedShortValue],[cached[@"light"]unsignedShortValue]):[self targetForLight:light];state.known=YES;state.updatedAt=now;
    }
    HPBulbMotion *held=self.heldTargets[key];HPColor target=self.adjusting?held.color:[self targetForLight:light];
    // A user's choice must reach the sender on the next visit to this bulb,
    // rather than spending minutes behind the 1-degree/sec ambient limiter.
    // The explicit native fade below provides the physical interpolation.
    BOOL requested=[self.retargets containsObject:key];state.color=requested?target:HPDrift(state.color,target,now-state.updatedAt);state.updatedAt=now;
    HPWireColor wire=HPVendorColor(state.color);NSArray *previous=self.sent[key];
    if(previous&&[previous[0]unsignedShortValue]==wire.lightness&&[previous[1]unsignedShortValue]==wire.hue&&[previous[2]unsignedShortValue]==wire.saturation){[self completeRetarget:key];return;}
    uint8_t steps=HPSmoothTransitionSteps((unsigned)self.lights.count);
    id transition=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"SigTransitionTime"),@selector(alloc));
    transition=((id(*)(id,SEL,uint8_t))objc_msgSend)(transition,NSSelectorFromString(@"initWithRawValue:"),steps);
    SEL set=NSSelectorFromString(@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:");Class sdk=NSClassFromString(@"SDKLibCommand");
    @try{
        if(![sdk respondsToSelector:set]){[self stop];self.status=@"The colour backend is unavailable";return;}
        ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,set,[light[@"address"]unsignedShortValue],wire.lightness,wire.hue,wire.saturation,transition,0,0,0,NO,nil,nil);
        self.sent[key]=@[@(wire.lightness),@(wire.hue),@(wire.saturation)];HPRememberColour(light,wire);self.lastSent=now;self.settleUntil=now+steps*.1;
        BOOL heldUpdate=self.adjusting;[self completeRetarget:key];
        if(!heldUpdate)self.status=[NSString stringWithFormat:@"%@ · slow drift · %lu lights",[self moodName],(unsigned long)self.lights.count];
    }@catch(NSException *exception){[self stop];self.status=@"Could not send the lighting effect";}
}
@end

#import "SpatialMap.inc"

@interface HPBulbSettings : UITableViewController @end
@implementation HPBulbSettings
- (void)viewDidLoad{[super viewDidLoad];self.title=@"Everyday light behaviour";}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView{return 2;}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section{return section==0?5:3;}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section{return section==0?@"Default fade time · all lights":@"After a power cut · all lights";}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section{
    return section==0?@"Choose how ordinary brightness and colour changes fade. Light Studio sets its own fade timing.":@"Restore previous uses each bulb's saved state. These commands change normal bulb settings; no firmware is replaced.";
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path{
    UITableViewCell *cell=[[UITableViewCell alloc]initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text=path.section==0?@[@"Instant",@"Half a second",@"One second",@"Two seconds",@"Five seconds"][path.row]:@[@"Stay off",@"Turn on",@"Restore previous light state"][path.row];return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path{
    [tableView deselectRowAtIndexPath:path animated:YES];[HPLightEngine.shared stop];
    id bearer=HPGet(NSClassFromString(@"SigBearer"),@"share");SEL open=NSSelectorFromString(@"isOpen");
    BOOL connected=[bearer respondsToSelector:open]&&((BOOL(*)(id,SEL))objc_msgSend)(bearer,open);
    NSString *message;
    if(!connected)message=@"Connect to your lights from Home, then apply this setting.";
    else @try{
        Class sdk=NSClassFromString(@"SDKLibCommand");
        if(path.section==0){
            uint8_t raw=[@[@0,@5,@10,@20,@50][path.row]unsignedCharValue];
            id transition=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"SigTransitionTime"),@selector(alloc));
            transition=((id(*)(id,SEL,uint8_t))objc_msgSend)(transition,NSSelectorFromString(@"initWithRawValue:"),raw);
            SEL selector=NSSelectorFromString(@"genericDefaultTransitionTimeSetWithDestination:defaultTransitionTime:retryCount:responseMaxCount:ack:successCallback:resultCallback:");
            ((id(*)(id,SEL,uint16_t,id,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,selector,0xffff,transition,0,0,NO,nil,nil);
            message=@"Fade setting sent to all lights. Try an ordinary colour or brightness change.";
        }else{
            SEL selector=NSSelectorFromString(@"genericOnPowerUpSetWithDestination:onPowerUp:retryCount:responseMaxCount:ack:successCallback:resultCallback:");
            ((id(*)(id,SEL,uint16_t,uint8_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,selector,0xffff,(uint8_t)path.row,0,0,NO,nil,nil);
            message=@"Power-on setting sent to all lights. It takes effect after their next power cycle.";
        }
    }@catch(NSException *exception){message=@"The bulb setting could not be sent.";}
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"Light behaviour" message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];[self presentViewController:alert animated:YES completion:nil];
}
@end

@interface HPPathEditor : UITableViewController
@property(nonatomic) BOOL scenes;
@property(nonatomic,strong) NSMutableArray *items;
@end
@implementation HPPathEditor
- (void)viewDidLoad {
    [super viewDidLoad];self.title=self.scenes?@"Scene shortcut order":@"Light travel path";
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc]initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(done)];
    NSArray *items=self.scenes?HPOrdered(HPExportRoutines(),@"HP.SceneOrder",@"uuid"):HPLightRecords();self.items=[items mutableCopy];
    self.tableView.editing=YES;self.tableView.allowsSelectionDuringEditing=NO;
}
- (void)done {
    NSMutableArray *ids=[NSMutableArray new];for(NSDictionary *item in self.items)[ids addObject:item[self.scenes?@"uuid":@"id"]];
    [NSUserDefaults.standardUserDefaults setObject:ids forKey:self.scenes?@"HP.SceneOrder":@"HP.LightOrder"];
    [NSUserDefaults.standardUserDefaults synchronize];
    if(!self.scenes&&HPLightEngine.shared.mode>=0)[HPLightEngine.shared start:HPLightEngine.shared.mode];
    [NSNotificationCenter.defaultCenter postNotificationName:@"HP.OrderChanged" object:nil];
    if(self.navigationController.viewControllers.count>1)[self.navigationController popViewControllerAnimated:YES];
    else [self dismissViewControllerAnimated:YES completion:nil];
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section{return self.items.count;}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)index {
    UITableViewCell *cell=[[UITableViewCell alloc]initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text=self.items[index.row][self.scenes?@"routineName":@"name"];cell.showsReorderControl=YES;return cell;
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    return self.scenes?@"Drag to reorder. Use the red minus to delete a scene; you will be asked to confirm.":@"Drag lamps into the order you want colour to travel through your apartment. The Comet and wave effects follow this path.";
}
- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)index{return self.scenes?UITableViewCellEditingStyleDelete:UITableViewCellEditingStyleNone;}
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)style forRowAtIndexPath:(NSIndexPath *)path{
    if(!self.scenes||style!=UITableViewCellEditingStyleDelete||path.row<0||(NSUInteger)path.row>=self.items.count)return;
    NSDictionary *scene=self.items[path.row];NSString *identifier=scene[@"uuid"];
    UIAlertController *confirm=[UIAlertController alertControllerWithTitle:[NSString stringWithFormat:@"Delete %@?",scene[@"routineName"]]message:@"The other scenes and the current lighting will be kept." preferredStyle:UIAlertControllerStyleAlert];
    [confirm addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [confirm addAction:[UIAlertAction actionWithTitle:@"Delete scene" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action){
        NSError *error=nil;
        if([HPSceneCapture deleteScene:identifier realm:HPRealm() error:&error]){
            NSIndexSet *removed=[self.items indexesOfObjectsPassingTest:^BOOL(NSDictionary *item,NSUInteger n,BOOL *stop){return [item[@"uuid"]isEqual:identifier];}];[self.items removeObjectsAtIndexes:removed];
            NSMutableArray *order=[[NSUserDefaults.standardUserDefaults arrayForKey:@"HP.SceneOrder"]mutableCopy]?:[NSMutableArray new];[order removeObject:identifier];[NSUserDefaults.standardUserDefaults setObject:order forKey:@"HP.SceneOrder"];
            [self.tableView reloadData];[NSNotificationCenter.defaultCenter postNotificationName:@"HP.OrderChanged" object:nil];
        }else{UIAlertController *failed=[UIAlertController alertControllerWithTitle:@"Could not delete scene" message:error.localizedDescription preferredStyle:UIAlertControllerStyleAlert];[failed addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];[self presentViewController:failed animated:YES completion:nil];}
    }]];[self presentViewController:confirm animated:YES completion:nil];
}
- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)index{return NO;}
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)index{return YES;}
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)from toIndexPath:(NSIndexPath *)to {
    id value=self.items[from.row];[self.items removeObjectAtIndex:from.row];[self.items insertObject:value atIndex:to.row];
}
@end

@interface HPStudioPreview : UIControl
@property(nonatomic,strong) NSArray *lights;
@end
@implementation HPStudioPreview
- (void)drawRect:(CGRect)rect {
    CGContextRef c=UIGraphicsGetCurrentContext();CGRect plot=CGRectInset(self.bounds,19,14);
    CGContextSetStrokeColorWithColor(c,[UIColor colorWithWhite:1 alpha:.055].CGColor);CGContextSetLineWidth(c,.5);
    for(int n=0;n<=6;n++){CGFloat x=plot.origin.x+n*plot.size.width/6.,y=plot.origin.y+n*plot.size.height/6.;CGContextMoveToPoint(c,x,plot.origin.y);CGContextAddLineToPoint(c,x,CGRectGetMaxY(plot));CGContextMoveToPoint(c,plot.origin.x,y);CGContextAddLineToPoint(c,CGRectGetMaxX(plot),y);}CGContextStrokePath(c);
    HPLightEngine *engine=HPLightEngine.shared;
    for(NSDictionary *light in self.lights){
        NSArray *sent=engine.sent[light[@"id"]];HPColor value=sent?HPStatusColor([sent[1]unsignedShortValue],[sent[2]unsignedShortValue],[sent[0]unsignedShortValue]):[engine targetForLight:light];
        UIColor *color=[UIColor colorWithHue:value.hue saturation:value.saturation brightness:value.lightness>0?.4+.6*value.lightness:.1 alpha:1];
        CGFloat x=plot.origin.x+[light[@"x"]doubleValue]*plot.size.width,y=plot.origin.y+[light[@"y"]doubleValue]*plot.size.height;
        CGContextSetShadowWithColor(c,CGSizeZero,13,color.CGColor);CGContextSetFillColorWithColor(c,color.CGColor);CGContextFillEllipseInRect(c,CGRectMake(x-5,y-5,10,10));
    }
}
@end
@interface HPModeCard : UIControl
@property(nonatomic,readonly) CAGradientLayer *palette;
@property(nonatomic,strong) UILabel *caption;
@property(nonatomic,strong) NSString *recipeID;
@end
@implementation HPModeCard
+ (Class)layerClass{return CAGradientLayer.class;}
- (CAGradientLayer *)palette{return (CAGradientLayer *)self.layer;}
- (void)layoutSubviews{[super layoutSubviews];self.caption.frame=CGRectInset(self.bounds,9,8);}
@end

@interface HPStudioScrollView:UIScrollView @end
@implementation HPStudioScrollView
- (BOOL)touchesShouldCancelInContentView:(UIView *)view{
    // UIKit otherwise returns NO for UIControl, including our full-width cards.
    // A vertical pan must cancel a card press and scroll; slider tracking stays local.
    for(UIView *v=view;v&&v!=self;v=v.superview)if([v isKindOfClass:UISlider.class])return NO;
    return YES;
}
@end
@interface HPStudio : UIViewController
@property(nonatomic,strong) UILabel *status;
@property(nonatomic,strong) HPStudioPreview *preview;
@property(nonatomic,strong) NSTimer *previewTimer;
@property(nonatomic,strong) UIButton *scopeButton,*pause,*save,*shuffleButton;
@property(nonatomic,strong) UISlider *brightnessSlider,*motionSlider,*depthSlider;
@property(nonatomic,strong) UILabel *brightnessLabel,*motionLabel,*depthLabel;
@property(nonatomic,strong) UISegmentedControl *category;
@property(nonatomic,strong) UIStackView *gallery;
@property(nonatomic,strong) HPSceneReadback *captureReader;
- (void)capturePausedSceneWithName:(NSString *)name completion:(void(^)(NSError *))completion;
- (BOOL)saveMovingMoodNamed:(NSString *)name error:(NSError **)error;
- (void)deleteMovingMood:(NSString *)identifier;
@end
@implementation HPStudio
- (UILabel *)text:(NSString *)text size:(CGFloat)size {
    UILabel *label=[UILabel new];label.text=text;label.font=[UIFont systemFontOfSize:size weight:UIFontWeightSemibold];label.textColor=UIColor.whiteColor;label.numberOfLines=0;return label;
}
- (UIButton *)button:(NSString *)title action:(SEL)action filled:(BOOL)filled{
    UIButton *button=[UIButton buttonWithType:UIButtonTypeSystem];[button setTitle:title forState:UIControlStateNormal];[button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];button.titleLabel.font=[UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];button.backgroundColor=filled?[UIColor colorWithRed:.30 green:.24 blue:.44 alpha:1]:[UIColor colorWithRed:.11 green:.13 blue:.18 alpha:1];button.layer.cornerRadius=13;[button.heightAnchor constraintEqualToConstant:44].active=YES;[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];return button;
}
- (UISlider *)slider:(NSString *)title minimum:(float)minimum value:(float)value action:(SEL)action row:(UIStackView *)row{
    UIStackView *column=[UIStackView new];column.axis=UILayoutConstraintAxisVertical;column.spacing=3;UILabel *label=[self text:title size:12];label.font=[UIFont systemFontOfSize:11 weight:UIFontWeightSemibold];label.adjustsFontSizeToFitWidth=YES;label.minimumScaleFactor=.75;label.numberOfLines=1;[column addArrangedSubview:label];if([title isEqual:@"Brightness"])self.brightnessLabel=label;else if([title isEqual:@"Motion"])self.motionLabel=label;else self.depthLabel=label;
    UISlider *slider=[UISlider new];slider.minimumValue=minimum;slider.maximumValue=1;slider.value=value;slider.accessibilityLabel=title;slider.continuous=YES;slider.exclusiveTouch=YES;slider.minimumTrackTintColor=[UIColor colorWithRed:.67 green:.61 blue:.84 alpha:1];[slider addTarget:self action:action forControlEvents:UIControlEventValueChanged];[column addArrangedSubview:slider];[row addArrangedSubview:column];return slider;
}
- (void)viewDidLoad {
    [super viewDidLoad];self.title=@"Light Studio";self.view.backgroundColor=[UIColor colorWithRed:.035 green:.045 blue:.07 alpha:1];self.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    UINavigationBarAppearance *appearance=[UINavigationBarAppearance new];[appearance configureWithOpaqueBackground];appearance.backgroundColor=self.view.backgroundColor;appearance.titleTextAttributes=@{NSForegroundColorAttributeName:UIColor.whiteColor};self.navigationItem.standardAppearance=appearance;self.navigationItem.scrollEdgeAppearance=appearance;self.navigationItem.compactAppearance=appearance;
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc]initWithTitle:@"More" style:UIBarButtonItemStylePlain target:self action:@selector(more)];
    UIScrollView *scroll=[HPStudioScrollView new];scroll.delaysContentTouches=NO;scroll.canCancelContentTouches=YES;scroll.alwaysBounceVertical=YES;scroll.translatesAutoresizingMaskIntoConstraints=NO;[self.view addSubview:scroll];UIStackView *stack=[UIStackView new];stack.axis=UILayoutConstraintAxisVertical;stack.spacing=12;stack.translatesAutoresizingMaskIntoConstraints=NO;[scroll addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[[scroll.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],[scroll.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],[scroll.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],[scroll.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],[stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:16],[stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-110],[stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:20],[stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-20],[stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-40]]];
    [stack addArrangedSubview:[self text:@"Let the room exhale." size:26]];
    UIStackView *room=[UIStackView new];room.axis=UILayoutConstraintAxisHorizontal;room.distribution=UIStackViewDistributionEqualSpacing;
    self.scopeButton=[UIButton buttonWithType:UIButtonTypeSystem];self.scopeButton.titleLabel.font=[UIFont systemFontOfSize:13];[self.scopeButton addTarget:self action:@selector(chooseRoom)forControlEvents:UIControlEventTouchUpInside];[room addArrangedSubview:self.scopeButton];
    UIButton *map=[UIButton buttonWithType:UIButtonTypeSystem];[map setTitle:@"Room preview · Edit map"forState:UIControlStateNormal];map.titleLabel.font=[UIFont systemFontOfSize:11];[map addTarget:self action:@selector(mapLights)forControlEvents:UIControlEventTouchUpInside];[room addArrangedSubview:map];[stack addArrangedSubview:room];
    self.preview=[HPStudioPreview new];self.preview.backgroundColor=[UIColor colorWithRed:.06 green:.075 blue:.10 alpha:1];self.preview.layer.cornerRadius=17;self.preview.clipsToBounds=YES;self.preview.accessibilityLabel=@"Room colour preview";self.preview.accessibilityHint=@"Approximate colours in your mapped positions. Opens the apartment map.";[self.preview addTarget:self action:@selector(mapLights)forControlEvents:UIControlEventTouchUpInside];[self.preview.heightAnchor constraintEqualToConstant:94].active=YES;[stack addArrangedSubview:self.preview];
    UIStackView *sliders=[UIStackView new];sliders.axis=UILayoutConstraintAxisHorizontal;sliders.spacing=14;sliders.distribution=UIStackViewDistributionFillEqually;
    self.brightnessSlider=[self slider:@"Brightness" minimum:.08 value:HPLightEngine.shared.brightness action:@selector(brightness:)row:sliders];self.motionSlider=[self slider:@"Motion" minimum:.15 value:HPLightEngine.shared.speed action:@selector(speed:)row:sliders];self.depthSlider=[self slider:@"Colour depth" minimum:.25 value:HPLightEngine.shared.depth action:@selector(depth:)row:sliders];[stack addArrangedSubview:sliders];
    UIStackView *actions=[UIStackView new];actions.axis=UILayoutConstraintAxisHorizontal;actions.spacing=10;actions.distribution=UIStackViewDistributionFillEqually;
    self.pause=[self button:@"Resume" action:@selector(toggle)filled:YES];self.save=[self button:@"Save as scene"action:@selector(saveScene)filled:NO];[actions addArrangedSubview:self.pause];[actions addArrangedSubview:self.save];[stack addArrangedSubview:actions];
    self.status=[self text:@""size:12];self.status.textColor=[UIColor colorWithWhite:.64 alpha:1];[self.status.heightAnchor constraintGreaterThanOrEqualToConstant:30].active=YES;[stack addArrangedSubview:self.status];
    self.shuffleButton=[self button:@"Shuffle colours"action:@selector(shuffleColours)filled:NO];self.shuffleButton.accessibilityIdentifier=@"HP.ShuffleColours";self.shuffleButton.accessibilityHint=@"Give this mode a new palette or mirror its flow. The room fades into the new look.";[stack addArrangedSubview:self.shuffleButton];
    self.category=[[UISegmentedControl alloc]initWithItems:@[@"Moods",@"Journeys",@"My moods"]];self.category.selectedSegmentIndex=0;self.category.selectedSegmentTintColor=[UIColor colorWithRed:.25 green:.23 blue:.34 alpha:1];[self.category setTitleTextAttributes:@{NSForegroundColorAttributeName:UIColor.whiteColor}forState:UIControlStateNormal];[self.category addTarget:self action:@selector(buildGallery)forControlEvents:UIControlEventValueChanged];[stack addArrangedSubview:self.category];
    self.gallery=[UIStackView new];self.gallery.axis=UILayoutConstraintAxisVertical;self.gallery.spacing=11;[stack addArrangedSubview:self.gallery];[self buildGallery];[self reloadPreview];
}
- (void)reloadPreview{[HPLightEngine.shared reloadLights];self.preview.lights=[HPLightEngine.shared.lights copy];}
- (HPModeCard *)card:(NSString *)name detail:(NSString *)detail mode:(int)mode journey:(int)journey{
    HPModeCard *button=[[HPModeCard alloc]initWithFrame:CGRectZero];NSMutableArray *colors=[NSMutableArray new];
    for(int n=0;n<3;n++){HPColor sample=HPStudioTarget(mode,journey,0,45,n*.5,.35,.8,1,0);[colors addObject:(id)[UIColor colorWithHue:sample.hue saturation:sample.saturation brightness:.22+.05*n alpha:1].CGColor];}
    button.palette.colors=colors;button.palette.startPoint=CGPointMake(0,0);button.palette.endPoint=CGPointMake(1,1);button.layer.cornerRadius=17;button.layer.masksToBounds=YES;button.layer.borderWidth=.5;button.layer.borderColor=[UIColor colorWithWhite:1 alpha:.13].CGColor;[button.heightAnchor constraintEqualToConstant:96].active=YES;
    NSMutableAttributedString *title=[[NSMutableAttributedString alloc]initWithString:[NSString stringWithFormat:@"%@\n%@",name,detail]attributes:@{NSForegroundColorAttributeName:UIColor.whiteColor,NSFontAttributeName:[UIFont systemFontOfSize:12]}];[title addAttribute:NSFontAttributeName value:[UIFont systemFontOfSize:17 weight:UIFontWeightSemibold]range:NSMakeRange(0,name.length)];button.caption=[UILabel new];button.caption.attributedText=title;button.caption.numberOfLines=3;button.caption.textAlignment=NSTextAlignmentCenter;button.caption.userInteractionEnabled=NO;button.caption.isAccessibilityElement=NO;[button addSubview:button.caption];button.opaque=NO;button.isAccessibilityElement=YES;button.accessibilityTraits=UIAccessibilityTraitButton;button.accessibilityLabel=[NSString stringWithFormat:@"%@, %@",name,detail];return button;
}
- (void)buildGallery{
    for(UIView *view in [self.gallery.arrangedSubviews copy]){[self.gallery removeArrangedSubview:view];[view removeFromSuperview];}
    NSInteger category=self.category.selectedSegmentIndex;
    if(category==0){for(int row=0;row<HP_MODE_COUNT/2;row++){
        UIStackView *pair=[UIStackView new];pair.axis=UILayoutConstraintAxisHorizontal;pair.spacing=11;pair.distribution=UIStackViewDistributionFillEqually;
        for(int col=0;col<2;col++){int mode=[HPThemeOrder()[row*2+col]intValue];HPModeCard *card=[self card:HPThemeNames()[mode]detail:HPThemeDetails()[mode]mode:mode journey:-1];card.tag=mode;card.accessibilityIdentifier=[@"HP.Mood."stringByAppendingString:HPThemeNames()[mode]];[card addTarget:self action:@selector(selectTheme:)forControlEvents:UIControlEventTouchUpInside];[pair addArrangedSubview:card];}[self.gallery addArrangedSubview:pair];
    }}else if(category==1){
        UILabel *hint=[self text:@"Related palettes unfold over half an hour, with two-minute dissolves. Journeys repeat gently."size:12];hint.textColor=[UIColor colorWithWhite:.62 alpha:1];[self.gallery addArrangedSubview:hint];
        for(int n=0;n<HP_JOURNEY_COUNT;n++){HPModeCard *card=[self card:HPJourneyNames()[n]detail:HPJourneyDetails()[n]mode:0 journey:n];card.tag=n;card.accessibilityIdentifier=[@"HP.Journey."stringByAppendingString:HPJourneyNames()[n]];[card addTarget:self action:@selector(selectJourney:)forControlEvents:UIControlEventTouchUpInside];[self.gallery addArrangedSubview:card];}
    }else{
        NSArray *recipes=HPSavedMoods();UILabel *hint=[self text:recipes.count?@"Tap to play. Touch and hold a mood to delete it.":@"Find a look you love. Use More, then Save moving mood to keep its colours, motion and starting point."size:13];hint.textColor=[UIColor colorWithWhite:.65 alpha:1];[self.gallery addArrangedSubview:hint];
        for(NSDictionary *recipe in recipes){NSString *detail=[NSString stringWithFormat:@"%@ · %@",[recipe[@"journey"]integerValue]>=0?HPJourneyNames()[[recipe[@"journey"]integerValue]]:HPThemeNames()[[recipe[@"mode"]integerValue]],recipe[@"scope"]];HPModeCard *card=[self card:recipe[@"name"]detail:detail mode:[recipe[@"mode"]intValue]journey:[recipe[@"journey"]intValue]];card.recipeID=recipe[@"id"];[card addTarget:self action:@selector(playSaved:)forControlEvents:UIControlEventTouchUpInside];[card addGestureRecognizer:[[UILongPressGestureRecognizer alloc]initWithTarget:self action:@selector(holdSaved:)]];[self.gallery addArrangedSubview:card];}
    }
}
- (void)syncSliders{self.brightnessSlider.value=HPLightEngine.shared.brightness;self.motionSlider.value=HPLightEngine.shared.speed;self.depthSlider.value=HPLightEngine.shared.depth;}
- (void)viewWillAppear:(BOOL)animated{[super viewWillAppear:animated];[self reloadPreview];[self syncSliders];[self refresh];[self.previewTimer invalidate];self.previewTimer=[NSTimer timerWithTimeInterval:.2 target:self selector:@selector(refresh)userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:self.previewTimer forMode:NSRunLoopCommonModes];}
- (void)viewDidDisappear:(BOOL)animated{[super viewDidDisappear:animated];[self.previewTimer invalidate];self.previewTimer=nil;}
- (void)refresh{
    HPLightEngine *engine=HPLightEngine.shared;self.status.text=engine.status;[self.scopeButton setTitle:engine.scope forState:UIControlStateNormal];[self.pause setTitle:engine.mode>=0?@"Pause":@"Resume"forState:UIControlStateNormal];self.pause.enabled=!self.captureReader&&!HPIdentifying;self.navigationItem.rightBarButtonItem.enabled=!self.captureReader&&!HPIdentifying;self.save.enabled=engine.mode<0&&!engine.adjusting&&engine.lights.count>0&&!self.captureReader&&!HPIdentifying;self.save.alpha=self.save.enabled?1:.4;
    BOOL enabled=!self.captureReader&&!HPIdentifying;self.brightnessSlider.enabled=enabled;self.motionSlider.enabled=enabled;self.depthSlider.enabled=enabled;self.shuffleButton.enabled=enabled&&engine.lights.count>0;
    self.brightnessLabel.text=[NSString stringWithFormat:@"Brightness %.0f%%",engine.brightness*100];self.motionLabel.text=[NSString stringWithFormat:@"Motion %.1f×",HPMotionRate(engine.speed)];self.depthLabel.text=[NSString stringWithFormat:@"Colour depth %.0f%%",engine.depth*100];
    // Do not assign slider.value on the refresh timer; the user's thumb owns it.
    for(UIView *row in self.gallery.arrangedSubviews){NSArray *cards=[row isKindOfClass:UIStackView.class]?[(UIStackView *)row arrangedSubviews]:@[row];for(UIView *view in cards)if([view isKindOfClass:HPModeCard.class]){HPModeCard *card=(id)view;BOOL selected=self.category.selectedSegmentIndex==0?engine.journey<0&&card.tag==engine.lastMode:self.category.selectedSegmentIndex==1&&card.tag==engine.journey;card.layer.borderWidth=selected?2:.5;card.layer.borderColor=[UIColor colorWithWhite:1 alpha:selected?.68:.13].CGColor;card.accessibilityTraits=UIAccessibilityTraitButton|(selected?UIAccessibilityTraitSelected:0);}}
    [self.preview setNeedsDisplay];
}
- (void)selectTheme:(UIControl *)button{if(self.captureReader)return;[HPLightEngine.shared start:button.tag];[self refresh];}
- (void)selectJourney:(UIControl *)button{if(self.captureReader)return;[HPLightEngine.shared startJourney:button.tag];[self refresh];}
- (void)brightness:(UISlider *)slider{if(self.captureReader||HPIdentifying)return;[HPLightEngine.shared changeBrightness:slider.value];[self refresh];}
- (void)speed:(UISlider *)slider{if(self.captureReader||HPIdentifying)return;[HPLightEngine.shared changeMotion:slider.value];[self refresh];}
- (void)depth:(UISlider *)slider{if(self.captureReader||HPIdentifying)return;[HPLightEngine.shared changeDepth:slider.value];[self refresh];}
- (void)shuffleColours{if(self.captureReader||HPIdentifying)return;[HPLightEngine.shared shuffleColours];[self refresh];}
- (void)stop{[HPLightEngine.shared stop];[self refresh];}
- (void)toggle{if(HPLightEngine.shared.mode>=0)[self stop];else[HPLightEngine.shared resume];[self refresh];}
- (void)more{
    UIAlertController *sheet=[UIAlertController alertControllerWithTitle:@"Make this mood yours" message:@"Your changes fade in over a few seconds. Brightness and Colour depth also shape a paused look. Motion changes travel speed when running."preferredStyle:UIAlertControllerStyleActionSheet];
    [sheet addAction:[UIAlertAction actionWithTitle:@"Save moving mood"style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){[self promptSaveMood];}]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"Restore original colours"style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){[HPLightEngine.shared restoreColours];[self refresh];}]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"Map lights"style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){[self mapLights];}]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"Cancel"style:UIAlertActionStyleCancel handler:nil]];sheet.popoverPresentationController.barButtonItem=self.navigationItem.rightBarButtonItem;[self presentViewController:sheet animated:YES completion:nil];
}
- (BOOL)saveMovingMoodNamed:(NSString *)name error:(NSError **)error{
    NSDictionary *recipe=[HPStudioPresets validatedRecipe:[HPLightEngine.shared recipeNamed:name]error:error];if(!recipe)return NO;NSMutableArray *recipes=[HPSavedMoods()mutableCopy];[recipes addObject:recipe];HPStoreMoods(recipes);HPLightEngine.shared.status=[NSString stringWithFormat:@"Saved moving mood: %@",recipe[@"name"]];[self buildGallery];[self refresh];return YES;
}
- (void)promptSaveMood{
    UIAlertController *prompt=[UIAlertController alertControllerWithTitle:@"Save moving mood"message:@"Keep this palette, its variation, room, brightness and motion. It will keep moving when you play it."preferredStyle:UIAlertControllerStyleAlert];[prompt addTextFieldWithConfigurationHandler:^(UITextField *field){field.text=[HPLightEngine.shared moodName];field.placeholder=@"Mood name";field.keyboardAppearance=UIKeyboardAppearanceDark;}];[prompt addAction:[UIAlertAction actionWithTitle:@"Cancel"style:UIAlertActionStyleCancel handler:nil]];
    [prompt addAction:[UIAlertAction actionWithTitle:@"Save mood"style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){NSError *error=nil;if(![self saveMovingMoodNamed:prompt.textFields.firstObject.text error:&error])HPAlert(self,@"Could not save mood",error.localizedDescription);else{self.category.selectedSegmentIndex=2;[self buildGallery];}}]];[self presentViewController:prompt animated:YES completion:nil];
}
- (void)playSaved:(HPModeCard *)card{if(self.captureReader)return;for(NSDictionary *recipe in HPSavedMoods())if([recipe[@"id"]isEqual:card.recipeID]){NSError *error=nil;if(![HPLightEngine.shared applyRecipe:recipe error:&error])HPAlert(self,@"Could not play mood",error.localizedDescription);[self reloadPreview];[self syncSliders];[self refresh];break;}}
- (void)deleteMovingMood:(NSString *)identifier{NSMutableArray *recipes=[HPSavedMoods()mutableCopy];NSIndexSet *indexes=[recipes indexesOfObjectsPassingTest:^BOOL(NSDictionary *recipe,NSUInteger index,BOOL *stop){return [recipe[@"id"]isEqual:identifier];}];[recipes removeObjectsAtIndexes:indexes];HPStoreMoods(recipes);[self buildGallery];}
- (void)holdSaved:(UILongPressGestureRecognizer *)gesture{
    if(gesture.state!=UIGestureRecognizerStateBegan)return;HPModeCard *card=(id)gesture.view;NSDictionary *chosen=nil;for(NSDictionary *recipe in HPSavedMoods())if([recipe[@"id"]isEqual:card.recipeID])chosen=recipe;if(!chosen)return;
    UIAlertController *confirm=[UIAlertController alertControllerWithTitle:[NSString stringWithFormat:@"Delete %@?",chosen[@"name"]]message:@"The current lighting and your static scenes will stay as they are."preferredStyle:UIAlertControllerStyleAlert];[confirm addAction:[UIAlertAction actionWithTitle:@"Cancel"style:UIAlertActionStyleCancel handler:nil]];[confirm addAction:[UIAlertAction actionWithTitle:@"Delete mood"style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action){[self deleteMovingMood:card.recipeID];}]];[self presentViewController:confirm animated:YES completion:nil];
}
- (void)saveScene{
    if(HPLightEngine.shared.mode>=0||self.captureReader)return;
    UIAlertController *prompt=[UIAlertController alertControllerWithTitle:@"Save this paused look" message:@"Your lights will be read and saved as a new scene." preferredStyle:UIAlertControllerStyleAlert];
    [prompt addTextFieldWithConfigurationHandler:^(UITextField *field){field.placeholder=@"Scene name";field.text=[[HPLightEngine.shared moodName]stringByAppendingString:@" Glow"];field.keyboardAppearance=UIKeyboardAppearanceDark;field.clearButtonMode=UITextFieldViewModeWhileEditing;}];
    [prompt addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [prompt addAction:[UIAlertAction actionWithTitle:@"Save scene" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){
        NSString *name=prompt.textFields.firstObject.text;
        [self capturePausedSceneWithName:name completion:^(NSError *error){
            if(!self.view.window)return;
            UIAlertController *result=[UIAlertController alertControllerWithTitle:error?@"Could not save scene":@"Scene saved" message:error.localizedDescription?:@"Your paused look is available with your other scenes and shortcuts." preferredStyle:UIAlertControllerStyleAlert];
            [result addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];[self presentViewController:result animated:YES completion:nil];
        }];
    }]];[self presentViewController:prompt animated:YES completion:nil];
}
- (void)capturePausedSceneWithName:(NSString *)name completion:(void(^)(NSError *))completion{
    HPLightEngine *engine=HPLightEngine.shared;
    NSString *trimmed=[name stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if(!trimmed.length||trimmed.length>40){completion(HPReadError(@"Give the scene a name of 1 to 40 characters."));return;}
    if(engine.mode>=0||engine.adjusting||self.captureReader||HPIdentifying){completion(HPReadError(@"Pause the lighting before saving a scene."));return;}
    [engine reloadLights];if(!engine.lights.count){completion(HPReadError(@"No colour lights are available to save."));return;}
    NSUInteger token=engine.generation;HPSceneReadback *reader=[HPSceneReadback new];reader.lights=[engine.lights copy];self.captureReader=reader;
    reader.stillPaused=^BOOL{return engine.mode<0&&engine.generation==token;};
    reader.progress=^(NSUInteger read,NSUInteger total){engine.status=[NSString stringWithFormat:@"Reading your paused look · %lu/%lu",(unsigned long)read,(unsigned long)total];};
    __weak HPStudio *weakSelf=self;
    reader.completion=^(NSArray *readings,NSError *error){
        HPStudio *controller=weakSelf;
        if(!error&&(engine.mode>=0||engine.generation!=token))error=HPReadError(@"The lighting changed while saving. Pause and try again.");
        if(!error)@try{
            NSDictionary *scene=[HPSceneCapture sceneWithName:trimmed readings:readings error:&error];
            if(scene&&[HPSceneCapture writeScene:scene realm:HPRealm() error:&error]){
                for(NSDictionary *reading in readings){engine.sent[reading[@"id"]]=@[reading[@"light"],reading[@"hue"],reading[@"sat"]];HPRememberColour(reading,(HPWireColor){[reading[@"hue"]unsignedShortValue],[reading[@"sat"]unsignedShortValue],[reading[@"light"]unsignedShortValue]});}
                engine.status=[NSString stringWithFormat:@"Saved %@ · %lu lights",trimmed,(unsigned long)readings.count];
                [NSNotificationCenter.defaultCenter postNotificationName:@"HP.OrderChanged" object:nil];
            }
        }@catch(NSException *exception){error=HPReadError(exception.reason?:@"The scene could not be saved.");}
        if(error)engine.status=error.localizedDescription;controller.captureReader=nil;[controller refresh];completion(error);
    };
    double wait=MAX(0,engine.settleUntil-NSProcessInfo.processInfo.systemUptime);
    engine.status=wait>0?@"Letting the fade settle before reading":@"Reading your paused look";[self refresh];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(wait*NSEC_PER_SEC)),dispatch_get_main_queue(),^{[reader next];});
}
- (void)mapLights{[self.navigationController pushViewController:[HPSpatialEditor new] animated:YES];}
- (void)chooseRoom {
    UIAlertController *sheet=[UIAlertController alertControllerWithTitle:@"Where should the light move?" message:nil preferredStyle:UIAlertControllerStyleActionSheet];
    NSMutableArray *names=[NSMutableArray arrayWithObject:@"All lights"];for(NSDictionary *group in HPMesh()[@"groups"])[names addObject:group[@"name"]];
    for(NSString *name in names)[sheet addAction:[UIAlertAction actionWithTitle:name style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){
        HPLightEngine *engine=HPLightEngine.shared;BOOL running=engine.mode>=0;[engine stop];engine.scope=name;if(running)[engine resume];[self reloadPreview];[self refresh];
    }]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    sheet.popoverPresentationController.sourceView=self.scopeButton;sheet.popoverPresentationController.sourceRect=self.scopeButton.bounds;[self presentViewController:sheet animated:YES completion:nil];
}
@end
