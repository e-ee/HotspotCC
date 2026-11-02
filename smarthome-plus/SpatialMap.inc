@interface HPRoomCanvas:UIView
@property(nonatomic,strong) NSArray *lights;
@property(nonatomic,strong) NSMutableArray *pins;
@property(nonatomic,weak) UIButton *draggingPin;
@property(nonatomic) CGPoint dragStart;
@property(nonatomic,copy) void(^picked)(NSDictionary *);
@property(nonatomic,copy) void(^moved)(NSDictionary *,CGPoint);
- (void)moveLightAtIndex:(NSUInteger)index to:(CGPoint)normalized;
@end
@implementation HPRoomCanvas
@synthesize lights=_lights;
- (CGRect)plot{return CGRectInset(self.bounds,24,24);}
- (void)setLights:(NSArray *)lights{
    _lights=lights;for(UIView *v in self.pins)[v removeFromSuperview];self.pins=[NSMutableArray new];
    for(NSUInteger i=0;i<lights.count;i++){
        UIButton *pin=[UIButton buttonWithType:UIButtonTypeCustom];pin.tag=i;pin.bounds=CGRectMake(0,0,36,36);pin.layer.cornerRadius=18;pin.layer.borderWidth=1;pin.layer.borderColor=[UIColor colorWithWhite:1 alpha:.38].CGColor;
        pin.backgroundColor=[UIColor colorWithHue:.57+i*.019 saturation:.38 brightness:.30 alpha:1];[pin setTitle:[NSString stringWithFormat:@"%lu",(unsigned long)i+1]forState:UIControlStateNormal];[pin setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];pin.titleLabel.font=[UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
        pin.accessibilityLabel=[NSString stringWithFormat:@"%lu, %@",(unsigned long)i+1,lights[i][@"name"]];pin.accessibilityHint=@"Tap to identify this light. Drag to change its apartment position.";
        [pin addTarget:self action:@selector(pick:)forControlEvents:UIControlEventTouchUpInside];UIPanGestureRecognizer *pan=[[UIPanGestureRecognizer alloc]initWithTarget:self action:@selector(drag:)];[pin addGestureRecognizer:pan];[self addSubview:pin];[self.pins addObject:pin];
    }[self setNeedsLayout];
}
- (void)layoutSubviews{[super layoutSubviews];CGRect plot=[self plot];for(NSUInteger i=0;i<self.lights.count;i++){UIButton *pin=self.pins[i];if(pin==self.draggingPin)continue;NSDictionary *light=self.lights[i];pin.center=CGPointMake(plot.origin.x+[light[@"x"]doubleValue]*plot.size.width,plot.origin.y+[light[@"y"]doubleValue]*plot.size.height);}}
- (void)drawRect:(CGRect)rect{
    CGContextRef c=UIGraphicsGetCurrentContext();CGRect plot=[self plot];CGContextSetStrokeColorWithColor(c,[UIColor colorWithWhite:1 alpha:.075].CGColor);CGContextSetLineWidth(c,.5);
    for(int i=0;i<=8;i++){CGFloat x=plot.origin.x+plot.size.width*i/8.,y=plot.origin.y+plot.size.height*i/8.;CGContextMoveToPoint(c,x,plot.origin.y);CGContextAddLineToPoint(c,x,CGRectGetMaxY(plot));CGContextMoveToPoint(c,plot.origin.x,y);CGContextAddLineToPoint(c,CGRectGetMaxX(plot),y);}CGContextStrokePath(c);
}
- (void)pick:(UIButton *)pin{for(UIButton *p in self.pins)p.layer.borderWidth=p==pin?2:1;if(self.picked)self.picked(self.lights[pin.tag]);}
- (void)moveLightAtIndex:(NSUInteger)index to:(CGPoint)point{
    if(index>=self.lights.count)return;point.x=HPClamp(point.x,0,1);point.y=HPClamp(point.y,0,1);
    NSMutableArray *lights=[self.lights mutableCopy];NSMutableDictionary *record=[lights[index]mutableCopy];record[@"x"]=@(point.x);record[@"y"]=@(point.y);lights[index]=record;_lights=lights;
    if(self.moved)self.moved(record,point);[self setNeedsLayout];
}
- (void)drag:(UIPanGestureRecognizer *)pan{
    UIButton *pin=(id)pan.view;if(pan.state==UIGestureRecognizerStateBegan){self.draggingPin=pin;self.dragStart=pin.center;}
    CGPoint delta=[pan translationInView:self];CGRect plot=[self plot];CGPoint center=CGPointMake(HPClamp(self.dragStart.x+delta.x,plot.origin.x,CGRectGetMaxX(plot)),HPClamp(self.dragStart.y+delta.y,plot.origin.y,CGRectGetMaxY(plot)));pin.center=center;
    if(pan.state==UIGestureRecognizerStateEnded||pan.state==UIGestureRecognizerStateCancelled){self.draggingPin=nil;[self moveLightAtIndex:pin.tag to:CGPointMake((center.x-plot.origin.x)/MAX(1,plot.size.width),(center.y-plot.origin.y)/MAX(1,plot.size.height))];}
}
@end
@interface HPSpatialEditor:UIViewController
@property(nonatomic,strong) HPRoomCanvas *canvas;
@property(nonatomic,strong) UILabel *status;
@end
@implementation HPSpatialEditor
- (UILabel *)label:(NSString *)text size:(CGFloat)size{
    UILabel *label=[UILabel new];label.text=text;label.textColor=UIColor.whiteColor;label.numberOfLines=0;label.font=[UIFont systemFontOfSize:size weight:UIFontWeightMedium];return label;
}
- (void)viewDidLoad{
    [super viewDidLoad];[HPLightEngine.shared stop];self.title=@"Apartment map";self.view.backgroundColor=[UIColor colorWithRed:.035 green:.045 blue:.065 alpha:1];
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc]initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(done)];
    UIScrollView *scroll=[UIScrollView new];scroll.translatesAutoresizingMaskIntoConstraints=NO;[self.view addSubview:scroll];UIStackView *stack=[UIStackView new];stack.axis=UILayoutConstraintAxisVertical;stack.spacing=14;stack.translatesAutoresizingMaskIntoConstraints=NO;[scroll addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[[scroll.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],[scroll.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],[scroll.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],[scroll.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],[stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:18],[stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-24],[stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:20],[stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-20],[stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-40]]];
    [stack addArrangedSubview:[self label:@"Give the light a place." size:26]];
    UILabel *hint=[self label:@"Drag each numbered light to its place around your apartment. Tap it to identify it with one short pulse, then restore its brightness." size:13];hint.textColor=[UIColor colorWithWhite:.67 alpha:1];[stack addArrangedSubview:hint];
    self.status=[self label:@"Studio is paused while you map. Positions save automatically." size:13];[stack addArrangedSubview:self.status];
    self.canvas=[HPRoomCanvas new];self.canvas.backgroundColor=[UIColor colorWithRed:.065 green:.08 blue:.11 alpha:1];self.canvas.layer.cornerRadius=22;self.canvas.clipsToBounds=YES;self.canvas.lights=HPLightRecords();[self.canvas.heightAnchor constraintEqualToConstant:MIN(620,MAX(350,UIScreen.mainScreen.bounds.size.height*.5))].active=YES;[stack addArrangedSubview:self.canvas];
    __weak HPSpatialEditor *weakSelf=self;
    self.canvas.moved=^(NSDictionary *light,CGPoint point){
        NSString *mesh=HPMeshUUID()?:@"current";NSMutableDictionary *layouts=[[NSUserDefaults.standardUserDefaults dictionaryForKey:@"HP.LightLayout"]mutableCopy]?:[NSMutableDictionary new];layouts[mesh]=[HPSpatialLayout layout:layouts[mesh]moving:light[@"id"]x:point.x y:point.y];[NSUserDefaults.standardUserDefaults setObject:layouts forKey:@"HP.LightLayout"];[NSUserDefaults.standardUserDefaults synchronize];weakSelf.status.text=[NSString stringWithFormat:@"Placed %@",light[@"name"]];[NSNotificationCenter.defaultCenter postNotificationName:@"HP.LayoutChanged" object:nil];
    };
    self.canvas.picked=^(NSDictionary *light){
        HPSpatialEditor *editor=weakSelf;if(!editor)return;[HPLightEngine.shared stop];editor.status.text=[NSString stringWithFormat:@"Identifying %@…",light[@"name"]];
        HPIdentifyLight(light,^(NSError *error){weakSelf.status.text=error.localizedDescription?:[NSString stringWithFormat:@"%@ · brightness restored",light[@"name"]];});
    };
    [stack addArrangedSubview:[self label:@"Your lights" size:16]];
    for(NSUInteger i=0;i<self.canvas.lights.count;i++){NSDictionary *light=self.canvas.lights[i];UILabel *name=[self label:[NSString stringWithFormat:@"%lu   %@",(unsigned long)i+1,light[@"name"]]size:13];name.textColor=[UIColor colorWithWhite:.7 alpha:1];[stack addArrangedSubview:name];}
    UILabel *note=[self label:@"The starting grid is a guide. Your placements drive the direction and distance of spatial effects. Returning to Studio keeps the lights paused until you choose a mood." size:12];note.textColor=[UIColor colorWithWhite:.55 alpha:1];[stack addArrangedSubview:note];
}
- (void)done{if(self.navigationController.viewControllers.count>1)[self.navigationController popViewControllerAnimated:YES];else[self dismissViewControllerAnimated:YES completion:nil];}
@end
