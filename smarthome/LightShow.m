#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <Accelerate/Accelerate.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <stdatomic.h>
#import <math.h>

static id send0(id x, NSString *s) { SEL q=NSSelectorFromString(s); return [x respondsToSelector:q]?((id(*)(id,SEL))objc_msgSend)(x,q):nil; }
static NSMutableOrderedSet<NSString *> *nativeTargets;
static IMP origHSL, origOnOff, origLightness;
static void remember(id x) {if([x isKindOfClass:NSString.class] && [x length]) @synchronized(nativeTargets){[nativeTargets addObject:x];}}
static int watchHSL(id c,SEL s,id t,int l,int h,int sat,unsigned char r,int tr,short d){remember(t);return ((int(*)(id,SEL,id,int,int,int,unsigned char,int,short))origHSL)(c,s,t,l,h,sat,r,tr,d);}
static int watchOnOff(id c,SEL s,id t,unsigned char on,unsigned char r,int tr,short d){remember(t);return ((int(*)(id,SEL,id,unsigned char,unsigned char,int,short))origOnOff)(c,s,t,on,r,tr,d);}
static int watchLightness(id c,SEL s,id t,int l,unsigned char r,int tr,short d){remember(t);return ((int(*)(id,SEL,id,int,unsigned char,int,short))origLightness)(c,s,t,l,r,tr,d);}
static void swizzle(Class c,NSString *s,IMP replacement,IMP *original){ Method m=class_getClassMethod(c,NSSelectorFromString(s));if(m)*original=method_setImplementation(m,replacement); }

@interface LSPanel : NSObject {
@public atomic_uint _bassBits, _midBits, _highBits;
}
@property(nonatomic,strong) UIWindow *window;
@property(nonatomic,strong) UIView *panel;
@property(nonatomic,strong) UIButton *start;
@property(nonatomic,strong) UILabel *status;
@property(nonatomic,strong) UISlider *gain;
@property(nonatomic,strong) AVAudioEngine *engine;
@property(nonatomic,strong) NSTimer *timer;
@property(nonatomic,strong) NSArray<NSString *> *targets;
@property(nonatomic) BOOL running;
@property(nonatomic) BOOL tapInstalled;
@property(nonatomic) BOOL native;
@property(nonatomic) BOOL hasRGB;
@property(nonatomic) BOOL hasWhite;
@property(nonatomic) NSUInteger cursor;
@property(nonatomic) float hue;
@property(nonatomic) float smooth;
@property(nonatomic) BOOL oldIdle;
@property(nonatomic) vDSP_DFT_Setup fft;
- (void)install;
- (void)toggle;
- (void)stop;
@end
static LSPanel *controller;
static void setFloat(atomic_uint *dst,float v){unsigned n;memcpy(&n,&v,4);atomic_store_explicit(dst,n,memory_order_relaxed);}
static float getFloat(atomic_uint *src){unsigned n=atomic_load_explicit(src,memory_order_relaxed);float v;memcpy(&v,&n,4);return v;}
@implementation LSPanel
- (void)install {
 if(self.window)return;
 UIWindow *host=UIApplication.sharedApplication.keyWindow;
 if(!host)for(UIWindow *w in UIApplication.sharedApplication.windows)if(w.windowLevel==UIWindowLevelNormal){host=w;break;}
 if(!host){dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC),dispatch_get_main_queue(),^{[self install];});return;}
 CGRect screen=host.bounds;CGFloat width=MIN(screen.size.width-24,430);
 if(@available(iOS 13.0,*)){if(host.windowScene)self.window=[[UIWindow alloc]initWithWindowScene:host.windowScene];}
 if(!self.window)self.window=[[UIWindow alloc]initWithFrame:CGRectZero];
 self.window.frame=CGRectMake((screen.size.width-width)/2,MAX(80,screen.size.height-205-host.safeAreaInsets.bottom),width,156);
 self.window.windowLevel=UIWindowLevelAlert+1;
 UIViewController *vc=[UIViewController new];self.window.rootViewController=vc;
 vc.view.backgroundColor=[UIColor colorWithWhite:0.06 alpha:0.96];vc.view.layer.cornerRadius=18;vc.view.clipsToBounds=YES;
 UILabel *title=[[UILabel alloc]initWithFrame:CGRectMake(16,8,width-32,24)];title.text=@"LIGHT SHOW · MICROPHONE";title.font=[UIFont boldSystemFontOfSize:13];title.textColor=UIColor.whiteColor;[vc.view addSubview:title];
 self.status=[[UILabel alloc]initWithFrame:CGRectMake(16,34,width-32,40)];self.status.numberOfLines=2;self.status.font=[UIFont systemFontOfSize:12];self.status.textColor=UIColor.lightGrayColor;self.status.text=@"Connect your lights in the app, then tap Start.";[vc.view addSubview:self.status];
 self.gain=[[UISlider alloc]initWithFrame:CGRectMake(12,78,width-24,26)];self.gain.minimumValue=0.5;self.gain.maximumValue=8;self.gain.value=2.5;self.gain.tintColor=[UIColor colorWithRed:0.6 green:0.45 blue:1 alpha:1];[vc.view addSubview:self.gain];
 self.start=[UIButton buttonWithType:UIButtonTypeSystem];self.start.frame=CGRectMake(12,111,width-24,34);[self.start setTitle:@"Start visualizer" forState:UIControlStateNormal];self.start.titleLabel.font=[UIFont boldSystemFontOfSize:16];[self.start addTarget:self action:@selector(toggle) forControlEvents:UIControlEventTouchUpInside];[vc.view addSubview:self.start];
 [self.window setHidden:NO];
 [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(stop) name:UIApplicationWillResignActiveNotification object:nil];
 [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(stop) name:AVAudioSessionInterruptionNotification object:nil];
}
- (void)discover {
 self.hasRGB=NO;self.hasWhite=NO;
 id ds=send0(NSClassFromString(@"SigDataSource"),@"share");
 id nodes=send0(ds,@"curNodes");
 if([nodes isKindOfClass:NSArray.class])for(id n in nodes){
  id rgb=send0(n,@"HSLAddresses"), white=send0(n,@"lightnessAddresses");
  if([rgb respondsToSelector:@selector(count)] && [rgb count])self.hasRGB=YES;
  if([white respondsToSelector:@selector(count)] && [white count])self.hasWhite=YES;
 }
 Class native=NSClassFromString(@"MeshNativeHelper");
 SEL connected=NSSelectorFromString(@"meshClientIsConnectedToNetwork");
 self.native=[native respondsToSelector:connected]&&((unsigned char(*)(id,SEL))objc_msgSend)(native,connected);
 if(self.native){
  id groups=nil;SEL g=NSSelectorFromString(@"meshClientGetAllGroups:");
  if([native respondsToSelector:g])groups=((id(*)(id,SEL,id))objc_msgSend)(native,g,nil);
  if([groups isKindOfClass:NSArray.class])for(id x in groups)remember(x);
  @synchronized(nativeTargets){self.targets=nativeTargets.array.copy;}
 }else self.targets=@[];
}
- (void)toggle {
 if(self.running){[self stop];return;}
 [AVAudioSession.sharedInstance requestRecordPermission:^(BOOL ok){dispatch_async(dispatch_get_main_queue(),^{
  if(!ok){self.status.text=@"Allow microphone access in Settings to react to music.";return;}
  [self begin];
 });}];
}
- (void)begin {
 if(self.running)return;
 [self discover];
 if(self.native && !self.targets.count){self.status.text=@"Select All Lights in the app and change colour once, then Start.";return;}
 if(!self.native && !self.hasRGB && !self.hasWhite){self.status.text=@"No configured lights found. Connect or pair them in the app first.";return;}
 NSError *err=nil;AVAudioSession *audio=AVAudioSession.sharedInstance;
 if(![audio setCategory:AVAudioSessionCategoryPlayAndRecord mode:AVAudioSessionModeDefault options:AVAudioSessionCategoryOptionMixWithOthers|AVAudioSessionCategoryOptionDefaultToSpeaker|AVAudioSessionCategoryOptionAllowBluetoothA2DP error:&err] || ![audio setActive:YES error:&err]){self.status.text=err.localizedDescription;return;}
 self.engine=[AVAudioEngine new];AVAudioInputNode *input=self.engine.inputNode;AVAudioFormat *fmt=[input outputFormatForBus:0];
 if(fmt.channelCount==0 || fmt.sampleRate<=0){self.status.text=@"No microphone input available.";[self stop];return;}
 if(!self.fft)self.fft=vDSP_DFT_zop_CreateSetup(NULL,1024,vDSP_DFT_FORWARD);
 if(!self.fft){self.status.text=@"Audio analysis could not initialize.";[self stop];return;}
 __weak LSPanel *weak=self;
 [input installTapOnBus:0 bufferSize:1024 format:fmt block:^(AVAudioPCMBuffer *buf,AVAudioTime *when){
  LSPanel *me=weak;if(!me || !buf.floatChannelData || buf.frameLength<1024)return;
  float re[1024],im[1024]={0},outR[1024],outI[1024];const float *samples=buf.floatChannelData[0];
  for(int i=0;i<1024;i++)re[i]=samples[i]*(0.5f-0.5f*cosf(2*M_PI*i/1023));
  vDSP_DFT_Execute(me.fft,re,im,outR,outI);
  float sums[3]={0};int count[3]={0};
  for(int k=1;k<512;k++){float f=k*fmt.sampleRate/1024;int band=f<250?0:(f<2500?1:2);if(f>10000)break;sums[band]+=outR[k]*outR[k]+outI[k]*outI[k];count[band]++;}
  setFloat(&me->_bassBits,sqrtf(sums[0]/MAX(1,count[0]))/20);
  setFloat(&me->_midBits,sqrtf(sums[1]/MAX(1,count[1]))/20);
  setFloat(&me->_highBits,sqrtf(sums[2]/MAX(1,count[2]))/20);
 }];self.tapInstalled=YES;
 [self.engine prepare];if(![self.engine startAndReturnError:&err]){[self stop];self.status.text=err.localizedDescription;return;}
 self.running=YES;self.oldIdle=UIApplication.sharedApplication.idleTimerDisabled;UIApplication.sharedApplication.idleTimerDisabled=YES;
 [self.start setTitle:@"Stop visualizer" forState:UIControlStateNormal];
 self.timer=[NSTimer scheduledTimerWithTimeInterval:0.16 target:self selector:@selector(tick) userInfo:nil repeats:YES];
}
- (void)tick {
 if(!self.running)return;
 float b=getFloat(&_bassBits)*self.gain.value,m=getFloat(&_midBits)*self.gain.value,h=getFloat(&_highBits)*self.gain.value;
 float energy=fminf(1,fmaxf(b,fmaxf(m,h)));self.smooth=energy>self.smooth?energy:self.smooth*0.78+energy*0.22;
 float sum=b+m+h+0.00001;float dominant=(m*0.34+h*0.69)/sum;
 self.hue=fmodf(self.hue+0.006+0.02*energy,1);float hue=fmodf(dominant*0.7+self.hue*0.3,1);
 uint16_t light=(uint16_t)(fminf(1,0.015+self.smooth)*65535),hu=(uint16_t)(hue*65535),sat=60000;
 @try {
 if(self.native){
  NSString *t=self.targets[self.cursor++%self.targets.count];
  SEL s=NSSelectorFromString(@"meshClientHslSet:lightness:hue:saturation:reliable:transitionTime:delay:");
  int result=((int(*)(id,SEL,id,int,int,int,unsigned char,int,short))objc_msgSend)(NSClassFromString(@"MeshNativeHelper"),s,t,light,hu,sat,0,0,0);
  self.status.text=[NSString stringWithFormat:@"Music reactive · %lu targets · send %d",(unsigned long)self.targets.count,result];
 }else{
  // Observe the SDK queue rather than building an unbounded backlog.
  id mesh=send0(NSClassFromString(@"SigMeshLib"),@"share");id commands=send0(mesh,@"commands");
  if([commands respondsToSelector:@selector(count)] && [commands count]){self.status.text=@"Music reactive · waiting for Bluetooth";return;}
  Class sdk=NSClassFromString(@"SDKLibCommand");
  if(self.hasRGB){SEL s=NSSelectorFromString(@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:retryCount:responseMaxCount:ack:successCallback:resultCallback:");
   ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,s,0xffff,light,hu,sat,0,0,NO,nil,nil);
  }else{SEL s=NSSelectorFromString(@"lightLightnessSetWithDestination:lightness:retryCount:responseMaxCount:ack:successCallback:resultCallback:");
   if([sdk respondsToSelector:s])((id(*)(id,SEL,uint16_t,uint16_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,s,0xffff,light,0,0,NO,nil,nil);
  }
  self.status.text=@"Music reactive · all nodes in the active mesh";
 }
 }@catch(NSException *e){[self stop];self.status.text=[@"Light command failed: " stringByAppendingString:e.reason?:@"unknown error"];}
 self.window.rootViewController.view.layer.borderColor=[UIColor colorWithHue:hue saturation:0.8 brightness:MAX(0.2,self.smooth) alpha:1].CGColor;self.window.rootViewController.view.layer.borderWidth=2;
}
- (void)stop {
 BOOL was=self.running;self.running=NO;[self.timer invalidate];self.timer=nil;
 if(self.tapInstalled){[self.engine.inputNode removeTapOnBus:0];self.tapInstalled=NO;}
 [self.engine stop];self.engine=nil;
 if(was)UIApplication.sharedApplication.idleTimerDisabled=self.oldIdle;
 [AVAudioSession.sharedInstance setActive:NO withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:nil];
 [self.start setTitle:@"Start visualizer" forState:UIControlStateNormal];
 if(was)self.status.text=@"Stopped. Lights keep their last colour.";
}
@end
__attribute__((constructor)) static void loadLightShow(void){
 @autoreleasepool{
 nativeTargets=[NSMutableOrderedSet new];
 Class c=NSClassFromString(@"MeshNativeHelper");
 swizzle(c,@"meshClientHslSet:lightness:hue:saturation:reliable:transitionTime:delay:",(IMP)watchHSL,&origHSL);
 swizzle(c,@"meshClientOnOffSet:onoff:reliable:transitionTime:delay:",(IMP)watchOnOff,&origOnOff);
 swizzle(c,@"meshClientLightnessSet:lightness:reliable:transition_time:delay:",(IMP)watchLightness,&origLightness);
 dispatch_after(dispatch_time(DISPATCH_TIME_NOW,3*NSEC_PER_SEC),dispatch_get_main_queue(),^{controller=[LSPanel new];[controller install];});
 }
}
