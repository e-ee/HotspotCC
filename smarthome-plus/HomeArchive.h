#import <Foundation/Foundation.h>

// Full Home transfers preserve Realm bytes, including legacy scene sentinels.
@interface HPHomeArchive : NSObject
+ (NSDictionary *)isolatedPreferences:(NSDictionary *)seed;
+ (NSDictionary *)requestForPreferences:(NSDictionary *)prefs;
+ (NSDictionary *)invitationForDocument:(NSDictionary *)document request:(NSDictionary *)request reservations:(NSDictionary *)ledger error:(NSError **)error;
+ (BOOL)isHomePath:(NSString *)path;
+ (NSDictionary *)validatedDocument:(id)document error:(NSError **)error;
+ (NSDictionary *)preparedDocument:(NSDictionary *)document receivingPreferences:(NSDictionary *)current error:(NSError **)error;
+ (NSData *)encodeDocument:(NSDictionary *)document error:(NSError **)error;
+ (NSDictionary *)readData:(NSData *)data error:(NSError **)error;
+ (NSDictionary *)documentsAtPath:(NSString *)path error:(NSError **)error;
+ (BOOL)stageDocument:(NSDictionary *)document statePath:(NSString *)state error:(NSError **)error;
+ (BOOL)applyPendingAtStatePath:(NSString *)state documentsPath:(NSString *)documents defaults:(NSUserDefaults *)defaults domain:(NSString *)domain error:(NSError **)error;
@end
