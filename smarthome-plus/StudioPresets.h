#import <Foundation/Foundation.h>
@interface HPStudioPresets : NSObject
+ (NSDictionary *)validatedRecipe:(NSDictionary *)recipe error:(NSError **)error;
+ (NSArray *)validRecipes:(id)recipes;
@end
