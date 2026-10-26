#import <UIKit/UIKit.h>
#import <Accelerate/Accelerate.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <stdatomic.h>
#import <math.h>
#import <dlfcn.h>
#import <arpa/inet.h>
#import <sys/socket.h>
#import <fcntl.h>
#import <unistd.h>
#import "MSHFProtocol.h"
#import "BeatCore.h"

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
static NSDictionary *LSReadPreferences(void){return [LSOriginalDefaults persistentDomainForName:@"com.ledvance.smart.lightshow.mesh"];}
static NSString *LSBootstrap(void){
 LSOwnRoot=NSHomeDirectory();LSOriginalRoot=LSOwnRoot;
 NSDictionary *seed=[NSDictionary dictionaryWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"LightShowSeed" withExtension:@"plist"]];
 LSOriginalDefaults=[[NSUserDefaults alloc]initWithSuiteName:@"com.ledvance.smart.lightshow.mesh"];
 NSDictionary *existing=LSReadPreferences();
 if(![existing[@"mesh.json"] isKindOfClass:NSData.class]){
  if(![seed[@"mesh.json"] isKindOfClass:NSData.class])return @"The private saved-light configuration is missing from this build.";
  for(NSString *key in seed)[LSOriginalDefaults setObject:seed[key] forKey:key];
  [LSOriginalDefaults synchronize];
 }
 LSSavedPreferences=LSReadPreferences();
 Method defaults=class_getClassMethod(NSUserDefaults.class,@selector(standardUserDefaults));method_setImplementation(defaults,(IMP)LSDefaults);
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
 CGContextRef ctx=UIGraphicsGetCurrentContext();
 for(int strand=0;strand<3;strand++){
  float e=strand==0?self.bass:(strand==1?self.mid:self.high);
  UIColor *c=[UIColor colorWithHue:fmodf(0.64+strand*0.09+self.phase*0.003,1) saturation:0.55 brightness:0.95 alpha:0.45+0.35*e];
  CGContextSetStrokeColorWithColor(ctx,c.CGColor);CGContextSetLineWidth(ctx,strand==0?2.5:1.5);
  UIBezierPath *path=[UIBezierPath bezierPath];
  for(int i=0;i<=100;i++){CGFloat x=rect.size.width*i/100.0,y=rect.size.height*0.5+sinf(i*0.065+self.phase+strand*0.8)*rect.size.height*0.28*e;if(i==0)[path moveToPoint:CGPointMake(x,y)];else[path addLineToPoint:CGPointMake(x,y)];}
  CGContextAddPath(ctx,path.CGPath);CGContextStrokePath(ctx);
 }

}
@end

@interface LSPanel : NSObject {
@public atomic_uint _bassBits, _midBits, _highBits;
 LSBeatState _beatState;
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
@property(nonatomic,strong) UIButton *helperButton;
@property(nonatomic,strong) NSTimer *audioTimer;
@property(nonatomic) int audioSocket;
@property(nonatomic) uint64_t audioSession,audioServer,audioRequest,audioSequence,audioEpoch;
@property(nonatomic) double audioLastFrame,lastAudioSubscribe;
@property(nonatomic) BOOL helperSeen;
@property(nonatomic,strong) UIImageView *artwork;
@property(nonatomic,strong) NSArray *livingZones;
@property(nonatomic,strong) NSArray *supportZones;
@property(nonatomic,strong) NSDictionary *tvZone;
@property(nonatomic,strong) NSDictionary *cornerZone;
@property(nonatomic) NSUInteger spatialTick;
@property(nonatomic) NSUInteger supportCursor;
@property(nonatomic) NSUInteger livingCursor;
@property(nonatomic) double sceneStarted;
@property(nonatomic) float sceneHue;
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
 [self label:@"BEAT ROOM" frame:CGRectMake(22,16,width-85,26) size:17 weight:UIFontWeightBold color:UIColor.whiteColor];
 UIButton *fold=[UIButton buttonWithType:UIButtonTypeSystem];fold.frame=CGRectMake(width-58,10,42,38);fold.hidden=YES;[fold setTitle:@"−" forState:UIControlStateNormal];fold.tintColor=UIColor.whiteColor;fold.titleLabel.font=[UIFont systemFontOfSize:28 weight:UIFontWeightLight];[fold addTarget:self action:@selector(fold:) forControlEvents:UIControlEventTouchUpInside];[self.panel addSubview:fold];
 [self label:@"ALL LIGHTS · REAL AUDIO" frame:CGRectMake(22,58,width-44,18) size:10 weight:UIFontWeightSemibold color:[UIColor colorWithWhite:0.65 alpha:1]];
 self.artwork=[[UIImageView alloc]initWithFrame:CGRectMake(22,83,76,76)];self.artwork.contentMode=UIViewContentModeScaleAspectFill;self.artwork.layer.cornerRadius=16;self.artwork.clipsToBounds=YES;self.artwork.backgroundColor=[UIColor colorWithWhite:0.13 alpha:1];[self.panel addSubview:self.artwork];
 self.track=[self label:@"Waiting for your music" frame:CGRectMake(114,83,width-136,28) size:23 weight:UIFontWeightSemibold color:UIColor.whiteColor];self.track.adjustsFontSizeToFitWidth=YES;self.track.minimumScaleFactor=0.6;
 self.status=[self label:LSBootstrapError?:@"Uses the bulbs already connected in SYLVANIA." frame:CGRectMake(114,118,width-136,42) size:12 weight:UIFontWeightRegular color:[UIColor colorWithWhite:0.65 alpha:1]];self.status.numberOfLines=2;
 self.spectrum=[[LSSpectrum alloc]initWithFrame:CGRectMake(22,178,width-44,50)];self.spectrum.backgroundColor=UIColor.clearColor;self.spectrum.bass=0.25;self.spectrum.mid=0.35;self.spectrum.high=0.2;[self.panel addSubview:self.spectrum];
 [self label:@"BEAT SYNC · SYSTEM AUDIO" frame:CGRectMake(22,246,width-44,22) size:12 weight:UIFontWeightSemibold color:[UIColor colorWithRed:0.76 green:0.68 blue:1 alpha:1]];
 self.palette=[[UISegmentedControl alloc]initWithItems:@[@"Pulse",@"Spectrum",@"Prism"]];self.palette.frame=CGRectMake(22,285,width-44,28);self.palette.selectedSegmentIndex=2;[self.panel addSubview:self.palette];
 [self label:@"BRIGHTNESS" frame:CGRectMake(22,323,100,16) size:10 weight:UIFontWeightSemibold color:[UIColor colorWithWhite:0.6 alpha:1]];
 self.gain=[[UISlider alloc]initWithFrame:CGRectMake(126,317,width-148,26)];self.gain.minimumValue=0.5;self.gain.maximumValue=8;self.gain.value=2.5;self.gain.tintColor=[UIColor colorWithRed:0.64 green:0.52 blue:1 alpha:1];[self.panel addSubview:self.gain];
 self.start=[UIButton buttonWithType:UIButtonTypeSystem];self.start.frame=CGRectMake(22,359,width-44,46);self.start.backgroundColor=[UIColor colorWithRed:0.76 green:0.68 blue:1 alpha:1];self.start.layer.cornerRadius=15;[self.start setTitleColor:[UIColor colorWithWhite:0.08 alpha:1] forState:UIControlStateNormal];[self.start setTitle:@"Start light show" forState:UIControlStateNormal];self.start.titleLabel.font=[UIFont systemFontOfSize:16 weight:UIFontWeightSemibold];[self.start addTarget:self action:@selector(toggle) forControlEvents:UIControlEventTouchUpInside];[self.panel addSubview:self.start];
 self.helperButton=[UIButton buttonWithType:UIButtonTypeSystem];self.helperButton.frame=CGRectMake(22,430,width-44,48);[self.helperButton setTitle:@"Install audio helper" forState:UIControlStateNormal];self.helperButton.tintColor=[UIColor colorWithRed:0.76 green:0.68 blue:1 alpha:1];[self.helperButton addTarget:self action:@selector(openAudioHelper) forControlEvents:UIControlEventTouchUpInside];[self.panel addSubview:self.helperButton];
 self.expanded=YES;self.artworkHue=0.7;self.tempo=0;
 [self.window setHidden:NO];
 [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(resumeLightShow) name:UIApplicationDidBecomeActiveNotification object:nil];

 [self connectSavedMesh];
 [self setupAudioClient];
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
- (void)setupAudioClient {
 self.audioSocket=socket(AF_INET,SOCK_DGRAM,0);if(self.audioSocket<0)return;
 fcntl(self.audioSocket,F_SETFL,O_NONBLOCK);struct sockaddr_in local={0};local.sin_len=sizeof(local);local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(bind(self.audioSocket,(struct sockaddr *)&local,sizeof(local))<0){close(self.audioSocket);self.audioSocket=-1;return;}
 arc4random_buf(&_audioSession,sizeof(_audioSession));if(!self.audioSession)self.audioSession=1;
 LSBeatInit(&_beatState);
 self.audioTimer=[NSTimer timerWithTimeInterval:0.025 target:self selector:@selector(pollAudio) userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:self.audioTimer forMode:NSRunLoopCommonModes];
}
- (void)pollAudio {
 if(self.audioSocket<0||self.userPaused)return;double now=NSProcessInfo.processInfo.systemUptime;
 if(now-self.lastAudioSubscribe>4){
  if(now-self.audioLastFrame>2)self.audioServer=0;
  uint8_t packet[MSHF_PROTOCOL_HEADER_SIZE+MSHF_PROTOCOL_SUBSCRIBE_PAYLOAD_SIZE]={0};
  MSHFEncodeHeader(packet,MSHFMessageTypeSubscribe,8,self.audioSession,self.audioServer,++_audioRequest);
  MSHFWriteU32(packet+32,MSHFFeatureMaskSpectrum);MSHFWriteU16(packet+36,40);MSHFWriteU16(packet+38,0);
  struct sockaddr_in server={0};server.sin_len=sizeof(server);server.sin_family=AF_INET;server.sin_port=htons(MSHF_PROTOCOL_PORT);server.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  sendto(self.audioSocket,packet,sizeof(packet),0,(struct sockaddr *)&server,sizeof(server));self.lastAudioSubscribe=now;
 }
 for(int packetCount=0;packetCount<16;packetCount++){
  uint8_t packet[MSHF_PROTOCOL_MAX_PACKET_SIZE];struct sockaddr_in from={0};socklen_t size=sizeof(from);ssize_t n=recvfrom(self.audioSocket,packet,sizeof(packet),0,(struct sockaddr *)&from,&size);if(n<=0)break;
  if(from.sin_family!=AF_INET||from.sin_addr.s_addr!=htonl(INADDR_LOOPBACK)||from.sin_port!=htons(MSHF_PROTOCOL_PORT))continue;
  MSHFPacketHeader h;if(!MSHFDecodeHeader(packet,n,&h)||h.type!=MSHFMessageTypeFeature||h.clientSessionID!=self.audioSession||!h.serverInstanceID||!h.sequence||h.payloadLength<28)continue;
  if(h.serverInstanceID!=self.audioServer){self.audioServer=h.serverInstanceID;self.audioSequence=0;LSBeatInit(&_beatState);}
  if(h.sequence<=self.audioSequence)continue;const uint8_t *p=packet+32;uint32_t mask=MSHFReadU32(p+8),status=MSHFReadU32(p+12);
  if(!(mask&MSHFFeatureMaskSpectrum)||h.payloadLength!=MSHFFeaturePayloadLength(mask)||!MSHFReadU64(p))continue;
  float sampleRate=MSHFReadF32(p+16),rms=MSHFReadF32(p+20);if(!isfinite(sampleRate)||sampleRate<=0||!isfinite(rms))continue;
  const uint8_t *bins=p+28+((mask&MSHFFeatureMaskWaveform)?64*4:0);float spectrum[64];BOOL valid=YES;
  for(int i=0;i<64;i++){spectrum[i]=MSHFReadF32(bins+i*4);if(!isfinite(spectrum[i])||spectrum[i]<-0.001||spectrum[i]>1.001)valid=NO;}
  if(!valid)continue;
  uint64_t epoch=MSHFReadU64(p);if(epoch!=self.audioEpoch){self.audioEpoch=epoch;LSBeatInit(&_beatState);}
  LSBeatUpdate(&_beatState,spectrum,rms,now,(status&MSHFFeatureStatusSilence)!=0);self.audioSequence=h.sequence;self.audioLastFrame=now;self.helperSeen=YES;
  self.helperButton.hidden=YES;
  if(!self.running&&!self.userPaused&&self.meshConnected)[self begin];
 }
 if(!self.helperSeen){self.helperButton.hidden=NO;if(self.meshConnected)self.status.text=@"Lights connected · install the audio helper once";}
 else if(now-self.audioLastFrame>1.5){if(self.meshConnected)self.status.text=@"Waiting for playback audio";}
}
- (void)openAudioHelper {NSURL *url=[NSURL URLWithString:@"sileo://package/com.ryannair05.audiosnapshotserver2"];[UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];}
- (void)resumeLightShow {if(!self.userPaused){[self refreshMedia];if(self.helperSeen)[self begin];}}

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
  self.rate=[rate respondsToSelector:@selector(doubleValue)]?rate.doubleValue:1; if(!getPlaying)self.playing=self.rate>0;
  NSNumber *elapsed=[self mediaValue:d suffix:@"ElapsedTime"];self.elapsed=[elapsed respondsToSelector:@selector(doubleValue)]?elapsed.doubleValue:0;
  NSDate *stamp=[self mediaValue:d suffix:@"Timestamp"];
  if([stamp isKindOfClass:NSDate.class])self.elapsed+=MAX(0,-stamp.timeIntervalSinceNow)*MAX(0,self.rate);
  self.mediaReceived=NSProcessInfo.processInfo.systemUptime;
  // Metadata sometimes contains BPM; otherwise this is a labelled playback pulse, not measured beats.
  id bpm=[self mediaValue:d suffix:@"BeatsPerMinute"];double tempo=[bpm respondsToSelector:@selector(doubleValue)]?[bpm doubleValue]:0;self.tempo=tempo>=40&&tempo<=240?tempo:0;
  NSData *art=[self mediaValue:d suffix:@"ArtworkData"];
  NSData *old=[self mediaValue:self.mediaInfo suffix:@"ArtworkData"];
  if([art isKindOfClass:NSData.class] && ![art isEqual:old]){
   UIImage *image=[UIImage imageWithData:art];self.artwork.image=image;CGImageRef cg=image.CGImage;
   if(cg){unsigned char pixel[16*16*4]={0};CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();CGContextRef ctx=CGBitmapContextCreate(pixel,16,16,8,64,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    if(ctx){CGContextDrawImage(ctx,CGRectMake(0,0,16,16),cg);float best=0,hue=self.artworkHue;
     for(int i=0;i<256;i++){CGFloat r=pixel[i*4]/255.0,g=pixel[i*4+1]/255.0,b=pixel[i*4+2]/255.0,h,s,v,a;[[UIColor colorWithRed:r green:g blue:b alpha:1]getHue:&h saturation:&s brightness:&v alpha:&a];float score=s*v;if(score>best){best=score;hue=h;}}
     self.artworkHue=hue;CGContextRelease(ctx);}
    CGColorSpaceRelease(space);
   }
  }
  self.mediaInfo=d.copy;
  if(!self.running && !self.userPaused && self.helperSeen && NSProcessInfo.processInfo.systemUptime-self.audioLastFrame<1)[self begin];
  if(!self.running && !self.userPaused && !LSBootstrapError && self.meshConnected)self.status.text=[artist isKindOfClass:NSString.class]?artist:@"Ready · starts when music plays";
 });
}

- (void)buildSpatialZones:(NSDictionary *)mesh dataSource:(id)ds {
 NSMutableArray *living=[NSMutableArray new],*support=[NSMutableArray new];NSMutableSet *livingGroups=[NSMutableSet new],*livingNames=[NSMutableSet new];NSDictionary *tv=nil,*corner=nil;
 for(NSDictionary *g in mesh[@"groups"]){
  unsigned a=0;NSString *address=[g[@"address"] uppercaseString];[[NSScanner scannerWithString:address?:@""]scanHexInt:&a];if(a<0xc000||a>0xfeff)continue;
  NSString *name=[g[@"name"] description]?:@"Lights";
  if([name.lowercaseString containsString:@"living"]){[livingGroups addObject:address];continue;}
  [support addObject:@{@"address":@(a),@"name":name,@"role":@3,@"phase":@([name.lowercaseString containsString:@"kitchen"]||[name.lowercaseString isEqual:@"new"]?0.25:([name.lowercaseString containsString:@"hall"]?0.5:0.75))}];
 }
 for(NSDictionary *node in mesh[@"nodes"]){
  BOOL inLiving=NO;
  for(NSDictionary *e in node[@"elements"])for(NSDictionary *m in e[@"models"])for(id sub in m[@"subscribe"])if([sub isKindOfClass:NSString.class] && [livingGroups containsObject:[sub uppercaseString]])inLiving=YES;
  if(inLiving && [node[@"name"] isKindOfClass:NSString.class])[livingNames addObject:node[@"name"]];
 }
 for(id node in send0(ds,@"curNodes")){
  NSString *name=send0(node,@"name");NSArray *addresses=send0(node,@"HSLAddresses");if(!addresses.count||![name isKindOfClass:NSString.class]||![livingNames containsObject:name])continue;
  if([name.lowercaseString containsString:@"tv"] && !tv){tv=@{@"address":addresses.firstObject,@"name":name,@"role":@1,@"phase":@0};continue;}
  if([name.lowercaseString containsString:@"corner"] && !corner){corner=@{@"address":addresses.firstObject,@"name":name,@"role":@2,@"phase":@0.15};continue;}
  [living addObject:@{@"address":addresses.firstObject,@"name":name,@"role":@0,@"phase":@(living.count*0.15)}];
 }
 self.livingZones=living;self.supportZones=support;self.tvZone=tv;self.cornerZone=corner;
}
- (NSDictionary *)nextSpatialZone {
 NSUInteger slot=self.spatialTick++%8;
 if((slot==1||slot==5)&&self.cornerZone)return self.cornerZone;
 if(slot==4&&self.tvZone)return self.tvZone;
 if(slot==3&&self.supportZones.count)return self.supportZones[self.supportCursor++%self.supportZones.count];
 if(self.livingZones.count)return self.livingZones[self.livingCursor++%self.livingZones.count];
 if(self.supportZones.count)return self.supportZones[self.supportCursor++%self.supportZones.count];
 return @{@"address":@65535,@"role":@3,@"phase":@0};
}
- (BOOL)prepareSavedMesh {
 if(self.meshPrepared)return YES;
 if(LSBootstrapError){self.status.text=LSBootstrapError;return NO;}
 // Use the independent local controller state, persisting its own sequence counter.
 NSDictionary *prefs=LSReadPreferences();
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
  [self buildSpatialZones:mesh dataSource:ds];
  NSMutableArray *names=[NSMutableArray new];
  for(NSDictionary *group in mesh[@"groups"])if([group[@"name"] isKindOfClass:NSString.class])[names addObject:group[@"name"]];

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
 self.native=NO;self.targets=@[];
}
- (void)toggle {
 if(self.running){self.userPaused=YES;[self stop];return;}
 self.userPaused=NO;self.lastAudioSubscribe=0;[self begin];
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
  self.running=YES;self.sceneStarted=NSProcessInfo.processInfo.systemUptime;self.sceneHue=self.artworkHue;self.oldIdle=UIApplication.sharedApplication.idleTimerDisabled;UIApplication.sharedApplication.idleTimerDisabled=YES;
  [self.start setTitle:@"Pause light show" forState:UIControlStateNormal];
  self.timer=[NSTimer timerWithTimeInterval:0.05 target:self selector:@selector(tick) userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop] addTimer:self.timer forMode:NSRunLoopCommonModes];return;
 }
}
- (void)tick {
 if(!self.running)return;
 id bearer=send0(NSClassFromString(@"SigBearer"),@"share");
 if([bearer respondsToSelector:NSSelectorFromString(@"isOpen")] && !((BOOL(*)(id,SEL))objc_msgSend)(bearer,NSSelectorFromString(@"isOpen"))){self.meshConnected=NO;self.status.text=@"Reconnecting to your saved lights";[self connectSavedMesh];return;}

 double now=NSProcessInfo.processInfo.systemUptime;
 if(!self.helperSeen||now-self.audioLastFrame>1.5){self.status.text=self.helperSeen?@"Waiting for playback audio":@"Install the audio helper for beat detection";return;}
 float energy=_beatState.level,pulse=_beatState.beat,wave=energy;
 float hue=_beatState.hue;
 if(self.palette.selectedSegmentIndex==0)hue=fmodf(self.artworkHue+_beatState.beats*0.045f,1);
 if(self.palette.selectedSegmentIndex==1){float total=_beatState.bass+_beatState.mid+_beatState.high+0.001;hue=fmodf(0.52+0.34*(_beatState.mid+2*_beatState.high)/(2*total),1);}
 float strength=fminf(1.3,fmaxf(0.2,self.gain.value/2.5));
 uint16_t light=(uint16_t)(fminf(0.95,fmaxf(0.025,energy*strength))*65535),hu=(uint16_t)(hue*65535),sat=(uint16_t)((0.7+0.25*pulse)*65535);
 self.spectrum.bass=_beatState.bass;self.spectrum.mid=_beatState.mid;self.spectrum.high=_beatState.high;self.spectrum.phase=now*0.7;[self.spectrum setNeedsDisplay];
 @try {
 if(self.native){
  NSString *t=self.targets[self.cursor++%self.targets.count];
  SEL s=NSSelectorFromString(@"meshClientHslSet:lightness:hue:saturation:reliable:transitionTime:delay:");
  int result=((int(*)(id,SEL,id,int,int,int,unsigned char,int,short))objc_msgSend)(NSClassFromString(@"MeshNativeHelper"),s,t,light,hu,sat,0,0,0);
  self.status.text=[NSString stringWithFormat:@"%lu lights · live beat sync",(unsigned long)self.savedBulbCount]; (void)result;
 }else{
  id peripheral=send0(bearer,@"getCurrentPeripheral");
  if([peripheral respondsToSelector:NSSelectorFromString(@"canSendWriteWithoutResponse")] && !((BOOL(*)(id,SEL))objc_msgSend)(peripheral,NSSelectorFromString(@"canSendWriteWithoutResponse")))return;
  // Observe the SDK queue rather than building an unbounded backlog.
  id mesh=send0(NSClassFromString(@"SigMeshLib"),@"share");id commands=send0(mesh,@"commands");
  if([commands respondsToSelector:@selector(count)] && [commands count]){self.status.text=@"Music reactive · waiting for Bluetooth";return;}
  uint16_t destination=0xffff;
  Class sdk=NSClassFromString(@"SDKLibCommand");
  if(self.hasRGB){SEL s=NSSelectorFromString(@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:retryCount:responseMaxCount:ack:successCallback:resultCallback:");
   ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,s,destination,light,hu,sat,0,0,NO,nil,nil);
  }else{SEL s=NSSelectorFromString(@"lightLightnessSetWithDestination:lightness:retryCount:responseMaxCount:ack:successCallback:resultCallback:");
   if([sdk respondsToSelector:s])((id(*)(id,SEL,uint16_t,uint16_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,s,destination,light,0,0,NO,nil,nil);
  }
  self.status.text=[NSString stringWithFormat:@"%lu lights · live beat sync",(unsigned long)self.savedBulbCount];
 }
 }@catch(NSException *e){[self stop];self.status.text=[@"Light command failed: " stringByAppendingString:e.reason?:@"unknown error"];}
 self.window.rootViewController.view.layer.borderColor=[UIColor colorWithHue:hue saturation:0.7 brightness:0.5+0.3*wave alpha:1].CGColor;self.window.rootViewController.view.layer.borderWidth=2;
}
- (void)stop {
 if(self.audioSocket>=0&&self.audioServer&&self.userPaused){uint8_t packet[32];MSHFEncodeHeader(packet,MSHFMessageTypeUnsubscribe,0,self.audioSession,self.audioServer,++_audioRequest);struct sockaddr_in server={0};server.sin_len=sizeof(server);server.sin_family=AF_INET;server.sin_port=htons(MSHF_PROTOCOL_PORT);server.sin_addr.s_addr=htonl(INADDR_LOOPBACK);sendto(self.audioSocket,packet,sizeof(packet),0,(struct sockaddr *)&server,sizeof(server));}
 BOOL was=self.running;self.running=NO;[self.timer invalidate];self.timer=nil;
 
 
 if(was)UIApplication.sharedApplication.idleTimerDisabled=self.oldIdle;
 
 [self.start setTitle:@"Start visualizer" forState:UIControlStateNormal];
 if(was)self.status.text=@"Stopped. Lights keep their last colour.";
}
@end
static UIWindow *LSHostWindow;
static BOOL LSLaunch(id delegate,SEL selector,UIApplication *application,NSDictionary *options){
 LSHostWindow=[[UIWindow alloc]initWithFrame:UIScreen.mainScreen.bounds];UIViewController *vc=[UIViewController new];vc.view.backgroundColor=[UIColor colorWithRed:0.035 green:0.043 blue:0.065 alpha:1];LSHostWindow.rootViewController=vc;
 if([delegate respondsToSelector:@selector(setWindow:)])((void(*)(id,SEL,id))objc_msgSend)(delegate,@selector(setWindow:),LSHostWindow);
 [LSHostWindow makeKeyAndVisible];
 controller=[LSPanel new];[controller install];return YES;
}
static void LSLifecycle(id d,SEL s,id application){}
__attribute__((constructor)) static void loadLightShow(void){
 @autoreleasepool{
 LSBootstrapError=LSBootstrap();nativeTargets=[NSMutableOrderedSet new];
 Class delegate=NSClassFromString(@"_TtC13SmartHome_Prd11AppDelegate");
 SEL launch=@selector(application:didFinishLaunchingWithOptions:);Method m=class_getInstanceMethod(delegate,launch);
 if(m)method_setImplementation(m,(IMP)LSLaunch);
 for(NSString *name in @[@"applicationDidBecomeActive:",@"applicationWillResignActive:",@"applicationDidEnterBackground:",@"applicationWillEnterForeground:",@"applicationWillTerminate:"]){Method lifecycle=class_getInstanceMethod(delegate,NSSelectorFromString(name));if(lifecycle)method_setImplementation(lifecycle,(IMP)LSLifecycle);}
 }
}
