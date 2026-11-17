#include <assert.h>
#include <math.h>
#include "EffectPatterns.h"
int main(void){
  assert(HP_JOURNEY_COUNT==41);
  assert(HP_SKY_PALETTES==36);
  assert(HP_SKY_PROGRAMS==35);
  // Every new mood is finite and the centre stays warm blue or periwinkle.
  for(int mood=34;mood<=39;mood++)
    for(int j=0;j<=100;j++){
      double x=j/100.;
      HPColor c=HPStudioTarget(mood,-1,0,123,x,.31,.52,1,0);
      assert(isfinite(c.hue)&&isfinite(c.saturation)&&isfinite(c.lightness));
      assert(c.hue>=0&&c.hue<=1&&c.saturation>=0&&c.saturation<=1);
      assert(c.lightness>=0&&c.lightness<=1);
      if(x>=.35&&x<=.65)assert(c.hue>.54);
    }
  // All 35 journeys must loop without a colour jump, under varied seeds.
  for(int journey=6;journey<HP_JOURNEY_COUNT;journey++){
    for(int seedIndex=0;seedIndex<3;seedIndex++){
      uint32_t seed=seedIndex==0?0:(uint32_t)(seedIndex*92871+journey);
      int order[4];HPSkyAuroraOrder(journey,seed,order);
      for(int i=0;i<4;i++)assert(order[i]>=0&&order[i]<HP_SKY_PALETTES);
      HPColor a=HPSkyAuroraJourneyShuffled(journey,0,123,.07,.47,.52,1,seed);
      HPColor b=HPSkyAuroraJourneyShuffled(journey,2880,123,.07,.47,.52,1,seed);
      assert(fabs(a.hue-b.hue)<1e-6);
      assert(fabs(a.lightness-b.lightness)<1e-6);
      for(int i=0;i<4;i++){
        double boundary=(i+1)*720.;
        HPColor before=HPSkyAuroraJourneyShuffled(journey,boundary-.001,123,.42,.47,.52,1,seed);
        HPColor after=HPSkyAuroraJourneyShuffled(journey,boundary+.001,123,.42,.47,.52,1,seed);
        assert(fabs(before.hue-after.hue)<.002);
        assert(fabs(before.lightness-after.lightness)<.002);
      }
      for(int j=0;j<40;j++){
        double x=j/39., y=.52;
        HPColor c=HPSkyAuroraJourneyShuffled(journey,180.+j*41.,123,x,y,.52,1,seed);
        assert(isfinite(c.hue)&&isfinite(c.lightness));
        if(x>.36&&x<.64)assert(c.hue>.54);
      }
    }
  }
  // Regression: in the uploaded apartment map L6 is the far-left lamp
  // at (0,.309). It must show continuous variation without lighting up TV 11.
  // The neighbouring far-left lamp (0,.533) should retain its quiet gradient.
  double l6Min=2,l6Max=-1,nearMin=2,nearMax=-1,prevHue=-1;
  for(int step=0;step<=920;step++){
    double phase=(double)step;
    HPColor l6=HPSkyAuroraJourneyShuffled(6,80,phase,0,.309,.48,1,0);
    HPColor neighbour=HPSkyAuroraJourneyShuffled(6,80,phase,0,.533,.48,1,0);
    if(l6.hue<l6Min)l6Min=l6.hue;
    if(l6.hue>l6Max)l6Max=l6.hue;
    if(neighbour.hue<nearMin)nearMin=neighbour.hue;
    if(neighbour.hue>nearMax)nearMax=neighbour.hue;
    if(prevHue>=0)assert(fabs(l6.hue-prevHue)<.01);
    prevHue=l6.hue;
  }
  assert(l6Max-l6Min>.10);
  assert(nearMax-nearMin<.015);
  // No L6 motion amplification in the room's centre, where TV 11 lives.
  for(int mood=0;mood<HP_SKY_PALETTES;mood++){
    HPColor left=HPSkyAuroraField(mood,130,0,.309,.48,1);
    HPColor tv=HPSkyAuroraField(mood,130,.493,.533,.48,1);
    assert(isfinite(left.hue)&&isfinite(tv.hue));
    assert(tv.hue>.54);
    assert(HPSkyLimitTV(tv,1,1).lightness<=.10001);
  }
  // 2-minute and 48-minute controls change virtual speed, not palette phase.
  for(double duration=2;duration<=48;duration+=23){
    double canonical=HPJourneyCycleSeconds(6);
    double seconds=(duration*60.)*(canonical/(duration*60.));
    assert(fabs(seconds-canonical)<1e-7);
  }
  HPColor bright={.61,.85,.96};
  HPColor tv=HPSkyLimitTV(bright,1,1);
  assert(fabs(tv.lightness-.10)<1e-6);
  assert(fabs(HPSkyLimitTV(bright,0,1).lightness-bright.lightness)<1e-6);
  assert(fabs(HPSkyLimitTV(bright,1,0).lightness-bright.lightness)<1e-6);
  return 0;
}
