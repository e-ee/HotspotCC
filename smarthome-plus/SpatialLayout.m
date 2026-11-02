#import "SpatialLayout.h"
#include <math.h>
@implementation HPSpatialLayout
+ (NSArray *)positionedLights:(NSArray *)lights layout:(NSDictionary *)layout{
    NSMutableArray *result=[NSMutableArray new];NSUInteger cols=MAX(1,(NSUInteger)ceil(sqrt(lights.count))),rows=MAX(1,(lights.count+cols-1)/cols);
    for(NSUInteger i=0;i<lights.count;i++){
        NSMutableDictionary *light=[lights[i]mutableCopy];NSDictionary *point=[layout isKindOfClass:NSDictionary.class]?layout[light[@"id"]]:nil;
        double x=(i%cols+.5)/cols,y=(i/cols+.5)/rows;
        if([point isKindOfClass:NSDictionary.class]&&[point[@"x"]isKindOfClass:NSNumber.class]&&[point[@"y"]isKindOfClass:NSNumber.class]){double px=[point[@"x"]doubleValue],py=[point[@"y"]doubleValue];if(isfinite(px)&&isfinite(py)){x=fmin(1,fmax(0,px));y=fmin(1,fmax(0,py));}}
        light[@"x"]=@(x);light[@"y"]=@(y);[result addObject:light];
    }return result;
}
+ (NSDictionary *)layout:(NSDictionary *)layout moving:(NSString *)identifier x:(double)x y:(double)y{
    NSMutableDictionary *result=[layout isKindOfClass:NSDictionary.class]?[layout mutableCopy]:[NSMutableDictionary new];
    if(identifier.length&&isfinite(x)&&isfinite(y))result[identifier]=@{@"x":@(fmin(1,fmax(0,x))),@"y":@(fmin(1,fmax(0,y)))};return result;
}
@end
