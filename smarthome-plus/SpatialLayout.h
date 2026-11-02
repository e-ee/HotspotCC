#import <Foundation/Foundation.h>
@interface HPSpatialLayout : NSObject
+ (NSArray *)positionedLights:(NSArray *)lights layout:(NSDictionary *)layout;
+ (NSDictionary *)layout:(NSDictionary *)layout moving:(NSString *)identifier x:(double)x y:(double)y;
@end
