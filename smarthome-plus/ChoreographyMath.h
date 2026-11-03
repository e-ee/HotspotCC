#ifndef HP_CHOREOGRAPHY_MATH_H
#define HP_CHOREOGRAPHY_MATH_H
#include "EffectPatterns.h"
static inline double HPEase(double t){t=HPClamp(t,0,1);return t*t*(3-2*t);}
// HSL here is the vendor's HSV-like hue/saturation/level representation.
static inline HPColor HPBlendColour(HPColor a,HPColor b,double t){
 t=HPClamp(t,0,1);double delta=b.hue-a.hue;delta-=floor(delta+.5);
 if(a.saturation<.01){a.hue=b.hue;delta=0;}
 if(b.saturation<.01)delta=0;
 return (HPColor){HPWrap(a.hue+delta*t),a.saturation+(b.saturation-a.saturation)*t,a.lightness+(b.lightness-a.lightness)*t};
}
static inline void HPDirectedPoint(double x,double y,double degrees,double *u,double *v){double r=degrees*3.141592653589793/180.;double span=fabs(cos(r))+fabs(sin(r));*u=.5+((x-.5)*cos(r)+(y-.5)*sin(r))/span;*v=.5+(-(x-.5)*sin(r)+(y-.5)*cos(r))/span;}
static inline HPColor HPLivingAurora(double phase,double x,double y,double brightness,double depth){
 double bend=.13*sin(y*4.5+phase/155.)+.07*sin(y*9.-phase/230.);
 double veil=.5+.5*sin((x+bend)*7.-phase/65.);
 double second=.5+.5*sin(x*3.1-y*4.7+phase/99.);
 HPColor jade={.365,.94,.48*brightness},blue={.61,.89,.41*brightness},violet={.765,.86,.45*brightness};
 HPColor c=HPBlendColour(jade,blue,HPEase(veil));c=HPBlendColour(c,violet,.55*pow(second,2));
 c.lightness*=.91+.09*sin(y*3.+phase/120.);c.saturation=HPColourDepth(c.saturation,depth);return c;
}
static inline HPColor HPExploreColour(HPColor c,int variant,double strength){
 strength=HPClamp(strength,0,1);
 if(variant==1)c.hue=HPWrap(c.hue-.035*strength);
 if(variant==2)c.hue=HPWrap(c.hue+.045*strength);
 if(variant==3)c.saturation=HPClamp(c.saturation+.25*strength,0,1);
 if(variant==4){c.saturation*=1-.45*strength;c.lightness*=1-.12*strength;}
 if(variant==5)c.lightness*=1-.35*strength;
 return c;
}
static inline HPColor HPChoreographyDrift(HPColor old,HPColor target,double elapsed){
 double dt=isfinite(elapsed)?HPClamp(elapsed,0,2):0;HPColor c=HPDrift(old,target,dt);double delta=target.hue-old.hue;delta-=floor(delta+.5);
 c.hue=HPWrap(old.hue+HPClamp(delta*(-expm1(-dt/7.)),-dt/360.,dt/360.));return c;
}
static inline double HPRippleWeight(double distance,double age,int inward){double radius=inward?1.5-age*.09:age*.09;return exp(-pow((distance-radius)/.16,2))*HPClamp(age/.9,0,1)*HPClamp((19.-age)/3.,0,1);}
#endif
