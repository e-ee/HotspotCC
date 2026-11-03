#include <assert.h>
#include <stdio.h>
#include "ChoreographyMath.h"
int main(void){
 HPColor a={.98,.8,.3},b={.02,.8,.4};HPColor mid=HPBlendColour(a,b,.5);assert(mid.hue<.01||mid.hue>.99);
 double u,v;HPDirectedPoint(1,.5,0,&u,&v);assert(u>.8&&fabs(v-.5)<1e-6);HPDirectedPoint(1,.5,180,&u,&v);assert(u<.2);HPDirectedPoint(.5,1,90,&u,&v);assert(u>.8);
 int jade=0,blue=0,violet=0;double maxDelta=0;
 for(int t=0;t<1800;t++)for(int i=0;i<14;i++){double x=(i%4)/3.,y=(i/4)/3.;HPColor c=HPLivingAurora(t,x,y,.6,1),n=HPLivingAurora(t+.1,x,y,.6,1);assert(c.hue>=0&&c.hue<=1&&c.lightness>0&&c.lightness<1&&c.saturation<=1);jade+=c.hue<.43;blue+=c.hue>.55&&c.hue<.67;violet+=c.hue>.67;double d=fabs(c.hue-n.hue);if(d>maxDelta)maxDelta=d;assert(d<.003);}
 assert(jade&&blue&&violet);assert(HPRippleWeight(.5,5.55,0)>.9);assert(HPRippleWeight(.5,0,0)==0);assert(HPRippleWeight(.5,19,0)==0);assert(HPEase(0)==0&&HPEase(1)==1);
 for(int i=0;i<6;i++){HPColor c=HPExploreColour(a,i,1);assert(c.hue>=0&&c.hue<=1&&c.saturation>=0&&c.saturation<=1&&c.lightness>=0&&c.lightness<=1);}
 printf("Choreography math passed; jade=%d blue=%d violet=%d max hue step=%.6f\n",jade,blue,violet,maxDelta);return 0;
}
