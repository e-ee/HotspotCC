#include "EffectPatterns.h"
#include <assert.h>
#include <stdio.h>
#include <float.h>
int main(void){
    for(int mode=0;mode<8;mode++){
        double temporal=0,spatial=0;HPColor previous=HPPattern(mode,0,0,.6,1);
        for(int frame=0;frame<1200;frame++){
            HPColor first=HPPattern(mode,frame*.1,0,.6,1);
            for(int light=0;light<14;light++){
                HPColor c=HPPattern(mode,frame*.1,light/14.,.6,1);
                assert(isfinite(c.hue)&&isfinite(c.saturation)&&isfinite(c.lightness));
                assert(c.hue>=0&&c.hue<1&&c.saturation>=0&&c.saturation<=1&&c.lightness>=0&&c.lightness<=.60001);
                spatial+=fabs(c.lightness-first.lightness)+fabs(c.hue-first.hue);
            }
            temporal+=fabs(first.hue-previous.hue)+fabs(first.lightness-previous.lightness);previous=first;
        }
        assert(temporal>.1 && spatial>1); // Each pattern changes over time AND across lights.
        HPColor invalid=HPPattern(mode,NAN,NAN,NAN,NAN);assert(isfinite(invalid.lightness));
        HPColor repeat=HPPattern(mode,19.2,.4,.6,1),again=HPPattern(mode,19.2,.4,.6,1);
        assert(repeat.hue==again.hue&&repeat.lightness==again.lightness);
    }
    assert(HPWord(0)==0&&HPWord(1)==65535);
    puts("Eight patterns: finite bounded output, independent spatial motion, temporal variation and deterministic playback passed.");
}
