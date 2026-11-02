static UIColor *HPNightBackground(void){return [UIColor colorWithRed:.035 green:.045 blue:.065 alpha:1];}
static UIColor *HPNightSurface(void){return [UIColor colorWithRed:.085 green:.10 blue:.135 alpha:1];}
static BOOL HPNeutral(UIColor *color,CGFloat *value){
    if(!color)return NO;CGFloat r,g,b,a;
    UIColor *resolved=[color resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark]];
    if(![resolved getRed:&r green:&g blue:&b alpha:&a]||a<.1)return NO;
    *value=(r+g+b)/3.;return MAX(r,MAX(g,b))-MIN(r,MIN(g,b))<.065;
}
static UIColor *HPReadableText(UIColor *color){CGFloat value;if(HPNeutral(color,&value)&&value<.72)return value<.35?UIColor.labelColor:UIColor.secondaryLabelColor;return color;}
static void HPThemeTree(UIView *view){
    NSString *name=NSStringFromClass(view.class);
    // Preserve the actual colours in scene chips and colour pickers.
    if([view isKindOfClass:UICollectionViewCell.class]||[name containsString:@"ColorPicker"]||[name containsString:@"ColourPicker"])return;
    if([name hasSuffix:@"BLECustomBackgroundImage"]){view.hidden=YES;return;}
    CGFloat value;
    if([name hasSuffix:@"BLEBackgroundView"])view.backgroundColor=HPNightBackground();
    else if(HPNeutral(view.backgroundColor,&value)&&value>.72)view.backgroundColor=value>.97?HPNightBackground():HPNightSurface();
    if([view isKindOfClass:UITableView.class]){UITableView *table=(id)view;table.backgroundColor=HPNightBackground();table.separatorColor=[UIColor colorWithWhite:.22 alpha:1];}
    if([view isKindOfClass:UILabel.class]){
        UILabel *label=(id)view;UIColor *color=HPReadableText(label.textColor);if(color&&![label.textColor isEqual:color])label.textColor=color;
        NSAttributedString *original=label.attributedText;
        if(original.length){NSMutableAttributedString *text=[original mutableCopy];__block BOOL changed=NO;
            [original enumerateAttribute:NSForegroundColorAttributeName inRange:NSMakeRange(0,original.length) options:0 usingBlock:^(id color,NSRange range,BOOL *stop){if([color isKindOfClass:UIColor.class]){UIColor *replacement=HPReadableText(color);if(![replacement isEqual:color]){[text addAttribute:NSForegroundColorAttributeName value:replacement range:range];changed=YES;}}}];
            if(changed)label.attributedText=text;
        }
    }
    if([view isKindOfClass:UIButton.class]){UIButton *button=(id)view;for(NSNumber *state in @[@(UIControlStateNormal),@(UIControlStateHighlighted),@(UIControlStateSelected)]){UIColor *color=[button titleColorForState:state.unsignedIntegerValue];UIColor *replacement=HPReadableText(color);if(replacement&&![replacement isEqual:color])[button setTitleColor:replacement forState:state.unsignedIntegerValue];}}
    if([view isKindOfClass:UITextField.class]){UITextField *field=(id)view;field.keyboardAppearance=UIKeyboardAppearanceDark;field.textColor=HPReadableText(field.textColor);if(field.placeholder.length)field.attributedPlaceholder=[[NSAttributedString alloc]initWithString:field.placeholder attributes:@{NSForegroundColorAttributeName:UIColor.placeholderTextColor}];}
    if([view isKindOfClass:UITextView.class]){UITextView *field=(id)view;field.keyboardAppearance=UIKeyboardAppearanceDark;field.textColor=HPReadableText(field.textColor);}
    for(UIView *child in view.subviews)HPThemeTree(child);
}
static BOOL HPLegacyController(UIViewController *controller){NSString *name=NSStringFromClass(controller.class);return [name hasPrefix:@"SmartHome_Prd."]||[name hasPrefix:@"_TtC13SmartHome_Prd"]||[NSBundle bundleForClass:controller.class]==NSBundle.mainBundle;}
static void HPThemeController(UIViewController *controller){
    controller.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    controller.view.window.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    if(HPLegacyController(controller))HPThemeTree(controller.view);
}
static IMP HPLayoutOriginal,HPViewWindowOriginal;
static BOOL HPTheming;
static void HPNightLayout(id self,SEL selector){
    ((void(*)(id,SEL))HPLayoutOriginal)(self,selector);
    if(!HPTheming&&[self isViewLoaded]){HPTheming=YES;HPThemeController(self);HPTheming=NO;}
}
static void HPNightWindow(UIView *self,SEL selector){
    ((void(*)(id,SEL))HPViewWindowOriginal)(self,selector);
    if(!self.window||HPTheming)return;
    UIResponder *owner=self;while(owner&&![owner isKindOfClass:UIViewController.class])owner=owner.nextResponder;
    if(owner&&HPLegacyController((id)owner)){HPTheming=YES;HPThemeTree(self);HPTheming=NO;}
}
static void HPInstallDarkTheme(void){
    [UINavigationBar appearance].barStyle=UIBarStyleBlack;
    [UIToolbar appearance].barStyle=UIBarStyleBlack;
    UINavigationBarAppearance *appearance=[UINavigationBarAppearance new];[appearance configureWithOpaqueBackground];appearance.backgroundColor=HPNightBackground();appearance.titleTextAttributes=@{NSForegroundColorAttributeName:UIColor.labelColor};appearance.largeTitleTextAttributes=appearance.titleTextAttributes;
    [UINavigationBar appearance].standardAppearance=appearance;[UINavigationBar appearance].scrollEdgeAppearance=appearance;[UINavigationBar appearance].compactAppearance=appearance;
    HPHook(UIViewController.class,@"viewDidLayoutSubviews",(IMP)HPNightLayout,&HPLayoutOriginal);
    HPHook(UIView.class,@"didMoveToWindow",(IMP)HPNightWindow,&HPViewWindowOriginal);
}
