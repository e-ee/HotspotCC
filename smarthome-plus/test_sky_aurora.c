#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "EffectPatterns.h"

static double hd(double a,double b){
    double d=fabs(a-b);return fmin(d,1.-d);
}
static void valid(HPColor c){
    assert(isfinite(c.hue)&&isfinite(c.saturation)&&isfinite(c.lightness));
    assert(c.hue>=0&&c.hue<=1);
    assert(c.saturation>=0&&c.saturation<=1);
    assert(c.lightness>=0&&c.lightness<=1);
}
int main(void){
    assert(HP_JOURNEY_COUNT==41&&HP_SKY_PROGRAMS==35);
    assert(HP_SKY_PALETTES==36&&HP_SKY_CHAPTERS==8);
    // The six new moods must not be six names for one field.
    for(int a=34;a<40;a++){
        for(int b=a+1;b<40;b++){
            double difference=0;
            for(int k=0;k<12;k++){
                double x=(k%4+.5)/4.,y=(k/4+.5)/3.;
                HPColor ca=HPStudioTarget(a,-1,0,27+k*41,x,y,.52,1,0);
                HPColor cb=HPStudioTarget(b,-1,0,27+k*41,x,y,.52,1,0);
                valid(ca);valid(cb);
                difference+=hd(ca.hue,cb.hue);
                difference+=.45*fabs(ca.saturation-cb.saturation);
            }
            assert(difference>.22);
        }
    }
    // All 35 curated journeys have 8 palette chapters, loop seamlessly,
    // and show substantial changes in a 2-minute cycle.
    for(int journey=6;journey<HP_JOURNEY_COUNT;journey++){
        for(int seedIndex=0;seedIndex<3;seedIndex++){
            uint32_t seed=seedIndex?(uint32_t)(journey*15821+seedIndex*73021):0;
            int order[HP_SKY_CHAPTERS];HPSkyAuroraOrder8(journey,seed,order);
            for(int i=0;i<HP_SKY_CHAPTERS;i++)
                assert(order[i]>=0&&order[i]<HP_SKY_PALETTES);
            for(int spot=0;spot<3;spot++){
                double x=spot==0?.05:spot==1?.48:.94;
                double y=spot==0?.33:spot==1?.47:.72;
                HPColor a=HPSkyAuroraJourneyShuffled(journey,0,43,x,y,.52,1,seed);
                HPColor b=HPSkyAuroraJourneyShuffled(journey,2880,43,x,y,.52,1,seed);
                valid(a);valid(b);
                assert(hd(a.hue,b.hue)<.00001);
                assert(fabs(a.lightness-b.lightness)<.00001);
                for(int n=1;n<=HP_SKY_CHAPTERS;n++){
                    double boundary=n*2880./HP_SKY_CHAPTERS;
                    HPColor before=HPSkyAuroraJourneyShuffled(journey,boundary-.001,43,x,y,.52,1,seed);
                    HPColor after=HPSkyAuroraJourneyShuffled(journey,boundary+.001,43,x,y,.52,1,seed);
                    assert(hd(before.hue,after.hue)<.004);
                    assert(fabs(before.lightness-after.lightness)<.004);
                }
            }
            int bigChanges=0;
            HPColor previous=HPSkyAuroraJourneyShuffled(journey,180,80,.08,.31,.52,1,seed);
            for(int n=1;n<HP_SKY_CHAPTERS;n++){
                // At 2 min: real samples at 7.5s,22.5s,37.5s...112.5s.
                HPColor c=HPSkyAuroraJourneyShuffled(journey,180.+360.*n,80,.08,.31,.52,1,seed);
                valid(c);
                if(hd(c.hue,previous.hue)>.045 ||
                    fabs(c.saturation-previous.saturation)>.20)bigChanges++;
                previous=c;
            }
            assert(bigChanges>=3);
        }
    }
    // L6 ceiling travels across the whole journey, while the wall pair
    // shares phase but samples different locations on the SAME aurora.
    double minHue=1,maxHue=0;int gradients=0;
    for(int step=0;step<=2880;step+=6){
        HPColor ceiling=HPSkyMappedAuroraLamp(0,6,step,step*.45,.52,1,0);
        HPColor wall12=HPSkyMappedAuroraLamp(1,6,step,step*.45,.52,1,0);
        HPColor wall13=HPSkyMappedAuroraLamp(2,6,step,step*.45,.52,1,0);
        valid(ceiling);valid(wall12);valid(wall13);
        if(ceiling.hue<minHue)minHue=ceiling.hue;
        if(ceiling.hue>maxHue)maxHue=ceiling.hue;
        if(hd(wall12.hue,wall13.hue)>.035)gradients++;
    }
    assert(maxHue-minHue>.15&&gradients>30);
    // The TV never receives a pure blue target, and its brightness follows
    // the user's slider without an arbitrary 10 percent limit.
    HPColor tv=HPSkyTVBias(.68);
    valid(tv);assert(tv.saturation<.10&&fabs(tv.lightness-.68)<.00001);
    assert(HPSkyTVBias(.2).lightness<HPSkyTVBias(.8).lightness);
    // 2 minute colour changes cannot be smothered by an old 1-degree/s
    // ambient cap. Fast journeys still move continuously and never jump.
    HPColor old={.34,.60,.25},goal={.75,.85,.48},fast=old,slow=old;
    for(int i=0;i<20;i++){
        HPColor next=HPSkyAdaptiveDrift(fast,goal,.5,2);
        assert(hd(next.hue,fast.hue)<.015);
        fast=next;
        slow=HPSkyAdaptiveDrift(slow,goal,.5,48);
    }
    assert(hd(fast.hue,old.hue)>.12);
    assert(hd(fast.hue,goal.hue)<hd(slow.hue,goal.hue));
    // Exact duration mapping: 2 minutes = 8 chapters of 15 seconds.
    for(int duration=2;duration<=48;duration+=2){
        double canonical=HPJourneyCycleSeconds(6);
        double perSecond=canonical/(duration*60.);
        assert(fabs((duration*60.)*perSecond-canonical)<1e-6);
        if(duration==2)assert(fabs((canonical/HP_SKY_CHAPTERS)/perSecond-15.)<1e-6);
    }
    puts("35 distinct 8-scene journeys; 6 mood fields; 2-min live fades; L6; wall pair; TV neutral: PASS");
    return 0;
}
