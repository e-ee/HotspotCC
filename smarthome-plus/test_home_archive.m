#import "HomeArchive.h"
#include <assert.h>

static NSString *A=@"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",*B=@"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
static NSDictionary *Node(NSString *uuid,NSString *address){return @{@"UUID":uuid,@"unicastAddress":address,@"deviceKey":@"0123456789abcdef0123456789abcdef",@"elements":@[@{}]};}
static NSDictionary *Controller(NSString *uuid,NSString *low,NSString *high){return @{@"UUID":uuid,@"allocatedUnicastRange":@[@{@"lowAddress":low,@"highAddress":high}]};}
static NSDictionary *Prefs(NSString *who,NSInteger counter){
    NSDictionary *mesh=@{@"meshUUID":@"fixture-home",@"ivIndex":@0,@"nodes":@[Node(A,@"7E00"),Node(B,@"7D00"),Node(@"CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",@"0020")],@"provisioners":@[Controller(A,@"7E00",@"7EFF"),Controller(B,@"7D00",@"7DFF")],@"groups":@[],@"netKeys":@[@{@"key":@"0123456789abcdef0123456789abcdef"}],@"appKeys":@[@{@"key":@"abcdef0123456789abcdef0123456789"}]};
    return @{@"mesh.json":[NSJSONSerialization dataWithJSONObject:mesh options:0 error:nil],@"currenProvisionerUUID_key":who,@"currenProvisionerSno_key":@(counter),@"uniqueID":who,@"HP.Profile":[who isEqual:A]?@"Device 1":@"Device 2",@"currentMeshProvisionAddress_key":@{who:@([who isEqual:A]?0x7e01:0x7d01)}};
}
int main(void){@autoreleasepool{
    NSError *error=nil;
    // Opaque database contents include values a scene JSON validator rejected.
    NSData *realm=[@"native Realm bytes; color=-1 cct=-1 deviceType=-1; 11 scenes"dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary *files=@{@"default.realm":realm,@"Network/network_fixture":[Prefs(A,9000)objectForKey:@"mesh.json"]};
    NSData *encoded=[HPHomeArchive encodeDocument:@{@"Preferences":Prefs(A,9000),@"Documents":files}error:&error];assert(encoded);
    NSDictionary *home=[HPHomeArchive readData:encoded error:&error];assert(home&&[home[@"Documents"][@"default.realm"]isEqual:realm]);
    NSDictionary *received=[HPHomeArchive preparedDocument:home receivingPreferences:Prefs(B,23000) error:&error];assert(received);
    NSDictionary *p=received[@"Preferences"];assert([p[@"currenProvisionerUUID_key"]isEqual:B]&&[p[@"HP.Profile"]isEqual:@"Device 2"]&&[p[@"currenProvisionerSno_key"]intValue]==23256);
    assert([received[@"Documents"][@"default.realm"]isEqual:realm]);
    NSDictionary *same=[HPHomeArchive preparedDocument:home receivingPreferences:Prefs(A,5000) error:&error];assert([same[@"Preferences"][@"currenProvisionerSno_key"]intValue]==9256);
    NSMutableDictionary *sender=[Prefs(A,9000)mutableCopy],*receiver=[Prefs(B,23000)mutableCopy];sender[@"HP.Float.PLUS"]=@[@.1,@.2];sender[@"HP.Float.STUDIO"]=@[@.2,@.3];sender[@"HP.LightLayout"]=@{@"fixture-home":@{@"lamp-0":@{@"x":@.2,@"y":@.8}}};sender[@"HP.StudioMoods"]=@{@"fixture-home":@[@{@"name":@"Calm evening",@"brightness":@.4}]};receiver[@"HP.Float.PLUS"]=@[@.8,@.9];receiver[@"HP.Float.STUDIO"]=@[@.8,@.7];
    NSDictionary *mapped=[HPHomeArchive readData:[HPHomeArchive encodeDocument:@{@"Preferences":sender,@"Documents":files}error:&error]error:&error];NSDictionary *moved=[HPHomeArchive preparedDocument:mapped receivingPreferences:receiver error:&error];assert([moved[@"Preferences"][@"HP.LightLayout"]isEqual:sender[@"HP.LightLayout"]]);assert([moved[@"Preferences"][@"HP.StudioMoods"]isEqual:sender[@"HP.StudioMoods"]]);assert([moved[@"Preferences"][@"HP.Float.PLUS"]isEqual:receiver[@"HP.Float.PLUS"]]&&[moved[@"Preferences"][@"HP.Float.STUDIO"]isEqual:receiver[@"HP.Float.STUDIO"]]);
    NSMutableDictionary *bad=[home mutableCopy],*badFiles=[files mutableCopy];badFiles[@"../outside"]=realm;bad[@"Documents"]=badFiles;assert(![HPHomeArchive validatedDocument:bad error:&error]);
    bad=[home mutableCopy];badFiles=[files mutableCopy];badFiles[@"default.realm"]=[@"altered database contents and old hash"dataUsingEncoding:NSUTF8StringEncoding];bad[@"Documents"]=badFiles;assert(![HPHomeArchive validatedDocument:bad error:&error]);
    NSMutableDictionary *legacy=[home mutableCopy];legacy[@"format"]=@"sylvania-plus-backup";legacy[@"version"]=@1;[legacy removeObjectForKey:@"Checksums"];assert([HPHomeArchive validatedDocument:legacy error:&error]);
    NSString *root=[NSTemporaryDirectory()stringByAppendingPathComponent:NSUUID.UUID.UUIDString],*docs=[root stringByAppendingPathComponent:@"Documents"],*state=[root stringByAppendingPathComponent:@"State"];
    [NSFileManager.defaultManager createDirectoryAtPath:docs withIntermediateDirectories:YES attributes:nil error:nil];NSData *old=[@"previous native Realm snapshot"dataUsingEncoding:NSUTF8StringEncoding];assert([old writeToFile:[docs stringByAppendingPathComponent:@"default.realm"]atomically:YES]);
    NSString *domain=[@"test.home."stringByAppendingString:NSUUID.UUID.UUIDString];NSUserDefaults *defaults=[[NSUserDefaults alloc]initWithSuiteName:domain];[defaults setPersistentDomain:Prefs(B,23000)forName:domain];
    assert([HPHomeArchive stageDocument:home statePath:state error:&error]);
    // Sequence advances between staging and restart: preserve the later value.
    [defaults setObject:@24000 forKey:@"currenProvisionerSno_key"];[defaults synchronize];
    assert([HPHomeArchive applyPendingAtStatePath:state documentsPath:docs defaults:defaults domain:domain error:&error]);
    assert([[NSData dataWithContentsOfFile:[docs stringByAppendingPathComponent:@"default.realm"]]isEqual:realm]);assert([defaults integerForKey:@"currenProvisionerSno_key"]==24256);assert([[defaults stringForKey:@"currenProvisionerUUID_key"]isEqual:B]);
    assert(![NSFileManager.defaultManager fileExistsAtPath:[state stringByAppendingPathComponent:@"Incoming.sylhome"]]);
    // Simulate a process interruption halfway through file replacement.
    NSDictionary *journal=@{@"Preferences":Prefs(B,25000),@"Documents":@{@"default.realm":old},@"TouchedPaths":@[@"default.realm",@"Network/network_fixture"],@"Committed":@NO};
    assert([journal writeToFile:[state stringByAppendingPathComponent:@"Restore.plist"]atomically:YES]);
    assert([HPHomeArchive applyPendingAtStatePath:state documentsPath:docs defaults:defaults domain:domain error:&error]);assert([[NSData dataWithContentsOfFile:[docs stringByAppendingPathComponent:@"default.realm"]]isEqual:old]);assert([defaults integerForKey:@"currenProvisionerSno_key"]==25000);
    [defaults removePersistentDomainForName:domain];[NSFileManager.defaultManager removeItemAtPath:root error:nil];
    puts("Home transfer: opaque scene preservation, per-device identity, monotonic counters, legacy import, corruption/path rejection, apply and interrupted-restore recovery passed.");
}}
