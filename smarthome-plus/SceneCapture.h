#import <Foundation/Foundation.h>
@interface HPSceneCapture : NSObject
+ (NSDictionary *)sceneWithName:(NSString *)name readings:(NSArray *)readings error:(NSError **)error;
+ (BOOL)writeScene:(NSDictionary *)scene realm:(id)realm error:(NSError **)error;
+ (BOOL)deleteScene:(NSString *)identifier realm:(id)realm error:(NSError **)error;
@end
