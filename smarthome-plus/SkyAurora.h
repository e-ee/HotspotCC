// Map-aware night-room auroral fields. All map coordinates are normalized
// public geometry, never the user's private Home controller IDs or network.
#ifndef HP_SKY_AURORA_H
#define HP_SKY_AURORA_H
#define HP_SKY_PALETTES 36
#define HP_SKY_PROGRAMS 35
static inline HPColor HPSkyAuroraField(int variant,double phase,double x,double y,double brightness,double depth){
    static const double blue[HP_SKY_PALETTES]={0.600,0.596,0.622,0.583,0.609,0.623,0.603,0.615,0.584,0.612,0.592,0.631,0.591,0.621,0.635,0.603,0.640,0.655,0.626,0.669,0.648,0.675,0.645,0.705,0.667,0.687,0.708,0.724,0.668,0.743,0.676,0.695,0.717,0.694,0.745,0.718};
    static const double left[HP_SKY_PALETTES]={0.375,0.397,0.425,0.355,0.400,0.418,0.382,0.405,0.390,0.427,0.410,0.442,0.374,0.422,0.433,0.387,0.410,0.400,0.398,0.426,0.413,0.738,0.430,0.765,0.740,0.759,0.780,0.795,0.741,0.828,0.764,0.776,0.796,0.765,0.830,0.784};
    static const double right[HP_SKY_PALETTES]={0.461,0.473,0.486,0.454,0.492,0.484,0.467,0.481,0.455,0.490,0.496,0.497,0.461,0.499,0.492,0.482,0.766,0.785,0.748,0.812,0.790,0.849,0.826,0.865,0.860,0.879,0.900,0.904,0.854,0.919,0.887,0.918,0.916,0.885,0.922,0.910};
    static const double level[HP_SKY_PALETTES]={0.880,0.900,0.890,0.910,0.850,0.760,0.870,0.890,0.880,0.890,0.850,0.850,0.870,0.880,0.820,0.880,0.870,0.880,0.890,0.850,0.890,0.860,0.870,0.840,0.890,0.870,0.850,0.870,0.860,0.870,0.900,0.860,0.870,0.880,0.870,0.850};
    variant=(variant>=0&&variant<HP_SKY_PALETTES)?variant:0;
    x=HPClamp(isfinite(x)?x:.5,0,1);y=HPClamp(isfinite(y)?y:.5,0,1);
    // Hekla Aurora-inspired warm blue centre, natural green ribbons confined
    // to the outer mapped lights. Other programs favour lilac and soft pink.
    // Darkness is low output with unlit wall, not a simulated black bulb.
    double leftEdge=pow(HPClamp((.36-x)/.36,0,1),1.65);
    double rightEdge=pow(HPClamp((x-.64)/.36,0,1),1.65);
    double bend=.075*sin(4.1*y+phase/165.)+.032*sin(8.7*y-phase/240.);
    double veil=.5+.5*sin((x+bend)*5.7-phase/92.);
    double second=.5+.5*sin(x*3.8-y*4.2+phase/133.);
    double h=blue[variant]+.014*(veil-.5)+.010*(second-.5);
    h=h*(1-leftEdge)+left[variant]*leftEdge;
    h=h*(1-rightEdge)+right[variant]*rightEdge;
    double saturation=.77+.09*veil+.04*(leftEdge+rightEdge);
    if(variant==2||variant==4||variant==14||variant==19||variant>=24)saturation-=.13;
    double lightness=(.49+.052*veil+.022*second)*level[variant]*HPClamp(brightness,0,1);
    HPColor c={(float)HPWrap(h),(float)HPClamp(saturation,0,1),(float)HPClamp(lightness,0,1)};
    c.saturation=HPColourDepth(c.saturation,depth);
    return c;
}
// Sky journeys use four long chapters and continuous circular dissolves.
// For the 48-minute default each chapter lasts 12 minutes; the last
// three minutes dissolve into the next chapter, including across the loop.
static inline void HPSkyAuroraOrder(int journey,uint32_t seed,int order[4]){
    static const uint8_t chapters[HP_SKY_PROGRAMS][4]={
        {1,0,3,2},   // Hekla-style green edges and warm-blue centre
        {16,17,18,20}, // lavender/blue auroral curtains
        {24,27,25,29}, // rosy cabin and pink-periwinkle veils
        {6,0,7,3},
        {7,13,6,2},
        {8,1,4,9},
        {9,0,10,11},
        {10,16,4,1},
        {11,19,5,14},
        {12,3,17,2},
        {13,4,22,7},
        {14,5,19,8},
        {15,8,2,7},
        {16,10,21,1},
        {17,6,4,0},
        {18,3,12,17},
        {19,13,11,5},
        {20,14,9,6},
        {21,16,3,10},
        {22,2,17,7},
        {23,20,5,11},
        {6,21,18,1},
        {8,10,15,4},
        {24,27,25,29},
        {25,28,27,34},
        {26,30,24,29},
        {27,25,33,31},
        {28,34,29,27},
        {29,32,35,24},
        {30,24,33,27},
        {31,25,34,29},
        {32,30,35,28},
        {33,27,31,24},
        {34,28,32,25},
        {35,32,29,24}
    };
    int program=(journey>=6&&journey<6+HP_SKY_PROGRAMS)?journey-6:0;
    for(int i=0;i<4;i++)order[i]=chapters[program][i];
    if(!seed)return;
    uint32_t state=seed^(uint32_t)((program+1)*0x9e3779b9U);
    for(int i=3;i>0;i--){
        state=state*1664525U+1013904223U;
        int j=(int)((state>>8)%(uint32_t)(i+1));
        int tmp=order[i];order[i]=order[j];order[j]=tmp;
    }
}
static inline HPColor HPSkyAuroraJourneyShuffled(int journey,double elapsed,double phase,double x,double y,double brightness,double depth,uint32_t seed){
    int chapter[4];HPSkyAuroraOrder(journey,seed,chapter);
    double t=fmod(fmax(0,isfinite(elapsed)?elapsed:0),2880.);
    int index=(int)(t/720.);double section=fmod(t,720.);
    double amount=HPClamp((section-540.)/180.,0,1);amount=amount*amount*(3-2*amount);
    HPColor a=HPSkyAuroraField(chapter[index],phase,x,y,brightness,depth);
    HPColor b=HPSkyAuroraField(chapter[(index+1)%4],phase,x,y,brightness,depth);
    // The palette sequence never jumps in hue or brightness across a boundary;
    // use shortest hue arc for the warm magenta/amber families.
    double delta=b.hue-a.hue;
    if(delta>.5)delta-=1.;
    else if(delta<-.5)delta+=1.;
    return (HPColor){(float)HPWrap(a.hue+delta*amount),
                     (float)(a.saturation+(b.saturation-a.saturation)*amount),
                     (float)(a.lightness+(b.lightness-a.lightness)*amount)};
}
static inline HPColor HPSkyAuroraJourney(int journey,double elapsed,double phase,double x,double y,double brightness,double depth){
    return HPSkyAuroraJourneyShuffled(journey,elapsed,phase,x,y,brightness,depth,0);
}

// The mapped L6 Lamp is ceiling-high, not the far-left wall lamp. Moving
// its virtual position through each journey's existing palette produces
// changing green, teal, blue, lavender and pink without off-theme colours.
// The 1.2 and 1.3 wall lamps use the SAME wave with a small phase-offset:
// one auroral curtain with a visible gradient, not separate independent chases.
static inline double HPSkySharedCurtainPosition(double elapsed,double phase){
    double cycle=fmod(fmax(0,isfinite(elapsed)?elapsed:0),2880.)/720.;
    double bend=.13*sin(phase/125.);
    return .5+.46*sin(6.283185307179586*(cycle+bend));
}
static inline HPColor HPSkyMappedAuroraLamp(int role,int journey,double elapsed,double phase,
                                             double brightness,double depth,uint32_t seed){
    double sweep=HPSkySharedCurtainPosition(elapsed,phase);
    // 0 = L6 ceiling, 1 = wall lamp 1.2, 2 = wall lamp 1.3.
    double x=HPClamp(sweep+(role==1?-.16:role==2?.16:0.),0,1);
    double y=role==0?.12:role==1?.36:.59;
    HPColor c=HPSkyAuroraJourneyShuffled(journey,elapsed,phase,x,y,brightness,depth,seed);
    // L6 is the primary ceiling bounce; the wall pair is subordinate.
    double factor=role==0?1.10:role==1?.91:.86;
    c.lightness=(float)HPClamp(c.lightness*factor,0,1);
    return c;
}
// TV bias is different from an aurora: nearly neutral, stationary, soft
// white, and calibrated visually to the wall. No pure blue and no mandatory
// 10% cap. Saturation near zero yields a near-neutral RGB mixture rather
// than a single short-wavelength blue LED. Does not imply true D65 calibration.
static inline HPColor HPSkyTVBias(double userBrightness){
    return (HPColor){.57f,.065f,(float)HPClamp(userBrightness*.70,.13,.46)};
}
static inline HPColor HPSkyLimitTV(HPColor colour,int tvLamp,int isJourney){
    if(tvLamp&&isJourney)colour.lightness=(float)HPClamp(colour.lightness,0,.10);
    return colour;
}
#endif
