#import "StudioPresets.h"
#include <assert.h>
#include <math.h>
int main(void){@autoreleasepool{
    NSDictionary *recipe=@{@"id":NSUUID.UUID.UUIDString,@"name":@"Quiet northern evening",@"scope":@"All lights",@"mode":@0,@"journey":@0,@"brightness":@.6,@"motion":@.35,@"depth":@.5,@"variation":@874,@"phase":@123.4,@"journeyElapsed":@341.2};
    assert([[HPStudioPresets validatedRecipe:recipe error:nil]isEqual:recipe]);
    NSData *data=[NSPropertyListSerialization dataWithPropertyList:@[recipe] format:NSPropertyListBinaryFormat_v1_0 options:0 error:nil];
    NSArray *copy=[NSPropertyListSerialization propertyListWithData:data options:0 format:NULL error:nil];assert([[HPStudioPresets validRecipes:copy]isEqual:@[recipe]]);
    for(NSString *key in @[@"mode",@"journey",@"brightness",@"motion",@"depth",@"variation",@"phase",@"journeyElapsed"]){NSMutableDictionary *bad=[recipe mutableCopy];bad[key]=@(NAN);assert(![HPStudioPresets validatedRecipe:bad error:nil]);}
    NSArray *badKeys=@[@"mode",@"mode",@"journey",@"brightness",@"motion",@"depth",@"variation",@"phase",@"name",@"scope",@"id"];
    NSArray *badValues=@[@24,@.5,@3,@2,@0,@0,@(-1),@(-1),@" ",@[],@"not-an-id"];
    for(NSUInteger n=0;n<badKeys.count;n++){NSMutableDictionary *bad=[recipe mutableCopy];bad[badKeys[n]]=badValues[n];assert(![HPStudioPresets validatedRecipe:bad error:nil]);}
    assert(([HPStudioPresets validRecipes:@[recipe,recipe,@{},@"bad"]].count==1));
    assert([HPStudioPresets validRecipes:@"bad"].count==0);
    puts("Moving moods: round-trip fidelity, independent identity, malformed settings, finite values and duplicate protection passed.");
}}
