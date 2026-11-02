// A separate non-key window survives modal presentation. Only the capsules accept
// touches; every other point passes through to the app or document picker.
@interface HPFloatingWindow : UIWindow @end
@implementation HPFloatingWindow
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event{UIView *hit=[super hitTest:point withEvent:event];return hit==self||hit==self.rootViewController.view?nil:hit;}
@end
@interface HPFloatingRoot : UIViewController @end
@implementation HPFloatingRoot
- (void)loadView{self.view=[[UIView alloc]initWithFrame:UIScreen.mainScreen.bounds];self.view.backgroundColor=UIColor.clearColor;}
- (void)viewDidLayoutSubviews{[super viewDidLayoutSubviews];[HPManager.shared layoutButtons];}
- (UIStatusBarStyle)preferredStatusBarStyle{return UIStatusBarStyleLightContent;}
@end
static NSString *HPFloatKey(UIButton *button){return button.tag==1?@"HP.Float.PLUS":@"HP.Float.STUDIO";}
@implementation HPManager
+ (instancetype)shared{static HPManager *value;static dispatch_once_t once;dispatch_once(&once,^{value=[HPManager new];});return value;}
- (UIButton *)makeShortcut:(NSString *)title tag:(NSInteger)tag action:(SEL)action{
    UIButton *button=[UIButton buttonWithType:UIButtonTypeCustom];button.tag=tag;
    button.accessibilityIdentifier=tag==1?@"HP.PLUS":@"HP.STUDIO";
    [button setTitle:title forState:UIControlStateNormal];[button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    [button setTitleColor:UIColor.whiteColor forState:UIControlStateHighlighted];
    button.titleLabel.font=[UIFont systemFontOfSize:11 weight:UIFontWeightBold];
    button.backgroundColor=[UIColor colorWithRed:.16 green:.19 blue:.25 alpha:.45];button.alpha=1;
    button.layer.cornerRadius=21;button.layer.borderWidth=.5;button.layer.borderColor=[UIColor colorWithWhite:1 alpha:.12].CGColor;
    button.tintAdjustmentMode=UIViewTintAdjustmentModeNormal;
    button.accessibilityLabel=tag==1?@"Open Plus controls. Drag to move.":@"Open Light Studio. Drag to move.";
    [button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
    UIPanGestureRecognizer *pan=[[UIPanGestureRecognizer alloc]initWithTarget:self action:@selector(drag:)];pan.maximumNumberOfTouches=1;[button addGestureRecognizer:pan];
    return button;
}
- (CGRect)centerLimits{
    UIView *view=self.overlay.rootViewController.view;UIEdgeInsets safe=view.safeAreaInsets;
    CGFloat bottom=view.bounds.size.height-safe.bottom-8;
    if(self.keyboardVisible){CGRect keyboard=[self.overlay convertRect:self.keyboardFrame fromWindow:nil];if(CGRectIntersectsRect(keyboard,view.bounds))bottom=MIN(bottom,CGRectGetMinY(keyboard)-8);}
    CGFloat left=safe.left+8+36,right=view.bounds.size.width-safe.right-8-36,top=safe.top+8+21;
    bottom=MAX(top,bottom-21);right=MAX(left,right);return CGRectMake(left,top,right-left,bottom-top);
}
- (CGPoint)clampedCenter:(CGPoint)point{
    CGRect limits=[self centerLimits];return CGPointMake(MAX(CGRectGetMinX(limits),MIN(CGRectGetMaxX(limits),point.x)),MAX(CGRectGetMinY(limits),MIN(CGRectGetMaxY(limits),point.y)));
}
- (CGPoint)storedCenter:(UIButton *)button{
    CGRect limits=[self centerLimits];NSArray *saved=[NSUserDefaults.standardUserDefaults arrayForKey:HPFloatKey(button)];
    double x=1,y=button.tag==1?.86:MAX(0,.86-56/MAX(1,limits.size.height));
    if(saved.count==2&&[saved[0]isKindOfClass:NSNumber.class]&&[saved[1]isKindOfClass:NSNumber.class]){double sx=[saved[0]doubleValue],sy=[saved[1]doubleValue];if(isfinite(sx)&&isfinite(sy)){x=HPClamp(sx,0,1);y=HPClamp(sy,0,1);}}
    return CGPointMake(limits.origin.x+x*limits.size.width,limits.origin.y+y*limits.size.height);
}
- (CGPoint)separatedCenter:(CGPoint)point forButton:(UIButton *)button{
    UIButton *other=button==self.button?self.studioButton:self.button;if(!other.superview)return point;
    CGRect proposed=CGRectMake(point.x-36,point.y-21,72,42);
    if(!CGRectIntersectsRect(CGRectInset(proposed,-4,-4),other.frame))return point;
    CGPoint choices[]={CGPointMake(point.x,other.center.y-52),CGPointMake(point.x,other.center.y+52),CGPointMake(other.center.x-82,point.y),CGPointMake(other.center.x+82,point.y)};
    double best=DBL_MAX;CGPoint result=point;
    for(int i=0;i<4;i++){CGPoint candidate=[self clampedCenter:choices[i]];CGRect frame=CGRectMake(candidate.x-36,candidate.y-21,72,42);if(CGRectIntersectsRect(CGRectInset(frame,-3,-3),other.frame))continue;double distance=hypot(candidate.x-point.x,candidate.y-point.y);if(distance<best){best=distance;result=candidate;}}
    return result;
}
- (void)moveButton:(UIButton *)button to:(CGPoint)point remember:(BOOL)remember{
    point=[self separatedCenter:[self clampedCenter:point]forButton:button];button.bounds=CGRectMake(0,0,72,42);button.center=point;
    if(remember){CGRect limits=[self centerLimits];double x=(point.x-limits.origin.x)/MAX(1,limits.size.width),y=(point.y-limits.origin.y)/MAX(1,limits.size.height);[NSUserDefaults.standardUserDefaults setObject:@[@(HPClamp(x,0,1)),@(HPClamp(y,0,1))]forKey:HPFloatKey(button)];[NSUserDefaults.standardUserDefaults synchronize];}
}
- (void)layoutButtons{
    if(!self.overlay)return;
    for(UIButton *button in @[self.button,self.studioButton]){
        if(button!=self.draggingButton)[self moveButton:button to:[self storedCenter:button]remember:NO];
        button.hidden=NO;button.alpha=1;button.enabled=YES;
    }
}
- (void)drag:(UIPanGestureRecognizer *)gesture{
    UIButton *button=(id)gesture.view;
    if(gesture.state==UIGestureRecognizerStateBegan){self.draggingButton=button;self.dragStart=button.center;}
    CGPoint offset=[gesture translationInView:self.overlay.rootViewController.view];
    [self moveButton:button to:CGPointMake(self.dragStart.x+offset.x,self.dragStart.y+offset.y)remember:NO];
    if(gesture.state==UIGestureRecognizerStateEnded||gesture.state==UIGestureRecognizerStateCancelled){[self moveButton:button to:button.center remember:YES];self.draggingButton=nil;}
}
- (void)install{
    if(self.installing)return;UIWindow *content=HPContentWindow();if(!content)return;self.installing=YES;
    if(!self.button){self.button=[self makeShortcut:@"PLUS" tag:1 action:@selector(show)];self.studioButton=[self makeShortcut:@"STUDIO" tag:2 action:@selector(showStudio)];}
    if(!self.overlay||self.overlay.windowScene!=content.windowScene){
        self.overlay.hidden=YES;
        self.overlay=content.windowScene?[[HPFloatingWindow alloc]initWithWindowScene:content.windowScene]:[[HPFloatingWindow alloc]initWithFrame:content.frame];
        self.overlay.accessibilityIdentifier=@"HP.FloatingWindow";self.overlay.backgroundColor=UIColor.clearColor;self.overlay.windowLevel=UIWindowLevelAlert+1;
        self.overlay.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;self.overlay.rootViewController=[HPFloatingRoot new];
        [self.overlay.rootViewController.view addSubview:self.button];[self.overlay.rootViewController.view addSubview:self.studioButton];
    }
    self.overlay.frame=content.frame;
    UIView *root=self.overlay.rootViewController.view;
    if(self.button.superview!=root)[root addSubview:self.button];if(self.studioButton.superview!=root)[root addSubview:self.studioButton];
    self.overlay.hidden=NO;[root bringSubviewToFront:self.button];[root bringSubviewToFront:self.studioButton];[self layoutButtons];self.installing=NO;
    if(!self.visibilityTimer){
        self.visibilityTimer=[NSTimer timerWithTimeInterval:1 target:self selector:@selector(keepVisible) userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:self.visibilityTimer forMode:NSRunLoopCommonModes];
        [NSNotificationCenter.defaultCenter addObserverForName:UIKeyboardWillChangeFrameNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note){
            self.keyboardFrame=[note.userInfo[UIKeyboardFrameEndUserInfoKey]CGRectValue];self.keyboardVisible=CGRectGetMinY(self.keyboardFrame)<UIScreen.mainScreen.bounds.size.height;[self layoutButtons];
        }];
    }
}
- (void)keepVisible{if(UIApplication.sharedApplication.applicationState!=UIApplicationStateActive)return;if(self.overlay.hidden||!self.button.superview||!self.studioButton.superview||self.button.hidden||self.studioButton.hidden)[self install];}
- (void)openController:(UIViewController *)controller{
    if([NSUserDefaults.standardUserDefaults boolForKey:@"HP.JoinPending"]&&![controller isKindOfClass:HPPanel.class])controller=[[HPPanel alloc]initWithStyle:UITableViewStyleInsetGrouped];
    UIViewController *top=HPTop();if(!top||[top isKindOfClass:UIAlertController.class]||[top isKindOfClass:UIDocumentPickerViewController.class])return;
    if([top isKindOfClass:controller.class])return;
    BOOL tools=[top isKindOfClass:HPSpatialEditor.class]||[top isKindOfClass:HPPanel.class]||[top isKindOfClass:HPStudio.class]||[top isKindOfClass:HPPathEditor.class]||[top isKindOfClass:HPBulbSettings.class];
    if(tools&&top.navigationController){
        for(UIViewController *existing in top.navigationController.viewControllers)if([existing isKindOfClass:controller.class]){[top.navigationController popToViewController:existing animated:YES];return;}
        [top.navigationController pushViewController:controller animated:YES];return;
    }
    if([controller isKindOfClass:HPStudio.class])controller.navigationItem.leftBarButtonItem=[[UIBarButtonItem alloc]initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(dismissTools)];
    UINavigationController *navigation=[[UINavigationController alloc]initWithRootViewController:controller];if([NSUserDefaults.standardUserDefaults boolForKey:@"HP.JoinPending"])navigation.modalPresentationStyle=UIModalPresentationFullScreen;[top presentViewController:navigation animated:YES completion:nil];
}
- (void)show{[self openController:[[HPPanel alloc]initWithStyle:UITableViewStyleInsetGrouped]];}
- (void)showStudio{[self openController:[HPStudio new]];}
- (void)dismissTools{[HPTop()dismissViewControllerAnimated:YES completion:nil];}
@end
