#import "HomeArchive.h"
#include <assert.h>
static NSDictionary *Mesh(NSDictionary *p){return [NSJSONSerialization JSONObjectWithData:p[@"mesh.json"]options:0 error:nil];}
int main(void){@autoreleasepool{
 NSString *authority=@"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",*key=@"0123456789ABCDEF0123456789ABCDEF";
 NSDictionary *mesh=@{@"meshUUID":@"fixture",@"ivIndex":@4,@"nodes":@[@{@"UUID":authority,@"deviceKey":key,@"unicastAddress":@"0001",@"elements":@[@{}]},@{@"UUID":@"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",@"deviceKey":key,@"unicastAddress":@"0020",@"elements":@[@{}]}],@"provisioners":@[@{@"UUID":authority,@"allocatedUnicastRange":@[@{@"lowAddress":@"0001",@"highAddress":@"00FF"}]}],@"groups":@[],@"netKeys":@[@{@"key":key}],@"appKeys":@[@{@"key":key}]};
 NSDictionary *prefs=@{@"mesh.json":[NSJSONSerialization dataWithJSONObject:mesh options:0 error:nil],@"currenProvisionerUUID_key":authority,@"currenProvisionerSno_key":@9000,@"HP.EnrollmentAuthority":authority};
 NSData *realm=[@"opaque native realm fixture bytes for enrollment"dataUsingEncoding:NSUTF8StringEncoding];NSError *error=nil;
 NSDictionary *doc=[HPHomeArchive readData:[HPHomeArchive encodeDocument:@{@"Preferences":prefs,@"Documents":@{@"default.realm":realm,@"Network/network_fixture":prefs[@"mesh.json"]}}error:&error]error:&error];assert(doc);
 NSDictionary *first=[HPHomeArchive isolatedPreferences:prefs],*second=[HPHomeArchive isolatedPreferences:prefs];assert(![first[@"HP.JoinUUID"]isEqual:second[@"HP.JoinUUID"]]);assert(![Mesh(first)[@"meshUUID"]isEqual:mesh[@"meshUUID"]]&&[Mesh(first)[@"nodes"]count]==1&&[Mesh(first)[@"groups"]count]==0);assert(![Mesh(first)[@"netKeys"][0][@"key"]isEqual:key]);
 NSDictionary *request=[HPHomeArchive requestForPreferences:first];assert(request);
 NSDictionary *result=[HPHomeArchive invitationForDocument:doc request:request reservations:@{} error:&error];assert(result);
 NSDictionary *invite=result[@"document"],*ledger=result[@"reservations"];
 assert(![HPHomeArchive preparedDocument:invite receivingPreferences:second error:&error]);
 NSDictionary *joined=[HPHomeArchive preparedDocument:invite receivingPreferences:first error:&error];assert(joined);
 NSDictionary *own=joined[@"Preferences"];assert([own[@"currenProvisionerUUID_key"]isEqual:request[@"UUID"]]&&![own[@"HP.JoinPending"]boolValue]&&[own[@"currenProvisionerSno_key"]intValue]==256);assert([joined[@"Documents"][@"default.realm"]isEqual:realm]);assert(![HPHomeArchive preparedDocument:invite receivingPreferences:own error:&error]);
 NSDictionary *repeat=[HPHomeArchive invitationForDocument:doc request:request reservations:ledger error:&error];assert([repeat[@"reservations"]isEqual:ledger]);
 NSMutableSet *addresses=[NSMutableSet new];
 for(int n=0;n<20;n++){NSDictionary *p=[HPHomeArchive isolatedPreferences:prefs],*req=[HPHomeArchive requestForPreferences:p];NSDictionary *next=[HPHomeArchive invitationForDocument:doc request:req reservations:ledger error:&error];assert(next);ledger=next[@"reservations"];NSString *address=ledger[req[@"UUID"]][@"node"][@"unicastAddress"];assert(![addresses containsObject:address]);[addresses addObject:address];NSDictionary *adopt=[HPHomeArchive preparedDocument:next[@"document"]receivingPreferences:p error:&error];assert(adopt&&[adopt[@"Preferences"][@"currenProvisionerSno_key"]intValue]==256);}
 assert(ledger.count==21);
 NSDictionary *ordinary=[HPHomeArchive preparedDocument:doc receivingPreferences:prefs error:&error];assert(ordinary&&[ordinary[@"Preferences"][@"currenProvisionerSno_key"]intValue]==9256);
 NSString *root=[NSTemporaryDirectory()stringByAppendingPathComponent:NSUUID.UUID.UUIDString],*state=[root stringByAppendingPathComponent:@"state"],*documents=[root stringByAppendingPathComponent:@"documents"],*domain=NSUUID.UUID.UUIDString;NSUserDefaults *defaults=[[NSUserDefaults alloc]initWithSuiteName:domain];[defaults setPersistentDomain:first forName:domain];assert([HPHomeArchive stageDocument:invite statePath:state error:&error]);assert([HPHomeArchive applyPendingAtStatePath:state documentsPath:documents defaults:defaults domain:domain error:&error]);assert([[defaults stringForKey:@"currenProvisionerUUID_key"]isEqual:request[@"UUID"]]&&![defaults boolForKey:@"HP.JoinPending"]);[defaults removePersistentDomainForName:domain];[NSFileManager.defaultManager removeItemAtPath:root error:nil];
 puts("Enrollment passed: isolated first launch, 21 unique reservations, repeat requests, invitation binding, replay rejection, native scene bytes and journaled adoption.");
}}
