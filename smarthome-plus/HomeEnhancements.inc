static __weak UIViewController *HPHomeController;
static char HPShortcutMapKey,HPShortcutDisableKey,HPLongPressKey;
@interface HPShortcutMap : NSObject
@property(nonatomic,strong) NSArray *natural,*indexes;
@end
@implementation HPShortcutMap @end

static NSArray *HPSceneSummaries(void){
    NSMutableArray *records=[NSMutableArray new];for(id scene in HPCall(HPRealm(),@"allObjects:",@"RoutinesInfo"))
        [records addObject:@{@"uuid":HPField(scene,@"uuid"),@"routineName":HPField(scene,@"routineName"),@"color":HPField(scene,@"color")}];
    return records;
}
static IMP HPCountOriginal,HPCellOriginal,HPTapOriginal;
static NSInteger HPHomeCount(id self,SEL selector,id collection,NSInteger section){
    NSInteger count=((NSInteger(*)(id,SEL,id,NSInteger))HPCountOriginal)(self,selector,collection,section);
    objc_setAssociatedObject(collection,&HPShortcutMapKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(objc_getAssociatedObject(collection,&HPShortcutDisableKey))return count;
    @try{
        NSArray *natural=HPSceneSummaries();if(natural.count!=(NSUInteger)count)return count;
        NSArray *ordered=HPOrdered(natural,@"HP.SceneOrder",@"uuid");NSMutableArray *indexes=[NSMutableArray new];
        for(NSDictionary *item in ordered){NSUInteger index=[natural indexOfObjectPassingTest:^BOOL(NSDictionary *candidate,NSUInteger n,BOOL *stop){return [candidate[@"uuid"]isEqual:item[@"uuid"]];}];if(index==NSNotFound)return count;[indexes addObject:@(index)];}
        HPShortcutMap *map=[HPShortcutMap new];map.natural=natural;map.indexes=indexes;
        objc_setAssociatedObject(collection,&HPShortcutMapKey,map,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }@catch(NSException *exception){}return count;
}
static id HPHomeCell(id self,SEL selector,UICollectionView *collection,NSIndexPath *path){
    id cell=((id(*)(id,SEL,id,id))HPCellOriginal)(self,selector,collection,path);
    HPShortcutMap *map=objc_getAssociatedObject(collection,&HPShortcutMapKey);
    if(!map||path.item<0||(NSUInteger)path.item>=map.indexes.count)return cell;
    UILabel *label=HPGet(cell,@"nameLabel");NSDictionary *natural=map.natural[path.item];
    // Fail back to the native order if the app's live cache has not caught up.
    if(![label.text isEqual:natural[@"routineName"]]){
        objc_setAssociatedObject(collection,&HPShortcutDisableKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(collection,&HPShortcutMapKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        collection.userInteractionEnabled=NO;
        dispatch_async(dispatch_get_main_queue(),^{[collection reloadData];collection.userInteractionEnabled=YES;});return cell;
    }
    NSDictionary *ordered=map.natural[[map.indexes[path.item]unsignedIntegerValue]];
    label.text=ordered[@"routineName"];uint32_t argb=[ordered[@"color"]unsignedIntValue];
    UIColor *color=[UIColor colorWithRed:((argb>>16)&255)/255. green:((argb>>8)&255)/255. blue:(argb&255)/255. alpha:((argb>>24)&255)/255.];
    [cell contentView].backgroundColor=color;return cell;
}
static void HPHomeTap(id self,SEL selector,UICollectionView *collection,NSIndexPath *path){
    [HPLightEngine.shared stop];HPShortcutMap *map=objc_getAssociatedObject(collection,&HPShortcutMapKey);
    if(map&&path.item>=0&&(NSUInteger)path.item<map.indexes.count)path=[NSIndexPath indexPathForItem:[map.indexes[path.item]integerValue] inSection:path.section];
    ((void(*)(id,SEL,id,id))HPTapOriginal)(self,selector,collection,path);
}
@interface HPManager (Ordering)
- (void)arrangeShortcuts:(UILongPressGestureRecognizer *)gesture;
@end
@implementation HPManager (Ordering)
- (void)arrangeShortcuts:(UILongPressGestureRecognizer *)gesture {
    if(gesture.state!=UIGestureRecognizerStateBegan)return;
    HPPathEditor *editor=[[HPPathEditor alloc]initWithStyle:UITableViewStyleInsetGrouped];editor.scenes=YES;
    // Present through a navigation controller because Home is not always in one.
    UINavigationController *navigation=[[UINavigationController alloc]initWithRootViewController:editor];
    editor.navigationItem.leftBarButtonItem=[[UIBarButtonItem alloc]initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(dismissOrder)];
    [HPTop() presentViewController:navigation animated:YES completion:nil];
}
- (void)dismissOrder {[HPTop() dismissViewControllerAnimated:YES completion:nil];}
@end
static void HPEnhanceHome(id controller){
    Class home=NSClassFromString(@"_TtC13SmartHome_Prd20ReHomeViewController");if(![controller isKindOfClass:home])return;
    HPHomeController=controller;UICollectionView *collection=HPGet(controller,@"collectionView");
    if(!collection||objc_getAssociatedObject(collection,&HPLongPressKey))return;
    UILongPressGestureRecognizer *press=[[UILongPressGestureRecognizer alloc]initWithTarget:HPManager.shared action:@selector(arrangeShortcuts:)];
    press.minimumPressDuration=.65;[collection addGestureRecognizer:press];objc_setAssociatedObject(collection,&HPLongPressKey,press,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static IMP HPProfileAwakeOriginal,HPMenuHeightOriginal;
static void HPProfileAwake(id self,SEL selector){
    ((void(*)(id,SEL))HPProfileAwakeOriginal)(self,selector);
    UIView *button=HPGet(self,@"backButton"),*label=HPGet(self,@"backButtonLabel");button.hidden=YES;label.hidden=YES;button.accessibilityElementsHidden=YES;label.accessibilityElementsHidden=YES;
}
static NSInteger HPStoredInteger(id object,const char *name,NSInteger fallback){
    Ivar ivar=class_getInstanceVariable([object class],name);if(!ivar)return fallback;
    NSInteger value;memcpy(&value,((const char *)(__bridge const void *)object)+ivar_getOffset(ivar),sizeof(value));return value;
}
static CGFloat HPMenuHeight(id self,SEL selector,id table,NSIndexPath *path){
    NSInteger section=HPStoredInteger(self,"PROFILE_SECTION",NSIntegerMin),row=HPStoredInteger(self,"PROFILE",NSIntegerMin);
    if(path.section==section&&path.row==row)return 0;
    return ((CGFloat(*)(id,SEL,id,id))HPMenuHeightOriginal)(self,selector,table,path);
}
static void HPNoExperience(id self,SEL selector,id sender){(void)self;(void)selector;(void)sender;}
static IMP HPManualHSLOriginal,HPManualCTLOriginal,HPManualOnOffOriginal,HPManualBrightnessOriginal;
static void HPManualStop(void){if(NSThread.isMainThread)[HPLightEngine.shared stop];else dispatch_async(dispatch_get_main_queue(),^{[HPLightEngine.shared stop];});}
static id HPManualHSL(id self,SEL selector,id target,int h,int s,int l,id callback){HPManualStop();return ((id(*)(id,SEL,id,int,int,int,id))HPManualHSLOriginal)(self,selector,target,h,s,l,callback);}
static id HPManualCTL(id self,SEL selector,id target,int l,int t,id callback){HPManualStop();return ((id(*)(id,SEL,id,int,int,id))HPManualCTLOriginal)(self,selector,target,l,t,callback);}
static id HPManualOnOff(id self,SEL selector,id target,BOOL on,id callback){HPManualStop();return ((id(*)(id,SEL,id,BOOL,id))HPManualOnOffOriginal)(self,selector,target,on,callback);}
static id HPManualBrightness(id self,SEL selector,id target,int l,id callback){HPManualStop();return ((id(*)(id,SEL,id,int,id))HPManualBrightnessOriginal)(self,selector,target,l,callback);}
static id HPDefaultFadeHSL(id cls,SEL selector,uint16_t target,uint16_t light,uint16_t hue,uint16_t saturation,NSInteger retries,NSInteger responses,BOOL ack,id success,id result){
    // The vendor's convenience method hard-codes a zero-duration transition.
    // Omitting its optional transition lets the bulb use its configured default.
    SEL full=NSSelectorFromString(@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:transitionTime:delay:retryCount:responseMaxCount:ack:successCallback:resultCallback:");
    return ((id(*)(id,SEL,uint16_t,uint16_t,uint16_t,uint16_t,id,uint8_t,NSInteger,NSInteger,BOOL,id,id))objc_msgSend)(cls,full,target,light,hue,saturation,nil,0,retries,responses,ack,success,result);
}
static void HPHook(Class cls,NSString *name,IMP replacement,IMP *original){
    Method method=class_getInstanceMethod(cls,NSSelectorFromString(name));if(method){IMP old=method_setImplementation(method,replacement);if(original)*original=old;}
}
static void HPInstallEnhancements(void){
    Class home=NSClassFromString(@"_TtC13SmartHome_Prd20ReHomeViewController");
    HPHook(home,@"collectionView:numberOfItemsInSection:",(IMP)HPHomeCount,&HPCountOriginal);
    HPHook(home,@"collectionView:cellForItemAtIndexPath:",(IMP)HPHomeCell,&HPCellOriginal);
    HPHook(home,@"collectionView:didSelectItemAtIndexPath:",(IMP)HPHomeTap,&HPTapOriginal);
    Class profile=NSClassFromString(@"_TtC13SmartHome_Prd26ReMenuProfileTableViewCell");
    HPHook(profile,@"awakeFromNib",(IMP)HPProfileAwake,&HPProfileAwakeOriginal);
    HPHook(profile,@"backButtonAction:",(IMP)HPNoExperience,NULL);
    HPHook(NSClassFromString(@"_TtC13SmartHome_Prd20ReMenuViewController"),@"tableView:heightForRowAtIndexPath:",(IMP)HPMenuHeight,&HPMenuHeightOriginal);
    Class api=NSClassFromString(@"DeviceControlApi");
    HPHook(api,@"setHSL:color:sat:lightness:onCompletion:",(IMP)HPManualHSL,&HPManualHSLOriginal);
    HPHook(api,@"setCTL:lightness:temp:onCompletion:",(IMP)HPManualCTL,&HPManualCTLOriginal);
    HPHook(api,@"setOnOff:state:onCompletion:",(IMP)HPManualOnOff,&HPManualOnOffOriginal);
    HPHook(api,@"setBrightness:level:onCompletion:",(IMP)HPManualBrightness,&HPManualBrightnessOriginal);
    Method hsl=class_getClassMethod(NSClassFromString(@"SDKLibCommand"),NSSelectorFromString(@"lightHSLSetWithDestination:HSLLight:HSLHue:HSLSaturation:retryCount:responseMaxCount:ack:successCallback:resultCallback:"));
    if(hsl)method_setImplementation(hsl,(IMP)HPDefaultFadeHSL);
    [NSNotificationCenter.defaultCenter addObserverForName:@"HP.OrderChanged" object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note){
        UICollectionView *collection=HPGet(HPHomeController,@"collectionView");
        if(!collection)return;
        objc_setAssociatedObject(collection,&HPShortcutDisableKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);[collection reloadData];
    }];
}
