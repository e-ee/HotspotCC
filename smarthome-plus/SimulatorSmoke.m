// UIKit/transport integration test. Fixtures contain no real home data.
#import <UIKit/UIKit.h>
#import <objc/message.h>
#include <assert.h>
static NSMutableArray *Writes;
static BOOL QueueBusy;
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
- (NSArray *)curNodes{
    static NSMutableArray *lamps;static dispatch_once_t once;dispatch_once(&once,^{lamps=[NSMutableArray new];for(int i=0;i<14;i++){HPMockLamp *lamp=[HPMockLamp new];lamp.address=0x20+i;lamp.name=[NSString stringWithFormat:@"Lamp %d",i];lamp.UUID=[NSString stringWithFormat:@"mock-lamp-%d",i];[lamps addObject:lamp];}});return lamps;
}
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
@interface SDKLibCommand:NSObject
+ (id)lightHSLSetWithDestination:(uint16_t)target HSLLight:(uint16_t)light HSLHue:(uint16_t)hue HSLSaturation:(uint16_t)saturation transitionTime:(id)transition delay:(uint8_t)delay retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result;
+ (id)lightHSLSetWithDestination:(uint16_t)target HSLLight:(uint16_t)light HSLHue:(uint16_t)hue HSLSaturation:(uint16_t)saturation retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result;
@end
@implementation SDKLibCommand
+ (id)lightHSLSetWithDestination:(uint16_t)target HSLLight:(uint16_t)light HSLHue:(uint16_t)hue HSLSaturation:(uint16_t)saturation transitionTime:(id)transition delay:(uint8_t)delay retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result{
    assert(!ack&&retries==0&&responses==0&&success==nil&&result==nil);
    if(!Writes)Writes=[NSMutableArray new];
    [Writes addObject:@{@"target":@(target),@"light":@(light),@"hue":@(hue),@"sat":@(saturation),@"transition":@([transition raw])}];return nil;
}
+ (id)lightHSLSetWithDestination:(uint16_t)target HSLLight:(uint16_t)light HSLHue:(uint16_t)hue HSLSaturation:(uint16_t)saturation retryCount:(NSInteger)retries responseMaxCount:(NSInteger)responses ack:(BOOL)ack successCallback:(id)success resultCallback:(id)result{return nil;}
@end
@interface DeviceControlApi:NSObject
+ (instancetype)share;
- (id)setBrightness:(id)target level:(int)level onCompletion:(id)callback;
@end
@implementation DeviceControlApi
+ (instancetype)share{static DeviceControlApi *s;static dispatch_once_t once;dispatch_once(&once,^{s=[self new];});return s;}
- (id)setBrightness:(id)target level:(int)level onCompletion:(id)callback{return nil;}
@end

@interface HPSmokeDelegate:UIResponder<UIApplicationDelegate>
@property(nonatomic,strong) UIWindow *window;
@end
@implementation HPSmokeDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)options{
    self.window=[[UIWindow alloc]initWithFrame:UIScreen.mainScreen.bounds];
    UIViewController *studio=[NSClassFromString(@"HPStudio") new];assert(studio);
    self.window.rootViewController=[[UINavigationController alloc]initWithRootViewController:studio];[self.window makeKeyAndVisible];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC),dispatch_get_main_queue(),^{
        id engine=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"HPLightEngine"),NSSelectorFromString(@"shared"));
        ((void(*)(id,SEL,NSInteger))objc_msgSend)(engine,NSSelectorFromString(@"start:"),0);
        assert(Writes.count==1);QueueBusy=YES;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,400*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
            assert(Writes.count==1);QueueBusy=NO;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2400*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
                NSMutableSet *targets=[NSMutableSet new],*hues=[NSMutableSet new],*levels=[NSMutableSet new];
                for(NSDictionary *write in Writes){[targets addObject:write[@"target"]];[hues addObject:write[@"hue"]];[levels addObject:write[@"light"]];assert([write[@"transition"]intValue]>0);}
                assert(targets.count==14&&hues.count>14&&levels.count>14);
                // The actual native-control hook must cancel the live effect timer.
                [[DeviceControlApi share]setBrightness:@"Lamp 1" level:40 onCompletion:nil];
                NSUInteger stopped=Writes.count;
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW,400*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
                    assert(Writes.count==stopped);
                    NSDictionary *report=@{@"passed":@YES,@"targets":@(targets.count),@"writes":@(Writes.count),@"differentHues":@(hues.count),@"backpressure":@YES,@"manualControlStopsEffect":@YES};
                    NSData *data=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];
                    NSString *path=[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:@"smoke-report.json"];
                    assert([data writeToFile:path atomically:YES]);
                });
            });
        });
    });return YES;
}
@end
int main(int argc,char **argv){@autoreleasepool{return UIApplicationMain(argc,argv,nil,NSStringFromClass(HPSmokeDelegate.class));}}
