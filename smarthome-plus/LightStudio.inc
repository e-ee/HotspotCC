// Included after the native scene/database helpers in HomePlus.m.
#import "EffectPatterns.h"
#import <unistd.h>

static NSArray *HPThemeNames(void){return @[@"Aurora",@"Comet",@"Ocean",@"Embers",@"Neon",@"Prism",@"Breathe",@"Velvet"];}
static NSArray *HPThemeDetails(void){return @[@"Jade and violet ribbons",@"A light travelling through home",@"Teal swells and silver foam",@"A living amber glow",@"Pink and cyan in motion",@"Colour flowing from lamp to lamp",@"Slow, warm waves of light",@"Plum, indigo and moving gold"];}
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
    return HPOrdered(records,@"HP.LightOrder",@"id");
}
static NSDictionary *HPMesh(void){NSData *data=[NSUserDefaults.standardUserDefaults dataForKey:@"mesh.json"];return data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;}

@interface HPLightEngine : NSObject
@property(nonatomic,strong) NSTimer *timer;
@property(nonatomic,strong) NSArray *lights;
@property(nonatomic,strong) NSMutableDictionary *sent;
@property(nonatomic,strong) id assertion;
@property(nonatomic) NSInteger mode,cursor;
@property(nonatomic) float brightness,speed;
@property(nonatomic) double began,lastSent,phase,lastTick;
@property(nonatomic) BOOL oldIdle,backgroundReady,connecting;
@property(nonatomic,strong) NSString *scope,*status;
+ (instancetype)shared;
- (void)start:(NSInteger)mode;
- (void)stop;
- (void)tick;
@end
@implementation HPLightEngine
+ (instancetype)shared {static HPLightEngine *engine;static dispatch_once_t once;dispatch_once(&once,^{
    engine=[HPLightEngine new];engine.mode=-1;engine.brightness=.6;engine.speed=1;engine.scope=@"All lights";engine.status=@"Choose a mood";
});return engine;}
- (void)reloadLights {
    NSArray *all=HPLightRecords();if([self.scope isEqual:@"All lights"]){self.lights=all;return;}
    NSDictionary *mesh=HPMesh();NSString *groupAddress;
    for(NSDictionary *group in mesh[@"groups"])if([group[@"name"] isEqual:self.scope]){groupAddress=[group[@"address"] uppercaseString];break;}
    NSMutableSet *members=[NSMutableSet new];
    for(NSDictionary *node in mesh[@"nodes"])for(NSDictionary *element in node[@"elements"])for(NSDictionary *model in element[@"models"])
        for(id subscription in model[@"subscribe"])if([subscription isKindOfClass:NSString.class]&&[[subscription uppercaseString]isEqual:groupAddress])
            [members addObject:node[@"UUID"]];
    NSMutableArray *selected=[NSMutableArray new];for(NSDictionary *light in all)if([members containsObject:light[@"id"]])[selected addObject:light];self.lights=selected;
}
- (void)start:(NSInteger)mode {
    if(mode<0||mode>=8)return;
    [self stop];[self reloadLights];if(!self.lights.count){self.status=@"No colour lights in this room";return;}
    self.mode=mode;self.began=NSProcessInfo.processInfo.systemUptime;self.lastTick=self.began;self.phase=0;self.connecting=NO;self.cursor=0;self.lastSent=0;self.sent=[NSMutableDictionary new];
    self.oldIdle=UIApplication.sharedApplication.idleTimerDisabled;UIApplication.sharedApplication.idleTimerDisabled=YES;
    // A scoped assertion keeps effects alive when supported by this TrollStore environment.
    dlopen("/System/Library/PrivateFrameworks/AssertionServices.framework/AssertionServices",RTLD_NOW);
    Class cls=NSClassFromString(@"BKSProcessAssertion");SEL init=NSSelectorFromString(@"initWithPID:flags:reason:name:withHandler:");
    if([cls instancesRespondToSelector:init]){
        id object=((id(*)(id,SEL))objc_msgSend)(cls,@selector(alloc));
        self.assertion=((id(*)(id,SEL,int,unsigned int,unsigned int,id,id))objc_msgSend)(object,init,getpid(),3,7,@"SYLVANIA light effects",nil);
        SEL valid=NSSelectorFromString(@"isValid");self.backgroundReady=[self.assertion respondsToSelector:valid]&&((BOOL(*)(id,SEL))objc_msgSend)(self.assertion,valid);
    }
    self.status=@"Starting light motion";
    self.timer=[NSTimer timerWithTimeInterval:.08 target:self selector:@selector(tick) userInfo:nil repeats:YES];
    [[NSRunLoop mainRunLoop]addTimer:self.timer forMode:NSRunLoopCommonModes];[self tick];
}
- (void)stop {
    BOOL was=self.timer!=nil;[self.timer invalidate];self.timer=nil;self.mode=-1;
    if(self.assertion){HPVoid(self.assertion,@"invalidate");self.assertion=nil;}
    if(was)UIApplication.sharedApplication.idleTimerDisabled=self.oldIdle;
    self.status=@"Paused · lights hold their last colours";
}
- (void)tick {
    if(self.mode<0||!self.lights.count)return;
    double now=NSProcessInfo.processInfo.systemUptime;
    self.phase+=MAX(0,now-self.lastTick)*self.speed;self.lastTick=now;
    id bearer=HPGet(NSClassFromString(@"SigBearer"),@"share");
    SEL isOpen=NSSelectorFromString(@"isOpen");
    if(![bearer respondsToSelector:isOpen]||!((BOOL(*)(id,SEL))objc_msgSend)(bearer,isOpen)){
        self.status=@"Reconnecting to your lights";
        if(!self.connecting&&now-self.lastSent>8){self.connecting=YES;self.lastSent=now;
            SEL connect=NSSelectorFromString(@"startMeshConnectWithComplete:");
            if([bearer respondsToSelector:connect])((void(*)(id,SEL,id))objc_msgSend)(bearer,connect,^(BOOL ok){dispatch_async(dispatch_get_main_queue(),^{self.connecting=NO;});});
            else self.connecting=NO;
        }return;
    }
    id peripheral=HPGet(bearer,@"getCurrentPeripheral");SEL ready=NSSelectorFromString(@"canSendWriteWithoutResponse");
    if([peripheral respondsToSelector:ready]&&!((BOOL(*)(id,SEL))objc_msgSend)(peripheral,ready))return;
    if([HPGet(HPGet(NSClassFromString(@"SigMeshLib"),@"share"),@"commands") count])return;
    // One queued command at a time. Each lamp performs the interpolation locally.
    NSUInteger index=(NSUInteger)self.cursor++%self.lights.count;NSDictionary *light=self.lights[index];
    HPColor color=HPPattern((int)self.mode,self.phase,index/(double)self.lights.count,self.brightness,1);
    uint16_t brightness=HPWord(color.lightness),hue=HPWord(color.hue),saturation=HPWord(color.saturation);
    NSArray *previous=self.sent[light[@"id"]];
    if(previous&&abs([previous[0] intValue]-brightness)<120&&abs([previous[1] intValue]-hue)<120&&abs([previous[2] intValue]-saturation)<120)return;
    Class transitionClass=NSClassFromString(@"SigTransitionTime");id transition=((id(*)(id,SEL))objc_msgSend)(transitionClass,@selector(alloc));
    // Hundred-millisecond steps. Cover most of a full update round with a smooth fade.
    uint8_t steps=(uint8_t)fmin(20,fmax(2,ceil(self.lights.count*.08*8)));
    transition=((id(*)(id,SEL,uint8_t))objc_msgSend)(transition,NSSelectorFromString(@"initWithRawValue:"),steps);
    SEL set=NSSelectorFromString(@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:");
    Class sdk=NSClassFromString(@"SDKLibCommand");
    @try {
        if(![sdk respondsToSelector:set]){[self stop];self.status=@"The colour backend is unavailable";return;}
        ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,set,[light[@"address"]unsignedShortValue],brightness,hue,saturation,transition,0,0,0,NO,nil,nil);
        self.sent[light[@"id"]]=@[@(brightness),@(hue),@(saturation)];self.lastSent=now;
        self.status=[NSString stringWithFormat:@"%@ · %lu lights · %@",HPThemeNames()[self.mode],(unsigned long)self.lights.count,self.scope];
    }@catch(NSException *exception){[self stop];self.status=@"Could not send the lighting effect";}
}
@end

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
    return self.scenes?@"Drag the handles to arrange the shortcuts across the top of Home. New scenes are added after your chosen order.":@"Drag lamps into the order you want colour to travel through your apartment. The Comet and wave effects follow this path.";
}
- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)index{return UITableViewCellEditingStyleNone;}
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
    for(int n=0;n<14;n++){
        HPColor c=HPPattern((int)MAX(0,self.mode),self.seconds,n/14.,self.brightness,self.speed);
        UIColor *color=[UIColor colorWithHue:c.hue saturation:c.saturation brightness:.32+.68*c.lightness alpha:1];
        CGFloat x=(n+.5)*rect.size.width/14,y=rect.size.height*.5+sinf(n*.4+self.seconds*.35)*14;
        CGContextSetShadowWithColor(context,CGSizeZero,18,color.CGColor);CGContextSetFillColorWithColor(context,color.CGColor);
        CGContextFillEllipseInRect(context,CGRectMake(x-7,y-7,14,14));
    }
}
@end

@interface HPStudio : UIViewController
@property(nonatomic,strong) UILabel *status;
@property(nonatomic,strong) HPStudioPreview *preview;
@property(nonatomic,strong) NSTimer *previewTimer;
@property(nonatomic,strong) UIButton *scopeButton,*pause;
@end
@implementation HPStudio
- (UILabel *)text:(NSString *)text size:(CGFloat)size {
    UILabel *label=[UILabel new];label.text=text;label.font=[UIFont systemFontOfSize:size weight:UIFontWeightSemibold];label.textColor=UIColor.whiteColor;label.numberOfLines=0;return label;
}
- (void)viewDidLoad {
    [super viewDidLoad];self.title=@"Light Studio";self.view.backgroundColor=[UIColor colorWithRed:.035 green:.045 blue:.07 alpha:1];
    UIScrollView *scroll=[UIScrollView new];scroll.translatesAutoresizingMaskIntoConstraints=NO;[self.view addSubview:scroll];
    UIStackView *stack=[UIStackView new];stack.axis=UILayoutConstraintAxisVertical;stack.spacing=18;stack.translatesAutoresizingMaskIntoConstraints=NO;[scroll addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[[scroll.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],[scroll.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],[scroll.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],[scroll.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],[stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:22],[stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-30],[stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:22],[stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-22],[stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-44]]];
    [stack addArrangedSubview:[self text:@"Let the apartment move." size:28]];
    UILabel *subtitle=[self text:@"Choose a mood. Colour travels through your lights." size:14];subtitle.textColor=[UIColor colorWithWhite:.65 alpha:1];[stack addArrangedSubview:subtitle];
    self.preview=[HPStudioPreview new];self.preview.backgroundColor=UIColor.clearColor;[self.preview.heightAnchor constraintEqualToConstant:90].active=YES;[stack addArrangedSubview:self.preview];
    self.scopeButton=[UIButton buttonWithType:UIButtonTypeSystem];self.scopeButton.contentHorizontalAlignment=UIControlContentHorizontalAlignmentLeft;[self.scopeButton addTarget:self action:@selector(chooseRoom) forControlEvents:UIControlEventTouchUpInside];[stack addArrangedSubview:self.scopeButton];
    for(int row=0;row<4;row++){
        UIStackView *pair=[UIStackView new];pair.axis=UILayoutConstraintAxisHorizontal;pair.spacing=12;pair.distribution=UIStackViewDistributionFillEqually;
        for(int col=0;col<2;col++){
            int mode=row*2+col;UIButton *button=[UIButton buttonWithType:UIButtonTypeSystem];button.tag=mode;
            HPColor sample=HPPattern(mode,4,.3,.8,1);button.backgroundColor=[UIColor colorWithHue:sample.hue saturation:.4 brightness:.23 alpha:1];
            button.layer.cornerRadius=18;[button.heightAnchor constraintEqualToConstant:91].active=YES;
            NSMutableAttributedString *title=[[NSMutableAttributedString alloc]initWithString:[NSString stringWithFormat:@"%@\n%@",HPThemeNames()[mode],HPThemeDetails()[mode]] attributes:@{NSForegroundColorAttributeName:UIColor.whiteColor,NSFontAttributeName:[UIFont systemFontOfSize:12 weight:UIFontWeightRegular]}];
            [title addAttribute:NSFontAttributeName value:[UIFont systemFontOfSize:18 weight:UIFontWeightSemibold] range:NSMakeRange(0,[HPThemeNames()[mode] length])];
            [button setAttributedTitle:title forState:UIControlStateNormal];button.titleLabel.numberOfLines=3;button.titleLabel.textAlignment=NSTextAlignmentCenter;button.contentEdgeInsets=UIEdgeInsetsMake(10,9,10,9);
            [button addTarget:self action:@selector(selectTheme:) forControlEvents:UIControlEventTouchUpInside];[pair addArrangedSubview:button];
        }[stack addArrangedSubview:pair];
    }
    [stack addArrangedSubview:[self text:@"Brightness" size:13]];
    UISlider *brightness=[UISlider new];brightness.minimumValue=.08;brightness.maximumValue=1;brightness.value=HPLightEngine.shared.brightness;[brightness addTarget:self action:@selector(brightness:) forControlEvents:UIControlEventValueChanged];[stack addArrangedSubview:brightness];
    [stack addArrangedSubview:[self text:@"Motion" size:13]];
    UISlider *speed=[UISlider new];speed.minimumValue=.2;speed.maximumValue=2;speed.value=HPLightEngine.shared.speed;[speed addTarget:self action:@selector(speed:) forControlEvents:UIControlEventValueChanged];[stack addArrangedSubview:speed];
    self.pause=[UIButton buttonWithType:UIButtonTypeSystem];self.pause.backgroundColor=[UIColor colorWithRed:.76 green:.67 blue:.98 alpha:1];self.pause.layer.cornerRadius=15;[self.pause.heightAnchor constraintEqualToConstant:48].active=YES;[self.pause setTitleColor:UIColor.blackColor forState:UIControlStateNormal];[self.pause addTarget:self action:@selector(stop) forControlEvents:UIControlEventTouchUpInside];[stack addArrangedSubview:self.pause];
    self.status=[self text:@"" size:12];self.status.textColor=[UIColor colorWithWhite:.65 alpha:1];[stack addArrangedSubview:self.status];
    UILabel *hint=[self text:@"A normal scene or light control pauses the effect. Pause leaves the current colours in place." size:12];hint.textColor=[UIColor colorWithWhite:.5 alpha:1];[stack addArrangedSubview:hint];
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];[self refresh];
    self.previewTimer=[NSTimer timerWithTimeInterval:.05 target:self selector:@selector(refresh) userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:self.previewTimer forMode:NSRunLoopCommonModes];
}
- (void)viewDidDisappear:(BOOL)animated{[super viewDidDisappear:animated];[self.previewTimer invalidate];self.previewTimer=nil;}
- (void)refresh {
    HPLightEngine *engine=HPLightEngine.shared;self.status.text=engine.status;[self.scopeButton setTitle:[@"Room: " stringByAppendingString:engine.scope] forState:UIControlStateNormal];
    [self.pause setTitle:engine.mode>=0?@"Pause lighting":@"Choose a mood above" forState:UIControlStateNormal];self.pause.enabled=engine.mode>=0;
    self.preview.mode=MAX(0,engine.mode);self.preview.seconds=engine.mode>=0?engine.phase:0;self.preview.brightness=engine.brightness;self.preview.speed=1;[self.preview setNeedsDisplay];
}
- (void)selectTheme:(UIButton *)button{[HPLightEngine.shared start:button.tag];[self refresh];}
- (void)brightness:(UISlider *)slider{HPLightEngine.shared.brightness=slider.value;}
- (void)speed:(UISlider *)slider{HPLightEngine.shared.speed=slider.value;}
- (void)stop{[HPLightEngine.shared stop];[self refresh];}
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
