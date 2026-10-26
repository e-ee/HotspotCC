#ifndef LS_BEAT_CORE_H
#define LS_BEAT_CORE_H
#include <math.h>
#include <stdbool.h>
#include <string.h>
typedef struct {float level, bass, mid, high, beat, hue;float peakRMS,lastBass,lastRMS,meanFlux,meanBass;double lastTime,lastBeat;unsigned beats;} LSBeatState;
static inline void LSBeatInit(LSBeatState *s){memset(s,0,sizeof(*s));s->peakRMS=0.03f;s->hue=0.57f;s->lastBeat=-10;}
static inline bool LSBeatUpdate(LSBeatState *s,const float bins[64],float rms,double now,bool silence){
 double dt=s->lastTime>0?fmin(0.25,fmax(0.001,now-s->lastTime)):1.0/30;s->lastTime=now;
 float bass=0,mid=0,high=0;
 for(int i=0;i<64;i++){float v=isfinite(bins[i])?fminf(1,fmaxf(0,bins[i])):0;if(i<18)bass+=v/18;else if(i<44)mid+=v/26;else high+=v/20;}
 rms=isfinite(rms)?fmaxf(0,rms):0;if(silence||rms<0.0005f){bass=mid=high=rms=0;}
 s->peakRMS=fmaxf(0.012f,fmaxf(rms,s->peakRMS*expf(-dt/4.0)));
 float energy=fminf(1,sqrtf(rms/s->peakRMS));
 float flux=fmaxf(0,bass-s->lastBass)+0.35f*fmaxf(0,energy-s->lastRMS);
 float threshold=fmaxf(0.018f,s->meanFlux*1.55f);
 bool onset=rms>0.001f&&flux>threshold&&bass>0.025f&&now-s->lastBeat>0.18;
 s->meanFlux=s->meanFlux*0.94f+flux*0.06f;s->meanBass=s->meanBass*0.94f+bass*0.06f;
 s->lastBass=bass;s->lastRMS=energy;
 s->beat*=expf(-dt/0.16);
 if(onset){s->beat=1;s->lastBeat=now;s->beats++;s->hue=fmodf(s->hue+0.083f+high*0.16f,1);}
 float target=rms>0?fminf(1,0.03f+0.58f*energy+0.39f*s->beat):0.025f;
 float coeff=1-expf(-dt/(target>s->level?0.025f:0.15f));s->level+=coeff*(target-s->level);
 s->bass=bass;s->mid=mid;s->high=high;return onset;
}
#endif
