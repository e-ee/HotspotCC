#include "EffectPatterns.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(void){
    for(int mode=0;mode<HP_MODE_COUNT;mode++){
        double movement=0,lo=1,hi=0;HPColor prior=HPPattern(mode,0,0,.6,1);
        for(int frame=0;frame<6000;frame++){
            HPColor c=HPPattern(mode,frame*.1,0,.6,1);
            assert(isfinite(c.hue)&&isfinite(c.saturation)&&isfinite(c.lightness));
            assert(c.hue>=0&&c.hue<=1&&c.saturation<=((mode==0||mode>=24)?.961:.65)&&c.lightness<=.34&&c.lightness>=.27);
            assert(fabs(c.hue-prior.hue)<.002&&fabs(c.lightness-prior.lightness)<.001);
            movement+=fabs(c.hue-prior.hue)+fabs(c.lightness-prior.lightness);prior=c;
            lo=fmin(lo,c.lightness);hi=fmax(hi,c.lightness);
        }
        assert(movement>.12&&hi-lo<.061); // Low contrast but still moving.
        HPColor state={.92,.9,.85};HPWireColor previous=HPVendorColor(state);
        for(int frame=0;frame<3000;frame++){
            int theme=(frame/37)%HP_MODE_COUNT;double dt=frame==80?500:1.4;
            HPColor target=HPPattern(theme,frame*1.4,.4,.6,.5),next=HPDrift(state,target,dt);
            double allowed=fmin(dt,2);
            assert(fabs(next.hue-state.hue)<=allowed/360.+.000001);
            assert(fabs(next.saturation-state.saturation)<=allowed*.012+.000001);
            assert(fabs(next.lightness-state.lightness)<=allowed*.008+.000001);
            HPWireColor wire=HPVendorColor(next);
            assert(abs((int)wire.hue-previous.hue)<=3&&abs((int)wire.saturation-previous.saturation)<=3&&abs((int)wire.lightness-previous.lightness)<=2);
            state=next;previous=wire;
        }
        HPColor invalid=HPPattern(mode,NAN,NAN,NAN,NAN);assert(isfinite(invalid.lightness));
        double spatial=0;for(int n=0;n<20;n++){
            HPColor a=HPPattern2D(mode,n*31.,.1,.2,.6,1),b=HPPattern2D(mode,n*31.,.8,.7,.6,1),near=HPPattern2D(mode,n*31.,.101,.201,.6,1),later=HPPattern2D(mode,n*31.+.1,.1,.2,.6,1);
            spatial+=fabs(a.hue-b.hue)+fabs(a.lightness-b.lightness);
            assert(fabs(a.hue-near.hue)<.004&&fabs(a.lightness-near.lightness)<.001);
            assert(fabs(a.hue-later.hue)<.002&&fabs(a.lightness-later.lightness)<.001);
            assert(a.lightness>.27&&a.lightness<.34);
        }assert(spatial>.03);
    }
    for(int journey=0;journey<HP_JOURNEY_COUNT;journey++){
        HPColor before=HPStudioTarget(0,journey,0,0,.3,.7,.6,1,0);double movement=0;
        for(int n=1;n<40000;n++){
            double seconds=n*.1;HPColor next=HPStudioTarget(0,journey,seconds,seconds*.5,.3,.7,.6,1,0);
            assert(fabs(before.hue-next.hue)<.002&&fabs(before.lightness-next.lightness)<.001);
            assert(next.lightness>=.27&&next.lightness<=.34);movement+=fabs(next.hue-before.hue);before=next;
        }assert(movement>.3);
        for(int boundary=1;boundary<=12;boundary++){
            double t=boundary*360.;HPColor a=HPStudioTarget(0,journey,t-.001,100,.2,.8,.6,1,0),b=HPStudioTarget(0,journey,t+.001,100,.2,.8,.6,1,0);assert(fabs(a.hue-b.hue)<.00001&&fabs(a.lightness-b.lightness)<.00001);
        }
    }
    HPColor base=HPStudioTarget(0,-1,0,143,.2,.8,.6,1,0),soft=HPStudioTarget(0,-1,0,143,.2,.8,.6,.3,0);assert(base.hue==soft.hue&&base.lightness==soft.lightness&&soft.saturation<.4&&base.saturation==1);
    double diversity=0;for(uint32_t seed=1;seed<80;seed++){HPColor a=HPStudioTarget(0,-1,0,100,.2,.8,.6,.8,seed),b=HPStudioTarget(0,-1,0,100,.2,.8,.6,.8,seed),near=HPStudioTarget(0,-1,0,100,.201,.801,.6,.8,seed);assert(a.hue==b.hue&&a.saturation==b.saturation);assert(fabs(a.hue-near.hue)<.006);diversity+=fabs(a.hue-base.hue);}assert(diversity>.5);
    HPColor bad=HPStudioTarget(0,0,NAN,NAN,NAN,NAN,NAN,NAN,UINT32_MAX);assert(isfinite(bad.hue)&&isfinite(bad.saturation)&&isfinite(bad.lightness));
    for(int mode=0;mode<HP_MODE_COUNT;mode++){
        HPColor pale=HPStudioTarget(mode,-1,0,0,.3,.7,.6,.25,0),medium=HPStudioTarget(mode,-1,0,0,.3,.7,.6,.6,0),rich=HPStudioTarget(mode,-1,0,0,.3,.7,.6,1,0);
        assert(rich.saturation==1&&medium.saturation>.46&&pale.saturation<.25&&pale.saturation<medium.saturation);
        assert(rich.hue==pale.hue&&rich.lightness==pale.lightness);
    }
    assert(HPHeldDepth(.4,.5,1)==1&&HPHeldDepth(.4,.5,.25)==0&&fabs(HPHeldDepth(.4,.5,.5)-.4)<.000001);
    for(int n=0;n<20;n++){double position=n/20.,phase=n*17.;HPColor original=HPPattern(0,phase,position,.6,1);assert(fabs(original.hue-(.36+.42*HPWave(position*.8-phase/180.)))<.000001);assert(fabs(original.saturation-(.72+.24*HPWave(position-phase/240.)))<.000001);}
    double green=0;for(int n=0;n<100;n++){HPColor a=HPPattern2D(24,0,n/99.,.65,.6,1);if(a.hue*360>=108&&a.hue*360<=145&&a.saturation>.8)green++;}assert(green>20);
    assert(fabs(HPMotionRate(.15)-.2)<.000001&&fabs(HPMotionRate(1)-40)<.000001&&HPMotionRate(.9)>HPMotionRate(.5));
    HPColor prism=HPStudioTarget(5,-1,0,100,.3,.7,.6,1,0);double recolour=0;for(int seed=1;seed<30;seed++){HPColor shuffled=HPStudioTarget(5,-1,0,100,.3,.7,.6,1,seed);recolour+=fabs(shuffled.hue-prism.hue);assert(shuffled.lightness>=.27&&shuffled.lightness<=.34);}assert(recolour>1);
    HPWireColor blue=HPVendorColor((HPColor){240.f/360.f,.8f,.6f});assert(blue.hue==240&&blue.saturation==80&&blue.lightness==60);
    HPWireColor vendor=HPVendorColor(HPStatusColor(240,80,60));assert(vendor.hue==240&&vendor.saturation==80&&vendor.lightness==60);
    HPWireColor normalized=HPVendorColor(HPStatusColor(43690,52428,39321));assert(normalized.hue==240&&normalized.saturation==80&&normalized.lightness==60);
    assert(HPSmoothTransitionSteps(14)==40&&HPSmoothTransitionSteps(40)==60);
    assert(HPPlaybackRate(NAN)==1&&HPPlaybackRate(-1)==.25&&HPPlaybackRate(100)==10);
    assert(HPPlaybackTransitionSteps(14,1)==40&&HPPlaybackTransitionSteps(14,10)==4&&HPPlaybackTransitionSteps(14,.25)==(0x40|16));
    HPColor old={.1,.2,.3},target={.9,.8,.7};
    HPColor normal=HPPlaybackDrift(old,target,1,1),fast=HPPlaybackDrift(old,target,1,10),legacy=HPDrift(old,target,1),bounded=HPPlaybackDrift(old,target,500,10);
    assert(normal.hue==legacy.hue&&normal.saturation==legacy.saturation&&normal.lightness==legacy.lightness);
    assert(fast.hue-old.hue>9*(normal.hue-old.hue));
    assert(fabs(bounded.hue-old.hue)<=20./360.+.000001);
    // Every mood and journey must reach the same target in one tenth the wall time at 10x.
    for(int journey=-1;journey<HP_JOURNEY_COUNT;journey++)for(int mode=0;mode<HP_MODE_COUNT;mode++){
        double time=317.25;
        HPColor a=HPStudioTarget(mode,journey,time,time*HPMotionRate(.5),.3,.7,.6,.8,0);
        double accelerated=time/10*HPPlaybackRate(10);
        HPColor b=HPStudioTarget(mode,journey,accelerated,accelerated*HPMotionRate(.5),.3,.7,.6,.8,0);
        assert(fabs(a.hue-b.hue)<.000001&&fabs(a.lightness-b.lightness)<.000001);
    }
    puts("Thirty-two moods and six seamless journeys: 2D spatial fields, adjacent-light continuity, continuous motion, limited contrast, bounded hardware steps, no restart/catch-up jumps, vendor/normalized status conversion and four-second fades passed.");
}
