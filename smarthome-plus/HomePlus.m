#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <dlfcn.h>
#import "SceneTransfer.h"
#import "HomeArchive.h"
#import "SceneCapture.h"
#import "SpatialLayout.h"
#import <sqlite3.h>

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
static void HPVoid(id object,NSString *selector){SEL sel=NSSelectorFromString(selector);if([object respondsToSelector:sel])((void(*)(id,SEL))objc_msgSend)(object,sel);}
static void HPSet(id object,NSString *selector,id value){SEL sel=NSSelectorFromString(selector);if([object respondsToSelector:sel])((void(*)(id,SEL,id))objc_msgSend)(object,sel,value);}
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
static NSString *HPStatePath(void){return [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject stringByAppendingPathComponent:@"HomePlus"];}
static void HPBootstrap(void) {
    @try {
        HPSeed = [NSDictionary dictionaryWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"HomePlusSeed" withExtension:@"plist"]];
        if (!HPSeed) { HPBootstrapError = @"The saved home is missing from this installation."; return; }
        NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
        if (![defaults boolForKey:@"HP.Seeded"]) {
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
        }
        NSError *restoreError=nil;
        // Restore before the original app opens Realm or the mesh transport.
        if(![HPHomeArchive applyPendingAtStatePath:HPStatePath() documentsPath:HPDocuments() defaults:defaults domain:NSBundle.mainBundle.bundleIdentifier error:&restoreError])HPBootstrapError=restoreError.localizedDescription;
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

static void HPAlert(UIViewController *controller, NSString *title, NSString *message);
#import "LightStudio.inc"

@interface HPPanel : UITableViewController <UIDocumentPickerDelegate>
@property(nonatomic,strong) NSArray *scenes;
@property(nonatomic,strong) NSString *detail;
@end

static UIWindow *HPContentWindow(void){
    UIWindow *fallback=nil;
    for(UIWindow *window in UIApplication.sharedApplication.windows){
        if(window.hidden||!window.rootViewController||window.windowLevel!=UIWindowLevelNormal||[window.accessibilityIdentifier isEqual:@"HP.FloatingWindow"])continue;
        if(window.windowScene&&window.windowScene.activationState!=UISceneActivationStateForegroundActive&&window.windowScene.activationState!=UISceneActivationStateForegroundInactive)continue;
        if(window.isKeyWindow)return window;if(!fallback)fallback=window;
    }
    return fallback;
}
@interface HPManager : NSObject
@property(nonatomic,strong) UIButton *button,*studioButton;
@property(nonatomic,strong) UIWindow *overlay;
@property(nonatomic,strong) NSTimer *visibilityTimer;
@property(nonatomic,weak) UIButton *draggingButton;
@property(nonatomic) CGPoint dragStart;
@property(nonatomic) CGRect keyboardFrame;
@property(nonatomic) BOOL installing,keyboardVisible;
+ (instancetype)shared;
- (void)install;
- (void)show;
- (void)showStudio;
- (void)layoutButtons;
- (void)moveButton:(UIButton *)button to:(CGPoint)point remember:(BOOL)remember;
@end

static UIViewController *HPTop(void) {
    UIWindow *window = HPContentWindow();
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

#import <float.h>
#import "FloatingControls.inc"

@implementation HPPanel
- (void)viewDidLoad {
    [super viewDidLoad]; self.title = @"SYLVANIA Plus";
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc]initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(done)];
    [self refresh];
}
- (void)done { [self dismissViewControllerAnimated:YES completion:nil]; }
- (void)refresh {
    @try { self.scenes=HPExportRoutines(); self.detail=HPBootstrapError?:[NSUserDefaults.standardUserDefaults stringForKey:@"HP.LastImport"]; }
    @catch (NSException *e) { self.scenes=@[]; self.detail=e.reason; }
    [self.tableView reloadData];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 4; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section==0?4:(section==3?1:(section==2?2:3)); }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return @[@"Make the room come alive",@"Your home",@"Keep your home portable",@"Direct local control"][section];
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section==0) return @"Explore moving colour, then arrange the shortcut row and the path effects take through your lights.";
    if (section==1) return self.detail ?: @"The original scene editor and controls are retained. New scenes have no ten-routine cap.";
    if (section==2) return @"One Home file carries your paired lights, groups, scenes and ordering. Import it on another numbered Device build to control the same bulbs. Home files include private pairing data.";
    return @"Install Device 1, Device 2 and Device 3 on different devices. Each has its own mesh address and sequence counter. Bluetooth control works without another phone running. Later edits stay local until you share them.";
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc]initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:nil];
    if(path.section==0){
        cell.textLabel.text=@[@"Open Light Studio",@"Arrange scene shortcuts",@"Map light travel path",@"Everyday light behaviour"][path.row];cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        if(path.row==0){cell.textLabel.textColor=[UIColor colorWithRed:.48 green:.23 blue:.8 alpha:1];cell.textLabel.font=[UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];}
    }else if(path.section==1) {
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
        cell.textLabel.text=@[@"Scene limit",@"Saved scenes",@"Controller"][path.row];
        cell.detailTextLabel.text=path.row==0?@"Removed":(path.row==1?[NSString stringWithFormat:@"%lu",(unsigned long)self.scenes.count]:[NSUserDefaults.standardUserDefaults stringForKey:@"HP.Profile"]?:HPSeed[@"Profile"]);
    } else if(path.section==2) {
        cell.textLabel.text=@[@"Export Home",@"Import Home"][path.row];
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
- (void)exportBackup {
    [HPLightEngine.shared stop];
    @try {
        id realm=HPRealm();NSURL *realmURL=[self exportLocation:@"default.realm"];
        NSError *error=nil;SEL selector=NSSelectorFromString(@"writeCopyToURL:encryptionKey:error:");
        BOOL saved=[realm respondsToSelector:selector]&&((BOOL(*)(id,SEL,id,id,NSError **))objc_msgSend)(realm,selector,realmURL,nil,&error);
        if(!saved){HPAlert(self,@"Could not export Home",error.localizedDescription?:@"The scene database could not be copied.");return;}
        NSMutableDictionary *files=[[HPHomeArchive documentsAtPath:HPDocuments() error:&error]mutableCopy];
        if(!files){HPAlert(self,@"Could not export Home",error.localizedDescription);return;}
        files[@"default.realm"]=[NSData dataWithContentsOfURL:realmURL];
        // SQLite's backup API includes any committed WAL data.
        NSString *sqlitePath=[HPDocuments() stringByAppendingPathComponent:@"UserModel.sqlite"];
        if([NSFileManager.defaultManager fileExistsAtPath:sqlitePath]){
            NSURL *copy=[self exportLocation:@"UserModel.sqlite"];sqlite3 *source=NULL,*target=NULL;
            int read=sqlite3_open_v2(sqlitePath.fileSystemRepresentation,&source,SQLITE_OPEN_READONLY,NULL);
            int opened=sqlite3_open(copy.path.fileSystemRepresentation,&target);int result=SQLITE_ERROR;
            if(read==SQLITE_OK&&opened==SQLITE_OK){sqlite3_backup *backup=sqlite3_backup_init(target,"main",source,"main");if(backup){result=sqlite3_backup_step(backup,-1);sqlite3_backup_finish(backup);}}
            if(source)sqlite3_close(source);if(target)sqlite3_close(target);
            if(result!=SQLITE_DONE){HPAlert(self,@"Could not export Home",@"The Home account database is busy. Try again in a moment.");return;}
            files[@"UserModel.sqlite"]=[NSData dataWithContentsOfURL:copy];
        }
        NSDictionary *preferences=[NSUserDefaults.standardUserDefaults persistentDomainForName:NSBundle.mainBundle.bundleIdentifier]?:@{};
        NSData *data=[HPHomeArchive encodeDocument:@{@"Profile":preferences[@"HP.Profile"]?:@"This device",@"Preferences":preferences,@"Documents":files}error:&error];
        NSURL *file=[self exportLocation:@"My_Home.sylhome"];
        if(!data||![data writeToURL:file options:NSDataWritingAtomic error:&error]){HPAlert(self,@"Could not export Home",error.localizedDescription);return;}
        [self shareFile:file];
    }@catch(NSException *exception){HPAlert(self,@"Could not export Home",exception.reason);}
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    if(path.section==0){
        if(path.row==0)[self.navigationController pushViewController:[HPStudio new] animated:YES];
        else if(path.row==2)[self.navigationController pushViewController:[HPSpatialEditor new] animated:YES];
        else if(path.row==3)[self.navigationController pushViewController:[[HPBulbSettings alloc]initWithStyle:UITableViewStyleInsetGrouped] animated:YES];
        else{HPPathEditor *editor=[[HPPathEditor alloc]initWithStyle:UITableViewStyleInsetGrouped];editor.scenes=path.row==1;[self.navigationController pushViewController:editor animated:YES];}
        return;
    }
    if(path.section!=2)return;
    if(path.row==0)[self exportBackup];
    else {
        UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc]initWithDocumentTypes:@[@"com.ledvance.smart.plus.home",@"public.data"] inMode:UIDocumentPickerModeImport];
        picker.delegate=self;[self presentViewController:picker animated:YES completion:nil];
    }
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url=urls.firstObject;if(!url)return;
    BOOL scoped=[url startAccessingSecurityScopedResource];NSError *error=nil;
    NSNumber *size;[url getResourceValue:&size forKey:NSURLFileSizeKey error:&error];
    NSData *data=size.unsignedLongLongValue<=70*1024*1024?[NSData dataWithContentsOfURL:url options:0 error:&error]:nil;
    NSDictionary *document=[HPHomeArchive readData:data error:&error];
    if(scoped)[url stopAccessingSecurityScopedResource];
    NSDictionary *preferences=[NSUserDefaults.standardUserDefaults persistentDomainForName:NSBundle.mainBundle.bundleIdentifier];
    if(document&&! [HPHomeArchive preparedDocument:document receivingPreferences:preferences error:&error])document=nil;
    // Let Realm validate the copied native database with the running app's schema.
    if(document)@try{
        NSURL *realmURL=[self exportLocation:@"Import.realm"];
        if(![document[@"Documents"][@"default.realm"]writeToURL:realmURL options:NSDataWritingAtomic error:&error])document=nil;
        else @autoreleasepool{
            id config=[HPGet(HPRealm(),@"configuration")copy];HPSet(config,@"setFileURL:",realmURL);
            SEL deleteOnMigration=NSSelectorFromString(@"setDeleteRealmIfMigrationNeeded:");
            if([config respondsToSelector:deleteOnMigration])((void(*)(id,SEL,BOOL))objc_msgSend)(config,deleteOnMigration,NO);
            ((void(*)(id,SEL,BOOL))objc_msgSend)(config,NSSelectorFromString(@"setReadOnly:"),YES);
            SEL open=NSSelectorFromString(@"realmWithConfiguration:error:");
            id checked=((id(*)(id,SEL,id,NSError **))objc_msgSend)(NSClassFromString(@"RLMRealm"),open,config,&error);
            if(!checked)document=nil;
        }
    }@catch(NSException *exception){document=nil;error=[NSError errorWithDomain:@"HomeImport" code:1 userInfo:@{NSLocalizedDescriptionKey:exception.reason?:@"The Home database cannot be opened."}];}
    NSString *message=error.localizedDescription;
    NSDictionary *ready=document;
    void(^present)(void)=^{
        if(!ready){HPAlert(self,@"Could not import Home",message?:@"Choose a valid SYLVANIA Plus Home file.");return;}
        UIAlertController *confirmation=[UIAlertController alertControllerWithTitle:@"Import this Home?" message:@"Replace this device's lights, groups, scenes and ordering with the saved Home. Your device keeps its own controller identity. Close and reopen SYLVANIA Plus after importing." preferredStyle:UIAlertControllerStyleAlert];
        [confirmation addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
        [confirmation addAction:[UIAlertAction actionWithTitle:@"Import Home" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action){
            [HPLightEngine.shared stop];NSError *stageError=nil;
            if([HPHomeArchive stageDocument:ready statePath:HPStatePath() error:&stageError])HPAlert(self,@"Home ready to import",@"Close SYLVANIA Plus from the app switcher, then reopen it to load your Home. No lights need to be paired again.");
            else HPAlert(self,@"Could not import Home",stageError.localizedDescription);
        }]];
        [self presentViewController:confirmation animated:YES completion:nil];
    };
    if(self.presentedViewController==controller)[controller dismissViewControllerAnimated:YES completion:present];else dispatch_async(dispatch_get_main_queue(),present);
}

@end

#import "HomeEnhancements.inc"
#import "DarkTheme.inc"

static IMP HPOriginalDidAppear;
static void HPDidAppear(id self,SEL selector,BOOL animated) {
    ((void(*)(id,SEL,BOOL))HPOriginalDidAppear)(self,selector,animated);
    HPThemeController(self);
    [[HPManager shared]install];
    HPEnhanceHome(self);
}
__attribute__((constructor)) static void HPInitialize(void) {
    @autoreleasepool {
        HPIdentifyTask=UIBackgroundTaskInvalid;
        HPBootstrap();
        HPInstallEnhancements();
        HPInstallDarkTheme();
        Method method=class_getInstanceMethod(UIViewController.class,@selector(viewDidAppear:));
        if(method)HPOriginalDidAppear=method_setImplementation(method,(IMP)HPDidAppear);
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note){[[HPManager shared]install];}];
    }
}
