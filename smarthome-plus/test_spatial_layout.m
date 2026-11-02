#import <Foundation/Foundation.h>
#import "SpatialLayout.h"
#include <assert.h>
int main(void){@autoreleasepool{
    NSMutableArray *lamps=[NSMutableArray new];for(int i=0;i<14;i++)[lamps addObject:@{@"id":[NSString stringWithFormat:@"lamp-%d",i],@"address":@(32+i),@"name":@"Lamp"}];
    NSArray *initial=[HPSpatialLayout positionedLights:lamps layout:nil];assert(initial.count==14);NSMutableSet *points=[NSMutableSet new];for(NSDictionary *light in initial){assert([light[@"x"]doubleValue]>0&&[light[@"x"]doubleValue]<1&&[light[@"y"]doubleValue]>0&&[light[@"y"]doubleValue]<1);[points addObject:@[light[@"x"],light[@"y"]]];}assert(points.count==14);
    NSDictionary *layout=[HPSpatialLayout layout:nil moving:@"lamp-0" x:.12 y:.84];layout=[HPSpatialLayout layout:layout moving:@"lamp-1" x:9 y:-4];NSArray *placed=[HPSpatialLayout positionedLights:lamps layout:layout];assert(fabs([placed[0][@"x"]doubleValue]-.12)<.00001&&fabs([placed[0][@"y"]doubleValue]-.84)<.00001);assert([placed[1][@"x"]intValue]==1&&[placed[1][@"y"]intValue]==0);assert([placed[13][@"id"]isEqual:@"lamp-13"]);
    assert([[HPSpatialLayout layout:layout moving:@"lamp-0" x:NAN y:.2]isEqual:layout]);
    NSArray *bad=[HPSpatialLayout positionedLights:lamps layout:@{@"lamp-0":@{@"x":@"bad",@"y":@.2}}];assert([bad[0]isEqual:initial[0]]);
    NSData *data=[NSPropertyListSerialization dataWithPropertyList:layout format:NSPropertyListBinaryFormat_v1_0 options:0 error:nil];NSDictionary *roundtrip=[NSPropertyListSerialization propertyListWithData:data options:0 format:NULL error:nil];assert([[HPSpatialLayout positionedLights:lamps layout:roundtrip]isEqual:placed]);
    puts("Apartment geometry: fourteen distinct defaults, stable identities, normalized drag positions, bounds/invalid values and persistent-layout round trip passed.");
}}
