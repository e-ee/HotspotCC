#import <Foundation/Foundation.h>

// Portable, versioned scene data. Never includes mesh or device keys.
@interface HPSceneTransfer : NSObject
+ (NSDictionary *)validatedDocument:(id)document meshUUID:(NSString *)meshUUID error:(NSError **)error;
+ (NSArray *)orderedRecords:(NSArray *)records identifiers:(NSArray *)identifiers key:(NSString *)key;
@end
