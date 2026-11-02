// Included after the native scene/database helpers in HomePlus.m.
#import "EffectPatterns.h"
#import "LightReadback.inc"
#import <unistd.h>

static NSArray *HPThemeNames(void){return @[@"Aurora",@"Comet",@"Ocean",@"Embers",@"Neon",@"Prism",@"Breathe",@"Velvet",@"Sunset",@"Moonlight",@"Fireflies",@"Tide",@"Silk",@"Rain",@"Dawn",@"Orbit",@"Forest Mist",@"Lagoon",@"Heather",@"Opal",@"Blue Hour",@"Coast",@"Rose Dusk",@"Polar Mist"];}
static NSArray *HPThemeDetails(void){return @[@"Slow jade and violet ribbons",@"A soft glow drifting through home",@"Long, quiet teal swells",@"A steady, living amber glow",@"Soft rose and turquoise drift",@"A slow journey through colour",@"Gentle, warm breathing light",@"Quiet plum and indigo",@"Honey and peach across home",@"Soft silver-blue moonlight",@"Warm pools wandering nearby",@"Quiet rings through your space",@"Rose and lilac ribbons",@"Cool light drifting downward",@"A slow wash of warm morning",@"Colour circling the apartment",@"Sage and eucalyptus in the mist",@"Still turquoise, deepening blue",@"Lilac hills under a quiet sky",@"Pearl, sea glass and lavender",@"The sky just after sunset",@"Pale blue meeting soft green",@"Dusty rose fading into mauve",@"Jade and ice-blue veils"];}
static NSArray *HPThemeOrder(void){return @[@0,@16,@17,@18,@19,@20,@21,@22,@23,@2,@8,@9,@11,@12,@13,@14,@3,@6,@7,@1,@4,@5,@10,@15];}
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
@property(nonatomic) double updatedAt,retryAt;
@property(nonatomic) BOOL known,reading;
@end
@implementation HPBulbMotion @end

@interface HPLightEngine : NSObject
@property(nonatomic,strong) NSTimer *timer;
@property(nonatomic,strong) NSArray *lights;
@property(nonatomic,strong) NSMutableDictionary *sent,*states;
@property(nonatomic,strong) id assertion;
@property(nonatomic) NSInteger mode,lastMode,cursor;
@property(nonatomic) NSUInteger generation;
@property(nonatomic) float brightness,speed;
@property(nonatomic) double lastSent,phase,lastTick,lastConnectAttempt,settleUntil;
@property(nonatomic) BOOL oldIdle,backgroundReady,connecting;
@property(nonatomic,strong) NSString *scope,*status;
+ (instancetype)shared;
- (void)start:(NSInteger)mode;
- (void)stop;
- (void)tick;
@end
@implementation HPLightEngine
+ (instancetype)shared {static HPLightEngine *engine;static dispatch_once_t once;dispatch_once(&once,^{
    engine=[HPLightEngine new];engine.mode=-1;engine.lastMode=0;
    NSUserDefaults *prefs=NSUserDefaults.standardUserDefaults;
    engine.brightness=[prefs objectForKey:@"HP.StudioBrightness"]?[prefs floatForKey:@"HP.StudioBrightness"]:.6;
    engine.speed=[prefs objectForKey:@"HP.StudioMotion"]?HPClamp([prefs floatForKey:@"HP.StudioMotion"],.15,1):.5;
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
- (void)start:(NSInteger)mode {
    if(mode<0||mode>=HP_MODE_COUNT)return;
    if(HPIdentifying){self.status=@"Finishing the light identification pulse";return;}
    [self reloadLights];if(!self.lights.count){[self stop];self.status=@"No colour lights in this room";return;}
    self.mode=mode;self.lastMode=mode;
    // A mood change reuses each lamp's current drift state; no colour reset.
    if(self.timer){self.status=[HPThemeNames()[mode]stringByAppendingString:@" · gently changing mood"];return;}
    self.generation++;self.lastTick=NSProcessInfo.processInfo.systemUptime;self.phase=0;self.connecting=NO;self.cursor=0;
    self.sent=[NSMutableDictionary new];self.states=[NSMutableDictionary new];
    self.oldIdle=UIApplication.sharedApplication.idleTimerDisabled;UIApplication.sharedApplication.idleTimerDisabled=YES;
    dlopen("/System/Library/PrivateFrameworks/AssertionServices.framework/AssertionServices",RTLD_NOW);
    Class cls=NSClassFromString(@"BKSProcessAssertion");SEL init=NSSelectorFromString(@"initWithPID:flags:reason:name:withHandler:");
    if([cls instancesRespondToSelector:init]){
        id object=((id(*)(id,SEL))objc_msgSend)(cls,@selector(alloc));
        self.assertion=((id(*)(id,SEL,int,unsigned int,unsigned int,id,id))objc_msgSend)(object,init,getpid(),3,7,@"Slow light drift",nil);
        SEL valid=NSSelectorFromString(@"isValid");self.backgroundReady=[self.assertion respondsToSelector:valid]&&((BOOL(*)(id,SEL))objc_msgSend)(self.assertion,valid);
    }
    self.status=@"Reading the starting colours";
    self.timer=[NSTimer timerWithTimeInterval:.10 target:self selector:@selector(tick) userInfo:nil repeats:YES];
    [[NSRunLoop mainRunLoop]addTimer:self.timer forMode:NSRunLoopCommonModes];[self tick];
}
- (void)stop {
    BOOL was=self.timer!=nil;[self.timer invalidate];self.timer=nil;self.mode=-1;self.generation++;
    if(self.assertion){HPVoid(self.assertion,@"invalidate");self.assertion=nil;}
    if(was)UIApplication.sharedApplication.idleTimerDisabled=self.oldIdle;
    self.status=@"Paused · save this look as a scene";
}
- (void)tick {
    if(self.mode<0||!self.lights.count)return;
    double now=NSProcessInfo.processInfo.systemUptime,elapsed=HPClamp(now-self.lastTick,0,.25);self.lastTick=now;
    if(!HPTransportOpen()){
        self.status=@"Reconnecting to your lights";
        if(!self.connecting&&now-self.lastConnectAttempt>8){self.connecting=YES;self.lastConnectAttempt=now;
            id bearer=HPGet(NSClassFromString(@"SigBearer"),@"share");SEL connect=NSSelectorFromString(@"startMeshConnectWithComplete:");
            if([bearer respondsToSelector:connect])((void(*)(id,SEL,id))objc_msgSend)(bearer,connect,^(BOOL ok){dispatch_async(dispatch_get_main_queue(),^{self.connecting=NO;});});else self.connecting=NO;
        }return;
    }
    self.phase+=elapsed*self.speed;
    id peripheral=HPGet(HPGet(NSClassFromString(@"SigBearer"),@"share"),@"getCurrentPeripheral");SEL ready=NSSelectorFromString(@"canSendWriteWithoutResponse");
    if([peripheral respondsToSelector:ready]&&!((BOOL(*)(id,SEL))objc_msgSend)(peripheral,ready))return;
    if(HPTransportBusy())return;
    NSUInteger index=(NSUInteger)self.cursor++%self.lights.count;NSDictionary *light=self.lights[index];NSString *key=light[@"id"];
    HPBulbMotion *state=self.states[key];if(!state){state=[HPBulbMotion new];self.states[key]=state;}
    if(!state.known){
        if(state.reading||now<state.retryAt)return;
        state.reading=YES;NSUInteger token=self.generation;
        HPReadColour(light,^(NSDictionary *reading,NSError *error){
            if(token!=self.generation)return;state.reading=NO;
            if(!reading){state.retryAt=NSProcessInfo.processInfo.systemUptime+8;self.status=@"Waiting for a light's current colour";return;}
            state.color=HPStatusColor([reading[@"hue"]unsignedShortValue],[reading[@"sat"]unsignedShortValue],[reading[@"light"]unsignedShortValue]);state.known=YES;state.updatedAt=NSProcessInfo.processInfo.systemUptime;
            self.sent[key]=@[reading[@"light"],reading[@"hue"],reading[@"sat"]];
        });return;
    }
    HPColor target=HPPattern2D((int)self.mode,self.phase,[light[@"x"]doubleValue],[light[@"y"]doubleValue],self.brightness,1);
    state.color=HPDrift(state.color,target,now-state.updatedAt);state.updatedAt=now;
    HPWireColor wire=HPVendorColor(state.color);NSArray *previous=self.sent[key];
    if(previous&&[previous[0]unsignedShortValue]==wire.lightness&&[previous[1]unsignedShortValue]==wire.hue&&[previous[2]unsignedShortValue]==wire.saturation)return;
    uint8_t steps=HPSmoothTransitionSteps((unsigned)self.lights.count);
    id transition=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"SigTransitionTime"),@selector(alloc));
    transition=((id(*)(id,SEL,uint8_t))objc_msgSend)(transition,NSSelectorFromString(@"initWithRawValue:"),steps);
    SEL set=NSSelectorFromString(@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:");Class sdk=NSClassFromString(@"SDKLibCommand");
    @try{
        if(![sdk respondsToSelector:set]){[self stop];self.status=@"The colour backend is unavailable";return;}
        ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,set,[light[@"address"]unsignedShortValue],wire.lightness,wire.hue,wire.saturation,transition,0,0,0,NO,nil,nil);
        self.sent[key]=@[@(wire.lightness),@(wire.hue),@(wire.saturation)];self.lastSent=now;self.settleUntil=now+steps*.1;
        self.status=[NSString stringWithFormat:@"%@ · slow drift · %lu lights",HPThemeNames()[self.mode],(unsigned long)self.lights.count];
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

@interface HPStudioPreview : UIView
@property(nonatomic) NSInteger mode;
@property(nonatomic) double seconds;
@property(nonatomic) float brightness,speed;
@end
@implementation HPStudioPreview
- (void)drawRect:(CGRect)rect {
    CGContextRef context=UIGraphicsGetCurrentContext();
    NSArray *lights=HPLightRecords();
    for(int n=0;n<14;n++){
        NSDictionary *light=lights.count?lights[n%lights.count]:nil;
        HPColor c=HPPattern2D((int)MAX(0,self.mode),self.seconds,light?[light[@"x"]doubleValue]:n/14.,light?[light[@"y"]doubleValue]:.5,self.brightness,self.speed);
        UIColor *color=[UIColor colorWithHue:c.hue saturation:c.saturation brightness:.32+.68*c.lightness alpha:1];
        CGFloat x=(n+.5)*rect.size.width/14,y=rect.size.height*.5+sinf(n*.4+self.seconds*.35)*14;
        CGContextSetShadowWithColor(context,CGSizeZero,18,color.CGColor);CGContextSetFillColorWithColor(context,color.CGColor);
        CGContextFillEllipseInRect(context,CGRectMake(x-7,y-7,14,14));
    }
}
@end

@interface HPModeCard : UIButton
@property(nonatomic,strong) CAGradientLayer *palette;
@end
@implementation HPModeCard
- (void)layoutSubviews{[super layoutSubviews];self.palette.frame=self.bounds;}
@end

@interface HPStudio : UIViewController
@property(nonatomic,strong) UILabel *status;
@property(nonatomic,strong) HPStudioPreview *preview;
@property(nonatomic,strong) NSTimer *previewTimer;
@property(nonatomic,strong) UIButton *scopeButton,*pause,*save;
@property(nonatomic,strong) HPSceneReadback *captureReader;
- (void)capturePausedSceneWithName:(NSString *)name completion:(void(^)(NSError *))completion;
@end
@implementation HPStudio
- (UILabel *)text:(NSString *)text size:(CGFloat)size {
    UILabel *label=[UILabel new];label.text=text;label.font=[UIFont systemFontOfSize:size weight:UIFontWeightSemibold];label.textColor=UIColor.whiteColor;label.numberOfLines=0;return label;
}
- (void)viewDidLoad {
    [super viewDidLoad];self.title=@"Light Studio";self.view.backgroundColor=[UIColor colorWithRed:.035 green:.045 blue:.07 alpha:1];
    self.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    UINavigationBarAppearance *appearance=[UINavigationBarAppearance new];[appearance configureWithOpaqueBackground];appearance.backgroundColor=self.view.backgroundColor;appearance.titleTextAttributes=@{NSForegroundColorAttributeName:UIColor.whiteColor};
    self.navigationItem.standardAppearance=appearance;self.navigationItem.scrollEdgeAppearance=appearance;self.navigationItem.compactAppearance=appearance;
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc]initWithTitle:@"Pause" style:UIBarButtonItemStylePlain target:self action:@selector(stop)];self.navigationItem.rightBarButtonItem.tintColor=[UIColor colorWithRed:.76 green:.67 blue:.98 alpha:1];
    UIScrollView *scroll=[UIScrollView new];scroll.translatesAutoresizingMaskIntoConstraints=NO;[self.view addSubview:scroll];
    UIStackView *stack=[UIStackView new];stack.axis=UILayoutConstraintAxisVertical;stack.spacing=18;stack.translatesAutoresizingMaskIntoConstraints=NO;[scroll addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[[scroll.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],[scroll.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],[scroll.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],[scroll.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],[stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:22],[stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-30],[stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:22],[stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-22],[stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-44]]];
    [stack addArrangedSubview:[self text:@"Let the room exhale." size:28]];
    UILabel *subtitle=[self text:@"Slow colour, soft light. Choose a mood and settle in." size:14];subtitle.textColor=[UIColor colorWithWhite:.65 alpha:1];[stack addArrangedSubview:subtitle];
    self.preview=[HPStudioPreview new];self.preview.backgroundColor=UIColor.clearColor;[self.preview.heightAnchor constraintEqualToConstant:90].active=YES;[stack addArrangedSubview:self.preview];
    self.scopeButton=[UIButton buttonWithType:UIButtonTypeSystem];self.scopeButton.contentHorizontalAlignment=UIControlContentHorizontalAlignmentLeft;[self.scopeButton addTarget:self action:@selector(chooseRoom) forControlEvents:UIControlEventTouchUpInside];[stack addArrangedSubview:self.scopeButton];
    [stack addArrangedSubview:[self text:@"Brightness" size:13]];
    UISlider *brightness=[UISlider new];brightness.minimumValue=.08;brightness.maximumValue=1;brightness.value=HPLightEngine.shared.brightness;[brightness addTarget:self action:@selector(brightness:) forControlEvents:UIControlEventValueChanged];[stack addArrangedSubview:brightness];
    [stack addArrangedSubview:[self text:@"Motion" size:13]];
    UISlider *speed=[UISlider new];speed.minimumValue=.15;speed.maximumValue=1;speed.value=HPLightEngine.shared.speed;[speed addTarget:self action:@selector(speed:) forControlEvents:UIControlEventValueChanged];[stack addArrangedSubview:speed];
    self.pause=[UIButton buttonWithType:UIButtonTypeSystem];self.pause.backgroundColor=[UIColor colorWithRed:.76 green:.67 blue:.98 alpha:1];self.pause.layer.cornerRadius=15;[self.pause.heightAnchor constraintEqualToConstant:48].active=YES;[self.pause setTitleColor:UIColor.blackColor forState:UIControlStateNormal];[self.pause addTarget:self action:@selector(stop) forControlEvents:UIControlEventTouchUpInside];[stack addArrangedSubview:self.pause];
    self.save=[UIButton buttonWithType:UIButtonTypeSystem];self.save.backgroundColor=[UIColor colorWithRed:.13 green:.15 blue:.21 alpha:1];self.save.layer.cornerRadius=15;[self.save.heightAnchor constraintEqualToConstant:48].active=YES;[self.save setTitle:@"Save as scene" forState:UIControlStateNormal];[self.save setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];[self.save addTarget:self action:@selector(saveScene) forControlEvents:UIControlEventTouchUpInside];[stack addArrangedSubview:self.save];
    [HPLightEngine.shared reloadLights];
    self.status=[self text:@"" size:12];self.status.textColor=[UIColor colorWithWhite:.65 alpha:1];[stack addArrangedSubview:self.status];
    UILabel *hint=[self text:@"Pause a look you love, then save it as a scene. Saving reads the actual lights after their fade settles." size:12];hint.textColor=[UIColor colorWithWhite:.5 alpha:1];[stack addArrangedSubview:hint];
    [stack addArrangedSubview:[self text:@"Slow moods for your apartment" size:15]];
    UIButton *map=[UIButton buttonWithType:UIButtonTypeSystem];[map setTitle:@"Map light travel path" forState:UIControlStateNormal];map.contentHorizontalAlignment=UIControlContentHorizontalAlignmentLeft;[map addTarget:self action:@selector(mapLights) forControlEvents:UIControlEventTouchUpInside];[stack addArrangedSubview:map];
    for(int row=0;row<(HP_MODE_COUNT+1)/2;row++){
        UIStackView *pair=[UIStackView new];pair.axis=UILayoutConstraintAxisHorizontal;pair.spacing=12;pair.distribution=UIStackViewDistributionFillEqually;
        for(int col=0;col<2;col++){
            int mode=[HPThemeOrder()[row*2+col]intValue];HPModeCard *button=[[HPModeCard alloc]initWithFrame:CGRectZero];button.tag=mode;button.accessibilityIdentifier=[@"HP.Mood."stringByAppendingString:HPThemeNames()[mode]];
            NSMutableArray *colors=[NSMutableArray new];for(int swatch=0;swatch<3;swatch++){HPColor sample=HPPattern2D(mode,45,swatch*.5,.35,.8,1);[colors addObject:(id)[UIColor colorWithHue:sample.hue saturation:sample.saturation brightness:.20+.045*swatch alpha:1].CGColor];}
            button.palette=[CAGradientLayer layer];button.palette.colors=colors;button.palette.startPoint=CGPointMake(0,0);button.palette.endPoint=CGPointMake(1,1);[button.layer insertSublayer:button.palette atIndex:0];
            button.layer.cornerRadius=18;button.layer.masksToBounds=YES;button.layer.borderWidth=.5;button.layer.borderColor=[UIColor colorWithWhite:1 alpha:.13].CGColor;[button.heightAnchor constraintEqualToConstant:100].active=YES;
            NSMutableAttributedString *title=[[NSMutableAttributedString alloc]initWithString:[NSString stringWithFormat:@"%@\n%@",HPThemeNames()[mode],HPThemeDetails()[mode]] attributes:@{NSForegroundColorAttributeName:UIColor.whiteColor,NSFontAttributeName:[UIFont systemFontOfSize:12 weight:UIFontWeightRegular]}];
            [title addAttribute:NSFontAttributeName value:[UIFont systemFontOfSize:18 weight:UIFontWeightSemibold] range:NSMakeRange(0,[HPThemeNames()[mode] length])];
            [button setAttributedTitle:title forState:UIControlStateNormal];button.titleLabel.numberOfLines=3;button.titleLabel.textAlignment=NSTextAlignmentCenter;button.contentEdgeInsets=UIEdgeInsetsMake(10,9,10,9);
            [button addTarget:self action:@selector(selectTheme:) forControlEvents:UIControlEventTouchUpInside];[pair addArrangedSubview:button];
        }[stack addArrangedSubview:pair];
    }

}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];[self refresh];
    self.previewTimer=[NSTimer timerWithTimeInterval:.05 target:self selector:@selector(refresh) userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:self.previewTimer forMode:NSRunLoopCommonModes];
}
- (void)viewDidDisappear:(BOOL)animated{[super viewDidDisappear:animated];[self.previewTimer invalidate];self.previewTimer=nil;}
- (void)refresh {
    HPLightEngine *engine=HPLightEngine.shared;self.status.text=engine.status;[self.scopeButton setTitle:[@"Room: " stringByAppendingString:engine.scope] forState:UIControlStateNormal];
    [self.pause setTitle:engine.mode>=0?@"Pause lighting":@"Lighting paused" forState:UIControlStateNormal];self.pause.enabled=engine.mode>=0;
    self.navigationItem.rightBarButtonItem.enabled=engine.mode>=0;
    self.save.enabled=engine.mode<0&&engine.lights.count>0&&!self.captureReader&&!HPIdentifying;
    self.preview.mode=engine.mode>=0?engine.mode:engine.lastMode;self.preview.seconds=engine.phase;self.preview.brightness=engine.brightness;self.preview.speed=1;[self.preview setNeedsDisplay];
}
- (void)selectTheme:(UIButton *)button{[HPLightEngine.shared start:button.tag];[self refresh];}
- (void)brightness:(UISlider *)slider{HPLightEngine.shared.brightness=slider.value;[NSUserDefaults.standardUserDefaults setFloat:slider.value forKey:@"HP.StudioBrightness"];}
- (void)speed:(UISlider *)slider{HPLightEngine.shared.speed=slider.value;[NSUserDefaults.standardUserDefaults setFloat:slider.value forKey:@"HP.StudioMotion"];}
- (void)stop{[HPLightEngine.shared stop];[self refresh];}
- (void)saveScene{
    if(HPLightEngine.shared.mode>=0||self.captureReader)return;
    UIAlertController *prompt=[UIAlertController alertControllerWithTitle:@"Save this paused look" message:@"Your lights will be read and saved as a new scene." preferredStyle:UIAlertControllerStyleAlert];
    [prompt addTextFieldWithConfigurationHandler:^(UITextField *field){field.placeholder=@"Scene name";field.text=[HPThemeNames()[HPLightEngine.shared.lastMode]stringByAppendingString:@" Glow"];field.keyboardAppearance=UIKeyboardAppearanceDark;field.clearButtonMode=UITextFieldViewModeWhileEditing;}];
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
    if(engine.mode>=0||self.captureReader||HPIdentifying){completion(HPReadError(@"Pause the lighting before saving a scene."));return;}
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
        HPLightEngine *engine=HPLightEngine.shared;engine.scope=name;if(engine.mode>=0)[engine start:engine.mode];[self refresh];
    }]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    sheet.popoverPresentationController.sourceView=self.scopeButton;sheet.popoverPresentationController.sourceRect=self.scopeButton.bounds;[self presentViewController:sheet animated:YES completion:nil];
}
@end
