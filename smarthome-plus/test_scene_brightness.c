#include "SceneBrightness.h"
#include <assert.h>
#include <stdio.h>
int main(void){for(int span=1;span<=100;span++)for(int value=0;value<=100;value++){int saved=HPSceneStoredBrightness(value,span);assert(saved*span/100==value);}puts("All 10,100 native scene brightness round trips passed.");}
