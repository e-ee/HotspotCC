#import <Foundation/Foundation.h>
#import "SceneTransfer.h"
#include <assert.h>

static NSDictionary *scene(NSString *uuid) {
    return @{@"uuid":uuid,@"routineName":@"Evening",@"color":@3,@"routineList":@[
        @{@"uuid":@"light-1",@"deviceName":@"TV light",@"color":@300,@"saturation":@80,
          @"lightness":@60,@"onOff":@YES,@"cct":@2700,@"isGroup":@NO,@"deviceType":@1}]};
}
static NSMutableDictionary *document(NSUInteger count) {
    NSMutableArray *scenes=[NSMutableArray new];
    for(NSUInteger n=0;n<count;n++)[scenes addObject:scene([NSString stringWithFormat:@"scene-%lu",(unsigned long)n])];
    return [@{@"format":@"sylvania-plus-scenes",@"version":@1,@"meshUUID":@"home-1",@"routines":scenes} mutableCopy];
}
int main(void) { @autoreleasepool {
    NSError *error;
    NSArray *native=@[@{@"uuid":@"a"},@{@"uuid":@"b"},@{@"uuid":@"c"},@{@"uuid":@"new"}];
    NSArray *ordered=[HPSceneTransfer orderedRecords:native identifiers:@[@"b",@"removed",@"a",@"b"] key:@"uuid"];
    assert([ordered[0][@"uuid"]isEqual:@"b"]&&[ordered[1][@"uuid"]isEqual:@"a"]&&[ordered[2][@"uuid"]isEqual:@"c"]&&[ordered[3][@"uuid"]isEqual:@"new"]);
    assert([[HPSceneTransfer orderedRecords:native identifiers:@[] key:@"uuid"]isEqual:native]);
    for(NSNumber *count in @[@0,@9,@10,@11,@100,@1000]) {
        NSDictionary *raw=document(count.unsignedIntegerValue);
        NSData *data=[NSJSONSerialization dataWithJSONObject:raw options:0 error:&error];assert(data);
        id decoded=[NSJSONSerialization JSONObjectWithData:data options:0 error:&error];
        NSDictionary *valid=[HPSceneTransfer validatedDocument:decoded meshUUID:@"home-1" error:&error];
        assert(valid && [valid[@"routines"] count]==count.unsignedIntegerValue);
    }
    assert(![HPSceneTransfer validatedDocument:document(11) meshUUID:@"other-home" error:&error]);
    // Regression: existing scene-card ARGB colours were incorrectly limited to UInt16.
    for(NSNumber *color in @[@0xFF79559E,@(-8825442),@0xFFFFFFFF]) {
        NSMutableDictionary *card=[scene(@"existing-scene") mutableCopy];card[@"color"]=color;
        NSMutableDictionary *existing=document(0);existing[@"routines"]=@[card];
        NSDictionary *valid=[HPSceneTransfer validatedDocument:existing meshUUID:@"home-1" error:&error];
        assert(valid && [valid[@"routines"][0][@"color"] isEqual:color]);
    }
    NSMutableDictionary *bad=document(1);bad[@"routines"]=@[@1];
    assert(![HPSceneTransfer validatedDocument:bad meshUUID:@"home-1" error:&error]);
    bad=document(1);bad[@"routines"]=@[scene(@"duplicate"),scene(@"duplicate")];
    assert(![HPSceneTransfer validatedDocument:bad meshUUID:@"home-1" error:&error]);
    bad=document(1);bad[@"format"]=@"other-format";
    assert(![HPSceneTransfer validatedDocument:bad meshUUID:@"home-1" error:&error]);
    NSMutableDictionary *command=[scene(@"x")[@"routineList"][0] mutableCopy];command[@"lightness"]=@(-1);
    NSMutableDictionary *item=[scene(@"x") mutableCopy];item[@"routineList"]=@[command];
    bad=document(0);bad[@"routines"]=@[item];
    assert(![HPSceneTransfer validatedDocument:bad meshUUID:@"home-1" error:&error]);
    command[@"lightness"]=@60;command[@"onOff"]=@2;
    assert(![HPSceneTransfer validatedDocument:bad meshUUID:@"home-1" error:&error]);
    puts("Scene transfer: 0/9/10/11/100/1000 round trips pass; wrong network, invalid fields, duplicate IDs and invalid switches rejected.");
} return 0; }
