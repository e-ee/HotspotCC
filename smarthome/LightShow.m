#import <UIKit/UIKit.h>
#import <Accelerate/Accelerate.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <stdatomic.h>
#import <math.h>
#import <dlfcn.h>

#import <mach-o/dyld.h>
#import "fishhook.h"
static NSString *LSOriginalRoot, *LSOwnRoot;
static NSUserDefaults *LSOriginalDefaults;
static NSDictionary *LSSavedPreferences;
static NSString *(*LSRealHome)(void);
static NSArray *(*LSRealSearch)(NSSearchPathDirectory,NSSearchPathDomainMask,BOOL);
static NSString *LSMapPath(NSString *p){
 if(LSOriginalRoot && LSOwnRoot && ([p isEqualToString:LSOwnRoot] || [p hasPrefix:[LSOwnRoot stringByAppendingString:@"/"]]))return [LSOriginalRoot stringByAppendingString:[p substringFromIndex:LSOwnRoot.length]];
 return p;
}
static NSString *LSHome(void){return LSOriginalRoot?:LSRealHome();}
static NSArray *LSSearch(NSSearchPathDirectory d,NSSearchPathDomainMask m,BOOL t){NSMutableArray *a=[NSMutableArray new];for(NSString *p in LSRealSearch(d,m,t))[a addObject:LSMapPath(p)];return a;}
static id LSDefaults(id c,SEL s){return LSOriginalDefaults;}
static IMP LSRealURLs;
static id LSURLs(id fm,SEL s,NSSearchPathDirectory d,NSSearchPathDomainMask m){
 NSArray *urls=((id(*)(id,SEL,NSSearchPathDirectory,NSSearchPathDomainMask))LSRealURLs)(fm,s,d,m);NSMutableArray *a=[NSMutableArray new];
 for(NSURL *u in urls)[a addObject:u.isFileURL?[NSURL fileURLWithPath:LSMapPath(u.path) isDirectory:YES]:u];return a;
}
static NSString *LSBootstrap(void){
 LSOwnRoot=NSHomeDirectory();NSFileManager *fm=NSFileManager.defaultManager;
 NSString *parent=@"/var/mobile/Containers/Data/Application";
 for(NSString *uuid in [fm contentsOfDirectoryAtPath:parent error:nil]){
  NSString *p=[parent stringByAppendingPathComponent:uuid];
  NSDictionary *meta=[NSDictionary dictionaryWithContentsOfFile:[p stringByAppendingPathComponent:@".com.apple.mobile_container_manager.metadata.plist"]];
  if([meta[@"MCMMetadataIdentifier"] isEqual:@"com.ledvance.smart"]){LSOriginalRoot=p;break;}
 }
 if(!LSOriginalRoot)return @"Original SYLVANIA data is not accessible. Pairing has not been changed.";
 // Reuse the existing state, including mesh sequence numbers. Do not clone it.
 struct rebinding bindings[]={{"NSHomeDirectory",LSHome,(void **)&LSRealHome},{"NSSearchPathForDirectoriesInDomains",LSSearch,(void **)&LSRealSearch}};
 rebind_symbols_image((void *)_dyld_get_image_header(0),_dyld_get_image_vmaddr_slide(0),bindings,2);
 Method urls=class_getInstanceMethod(NSFileManager.class,@selector(URLsForDirectory:inDomains:));LSRealURLs=method_setImplementation(urls,(IMP)LSURLs);
 LSSavedPreferences=[NSDictionary dictionaryWithContentsOfFile:[LSOriginalRoot stringByAppendingPathComponent:@"Library/Preferences/com.ledvance.smart.plist"]];
 if(![LSSavedPreferences[@"mesh.json"] isKindOfClass:NSData.class])return @"The original app has no readable saved mesh configuration.";
 LSOriginalDefaults=[[NSUserDefaults alloc]initWithSuiteName:@"com.ledvance.smart"];
 [LSOriginalDefaults registerDefaults:LSSavedPreferences];
 if(LSOriginalDefaults){Method m=class_getClassMethod(NSUserDefaults.class,@selector(standardUserDefaults));method_setImplementation(m,(IMP)LSDefaults);}
 return nil;
}
static NSString *LSBootstrapError;

static id send0(id x, NSString *s) { SEL q=NSSelectorFromString(s); return [x respondsToSelector:q]?((id(*)(id,SEL))objc_msgSend)(x,q):nil; }
static NSMutableOrderedSet<NSString *> *nativeTargets;
static IMP origHSL, origOnOff, origLightness;
static void remember(id x) {if([x isKindOfClass:NSString.class] && [x length]) @synchronized(nativeTargets){[nativeTargets addObject:x];}}
static int watchHSL(id c,SEL s,id t,int l,int h,int sat,unsigned char r,int tr,short d){remember(t);return ((int(*)(id,SEL,id,int,int,int,unsigned char,int,short))origHSL)(c,s,t,l,h,sat,r,tr,d);}
static int watchOnOff(id c,SEL s,id t,unsigned char on,unsigned char r,int tr,short d){remember(t);return ((int(*)(id,SEL,id,unsigned char,unsigned char,int,short))origOnOff)(c,s,t,on,r,tr,d);}
static int watchLightness(id c,SEL s,id t,int l,unsigned char r,int tr,short d){remember(t);return ((int(*)(id,SEL,id,int,unsigned char,int,short))origLightness)(c,s,t,l,r,tr,d);}
static void swizzle(Class c,NSString *s,IMP replacement,IMP *original){ Method m=class_getClassMethod(c,NSSelectorFromString(s));if(m)*original=method_setImplementation(m,replacement); }

@interface LSSpectrum : UIView
@property(nonatomic) float bass, mid, high, phase;
@property(nonatomic) BOOL audio;
@end
@implementation LSSpectrum
- (void)drawRect:(CGRect)rect {
 CGContextRef ctx=UIGraphicsGetCurrentContext();CGFloat gap=4,w=(rect.size.width-31*gap)/32;
 for(int i=0;i<32;i++){
  float e=i<10?self.bass:(i<22?self.mid:self.high);
  float wave=(0.45+0.55*powf(sinf(i*0.21+self.phase),2));
  CGFloat height=MAX(3,MIN(rect.size.height,rect.size.height*e*wave));
  CGRect r=CGRectMake(i*(w+gap),(rect.size.height-height)/2,w,height);
  UIColor *c=[UIColor colorWithHue:fmodf(0.64+i/120.0+self.phase*0.006,1) saturation:0.6 brightness:0.95 alpha:0.6+0.4*e];
  CGContextSetFillColorWithColor(ctx,c.CGColor);UIBezierPath *path=[UIBezierPath bezierPathWithRoundedRect:r cornerRadius:w/2];CGContextAddPath(ctx,path.CGPath);CGContextFillPath(ctx);
 }
}
@end

@interface LSPanel : NSObject {
@public atomic_uint _bassBits, _midBits, _highBits;
}
@property(nonatomic,strong) UIWindow *window;
@property(nonatomic,strong) LSSpectrum *spectrum;
@property(nonatomic,strong) UISegmentedControl *mode;
@property(nonatomic,strong) UISegmentedControl *palette;
@property(nonatomic,strong) UILabel *track;
@property(nonatomic,strong) NSTimer *mediaTimer;
@property(nonatomic,strong) NSDictionary *mediaInfo;
@property(nonatomic) BOOL playing;
@property(nonatomic) BOOL userPaused;
@property(nonatomic) BOOL meshPrepared;
@property(nonatomic) BOOL meshConnecting;
@property(nonatomic) BOOL meshConnected;
@property(nonatomic) NSUInteger savedBulbCount;
@property(nonatomic,strong) UILabel *savedGroups;
@property(nonatomic) NSTimeInterval lastConnectAttempt;
@property(nonatomic) BOOL expanded;
@property(nonatomic) float artworkHue;
@property(nonatomic) NSTimeInterval mediaReceived;
@property(nonatomic) double elapsed;
@property(nonatomic) double tempo;
@property(nonatomic) double rate;
@property(nonatomic,strong) UIView *panel;
@property(nonatomic,strong) UIButton *start;
@property(nonatomic,strong) UILabel *status;
@property(nonatomic,strong) UISlider *gain;
@property(nonatomic,strong) NSTimer *timer;
@property(nonatomic,strong) NSArray<NSString *> *targets;
@property(nonatomic) BOOL running;
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
- (UILabel *)label:(NSString *)text frame:(CGRect)f size:(CGFloat)size weight:(UIFontWeight)weight color:(UIColor *)c {
 UILabel *l=[[UILabel alloc]initWithFrame:f];l.text=text;l.font=[UIFont systemFontOfSize:size weight:weight];l.textColor=c;[self.panel addSubview:l];return l;
}
- (void)install {
 if(self.window)return;
 UIWindow *host=UIApplication.sharedApplication.keyWindow;
 if(!host)for(UIWindow *w in UIApplication.sharedApplication.windows)if(w.windowLevel==UIWindowLevelNormal){host=w;break;}
 if(!host){dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC),dispatch_get_main_queue(),^{[self install];});return;}
 CGFloat width=MIN(host.bounds.size.width-24,410),height=MIN(424,host.bounds.size.height-100);
 if(@available(iOS 13.0,*)){if(host.windowScene)self.window=[[UIWindow alloc]initWithWindowScene:host.windowScene];}
 if(!self.window)self.window=[[UIWindow alloc]initWithFrame:CGRectZero];
 self.window.frame=host.bounds;
 width=host.bounds.size.width; height=host.bounds.size.height;
 self.window.windowLevel=UIWindowLevelAlert+1;
 UIViewController *vc=[UIViewController new];self.window.rootViewController=vc;vc.view.backgroundColor=[UIColor colorWithRed:0.035 green:0.043 blue:0.065 alpha:0.98];vc.view.layer.cornerRadius=28;vc.view.clipsToBounds=YES;
 UIScrollView *content=[[UIScrollView alloc]initWithFrame:CGRectMake(0,host.safeAreaInsets.top+10,width,height-host.safeAreaInsets.top-host.safeAreaInsets.bottom-10)];content.contentSize=CGSizeMake(width,720);content.showsVerticalScrollIndicator=NO;self.panel=content;[vc.view addSubview:content];
 self.window.layer.shadowColor=UIColor.blackColor.CGColor;self.window.layer.shadowRadius=24;self.window.layer.shadowOpacity=0.4;self.window.layer.shadowOffset=CGSizeMake(0,8);
 CAGradientLayer *gl=[CAGradientLayer layer];gl.frame=CGRectMake(0,0,width,height);gl.colors=@[(id)[UIColor colorWithRed:0.17 green:0.12 blue:0.30 alpha:1].CGColor,(id)[UIColor colorWithRed:0.035 green:0.043 blue:0.065 alpha:1].CGColor];gl.startPoint=CGPointZero;gl.endPoint=CGPointMake(0.9,0.8);[vc.view.layer insertSublayer:gl atIndex:0];
 [self label:@"LIGHT SHOW" frame:CGRectMake(22,16,width-85,26) size:17 weight:UIFontWeightBold color:UIColor.whiteColor];
 UIButton *fold=[UIButton buttonWithType:UIButtonTypeSystem];fold.frame=CGRectMake(width-58,10,42,38);fold.hidden=YES;[fold setTitle:@"−" forState:UIControlStateNormal];fold.tintColor=UIColor.whiteColor;fold.titleLabel.font=[UIFont systemFontOfSize:28 weight:UIFontWeightLight];[fold addTarget:self action:@selector(fold:) forControlEvents:UIControlEventTouchUpInside];[self.panel addSubview:fold];
 [self label:@"YOUR MUSIC, IN COLOUR" frame:CGRectMake(22,58,width-44,18) size:10 weight:UIFontWeightSemibold color:[UIColor colorWithWhite:0.65 alpha:1]];
 self.track=[self label:@"Waiting for your music" frame:CGRectMake(22,83,width-44,28) size:23 weight:UIFontWeightSemibold color:UIColor.whiteColor];self.track.adjustsFontSizeToFitWidth=YES;self.track.minimumScaleFactor=0.6;
 self.status=[self label:LSBootstrapError?:@"Uses the bulbs already connected in SYLVANIA." frame:CGRectMake(22,114,width-44,36) size:12 weight:UIFontWeightRegular color:[UIColor colorWithWhite:0.65 alpha:1]];self.status.numberOfLines=2;
 self.spectrum=[[LSSpectrum alloc]initWithFrame:CGRectMake(22,158,width-44,70)];self.spectrum.backgroundColor=UIColor.clearColor;self.spectrum.bass=0.25;self.spectrum.mid=0.35;self.spectrum.high=0.2;[self.panel addSubview:self.spectrum];
 [self label:@"NOW PLAYING · AUTOMATIC" frame:CGRectMake(22,246,width-44,22) size:12 weight:UIFontWeightSemibold color:[UIColor colorWithRed:0.76 green:0.68 blue:1 alpha:1]];
 self.palette=[[UISegmentedControl alloc]initWithItems:@[@"Artwork",@"Aurora",@"Ember"]];self.palette.frame=CGRectMake(22,285,width-44,28);self.palette.selectedSegmentIndex=0;[self.panel addSubview:self.palette];
 [self label:@"INTENSITY" frame:CGRectMake(22,323,100,16) size:10 weight:UIFontWeightSemibold color:[UIColor colorWithWhite:0.6 alpha:1]];
 self.gain=[[UISlider alloc]initWithFrame:CGRectMake(126,317,width-148,26)];self.gain.minimumValue=0.5;self.gain.maximumValue=8;self.gain.value=2.5;self.gain.tintColor=[UIColor colorWithRed:0.64 green:0.52 blue:1 alpha:1];[self.panel addSubview:self.gain];
 self.start=[UIButton buttonWithType:UIButtonTypeSystem];self.start.frame=CGRectMake(22,359,width-44,46);self.start.backgroundColor=[UIColor colorWithRed:0.76 green:0.68 blue:1 alpha:1];self.start.layer.cornerRadius=15;[self.start setTitleColor:[UIColor colorWithWhite:0.08 alpha:1] forState:UIControlStateNormal];[self.start setTitle:@"Start light show" forState:UIControlStateNormal];self.start.titleLabel.font=[UIFont systemFontOfSize:16 weight:UIFontWeightSemibold];[self.start addTarget:self action:@selector(toggle) forControlEvents:UIControlEventTouchUpInside];[self.panel addSubview:self.start];
 self.savedGroups=[self label:@"Loading your saved groups…" frame:CGRectMake(22,440,width-44,240) size:17 weight:UIFontWeightMedium color:[UIColor colorWithWhite:0.75 alpha:1]];self.savedGroups.numberOfLines=0;
 self.expanded=YES;self.artworkHue=0.7;self.tempo=120;
 [self.window setHidden:NO];
 [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(stop) name:UIApplicationWillResignActiveNotification object:nil];

 [self connectSavedMesh];
 [self refreshMedia];self.mediaTimer=[NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(refreshMedia) userInfo:nil repeats:YES];
}
- (void)fold:(UIButton *)button {
 self.expanded=!self.expanded;CGRect f=self.window.frame;CGFloat h=self.expanded?424:56;f.origin.y+=f.size.height-h;f.size.height=h;
 [UIView animateWithDuration:0.25 animations:^{self.window.frame=f;}];[button setTitle:self.expanded?@"−":@"+" forState:UIControlStateNormal];
}
- (id)mediaValue:(NSDictionary *)d suffix:(NSString *)suffix {
 id value=d[[@"kMRMediaRemoteNowPlayingInfo" stringByAppendingString:suffix]];
 if(value)return value;
 value=d[suffix];if(value)return value;
 for(id k in d)if([[k description]hasSuffix:[@"Info" stringByAppendingString:suffix]])return d[k];return nil;
}
- (void)refreshMedia {
 [self connectSavedMesh];
 static void *handle;static void (*getInfo)(dispatch_queue_t,void(^)(CFDictionaryRef));static void (*getPlaying)(dispatch_queue_t,void(^)(BOOL));
 static dispatch_once_t once;dispatch_once(&once,^{
  handle=dlopen("/System/Library/PrivateFrameworks/MediaRemote.framework/MediaRemote",RTLD_NOW);
  if(handle){getInfo=dlsym(handle,"MRMediaRemoteGetNowPlayingInfo");getPlaying=dlsym(handle,"MRMediaRemoteGetNowPlayingApplicationIsPlaying");}
 });
 if(!getInfo){self.status.text=@"Now Playing is unavailable on this device.";return;}
 if(getPlaying)getPlaying(dispatch_get_main_queue(),^(BOOL playing){self.playing=playing;});
 getInfo(dispatch_get_main_queue(),^(CFDictionaryRef raw){
  NSDictionary *d=(__bridge NSDictionary *)raw;
  if(![d isKindOfClass:NSDictionary.class] || !d.count){self.track.text=@"Play your music";self.playing=NO;return;}
  NSString *title=[self mediaValue:d suffix:@"Title"],*artist=[self mediaValue:d suffix:@"Artist"];
  self.track.text=[title isKindOfClass:NSString.class]?title:@"Now Playing";
  NSNumber *rate=[self mediaValue:d suffix:@"PlaybackRate"];
  if([rate respondsToSelector:@selector(doubleValue)]){self.rate=rate.doubleValue;if(!getPlaying)self.playing=self.rate>0;}
  NSNumber *elapsed=[self mediaValue:d suffix:@"ElapsedTime"];self.elapsed=[elapsed respondsToSelector:@selector(doubleValue)]?elapsed.doubleValue:0;
  NSDate *stamp=[self mediaValue:d suffix:@"Timestamp"];
  if([stamp isKindOfClass:NSDate.class])self.elapsed+=MAX(0,-stamp.timeIntervalSinceNow)*MAX(0,self.rate);
  self.mediaReceived=NSProcessInfo.processInfo.systemUptime;
  // Metadata sometimes contains BPM; otherwise this is a labelled playback pulse, not measured beats.
  id bpm=[self mediaValue:d suffix:@"BeatsPerMinute"];double tempo=[bpm respondsToSelector:@selector(doubleValue)]?[bpm doubleValue]:0;self.tempo=tempo>=40&&tempo<=240?tempo:120;
  NSData *art=[self mediaValue:d suffix:@"ArtworkData"];
  NSData *old=[self mediaValue:self.mediaInfo suffix:@"ArtworkData"];
  if([art isKindOfClass:NSData.class] && ![art isEqual:old]){
   UIImage *image=[UIImage imageWithData:art];CGImageRef cg=image.CGImage;
   if(cg){unsigned char pixel[16*16*4]={0};CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();CGContextRef ctx=CGBitmapContextCreate(pixel,16,16,8,64,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    if(ctx){CGContextDrawImage(ctx,CGRectMake(0,0,16,16),cg);float best=0,hue=self.artworkHue;
     for(int i=0;i<256;i++){CGFloat r=pixel[i*4]/255.0,g=pixel[i*4+1]/255.0,b=pixel[i*4+2]/255.0,h,s,v,a;[[UIColor colorWithRed:r green:g blue:b alpha:1]getHue:&h saturation:&s brightness:&v alpha:&a];float score=s*v;if(score>best){best=score;hue=h;}}
     self.artworkHue=hue;CGContextRelease(ctx);}
    CGColorSpaceRelease(space);
   }
  }
  self.mediaInfo=d.copy;
  if(!self.running && !self.userPaused && self.mode.selectedSegmentIndex==0 && self.playing)[self begin];
  if(!self.running && !self.userPaused && !LSBootstrapError)self.status.text=[artist isKindOfClass:NSString.class]?artist:@"Ready · starts when music plays";
 });
}

- (BOOL)prepareSavedMesh {
 if(self.meshPrepared)return YES;
 if(LSBootstrapError){self.status.text=LSBootstrapError;return NO;}
 // Read the live original preferences again, never embed or clone the uploaded snapshot.
 NSDictionary *prefs=[NSDictionary dictionaryWithContentsOfFile:[LSOriginalRoot stringByAppendingPathComponent:@"Library/Preferences/com.ledvance.smart.plist"]];
 NSData *data=prefs[@"mesh.json"];
 if(![data isKindOfClass:NSData.class]){self.status.text=@"The original saved mesh could not be read.";return NO;}
 NSDictionary *mesh=[NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
 if(![mesh isKindOfClass:NSDictionary.class] || ![mesh[@"nodes"] isKindOfClass:NSArray.class] || ![mesh[@"netKeys"] count]){self.status.text=@"The original mesh configuration is incomplete.";return NO;}
 NSString *uuid=prefs[@"currenProvisionerUUID_key"];
 if(![uuid isKindOfClass:NSString.class] || !uuid.length){self.status.text=@"The saved controller identity is unavailable.";return NO;}
 [LSOriginalDefaults registerDefaults:prefs];
 id ds=send0(NSClassFromString(@"SigDataSource"),@"share");
 SEL set=NSSelectorFromString(@"setDictionaryToDataSource:");
 if(![ds respondsToSelector:set]){self.status.text=@"The lighting backend could not initialize.";return NO;}
 @try {
  ((void(*)(id,SEL,id))objc_msgSend)(ds,set,mesh);
  ((void(*)(id,SEL,id))objc_msgSend)(ds,NSSelectorFromString(@"saveCurrentProvisionerUUID:"),uuid);
  // Every lighting model in the supplied configuration is bound to app-key index 0.
  for(id key in send0(ds,@"appKeys"))if([[key valueForKey:@"index"] unsignedIntegerValue]==0){((void(*)(id,SEL,id))objc_msgSend)(ds,NSSelectorFromString(@"setCurAppkeyModel:"),key);break;}
  NSArray *nodes=send0(ds,@"curNodes");NSUInteger count=0;
  for(id n in nodes)if([send0(n,@"HSLAddresses") count] || [send0(n,@"lightnessAddresses") count])count++;
  self.savedBulbCount=count;self.meshPrepared=count>0;
  NSMutableArray *names=[NSMutableArray new];
  for(NSDictionary *group in mesh[@"groups"])if([group[@"name"] isKindOfClass:NSString.class])[names addObject:group[@"name"]];
  self.savedGroups.text=[NSString stringWithFormat:@"YOUR SAVED GROUPS\n\n%@",[names componentsJoinedByString:@"\n\n"]];
  self.status.text=[NSString stringWithFormat:@"%lu saved lights · connecting",(unsigned long)count];
 }@catch(NSException *e){self.status.text=@"The saved light network could not be loaded.";return NO;}
 return self.meshPrepared;
}
- (void)connectSavedMesh {
 if(![self prepareSavedMesh])return;
 send0(NSClassFromString(@"SigMeshLib"),@"share");
 id bearer=send0(NSClassFromString(@"SigBearer"),@"share");
 if([bearer respondsToSelector:NSSelectorFromString(@"isOpen")])self.meshConnected=((BOOL(*)(id,SEL))objc_msgSend)(bearer,NSSelectorFromString(@"isOpen"));
 if(self.meshConnected || self.meshConnecting)return;
 if(NSProcessInfo.processInfo.systemUptime-self.lastConnectAttempt<8)return;
 self.meshConnecting=YES;self.lastConnectAttempt=NSProcessInfo.processInfo.systemUptime;
 SEL start=NSSelectorFromString(@"startMeshConnectWithComplete:");
 if(![bearer respondsToSelector:start]){self.meshConnecting=NO;return;}
 __weak LSPanel *weak=self;
 void (^completion)(BOOL)=^(BOOL ok){dispatch_async(dispatch_get_main_queue(),^{LSPanel *me=weak;if(!me)return;me.meshConnecting=NO;me.meshConnected=ok;me.status.text=ok?[NSString stringWithFormat:@"%lu lights connected · ready",(unsigned long)me.savedBulbCount]:@"Looking for your saved lights over Bluetooth";});};
 ((void(*)(id,SEL,id))objc_msgSend)(bearer,start,completion);
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
 if(self.running){self.userPaused=YES;[self stop];return;}
 self.userPaused=NO;[self begin];
}
- (void)begin {
 if(self.running)return;
 if(![self prepareSavedMesh])return;
 [self connectSavedMesh];
 if(!self.meshConnected)return;
 [self discover];
 if(self.native && !self.targets.count){self.status.text=@"Loading your existing light groups.";return;}
 if(!self.native && !self.hasRGB && !self.hasWhite){self.status.text=@"Waiting for your existing light network to load.";return;}
 if(self.mode.selectedSegmentIndex==0){
  self.running=YES;self.oldIdle=UIApplication.sharedApplication.idleTimerDisabled;UIApplication.sharedApplication.idleTimerDisabled=YES;
  [self.start setTitle:@"Pause light show" forState:UIControlStateNormal];
  self.timer=[NSTimer scheduledTimerWithTimeInterval:0.16 target:self selector:@selector(tick) userInfo:nil repeats:YES];return;
 }
}
- (void)tick {
 if(!self.running)return;
 id bearer=send0(NSClassFromString(@"SigBearer"),@"share");
 if([bearer respondsToSelector:NSSelectorFromString(@"isOpen")] && !((BOOL(*)(id,SEL))objc_msgSend)(bearer,NSSelectorFromString(@"isOpen"))){self.meshConnected=NO;self.status.text=@"Reconnecting to your saved lights";[self connectSavedMesh];return;}
 if(self.mode.selectedSegmentIndex==0 && !self.playing){self.status.text=@"Music paused · waiting";return;}
 float b=getFloat(&_bassBits)*self.gain.value,m=getFloat(&_midBits)*self.gain.value,h=getFloat(&_highBits)*self.gain.value;
 if(self.mode.selectedSegmentIndex==0){
  double pos=self.elapsed+(NSProcessInfo.processInfo.systemUptime-self.mediaReceived)*MAX(0,self.rate);
  double phase=fmod(pos*self.tempo/60.0,1.0);float pulse=expf(-5.5*phase);
  b=0.10+0.75*pulse;m=0.12+0.40*pulse;h=0.08+0.25*pulse;
 }
 self.spectrum.bass=fminf(1,b);self.spectrum.mid=fminf(1,m);self.spectrum.high=fminf(1,h);self.spectrum.phase+=0.19;[self.spectrum setNeedsDisplay];
 float energy=fminf(1,fmaxf(b,fmaxf(m,h)));self.smooth=energy>self.smooth?energy:self.smooth*0.78+energy*0.22;
 float sum=b+m+h+0.00001;float dominant=(m*0.34+h*0.69)/sum;
 self.hue=fmodf(self.hue+0.006+0.02*energy,1);float hue=fmodf(dominant*0.7+self.hue*0.3,1);
 if(self.mode.selectedSegmentIndex==0)hue=self.artworkHue;
 if(self.palette.selectedSegmentIndex==1)hue=fmodf(self.hue,1);
 if(self.palette.selectedSegmentIndex==2)hue=0.015+0.075*energy;
 uint16_t light=(uint16_t)(fminf(1,0.015+self.smooth)*65535),hu=(uint16_t)(hue*65535),sat=60000;
 @try {
 if(self.native){
  NSString *t=self.targets[self.cursor++%self.targets.count];
  SEL s=NSSelectorFromString(@"meshClientHslSet:lightness:hue:saturation:reliable:transitionTime:delay:");
  int result=((int(*)(id,SEL,id,int,int,int,unsigned char,int,short))objc_msgSend)(NSClassFromString(@"MeshNativeHelper"),s,t,light,hu,sat,0,0,0);
  self.status.text=self.mode.selectedSegmentIndex==0?@"Following Now Playing · playback pulse":@"Following Now Playing · playback pulse"; (void)result;
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
  self.status.text=self.mode.selectedSegmentIndex==0?@"Following Now Playing · playback pulse":@"Following Now Playing · playback pulse";
 }
 }@catch(NSException *e){[self stop];self.status.text=[@"Light command failed: " stringByAppendingString:e.reason?:@"unknown error"];}
 self.window.rootViewController.view.layer.borderColor=[UIColor colorWithHue:hue saturation:0.8 brightness:MAX(0.2,self.smooth) alpha:1].CGColor;self.window.rootViewController.view.layer.borderWidth=2;
}
- (void)stop {
 BOOL was=self.running;self.running=NO;[self.timer invalidate];self.timer=nil;
 
 
 if(was)UIApplication.sharedApplication.idleTimerDisabled=self.oldIdle;
 
 [self.start setTitle:@"Start visualizer" forState:UIControlStateNormal];
 if(was)self.status.text=@"Stopped. Lights keep their last colour.";
}
@end
__attribute__((constructor)) static void loadLightShow(void){
 @autoreleasepool{
 LSBootstrapError=LSBootstrap();
 nativeTargets=[NSMutableOrderedSet new];
 Class c=NSClassFromString(@"MeshNativeHelper");
 swizzle(c,@"meshClientHslSet:lightness:hue:saturation:reliable:transitionTime:delay:",(IMP)watchHSL,&origHSL);
 swizzle(c,@"meshClientOnOffSet:onoff:reliable:transitionTime:delay:",(IMP)watchOnOff,&origOnOff);
 swizzle(c,@"meshClientLightnessSet:lightness:reliable:transition_time:delay:",(IMP)watchLightness,&origLightness);
 dispatch_after(dispatch_time(DISPATCH_TIME_NOW,3*NSEC_PER_SEC),dispatch_get_main_queue(),^{controller=[LSPanel new];[controller install];});
 }
}
