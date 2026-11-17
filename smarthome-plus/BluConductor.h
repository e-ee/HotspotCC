// Blu Conductor. Effects are built from the user's *existing* Blu scene,
// one original HSL value per real mapped bulb. Never store Home IDs here.
// Native Realm scene records are read locally by HomePlus.m at runtime.
#ifndef HP_BLU_CONDUCTOR_H
#define HP_BLU_CONDUCTOR_H
#define HP_BLU_MAX 48
#define HP_BLU_MOOD_FIRST 42
#define HP_BLU_MOOD_COUNT 6
#define HP_BLU_JOURNEY_FIRST 44
#define HP_BLU_JOURNEY_COUNT 12

typedef struct {
    HPColor anchor;
    double x,y;
    int valid, tv,ceiling,wall12,wall13,corner9,corner12;
    int partner;
} HPBluBulb;
typedef struct {
    HPBluBulb bulb[HP_BLU_MAX];
    int count,validCount,greenCount,blueCount,pairCount;
} HPBluStage;

static inline int HPBluIsGreen(HPColor c){
    return c.saturation>=.35&&c.hue>=.285&&c.hue<.452;
}
static inline int HPBluIsBlue(HPColor c){
    return c.saturation>=.30&&c.hue>=.475&&c.hue<=.715;
}
static inline double HPBluEase(double t){
    t=HPClamp(t,0,1);return t*t*(3.-2.*t);
}
static inline double HPBluHueDistance(double a,double b){
    double d=fabs(a-b);return fmin(d,1.-d);
}
static inline HPColor HPBluLerp(HPColor a,HPColor b,double t){
    t=HPBluEase(t);
    return (HPColor){(float)HPSkyHueMix(a.hue,b.hue,t),
      (float)HPClamp(HPSkyMix(a.saturation,b.saturation,t),0,1),
      (float)HPClamp(HPSkyMix(a.lightness,b.lightness,t),0,1)};
}
static inline int HPBluIsAurora(int mode,int journey){
    return (mode>=HP_BLU_MOOD_FIRST&&mode<HP_BLU_MOOD_FIRST+HP_BLU_MOOD_COUNT)||
      (journey>=HP_BLU_JOURNEY_FIRST&&journey<HP_BLU_JOURNEY_FIRST+HP_BLU_JOURNEY_COUNT);
}
static inline void HPBluPair(HPBluStage *stage){
    if(!stage)return;
    stage->greenCount=stage->blueCount=stage->pairCount=stage->validCount=0;
    for(int i=0;i<stage->count;i++){
        HPBluBulb *b=&stage->bulb[i];b->partner=-1;
        if(!b->valid)continue;
        stage->validCount++;
        if(HPBluIsGreen(b->anchor)&&!b->tv)stage->greenCount++;
        if(HPBluIsBlue(b->anchor)&&!b->tv)stage->blueCount++;
    }
    // Every green must be matched with a real neighbouring Blu-blue bulb.
    // One green + one blue remain one green + one blue as they exchange roles.
    // Distinct blue receivers prevent a green wash of every fixture at once.
    for(int g=0;g<stage->count;g++){
        HPBluBulb *a=&stage->bulb[g];
        if(!a->valid||a->tv||!HPBluIsGreen(a->anchor))continue;
        int nearest=-1;double best=1e4;
        for(int i=0;i<stage->count;i++){
            HPBluBulb *b=&stage->bulb[i];
            if(i==g||!b->valid||b->tv||b->partner>=0||!HPBluIsBlue(b->anchor))continue;
            double dx=a->x-b->x,dy=a->y-b->y;
            double dist=dx*dx+dy*dy;
            // Prefer adjacent colours seen together in the actual room,
            // rather than sending green across a distant wall.
            if(dist>1.15)continue;
            // Ceiling and corner fixtures can have elevated influence, but
            // their original individual brightness remains authoritative.
            if((a->corner9&&b->corner12)||(b->corner9&&a->corner12))dist*=.30;
            if(dist<best){best=dist;nearest=i;}
        }
        if(nearest>=0){a->partner=nearest;stage->bulb[nearest].partner=g;stage->pairCount++;}
    }
}
static inline double HPBluCycle(double elapsed,double phase,int journey){
    if(journey>=HP_BLU_JOURNEY_FIRST){
        double t=fmod(fmax(0,isfinite(elapsed)?elapsed:0),2880.);
        return t/2880.;
    }
    return (isfinite(phase)?phase:0.)/(6.283185307179586*110.);
}
static inline double HPBluWave(int program,double cycle,double x,double y,double phase,uint32_t seed){
    const double twoPi=6.283185307179586;
    // 12 genuinely different movements: still, travelling, mirrored,
    // diagonal, rare curtain, ceiling bloom, counter-current and more.
    double shift=(seed%23)*.008;
    double w=0.;
    switch(program%12){
        case 0:w=1.*cycle+.06*x;break; // Original Blu: almost faithful
        case 1:w=2.*cycle+.24*x-.08*y;break; // Blue/green exchange
        case 2:w=3.*cycle-.23*x+.18*y;break; // Aurora left-to-right
        case 3:w=1.*cycle+.15*y;break; // Slow ceiling tides
        case 4:w=4.*cycle+.11*(1-x)+.09*y;break; // Fast northern curtains
        case 5:w=2.*cycle-.26*y;break; // Falling silk
        case 6:w=1.*cycle+.22*(x+y);break; // Diagonal
        case 7:w=3.*cycle+.09*sin(4*x+twoPi*cycle);break; // Constellation
        case 8:w=2.*cycle+.13*(x-y);break; // Mirror
        case 9:w=1.*cycle+.16*x-.09*sin(twoPi*cycle);break; // Soft glass
        case 10:w=3.*cycle+.14*y;break; // Ceiling bloom
        default:w=2.*cycle-.18*x+.13*y;break; // Crosswinds
    }
    // Fractional phase is NOT added to journey cycles: that would create a
    // discontinuity at the 2..48 minute repeat boundary.
    if(journey<HP_BLU_JOURNEY_FIRST)w+=phase/1550.;
    return .5-.5*cos(twoPi*(w+shift));
}
static inline double HPBluAmplitude(int program){
    static const double amounts[12]={
       .08,.88,.96,.50,.92,.62,.58,.80,.74,.34,.79,.67
    };
    return amounts[(program%12+12)%12];
}
static inline HPColor HPBluTarget(const HPBluStage *stage,int index,int journey,
                                  int mode,double elapsed,double phase,
                                  double brightness,double depth,double blend,uint32_t seed){
    if(!stage||index<0||index>=stage->count||!stage->bulb[index].valid)
        return (HPColor){.575f,.55f,(float)HPClamp(brightness*.4,0,1)};
    const HPBluBulb *me=&stage->bulb[index];
    HPColor c=me->anchor;
    const int program=journey>=HP_BLU_JOURNEY_FIRST?
        journey-HP_BLU_JOURNEY_FIRST:mode-HP_BLU_MOOD_FIRST;
    double cycle=HPBluCycle(elapsed,phase,journey);
    if(me->tv){
        // Blu TV remains ONE colour at any instant, not a gradient between
        // fixtures. Trust the Blu source if already soft teal. Suppress
        // painfully saturated pure blue and overpowering green.
        double h=HPClamp(me->anchor.hue,.448,.538);
        double soft=.5+.5*sin(6.283185307179586*cycle+.5);
        c.hue=(float)HPClamp(h+.012*(soft-.5),.443,.545);
        c.saturation=(float)HPClamp(me->anchor.saturation,.30,.64);
    }else if(me->partner>=0){
        const HPBluBulb *other=&stage->bulb[me->partner];
        // The shared midpoint means both paired lamps exchange roles
        // simultaneously. If one becomes greener, the other becomes bluer.
        double mx=(me->x+other->x)*.5,my=(me->y+other->y)*.5;
        double wave=HPBluWave(program,cycle,mx,my,phase,seed);
        double exchange=HPClamp(wave*HPBluAmplitude(program)*HPClamp(blend,0,1),0,.98);
        // The same exchange factor is used on BOTH sides.
        c=HPBluLerp(me->anchor,other->anchor,exchange);
    }else{
        // Ineligible/non-green bulbs stay in their Blu colour family.
        // They subtly modulate lightness, never turn unrelated bulbs green.
        double wave=sin(6.283185307179586*(cycle*(1+program%3)+.12*me->x-.06*me->y));
        double amp=(program==0?.015:program%5==0?.10:.05)*HPClamp(blend,0,1);
        c.lightness=(float)HPClamp(c.lightness*(1+amp*wave),0,1);
    }
    // Coordinate ceiling and the user's 1.2/1.3 wall pair using ONE
    // shared field phase, but do not override their actual scene colours.
    if(!me->tv&&(me->ceiling||me->wall12||me->wall13||
                 me->corner9||me->corner12)){
        double wave=sin(6.283185307179586*(cycle*(program%3+1)+
            (me->wall13?.05:me->corner12?.11:me->ceiling?.17:.0)));
        double amount=program==0?.014:.047;
        c.lightness=(float)HPClamp(c.lightness*(1+amount*wave),0,1);
    }
    // The saved scene defines each bulb's physical brightness ratio.
    // .48 is the Blu-era master level in the supplied Home backup.
    double multiplier=HPClamp(brightness/.48,.18,2.05);
    c.lightness=(float)HPClamp(c.lightness*multiplier,0,1);
    c.saturation=HPColourDepth(c.saturation,depth);
    return c;
}
#endif
