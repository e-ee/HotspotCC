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
            assert(c.hue>=0&&c.hue<=1&&c.saturation<=.65&&c.lightness<=.34&&c.lightness>=.27);
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
    HPWireColor blue=HPVendorColor((HPColor){240.f/360.f,.8f,.6f});assert(blue.hue==240&&blue.saturation==80&&blue.lightness==60);
    HPWireColor vendor=HPVendorColor(HPStatusColor(240,80,60));assert(vendor.hue==240&&vendor.saturation==80&&vendor.lightness==60);
    HPWireColor normalized=HPVendorColor(HPStatusColor(43690,52428,39321));assert(normalized.hue==240&&normalized.saturation==80&&normalized.lightness==60);
    assert(HPSmoothTransitionSteps(14)==40&&HPSmoothTransitionSteps(40)==60);
    puts("Twenty-four gentle palettes: 2D spatial fields, adjacent-light continuity, continuous motion, limited contrast, bounded hardware steps, no restart/catch-up jumps, vendor/normalized status conversion and four-second fades passed.");
}
