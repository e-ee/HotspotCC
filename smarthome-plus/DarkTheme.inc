static UIColor *HPNightBackground(void){return [UIColor colorWithRed:.035 green:.045 blue:.065 alpha:1];}
static UIColor *HPNightSurface(void){return [UIColor colorWithRed:.085 green:.10 blue:.135 alpha:1];}
static char HPBadgeClearKey,HPBadgeSurfaceKey,HPBadgeTextKey,HPReadableLabelKey,HPWhiteNavKey;
static BOOL HPTheming,HPNavStyling;
static BOOL HPNeutral(UIColor *color,CGFloat *value){
    if(!color)return NO;CGFloat r,g,b,a;UIColor *resolved=[color resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark]];
    if(![resolved getRed:&r green:&g blue:&b alpha:&a]||a<.1)return NO;*value=(r+g+b)/3.;return MAX(r,MAX(g,b))-MIN(r,MIN(g,b))<.065;
}
static UIColor *HPReadableText(UIColor *color){CGFloat v;if(HPNeutral(color,&v)&&v<.72)return v<.35?UIColor.labelColor:UIColor.secondaryLabelColor;return color;}
static BOOL HPThemeExempt(UIView *view){
    for(UIView *v=view;v;v=v.superview){NSString *name=NSStringFromClass(v.class);if([v isKindOfClass:UICollectionViewCell.class]||[name containsString:@"ColorPicker"]||[name containsString:@"ColourPicker"])return YES;}
    return NO;
}
static BOOL HPBrightButtonText(UIView *view){
    for(UIView *v=view;v;v=v.superview)if([v isKindOfClass:UIButton.class]){CGFloat r,g,b,a;if([v.backgroundColor getRed:&r green:&g blue:&b alpha:&a]&&a>.5&&MIN(r,MIN(g,b))>.55)return YES;break;}return NO;
}
static NSAttributedString *HPReadableAttributes(UILabel *label,NSAttributedString *original){
    if(!original.length)return original;BOOL white=objc_getAssociatedObject(label,&HPBadgeTextKey)!=nil;
    if(!white&&!objc_getAssociatedObject(label,&HPReadableLabelKey))return original;
    NSMutableAttributedString *text=[original mutableCopy];__block BOOL changed=NO;
    [original enumerateAttribute:NSForegroundColorAttributeName inRange:NSMakeRange(0,original.length)options:0 usingBlock:^(id color,NSRange range,BOOL *stop){
        if([color isKindOfClass:UIColor.class]){UIColor *replacement=white?UIColor.whiteColor:HPReadableText(color);if(![replacement isEqual:color]){[text addAttribute:NSForegroundColorAttributeName value:replacement range:range];changed=YES;}}
    }];return changed?text:original;
}
static void HPThemeTree(UIView *view);
static void HPStyleBadge(UIView *badge){
    UIView *content=HPGet(badge,@"view");BOOL wrapper=[content isKindOfClass:UIView.class]&&content!=badge;objc_setAssociatedObject(badge,wrapper?&HPBadgeClearKey:&HPBadgeSurfaceKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);badge.backgroundColor=wrapper?UIColor.clearColor:HPNightSurface();
    if([content isKindOfClass:UIView.class]){objc_setAssociatedObject(content,&HPBadgeSurfaceKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);content.backgroundColor=HPNightSurface();}
    id label=HPGet(badge,@"title");if([label isKindOfClass:UILabel.class]){objc_setAssociatedObject(label,&HPBadgeTextKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);[label setTextColor:UIColor.whiteColor];NSAttributedString *old=[label attributedText],*text=HPReadableAttributes(label,old);if(text!=old)[label setAttributedText:text];}
    for(UIView *child in badge.subviews)HPThemeTree(child);
}
static void HPWhiteNavView(UIView *view){
    if(!view)return;objc_setAssociatedObject(view,&HPWhiteNavKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);view.tintColor=UIColor.whiteColor;
    if([view isKindOfClass:UIButton.class]){UIButton *b=(id)view;for(NSNumber *state in @[@(UIControlStateNormal),@(UIControlStateHighlighted),@(UIControlStateSelected)]){UIImage *image=[b imageForState:state.unsignedIntegerValue];if(image&&image.renderingMode!=UIImageRenderingModeAlwaysTemplate)[b setImage:[image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate]forState:state.unsignedIntegerValue];}}
    if([view isKindOfClass:UIImageView.class]){UIImageView *image=(id)view;if(image.image&&image.image.renderingMode!=UIImageRenderingModeAlwaysTemplate)image.image=[image.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];}
    for(UIView *child in view.subviews)HPWhiteNavView(child);
}
static void HPStyleNavigation(UINavigationItem *item){
    for(UIBarButtonItem *button in item.leftBarButtonItems){objc_setAssociatedObject(button,&HPWhiteNavKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);button.tintColor=UIColor.whiteColor;UIImage *image=button.image;if(image&&image.renderingMode!=UIImageRenderingModeAlwaysTemplate)button.image=[image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];HPWhiteNavView(button.customView);}
}
static void HPThemeTree(UIView *view){
    if(HPThemeExempt(view))return;NSString *name=NSStringFromClass(view.class);
    if([name hasSuffix:@"ReBLENetworkIndicatorButton"]){HPStyleBadge(view);return;}
    if([name hasSuffix:@"BLECustomBackgroundImage"]){view.hidden=YES;return;}
    CGFloat v;if([name hasSuffix:@"BLEBackgroundView"])view.backgroundColor=HPNightBackground();else if(HPNeutral(view.backgroundColor,&v)&&v>.72)view.backgroundColor=v>.97?HPNightBackground():HPNightSurface();
    if([view isKindOfClass:UITableView.class]){UITableView *table=(id)view;table.backgroundColor=HPNightBackground();table.separatorColor=[UIColor colorWithWhite:.22 alpha:1];}
    BOOL brightText=HPBrightButtonText(view);
    if([view isKindOfClass:UILabel.class]&&!brightText){UILabel *label=(id)view;objc_setAssociatedObject(label,&HPReadableLabelKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);UIColor *color=HPReadableText(label.textColor);if(color&&![label.textColor isEqual:color])label.textColor=color;NSAttributedString *old=label.attributedText,*text=HPReadableAttributes(label,old);if(text!=old)label.attributedText=text;}
    if([view isKindOfClass:UIButton.class]&&!brightText){UIButton *button=(id)view;for(NSNumber *state in @[@(UIControlStateNormal),@(UIControlStateHighlighted),@(UIControlStateSelected)]){UIColor *color=[button titleColorForState:state.unsignedIntegerValue],*replacement=HPReadableText(color);if(replacement&&![replacement isEqual:color])[button setTitleColor:replacement forState:state.unsignedIntegerValue];}}
    if([view isKindOfClass:UITextField.class]){UITextField *field=(id)view;field.keyboardAppearance=UIKeyboardAppearanceDark;UIColor *text=HPReadableText(field.textColor);if(text&&![field.textColor isEqual:text])field.textColor=text;if(field.placeholder.length){NSAttributedString *placeholder=[[NSAttributedString alloc]initWithString:field.placeholder attributes:@{NSForegroundColorAttributeName:UIColor.placeholderTextColor}];if(![field.attributedPlaceholder isEqual:placeholder])field.attributedPlaceholder=placeholder;}}
    if([view isKindOfClass:UITextView.class]){UITextView *field=(id)view;field.keyboardAppearance=UIKeyboardAppearanceDark;field.textColor=HPReadableText(field.textColor);}
    for(UIView *child in view.subviews)HPThemeTree(child);
}
static BOOL HPLegacyController(UIViewController *controller){NSString *name=NSStringFromClass(controller.class);if([name hasPrefix:@"HP"])return NO;return [name hasPrefix:@"SmartHome_Prd."]||[name hasPrefix:@"_TtC13SmartHome_Prd"]||[NSBundle bundleForClass:controller.class]==NSBundle.mainBundle;}
static void HPThemeController(UIViewController *controller){controller.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;controller.view.window.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;HPRemoveLogo(controller);if(HPLegacyController(controller)){HPStyleNavigation(controller.navigationItem);HPThemeTree(controller.view);}}
static IMP HPLayoutOriginal,HPViewWindowOriginal,HPBadgeLayoutOriginal,HPBackgroundOriginal,HPTextColorOriginal,HPAttributedOriginal,HPLabelLayoutOriginal,HPTintOriginal,HPBarTintOriginal,HPBarImageOriginal,HPButtonImageOriginal,HPImageOriginal,HPHiddenOriginal,HPNavLayoutOriginal;
static void HPNightLayout(id self,SEL selector){((void(*)(id,SEL))HPLayoutOriginal)(self,selector);if(!HPTheming&&[self isViewLoaded]){HPTheming=YES;HPThemeController(self);HPTheming=NO;}}
static void HPNightWindow(UIView *self,SEL selector){((void(*)(id,SEL))HPViewWindowOriginal)(self,selector);if(!self.window||HPTheming||HPThemeExempt(self))return;UIResponder *owner=self;while(owner&&![owner isKindOfClass:UIViewController.class])owner=owner.nextResponder;if(owner&&HPLegacyController((id)owner)){HPTheming=YES;HPThemeTree(self);HPTheming=NO;}}
static void HPBadgeLayout(id self,SEL selector){((void(*)(id,SEL))HPBadgeLayoutOriginal)(self,selector);HPStyleBadge(self);}
static void HPNightSetBackground(UIView *self,SEL selector,UIColor *color){if(objc_getAssociatedObject(self,&HPBadgeClearKey))color=UIColor.clearColor;else if(objc_getAssociatedObject(self,&HPBadgeSurfaceKey))color=HPNightSurface();((void(*)(id,SEL,id))HPBackgroundOriginal)(self,selector,color);}
static void HPNightText(UILabel *self,SEL selector,UIColor *color){if(objc_getAssociatedObject(self,&HPBadgeTextKey))color=UIColor.whiteColor;else if(objc_getAssociatedObject(self,&HPReadableLabelKey))color=HPReadableText(color);((void(*)(id,SEL,id))HPTextColorOriginal)(self,selector,color);}
static void HPNightAttributed(UILabel *self,SEL selector,NSAttributedString *text){((void(*)(id,SEL,id))HPAttributedOriginal)(self,selector,HPReadableAttributes(self,text));}
static void HPNightLabelLayout(UILabel *self,SEL selector){((void(*)(id,SEL))HPLabelLayoutOriginal)(self,selector);if(objc_getAssociatedObject(self,&HPReadableLabelKey)){UIColor *color=objc_getAssociatedObject(self,&HPBadgeTextKey)?UIColor.whiteColor:HPReadableText(self.textColor);if(color&&![color isEqual:self.textColor])self.textColor=color;NSAttributedString *old=self.attributedText,*text=HPReadableAttributes(self,old);if(text!=old)self.attributedText=text;}}
static void HPNightTint(id self,SEL selector,UIColor *color){if(objc_getAssociatedObject(self,&HPWhiteNavKey))color=UIColor.whiteColor;((void(*)(id,SEL,id))HPTintOriginal)(self,selector,color);}
static void HPNightBarTint(id self,SEL selector,UIColor *color){if(objc_getAssociatedObject(self,&HPWhiteNavKey))color=UIColor.whiteColor;((void(*)(id,SEL,id))HPBarTintOriginal)(self,selector,color);}
static UIImage *HPNavImage(id self,UIImage *image){return objc_getAssociatedObject(self,&HPWhiteNavKey)&&image?[image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate]:image;}
static void HPNightBarImage(id self,SEL selector,UIImage *image){((void(*)(id,SEL,id))HPBarImageOriginal)(self,selector,HPNavImage(self,image));}
static void HPNightButtonImage(id self,SEL selector,UIImage *image,UIControlState state){((void(*)(id,SEL,id,UIControlState))HPButtonImageOriginal)(self,selector,HPNavImage(self,image),state);}
static void HPNightImage(id self,SEL selector,UIImage *image){((void(*)(id,SEL,id))HPImageOriginal)(self,selector,HPNavImage(self,image));}
static void HPNightHidden(id self,SEL selector,BOOL hidden){if(objc_getAssociatedObject(self,&HPRemovedLogoKey))hidden=YES;((void(*)(id,SEL,BOOL))HPHiddenOriginal)(self,selector,hidden);}
static void HPNightNavLayout(UINavigationBar *self,SEL selector){((void(*)(id,SEL))HPNavLayoutOriginal)(self,selector);if(HPNavStyling)return;HPNavStyling=YES;UIViewController *top=HPTop();if(top&&HPLegacyController(top)){HPRemoveLogo(top);HPStyleNavigation(self.topItem);}HPNavStyling=NO;}
static void HPInstallDarkTheme(void){
    [UINavigationBar appearance].barStyle=UIBarStyleBlack;[UIToolbar appearance].barStyle=UIBarStyleBlack;
    UINavigationBarAppearance *appearance=[UINavigationBarAppearance new];[appearance configureWithOpaqueBackground];appearance.backgroundColor=HPNightBackground();appearance.titleTextAttributes=@{NSForegroundColorAttributeName:UIColor.labelColor};appearance.largeTitleTextAttributes=appearance.titleTextAttributes;
    [UINavigationBar appearance].standardAppearance=appearance;[UINavigationBar appearance].scrollEdgeAppearance=appearance;[UINavigationBar appearance].compactAppearance=appearance;
    HPHook(UIViewController.class,@"viewDidLayoutSubviews",(IMP)HPNightLayout,&HPLayoutOriginal);HPHook(UIView.class,@"didMoveToWindow",(IMP)HPNightWindow,&HPViewWindowOriginal);
    HPHook(NSClassFromString(@"_TtC13SmartHome_Prd27ReBLENetworkIndicatorButton"),@"layoutSubviews",(IMP)HPBadgeLayout,&HPBadgeLayoutOriginal);
    HPHook(UIView.class,@"setBackgroundColor:",(IMP)HPNightSetBackground,&HPBackgroundOriginal);HPHook(UILabel.class,@"setTextColor:",(IMP)HPNightText,&HPTextColorOriginal);
    HPHook(UILabel.class,@"setAttributedText:",(IMP)HPNightAttributed,&HPAttributedOriginal);HPHook(UILabel.class,@"layoutSubviews",(IMP)HPNightLabelLayout,&HPLabelLayoutOriginal);
    HPHook(UIView.class,@"setTintColor:",(IMP)HPNightTint,&HPTintOriginal);HPHook(UIBarButtonItem.class,@"setTintColor:",(IMP)HPNightBarTint,&HPBarTintOriginal);
    HPHook(UIBarButtonItem.class,@"setImage:",(IMP)HPNightBarImage,&HPBarImageOriginal);HPHook(UIButton.class,@"setImage:forState:",(IMP)HPNightButtonImage,&HPButtonImageOriginal);HPHook(UIImageView.class,@"setImage:",(IMP)HPNightImage,&HPImageOriginal);
    HPHook(UIView.class,@"setHidden:",(IMP)HPNightHidden,&HPHiddenOriginal);HPHook(UINavigationBar.class,@"layoutSubviews",(IMP)HPNightNavLayout,&HPNavLayoutOriginal);
}
