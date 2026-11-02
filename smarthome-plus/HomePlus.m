#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <dlfcn.h>
#import "SceneTransfer.h"

// All runtime calls below use selectors and ABIs inspected in the supplied 3.0.2 app.
// The scene-count branch is patched in the executable by make_homeplus.py.
static NSString *HPBootstrapError;
static NSDictionary *HPSeed;
static id HPGet(id object, NSString *selector) {
    SEL sel = NSSelectorFromString(selector);
    return [object respondsToSelector:sel] ? ((id(*)(id,SEL))objc_msgSend)(object,sel) : nil;
}
static id HPCall(id object, NSString *selector, id value) {
    SEL sel = NSSelectorFromString(selector);
    if (![object respondsToSelector:sel]) @throw [NSException exceptionWithName:@"HPUnavailable" reason:[@"Unavailable method: " stringByAppendingString:selector] userInfo:nil];
    return ((id(*)(id,SEL,id))objc_msgSend)(object,sel,value);
}
static id HPField(id object, NSString *key) {
    // Swift List properties have no Objective-C getter. Realm's own dynamic
    // accessor returns the underlying RLMArray, rather than a Swift List box.
    static id (*dynamicGet)(id,NSString *);static dispatch_once_t once;
    dispatch_once(&once,^{dynamicGet=dlsym(RTLD_DEFAULT,"RLMObjectBaseObjectForKeyedSubscript");});
    if(dynamicGet) return dynamicGet(object,key);
    if ([object respondsToSelector:NSSelectorFromString(@"objectForKeyedSubscript:")])
        return HPCall(object,@"objectForKeyedSubscript:",key);
    return [object valueForKey:key];
}
static NSString *HPDocuments(void) {
    return NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject;
}
static NSString *HPMeshUUID(void) {
    NSData *data = [NSUserDefaults.standardUserDefaults dataForKey:@"mesh.json"];
    NSDictionary *mesh = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    return mesh[@"meshUUID"];
}
static void HPBootstrap(void) {
    @try {
        HPSeed = [NSDictionary dictionaryWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"HomePlusSeed" withExtension:@"plist"]];
        if (!HPSeed) { HPBootstrapError = @"The saved home is missing from this installation."; return; }
        NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
        if ([defaults boolForKey:@"HP.Seeded"]) return;
        NSFileManager *fm = NSFileManager.defaultManager;
        // This is a separate app container. Never enumerate, open or modify the original app container.
        for (NSString *relative in HPSeed[@"Documents"]) {
            if ([relative hasPrefix:@"/"] || [[relative pathComponents] containsObject:@".."])
                @throw [NSException exceptionWithName:@"HPSeed" reason:@"Invalid saved-home path." userInfo:nil];
            NSString *path = [HPDocuments() stringByAppendingPathComponent:relative];
            if ([fm fileExistsAtPath:path]) continue;
            NSError *error;
            if (![fm createDirectoryAtPath:path.stringByDeletingLastPathComponent withIntermediateDirectories:YES attributes:nil error:&error] ||
                ![HPSeed[@"Documents"][relative] writeToFile:path options:NSDataWritingAtomic error:&error])
                @throw [NSException exceptionWithName:@"HPSeed" reason:error.localizedDescription userInfo:nil];
        }
        NSDictionary *prefs = HPSeed[@"Preferences"];
        if (![prefs[@"mesh.json"] isKindOfClass:NSData.class])
            @throw [NSException exceptionWithName:@"HPSeed" reason:@"Saved light network is unavailable." userInfo:nil];
        for (NSString *key in prefs) [defaults setObject:prefs[key] forKey:key];
        [defaults setBool:YES forKey:@"HP.Seeded"];
        [defaults synchronize];
    } @catch (NSException *exception) { HPBootstrapError = exception.reason; }
}

static id HPRealm(void) {
    id realm = HPGet(NSClassFromString(@"RLMRealm"),@"defaultRealm");
    if (!realm) @throw [NSException exceptionWithName:@"HPRealm" reason:@"The scene database is not ready." userInfo:nil];
    return realm;
}
static NSArray *HPExportRoutines(void) {
    id results = HPCall(HPRealm(),@"allObjects:",@"RoutinesInfo");
    NSMutableArray *scenes = [NSMutableArray new];
    for (id scene in results) {
        NSMutableArray *commands = [NSMutableArray new];
        for (id device in HPField(scene,@"routineList")) {
            NSMutableDictionary *entry = [NSMutableDictionary new];
            for (NSString *key in @[@"uuid",@"deviceName",@"color",@"saturation",@"lightness",@"onOff",@"cct",@"isGroup",@"deviceType"])
                entry[key] = HPField(device,key) ?: @0;
            [commands addObject:entry];
        }
        [scenes addObject:@{@"uuid":HPField(scene,@"uuid"),@"routineName":HPField(scene,@"routineName"),
                            @"color":HPField(scene,@"color"),@"routineList":commands}];
    }
    return scenes;
}

@interface HPPanel : UITableViewController <UIDocumentPickerDelegate>
@property(nonatomic,strong) NSArray *scenes;
@property(nonatomic,strong) NSString *detail;
@end

@interface HPManager : NSObject
@property(nonatomic,strong) UIButton *button;
+ (instancetype)shared;
- (void)install;
- (void)show;
@end

static UIViewController *HPTop(void) {
    UIWindow *window = UIApplication.sharedApplication.keyWindow;
    UIViewController *controller = window.rootViewController;
    while (controller) {
        if (controller.presentedViewController) controller = controller.presentedViewController;
        else if ([controller isKindOfClass:UINavigationController.class]) controller = [(UINavigationController *)controller topViewController];
        else if ([controller isKindOfClass:UITabBarController.class]) controller = [(UITabBarController *)controller selectedViewController];
        else break;
    }
    return controller;
}
static void HPAlert(UIViewController *controller, NSString *title, NSString *message) {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [controller presentViewController:alert animated:YES completion:nil];
}

@implementation HPManager
+ (instancetype)shared { static HPManager *value; static dispatch_once_t once; dispatch_once(&once,^{value=[HPManager new];}); return value; }
- (void)install {
    UIWindow *window = UIApplication.sharedApplication.keyWindow;
    if (!window) return;
    if (!self.button) {
        self.button = [UIButton buttonWithType:UIButtonTypeSystem];
        [self.button setTitle:@"PLUS" forState:UIControlStateNormal];
        self.button.titleLabel.font = [UIFont systemFontOfSize:11 weight:UIFontWeightBold];
        [self.button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
        self.button.backgroundColor = [UIColor colorWithRed:0.12 green:0.14 blue:0.19 alpha:0.94];
        self.button.layer.cornerRadius = 20;
        self.button.accessibilityLabel = @"SYLVANIA Plus, scene sharing and backups";
        [self.button addTarget:self action:@selector(show) forControlEvents:UIControlEventTouchUpInside];
    }
    if (self.button.superview != window) [window addSubview:self.button];
    self.button.frame = CGRectMake(window.bounds.size.width-78,window.bounds.size.height-window.safeAreaInsets.bottom-104,64,40);
    self.button.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleTopMargin;
    [window bringSubviewToFront:self.button];
}
- (void)show {
    if ([HPTop() isKindOfClass:HPPanel.class]) return;
    HPPanel *panel = [[HPPanel alloc]initWithStyle:UITableViewStyleInsetGrouped];
    UINavigationController *navigation = [[UINavigationController alloc]initWithRootViewController:panel];
    [HPTop() presentViewController:navigation animated:YES completion:nil];
}
@end

@implementation HPPanel
- (void)viewDidLoad {
    [super viewDidLoad]; self.title = @"SYLVANIA Plus";
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc]initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(done)];
    [self refresh];
}
- (void)done { [self dismissViewControllerAnimated:YES completion:nil]; }
- (void)refresh {
    @try { self.scenes=HPExportRoutines(); self.detail=HPBootstrapError; }
    @catch (NSException *e) { self.scenes=@[]; self.detail=e.reason; }
    [self.tableView reloadData];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 3; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section==0?3:(section==1?3:1); }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return @[@"Your home",@"Keep your home portable",@"Direct local control"][section];
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section==0) return self.detail ?: @"The original scene editor and controls are retained. New scenes have no ten-routine cap.";
    if (section==1) return @"Scene files contain light settings, without pairing keys. A full home backup includes private pairing data. Imports merge scenes by their identifier.";
    return @"Install Device 1, Device 2 and Device 3 on different devices. Each has its own mesh address and sequence counter. Bluetooth control works without another phone running. Later edits stay local until you share them.";
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc]initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:nil];
    if(path.section==0) {
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
        cell.textLabel.text=@[@"Scene limit",@"Saved scenes",@"Controller"][path.row];
        cell.detailTextLabel.text=path.row==0?@"Removed":(path.row==1?[NSString stringWithFormat:@"%lu",(unsigned long)self.scenes.count]:[NSUserDefaults.standardUserDefaults stringForKey:@"HP.Profile"]?:HPSeed[@"Profile"]);
    } else if(path.section==1) {
        cell.textLabel.text=@[@"Share scenes",@"Import scenes",@"Export full home backup"][path.row];
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
    } else {
        cell.textLabel.text=@"Existing lights and groups";cell.detailTextLabel.text=HPSeed[@"Summary"];
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
    }
    return cell;
}
- (void)shareFile:(NSURL *)file {
    UIActivityViewController *sheet = [[UIActivityViewController alloc]initWithActivityItems:@[file] applicationActivities:nil];
    sheet.popoverPresentationController.sourceView=self.view;
    sheet.popoverPresentationController.sourceRect=CGRectMake(self.view.bounds.size.width/2,self.view.bounds.size.height/2,1,1);
    [self presentViewController:sheet animated:YES completion:nil];
}
- (NSURL *)exportLocation:(NSString *)name {
    NSString *directory = [NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
    [NSFileManager.defaultManager createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:nil];
    return [NSURL fileURLWithPath:[directory stringByAppendingPathComponent:name]];
}
- (void)exportScenes {
    @try {
        NSDictionary *document=@{@"format":@"sylvania-plus-scenes",@"version":@1,@"meshUUID":HPMeshUUID(),@"routines":HPExportRoutines()};
        NSError *error;
        NSDictionary *valid=[HPSceneTransfer validatedDocument:document meshUUID:HPMeshUUID() error:&error];
        if (!valid) {HPAlert(self,@"Could not export scenes",error.localizedDescription);return;}
        NSData *data=[NSJSONSerialization dataWithJSONObject:valid options:NSJSONWritingPrettyPrinted error:&error];
        NSURL *file=[self exportLocation:@"My_Home.sylscenes"];
        if (![data writeToURL:file options:NSDataWritingAtomic error:&error]) {HPAlert(self,@"Could not save scenes",error.localizedDescription);return;}
        [self shareFile:file];
    } @catch(NSException *e) {HPAlert(self,@"Could not export scenes",e.reason);}
}
- (void)exportBackup {
    @try {
        id realm=HPRealm();NSURL *realmURL=[self exportLocation:@"default.realm"];
        NSError *error=nil;SEL selector=NSSelectorFromString(@"writeCopyToURL:encryptionKey:error:");
        BOOL saved=[realm respondsToSelector:selector] && ((BOOL(*)(id,SEL,id,id,NSError **))objc_msgSend)(realm,selector,realmURL,nil,&error);
        if (!saved) {HPAlert(self,@"Could not create backup",error.localizedDescription?:@"The scene database could not be copied.");return;}
        NSMutableDictionary *files=[NSMutableDictionary new];
        files[@"default.realm"]=[NSData dataWithContentsOfURL:realmURL];
        NSString *documents=HPDocuments();
        for(NSString *relative in @[@"Network",@"UserModel.sqlite",@"AppStatusAssistant.archiver",@"networkCompareList.archiver"]) {
            NSString *path=[documents stringByAppendingPathComponent:relative];BOOL directory=NO;
            if (![NSFileManager.defaultManager fileExistsAtPath:path isDirectory:&directory]) continue;
            if (directory) {
                for(NSString *name in [NSFileManager.defaultManager contentsOfDirectoryAtPath:path error:nil]) {
                    NSData *data=[NSData dataWithContentsOfFile:[path stringByAppendingPathComponent:name]];
                    if (data) files[[relative stringByAppendingPathComponent:name]]=data;
                }
            } else {
                NSData *data=[NSData dataWithContentsOfFile:path];if(data)files[relative]=data;
            }
        }
        NSDictionary *preferences=[NSUserDefaults.standardUserDefaults persistentDomainForName:NSBundle.mainBundle.bundleIdentifier]?:@{};
        NSDictionary *backup=@{@"format":@"sylvania-plus-backup",@"version":@1,@"Profile":HPSeed[@"Profile"],@"Preferences":preferences,@"Documents":files};
        NSData *data=[NSPropertyListSerialization dataWithPropertyList:backup format:NSPropertyListBinaryFormat_v1_0 options:0 error:&error];
        NSURL *file=[self exportLocation:@"My_Private_Home.sylbackup"];
        if(![data writeToURL:file options:NSDataWritingAtomic error:&error]){HPAlert(self,@"Could not save backup",error.localizedDescription);return;}
        [self shareFile:file];
    } @catch(NSException *e){HPAlert(self,@"Could not create backup",e.reason);}
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];if(path.section!=1)return;
    if(path.row==0)[self exportScenes];
    else if(path.row==2)[self exportBackup];
    else {
        UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc]initWithDocumentTypes:@[@"com.ledvance.smart.plus.scenes",@"public.json"] inMode:UIDocumentPickerModeImport];
        picker.delegate=self;[self presentViewController:picker animated:YES completion:nil];
    }
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url=urls.firstObject;if(!url)return;
    NSNumber *size;[url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];
    // Defer alerts until the system picker has actually finished dismissing.
    if(size.unsignedLongLongValue>5*1024*1024){
        [controller dismissViewControllerAnimated:YES completion:^{HPAlert(self,@"Could not import",@"This scene file is too large.");}];return;
    }
    NSError *error;NSData *data=[NSData dataWithContentsOfURL:url options:0 error:&error];
    id raw=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:&error]:nil;
    NSDictionary *document=[HPSceneTransfer validatedDocument:raw meshUUID:HPMeshUUID() error:&error];
    if(!document){
        NSString *message=error.localizedDescription;
        [controller dismissViewControllerAnimated:YES completion:^{HPAlert(self,@"Could not import",message);}];return;
    }
    UIAlertController *confirmation=[UIAlertController alertControllerWithTitle:@"Import scenes?" message:[NSString stringWithFormat:@"Import %lu scenes into this device. Matching scene IDs will be replaced. Other scenes and light pairings stay as they are.",(unsigned long)[document[@"routines"] count]] preferredStyle:UIAlertControllerStyleAlert];
    [confirmation addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
    [confirmation addAction:[UIAlertAction actionWithTitle:@"Import" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){[self importScenes:document[@"routines"]];}]];
    if(self.presentedViewController==controller)
        [controller dismissViewControllerAnimated:YES completion:^{[self presentViewController:confirmation animated:YES completion:nil];}];
    else dispatch_async(dispatch_get_main_queue(),^{[self presentViewController:confirmation animated:YES completion:nil];});
}
- (void)importScenes:(NSArray *)scenes {
    id realm=nil;
    @try {
        realm=HPRealm();HPGet(realm,@"beginWriteTransaction");
        for(NSDictionary *scene in scenes) {
            SEL query=NSSelectorFromString(@"objects:withPredicate:");
            id matches=((id(*)(id,SEL,id,id))objc_msgSend)(realm,query,@"RoutinesInfo",[NSPredicate predicateWithFormat:@"uuid == %@",scene[@"uuid"]]);
            NSMutableArray *old=[NSMutableArray new];for(id object in matches)[old addObject:object];
            for(id object in old) {
                id children=HPField(object,@"routineList");HPCall(realm,@"deleteObjects:",children);HPCall(realm,@"deleteObject:",object);
            }
            SEL create=NSSelectorFromString(@"createObject:withValue:");
            ((id(*)(id,SEL,id,id))objc_msgSend)(realm,create,@"RoutinesInfo",scene);
        }
        NSError *error=nil;SEL commit=NSSelectorFromString(@"commitWriteTransaction:");
        BOOL ok=((BOOL(*)(id,SEL,NSError **))objc_msgSend)(realm,commit,&error);
        if(!ok){HPAlert(self,@"Could not import",error.localizedDescription);return;}
        [self refresh];HPAlert(self,@"Scenes imported",@"Return to Scenes to use them. No lights were paired or reset.");
    } @catch(NSException *e) {
        // Roll the entire import back if any field or database operation fails.
        if(realm && [[realm valueForKey:@"inWriteTransaction"] boolValue])HPGet(realm,@"cancelWriteTransaction");
        HPAlert(self,@"Could not import",e.reason);
    }
}
@end

static IMP HPOriginalDidAppear;
static void HPDidAppear(id self,SEL selector,BOOL animated) {
    ((void(*)(id,SEL,BOOL))HPOriginalDidAppear)(self,selector,animated);
    [[HPManager shared]install];
}
__attribute__((constructor)) static void HPInitialize(void) {
    @autoreleasepool {
        HPBootstrap();
        Method method=class_getInstanceMethod(UIViewController.class,@selector(viewDidAppear:));
        if(method)HPOriginalDidAppear=method_setImplementation(method,(IMP)HPDidAppear);
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note){[[HPManager shared]install];}];
    }
}
