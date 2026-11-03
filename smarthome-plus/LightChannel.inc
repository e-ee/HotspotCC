// The native bulbs retain independent HSL and CTL registers. Reading HSL after
// a CTL command can legitimately return the last coloured mood, not visible white.
static NSString *HPChannelKey(void){return [@"HP.LightChannels."stringByAppendingString:HPMeshUUID()?:@"current"];}
static BOOL HPUsesWhiteChannel(NSDictionary *light){return [[NSUserDefaults.standardUserDefaults dictionaryForKey:HPChannelKey()][light[@"id"]]isEqual:@"ctl"];}
static void HPRememberChannel(uint16_t destination,NSString *channel){
 NSMutableArray *ids=[NSMutableArray new];NSDictionary *mesh=nil;
 if(destination>=0x8000){NSData *data=[NSUserDefaults.standardUserDefaults dataForKey:@"mesh.json"];if(data)mesh=[NSJSONSerialization JSONObjectWithData:data options:0 error:nil];}
 for(id node in HPGet(HPGet(NSClassFromString(@"SigDataSource"),@"share"),@"curNodes")){
  NSString *uid=HPGet(node,@"UUID");if(!uid)continue;uint16_t primary=((uint16_t(*)(id,SEL))objc_msgSend)(node,NSSelectorFromString(@"address"));
  BOOL matches=destination==primary||destination==0xffff;
  for(id address in HPGet(node,@"HSLAddresses"))if([address unsignedShortValue]==destination)matches=YES;
  if(!matches&&destination>=0x8000)for(NSDictionary *record in mesh[@"nodes"])if([record[@"UUID"]isEqual:uid])for(NSDictionary *element in record[@"elements"])for(NSDictionary *model in element[@"models"])for(id a in model[@"subscribe"]){unsigned value=0;if([a isKindOfClass:NSString.class])[[NSScanner scannerWithString:a]scanHexInt:&value];else if([a isKindOfClass:NSNumber.class])value=[a unsignedIntValue];if(value==destination)matches=YES;}
  if(matches)[ids addObject:uid];
 }
 if(!ids.count)return;
 NSUserDefaults *prefs=NSUserDefaults.standardUserDefaults;@synchronized(prefs){NSString *key=HPChannelKey();NSDictionary *old=[prefs dictionaryForKey:key]?:@{};NSMutableDictionary *modes=nil;
 for(NSString *uid in ids)if(![old[uid]isEqual:channel]){if(!modes)modes=[old mutableCopy];modes[uid]=channel;}if(modes)[prefs setObject:modes forKey:key];}
}
static IMP HPTrackedHSLFullOriginal,HPTrackedHSLShortOriginal,HPTrackedCTLFullOriginal,HPTrackedCTLShortOriginal,HPTrackedTempFullOriginal,HPTrackedTempShortOriginal;
#define HPFullParameters id self,SEL sel,uint16_t dst,uint16_t level,uint16_t a,uint16_t b,id transition,uint8_t delay,NSInteger retry,NSInteger count,BOOL ack,id success,id result
#define HPFullCall(original) ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))original)(self,sel,dst,level,a,b,transition,delay,retry,count,ack,success,result)
static id HPTrackedHSLFull(HPFullParameters){id value=HPFullCall(HPTrackedHSLFullOriginal);HPRememberChannel(dst,@"hsl");return value;}
static id HPTrackedCTLFull(HPFullParameters){id value=HPFullCall(HPTrackedCTLFullOriginal);HPRememberChannel(dst,@"ctl");return value;}
#define HPShortParameters id self,SEL sel,uint16_t dst,uint16_t level,uint16_t a,uint16_t b,NSInteger retry,NSInteger count,BOOL ack,id success,id result
#define HPShortCall(original) ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,NSInteger,NSInteger,BOOL,id,id))original)(self,sel,dst,level,a,b,retry,count,ack,success,result)
static id HPTrackedHSLShort(HPShortParameters){id value=HPShortCall(HPTrackedHSLShortOriginal);HPRememberChannel(dst,@"hsl");return value;}
static id HPTrackedCTLShort(HPShortParameters){id value=HPShortCall(HPTrackedCTLShortOriginal);HPRememberChannel(dst,@"ctl");return value;}
static id HPTrackedTempFull(id self,SEL sel,uint16_t dst,uint16_t temp,uint16_t delta,id transition,uint8_t delay,NSInteger retry,NSInteger count,BOOL ack,id success,id result){id value=((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))HPTrackedTempFullOriginal)(self,sel,dst,temp,delta,transition,delay,retry,count,ack,success,result);HPRememberChannel(dst,@"ctl");return value;}
static id HPTrackedTempShort(id self,SEL sel,uint16_t dst,uint16_t temp,uint16_t delta,NSInteger retry,NSInteger count,BOOL ack,id success,id result){id value=((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,NSInteger,NSInteger,BOOL,id,id))HPTrackedTempShortOriginal)(self,sel,dst,temp,delta,retry,count,ack,success,result);HPRememberChannel(dst,@"ctl");return value;}
#undef HPFullParameters
#undef HPFullCall
#undef HPShortParameters
#undef HPShortCall
static void HPInstallChannelTracking(void){
 Class cls=object_getClass(NSClassFromString(@"SDKLibCommand"));if(!cls)return;
 NSArray *names=@[@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:",@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:retryCount:responseMaxCount:ack:successCallback:resultCallback:",@"lightCTLSetWithDestination:lightness:temperature:deltaUV:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:",@"lightCTLSetWithDestination:lightness:temperature:deltaUV:retryCount:responseMaxCount:ack:successCallback:resultCallback:",@"lightCTLTemperatureSetWithDestination:temperature:deltaUV:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:",@"lightCTLTemperatureSetWithDestination:temperature:deltaUV:retryCount:responseMaxCount:ack:successCallback:resultCallback:"];
 IMP replacements[]={(IMP)HPTrackedHSLFull,(IMP)HPTrackedHSLShort,(IMP)HPTrackedCTLFull,(IMP)HPTrackedCTLShort,(IMP)HPTrackedTempFull,(IMP)HPTrackedTempShort};IMP *originals[]={&HPTrackedHSLFullOriginal,&HPTrackedHSLShortOriginal,&HPTrackedCTLFullOriginal,&HPTrackedCTLShortOriginal,&HPTrackedTempFullOriginal,&HPTrackedTempShortOriginal};
 for(NSUInteger n=0;n<names.count;n++){SEL sel=NSSelectorFromString(names[n]);Method method=class_getInstanceMethod(cls,sel);if(method)*originals[n]=method_setImplementation(method,replacements[n]);}
}
