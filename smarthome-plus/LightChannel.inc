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
// Observe the existing native UI/API controls, rather than replacing SDK setters.
// Studio sends HSL directly and records its channel explicitly after each send.
static void HPRememberNamedChannel(id target,NSString *channel){
 if(![target isKindOfClass:NSString.class])return;
 for(id node in HPGet(HPGet(NSClassFromString(@"SigDataSource"),@"share"),@"curNodes"))if([HPGet(node,@"name")isEqual:target]||[HPGet(node,@"UUID")isEqual:target]){uint16_t address=((uint16_t(*)(id,SEL))objc_msgSend)(node,NSSelectorFromString(@"address"));HPRememberChannel(address,channel);return;}
 NSData *data=[NSUserDefaults.standardUserDefaults dataForKey:@"mesh.json"];NSDictionary *mesh=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;
 for(NSDictionary *group in mesh[@"groups"])if([group[@"name"]isEqual:target]){unsigned address=0;id value=group[@"address"];if([value isKindOfClass:NSString.class])[[NSScanner scannerWithString:value]scanHexInt:&address];else if([value isKindOfClass:NSNumber.class])address=[value unsignedIntValue];if(address>0&&address<=65535)HPRememberChannel((uint16_t)address,channel);return;}
}
