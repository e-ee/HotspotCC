// Captured Studio scenes contain sender units. Replay those units through the
// same paced SDK endpoint as Studio, without the native routine conversion.
@interface HPDirectScene : NSObject
@property(nonatomic,strong) NSArray *entries,*lights;
@property(nonatomic) NSUInteger index,generation;
@property(nonatomic) double deadline;
- (void)next;
@end
@implementation HPDirectScene
- (void)next{
 HPLightEngine *engine=HPLightEngine.shared;if(engine.generation!=self.generation)return;
 if(self.index>=self.entries.count){engine.status=@"Scene applied";return;}
 if(NSProcessInfo.processInfo.systemUptime>self.deadline){engine.status=@"Scene interrupted: connection unavailable";return;}
 if(!HPTransportOpen()||HPTransportBusy()){dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self next];});return;}
 NSDictionary *entry=self.entries[self.index],*light=nil;
 for(NSDictionary *candidate in self.lights)if([candidate[@"name"]isEqual:entry[@"deviceName"]]){light=candidate;break;}
 if(!light){self.index++;[self next];return;}
 int level=[entry[@"onOff"]boolValue]?[entry[@"lightness"]intValue]:0;
 BOOL sent=NO;
 if([entry[@"deviceType"]intValue]==8){
  NSDictionary *value=@{@"light":@(level),@"hue":entry[@"color"],@"sat":entry[@"saturation"]};
  sent=HPIdentifyColour(light,value);
  if(sent)HPRememberColour(light,(HPWireColor){[entry[@"color"]unsignedShortValue],[entry[@"saturation"]unsignedShortValue],level});
 }else if([entry[@"deviceType"]intValue]==7){
  SEL set=NSSelectorFromString(@"lightCTLSetWithDestination:lightness:temperature:deltaUV:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:");Class sdk=NSClassFromString(@"SDKLibCommand");
  if([sdk respondsToSelector:set]){
   id transition=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"SigTransitionTime"),@selector(alloc));transition=((id(*)(id,SEL,uint8_t))objc_msgSend)(transition,NSSelectorFromString(@"initWithRawValue:"),40);
   ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(sdk,set,[light[@"address"]unsignedShortValue],level,[entry[@"cct"]unsignedShortValue],0,transition,0,0,0,NO,nil,nil);HPRememberChannel([light[@"address"]unsignedShortValue],@"ctl");sent=YES;
  }
 }
 if(!sent){engine.status=@"Could not send this scene";return;}
 self.index++;dispatch_after(dispatch_time(DISPATCH_TIME_NOW,200*NSEC_PER_MSEC),dispatch_get_main_queue(),^{[self next];});
}
@end
static BOOL HPPlayDirectScene(NSInteger index){
 NSArray *scenes=HPExportRoutines();if(index<0||(NSUInteger)index>=scenes.count)return NO;
 NSDictionary *scene=scenes[index];if(![[NSUserDefaults.standardUserDefaults arrayForKey:@"HP.DirectSceneIDs"]containsObject:scene[@"uuid"]])return NO;
 HPDirectScene *player=[HPDirectScene new];player.entries=scene[@"routineList"];player.lights=HPLightRecords();
 [HPLightEngine.shared stop];player.generation=HPLightEngine.shared.generation;player.deadline=NSProcessInfo.processInfo.systemUptime+15;[player next];return YES;
}
static IMP HPSceneListTapOriginal;
static void HPSceneListTap(id self,SEL selector,UITableView *table,NSIndexPath *path){
 if(HPPlayDirectScene(path.row)){[table deselectRowAtIndexPath:path animated:YES];return;}
 ((void(*)(id,SEL,id,id))HPSceneListTapOriginal)(self,selector,table,path);
}
