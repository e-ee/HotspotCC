#include <assert.h>
#include <stdio.h>
#include "BeatCore.h"
int main(void){
 LSBeatState s;LSBeatInit(&s);float bins[64]={0};
 for(int i=0;i<400;i++){for(int j=0;j<64;j++)bins[j]=0.4;LSBeatUpdate(&s,bins,0.08,10+i*0.025,false);}
 assert(s.beats<=2);assert(s.level>0.2&&s.level<=1);unsigned steady=s.beats;
 LSBeatInit(&s);
 for(int i=0;i<400;i++){bool kick=(i%20)<2;for(int j=0;j<64;j++)bins[j]=j<18?(kick?0.85:0.02):0.04;LSBeatUpdate(&s,bins,kick?0.25:0.008,10+i*0.025,false);}
 assert(s.beats>=18&&s.beats<=22);unsigned rhythmic=s.beats;
 for(int i=0;i<200;i++){for(int j=0;j<64;j++)bins[j]=0;LSBeatUpdate(&s,bins,0,20+i*0.025,true);}
 assert(s.level<0.03&&s.beat<0.001);
 unsigned before=s.beats;for(int j=0;j<64;j++)bins[j]=NAN;LSBeatUpdate(&s,bins,NAN,26,false);assert(isfinite(s.level)&&s.beats==before);
 printf("Beat tests: steady tone %u onsets; 20 synthetic kicks %u onsets; silence settles; NaN rejected.\n",steady,rhythmic);
}
