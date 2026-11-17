#include <assert.h>
#include <math.h>
#include "EffectPatterns.h"
int main(void){
  for(int mood=34;mood<=39;mood++){
    for(int i=0;i<30;i++){
      double t=i*29.73;
      for(int j=0;j<=100;j++){
        double x=j/100.,y=.31;
        HPColor c=HPStudioTarget(mood,-1,0,t,x,y,1,1,0);
        assert(isfinite(c.hue)&&isfinite(c.saturation)&&isfinite(c.lightness));
        assert(c.hue>=0&&c.hue<=1&&c.saturation>=0&&c.saturation<=1);
        assert(c.lightness>=0&&c.lightness<=1);
        if(x>=.25&&x<=.75)assert(c.hue>.50);
      }
    }
  }
  for(int journey=6;journey<=8;journey++){
   for(int i=0;i<=2880;i++){
      HPColor c=HPStudioTarget(0,journey,i,123,.5,.5,.8,1,0);
      assert(c.hue>.5);
   }
   HPColor a=HPStudioTarget(0,journey,0,123,.07,.47,.8,1,0);
   HPColor b=HPStudioTarget(0,journey,2880,123,.07,.47,.8,1,0);
   assert(fabs(a.hue-b.hue)<1e-6);
  }
  return 0;
}
