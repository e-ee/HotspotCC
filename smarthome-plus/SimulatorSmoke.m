// Real UIKit and production add-on, with synthetic lamp and database endpoints.
// Real Realm storage is tested separately by test_capture_realm.m.
#import <UIKit/UIKit.h>
#import <objc/message.h>
#import "SceneCapture.h"
#import "EffectPatterns.h"
#import "StudioPresets.h"
#include <assert.h>
#include <math.h>
static NSMutableArray *Writes,*Reads,*BrightnessWrites;
static NSMutableDictionary *LampValues,*LastTargets;
static BOOL QueueBusy,DropReads,Disconnected;
static NSUInteger PendingReads,CancelledReads;
static NSMutableArray *MarkerWrites;
static NSUInteger ExplicitFadedWrites,AmbientWrites;
static uint16_t FailReadTarget;
static NSString *TestWait;
static void HPTestFailure(const char *condition,const char *file,int line) __attribute__((noreturn));
static void HPTestFailure(const char *condition,const char *file,int line){
    NSDictionary *failure=@{@"condition":@(condition),@"file":@(file),@"line":@(line),@"wait":TestWait?:@"",@"writes":@(Writes.count),@"reads":@(Reads.count),@"pendingReads":@(PendingReads),@"markers":MarkerWrites?:@[],@"firstLamp":LampValues[@0x20]?:@{}};
    NSString *path=[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:@"failure-report.json"];[[NSJSONSerialization dataWithJSONObject:failure options:NSJSONWritingPrettyPrinted error:nil]writeToFile:path atomically:YES];abort();
}
#undef assert
#define assert(condition) ((condition)?(void)0:HPTestFailure(#condition,__FILE__,__LINE__))
@interface HPMockLamp:NSObject
@property(nonatomic) uint16_t address;
@property(nonatomic,strong) NSString *name,*UUID;
- (NSArray *)HSLAddresses;
- (uint16_t)HSL_Hue;
- (uint16_t)HSL_Saturation;
- (uint16_t)HSL_Lightness;
@end
@implementation HPMockLamp
- (NSArray *)HSLAddresses{return @[@(self.address)];}
- (uint16_t)HSL_Hue{return (uint16_t)lround([LampValues[@(self.address)][@"hue"]doubleValue]/360.*65535);}
- (uint16_t)HSL_Saturation{return (uint16_t)lround([LampValues[@(self.address)][@"sat"]doubleValue]/100.*65535);}
- (uint16_t)HSL_Lightness{return (uint16_t)lround([LampValues[@(self.address)][@"light"]doubleValue]/100.*65535);}
@end
@interface SigDataSource:NSObject
+ (instancetype)share;
- (NSArray *)curNodes;
@end
@implementation SigDataSource
+ (instancetype)share{static SigDataSource *s;static dispatch_once_t once;dispatch_once(&once,^{s=[self new];});return s;}
- (NSArray *)curNodes{static NSMutableArray *lamps;static dispatch_once_t once;dispatch_once(&once,^{lamps=[NSMutableArray new];for(int i=0;i<14;i++){HPMockLamp *lamp=[HPMockLamp new];lamp.address=0x20+i;lamp.name=[NSString stringWithFormat:@"Lamp %d",i+1];lamp.UUID=[NSString stringWithFormat:@"mock-lamp-%d",i];[lamps addObject:lamp];}});return lamps;}
@end
@interface SigBearer:NSObject
+ (instancetype)share;
- (BOOL)isOpen;
- (id)getCurrentPeripheral;
@end
@implementation SigBearer
+ (instancetype)share{static SigBearer *s;static dispatch_once_t once;dispatch_once(&once,^{s=[self new];});return s;}
- (BOOL)isOpen{return !Disconnected;}
- (id)getCurrentPeripheral{return nil;}
@end
@interface SigMeshLib:NSObject
+ (instancetype)share;
- (NSArray *)commands;
@end
@implementation SigMeshLib
+ (instancetype)share{static SigMeshLib *s;static dispatch_once_t once;dispatch_once(&once,^{s=[self new];});return s;}
- (NSArray *)commands{return QueueBusy||PendingReads?@[@"pending"]:@[];}
@end
@interface SigTransitionTime:NSObject
@property(nonatomic) uint8_t raw;
- (instancetype)initWithRawValue:(uint8_t)raw;
@end
@implementation SigTransitionTime
- (instancetype)initWithRawValue:(uint8_t)raw{self=[super init];if(self)self.raw=raw;return self;}
@end
@interface HPMockStatus:NSObject
@property(nonatomic) uint16_t HSLLightness,HSLHue,HSLSaturation;
@end
@implementation HPMockStatus @end
static void InitLamps(void){if(LampValues)return;Writes=[NSMutableArray new];Reads=[NSMutableArray new];BrightnessWrites=[NSMutableArray new];MarkerWrites=[NSMutableArray new];LampValues=[NSMutableDictionary new];LastTargets=[NSMutableDictionary new];for(int i=0;i<14;i++){NSDictionary *value=@{@"hue":@(230+i),@"sat":@62,@"light":@35};LampValues[@(0x20+i)]=value;LastTargets[@(0x20+i)]=value;}}
@interface HPMockReadHandle:NSObject
@property(nonatomic) BOOL cancelled;
- (void)cancel;
@end
@implementation HPMockReadHandle
- (void)cancel{if(!self.cancelled){self.cancelled=YES;assert(PendingReads>0);PendingReads--;CancelledReads++;}}
@end
@interface SDKLibCommand:NSObject
+ (id)lightHSLGetWithDestination:(uint16_t)target retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses successCallback:(void(^)(uint16_t,uint16_t,id))success resultCallback:(void(^)(BOOL,NSError *))result;
+ (id)lightHSLSetWithDestination:(uint16_t)target HSLLight:(uint16_t)light HSLHue:(uint16_t)hue HSLSaturation:(uint16_t)saturation transitionTime:(id)transition delay:(uint8_t)delay retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result;
@end
@implementation SDKLibCommand
+ (id)lightLightnessSetWithDestination:(uint16_t)target lightness:(uint16_t)level transitionTime:(id)transition delay:(uint8_t)delay retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result{
    assert(!ack&&retries==0&&responses==0&&success==nil&&result==nil&&[transition raw]==6&&level<=100);NSMutableDictionary *value=[LampValues[@(target)]mutableCopy];assert(value);value[@"light"]=@(level);LampValues[@(target)]=value;[BrightnessWrites addObject:@{@"target":@(target),@"level":@(level)}];return nil;
}
+ (id)lightHSLGetWithDestination:(uint16_t)target retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses successCallback:(void(^)(uint16_t,uint16_t,id))success resultCallback:(void(^)(BOOL,NSError *))result{
    InitLamps();assert(retries==0&&responses==1);[Reads addObject:@(target)];if(DropReads){PendingReads++;return [HPMockReadHandle new];}
    if(target==FailReadTarget){result(NO,[NSError errorWithDomain:@"mock" code:1 userInfo:@{NSLocalizedDescriptionKey:@"A test light did not respond"}]);return nil;}
    NSDictionary *v=LampValues[@(target)];assert(v);HPMockStatus *status=[HPMockStatus new];status.HSLHue=[v[@"hue"]unsignedShortValue];status.HSLSaturation=[v[@"sat"]unsignedShortValue];status.HSLLightness=[v[@"light"]unsignedShortValue];success(target,0x7e00,status);result(YES,nil);return nil;
}
+ (id)lightHSLSetWithDestination:(uint16_t)target HSLLight:(uint16_t)light HSLHue:(uint16_t)hue HSLSaturation:(uint16_t)saturation transitionTime:(id)transition delay:(uint8_t)delay retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result{
    InitLamps();assert(!ack&&retries==0&&responses==0&&success==nil&&result==nil);assert(hue<=360&&saturation<=100&&light<=100);assert(([transition raw]>=40&&[transition raw]<=60)||[transition raw]==10);
    NSDictionary *previous=LastTargets[@(target)];assert(previous);
    id engine=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"HPLightEngine"),NSSelectorFromString(@"shared"));NSString *key=[NSString stringWithFormat:@"mock-lamp-%d",target-0x20];BOOL requested=[[engine valueForKey:@"retargets"]containsObject:key];
    if([transition raw]!=10){if(requested){assert([transition raw]>=40);ExplicitFadedWrites++;}else{assert(abs([previous[@"hue"]intValue]-hue)<=3&&abs([previous[@"sat"]intValue]-saturation)<=3&&abs([previous[@"light"]intValue]-light)<=2);AmbientWrites++;}}else[MarkerWrites addObject:@{@"target":@(target),@"hue":@(hue),@"sat":@(saturation),@"light":@(light)}];
    NSDictionary *value=@{@"hue":@(hue),@"sat":@(saturation),@"light":@(light)};LampValues[@(target)]=value;LastTargets[@(target)]=value;
    [Writes addObject:@{@"target":@(target),@"light":@(light),@"hue":@(hue),@"sat":@(saturation),@"transition":@([transition raw])}];return nil;
}
@end
@interface DeviceControlApi:NSObject
+ (instancetype)share;
- (id)setBrightness:(id)target level:(int)level onCompletion:(id)callback;
@end
@implementation DeviceControlApi
+ (instancetype)share{static DeviceControlApi *s;static dispatch_once_t once;dispatch_once(&once,^{s=[self new];});return s;}
- (id)setBrightness:(id)target level:(int)level onCompletion:(id)callback{return nil;}
@end
@interface RLMRealm:NSObject
@property(nonatomic,strong) NSMutableArray *scenes,*pending;
@property(nonatomic) BOOL inWriteTransaction;
+ (instancetype)defaultRealm;
- (NSArray *)allObjects:(NSString *)name;
- (NSArray *)objects:(NSString *)name withPredicate:(NSPredicate *)predicate;
- (void)deleteObject:(id)object;
- (void)beginWriteTransaction;
- (id)createObject:(NSString *)name withValue:(NSDictionary *)value;
- (BOOL)commitWriteTransaction:(NSError **)error;
- (void)cancelWriteTransaction;
@end
@implementation RLMRealm
+ (instancetype)defaultRealm{static RLMRealm *realm;static dispatch_once_t once;dispatch_once(&once,^{realm=[self new];realm.scenes=[NSMutableArray new];for(int i=0;i<11;i++)[realm.scenes addObject:@{@"uuid":[NSString stringWithFormat:@"scene-%d",i],@"routineName":[NSString stringWithFormat:@"Saved scene %d",i+1],@"color":@0xff665588,@"routineList":@[]}];});return realm;}
- (NSArray *)allObjects:(NSString *)name{return self.scenes;}
- (NSArray *)objects:(NSString *)name withPredicate:(NSPredicate *)predicate{return [(self.inWriteTransaction?self.pending:self.scenes)filteredArrayUsingPredicate:predicate];}
- (void)deleteObject:(id)object{assert(self.inWriteTransaction);[self.pending removeObject:object];}
- (void)beginWriteTransaction{assert(!self.inWriteTransaction);self.inWriteTransaction=YES;self.pending=[self.scenes mutableCopy];}
- (id)createObject:(NSString *)name withValue:(NSDictionary *)value{assert(self.inWriteTransaction&&[name isEqual:@"RoutinesInfo"]);[self.pending addObject:value];return value;}
- (BOOL)commitWriteTransaction:(NSError **)error{self.scenes=self.pending;self.pending=nil;self.inWriteTransaction=NO;return YES;}
- (void)cancelWriteTransaction{self.pending=nil;self.inWriteTransaction=NO;}
@end
// These exact class/ivar names exercise the real badge/menu/logo hooks.
@interface _TtC13SmartHome_Prd27ReBLENetworkIndicatorButton:UIView
@property(nonatomic,strong) UIView *view,*indicatorDot;
@property(nonatomic,strong) UILabel *title;
@end
@implementation _TtC13SmartHome_Prd27ReBLENetworkIndicatorButton
- (instancetype)initWithFrame:(CGRect)frame{self=[super initWithFrame:frame];if(self){self.view=[[UIView alloc]initWithFrame:self.bounds];self.view.backgroundColor=UIColor.whiteColor;self.view.layer.cornerRadius=21;self.view.layer.borderWidth=1;self.view.layer.borderColor=UIColor.orangeColor.CGColor;[self addSubview:self.view];self.indicatorDot=[[UIView alloc]initWithFrame:CGRectMake(12,15,12,12)];self.indicatorDot.backgroundColor=UIColor.greenColor;self.indicatorDot.layer.cornerRadius=6;[self.view addSubview:self.indicatorDot];self.title=[[UILabel alloc]initWithFrame:CGRectMake(32,0,96,42)];self.title.text=@"Connected";self.title.textColor=UIColor.blackColor;self.title.font=[UIFont systemFontOfSize:13];[self.view addSubview:self.title];}return self;}
@end
@interface _TtC13SmartHome_Prd20ReHomeViewController:UIViewController{
@public UIImageView *$__lazy_storage_$_logoView;
}
@property(nonatomic,strong) _TtC13SmartHome_Prd27ReBLENetworkIndicatorButton *badge;
@property(nonatomic,strong) UILabel *roomLabel;
@property(nonatomic,strong) UIButton *menuButton;
@end
@implementation _TtC13SmartHome_Prd20ReHomeViewController
- (void)viewDidLoad{[super viewDidLoad];self.view.backgroundColor=UIColor.whiteColor;self.badge=[[_TtC13SmartHome_Prd27ReBLENetworkIndicatorButton alloc]initWithFrame:CGRectMake(110,130,145,42)];[self.view addSubview:self.badge];$__lazy_storage_$_logoView=[[UIImageView alloc]initWithFrame:CGRectMake(270,130,70,42)];$__lazy_storage_$_logoView.backgroundColor=UIColor.orangeColor;[self.view addSubview:$__lazy_storage_$_logoView];self.roomLabel=[[UILabel alloc]initWithFrame:CGRectMake(24,210,340,80)];self.roomLabel.text=@"Your home, after dark";self.roomLabel.textColor=UIColor.blackColor;self.roomLabel.font=[UIFont systemFontOfSize:24];[self.view addSubview:self.roomLabel];self.menuButton=[UIButton buttonWithType:UIButtonTypeCustom];[self.menuButton setImage:[[UIImage systemImageNamed:@"line.3.horizontal"]imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal]forState:UIControlStateNormal];self.menuButton.tintColor=UIColor.blackColor;self.navigationItem.leftBarButtonItem=[[UIBarButtonItem alloc]initWithCustomView:self.menuButton];}
@end
@interface _TtC13SmartHome_Prd20ReMenuViewController:UIViewController{
@public NSInteger HELP_AND_SUPPORT,LEGAL_SECTION,NETWORK_STATUS_SECTION,NETWOKR_ANALYSE;
}
@property(nonatomic) NSInteger selected;
- (NSInteger)tableView:(id)table numberOfRowsInSection:(NSInteger)section;
- (id)tableView:(id)table viewForHeaderInSection:(NSInteger)section;
- (CGFloat)tableView:(id)table heightForHeaderInSection:(NSInteger)section;
@end
@implementation _TtC13SmartHome_Prd20ReMenuViewController
- (instancetype)init{self=[super init];if(self){HELP_AND_SUPPORT=3;LEGAL_SECTION=4;NETWORK_STATUS_SECTION=0;NETWOKR_ANALYSE=1;}return self;}
- (NSInteger)tableView:(id)table numberOfRowsInSection:(NSInteger)section{return section==0?3:2;}
- (UITableViewCell *)tableView:(id)table cellForRowAtIndexPath:(NSIndexPath *)path{UITableViewCell *cell=[UITableViewCell new];cell.textLabel.text=[NSString stringWithFormat:@"Original row %ld",(long)path.row];return cell;}
- (void)tableView:(id)table didSelectRowAtIndexPath:(NSIndexPath *)path{self.selected=path.row;}
- (CGFloat)tableView:(id)table heightForRowAtIndexPath:(NSIndexPath *)path{return 40+path.row;}
- (id)tableView:(id)table viewForHeaderInSection:(NSInteger)section{return [UIView new];}
- (CGFloat)tableView:(id)table heightForHeaderInSection:(NSInteger)section{return 44;}
@end
@interface NSObject(HPSmokeCalls)
- (void)start:(NSInteger)mode;
- (void)startJourney:(NSInteger)journey;
- (void)resume;
- (void)tick;
- (NSDictionary *)recipeNamed:(NSString *)name;
- (BOOL)applyRecipe:(NSDictionary *)recipe error:(NSError **)error;
- (BOOL)saveMovingMoodNamed:(NSString *)name error:(NSError **)error;
- (void)deleteMovingMood:(NSString *)identifier;
- (void)buildGallery;
- (void)playSaved:(UIButton *)card;
- (void)nextLight;
- (void)shuffleColours;
- (void)restoreColours;
- (void)stop;
- (void)install;
- (void)moveButton:(UIButton *)button to:(CGPoint)point remember:(BOOL)remember;
- (void)capturePausedSceneWithName:(NSString *)name completion:(void(^)(NSError *))completion;
- (void)moveLightAtIndex:(NSUInteger)index to:(CGPoint)point;
@end
static id Shared(NSString *name){return ((id(*)(id,SEL))objc_msgSend)(NSClassFromString(name),NSSelectorFromString(@"shared"));}
static void Later(double seconds,dispatch_block_t block){dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(seconds*NSEC_PER_SEC)),dispatch_get_main_queue(),block);}
static void WaitUntil(double deadline,NSString *name,BOOL(^predicate)(void),dispatch_block_t completion){
    TestWait=name;
    if(predicate()){completion();return;}
    if(NSProcessInfo.processInfo.systemUptime>=deadline){NSLog(@"Timed out waiting for %@: markers %lu, pending reads %lu, last marker %@",name,(unsigned long)MarkerWrites.count,(unsigned long)PendingReads,MarkerWrites.lastObject);assert(predicate());return;}
    Later(.1,^{WaitUntil(deadline,name,predicate,completion);});
}
static void Until(double seconds,NSString *name,BOOL(^predicate)(void),dispatch_block_t completion){WaitUntil(NSProcessInfo.processInfo.systemUptime+seconds,name,predicate,completion);}
static void RealScreen(dispatch_block_t completion){
    NSString *documents=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
    assert([[@"ready"dataUsingEncoding:NSUTF8StringEncoding]writeToFile:[documents stringByAppendingPathComponent:@"ActualStudio.request"]atomically:YES]);
    Until(120,@"operating-system screen capture",^BOOL{return [NSFileManager.defaultManager fileExistsAtPath:[documents stringByAppendingPathComponent:@"ActualStudio.done"]];},completion);
}
static void SaveScreen(UIWindow *window,NSString *name){UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc]initWithBounds:window.bounds];UIImage *image=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){NSArray *windows=[UIApplication.sharedApplication.windows sortedArrayUsingComparator:^NSComparisonResult(UIWindow *a,UIWindow *b){return a.windowLevel<b.windowLevel?NSOrderedAscending:NSOrderedDescending;}];for(UIWindow *w in windows)if(!w.hidden)[w.layer renderInContext:context.CGContext];}];NSString *path=[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:name];assert([UIImagePNGRepresentation(image)writeToFile:path atomically:YES]);}
static void AssertVisibleCardText(UIViewController *studio){
    // Metadata alone missed labels that disappeared after a round trip through
    // the gallery. Check actual white text pixels inside each visible card.
    [studio.view layoutIfNeeded];UIStackView *gallery=[studio valueForKey:@"gallery"];NSUInteger checked=0;
    for(UIView *row in gallery.arrangedSubviews){NSArray *cards=[row isKindOfClass:UIStackView.class]?[(UIStackView *)row arrangedSubviews]:@[row];for(UIControl *card in cards){
        if(![card isKindOfClass:NSClassFromString(@"HPModeCard")])continue;CGRect screen=[card convertRect:card.bounds toView:studio.view.window];if(CGRectGetMinY(screen)<0||CGRectGetMaxY(screen)>studio.view.window.bounds.size.height)continue;
        UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];format.scale=1;UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc]initWithSize:card.bounds.size format:format];UIImage *image=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){[card.layer renderInContext:context.CGContext];}];
        size_t width=CGImageGetWidth(image.CGImage),height=CGImageGetHeight(image.CGImage);unsigned char *pixels=calloc(width*height,4);CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();CGContextRef bitmap=CGBitmapContextCreate(pixels,width,height,8,width*4,space,kCGBitmapByteOrder32Big|kCGImageAlphaPremultipliedLast);assert(bitmap);CGContextDrawImage(bitmap,CGRectMake(0,0,width,height),image.CGImage);NSUInteger bright=0;
        for(size_t y=12;y+12<height;y++)for(size_t x=12;x+12<width;x++){unsigned char *p=pixels+(y*width+x)*4;if(p[0]>180&&p[1]>180&&p[2]>180)bright++;}CGContextRelease(bitmap);CGColorSpaceRelease(space);free(pixels);if(bright<=100)NSLog(@"Missing rendered caption for %@: %lu pixels",card.accessibilityLabel,(unsigned long)bright);assert(bright>100);checked++;
    }}assert(checked>0);
}
@interface HPSmokeDelegate:UIResponder<UIApplicationDelegate>
@property(nonatomic,strong) UIWindow *window;
@property(nonatomic,strong) UIViewController *studio;
- (void)checkInterface;
- (void)checkMovingMoods;
- (void)checkControls:(dispatch_block_t)completion;
- (void)checkStorage;
- (void)checkShuffle:(dispatch_block_t)completion;
- (void)checkMap:(UINavigationController *)navigation;
- (void)checkDeletion:(UINavigationController *)navigation;
- (void)finishReport;
@end
@implementation HPSmokeDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)options{
    UIViewController *launch=[[UIStoryboard storyboardWithName:@"HomePlusBlankLaunch360"bundle:NSBundle.mainBundle]instantiateInitialViewController];[launch loadViewIfNeeded];assert(launch.view.subviews.count==0);CGFloat r,g,b,a;[launch.view.backgroundColor getRed:&r green:&g blue:&b alpha:&a];assert(r<.1&&g<.1&&b<.1);
    InitLamps();self.window=[[UIWindow alloc]initWithFrame:UIScreen.mainScreen.bounds];self.studio=[NSClassFromString(@"HPStudio")new];assert(self.studio);self.window.rootViewController=[[UINavigationController alloc]initWithRootViewController:self.studio];[self.window makeKeyAndVisible];
    Later(1,^{id engine=Shared(@"HPLightEngine");DropReads=YES;QueueBusy=YES;[engine start:0];assert(Reads.count==0&&Writes.count==0);
        Later(.4,^{assert(Writes.count==0&&Reads.count==0);QueueBusy=NO;
            Later(5.2,^{NSMutableSet *targets=[NSMutableSet new];for(NSDictionary *write in Writes)[targets addObject:write[@"target"]];assert(targets.count==14&&Reads.count==0);DropReads=NO;assert(AmbientWrites>0);SaveScreen(self.window,@"AuroraGreen.png");[self checkControls:^{[self checkShuffle:^{[self checkStorage];}];}];});
        });
    });return YES;
}
- (void)checkControls:(dispatch_block_t)completion{
    UIView *whiteControl=[[UIView alloc]initWithFrame:CGRectMake(0,0,2,2)];whiteControl.backgroundColor=UIColor.whiteColor;[self.studio.view addSubview:whiteControl];[self.window layoutIfNeeded];CGFloat red,green,blue,alpha;[whiteControl.backgroundColor getRed:&red green:&green blue:&blue alpha:&alpha];assert(red>.95&&green>.95&&blue>.95);[whiteControl removeFromSuperview];
    id engine=Shared(@"HPLightEngine");UIStackView *gallery=[self.studio valueForKey:@"gallery"];UIScrollView *scroll=(id)self.studio.view.subviews.firstObject;UIButton *embers=nil;NSUInteger checked=0;
    // Exercise every actual mode card while already running, not engine setters.
    for(UIStackView *row in gallery.arrangedSubviews)if([row isKindOfClass:UIStackView.class])for(UIButton *card in row.arrangedSubviews){
        CGRect rect=[card convertRect:card.bounds toView:scroll];[scroll scrollRectToVisible:rect animated:NO];[self.window layoutIfNeeded];
        CGPoint point=[card convertPoint:CGPointMake(CGRectGetMidX(card.bounds),CGRectGetMidY(card.bounds))toView:self.window];UIView *hit=[self.window hitTest:point withEvent:nil];assert(hit==card||[hit isDescendantOfView:card]);assert(scroll.canCancelContentTouches&&scroll.scrollEnabled&&scroll.panGestureRecognizer.enabled&&[scroll touchesShouldCancelInContentView:card]&&[scroll touchesShouldCancelInContentView:[card valueForKey:@"caption"]]);
        [card sendActionsForControlEvents:UIControlEventTouchUpInside];assert([[engine valueForKey:@"lastMode"]integerValue]==card.tag&&[[engine valueForKey:@"journey"]integerValue]==-1);checked++;if(card.tag==3)embers=card;
    }assert(checked==HP_MODE_COUNT&&embers);
    [embers sendActionsForControlEvents:UIControlEventTouchUpInside];[scroll setContentOffset:CGPointZero animated:NO];
    Later(1.8,^{for(NSDictionary *value in LampValues.allValues)assert([value[@"hue"]intValue]<40); // Old limiter takes almost four minutes.
        UISlider *brightness=[self.studio valueForKey:@"brightnessSlider"],*depth=[self.studio valueForKey:@"depthSlider"],*motion=[self.studio valueForKey:@"motionSlider"];
        CGPoint point=[brightness convertPoint:CGPointMake(CGRectGetMidX(brightness.bounds),CGRectGetMidY(brightness.bounds))toView:self.window];UIView *hit=[self.window hitTest:point withEvent:nil];assert(hit==brightness||[hit isDescendantOfView:brightness]);assert(![scroll touchesShouldCancelInContentView:brightness]);
        brightness.value=.1;[brightness sendActionsForControlEvents:UIControlEventValueChanged];assert(fabs([[engine valueForKey:@"brightness"]floatValue]-.1)<.001);
        Later(1.8,^{for(NSDictionary *value in LampValues.allValues)assert([value[@"light"]intValue]>=4&&[value[@"light"]intValue]<=6);
            assert(fabs(brightness.value-.1)<.001);depth.value=.25;[depth sendActionsForControlEvents:UIControlEventValueChanged];
            Later(1.8,^{for(NSDictionary *value in LampValues.allValues)assert([value[@"sat"]intValue]<=16);
                motion.value=.15;[motion sendActionsForControlEvents:UIControlEventValueChanged];double lowStart=[[engine valueForKey:@"phase"]doubleValue];
                Later(.7,^{double lowDistance=[[engine valueForKey:@"phase"]doubleValue]-lowStart;motion.value=1;[motion sendActionsForControlEvents:UIControlEventValueChanged];double highStart=[[engine valueForKey:@"phase"]doubleValue];
                    Later(.7,^{double highDistance=[[engine valueForKey:@"phase"]doubleValue]-highStart;assert(lowDistance>.04&&highDistance>lowDistance*5);for(UIStackView *row in gallery.arrangedSubviews)for(UIButton *card in row.arrangedSubviews){UILabel *caption=[card valueForKey:@"caption"];assert(caption.attributedText.length>0&&caption.bounds.size.height>30&&caption.bounds.size.width>40&&!caption.hidden&&caption.alpha==1&&caption.superview==card);}AssertVisibleCardText(self.studio);SaveScreen(self.window,@"StudioControls.png");
                        [engine stop];double frozen=[[engine valueForKey:@"phase"]doubleValue];NSDictionary *held=[LampValues[@0x20]copy];brightness.value=.2;[brightness sendActionsForControlEvents:UIControlEventValueChanged];
                        Later(1.8,^{assert([[engine valueForKey:@"mode"]integerValue]==-1&&![[engine valueForKey:@"adjusting"]boolValue]);assert(abs([LampValues[@0x20][@"light"]intValue]-[held[@"light"]intValue]*2)<=1&&[LampValues[@0x20][@"hue"]isEqual:held[@"hue"]]);depth.value=.5;[depth sendActionsForControlEvents:UIControlEventValueChanged];
                            Later(1.8,^{assert([[engine valueForKey:@"mode"]integerValue]==-1&&[[engine valueForKey:@"phase"]doubleValue]==frozen);assert([LampValues[@0x20][@"sat"]intValue]>=[held[@"sat"]intValue]+20&&[LampValues[@0x20][@"hue"]isEqual:held[@"hue"]]);completion();});
                        });
                    });
                });
            });
        });
    });
}
- (void)checkShuffle:(dispatch_block_t)completion{
    id engine=Shared(@"HPLightEngine");UIButton *prism=nil;UIStackView *gallery=[self.studio valueForKey:@"gallery"];for(UIStackView *row in gallery.arrangedSubviews)for(UIButton *card in row.arrangedSubviews)if(card.tag==5)prism=card;assert(prism);[prism sendActionsForControlEvents:UIControlEventTouchUpInside];
    Until(4,@"Prism selection",^BOOL{return [[engine valueForKey:@"retargets"]count]==0;},^{NSDictionary *before=[LampValues copy];UIButton *shuffle=[self.studio valueForKey:@"shuffleButton"];UISegmentedControl *category=[self.studio valueForKey:@"category"];assert(shuffle.superview==category.superview&&CGRectGetMaxY(shuffle.frame)<CGRectGetMinY(category.frame));uint32_t seed=[[engine valueForKey:@"variation"]unsignedIntValue];[shuffle sendActionsForControlEvents:UIControlEventTouchUpInside];assert([[engine valueForKey:@"variation"]unsignedIntValue]!=seed&&[[engine valueForKey:@"lastMode"]intValue]==5);
        Until(4,@"shuffled colours",^BOOL{return [[engine valueForKey:@"retargets"]count]==0;},^{double change=0;for(NSNumber *key in LampValues)change+=abs([LampValues[key][@"hue"]intValue]-[before[key][@"hue"]intValue]);assert(change>50);AssertVisibleCardText(self.studio);SaveScreen(self.window,@"ShuffleColours.png");[engine stop];NSDictionary *held=[LampValues copy];double phase=[[engine valueForKey:@"phase"]doubleValue];[shuffle sendActionsForControlEvents:UIControlEventTouchUpInside];
            Until(4,@"shuffled paused look",^BOOL{return ![[engine valueForKey:@"adjusting"]boolValue];},^{assert([[engine valueForKey:@"mode"]intValue]==-1&&[[engine valueForKey:@"phase"]doubleValue]==phase);for(NSNumber *key in LampValues){assert(abs([LampValues[key][@"light"]intValue]-[held[key][@"light"]intValue])<=1&&abs([LampValues[key][@"sat"]intValue]-[held[key][@"sat"]intValue])<=1);}NSDictionary *recipe=[engine recipeNamed:@"Shuffled Prism"];assert([HPStudioPresets validatedRecipe:recipe error:nil]&&[recipe[@"variation"]unsignedIntValue]==[[engine valueForKey:@"variation"]unsignedIntValue]);[engine restoreColours];
                Until(4,@"original palette restored",^BOOL{return ![[engine valueForKey:@"adjusting"]boolValue];},^{assert([[engine valueForKey:@"variation"]unsignedIntValue]==0);RealScreen(completion);});
            });
        });
    });
}
- (void)checkStorage{
    id engine=Shared(@"HPLightEngine");[[DeviceControlApi share]setBrightness:@"Lamp 1"level:40 onCompletion:nil];NSUInteger stopped=Writes.count;
    Later(.3,^{assert(Writes.count==stopped);[engine start:3];
        Later(2.8,^{[self.studio stop];NSUInteger beforeReads=Reads.count;
            for(int i=0;i<14;i++){NSDictionary *v=@{@"hue":@(201+i),@"sat":@43,@"light":@22};LampValues[@(0x20+i)]=v;LastTargets[@(0x20+i)]=v;}
            [self.studio capturePausedSceneWithName:@"Aurora Glow"completion:^(NSError *error){assert(!error);RLMRealm *realm=[RLMRealm defaultRealm];assert(realm.scenes.count==12);NSDictionary *scene=realm.scenes.lastObject;NSArray *lights=scene[@"routineList"];assert(lights.count==14&&[scene[@"routineName"]isEqual:@"Aurora Glow"]);
                for(int i=0;i<14;i++){NSDictionary *lamp=lights[i];assert([lamp[@"deviceType"]intValue]==8&&[lamp[@"color"]intValue]==201+i&&[lamp[@"saturation"]intValue]==43&&[lamp[@"lightness"]intValue]==22);}
                assert(Reads.count>=beforeReads+14);SaveScreen(self.window,@"LightStudio.png");UIScrollView *scroll=(id)self.studio.view.subviews.firstObject;[scroll setContentOffset:CGPointMake(0,MAX(0,scroll.contentSize.height-scroll.bounds.size.height+scroll.adjustedContentInset.bottom))animated:NO];SaveScreen(self.window,@"PausedScene.png");
                FailReadTarget=0x25;[self.studio capturePausedSceneWithName:@"Must not save"completion:^(NSError *failure){assert(failure&&[RLMRealm defaultRealm].scenes.count==12);FailReadTarget=0;[self checkMovingMoods];}];
            }];Later(.2,^{assert(Reads.count==beforeReads);});
        });
    });
}
- (void)checkMovingMoods{
    id engine=Shared(@"HPLightEngine");[engine setValue:@.42 forKey:@"brightness"];[engine setValue:@.25 forKey:@"speed"];[engine setValue:@.55 forKey:@"depth"];[engine setValue:@874 forKey:@"variation"];[engine startJourney:0];[engine setValue:@152.5 forKey:@"phase"];[engine setValue:@301.5 forKey:@"journeyElapsed"];[engine stop];
    NSError *error=nil;assert([self.studio saveMovingMoodNamed:@"Northern quiet"error:&error]&&!error);NSDictionary *homes=[NSUserDefaults.standardUserDefaults dictionaryForKey:@"HP.StudioMoods"];NSArray *recipes=homes.allValues.firstObject;assert(recipes.count==1);NSDictionary *saved=recipes[0];assert([saved[@"journey"]intValue]==0&&[saved[@"variation"]intValue]==874&&[saved[@"phase"]doubleValue]==152.5);
    [engine setValue:@0 forKey:@"phase"];assert([engine applyRecipe:saved error:&error]&&!error);assert(fabs([[engine valueForKey:@"phase"]doubleValue]-152.5)<.02&&fabs([[engine valueForKey:@"depth"]floatValue]-.55)<.001);[engine stop];double phase=[[engine valueForKey:@"phase"]doubleValue];[engine resume];assert(fabs([[engine valueForKey:@"phase"]doubleValue]-phase)<.02);
    // Reconnect must use the changed native lamp cache, not stale sent targets.
    Disconnected=YES;[engine tick];NSUInteger writes=Writes.count;NSDictionary *v=@{@"hue":@111,@"sat":@44,@"light":@31};LampValues[@0x20]=v;LastTargets[@0x20]=v;[engine setValue:@0 forKey:@"cursor"];Disconnected=NO;[engine tick];assert(Writes.count==writes+1&&[Writes.lastObject[@"hue"]intValue]==111);[engine stop];
    UISegmentedControl *category=[self.studio valueForKey:@"category"];category.selectedSegmentIndex=1;[self.studio buildGallery];UIScrollView *scroll=(id)self.studio.view.subviews.firstObject;[scroll setContentOffset:CGPointZero animated:NO];SaveScreen(self.window,@"Journeys.png");
    category.selectedSegmentIndex=2;[self.studio buildGallery];SaveScreen(self.window,@"SavedMoods.png");
    assert([self.studio saveMovingMoodNamed:@"Second mood"error:&error]);[self.studio deleteMovingMood:saved[@"id"]];homes=[NSUserDefaults.standardUserDefaults dictionaryForKey:@"HP.StudioMoods"];recipes=homes.allValues.firstObject;assert(recipes.count==1&&![recipes[0][@"id"]isEqual:saved[@"id"]]);
    // A genuinely silent request must release its own queue handle on timeout.
    [engine setValue:@0 forKey:@"settleUntil"];DropReads=YES;NSUInteger cancelled=CancelledReads;
    [self.studio capturePausedSceneWithName:@"No reply"completion:^(NSError *failure){assert(failure&&[RLMRealm defaultRealm].scenes.count==12&&PendingReads==0&&CancelledReads==cancelled+1);DropReads=NO;[self checkInterface];}];
}
- (void)checkInterface{
    _TtC13SmartHome_Prd20ReMenuViewController *menu=[_TtC13SmartHome_Prd20ReMenuViewController new];assert([menu tableView:nil numberOfRowsInSection:0]==2&&[menu tableView:nil numberOfRowsInSection:3]==0&&[menu tableView:nil numberOfRowsInSection:4]==0);assert([menu tableView:nil viewForHeaderInSection:3]==nil);NSIndexPath *remaining=[NSIndexPath indexPathForRow:1 inSection:0];assert([[[menu tableView:nil cellForRowAtIndexPath:remaining]textLabel].text isEqual:@"Original row 2"]);[menu tableView:nil didSelectRowAtIndexPath:remaining];assert(menu.selected==2&&[menu tableView:nil heightForRowAtIndexPath:remaining]==42);
    _TtC13SmartHome_Prd20ReHomeViewController *home=[_TtC13SmartHome_Prd20ReHomeViewController new];self.window.rootViewController=[[UINavigationController alloc]initWithRootViewController:home];
    Later(.3,^{assert(home->$__lazy_storage_$_logoView.hidden);CGFloat r,g,b,a;[home.badge.view.backgroundColor getRed:&r green:&g blue:&b alpha:&a];assert(r<.2&&g<.2&&b<.2);
        home.badge.view.backgroundColor=UIColor.whiteColor;home.badge.title.textColor=UIColor.blackColor;[home.badge.view.backgroundColor getRed:&r green:&g blue:&b alpha:&a];assert(r<.2);[home.badge.title.textColor getRed:&r green:&g blue:&b alpha:&a];assert(r>.95&&g>.95&&b>.95);
        // Reproduce the original app assigning black after page changes.
        home.roomLabel.textColor=UIColor.blackColor;UIColor *readable=[home.roomLabel.textColor resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark]];[readable getRed:&r green:&g blue:&b alpha:&a];assert(r>.9&&g>.9&&b>.9);
        home.roomLabel.attributedText=[[NSAttributedString alloc]initWithString:@"Living Room" attributes:@{NSForegroundColorAttributeName:UIColor.blackColor}];readable=[[home.roomLabel.attributedText attribute:NSForegroundColorAttributeName atIndex:0 effectiveRange:NULL]resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark]];[readable getRed:&r green:&g blue:&b alpha:&a];assert(r>.9);
        home.menuButton.tintColor=UIColor.blackColor;[home.menuButton setImage:[[UIImage systemImageNamed:@"line.3.horizontal"]imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal]forState:UIControlStateNormal];[home.menuButton.tintColor getRed:&r green:&g blue:&b alpha:&a];assert(r>.95&&[home.menuButton imageForState:UIControlStateNormal].renderingMode==UIImageRenderingModeAlwaysTemplate);
        home->$__lazy_storage_$_logoView.hidden=NO;assert(home->$__lazy_storage_$_logoView.hidden);
        id manager=Shared(@"HPManager");UIButton *plus=[manager valueForKey:@"button"],*studio=[manager valueForKey:@"studioButton"];UIWindow *overlay=[manager valueForKey:@"overlay"];
        assert(plus&&studio&&!plus.hidden&&!studio.hidden&&!overlay.hidden&&plus.alpha==1&&studio.alpha==1&&UIApplication.sharedApplication.keyWindow==self.window);
        [[plus titleColorForState:UIControlStateNormal]getRed:&r green:&g blue:&b alpha:&a];assert(r>.95&&g>.95&&b>.95&&a==1);
        [manager moveButton:plus to:CGPointMake(70,260)remember:YES];[manager moveButton:studio to:CGPointMake(70,320)remember:YES];CGPoint position=plus.center;
        [plus removeFromSuperview];[studio removeFromSuperview];overlay.hidden=YES;[manager install];assert(CGPointEqualToPoint(plus.center,position)&&plus.superview&&studio.superview&&!overlay.hidden);
        assert([overlay hitTest:CGPointMake(20,500)withEvent:nil]==nil);
        SaveScreen(self.window,@"LegacyDark.png");
        [plus sendActionsForControlEvents:UIControlEventTouchUpInside];
        Until(8,@"Plus panel presentation",^BOOL{UIViewController *presented=home.presentedViewController;return [presented isKindOfClass:UINavigationController.class]&&[[(UINavigationController *)presented topViewController]isKindOfClass:NSClassFromString(@"HPPanel")]&&!presented.isBeingPresented&&!presented.transitionCoordinator;},^{UINavigationController *nav=(id)home.presentedViewController;assert([nav isKindOfClass:UINavigationController.class]);UITableViewController *panel=(id)nav.topViewController;assert([panel isKindOfClass:NSClassFromString(@"HPPanel")]);assert(!plus.hidden&&!studio.hidden);
            assert([panel.tableView.dataSource tableView:panel.tableView numberOfRowsInSection:2]==3);UITableViewCell *cell=[panel.tableView.dataSource tableView:panel.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:1 inSection:2]];assert([cell.textLabel.text isEqual:@"Import Home"]);SaveScreen(self.window,@"HomeDark.png");
            [studio sendActionsForControlEvents:UIControlEventTouchUpInside];Until(8,@"Studio shortcut navigation",^BOOL{return [nav.topViewController isKindOfClass:NSClassFromString(@"HPStudio")]&&!nav.transitionCoordinator;},^{[self checkMap:nav];});
        });
    });
}
- (void)checkMap:(UINavigationController *)navigation{
    UIViewController *map=[NSClassFromString(@"HPSpatialEditor")new];[navigation pushViewController:map animated:NO];[map.view layoutIfNeeded];id canvas=[map valueForKey:@"canvas"];assert([[canvas valueForKey:@"lights"]count]==14);
    NSUInteger colors=Writes.count;[canvas moveLightAtIndex:0 to:CGPointMake(.12,.84)];assert(Writes.count==colors&&BrightnessWrites.count==0);
    UIViewController *again=[NSClassFromString(@"HPSpatialEditor")new];[again loadViewIfNeeded];NSDictionary *placed=[[again valueForKey:@"canvas"]valueForKey:@"lights"][0];assert(fabs([placed[@"x"]doubleValue]-.12)<.00001&&fabs([placed[@"y"]doubleValue]-.84)<.00001);
    NSDictionary *original=[LampValues[@0x20]copy];UIButton *pin=[[canvas valueForKey:@"pins"]firstObject];[pin sendActionsForControlEvents:UIControlEventTouchUpInside];
    Until(3,@"first marker",^BOOL{return MarkerWrites.count>=1;},^{assert(MarkerWrites.count==1&&[MarkerWrites[0][@"target"]intValue]==0x20&&[MarkerWrites[0][@"sat"]intValue]==90&&[MarkerWrites[0][@"light"]intValue]==65&&BrightnessWrites.count==0);
        Until(10,@"first light restored",^BOOL{return MarkerWrites.count==2&&[LampValues[@0x20]isEqual:original]&&[[map valueForKey:@"nextButton"]isEnabled];},^{assert(Writes.count==colors+2);SaveScreen(self.window,@"ApartmentMap.png");
            NSDictionary *second=[LampValues[@0x21]copy];DropReads=YES;[map nextLight];
            Until(3,@"next marker",^BOOL{return MarkerWrites.count>=3;},^{assert([MarkerWrites[2][@"target"]intValue]==0x21);
                Until(15,@"restore and cancel unanswered query",^BOOL{return MarkerWrites.count==4&&[LampValues[@0x21]isEqual:second]&&PendingReads==0&&[[map valueForKey:@"nextButton"]isEnabled];},^{DropReads=NO;[self checkDeletion:navigation];});
            });
        });
    });
}
- (void)checkDeletion:(UINavigationController *)navigation{
    UITableViewController *editor=[[NSClassFromString(@"HPPathEditor")alloc]initWithStyle:UITableViewStyleInsetGrouped];[editor setValue:@YES forKey:@"scenes"];[navigation pushViewController:editor animated:NO];[editor loadViewIfNeeded];NSArray *items=[editor valueForKey:@"items"];assert(items.count==12);
    NSUInteger commands=Writes.count;NSIndexPath *first=[NSIndexPath indexPathForRow:0 inSection:0];[editor.tableView.dataSource tableView:editor.tableView commitEditingStyle:UITableViewCellEditingStyleDelete forRowAtIndexPath:first];
    Later(.4,^{UIAlertController *confirmation=(id)editor.presentedViewController;assert([confirmation isKindOfClass:UIAlertController.class]&&[confirmation.title containsString:@"Saved scene 1"]);UIAlertAction *action=confirmation.actions.lastObject;assert(action.style==UIAlertActionStyleDestructive);void(^handler)(UIAlertAction *)=[action valueForKey:@"handler"];assert(handler);[confirmation dismissViewControllerAnimated:NO completion:^{handler(action);assert([RLMRealm defaultRealm].scenes.count==11&&[[editor valueForKey:@"items"]count]==11&&Writes.count==commands);[self finishReport];}];});
}
- (void)finishReport{
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:@"HP.JoinPending"];UITableViewController *setup=[[NSClassFromString(@"HPPanel")alloc]initWithStyle:UITableViewStyleInsetGrouped];[setup loadViewIfNeeded];assert([setup.title isEqual:@"Add this device"]&&setup.navigationItem.rightBarButtonItem==nil&&[setup.tableView.dataSource numberOfSectionsInTableView:setup.tableView]==1&&[setup.tableView.dataSource tableView:setup.tableView numberOfRowsInSection:0]==2);[NSUserDefaults.standardUserDefaults removeObjectForKey:@"HP.JoinPending"];

    NSDictionary *report=@{@"passed":@YES,@"targets":@14,@"moods":@32,@"journeys":@6,@"writes":@(Writes.count),@"explicitFadedWrites":@(ExplicitFadedWrites),@"ambientBoundedWrites":@(AmbientWrites),@"allModeCardsSwitchWhileRunning":@YES,@"modeChangeReachesAllBulbsWithinOneRound":@YES,@"brightnessAndDepthRespondWhileRunningAndPaused":@YES,@"motionSliderChangesPhaseRate":@YES,@"sliderThumbSurvivesRefresh":@YES,@"galleryCaptionsSurviveScrollingAndRepeatedSelection":@YES,@"renderedGalleryTextPresentAfterScrolling":@YES,@"legacyThemeDoesNotRepaintStudioControls":@YES,@"sliderDragDoesNotScrollOrCancel":@YES,@"cardAndCaptionDragsCancelForVerticalScrolling":@YES,@"motionSliderMaximum10x":@YES,@"shuffleRecoloursPrismAndPreservesPausedLevels":@YES,@"shuffledRecipeRemembersVariation":@YES,@"originalPaletteCanBeRestored":@YES,@"reads":@(Reads.count),@"backpressure":@YES,@"manualControlStopsEffect":@YES,@"boundedSmallSteps":@YES,@"fourSecondFades":@YES,@"actualReadbackSceneSaved":@YES,@"failedReadLeavesScenesIntact":@YES,@"floatingButtonsWhiteAndPersistent":@YES,@"dragPositionRemembered":@YES,@"touchesPassThrough":@YES,@"darkConnectionBadge":@YES,@"lateBlackTextAndMenuAssignmentsGuarded":@YES,@"networkAnalyseRemoved":@YES,@"logoRemoved":@YES,@"launchStoryboardContainsNoTextOrImages":@YES,@"directStudioShortcut":@YES,@"apartmentMapPositionRemembered":@YES,@"gridTapMarksOnlyOneBulbAndRestoresColour":@YES,@"nextLightWorksWithoutColourReplies":@YES,@"studioStartsWithoutColourReplies":@YES,@"timedOutReadCancelsOnlyItsOwnHandle":@YES,@"journeyAndMovingMoodRoundTrip":@YES,@"pauseResumeKeepsPhase":@YES,@"reconnectUsesChangedNativeCache":@YES,@"sceneDeletionAfterNamedConfirmation":@YES};
    NSData *data=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];NSString *path=[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:@"smoke-report.json"];assert([data writeToFile:path atomically:YES]);
}
@end
int main(int argc,char **argv){@autoreleasepool{return UIApplicationMain(argc,argv,nil,NSStringFromClass(HPSmokeDelegate.class));}}
