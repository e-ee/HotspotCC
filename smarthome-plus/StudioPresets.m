#import "StudioPresets.h"
#import "EffectPatterns.h"
@implementation HPStudioPresets
+ (NSDictionary *)validatedRecipe:(NSDictionary *)recipe error:(NSError **)error {
    if(error)*error=nil;
    if(![recipe isKindOfClass:NSDictionary.class])goto invalid;
    for(NSString *key in @[@"id",@"name",@"scope"]){id value=recipe[key];if(![value isKindOfClass:NSString.class]||![value length]||[value length]>100)goto invalid;}
    if(![[NSUUID alloc]initWithUUIDString:recipe[@"id"]])goto invalid;
    if(![recipe[@"name"]stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet].length||[recipe[@"name"]length]>40)goto invalid;
    for(NSString *key in @[@"mode",@"journey",@"brightness",@"motion",@"depth",@"variation",@"phase",@"journeyElapsed"]){
        id number=recipe[key];if(![number isKindOfClass:NSNumber.class]||!isfinite([number doubleValue]))goto invalid;
    }
    if([recipe[@"mode"]doubleValue]!=[recipe[@"mode"]integerValue]||[recipe[@"mode"]integerValue]<0||[recipe[@"mode"]integerValue]>=HP_MODE_COUNT)goto invalid;
    if([recipe[@"journey"]doubleValue]!=[recipe[@"journey"]integerValue]||[recipe[@"journey"]integerValue]<-1||[recipe[@"journey"]integerValue]>=HP_JOURNEY_COUNT)goto invalid;
    if([recipe[@"brightness"]doubleValue]<.0799||[recipe[@"brightness"]doubleValue]>1||[recipe[@"motion"]doubleValue]<.1499||[recipe[@"motion"]doubleValue]>1||[recipe[@"depth"]doubleValue]<.25||[recipe[@"depth"]doubleValue]>1)goto invalid;
    if([recipe[@"variation"]doubleValue]<0||[recipe[@"variation"]doubleValue]>UINT32_MAX||[recipe[@"variation"]doubleValue]!=[recipe[@"variation"]unsignedIntValue])goto invalid;
    for(NSString *key in @[@"phase",@"journeyElapsed"])if([recipe[key]doubleValue]<0||[recipe[key]doubleValue]>1e9)goto invalid;
    return [recipe dictionaryWithValuesForKeys:@[@"id",@"name",@"scope",@"mode",@"journey",@"brightness",@"motion",@"depth",@"variation",@"phase",@"journeyElapsed"]];
invalid:
    if(error)*error=[NSError errorWithDomain:@"HP.StudioPresets" code:1 userInfo:@{NSLocalizedDescriptionKey:@"This moving mood has invalid settings."}];
    return nil;
}
+ (NSArray *)validRecipes:(id)recipes {
    if(![recipes isKindOfClass:NSArray.class])return @[];
    NSMutableArray *result=[NSMutableArray new];NSMutableSet *seen=[NSMutableSet new];
    for(id item in recipes){NSDictionary *recipe=[self validatedRecipe:item error:nil];if(recipe&&![seen containsObject:recipe[@"id"]]){[result addObject:recipe];[seen addObject:recipe[@"id"]];}}
    return result;
}
@end
