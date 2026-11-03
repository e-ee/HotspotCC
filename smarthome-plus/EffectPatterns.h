#ifndef HP_EFFECT_PATTERNS_H
#define HP_EFFECT_PATTERNS_H
#include <math.h>
#include <stdint.h>
#define HP_MODE_COUNT 34
#define HP_JOURNEY_COUNT 6
typedef struct {float hue,saturation,lightness;} HPColor;
typedef struct {uint16_t hue,saturation,lightness;} HPWireColor;
static inline double HPClamp(double x,double lo,double hi){return fmin(hi,fmax(lo,x));}
// A curved control preserves gentle lower settings and reaches the requested 10x.
static inline double HPMotionRate(double value){double x=HPClamp((isfinite(value)?value:.5)-.15,0,.85)/.85;return .2+9.8*x*x*x;}
static inline double HPWrap(double x){return x-floor(x);}
static inline double HPWave(double x){return .5+.5*sin(x*6.283185307179586);}
// Two broad, overlapping fields keep neighbouring lights related while avoiding
// a mechanical rainbow chase. Palette limits never cross the hue seam.
static inline HPColor HPNaturalField(int mode,double seconds,double x,double y,double brightness,double speed){
    double t=seconds*speed;
    double a=HPWave(.58*x+.24*y-t/260.);
    double b=HPWave(-.21*x+.63*y+t/330.);
    double blend=.68*a+.32*b,h=.5,s=.4;
    switch(mode){
        case 16:h=.28+.16*blend;s=.30+.18*b;break; // Misty forest: sage to eucalyptus.
        case 17:h=.48+.15*blend;s=.42+.18*b;break; // Lagoon: turquoise to blue.
        case 18:h=.69+.12*blend;s=.30+.17*b;break; // Heather: blue-violet to lilac.
        case 19:h=.49+.29*blend;s=.16+.18*b;break; // Opal: pearl, sea glass and lavender.
        case 20:h=.59+.13*blend;s=.36+.20*b;break; // Blue hour: sky to soft indigo.
        case 21:h=.42+.15*blend;s=.28+.22*b;break; // Coast: sea glass and pale blue.
        case 22:h=.86+.08*blend;s=.24+.21*b;break; // Rose dusk: dusty rose and mauve.
        default:h=.39+.29*blend;s=.24+.23*b;break; // Polar mist: jade, ice blue, lavender.
    }
    return (HPColor){(float)h,(float)s,(float)((.50+.035*(.55*a+.45*b))*brightness)};
}
static inline HPColor HPNewField(int mode,double seconds,double x,double y,double brightness,double speed){
    double t=seconds*speed,a=HPWave(.62*x+.18*y-t/240.),b=HPWave(-.24*x+.66*y+t/310.),mix=.6*a+.4*b,h=.5,s=.7,l=.51;
    switch(mode){
        case 24:h=.29+.20*pow(mix,1.5);s=.80+.12*b;l=.49+.05*a;break;
        case 25:{double flow=HPWave(y*.82+.10*sin(x*5-t/160.)-t/220.);h=.46+.20*flow;s=.55+.2*b;l=.49+.045*flow;break;}
        case 26:{double petals=HPWave(.47*x-.31*y+t/280.)*.7+.3*b;h=.77+.17*petals;s=.36+.24*a;l=.50+.04*b;break;}
        case 27:h=.025+.10*HPWave(.54*x+.42*y-t/260.);s=.63+.15*b;l=.49+.05*a;break;
        case 28:{double cloud=.55*a+.25*b+.20*HPWave(.9*x-.8*y+t/390.);h=.58+.30*cloud;s=.52+.2*a;l=.48+.06*cloud;break;}
        case 29:h=.37+.22*(.45*a+.55*b);s=.40+.24*a;l=.50+.035*b;break;
        case 30:{double cx=.5+.3*sin(t/170.),cy=.5+.3*cos(t/210.),glow=exp(-6*((x-cx)*(x-cx)+(y-cy)*(y-cy)));double warmth=.55*glow+.45*a;h=.055+.06*warmth;s=.52+.14*b;l=.49+.05*warmth;break;}
        default:{double angle=t/310.,u=(x-.5)*cos(angle)+(y-.5)*sin(angle),v=(y-.5)*cos(angle)-(x-.5)*sin(angle);double petals=.5+.5*cos(u*5)*cos(v*5);h=.07+.79*petals;s=.60+.12*a;l=.50+.04*b;break;}
    }return (HPColor){h,s,l*brightness};
}
// Smooth periodic palettes: no on/off pulses, sharp heads or hue wrap.
static inline HPColor HPPattern(int mode,double seconds,double position,double brightness,double speed){
    seconds=isfinite(seconds)?seconds:0;position=isfinite(position)?HPWrap(position):0;
    brightness=isfinite(brightness)?HPClamp(brightness,.04,1):.6;
    speed=isfinite(speed)?HPClamp(speed,.15,1):.5;
    if(mode>=24)return HPNewField(mode,seconds,position,.5,brightness,speed);
    if(mode>=16)return HPNaturalField(mode,seconds,position,.5,brightness,speed);
    double t=seconds*speed,h=.6,s=.5,l=.52;
    switch(mode){
        case 0:{double ribbon=HPWave(position*.8-t/180.);h=.36+.42*ribbon;s=.72+.24*HPWave(position-t/240.);l=.49+.055*HPWave(position-t/160.);break;}
        case 1:{double head=HPWave(position-t/90.);head=head*head;h=.52+.12*(1-head);s=.48+.10*(1-head);l=.47+.09*head;break;}
        case 2:{double swell=HPWave(position*.8-t/200.);h=.49+.105*swell;s=.46+.08*HPWave(position+t/210.);l=.49+.045*swell;break;}
        case 3:{double glow=.5+.32*sin(t*.035+position*5)+.18*sin(t*.019+position*9);h=.055+.03*glow;s=.52+.07*glow;l=.49+.045*glow;break;}
        case 4:{double lane=HPWave(position*.65-t/210.);h=.51+.32*lane;s=.48+.08*lane;l=.50+.035*HPWave(position+t/175.);break;}
        case 5:{h=.08+.77*HPWave(position*.7+t/300.);s=.48;l=.51+.035*HPWave(position-t/220.);break;}
        case 6:{h=.085+.018*HPWave(position*.3+t/210.);s=.34;l=.49+.045*HPWave(t/70.-position*.2);break;}
        case 7:{double ribbon=HPWave(position*.6-t/240.);h=.70+.13*ribbon;s=.43+.12*ribbon;l=.49+.045*HPWave(position-t/190.);break;}
        case 8:{double glow=HPWave(position*.6-t/240.);h=.035+.065*glow;s=.45+.10*glow;l=.50+.04*glow;break;}
        case 9:{double glow=HPWave(position*.5+t/220.);h=.61+.015*glow;s=.12+.08*glow;l=.52+.025*glow;break;}
        case 10:{double glow=HPWave(position-t/170.);h=.10+.02*glow;s=.47;l=.49+.05*glow;break;}
        case 11:{double wave=HPWave(position*.8-t/190.);h=.48+.13*wave;s=.48+.05*wave;l=.50+.04*wave;break;}
        case 12:{double fold=HPWave(position*.7+t/260.);h=.73+.15*fold;s=.34+.10*fold;l=.51+.03*fold;break;}
        case 13:{double rain=HPWave(position*.9-t/180.);h=.55+.10*rain;s=.32+.10*rain;l=.50+.03*rain;break;}
        case 14:{double dawn=HPWave(position*.65-t/280.);h=.04+.06*dawn;s=.32+.12*dawn;l=.50+.045*dawn;break;}
        default:{double orbit=HPWave(position-t/210.);h=.48+.28*orbit;s=.42+.10*orbit;l=.50+.04*orbit;break;}
    }
    return (HPColor){(float)h,(float)s,(float)HPClamp(l*brightness,0,1)};
}
static inline HPColor HPPattern2D(int mode,double seconds,double x,double y,double brightness,double speed){
    x=isfinite(x)?HPClamp(x,0,1):.5;y=isfinite(y)?HPClamp(y,0,1):.5;
    seconds=isfinite(seconds)?seconds:0;brightness=isfinite(brightness)?HPClamp(brightness,.04,1):.6;speed=isfinite(speed)?HPClamp(speed,.15,1):.5;
    if(mode>=24)return HPNewField(mode,seconds,x,y,brightness,speed);
    if(mode>=16)return HPNaturalField(mode,seconds,x,y,brightness,speed);
    double position=.65*x+.35*y;
    if(mode==2||mode==13||mode==14)position=.18*x+.82*y;
    if(mode==4||mode==12)position=.65*x-.35*y+.35;
    if(mode==6||mode==11)position=hypot(x-.5,y-.5)/.7071067812;
    if(mode==1||mode==10||mode==15){
        double time=(isfinite(seconds)?seconds:0)*HPClamp(isfinite(speed)?speed:.5,.15,1);
        double angle=time*6.28318530718/(mode==1?150.:240.);
        double cx=.5+.35*cos(angle),cy=.5+.35*sin(angle),distance=hypot(x-cx,y-cy);
        double glow=exp(-distance*distance/.15),level=(mode==1?.48:.49)+.055*glow;
        if(mode==10)return (HPColor){(float)(.10+.02*glow),.47,(float)(level*HPClamp(brightness,.04,1))};
        if(mode==1)return (HPColor){(float)(.52+.12*(1-glow)),(float)(.48+.1*(1-glow)),(float)(level*HPClamp(brightness,.04,1))};
        return (HPColor){(float)(.48+.28*HPWave(distance*.8-time/210.)),(float)(.42+.1*glow),(float)(level*HPClamp(brightness,.04,1))};
    }
    return HPPattern(mode,seconds,position,brightness,speed);
}
static inline HPWireColor HPVendorColor(HPColor color){return (HPWireColor){(uint16_t)lround(HPClamp(color.hue,0,1)*360),(uint16_t)lround(HPClamp(color.saturation,0,1)*100),(uint16_t)lround(HPClamp(color.lightness,0,1)*100)};}
// The slider spans pale colour to full saturation, rather than merely scaling
// an already muted palette. Hue and brightness are untouched.
static inline double HPColourDepth(double saturation,double depth){
    double x=(HPClamp(isfinite(depth)?depth:1,.25,1)-.25)/.75;
    return HPClamp(HPClamp(saturation,0,1)*.25*(1-x)+x,0,1);
}
static inline double HPHeldDepth(double saturation,double startingDepth,double depth){
    saturation=HPClamp(saturation,0,1);startingDepth=HPClamp(startingDepth,.25,1);depth=HPClamp(depth,.25,1);
    if(depth>=startingDepth)return startingDepth>=.999999?saturation:saturation+(1-saturation)*(depth-startingDepth)/(1-startingDepth);
    return startingDepth<=.250001?saturation:saturation*(depth-.25)/(startingDepth-.25);
}
// Each chapter lasts six active minutes: four to settle, two to dissolve.
// Related palettes interpolate without crossing the hue seam. The next chapter
// uses the same field phase, so every boundary (including the loop) is continuous.
static inline HPColor HPStudioTarget(int mode,int journey,double journeySeconds,double phase,double x,double y,double brightness,double depth,uint32_t variation){
    phase=isfinite(phase)?phase:0;
    x=isfinite(x)?HPClamp(x,0,1):.5;y=isfinite(y)?HPClamp(y,0,1):.5;
    if(variation){
        double originalX=x;
        switch(variation%4){case 1:x=1-y;y=originalX;break;case 2:x=1-x;y=1-y;break;case 3:x=y;y=1-originalX;break;default:break;}
        if(variation&4)x=1-x;
        phase+=(variation%997)*.73;
    }
    HPColor result;
    if(journey>=0&&journey<HP_JOURNEY_COUNT){
        static const int chapters[HP_JOURNEY_COUNT][6]={{0,23,18,19,20,0},{21,17,2,11,9,23},{14,8,3,6,10,14},{16,24,29,21,25,16},{26,18,28,7,20,9},{14,27,8,6,30,3}};
        static const int counts[HP_JOURNEY_COUNT]={5,6,5,5,6,6};
        double time=isfinite(journeySeconds)?fmax(0,journeySeconds):0;
        time=fmod(time,counts[journey]*360.);
        int chapter=(int)(time/360.);double mix=HPClamp((fmod(time,360.)-240.)/120.,0,1);mix=mix*mix*(3-2*mix);
        HPColor a=HPPattern2D(chapters[journey][chapter],phase,x,y,brightness,1);
        HPColor b=HPPattern2D(chapters[journey][(chapter+1)%counts[journey]],phase,x,y,brightness,1);
        result=(HPColor){a.hue+(b.hue-a.hue)*mix,a.saturation+(b.saturation-a.saturation)*mix,a.lightness+(b.lightness-a.lightness)*mix};
    }else result=HPPattern2D(mode,phase,x,y,brightness,1);
    if(variation){
        unsigned style=(variation>>3)%4;
        if(mode==5||mode==31||style>=2){
            static const double ranges[8][2]={{.04,.94},{.34,.75},{.04,.16},{.72,.94},{.43,.65},{.16,.52},{.04,.32},{.55,.81}};
            unsigned palette=(variation/31)%8;double pigment=HPWave(result.hue*1.6+(variation%251)/251.);
            result.hue=ranges[palette][0]+(ranges[palette][1]-ranges[palette][0])*pigment;
        }else if(style==1)result.hue=1-result.hue;
    }
    result.saturation=HPColourDepth(result.saturation,depth);
    return result;
}
// Older firmware may report normalized status. Command units stay degrees/percent.
static inline HPColor HPStatusColor(uint16_t hue,uint16_t sat,uint16_t light){
    if(hue>360||sat>100||light>100)return (HPColor){hue/65535.f,sat/65535.f,light/65535.f};
    return (HPColor){hue/360.f,sat/100.f,light/100.f};
}
static inline double HPDriftComponent(double old,double target,double dt,double rate){
    if(!isfinite(old))old=target;
    if(!isfinite(target))target=old;
    double change=(target-old)*(-expm1(-dt/7.));
    return HPClamp(old+HPClamp(change,-rate*dt,rate*dt),0,1);
}
// Small adjacent steps also protect bulbs that ignore optional transition fields.
// Bound elapsed time after suspension/congestion rather than jumping to catch up.
static inline HPColor HPDrift(HPColor old,HPColor target,double elapsed){
    double dt=isfinite(elapsed)?HPClamp(elapsed,0,2):0;
    return (HPColor){(float)HPDriftComponent(old.hue,target.hue,dt,1./360.),(float)HPDriftComponent(old.saturation,target.saturation,dt,.012),(float)HPDriftComponent(old.lightness,target.lightness,dt,.008)};
}
static inline uint8_t HPSmoothTransitionSteps(unsigned count){return (uint8_t)HPClamp(ceil(count*2.2),40,60);}
#endif

