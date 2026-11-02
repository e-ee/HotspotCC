#ifndef HP_EFFECT_PATTERNS_H
#define HP_EFFECT_PATTERNS_H
#include <math.h>
#include <stdint.h>

typedef struct { float hue,saturation,lightness; } HPColor;
static inline double HPClamp(double x,double lo,double hi){return fmin(hi,fmax(lo,x));}
static inline double HPWrap(double x){return x-floor(x);}
static inline double HPWave(double x){return .5+.5*sin(x*6.283185307179586);}
// Stateless, deterministic patterns. Position follows the user-configurable light path.
static inline HPColor HPPattern(int mode,double seconds,double position,double brightness,double speed){
    seconds=isfinite(seconds)?seconds:0;position=isfinite(position)?HPWrap(position):0;
    brightness=isfinite(brightness)?HPClamp(brightness,.04,1):.5;
    speed=isfinite(speed)?HPClamp(speed,.15,2):1;
    double t=seconds*speed,h=.6,s=.9,l=.5;
    switch(mode){
        case 0: { // Aurora: ribbons of jade, blue and violet with passing highlights.
            double ribbon=HPWave(position*.8-t*.025);
            h=.36+.42*ribbon;s=.72+.24*HPWave(position-t*.014);
            l=.16+.62*HPWave(position*1.5-t*.045);break;
        }
        case 1: { // Comet: a pearl head, cyan body and violet tail.
            double behind=HPWrap(t/12-position),head=exp(-pow(behind/.12,2));
            h=HPWrap(.5+.25*HPClamp(behind/.38,0,1));s=.88-.62*head;
            l=.035+.94*exp(-behind*7);break;
        }
        case 2: { // Ocean: overlapping teal swells with blue-white foam.
            double swell=HPWave(position*1.2-t*.035),foam=pow(HPWave(position*2+t*.019),5);
            h=.48+.14*swell;s=.9-.65*foam;l=.18+.48*swell+.2*foam;break;
        }
        case 3: { // Embers: independent slow ember motion; no on/off strobe.
            double f=.48+.26*sin(t*2.3+position*31)+.15*sin(t*4.9+position*77);
            h=.012+.065*HPClamp(f,0,1);s=.86+.1*HPWave(position+t*.06);
            l=.13+.64*HPClamp(f,0,1);break;
        }
        case 4: { // Neon: pink/cyan lanes sweeping in opposite directions.
            double lane=HPWave(position-t*.065);
            h=.5+.38*lane;s=.94;l=.16+.68*HPWave(position*2+t*.055);break;
        }
        case 5: { // Prism: a full-spectrum travelling colour wave.
            h=HPWrap(position*.83+t*.025);s=.84;
            l=.24+.56*HPWave(position-t*.045);break;
        }
        case 6: { // Breathe: warm, offset breaths drifting through the home.
            h=.07+.025*HPWave(position+t*.012);s=.38;
            l=.12+.64*HPWave(t/11-position*.32);break;
        }
        default: { // Velvet: warm gold highlights moving through plum and indigo.
            double gold=pow(HPWave(position-t*.018),7);
            h=HPWrap(.73+.15*HPWave(position*.6+t*.015)+gold*.22);
            s=.62+.3*(1-gold);l=.12+.35*HPWave(position-t*.028)+.25*gold;break;
        }
    }
    return (HPColor){(float)HPWrap(h),(float)HPClamp(s,0,1),(float)HPClamp(l*brightness,.008,1)};
}
static inline uint16_t HPWord(float value){return (uint16_t)lround(HPClamp(value,0,1)*65535);}
#endif
