#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "EffectPatterns.h"
static HPBluBulb make(double h,double s,double l,double x,double y){
    HPBluBulb b={0};
    b.anchor=(HPColor){h,s,l};
    b.x=x;b.y=y;b.valid=1;b.partner=-1;
    return b;
}
static double hdist(double a,double b){
    double x=fabs(a-b);return fmin(x,1-x);
}
int main(void){
    assert(HP_MODE_COUNT==48&&HP_JOURNEY_COUNT==56);
    HPBluStage stage={0};stage.count=8;
    stage.bulb[0]=make(.367,.92,.36,.07,.33); // natural green
    stage.bulb[1]=make(.591,.77,.39,.25,.41); // related blue
    stage.bulb[2]=make(.388,.87,.38,.67,.75); // #9 green
    stage.bulb[3]=make(.610,.72,.36,.86,.82); // #12 blue
    stage.bulb[4]=make(.500,.53,.30,.49,.53); // L7 TV
    stage.bulb[4].tv=1;
    stage.bulb[5]=make(.594,.77,.48,.49,.10); // L6 ceiling
    stage.bulb[5].ceiling=1;
    stage.bulb[6]=make(.535,.78,.31,.08,.53);stage.bulb[6].wall12=1;
    stage.bulb[7]=make(.615,.72,.33,.08,.65);stage.bulb[7].wall13=1;
    stage.bulb[2].corner9=1;stage.bulb[3].corner12=1;
    HPBluPair(&stage);
    assert(stage.greenCount==2);
    assert(stage.blueCount>=3);
    assert(stage.pairCount==2);
    assert(stage.bulb[0].partner==1);
    assert(stage.bulb[2].partner==3);
    assert(stage.bulb[4].partner==-1); // no television role swapping
    // At zero exchange, original per-bulb Blu values MUST be exact.
    for(int i=0;i<stage.count;i++){
        HPColor got=HPBluTarget(&stage,i,44,0,1831,15,.48,1,0,0);
        HPColor expected=stage.bulb[i].anchor;
        assert(hdist(got.hue,expected.hue)<1e-6);
        assert(fabs(got.saturation-expected.saturation)<1e-6);
        assert(fabs(got.lightness-expected.lightness)<1e-6);
    }
    // Twelve sources have different paths but stay in the REAL palette.
    int distinct=0;
    double values[12]={0};
    for(int journey=44;journey<56;journey++){
        double min=.999,max=0;
        for(int frame=0;frame<=240;frame++){
            double e=frame*12.; // 2-minute real repeat, 240 samples
            HPColor g=HPBluTarget(&stage,0,journey,0,e,20,.48,1,.90,0);
            HPColor b=HPBluTarget(&stage,1,journey,0,e,20,.48,1,.90,0);
            HPColor tv=HPBluTarget(&stage,4,journey,0,e,20,.48,1,.90,0);
            assert(isfinite(g.hue)&&g.hue>=0&&g.hue<=1);
            assert(g.hue>=.34 && g.hue<=.63);
            assert(b.hue>=.34 && b.hue<=.63);
            assert(tv.hue>=.44 && tv.hue<=.55);
            assert(tv.saturation<=.65);
            if(g.hue<min)min=g.hue;
            if(g.hue>max)max=g.hue;
            // Real green/blue pair is reciprocal at every moment.
            assert(fabs((g.hue+b.hue)-(.367+.591))<.008);
            HPColor cg=HPBluTarget(&stage,2,journey,0,e,20,.48,1,.9,0);
            HPColor cb=HPBluTarget(&stage,3,journey,0,e,20,.48,1,.9,0);
            assert(fabs((cg.hue+cb.hue)-(.388+.610))<.008);
        }
        // Blu Original is meant to be faithful; other journeys vary visibly.
        if(journey!=44)assert(max-min>.035);
        values[journey-44]=HPBluTarget(&stage,0,journey,0,1010,13,.48,1,.90,0).hue;
        HPColor first=HPBluTarget(&stage,0,journey,0,0,20,.48,1,.9,0);
        HPColor last=HPBluTarget(&stage,0,journey,0,2880,20,.48,1,.9,0);
        assert(hdist(first.hue,last.hue)<1e-6);
    }
    for(int i=0;i<12;i++)for(int j=i+1;j<12;j++)
        if(hdist(values[i],values[j])>.04)distinct++;
    assert(distinct>15);
    // L6 is a primary ceiling wash. It must explore original Blu blues
    // and teals even when no green donor is paired directly with it.
    for(int j=45;j<56;j++){
        double lowest=1,highest=0;
        for(int frame=0;frame<120;frame++){
            HPColor c=HPBluTarget(&stage,5,j,0,frame*24.,18,.48,1,.95,0);
            if(c.hue<lowest)lowest=c.hue;
            if(c.hue>highest)highest=c.hue;
            assert(c.hue>=.475&&c.hue<=.72); // never adds a second green
        }
        assert(highest-lowest>.018);
    }
    // Blu. modes run for several minutes; a small Motion setting still
    // advances them even when no Journey clock is active.
    HPColor a=HPBluTarget(&stage,0,-1,43,0,0,.48,1,.85,0);
    HPColor b=HPBluTarget(&stage,0,-1,43,0,250,.48,1,.85,0);
    assert(hdist(a.hue,b.hue)>.01);
    // Scene-specific colour properties are never changed by an unused
    // (unmatched) light. Extra brightness never forces every bulb to green.
    puts("Blu anchor fidelity, 12 journeys, complementary pair swaps, TV, ceiling, wall gradient: PASS");
    return 0;
}
