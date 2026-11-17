// Map-aware night-room auroral fields. All map coordinates are normalized
// public geometry, never the user's private Home controller IDs or network.
#ifndef HP_SKY_AURORA_H
#define HP_SKY_AURORA_H
#define HP_SKY_PALETTES 36
#define HP_SKY_PROGRAMS 35
static inline HPColor HPSkyAuroraField(int variant,double phase,double x,double y,double brightness,double depth){
    static const double blue[HP_SKY_PALETTES]={0.592,0.588,0.614,0.580,0.603,0.625,0.601,0.617,0.579,0.601,0.592,0.647,0.583,0.617,0.652,0.632,0.585,0.602,0.577,0.611,0.624,0.571,0.600,0.629,0.085,0.098,0.052,0.106,0.948,0.895,0.070,0.093,0.112,0.063,0.970,0.090};
    static const double left[HP_SKY_PALETTES]={0.404,0.438,0.466,0.398,0.449,0.457,0.404,0.421,0.416,0.442,0.438,0.440,0.409,0.450,0.469,0.442,0.425,0.434,0.401,0.432,0.439,0.403,0.442,0.453,0.075,0.080,0.045,0.091,0.936,0.872,0.065,0.075,0.092,0.051,0.957,0.073};
    static const double right[HP_SKY_PALETTES]={0.466,0.486,0.500,0.457,0.496,0.481,0.471,0.483,0.466,0.494,0.490,0.488,0.472,0.499,0.498,0.487,0.480,0.490,0.467,0.488,0.486,0.473,0.500,0.493,0.120,0.138,0.077,0.140,0.975,0.929,0.110,0.130,0.150,0.089,0.990,0.120};
    static const double level[HP_SKY_PALETTES]={1.000,0.960,0.920,1.010,0.950,0.660,0.740,0.710,0.720,0.740,0.780,0.650,0.760,0.750,0.670,0.710,0.750,0.770,0.760,0.670,0.700,0.770,0.720,0.660,0.750,0.720,0.690,0.730,0.640,0.600,0.630,0.670,0.660,0.650,0.590,0.580};
    variant=(variant>=0&&variant<HP_SKY_PALETTES)?variant:0;
    x=HPClamp(isfinite(x)?x:.5,0,1);y=HPClamp(isfinite(y)?y:.5,0,1);
    // For cool palettes: centre remains blue, green and teal occupy outer edges.
    // Warm palettes have warm edges only, never green through the central TV wall.
    double leftEdge=pow(HPClamp((.36-x)/.36,0,1),1.65);
    double rightEdge=pow(HPClamp((x-.64)/.36,0,1),1.65);
    double bend=.075*sin(4.1*y+phase/165.)+.032*sin(8.7*y-phase/240.);
    double veil=.5+.5*sin((x+bend)*5.7-phase/92.);
    double second=.5+.5*sin(x*3.8-y*4.2+phase/133.);
    double h=blue[variant]+.014*(veil-.5)+.010*(second-.5);
    // Warm pinks can straddle hue zero. Keep hues together before wrapping.
    double l=left[variant],r=right[variant];
    if(variant>=24){
        if(h>.8&&l<.2)l+=1.;
        if(h>.8&&r<.2)r+=1.;
        if(h<.2&&l>.8)l-=1.;
        if(h<.2&&r>.8)r-=1.;
    }
    h=h*(1-leftEdge)+l*leftEdge;
    h=h*(1-rightEdge)+r*rightEdge;
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
        {1,0,3,2},
        {2,4,1,3},
        {5,1,0,5},
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
#endif
