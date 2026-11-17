// Boeing Sky Interior-inspired blue/teal aurora. Coordinates come from the
// user's existing room map; no private identifiers or topology are embedded.
// Hue is HSV-like in SYLVANIA's HSL vendor interface (0..1 circle).
#ifndef HP_SKY_AURORA_H
#define HP_SKY_AURORA_H
static inline HPColor HPSkyAuroraField(int variant,double phase,double x,double y,double brightness,double depth){
    static const double blue[6]={.592,.588,.614,.580,.603,.625};
    static const double left[6]={.404,.438,.466,.398,.449,.457};
    static const double right[6]={.466,.486,.500,.457,.496,.481};
    static const double level[6]={1.00,.96,.92,1.01,.95,.66};
    variant=(variant>=0&&variant<6)?variant:0;
    x=HPClamp(isfinite(x)?x:.5,0,1);y=HPClamp(isfinite(y)?y:.5,0,1);
    // The centre stays blue. Green can occur only on outer mapped positions.
    double leftEdge=pow(HPClamp((.36-x)/.36,0,1),1.65);
    double rightEdge=pow(HPClamp((x-.64)/.36,0,1),1.65);
    double bend=.075*sin(4.1*y+phase/165.)+.032*sin(8.7*y-phase/240.);
    double veil=.5+.5*sin((x+bend)*5.7-phase/92.);
    double second=.5+.5*sin(x*3.8-y*4.2+phase/133.);
    double hue=blue[variant]+.014*(veil-.5)+.010*(second-.5);
    hue=hue*(1-leftEdge)+left[variant]*leftEdge;
    hue=hue*(1-rightEdge)+right[variant]*rightEdge;
    double saturation=.80+.09*veil+.04*(leftEdge+rightEdge);
    if(variant==2||variant==4)saturation-=.13;
    double lightness=(.49+.052*veil+.022*second)*level[variant]*HPClamp(brightness,0,1);
    HPColor c={(float)HPClamp(hue,0,1),(float)HPClamp(saturation,0,1),(float)HPClamp(lightness,0,1)};
    c.saturation=HPColourDepth(c.saturation,depth);
    return c;
}
// 4 x 12-minute chapters, with a 3-minute ease between related colour fields.
// Each boundary uses the same shared phase, including the looping boundary.
static inline HPColor HPSkyAuroraJourney(int journey,double elapsed,double phase,double x,double y,double brightness,double depth){
    static const int chapter[3][4]={{1,0,3,2},{2,4,1,3},{5,1,0,5}};
    int program=(journey>=6&&journey<=8)?journey-6:0;
    double t=fmod(fmax(0,isfinite(elapsed)?elapsed:0),4.*720.);
    int index=(int)(t/720.);double section=fmod(t,720.);
    double amount=HPClamp((section-540.)/180.,0,1);amount=amount*amount*(3-2*amount);
    HPColor a=HPSkyAuroraField(chapter[program][index],phase,x,y,brightness,depth);
    HPColor b=HPSkyAuroraField(chapter[program][(index+1)%4],phase,x,y,brightness,depth);
    // These palettes all occupy a short, contiguous blue-to-teal hue arc.
    return (HPColor){(float)(a.hue+(b.hue-a.hue)*amount),
                     (float)(a.saturation+(b.saturation-a.saturation)*amount),
                     (float)(a.lightness+(b.lightness-a.lightness)*amount)};
}
#endif
