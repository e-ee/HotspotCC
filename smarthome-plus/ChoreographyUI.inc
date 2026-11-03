@interface HPTouchCanvas : HPStudioPreview
@property(nonatomic) BOOL painting;
@property(nonatomic) HPColor brush;
@property(nonatomic) double radius;
@property(nonatomic,strong) NSMutableArray *undoLooks;
- (void)touchAt:(CGPoint)point beginning:(BOOL)beginning;
@end
@implementation HPTouchCanvas
- (instancetype)initWithFrame:(CGRect)frame{if((self=[super initWithFrame:frame])){self.radius=.35;self.brush=(HPColor){.76,.85,.32};self.undoLooks=[NSMutableArray new];[self addGestureRecognizer:[[UIPanGestureRecognizer alloc]initWithTarget:self action:@selector(pan:)]];[self addGestureRecognizer:[[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(tap:)]];}return self;}
- (void)touchAt:(CGPoint)point beginning:(BOOL)beginning{
 CGRect plot=CGRectInset(self.bounds,19,14);CGPoint normalized=CGPointMake(HPClamp((point.x-plot.origin.x)/MAX(1,plot.size.width),0,1),HPClamp((point.y-plot.origin.y)/MAX(1,plot.size.height),0,1));
 HPLightEngine *e=HPLightEngine.shared;if(self.painting){if(beginning){[self.undoLooks addObject:[e snapshotLook]];if(self.undoLooks.count>20)[self.undoLooks removeObjectAtIndex:0];}[e paintAt:normalized colour:self.brush radius:self.radius];}else [e rippleAt:normalized];[self setNeedsDisplay];
}
- (void)tap:(UITapGestureRecognizer *)g{[self touchAt:[g locationInView:self]beginning:YES];}
- (void)pan:(UIPanGestureRecognizer *)g{if(self.painting||g.state==UIGestureRecognizerStateBegan)[self touchAt:[g locationInView:self]beginning:g.state==UIGestureRecognizerStateBegan];}
- (void)drawRect:(CGRect)rect{[super drawRect:rect];CGRect plot=CGRectInset(self.bounds,19,14);NSUInteger i=0;for(NSDictionary *light in self.lights){NSString *number=[NSString stringWithFormat:@"%lu",(unsigned long)++i];CGPoint p=CGPointMake(plot.origin.x+[light[@"x"]doubleValue]*plot.size.width+7,plot.origin.y+[light[@"y"]doubleValue]*plot.size.height-7);[number drawAtPoint:p withAttributes:@{NSForegroundColorAttributeName:UIColor.whiteColor,NSFontAttributeName:[UIFont systemFontOfSize:10]}];}
 HPLightEngine *e=HPLightEngine.shared;if(!self.painting){CGPoint p=CGPointMake(plot.origin.x+e.rippleOrigin.x*plot.size.width,plot.origin.y+e.rippleOrigin.y*plot.size.height);CGContextRef c=UIGraphicsGetCurrentContext();CGContextSetStrokeColorWithColor(c,[UIColor colorWithWhite:1 alpha:.6].CGColor);CGContextStrokeEllipseInRect(c,CGRectMake(p.x-12,p.y-12,24,24));}}
@end
@interface HPTouchStudio : UIViewController<UIColorPickerViewControllerDelegate>
@property(nonatomic,strong) HPTouchCanvas *canvas;
@property(nonatomic,strong) NSTimer *timer;
@end
@implementation HPTouchStudio
- (void)viewDidLoad{[super viewDidLoad];self.title=@"Paint & touch";self.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;self.view.backgroundColor=[UIColor colorWithRed:.035 green:.045 blue:.07 alpha:1];
 UIStackView *stack=[UIStackView new];stack.axis=UILayoutConstraintAxisVertical;stack.spacing=14;stack.translatesAutoresizingMaskIntoConstraints=NO;[self.view addSubview:stack];
 [NSLayoutConstraint activateConstraints:@[[stack.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:16],[stack.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:20],[stack.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-20],[stack.bottomAnchor constraintLessThanOrEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor constant:-16]]];
 UILabel *hint=[UILabel new];hint.numberOfLines=0;hint.font=[UIFont systemFontOfSize:13];hint.textColor=UIColor.lightGrayColor;hint.text=@"Paint blends colour into nearby bulbs. Ripple sends a soft wave from your touch. Your apartment map supplies the positions.";[stack addArrangedSubview:hint];
 UISegmentedControl *mode=[[UISegmentedControl alloc]initWithItems:@[@"Paint",@"Ripple"]];mode.selectedSegmentIndex=0;[mode addTarget:self action:@selector(mode:)forControlEvents:UIControlEventValueChanged];[stack addArrangedSubview:mode];
 self.canvas=[HPTouchCanvas new];self.canvas.painting=YES;self.canvas.lights=HPLightEngine.shared.lights;self.canvas.backgroundColor=[UIColor colorWithRed:.06 green:.075 blue:.10 alpha:1];self.canvas.layer.cornerRadius=20;self.canvas.accessibilityLabel=@"Interactive apartment canvas";self.canvas.accessibilityHint=@"Drag to paint. Switch to Ripple and tap to choose its origin.";[self.canvas.heightAnchor constraintEqualToConstant:MAX(180,MIN(420,UIScreen.mainScreen.bounds.size.height*.40))].active=YES;[stack addArrangedSubview:self.canvas];
 UIStackView *buttons=[UIStackView new];buttons.spacing=8;buttons.distribution=UIStackViewDistributionFillEqually;for(NSString *title in @[@"Colour",@"Undo",@"Pause / Play"]){UIButton *b=[UIButton buttonWithType:UIButtonTypeSystem];[b setTitle:title forState:UIControlStateNormal];[b setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];b.backgroundColor=[UIColor colorWithWhite:.12 alpha:1];b.layer.cornerRadius=12;[b.heightAnchor constraintEqualToConstant:44].active=YES;[b addTarget:self action:[title isEqual:@"Colour"]?@selector(colour):[title isEqual:@"Undo"]?@selector(undo):@selector(pause)forControlEvents:UIControlEventTouchUpInside];[buttons addArrangedSubview:b];}[stack addArrangedSubview:buttons];
 UILabel *radius=[UILabel new];radius.text=@"Brush reach";radius.textColor=UIColor.whiteColor;[stack addArrangedSubview:radius];UISlider *slider=[UISlider new];slider.minimumValue=.12;slider.maximumValue=.8;slider.value=.35;slider.accessibilityLabel=@"Brush reach";[slider addTarget:self action:@selector(radius:)forControlEvents:UIControlEventValueChanged];[stack addArrangedSubview:slider];
 UILabel *save=[UILabel new];save.numberOfLines=0;save.font=[UIFont systemFontOfSize:12];save.textColor=UIColor.lightGrayColor;save.text=@"Return to Studio and pause to save a scene. Create → Explore lets you save a painted look as a reusable variation.";[stack addArrangedSubview:save];
}
- (void)viewWillAppear:(BOOL)animated{[super viewWillAppear:animated];self.timer=[NSTimer scheduledTimerWithTimeInterval:.2 target:self.canvas selector:@selector(setNeedsDisplay)userInfo:nil repeats:YES];}
- (void)viewDidDisappear:(BOOL)animated{[super viewDidDisappear:animated];[self.timer invalidate];self.timer=nil;}
- (void)mode:(UISegmentedControl *)s{self.canvas.painting=s.selectedSegmentIndex==0;}
- (void)radius:(UISlider *)s{self.canvas.radius=s.value;}
- (void)pause{HPLightEngine *e=HPLightEngine.shared;if(e.mode>=0)[e stop];else[e resume];}
- (void)undo{if(!self.canvas.undoLooks.count)return;HPLightEngine *e=HPLightEngine.shared;NSDictionary *look=self.canvas.undoLooks.lastObject;[self.canvas.undoLooks removeLastObject];[e beginCanvas];e.painted=[look mutableCopy];}
- (void)colour{UIColorPickerViewController *picker=[UIColorPickerViewController new];picker.supportsAlpha=NO;HPColor c=self.canvas.brush;picker.selectedColor=[UIColor colorWithHue:c.hue saturation:c.saturation brightness:c.lightness alpha:1];picker.delegate=self;[self presentViewController:picker animated:YES completion:nil];}
- (void)colorPickerViewControllerDidSelectColor:(UIColorPickerViewController *)picker{CGFloat h,s,b,a;[picker.selectedColor getHue:&h saturation:&s brightness:&b alpha:&a];self.canvas.brush=(HPColor){h,s,b};}
@end

@interface HPStudioTools : UITableViewController
@property(nonatomic) NSInteger screen,selectedStep;
@property(nonatomic,strong) NSArray *choices;
@property(nonatomic,copy) void(^picked)(NSDictionary *);
@property(nonatomic,strong) NSDictionary *sceneA,*sceneB;
@property(nonatomic,strong) NSMutableArray *steps;
@property(nonatomic,strong) NSString *journeyName,*journeyID;
@property(nonatomic) BOOL loop;
@property(nonatomic) double duration,stagger;
@end
@implementation HPStudioTools
- (instancetype)init{if((self=[super initWithStyle:UITableViewStyleInsetGrouped])){self.duration=120;self.stagger=.25;self.steps=[NSMutableArray new];self.journeyName=@"Evening journey";self.loop=YES;}return self;}
- (void)viewDidLoad{[super viewDidLoad];self.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;self.tableView.backgroundColor=[UIColor colorWithRed:.035 green:.045 blue:.07 alpha:1];self.tableView.rowHeight=UITableViewAutomaticDimension;self.tableView.estimatedRowHeight=58;
 NSArray *names=@[@"Create",@"Wave direction",@"Explore this look",@"Scene morph",@"Journey composer",@"Sleep descent",@"Choose",@"My journeys",@"Chapter timing"];
 self.title=names[MIN(8,self.screen)];if(self.screen==4){self.navigationItem.rightBarButtonItem=self.editButtonItem;}
}
- (void)viewWillAppear:(BOOL)animated{[super viewWillAppear:animated];[self.tableView reloadData];}
- (void)open:(NSInteger)screen{HPStudioTools *next=[HPStudioTools new];next.screen=screen;[self.navigationController pushViewController:next animated:YES];}
- (void)pick:(NSArray *)choices title:(NSString *)title completion:(void(^)(NSDictionary *))completion{HPStudioTools *next=[HPStudioTools new];next.screen=6;next.choices=choices;next.picked=completion;[self.navigationController pushViewController:next animated:YES];next.title=title;}
- (NSArray *)sceneChoices{NSMutableArray *choices=[NSMutableArray new];@try{for(NSDictionary *scene in HPExportRoutines()){NSDictionary *look=HPSceneLook(scene,HPLightRecords());if(look.count)[choices addObject:@{@"name":scene[@"routineName"],@"look":look}];}}@catch(NSException *exception){HPAlert(self,@"Scenes unavailable",exception.reason);}return choices;}
- (NSArray *)chapterChoices{NSMutableArray *choices=[[self sceneChoices]mutableCopy];HPLightEngine *e=HPLightEngine.shared;for(int mode=0;mode<HP_MODE_COUNT;mode++){NSMutableDictionary *r=[[e recipeNamed:HPThemeNames()[mode]]mutableCopy];r[@"mode"]=@(mode);r[@"journey"]=@-1;r[@"creativeKind"]=@0;r[@"phase"]=@0;r[@"variation"]=@0;[choices addObject:@{@"name":HPThemeNames()[mode],@"recipe":r}];}for(NSDictionary *r in HPSavedMoods())[choices addObject:@{@"name":r[@"name"],@"recipe":r}];return choices;}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)table{return self.screen==4?3:1;}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section{
 switch(self.screen){case 0:return 9;case 1:return 5;case 2:return 8;case 3:return 5;case 4:return section==0?4:section==1?self.steps.count:2;case 5:return 6;case 6:return self.choices.count;case 7:return HPJourneysSaved().count+1;case 8:return 3;default:return 0;}
}
- (NSString *)tableView:(UITableView *)table titleForFooterInSection:(NSInteger)section{
 if(self.screen==0)return @"All effects use the existing Bluetooth connection. Changes blend through the same paced sender as Studio.";
 if(self.screen==1)return @"Angle follows your apartment map: 0° left to right, 90° top to bottom. Reverse flips travel. Inward/outward applies to touch ripples. Tap the touch canvas to choose their origin.";
 if(self.screen==2)return @"Your starting look stays intact. Each variation is calculated from it, so repeated taps do not accumulate changes. Save a variation to My moods, or pause and save a scene in Studio.";
 if(self.screen==3)return @"Scenes contribute their HSL bulbs and off states. Missing bulbs retain their current look. CT-only settings are omitted. A stagger sends the dissolve across your mapped room.";
 if(self.screen==4&&section==1)return @"Tap a chapter to edit its hold and dissolve times. Edit lets you reorder or delete chapters. Save keeps the journey; Play starts it.";
 if(self.screen==5)return @"Movement eases, colour warms, and brightness fades to off. The timer measures real active time, independent of Speed. Pausing Studio also pauses the descent.";
 if(self.screen==6&&!self.choices.count)return @"No compatible saved scenes yet. Pause a Studio look and save it as a scene first.";
 return nil;
}
- (UITableViewCell *)cell:(NSString *)title detail:(NSString *)detail{UITableViewCell *cell=[[UITableViewCell alloc]initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];cell.textLabel.text=title;cell.textLabel.textColor=UIColor.whiteColor;cell.textLabel.numberOfLines=0;cell.detailTextLabel.text=detail;cell.detailTextLabel.numberOfLines=0;cell.detailTextLabel.textColor=UIColor.lightGrayColor;cell.backgroundColor=[UIColor colorWithRed:.075 green:.085 blue:.12 alpha:1];return cell;}
- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)p{
 HPLightEngine *e=HPLightEngine.shared;NSInteger n=p.row;
 if(self.screen==0){NSArray *titles=@[@"Living Aurora",@"Paint the room & touch ripples",@"Morph between scenes",@"Compose a journey",@"Explore around this look",@"New colour variation",@"Save this variation",@"Sleep descent",@"Wave direction"];
 NSArray *details=@[@"Layered jade, blue and violet curtains",@"Blend colours with your finger; tap to send waves",@"Smooth dissolves with spatial stagger",@"Arrange moods and scenes into an evening",@"Warmer, cooler, deeper, softer or darker",@"Fresh colours while keeping the mood’s character",@"Keep this palette and its starting point",@"Gradually warm and dim the room to off",@"Angle, reverse and ripple travel"];
 return [self cell:titles[n]detail:details[n]];}
 if(self.screen==1){if(n==0){UITableViewCell *cell=[self cell:[NSString stringWithFormat:@"Angle %.0f°",e.waveAngle]detail:nil];UISlider *slider=[[UISlider alloc]initWithFrame:CGRectMake(0,0,145,40)];slider.minimumValue=0;slider.maximumValue=360;slider.value=e.waveAngle;slider.accessibilityLabel=@"Wave direction angle";[slider addTarget:self action:@selector(angle:)forControlEvents:UIControlEventValueChanged];cell.accessoryView=slider;cell.selectionStyle=UITableViewCellSelectionStyleNone;return cell;}
 return [self cell:@[@"",@"Reverse waves",@"Touch ripple direction",@"Reset direction",@"Open touch canvas"][n]detail:n==1?(e.waveReverse?@"Reversed":@"Forward"):n==2?(e.rippleInward?@"Inward":@"Outward"):nil];}
 if(self.screen==2)return [self cell:@[@"Original look",@"Warmer",@"Cooler",@"Deeper colour",@"Softer",@"Darker",@"Save variation",@"Start from current look"][n]detail:nil];
 if(self.screen==3)return [self cell:@[self.sceneA?[@"From: "stringByAppendingString:self.sceneA[@"name"]]:@"Choose first scene",self.sceneB?[@"To: "stringByAppendingString:self.sceneB[@"name"]]:@"Choose second scene",[NSString stringWithFormat:@"Duration: %.1f minutes",self.duration/60.],[NSString stringWithFormat:@"Spatial stagger: %.0f%%",self.stagger*100],@"Play dissolve"][n]detail:nil];
 if(self.screen==4){if(p.section==0)return [self cell:@[self.journeyName,self.loop?@"Loop: On":@"Loop: Off",@"Add chapter",@"Choose from saved journeys"][n]detail:nil];if(p.section==1){NSDictionary *step=self.steps[n];return [self cell:step[@"name"]detail:[NSString stringWithFormat:@"Hold %.1f min · dissolve %.1f min",[step[@"hold"]doubleValue]/60.,[step[@"fade"]doubleValue]/60.]];}return [self cell:n==0?@"Save journey":@"Play journey"detail:nil];}
 if(self.screen==5)return [self cell:@[@"5 minutes",@"15 minutes",@"30 minutes",@"60 minutes",@"120 minutes",@"Cancel sleep descent"][n]detail:e.sleepActive?[NSString stringWithFormat:@"%.0f minutes remaining",MAX(0,e.sleepDuration-e.sleepElapsed)/60.]:nil];
 if(self.screen==6)return [self cell:self.choices[n][@"name"]detail:nil];
 if(self.screen==7)return [self cell:n==0?@"New journey":HPJourneysSaved()[n-1][@"name"]detail:nil];
 if(self.screen==8){NSDictionary *step=self.steps[self.selectedStep];return [self cell:n==0?step[@"name"]:n==1?[NSString stringWithFormat:@"Hold %.1f minutes",[step[@"hold"]doubleValue]/60.]:[NSString stringWithFormat:@"Dissolve %.1f minutes",[step[@"fade"]doubleValue]/60.]detail:nil];}return [self cell:@""detail:nil];
}
- (void)angle:(UISlider *)s{HPLightEngine.shared.waveAngle=s.value;[NSUserDefaults.standardUserDefaults setDouble:s.value forKey:@"HP.WaveAngle"];UITableViewCell *cell=[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];cell.textLabel.text=[NSString stringWithFormat:@"Angle %.0f°",s.value];}
- (void)number:(NSString *)title value:(double)value minimum:(double)minimum maximum:(double)maximum completion:(void(^)(double))completion{
 UIAlertController *prompt=[UIAlertController alertControllerWithTitle:title message:[NSString stringWithFormat:@"Enter %.2g to %.2g.",minimum,maximum]preferredStyle:UIAlertControllerStyleAlert];[prompt addTextFieldWithConfigurationHandler:^(UITextField *f){f.text=[NSString stringWithFormat:@"%.2g",value];f.keyboardType=UIKeyboardTypeDecimalPad;}];[prompt addAction:[UIAlertAction actionWithTitle:@"Cancel"style:UIAlertActionStyleCancel handler:nil]];[prompt addAction:[UIAlertAction actionWithTitle:@"Apply"style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){NSString *text=prompt.textFields.firstObject.text;NSScanner *scanner=[NSScanner scannerWithString:text];double n=0;if(![scanner scanDouble:&n]||!scanner.isAtEnd||!isfinite(n)||n<minimum||n>maximum){HPAlert(self,@"Value out of range",[NSString stringWithFormat:@"Use %.2g to %.2g.",minimum,maximum]);return;}completion(n);[self.tableView reloadData];}]];[self presentViewController:prompt animated:YES completion:nil];
}
- (void)name:(NSString *)title current:(NSString *)current completion:(void(^)(NSString *))completion{
 UIAlertController *prompt=[UIAlertController alertControllerWithTitle:title message:nil preferredStyle:UIAlertControllerStyleAlert];[prompt addTextFieldWithConfigurationHandler:^(UITextField *f){f.text=current;f.keyboardAppearance=UIKeyboardAppearanceDark;}];[prompt addAction:[UIAlertAction actionWithTitle:@"Cancel"style:UIAlertActionStyleCancel handler:nil]];[prompt addAction:[UIAlertAction actionWithTitle:@"Save"style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){NSString *name=[prompt.textFields.firstObject.text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];if(!name.length||name.length>40){HPAlert(self,@"Choose a name",@"Use 1 to 40 characters.");return;}completion(name);[self.tableView reloadData];}]];[self presentViewController:prompt animated:YES completion:nil];
}
- (void)saveVariation{[self name:@"Save variation"current:[HPLightEngine.shared moodName]completion:^(NSString *name){NSError *error=nil;NSDictionary *r=[HPStudioPresets validatedRecipe:[HPLightEngine.shared recipeNamed:name]error:&error];if(!r){HPAlert(self,@"Could not save",error.localizedDescription);return;}NSMutableArray *saved=[HPSavedMoods()mutableCopy];[saved addObject:r];HPStoreMoods(saved);HPAlert(self,@"Variation saved",@"Find it in Studio → My moods.");}];}
- (void)saveJourney{if(!self.steps.count){HPAlert(self,@"Add a chapter",@"Choose at least one mood or scene.");return;}NSMutableArray *saved=[HPJourneysSaved()mutableCopy];NSString *identifier=self.journeyID?:NSUUID.UUID.UUIDString;NSDictionary *j=@{@"id":identifier,@"name":self.journeyName,@"steps":[self.steps copy],@"loop":@(self.loop)};NSUInteger i=[saved indexOfObjectPassingTest:^BOOL(NSDictionary *a,NSUInteger i,BOOL *stop){return [a[@"id"]isEqual:identifier];}];if(i==NSNotFound)[saved addObject:j];else saved[i]=j;self.journeyID=identifier;HPStoreJourneys(saved);HPAlert(self,@"Journey saved",self.journeyName);}
- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)p{
 [table deselectRowAtIndexPath:p animated:YES];NSInteger n=p.row;HPLightEngine *e=HPLightEngine.shared;if(HPIdentifying)return;
 if(self.screen==0){switch(n){case 0:[e start:32];[self.navigationController popViewControllerAnimated:YES];break;case 1:[self.navigationController pushViewController:[HPTouchStudio new]animated:YES];break;case 2:[self open:3];break;case 3:[self open:7];break;case 4:if(e.creativeKind!=4)[e beginExplore:0];[self open:2];break;case 5:if(e.creativeKind==1||e.creativeKind==4)[e beginExplore:arc4random_uniform(5)+1];else[e shuffleColours];break;case 6:[self saveVariation];break;case 7:[self open:5];break;case 8:[self open:1];break;}return;}
 if(self.screen==1){if(n==1){e.waveReverse=!e.waveReverse;[NSUserDefaults.standardUserDefaults setBool:e.waveReverse forKey:@"HP.WaveReverse"];}if(n==2){e.rippleInward=!e.rippleInward;[NSUserDefaults.standardUserDefaults setBool:e.rippleInward forKey:@"HP.RippleInward"];}if(n==3){e.waveAngle=0;e.waveReverse=NO;[NSUserDefaults.standardUserDefaults setDouble:0 forKey:@"HP.WaveAngle"];[NSUserDefaults.standardUserDefaults setBool:NO forKey:@"HP.WaveReverse"];}if(n==4)[self.navigationController pushViewController:[HPTouchStudio new]animated:YES];[table reloadData];return;}
 if(self.screen==2){if(n<6)[e beginExplore:n];if(n==6)[self saveVariation];if(n==7){NSDictionary *look=[e snapshotLook];e.anchorLook=look;e.creativeKind=4;e.exploreVariant=0;}return;}
 if(self.screen==3){if(n<2)[self pick:[self sceneChoices]title:@"Choose scene"completion:^(NSDictionary *scene){if(n==0)self.sceneA=scene;else self.sceneB=scene;[self.tableView reloadData];}];if(n==2)[self number:@"Dissolve minutes"value:self.duration/60. minimum:5./60. maximum:120 completion:^(double value){self.duration=value*60.;}];if(n==3)[self number:@"Stagger percent"value:self.stagger*100 minimum:0 maximum:65 completion:^(double value){self.stagger=value/100.;}];if(n==4){if(!self.sceneA||!self.sceneB||![e beginMorph:self.sceneA[@"look"]to:self.sceneB[@"look"]duration:self.duration stagger:self.stagger])HPAlert(self,@"Choose two scenes",@"Both need at least one compatible light.");else[self.navigationController popToRootViewControllerAnimated:YES];}return;}
 if(self.screen==4){if(p.section==0){if(n==0)[self name:@"Journey name"current:self.journeyName completion:^(NSString *name){self.journeyName=name;}];if(n==1){self.loop=!self.loop;[table reloadData];}if(n==2)[self pick:[self chapterChoices]title:@"Add mood or scene"completion:^(NSDictionary *choice){NSMutableDictionary *step=[choice mutableCopy];step[@"hold"]=@240;step[@"fade"]=@120;[self.steps addObject:step];[self.tableView reloadData];}];if(n==3)[self open:7];}
 else if(p.section==1){HPStudioTools *next=[HPStudioTools new];next.screen=8;next.steps=self.steps;next.selectedStep=n;[self.navigationController pushViewController:next animated:YES];}
 else if(n==0)[self saveJourney];else if([e beginSequence:self.steps name:self.journeyName loop:self.loop])[self.navigationController popToRootViewControllerAnimated:YES];else HPAlert(self,@"Journey needs chapters",@"Add at least one valid mood or scene.");return;}
 if(self.screen==5){if(n==5)[e cancelSleep];else{[e beginSleep:[@[@300,@900,@1800,@3600,@7200][n]doubleValue]];[self.navigationController popToRootViewControllerAnimated:YES];}return;}
 if(self.screen==6){if(self.picked)self.picked(self.choices[n]);[self.navigationController popViewControllerAnimated:YES];return;}
 if(self.screen==7){HPStudioTools *next=[HPStudioTools new];next.screen=4;if(n>0){NSDictionary *j=HPJourneysSaved()[n-1];next.journeyName=j[@"name"];next.journeyID=j[@"id"];next.steps=[j[@"steps"]mutableCopy];next.loop=[j[@"loop"]boolValue];}[self.navigationController pushViewController:next animated:YES];return;}
 if(self.screen==8&&n>0){NSString *key=n==1?@"hold":@"fade";[self number:n==1?@"Hold minutes":@"Dissolve minutes"value:[self.steps[self.selectedStep][key]doubleValue]/60. minimum:5./60. maximum:n==1?120:60 completion:^(double value){NSMutableDictionary *step=[self.steps[self.selectedStep]mutableCopy];step[key]=@(value*60.);self.steps[self.selectedStep]=step;}];}
}
- (BOOL)tableView:(UITableView *)table canMoveRowAtIndexPath:(NSIndexPath *)p{return self.screen==4&&p.section==1;}
- (NSIndexPath *)tableView:(UITableView *)table targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)from toProposedIndexPath:(NSIndexPath *)to{return to.section==1?to:from;}
- (void)tableView:(UITableView *)table moveRowAtIndexPath:(NSIndexPath *)from toIndexPath:(NSIndexPath *)to{NSDictionary *step=self.steps[from.row];[self.steps removeObjectAtIndex:from.row];[self.steps insertObject:step atIndex:to.row];}
- (UITableViewCellEditingStyle)tableView:(UITableView *)table editingStyleForRowAtIndexPath:(NSIndexPath *)p{return (self.screen==4&&p.section==1)||(self.screen==7&&p.row>0)?UITableViewCellEditingStyleDelete:UITableViewCellEditingStyleNone;}
- (void)tableView:(UITableView *)table commitEditingStyle:(UITableViewCellEditingStyle)style forRowAtIndexPath:(NSIndexPath *)p{if(self.screen==4)[self.steps removeObjectAtIndex:p.row];else if(self.screen==7){NSMutableArray *a=[HPJourneysSaved()mutableCopy];[a removeObjectAtIndex:p.row-1];HPStoreJourneys(a);}[table reloadData];}
@end
static UIViewController *HPCreateStudioTools(void){return [HPStudioTools new];}
