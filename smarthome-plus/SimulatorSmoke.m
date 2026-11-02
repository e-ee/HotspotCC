// Real UIKit and production add-on, with synthetic lamp and database endpoints.
// Real Realm storage is tested separately by test_capture_realm.m.
#import <UIKit/UIKit.h>
#import <objc/message.h>
#import "SceneCapture.h"
#include <assert.h>
static NSMutableArray *Writes,*Reads,*BrightnessWrites;
static NSMutableDictionary *LampValues,*LastTargets;
static BOOL QueueBusy;
static uint16_t FailReadTarget;
@interface HPMockLamp:NSObject
@property(nonatomic) uint16_t address;
@property(nonatomic,strong) NSString *name,*UUID;
- (NSArray *)HSLAddresses;
@end
@implementation HPMockLamp
- (NSArray *)HSLAddresses{return @[@(self.address)];}
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
- (BOOL)isOpen{return YES;}
- (id)getCurrentPeripheral{return nil;}
@end
@interface SigMeshLib:NSObject
+ (instancetype)share;
- (NSArray *)commands;
@end
@implementation SigMeshLib
+ (instancetype)share{static SigMeshLib *s;static dispatch_once_t once;dispatch_once(&once,^{s=[self new];});return s;}
- (NSArray *)commands{return QueueBusy?@[@"pending"]:@[];}
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
static void InitLamps(void){if(LampValues)return;Writes=[NSMutableArray new];Reads=[NSMutableArray new];BrightnessWrites=[NSMutableArray new];LampValues=[NSMutableDictionary new];LastTargets=[NSMutableDictionary new];for(int i=0;i<14;i++){NSDictionary *value=@{@"hue":@(230+i),@"sat":@62,@"light":@35};LampValues[@(0x20+i)]=value;LastTargets[@(0x20+i)]=value;}}
@interface SDKLibCommand:NSObject
+ (id)lightHSLGetWithDestination:(uint16_t)target retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses successCallback:(void(^)(uint16_t,uint16_t,id))success resultCallback:(void(^)(BOOL,NSError *))result;
+ (id)lightHSLSetWithDestination:(uint16_t)target HSLLight:(uint16_t)light HSLHue:(uint16_t)hue HSLSaturation:(uint16_t)saturation transitionTime:(id)transition delay:(uint8_t)delay retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result;
@end
@implementation SDKLibCommand
+ (id)lightLightnessSetWithDestination:(uint16_t)target lightness:(uint16_t)level transitionTime:(id)transition delay:(uint8_t)delay retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result{
    assert(!ack&&retries==0&&responses==0&&success==nil&&result==nil&&[transition raw]==6&&level<=100);NSMutableDictionary *value=[LampValues[@(target)]mutableCopy];assert(value);value[@"light"]=@(level);LampValues[@(target)]=value;[BrightnessWrites addObject:@{@"target":@(target),@"level":@(level)}];return nil;
}
+ (id)lightHSLGetWithDestination:(uint16_t)target retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses successCallback:(void(^)(uint16_t,uint16_t,id))success resultCallback:(void(^)(BOOL,NSError *))result{
    InitLamps();assert(retries==0&&responses==1);[Reads addObject:@(target)];
    if(target==FailReadTarget){result(NO,[NSError errorWithDomain:@"mock" code:1 userInfo:@{NSLocalizedDescriptionKey:@"A test light did not respond"}]);return nil;}
    NSDictionary *v=LampValues[@(target)];assert(v);HPMockStatus *status=[HPMockStatus new];status.HSLHue=[v[@"hue"]unsignedShortValue];status.HSLSaturation=[v[@"sat"]unsignedShortValue];status.HSLLightness=[v[@"light"]unsignedShortValue];success(target,0x7e00,status);result(YES,nil);return nil;
}
+ (id)lightHSLSetWithDestination:(uint16_t)target HSLLight:(uint16_t)light HSLHue:(uint16_t)hue HSLSaturation:(uint16_t)saturation transitionTime:(id)transition delay:(uint8_t)delay retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result{
    InitLamps();assert(!ack&&retries==0&&responses==0&&success==nil&&result==nil);assert(hue<=360&&saturation<=100&&light<=100);assert([transition raw]>=40&&[transition raw]<=60);
    NSDictionary *previous=LastTargets[@(target)];assert(previous);
    assert(abs([previous[@"hue"]intValue]-hue)<=3&&abs([previous[@"sat"]intValue]-saturation)<=3&&abs([previous[@"light"]intValue]-light)<=2);
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
- (void)stop;
- (void)install;
- (void)moveButton:(UIButton *)button to:(CGPoint)point remember:(BOOL)remember;
- (void)capturePausedSceneWithName:(NSString *)name completion:(void(^)(NSError *))completion;
- (void)moveLightAtIndex:(NSUInteger)index to:(CGPoint)point;
@end
static id Shared(NSString *name){return ((id(*)(id,SEL))objc_msgSend)(NSClassFromString(name),NSSelectorFromString(@"shared"));}
static void Later(double seconds,dispatch_block_t block){dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(seconds*NSEC_PER_SEC)),dispatch_get_main_queue(),block);}
static void SaveScreen(UIWindow *window,NSString *name){UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc]initWithBounds:window.bounds];UIImage *image=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){NSArray *windows=[UIApplication.sharedApplication.windows sortedArrayUsingComparator:^NSComparisonResult(UIWindow *a,UIWindow *b){return a.windowLevel<b.windowLevel?NSOrderedAscending:NSOrderedDescending;}];for(UIWindow *w in windows)if(!w.hidden)[w drawViewHierarchyInRect:window.bounds afterScreenUpdates:YES];}];NSString *path=[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:name];assert([UIImagePNGRepresentation(image)writeToFile:path atomically:YES]);}
@interface HPSmokeDelegate:UIResponder<UIApplicationDelegate>
@property(nonatomic,strong) UIWindow *window;
@property(nonatomic,strong) UIViewController *studio;
- (void)checkInterface;
- (void)checkMap:(UINavigationController *)navigation;
- (void)checkDeletion:(UINavigationController *)navigation;
- (void)finishReport;
@end
@implementation HPSmokeDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)options{
    InitLamps();self.window=[[UIWindow alloc]initWithFrame:UIScreen.mainScreen.bounds];self.studio=[NSClassFromString(@"HPStudio")new];assert(self.studio);self.window.rootViewController=[[UINavigationController alloc]initWithRootViewController:self.studio];[self.window makeKeyAndVisible];
    Later(1,^{id engine=Shared(@"HPLightEngine");[engine start:0];assert(Reads.count==1&&Writes.count==0);QueueBusy=YES;
        Later(.4,^{assert(Writes.count==0&&Reads.count==1);QueueBusy=NO;
            Later(5.2,^{NSMutableSet *targets=[NSMutableSet new];for(NSDictionary *write in Writes)[targets addObject:write[@"target"]];assert(targets.count==14);
                [[DeviceControlApi share]setBrightness:@"Lamp 1" level:40 onCompletion:nil];NSUInteger stopped=Writes.count;
                Later(.3,^{assert(Writes.count==stopped);[engine start:3];
                    Later(2.8,^{[self.studio stop];NSUInteger beforeReads=Reads.count;
                        // Simulate physical lamp values differing from animation targets.
                        for(int i=0;i<14;i++)LampValues[@(0x20+i)]=@{@"hue":@(201+i),@"sat":@43,@"light":@22};
                        [self.studio capturePausedSceneWithName:@"Aurora Glow" completion:^(NSError *error){assert(!error);RLMRealm *realm=[RLMRealm defaultRealm];assert(realm.scenes.count==12);NSDictionary *scene=realm.scenes.lastObject;NSArray *lights=scene[@"routineList"];assert(lights.count==14&&[scene[@"routineName"]isEqual:@"Aurora Glow"]);
                            for(int i=0;i<14;i++){NSDictionary *lamp=lights[i];assert([lamp[@"deviceType"]intValue]==8&&[lamp[@"color"]intValue]==201+i&&[lamp[@"saturation"]intValue]==43&&[lamp[@"lightness"]intValue]==22);}
                            assert(Reads.count>=beforeReads+14);SaveScreen(self.window,@"LightStudio.png");
                            UIScrollView *scroll=(id)self.studio.view.subviews.firstObject;[scroll setContentOffset:CGPointMake(0,MAX(0,scroll.contentSize.height-scroll.bounds.size.height+scroll.adjustedContentInset.bottom)) animated:NO];SaveScreen(self.window,@"PausedScene.png");
                            FailReadTarget=0x25;[self.studio capturePausedSceneWithName:@"Must not save" completion:^(NSError *failure){assert(failure&&[RLMRealm defaultRealm].scenes.count==12);FailReadTarget=0;[self checkInterface];}];
                        }];
                        Later(.2,^{assert(Reads.count==beforeReads);}); // Wait for the last fade before reading.
                    });
                });
            });
        });
    });return YES;
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
        Later(.5,^{UINavigationController *nav=(id)home.presentedViewController;assert([nav isKindOfClass:UINavigationController.class]);UITableViewController *panel=(id)nav.topViewController;assert([panel isKindOfClass:NSClassFromString(@"HPPanel")]);assert(!plus.hidden&&!studio.hidden);
            assert([panel.tableView.dataSource tableView:panel.tableView numberOfRowsInSection:2]==2);UITableViewCell *cell=[panel.tableView.dataSource tableView:panel.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:1 inSection:2]];assert([cell.textLabel.text isEqual:@"Import Home"]);SaveScreen(self.window,@"HomeDark.png");
            [studio sendActionsForControlEvents:UIControlEventTouchUpInside];Later(.4,^{assert([nav.topViewController isKindOfClass:NSClassFromString(@"HPStudio")]);[self checkMap:nav];});
        });
    });
}
- (void)checkMap:(UINavigationController *)navigation{
    UIViewController *map=[NSClassFromString(@"HPSpatialEditor")new];[navigation pushViewController:map animated:NO];[map.view layoutIfNeeded];id canvas=[map valueForKey:@"canvas"];assert([[canvas valueForKey:@"lights"]count]==14);
    NSUInteger colors=Writes.count;[canvas moveLightAtIndex:0 to:CGPointMake(.12,.84)];assert(Writes.count==colors&&BrightnessWrites.count==0);
    UIViewController *again=[NSClassFromString(@"HPSpatialEditor")new];[again loadViewIfNeeded];NSDictionary *placed=[[again valueForKey:@"canvas"]valueForKey:@"lights"][0];assert(fabs([placed[@"x"]doubleValue]-.12)<.00001&&fabs([placed[@"y"]doubleValue]-.84)<.00001);
    NSDictionary *original=[LampValues[@0x20]copy];UIButton *pin=[[canvas valueForKey:@"pins"]firstObject];[pin sendActionsForControlEvents:UIControlEventTouchUpInside];
    Later(.3,^{assert(BrightnessWrites.count==1&&[BrightnessWrites[0][@"target"]intValue]==0x20&&[BrightnessWrites[0][@"level"]intValue]!=[original[@"light"]intValue]);
        Later(1.7,^{assert(BrightnessWrites.count==2&&[LampValues[@0x20]isEqual:original]&&Writes.count==colors);SaveScreen(self.window,@"ApartmentMap.png");[self checkDeletion:navigation];});
    });
}
- (void)checkDeletion:(UINavigationController *)navigation{
    UITableViewController *editor=[[NSClassFromString(@"HPPathEditor")alloc]initWithStyle:UITableViewStyleInsetGrouped];[editor setValue:@YES forKey:@"scenes"];[navigation pushViewController:editor animated:NO];[editor loadViewIfNeeded];NSArray *items=[editor valueForKey:@"items"];assert(items.count==12);
    NSUInteger commands=Writes.count;NSIndexPath *first=[NSIndexPath indexPathForRow:0 inSection:0];[editor.tableView.dataSource tableView:editor.tableView commitEditingStyle:UITableViewCellEditingStyleDelete forRowAtIndexPath:first];
    Later(.4,^{UIAlertController *confirmation=(id)editor.presentedViewController;assert([confirmation isKindOfClass:UIAlertController.class]&&[confirmation.title containsString:@"Saved scene 1"]);UIAlertAction *action=confirmation.actions.lastObject;assert(action.style==UIAlertActionStyleDestructive);void(^handler)(UIAlertAction *)=[action valueForKey:@"handler"];assert(handler);[confirmation dismissViewControllerAnimated:NO completion:^{handler(action);assert([RLMRealm defaultRealm].scenes.count==11&&[[editor valueForKey:@"items"]count]==11&&Writes.count==commands);[self finishReport];}];});
}
- (void)finishReport{
    NSDictionary *report=@{@"passed":@YES,@"targets":@14,@"moods":@24,@"writes":@(Writes.count),@"reads":@(Reads.count),@"backpressure":@YES,@"manualControlStopsEffect":@YES,@"boundedSmallSteps":@YES,@"fourSecondFades":@YES,@"actualReadbackSceneSaved":@YES,@"failedReadLeavesScenesIntact":@YES,@"floatingButtonsWhiteAndPersistent":@YES,@"dragPositionRemembered":@YES,@"touchesPassThrough":@YES,@"darkConnectionBadge":@YES,@"lateBlackTextAndMenuAssignmentsGuarded":@YES,@"networkAnalyseRemoved":@YES,@"logoRemoved":@YES,@"directStudioShortcut":@YES,@"apartmentMapPositionRemembered":@YES,@"gridTapPulsesOnlyOneBulbAndRestoresBrightness":@YES,@"sceneDeletionAfterNamedConfirmation":@YES};
    NSData *data=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];NSString *path=[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:@"smoke-report.json"];assert([data writeToFile:path atomically:YES]);
}
@end
int main(int argc,char **argv){@autoreleasepool{return UIApplicationMain(argc,argv,nil,NSStringFromClass(HPSmokeDelegate.class));}}
