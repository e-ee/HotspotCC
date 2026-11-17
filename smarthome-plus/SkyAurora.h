// Distinct after-dark lighting journeys with spatial gradients. Normalized
// map coordinates only: no controller secrets or UUIDs live in this file.
#ifndef HP_SKY_AURORA_H
#define HP_SKY_AURORA_H
#define HP_SKY_PALETTES 36
#define HP_SKY_PROGRAMS 35
#define HP_SKY_CHAPTERS 8

static inline double HPSkyMix(double a,double b,double t){
    return a+(b-a)*HPClamp(t,0,1);
}
static inline double HPSkyHueMix(double a,double b,double t){
    double delta=b-a;
    if(delta>.5)delta-=1.;
    else if(delta<-.5)delta+=1.;
    return HPWrap(a+delta*HPClamp(t,0,1));
}
static inline double HPSkyEase(double t){
    t=HPClamp(t,0,1);return t*t*(3.-2.*t);
}
static inline HPColor HPSkyAuroraField(int variant,double phase,double x,double y,double brightness,double depth){
    // Centre, left curtain, right curtain, saturation and output are
    // genuinely different for each colour. Green / teal / blue / violet /
    // pink and soft warm looks; no forced black, no piercing cobalt behind TV.
    static const double center[HP_SKY_PALETTES]={0.595,0.590,0.670,0.770,0.570,0.750,0.590,0.635,0.780,0.555,0.615,0.710,0.590,0.545,0.655,0.595,0.740,0.780,0.830,0.865,0.695,0.900,0.740,0.650,0.045,0.070,0.970,0.992,0.835,0.102,0.925,0.045,0.775,0.015,0.815,0.630};
    static const double left[HP_SKY_PALETTES]={0.345,0.475,0.530,0.870,0.465,0.640,0.305,0.490,0.915,0.445,0.315,0.780,0.365,0.480,0.565,0.345,0.665,0.705,0.730,0.760,0.610,0.790,0.610,0.725,0.960,0.985,0.865,0.915,0.930,0.035,0.815,0.975,0.925,0.935,0.930,0.695};
    static const double right[HP_SKY_PALETTES]={0.490,0.720,0.805,0.945,0.695,0.865,0.480,0.775,0.965,0.630,0.825,0.885,0.740,0.685,0.795,0.460,0.850,0.900,0.960,0.925,0.850,0.965,0.840,0.840,0.105,0.135,0.050,0.070,0.990,0.940,0.980,0.140,0.060,0.095,0.715,0.815};
    static const double sat[HP_SKY_PALETTES]={0.920,0.840,0.620,0.640,0.770,0.670,0.900,0.770,0.700,0.790,0.930,0.610,0.850,0.700,0.720,0.880,0.580,0.660,0.660,0.690,0.570,0.600,0.650,0.820,0.570,0.500,0.600,0.650,0.530,0.540,0.580,0.560,0.610,0.670,0.590,0.460};
    static const double level[HP_SKY_PALETTES]={0.860,0.880,0.910,0.890,0.910,0.760,0.900,0.920,0.880,0.910,0.900,0.870,0.890,0.900,0.880,0.900,0.910,0.850,0.900,0.890,0.890,0.890,0.860,0.880,0.890,0.860,0.880,0.870,0.890,0.790,0.900,0.840,0.830,0.850,0.840,0.780};
    variant=(variant>=0&&variant<HP_SKY_PALETTES)?variant:0;
    x=HPClamp(isfinite(x)?x:.5,0,1);y=HPClamp(isfinite(y)?y:.5,0,1);
    // 6 visibly different field geometries, also used by the 6 new moods.
    // No hard on/off pulses: all intensity functions are smooth and periodic.
    int geometry=variant%6;
    double a=0,b=0;
    if(geometry==0){ // tall aurora draperies / curtains
        a=sin(6.2*x+1.2*sin(y*5.+phase/34.)-phase/31.);
        b=sin(3.4*x-4.2*y+phase/53.);
    }else if(geometry==1){ // wide horizontal colour-tide
        a=sin(6.3*y+2.2*x+phase/42.);
        b=sin(3.8*y-2.7*x-phase/61.);
    }else if(geometry==2){ // feathery diagonal wave
        a=sin(5.4*(x+y)+phase/37.);
        b=sin(6.7*(x-y)-phase/47.);
    }else if(geometry==3){ // overlapping soft light pools
        a=sin(4.8*x+phase/40.)*cos(4.6*y-phase/49.);
        b=sin(3.9*y+phase/53.);
    }else if(geometry==4){ // slowly unfurling cloud bank
        a=sin(3.7*(.5*x+y)+.70*sin(x*7.+phase/41.)-phase/36.);
        b=sin(2.6*(x-y)+phase/63.);
    }else{ // split ribbons with counter-motion
        a=sin(7.4*x-1.5*y-phase/31.);
        b=sin(7.8*y+2.8*x+phase/43.);
    }
    double leftWeight=pow(HPClamp((.40-x)/.40,0,1),1.30);
    double rightWeight=pow(HPClamp((x-.60)/.40,0,1),1.30);
    double hue=center[variant];
    hue=HPSkyHueMix(hue,left[variant],leftWeight*(.80+.20*(.5+.5*a)));
    hue=HPSkyHueMix(hue,right[variant],rightWeight*(.80+.20*(.5+.5*b)));
    // Large enough flowing accents to be visible, never a full rainbow chase.
    hue=HPWrap(hue+.027*a+.016*b);
    double saturation=HPClamp(sat[variant]+.055*a+.025*b,0,1);
    double lightness=(.48+.056*a+.035*b)*level[variant]*HPClamp(brightness,0,1);
    HPColor c={(float)hue,(float)saturation,(float)HPClamp(lightness,0,1)};
    c.saturation=HPColourDepth(c.saturation,depth);
    return c;
}
// Unique, curated eight-chapter colour stories for every journey. Each
// 2-minute loop has eight 15-second chapters, not four near-identical swatches.
static inline void HPSkyAuroraOrder8(int journey,uint32_t seed,int order[HP_SKY_CHAPTERS]){
    static const uint8_t chapters[HP_SKY_PROGRAMS][HP_SKY_CHAPTERS]={
    {0,6,10,7,12,18,2,0},
    {2,16,20,17,22,18,21,2},
    {3,21,24,27,30,18,26,3},
    {1,7,13,4,14,16,35,1},
    {16,10,17,20,12,21,22,16},
    {1,0,7,9,23,2,11,1},
    {4,35,14,22,16,20,2,4},
    {7,13,14,1,6,10,16,7},
    {14,20,17,22,35,16,2,14},
    {12,14,16,17,18,21,22,12},
    {3,19,18,30,27,21,28,3},
    {5,23,17,19,20,22,35,5},
    {18,21,26,30,34,19,27,18},
    {20,16,22,17,34,14,18,20},
    {2,13,14,7,35,20,2,13},
    {9,23,14,16,17,21,5,9},
    {35,21,30,34,19,28,18,35},
    {4,9,13,7,14,2,20,4},
    {1,0,7,14,16,20,35,1},
    {14,22,20,21,18,16,35,14},
    {35,13,16,20,22,17,21,35},
    {23,16,19,18,22,17,5,23},
    {10,6,0,12,14,18,19,10},
    {30,21,26,24,27,31,29,30},
    {21,30,34,19,28,27,3,21},
    {16,20,22,17,2,35,18,16},
    {20,17,28,32,34,24,18,20},
    {18,21,30,28,26,27,19,18},
    {17,22,20,32,34,28,5,17},
    {30,27,26,31,24,29,21,30},
    {28,18,21,26,34,30,27,28},
    {26,27,24,31,29,33,30,26},
    {21,30,28,34,19,26,18,21},
    {34,28,32,27,30,24,31,34},
    {30,34,21,18,28,26,35,30}
    };
    int program=(journey>=6&&journey<6+HP_SKY_PROGRAMS)?journey-6:0;
    for(int i=0;i<HP_SKY_CHAPTERS;i++)order[i]=chapters[program][i];
    if(!seed)return;
    uint32_t state=seed^(uint32_t)((program+1)*0x9e3779b9U);
    for(int i=HP_SKY_CHAPTERS-1;i>0;i--){
        state=state*1664525U+1013904223U;
        int j=(int)((state>>8)%(uint32_t)(i+1));
        int temp=order[i];order[i]=order[j];order[j]=temp;
    }
}
// Compatibility for controls that inspect four chapter IDs while shuffling.
static inline void HPSkyAuroraOrder(int journey,uint32_t seed,int order[4]){
    int full[HP_SKY_CHAPTERS];HPSkyAuroraOrder8(journey,seed,full);
    for(int i=0;i<4;i++)order[i]=full[i];
}
static inline HPColor HPSkyAuroraJourneyShuffled(int journey,double elapsed,double phase,double x,double y,double brightness,double depth,uint32_t seed){
    int order[HP_SKY_CHAPTERS];HPSkyAuroraOrder8(journey,seed,order);
    int program=(journey>=6&&journey<41)?journey-6:0;
    double t=fmod(fmax(0,isfinite(elapsed)?elapsed:0),2880.);
    double chapterLength=2880./HP_SKY_CHAPTERS;
    int chapter=(int)(t/chapterLength);
    double within=fmod(t,chapterLength);
    // Last 40% of each segment is a cosine-smooth dissolve; at two minutes
    // this produces 6-second fades and 9-second colour holds.
    double blend=HPSkyEase((within-chapterLength*.60)/(chapterLength*.40));
    double loopAngle=t*(6.283185307179586/2880.);
    // A distinct spatial choreography for each programme. Coordinated
    // lamps stay grouped because these are functions of global time/map.
    double u=HPClamp(isfinite(x)?x:.5,0,1),v=HPClamp(isfinite(y)?y:.5,0,1);
    double warp=loopAngle*(1+(program%4));
    switch(program%9){
        case 0: u=HPClamp(u+.10*sin(v*5.+warp),0,1);break; // northern curtain
        case 1: v=HPClamp(v+.20*sin(u*4.-warp),0,1);break; // lavender drape
        case 2: u=HPClamp(u+.12*sin(warp+v*2.),0,1);v=HPClamp(v+.08*cos(warp+u*3.),0,1);break;
        case 3: v=HPClamp(v+.17*sin(3.*u+warp),0,1);break; // horizon swells
        case 4: u=HPClamp(u+.14*sin(warp+v*6.),0,1);break; // waterfall
        case 5: u=HPClamp(u+.12*cos(warp),0,1);break; // wide silk
        case 6: v=HPClamp(v+.15*cos(warp+u*5.),0,1);break; // banks
        case 7: u=HPClamp(u+.11*sin(warp-v*5.),0,1);break; // travelling
        default: v=HPClamp(v+.13*sin(warp+u*3.),0,1);break; // crescent
    }
    // A few programmes use muted, narrower saturation for softer lounge
    // gradients. Others favour vivid natural aurora.
    double programDepth=depth;
    if(program%7==2||program%7==5)programDepth=HPClamp(depth*.77,.25,1);
    HPColor a=HPSkyAuroraField(order[chapter],phase+loopAngle*19.,u,v,brightness,programDepth);
    HPColor b=HPSkyAuroraField(order[(chapter+1)%HP_SKY_CHAPTERS],phase+loopAngle*19.,u,v,brightness,programDepth);
    HPColor c={(float)HPSkyHueMix(a.hue,b.hue,blend),
        (float)HPSkyMix(a.saturation,b.saturation,blend),
        (float)HPSkyMix(a.lightness,b.lightness,blend)};
    // Journey-specific gentle breathing, always nonzero and never a flash.
    double breath=.96+.04*sin(warp+(program%5)*.8);
    c.lightness=(float)HPClamp(c.lightness*breath,0,1);
    return c;
}
static inline HPColor HPSkyAuroraJourney(int journey,double elapsed,double phase,double x,double y,double brightness,double depth){
    return HPSkyAuroraJourneyShuffled(journey,elapsed,phase,x,y,brightness,depth,0);
}
// L6 is ceiling-high and should be the leading aurora. Lights 1.2 and
// 1.3 share its phase as a coordinated wall gradient.
static inline double HPSkySharedCurtainPosition(double elapsed,double phase){
    double circle=fmod(fmax(0,isfinite(elapsed)?elapsed:0),2880.)/2880.;
    return .5+.43*sin(2.*6.283185307179586*circle+.17*sin(phase/48.));
}
static inline HPColor HPSkyMappedAuroraLamp(int role,int journey,double elapsed,double phase,
        double brightness,double depth,uint32_t seed){
    double sweep=HPSkySharedCurtainPosition(elapsed,phase);
    double x=HPClamp(sweep+(role==1?-.17:role==2?.17:0.),0,1);
    double y=role==0?.12:role==1?.36:.59;
    HPColor c=HPSkyAuroraJourneyShuffled(journey,elapsed,phase,x,y,brightness,depth,seed);
    c.lightness=(float)HPClamp(c.lightness*(role==0?1.12:role==1?.92:.86),0,1);
    return c;
}
// TV 11 is dedicated neutral bias light: no saturated cobalt. This does
// not claim calibrated D65; user brightness control remains authoritative.
static inline HPColor HPSkyTVBias(double userBrightness){
    return (HPColor){.57f,.065f,(float)HPClamp(userBrightness,.08,1)};
}
// Fast chapters need a faster but STILL smooth chase. This is used only
// for Journeys with short durations, never static scenes or other moods.
static inline HPColor HPSkyAdaptiveDrift(HPColor old,HPColor target,double elapsed,double minutes){
    double dt=isfinite(elapsed)?HPClamp(elapsed,0,2):0;
    double m=HPClamp(isfinite(minutes)?minutes:48,2,48);
    double urgency=HPClamp((48.-m)/46.,0,1);
    double tau=7.-4.4*urgency; // 2.6 seconds at the two-minute end
    double speedDegrees=1.+6.*urgency; // <=7 degrees/sec, not arbitrary jumps
    double gain=-expm1(-dt/tau);
    double dh=target.hue-old.hue;dh-=floor(dh+.5);
    HPColor c;
    c.hue=(float)HPWrap(old.hue+HPClamp(dh*gain,-dt*speedDegrees/360.,dt*speedDegrees/360.));
    c.saturation=(float)HPClamp(old.saturation+HPClamp((target.saturation-old.saturation)*gain,-dt*(.016+.055*urgency),dt*(.016+.055*urgency)),0,1);
    c.lightness=(float)HPClamp(old.lightness+HPClamp((target.lightness-old.lightness)*gain,-dt*(.012+.060*urgency),dt*(.012+.060*urgency)),0,1);
    return c;
}
#endif
