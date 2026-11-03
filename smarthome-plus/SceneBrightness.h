#ifndef HP_SCENE_BRIGHTNESS_H
#define HP_SCENE_BRIGHTNESS_H
// Native scene playback truncates percent * (maxBrightness-minBrightness)/100.
// Capture receives the already effective bulb level, so invert that mapping once.
static inline int HPSceneStoredBrightness(int actual,int span){if(span<1||span>100)span=100;return actual==0?0:(actual*100+span-1)/span;}
#endif
