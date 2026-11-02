#import "HomeArchive.h"
#import <CommonCrypto/CommonDigest.h>

static id HPHomeFail(NSError **error,NSString *message){if(error)*error=[NSError errorWithDomain:@"HomeArchive" code:1 userInfo:@{NSLocalizedDescriptionKey:message}];return nil;}
static NSString *HPDigest(NSData *data){unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256(data.bytes,(CC_LONG)data.length,digest);NSMutableString *s=[NSMutableString new];for(int i=0;i<CC_SHA256_DIGEST_LENGTH;i++)[s appendFormat:@"%02x",digest[i]];return s;}
static BOOL HPHex(NSString *s,NSUInteger count){return [s isKindOfClass:NSString.class]&&s.length==count&&[s rangeOfCharacterFromSet:[[NSCharacterSet characterSetWithCharactersInString:@"0123456789abcdefABCDEF"]invertedSet]].location==NSNotFound;}
static NSUInteger HPAddress(NSString *s){return HPHex(s,4)?strtoul(s.UTF8String,NULL,16):0;}
static NSDictionary *HPDecodeMesh(NSDictionary *prefs){id data=prefs[@"mesh.json"];return [data isKindOfClass:NSData.class]?[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:nil]:nil;}
static BOOL HPWritePlist(id value,NSString *path,NSError **error){NSData *data=[NSPropertyListSerialization dataWithPropertyList:value format:NSPropertyListBinaryFormat_v1_0 options:0 error:error];return data&&[data writeToFile:path options:NSDataWritingAtomic error:error];}

@implementation HPHomeArchive
+ (BOOL)isHomePath:(NSString *)path{
    if(![path isKindOfClass:NSString.class]||[path hasPrefix:@"/"]||[path.pathComponents containsObject:@".."]||[path.pathComponents containsObject:@"."])return NO;
    if([@[@"default.realm",@"UserModel.sqlite",@"AppStatusAssistant.archiver",@"networkCompareList.archiver"]containsObject:path])return YES;
    return path.pathComponents.count==2&&[path.pathComponents[0]isEqual:@"Network"]&&[path.lastPathComponent hasPrefix:@"network_"]&&path.lastPathComponent.length<200;
}
+ (NSDictionary *)validatedDocument:(id)raw error:(NSError **)error{
    if(![raw isKindOfClass:NSDictionary.class])return HPHomeFail(error,@"Choose a SYLVANIA Plus Home file.");
    NSDictionary *d=raw;BOOL legacy=[d[@"format"]isEqual:@"sylvania-plus-backup"]&&[d[@"version"]isEqual:@1];
    if(!legacy&&(![d[@"format"]isEqual:@"sylvania-plus-home"]||![d[@"version"]isEqual:@2]))return HPHomeFail(error,@"This Home file uses an unsupported format.");
    NSDictionary *files=d[@"Documents"],*prefs=d[@"Preferences"];
    if(![files isKindOfClass:NSDictionary.class]||![prefs isKindOfClass:NSDictionary.class]||files.count>128||![files[@"default.realm"]isKindOfClass:NSData.class]||[files[@"default.realm"]length]<24)return HPHomeFail(error,@"The Home file is missing its scene database.");
    NSUInteger bytes=0;for(id path in files){id data=files[path];if(![self isHomePath:path]||![data isKindOfClass:NSData.class])return HPHomeFail(error,@"The Home file contains an unsupported document.");bytes+=[data length];if(bytes>64*1024*1024)return HPHomeFail(error,@"This Home file is too large.");}
    if(!legacy){NSDictionary *hashes=d[@"Checksums"];if(![hashes isKindOfClass:NSDictionary.class]||hashes.count!=files.count)return HPHomeFail(error,@"The Home file is incomplete.");for(NSString *path in files)if(![HPDigest(files[path])isEqual:hashes[path]])return HPHomeFail(error,@"The Home file is damaged. Export it again from the source device.");}
    NSDictionary *mesh=HPDecodeMesh(prefs);
    if(![mesh isKindOfClass:NSDictionary.class]||![mesh[@"meshUUID"]isKindOfClass:NSString.class]||![mesh[@"nodes"]isKindOfClass:NSArray.class]||![mesh[@"provisioners"]isKindOfClass:NSArray.class]||![mesh[@"groups"]isKindOfClass:NSArray.class]||![mesh[@"netKeys"]isKindOfClass:NSArray.class]||![mesh[@"appKeys"]isKindOfClass:NSArray.class]||![mesh[@"netKeys"]count]||![mesh[@"appKeys"]count])return HPHomeFail(error,@"The Home file is missing its light network.");
    for(NSString *key in @[@"netKeys",@"appKeys"])for(id entry in mesh[key])if(![entry isKindOfClass:NSDictionary.class]||!HPHex(entry[@"key"],32))return HPHomeFail(error,@"The Home file has an incomplete network key.");
    NSMutableIndexSet *occupied=[NSMutableIndexSet new];NSMutableSet *ids=[NSMutableSet new];
    for(id node in mesh[@"nodes"]){
        if(![node isKindOfClass:NSDictionary.class]||![node[@"UUID"]isKindOfClass:NSString.class]||[ids containsObject:node[@"UUID"]]||!HPHex(node[@"deviceKey"],32)||![node[@"elements"]isKindOfClass:NSArray.class])return HPHomeFail(error,@"The Home file contains an incomplete light record.");
        NSUInteger start=HPAddress(node[@"unicastAddress"]),count=[node[@"elements"]count];
        if(!start||!count||start+count>0x8000||[occupied intersectsIndexesInRange:NSMakeRange(start,count)])return HPHomeFail(error,@"The Home file contains overlapping light addresses.");
        [ids addObject:node[@"UUID"]];[occupied addIndexesInRange:NSMakeRange(start,count)];
    }
    for(id p in mesh[@"provisioners"])if(![p isKindOfClass:NSDictionary.class]||![p[@"UUID"]isKindOfClass:NSString.class]||![p[@"allocatedUnicastRange"]isKindOfClass:NSArray.class])return HPHomeFail(error,@"The Home file contains an incomplete controller.");
    return d;
}
+ (NSData *)encodeDocument:(NSDictionary *)document error:(NSError **)error{
    NSMutableDictionary *d=[document mutableCopy];d[@"format"]=@"sylvania-plus-home";d[@"version"]=@2;
    NSMutableDictionary *hashes=[NSMutableDictionary new];for(NSString *path in d[@"Documents"])hashes[path]=HPDigest(d[@"Documents"][path]);d[@"Checksums"]=hashes;
    if(![self validatedDocument:d error:error])return nil;
    return [NSPropertyListSerialization dataWithPropertyList:d format:NSPropertyListBinaryFormat_v1_0 options:0 error:error];
}
+ (NSDictionary *)readData:(NSData *)data error:(NSError **)error{
    if(!data||data.length>70*1024*1024)return HPHomeFail(error,@"Choose a Home file smaller than 70 MB.");
    id d=[NSPropertyListSerialization propertyListWithData:data options:NSPropertyListMutableContainersAndLeaves format:nil error:error];return [self validatedDocument:d error:error];
}
+ (NSDictionary *)preparedDocument:(NSDictionary *)document receivingPreferences:(NSDictionary *)current error:(NSError **)error{
    NSDictionary *valid=[self validatedDocument:document error:error];if(!valid)return nil;
    NSMutableDictionary *mesh=[HPDecodeMesh(valid[@"Preferences"])mutableCopy];NSDictionary *own=HPDecodeMesh(current);
    NSString *uuid=current[@"currenProvisionerUUID_key"];NSDictionary *ownNode,*ownController;
    for(NSDictionary *n in own[@"nodes"])if([n[@"UUID"]isEqual:uuid])ownNode=n;
    for(NSDictionary *p in own[@"provisioners"])if([p[@"UUID"]isEqual:uuid])ownController=p;
    if(!ownNode||!ownController)return HPHomeFail(error,@"This device's controller identity is unavailable. Open your existing Home first.");
    // Never take the sending phone's identity or sequence number.
    NSUInteger start=HPAddress(ownNode[@"unicastAddress"]),count=[ownNode[@"elements"]count];
    for(NSDictionary *n in mesh[@"nodes"]){if([n[@"UUID"]isEqual:uuid])continue;NSUInteger other=HPAddress(n[@"unicastAddress"]),end=other+[n[@"elements"]count];if(other<start+count&&end>start)return HPHomeFail(error,@"This Home uses this device's controller address. Use a different numbered Device build.");}
    for(NSDictionary *p in mesh[@"provisioners"]){if([p[@"UUID"]isEqual:uuid])continue;for(NSDictionary *r in p[@"allocatedUnicastRange"])for(NSDictionary *mine in ownController[@"allocatedUnicastRange"])if(HPAddress(r[@"lowAddress"])<=HPAddress(mine[@"highAddress"])&&HPAddress(r[@"highAddress"])>=HPAddress(mine[@"lowAddress"]))return HPHomeFail(error,@"This Home reserves this controller's address range. Use a different numbered Device build.");}
    NSMutableArray *nodes=[NSMutableArray new],*controllers=[NSMutableArray new];
    for(NSDictionary *n in mesh[@"nodes"])if(![n[@"UUID"]isEqual:uuid])[nodes addObject:n];[nodes addObject:ownNode];
    for(NSDictionary *p in mesh[@"provisioners"])if(![p[@"UUID"]isEqual:uuid])[controllers addObject:p];[controllers addObject:ownController];
    mesh[@"nodes"]=nodes;mesh[@"provisioners"]=controllers;
    BOOL sameHome=[own[@"meshUUID"]isEqual:mesh[@"meshUUID"]];
    if(sameHome&&[own[@"ivIndex"]longLongValue]>[mesh[@"ivIndex"]longLongValue])mesh[@"ivIndex"]=own[@"ivIndex"];
    NSMutableDictionary *prefs=[valid[@"Preferences"]mutableCopy];long long counter=[current[@"currenProvisionerSno_key"]longLongValue];
    if(sameHome&&[prefs[@"currenProvisionerUUID_key"]isEqual:uuid])counter=MAX(counter,[prefs[@"currenProvisionerSno_key"]longLongValue]);
    if(counter<0||counter>=0xffff00)return HPHomeFail(error,@"This controller needs a mesh sequence refresh before a Home can be imported.");
    prefs[@"currenProvisionerUUID_key"]=uuid;prefs[@"currenProvisionerSno_key"]=@(counter+256);
    prefs[@"uniqueID"]=current[@"uniqueID"]?:uuid;prefs[@"HP.Profile"]=current[@"HP.Profile"]?:@"This device";prefs[@"HP.Seeded"]=@YES;
    NSMutableDictionary *addresses=[prefs[@"currentMeshProvisionAddress_key"]isKindOfClass:NSDictionary.class]?[prefs[@"currentMeshProvisionAddress_key"]mutableCopy]:[NSMutableDictionary new];
    NSUInteger next=[current[@"currentMeshProvisionAddress_key"][uuid]unsignedIntegerValue];addresses[uuid]=@(MAX(MAX(next,[addresses[uuid]unsignedIntegerValue]),start+count));prefs[@"currentMeshProvisionAddress_key"]=addresses;
    for(NSString *key in @[@"kUUIDOfConnectedDevices",@"scanList_key"]){[prefs removeObjectForKey:key];if(sameHome&&current[key])prefs[key]=current[key];}
    prefs[@"InfoAlertPresented"]=@YES;prefs[@"SHOWALERT"]=@NO;
    NSData *meshData=[NSJSONSerialization dataWithJSONObject:mesh options:0 error:error];if(!meshData)return nil;prefs[@"mesh.json"]=meshData;
    NSMutableDictionary *files=[valid[@"Documents"]mutableCopy];
    for(NSString *path in files.allKeys)if([path hasPrefix:@"Network/"]){NSDictionary *network=[NSJSONSerialization JSONObjectWithData:files[path] options:0 error:nil];if([network[@"meshUUID"]isEqual:mesh[@"meshUUID"]])files[path]=meshData;}
    NSMutableDictionary *result=[valid mutableCopy];result[@"Preferences"]=prefs;result[@"Documents"]=files;result[@"Profile"]=prefs[@"HP.Profile"];
    // Recompute hashes after adjusting the receiving controller.
    NSData *encoded=[self encodeDocument:result error:error];return encoded?[self readData:encoded error:error]:nil;
}
+ (NSDictionary *)documentsAtPath:(NSString *)root error:(NSError **)error{
    NSMutableDictionary *files=[NSMutableDictionary new];NSFileManager *fm=NSFileManager.defaultManager;
    for(NSString *path in @[@"default.realm",@"UserModel.sqlite",@"AppStatusAssistant.archiver",@"networkCompareList.archiver"]){NSString *full=[root stringByAppendingPathComponent:path];if([fm fileExistsAtPath:full]){NSData *data=[NSData dataWithContentsOfFile:full options:0 error:error];if(!data)return nil;files[path]=data;}}
    for(NSString *name in [fm contentsOfDirectoryAtPath:[root stringByAppendingPathComponent:@"Network"] error:nil]){NSString *path=[@"Network" stringByAppendingPathComponent:name];if([self isHomePath:path]){NSData *data=[NSData dataWithContentsOfFile:[root stringByAppendingPathComponent:path]options:0 error:error];if(!data)return nil;files[path]=data;}}
    return files;
}
+ (BOOL)stageDocument:(NSDictionary *)document statePath:(NSString *)state error:(NSError **)error{
    NSData *data=[self encodeDocument:document error:error];if(!data)return NO;
    if(![NSFileManager.defaultManager createDirectoryAtPath:state withIntermediateDirectories:YES attributes:nil error:error])return NO;
    return [data writeToFile:[state stringByAppendingPathComponent:@"Incoming.sylhome"]options:NSDataWritingAtomic error:error];
}
+ (BOOL)writeDocuments:(NSDictionary *)files touched:(NSArray *)paths root:(NSString *)root error:(NSError **)error{
    NSFileManager *fm=NSFileManager.defaultManager;
    for(NSString *relative in paths){if(![self isHomePath:relative])return NO;NSString *path=[root stringByAppendingPathComponent:relative];NSData *data=files[relative];if(data){if(![fm createDirectoryAtPath:path.stringByDeletingLastPathComponent withIntermediateDirectories:YES attributes:nil error:error]||![data writeToFile:path options:NSDataWritingAtomic error:error])return NO;}else if([fm fileExistsAtPath:path]&&![fm removeItemAtPath:path error:error])return NO;}
    for(NSString *temporary in @[@"default.realm.lock",@"default.realm.note",@"default.realm.management",@"UserModel.sqlite-wal",@"UserModel.sqlite-shm"])[fm removeItemAtPath:[root stringByAppendingPathComponent:temporary]error:nil];return YES;
}
+ (BOOL)applyPendingAtStatePath:(NSString *)state documentsPath:(NSString *)root defaults:(NSUserDefaults *)defaults domain:(NSString *)domain error:(NSError **)error{
    NSFileManager *fm=NSFileManager.defaultManager;NSString *journalPath=[state stringByAppendingPathComponent:@"Restore.plist"],*incoming=[state stringByAppendingPathComponent:@"Incoming.sylhome"];
    NSDictionary *journal=[NSDictionary dictionaryWithContentsOfFile:journalPath];
    if(journal){
        if(![journal[@"Committed"]boolValue]){if(![self writeDocuments:journal[@"Documents"]touched:journal[@"TouchedPaths"]root:root error:error])return NO;[defaults setPersistentDomain:journal[@"Preferences"]forName:domain];[defaults synchronize];}
        else [fm removeItemAtPath:incoming error:nil];
        [fm removeItemAtPath:journalPath error:nil];
    }
    if(![fm fileExistsAtPath:incoming])return YES;
    NSDictionary *document=[self readData:[NSData dataWithContentsOfFile:incoming options:0 error:error]error:error];if(!document)return NO;
    NSDictionary *prefs=[defaults persistentDomainForName:domain]?:@{};
    NSDictionary *prepared=[self preparedDocument:document receivingPreferences:prefs error:error];if(!prepared)return NO;
    NSDictionary *before=[self documentsAtPath:root error:error];if(!before)return NO;
    NSMutableSet *touched=[NSMutableSet setWithArray:before.allKeys];[touched addObjectsFromArray:[prepared[@"Documents"]allKeys]];
    NSMutableDictionary *rollback=[@{@"Preferences":prefs,@"Documents":before,@"TouchedPaths":touched.allObjects,@"Committed":@NO}mutableCopy];
    if(!HPWritePlist(rollback,journalPath,error))return NO;
    if(![self writeDocuments:prepared[@"Documents"]touched:touched.allObjects root:root error:error]){
        if([self writeDocuments:before touched:touched.allObjects root:root error:nil])[fm removeItemAtPath:journalPath error:nil];return NO;
    }
    [defaults setPersistentDomain:prepared[@"Preferences"]forName:domain];[defaults setObject:@"Home imported" forKey:@"HP.LastImport"];[defaults synchronize];
    rollback[@"Committed"]=@YES;
    if(!HPWritePlist(rollback,journalPath,error)){
        if([self writeDocuments:before touched:touched.allObjects root:root error:nil]){[defaults setPersistentDomain:prefs forName:domain];[defaults synchronize];[fm removeItemAtPath:journalPath error:nil];}
        return NO;
    }
    [fm removeItemAtPath:incoming error:nil];[fm removeItemAtPath:journalPath error:nil];return YES;
}
@end
