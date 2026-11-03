// All creative features share the existing paced HSL sender and native fades.
static NSArray *HPColourArray(HPColor c){return @[@(c.hue),@(c.saturation),@(c.lightness)];}
static HPColor HPArrayColour(NSArray *a){return (HPColor){[a[0]floatValue],[a[1]floatValue],[a[2]floatValue]};}
static BOOL HPValidLook(id look){
 if(![look isKindOfClass:NSDictionary.class]||[look count]>4096)return NO;
 for(id key in look){id a=look[key];if(![key isKindOfClass:NSString.class]||![a isKindOfClass:NSArray.class]||[a count]!=3)return NO;for(id n in a)if(![n isKindOfClass:NSNumber.class]||!isfinite([n doubleValue])||[n doubleValue]<0||[n doubleValue]>1)return NO;}return YES;
}
static BOOL HPNodeInGroup(NSString *uuid,NSString *name){
 NSDictionary *mesh=HPMesh();NSString *address=nil;
 for(NSDictionary *g in mesh[@"groups"])if([g[@"name"]isEqual:name])address=[g[@"address"]uppercaseString];
 if(!address)return NO;
 for(NSDictionary *node in mesh[@"nodes"])if([node[@"UUID"]isEqual:uuid])for(NSDictionary *element in node[@"elements"])for(NSDictionary *model in element[@"models"])for(id a in model[@"subscribe"])if([a isKindOfClass:NSString.class]&&[[a uppercaseString]isEqual:address])return YES;
 return NO;
}
static NSDictionary *HPSceneLook(NSDictionary *scene,NSArray *lights){
 NSMutableDictionary *look=[NSMutableDictionary new];
 // Expand group entries first; explicit bulb settings take precedence.
 for(int pass=0;pass<2;pass++)for(NSDictionary *entry in scene[@"routineList"]){
 BOOL group=[entry[@"isGroup"]boolValue];if(group!=(pass==0))continue;
 BOOL off=![entry[@"onOff"]boolValue];double hue=[entry[@"color"]doubleValue],sat=[entry[@"saturation"]doubleValue],level=[entry[@"lightness"]doubleValue];
 NSInteger type=[entry[@"deviceType"]integerValue];
 if(!off&&type!=8&&type!=4)continue; // CT-only entries cannot supply an HSL colour.
 if(!off&&(!isfinite(hue)||!isfinite(sat)||!isfinite(level)||hue<0||hue>360||sat<0||sat>100||level<0||level>100))continue;
 for(NSDictionary *light in lights){BOOL match=group?HPNodeInGroup(light[@"id"],entry[@"deviceName"]):[light[@"name"]isEqual:entry[@"deviceName"]];if(match)look[light[@"id"]]=HPColourArray((HPColor){off?0:hue/360.,off?0:sat/100.,off?0:level/100.});}
 }return look;
}
static NSString *HPCreativeHomeKey(NSString *key){return [NSString stringWithFormat:@"HP.%@.%@",key,HPMoodStorageKey()];}
static NSArray *HPJourneysSaved(void){id a=[NSUserDefaults.standardUserDefaults arrayForKey:HPCreativeHomeKey(@"ComposedJourneys")];return a?:@[];}
static void HPStoreJourneys(NSArray *a){[NSUserDefaults.standardUserDefaults setObject:a forKey:HPCreativeHomeKey(@"ComposedJourneys")];}
@implementation HPLightEngine (Choreography)
- (void)clearCreative{
 self.creativeBrightness=self.brightness;self.creativeDepth=self.depth;self.creativeKind=0;self.creativeName=nil;self.anchorLook=nil;self.painted=nil;self.morphFrom=nil;self.morphTo=nil;self.customSteps=nil;self.sequenceFallback=nil;self.creativeElapsed=0;self.rippleAge=100;
}
- (NSDictionary *)snapshotLook{
 [self reloadLights];NSMutableDictionary *look=[NSMutableDictionary new];
 for(NSDictionary *light in self.lights){NSArray *sent=self.sent[light[@"id"]];NSDictionary *cached=HPCachedColour(light);HPColor c;
 if(sent)c=HPStatusColor([sent[1]unsignedShortValue],[sent[2]unsignedShortValue],[sent[0]unsignedShortValue]);
 else if(cached)c=HPStatusColor([cached[@"hue"]unsignedShortValue],[cached[@"sat"]unsignedShortValue],[cached[@"light"]unsignedShortValue]);else c=[self targetForLight:light];
 look[light[@"id"]]=HPColourArray(c);
 }return look;
}
- (void)beginCanvas{
 NSDictionary *look=[self snapshotLook];[self clearCreative];[self activate:self.lastMode journey:-1 reset:NO retarget:NO];self.creativeKind=1;self.creativeName=@"Painted room";self.painted=[look mutableCopy];self.retargets=[NSMutableSet new];
}
- (void)paintAt:(CGPoint)point colour:(HPColor)colour radius:(double)radius{
 if(self.creativeKind!=1)[self beginCanvas];if(self.mode<0)[self resume];
 for(NSDictionary *light in self.lights){double d=hypot([light[@"x"]doubleValue]-point.x,[light[@"y"]doubleValue]-point.y);double weight=.40*exp(-d*d/(MAX(.02,radius*radius)*.45));if(weight<.005)continue;
 HPColor old=HPArrayColour(self.painted[light[@"id"]]);self.painted[light[@"id"]]=HPColourArray(HPBlendColour(old,colour,weight));[self.retargets addObject:light[@"id"]];}
 self.status=@"Painting · pause to save the finished look";
}
- (void)rippleAt:(CGPoint)point{
 if(self.mode<0)[self resume];self.rippleOrigin=CGPointMake(HPClamp(point.x,0,1),HPClamp(point.y,0,1));self.rippleAge=0;self.status=@"A soft ripple is travelling through the room";
}
- (void)beginExplore:(NSInteger)variant{
 if(self.creativeKind!=4){NSDictionary *look=[self snapshotLook];[self clearCreative];self.anchorLook=look;self.creativeKind=4;self.creativeName=@"Exploring this look";}
 self.exploreVariant=variant;[self activate:self.lastMode journey:-1 reset:NO retarget:NO];self.status=@"Exploring gently around your original look";
}
- (BOOL)beginMorph:(NSDictionary *)from to:(NSDictionary *)to duration:(double)duration stagger:(double)stagger{
 if(!HPValidLook(from)||!HPValidLook(to)||!from.count||!to.count)return NO;
 NSDictionary *fallback=[self snapshotLook];NSMutableDictionary *a=[fallback mutableCopy],*b=[fallback mutableCopy];[a addEntriesFromDictionary:from];[b addEntriesFromDictionary:to];
 [self clearCreative];self.morphFrom=a;self.morphTo=b;self.creativeDuration=HPClamp(duration,5,7200);self.morphStagger=HPClamp(stagger,0,.65);self.creativeKind=2;self.creativeName=@"Scene dissolve";[self activate:self.lastMode journey:-1 reset:NO retarget:NO];return YES;
}
- (BOOL)beginSequence:(NSArray *)steps name:(NSString *)name loop:(BOOL)loop{
 if(![steps isKindOfClass:NSArray.class]||!steps.count||steps.count>100)return NO;
 for(NSDictionary *step in steps){if(![step isKindOfClass:NSDictionary.class])return NO;
 if(![step[@"hold"]isKindOfClass:NSNumber.class]||!isfinite([step[@"hold"]doubleValue])||[step[@"hold"]doubleValue]<5||[step[@"hold"]doubleValue]>7200)return NO;
 if(![step[@"fade"]isKindOfClass:NSNumber.class]||!isfinite([step[@"fade"]doubleValue])||[step[@"fade"]doubleValue]<5||[step[@"fade"]doubleValue]>3600)return NO;
 if(step[@"look"]){if(!HPValidLook(step[@"look"])||![step[@"look"]count])return NO;}
 else if(![HPStudioPresets validatedRecipe:step[@"recipe"] error:nil])return NO;
 }
 NSDictionary *fallback=[self snapshotLook];[self clearCreative];self.sequenceFallback=fallback;self.customSteps=[steps copy];self.sequenceLoop=loop;self.creativeKind=3;self.creativeName=name?:@"My journey";[self activate:self.lastMode journey:-1 reset:NO retarget:NO];return YES;
}
- (HPColor)colourForStep:(NSDictionary *)step light:(NSDictionary *)light fallback:(HPColor)fallback{
 if(step[@"look"]){NSArray *a=step[@"look"][light[@"id"]];return a?HPArrayColour(a):fallback;}
 NSDictionary *r=step[@"recipe"];double x=[light[@"x"]doubleValue],y=[light[@"y"]doubleValue],u,v;HPDirectedPoint(x,y,[r[@"angle"]doubleValue],&u,&v);double phase=[r[@"phase"]doubleValue]+self.creativeElapsed*HPMotionRate([r[@"motion"]doubleValue]);
 if([r[@"reverse"]boolValue])phase=-phase;
 HPColor c=[r[@"mode"]integerValue]==32?HPLivingAurora(phase,u,v,[r[@"brightness"]doubleValue],[r[@"depth"]doubleValue]):HPStudioTarget([r[@"mode"]intValue],[r[@"journey"]intValue],[r[@"journeyElapsed"]doubleValue]+self.creativeElapsed,phase,u,v,[r[@"brightness"]doubleValue],[r[@"depth"]doubleValue],[r[@"variation"]unsignedIntValue]);
 if([r[@"creativeKind"]intValue]==1&&r[@"painted"][light[@"id"]])c=HPArrayColour(r[@"painted"][light[@"id"]]);
 if([r[@"creativeKind"]intValue]==4&&r[@"anchorLook"][light[@"id"]])c=HPExploreColour(HPArrayColour(r[@"anchorLook"][light[@"id"]]),[r[@"exploreVariant"]intValue],.8);
 return c;
}
- (HPColor)creativeTarget:(NSDictionary *)light{
 double x=[light[@"x"]doubleValue],y=[light[@"y"]doubleValue],u,v;HPDirectedPoint(x,y,self.waveAngle,&u,&v);double phase=self.waveReverse?-self.phase:self.phase;if(self.lastMode==32&&self.variation){phase+=(self.variation%997)*.73;if(self.variation&1)u=1-u;if(self.variation&2)v=1-v;}
 HPColor c=self.lastMode==32?HPLivingAurora(phase,u,v,self.brightness,self.depth):self.lastMode==33?HPStudioTarget(30,-1,0,phase,u,v,self.brightness,self.depth,self.variation):HPStudioTarget((int)self.lastMode,(int)self.journey,self.journeyElapsed,phase,u,v,self.brightness,self.depth,self.variation);
 // Preserve aurora greens; other palettes favour blues, warm tones and violet.
 if(self.lastMode!=0&&self.lastMode!=24&&self.lastMode!=32&&c.hue>.23&&c.hue<.45)c.hue=.48+(c.hue-.23)*.25;
 NSString *key=light[@"id"];
 if(self.creativeKind==1&&self.painted[key])c=HPArrayColour(self.painted[key]);
 if(self.creativeKind==2){double position=u;if(self.waveReverse)position=1-position;double t=(self.creativeElapsed/self.creativeDuration-position*self.morphStagger)/(1-self.morphStagger);c=HPBlendColour(HPArrayColour(self.morphFrom[key]),HPArrayColour(self.morphTo[key]),HPEase(t));}
 if(self.creativeKind==3&&self.customSteps.count){
 double total=0;for(NSDictionary *step in self.customSteps)total+=[step[@"hold"]doubleValue]+[step[@"fade"]doubleValue];double t=self.sequenceLoop?fmod(self.creativeElapsed,total):MIN(self.creativeElapsed,total);NSUInteger index=0;
 for(;index+1<self.customSteps.count;index++){NSDictionary *step=self.customSteps[index];double d=[step[@"hold"]doubleValue]+[step[@"fade"]doubleValue];if(t<d)break;t-=d;}
 NSDictionary *step=self.customSteps[index];HPColor fallback=self.sequenceFallback[key]?HPArrayColour(self.sequenceFallback[key]):c;HPColor a=[self colourForStep:step light:light fallback:fallback];
 NSUInteger next=index+1<self.customSteps.count?index+1:self.sequenceLoop?0:index;HPColor b=[self colourForStep:self.customSteps[next]light:light fallback:a];double blend=HPEase((t-[step[@"hold"]doubleValue])/[step[@"fade"]doubleValue]);c=HPBlendColour(a,b,blend);
 }
 if(self.creativeKind==4&&self.anchorLook[key]){HPColor a=HPArrayColour(self.anchorLook[key]);c=HPExploreColour(a,(int)self.exploreVariant,.7+.3*HPWave(u*.4+v*.3+phase/500.));}
 if(self.creativeKind){c.lightness=HPClamp(c.lightness*self.brightness/MAX(.08,self.creativeBrightness),0,1);c.saturation=HPHeldDepth(c.saturation,self.creativeDepth?:self.depth,self.depth);}
 if(self.rippleAge<19){double d=hypot(x-self.rippleOrigin.x,y-self.rippleOrigin.y);HPColor glow=c;glow.hue=HPWrap(c.hue+.055);glow.lightness=HPClamp(c.lightness+.08,0,1);c=HPBlendColour(c,glow,.85*HPRippleWeight(d,self.rippleAge,self.rippleInward));}
 if(self.sleepActive){double t=HPEase(self.sleepElapsed/MAX(1,self.sleepDuration));HPColor warm={.075,.50,c.lightness};c=HPBlendColour(c,warm,t);c.lightness*=pow(1-t,1.4);}
 return c;
}
- (void)advanceCreative:(double)elapsed{
 self.creativeElapsed+=elapsed*self.playbackRate;self.rippleAge+=elapsed*self.playbackRate;
 if(self.sleepActive){double old=self.sleepElapsed;self.sleepElapsed=MIN(self.sleepDuration,self.sleepElapsed+elapsed);
 if(old<self.sleepDuration&&self.sleepElapsed>=self.sleepDuration)[self requestRetarget];
 if(self.sleepElapsed>=self.sleepDuration){BOOL off=self.sent.count>=self.lights.count;for(NSDictionary *light in self.lights)if(!self.sent[light[@"id"]]||[self.sent[light[@"id"]][0]intValue]!=0)off=NO;if(off){[self stop];self.sleepActive=NO;self.status=@"Sleep descent complete · lights off";}}
 }
}
- (void)beginSleep:(double)seconds{
 if(self.mode<0){[self beginCanvas];}self.sleepDuration=HPClamp(seconds,60,14400);self.sleepElapsed=0;self.sleepActive=YES;self.status=@"Sleep descent · slowly warming and dimming to off";
}
- (void)cancelSleep{self.sleepActive=NO;self.sleepElapsed=0;self.status=@"Sleep descent cancelled";}
@end
