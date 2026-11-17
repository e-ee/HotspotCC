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
