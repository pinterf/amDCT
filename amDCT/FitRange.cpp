
#include "FitRange.h"   

#include <math.h>       // Needed for pow()

#include "amDCTtypedefs.h"  // needed for uint32_t etc.  

#include <stdio.h>   



/*
#include <stdio.h>

double mapRange(double a1,double a2,double b1,double b2,double s)
{
  return b1 + (s-a1)*(b2-b1)/(a2-a1);
}

int main()
{
  int i;
  puts("Mapping [0,10] to [-1,0] at intervals of 1:");

  for(i=0;i<=10;i++)
  {
    printf("f(%d) = %g\n",i,mapRange(0,10,-1,0,i));
  }

  return 0;
}

//void testFitrange(char *msgBuf) {
//
//  int  cval;
//  int src2;
//  int src1;
//  int dst2;
//  int dst1;
//  int nval;
//
//
//  cval = 100;
//  src2 =  50;
//  src1 = 150;
//  dst2 =  50;
//  dst1 = 150;
//
//  nval = 0;
//  nval = Int_FitRange_Int(cval, src1, src2, dst1, dst2);
//
//  sprintf_s(msgBuf, 255, "c%3d n%3d Mi%3d Mx%3d din=%3d dax= %3d\n", cval, nval, src1, src2, dst1, dst2);
////sprintf_s(msgBuf, sizeof(msgBuf), "maxSmooth=%3d maxSrc=%3d", (int)FrameInfoArgs.frame_SmoothMax, (int)FrameInfoArgs.frame_Max);

  for (x=src1, x<src2, s++) {
    nval = Int_FitRange_Int(x, src1, src2, dst1, dst2);
    printf("%3d  %3d\n", x, nval);
  }
//
//  return;
//}
*/






/*
  Use the formula
    DstVal=((curval - src2) * (dstRange / srcRange)) + dst2
  to fit the current value into the destination range.
  If the input value falls outside the source range
  then the output value would be outside of the destination range.
  If the output value is outside the destination range then it is
  clipped to the destination range unless the routine name ends with _NoClip
  in which case the unclipped output value is returned.
*/

/*
#define fitRange(curVal, src1, src2, dst1, dst2) \
  ( ((curVal) - (src2)) * ( ( ((dst1)-(dst2)) + ( ((src1)-(src2)) >> 1 ) ) / ((src1)-(src2)) )  + (dst2) )
*/
/*
// no rounding  ( ((curVal) - (src2)) * ( ((dst1)-(dst2)) / ((src1)-(src2)) )  + (dst2) )

  out = (int) (( (float)(curVal - src2) * ( (float)(dst1-dst2) / (float)(src1-src2) )  + (float)dst2) + 0.5F);
  out = (int) (( (float)       (a)        * ( (float)      (b)       / (float)      (c) )        + (float)dst2) + 0.5F);
  out =  a * ( b / c)   + dst2;
  out =  (a * b) / (c)  + dst2;

  out =  (a * b) + ((a * b)>>1) / (c)   + dst2;

  d   = (a * b)
  out =  (d) + ((d>>1) / c )  + dst2;
 */



 //
 //  NAME PARTS
 //
 //  XXX_FitRange_YYY_ZZZ
 //    XXX either Float or Int. The data type being returned.
 //    YYY either Float or Int. The data type of the arguments.
 //    ZZZ NoClip     If specified the return value is NOT clipped to the dest arguments.
 //   
 //  Float_FitRange_Float()  
 //  If float precedes fitRange the routine returns a float otherwise it returns an integer that has been rounded.
 //  If Float is the last part of the routine name, the routine takes floats as arguments otherwise arguments are integers.
 //
 //      BOUNDARY CONDITIONS
 //  If src1 == src2, the input range is 0  then the average of dst1 and dst2 is returned.
 //  If dst1 == dst2, then return dst2.
 //  If curVal is outside the range src1 and src2, and the routine name contains _NoClip then the returned value will be larger than dst1 or smaller than dst2.
 //      otherwise the output returned will be clipped to dst1 or dst2. 

 // Functions
 //  Int_FitRange_Int    
 //  Int_FitRange_Float       
 //
 //  Float_FitRange_Float_NoClip        
 //  Float_FitRange_Float     
 //
 //  Int_fitRangeGamma_Int
 //  Float_fitRangeGamma_Float
 //



int
Int_FitRange_Int(
  int curVal,
  int src1,
  int src2,
  int dst1,
  int dst2)
{

  //  int usesrc1, usesrc2; // These are the actual max and min values since the values of the arguments may be reversed.
  int out, srcDist, dstDist;
  float srcDstRatio;
  float outF = (float)curVal;


  // IF src1 < src2 we need to swap the Max and Min values for both the src and dst.
  if (src1 < src2) {
    int temp;

    temp = src2;
    src2 = src1;
    src1 = temp;

    temp = dst2;
    dst2 = dst1;
    dst1 = temp;
  }


  if (dst1 == dst2)   return(dst2);
  if (curVal >= src1)   return(dst1);
  if (curVal <= src2)   return(dst2);

  if (src1 == src2)     return((dst1 + dst2 + 1) >> 1);  // There are 3 reasonable choices. 
  //    1: return MIN  
  //    2: return MAX 
  //    3. return AVG  (dst1 + dst2 + 1)>>1)

// We need the distance between the source values as well as the distance between the destination values.
  srcDist = src1 - src2;

  if (dst1 > dst2) dstDist = dst1 - dst2;
  else             dstDist = dst2 - dst1;


  srcDstRatio = (float)((float)dstDist / (float)srcDist);


  // Now take care of the 2 cases that can occur depending on which way the source values and the dest values run.
  if (src2 < src1 && dst2 < dst1)   outF = ((curVal - src2) * srcDstRatio) + dst2;
  if (src2 < src1 && dst2 > dst1)   outF = ((curVal - src2) * (0.0F - srcDstRatio)) + dst2;


  out = (int)ROUND_TOINT(outF);

  // Clip the return value if necessary.
  // dst2 and dst1 are ints they can be returned as is.  
  if (dst2 < dst1) {
    if (out > dst1) return(dst1);
    if (out < dst2) return(dst2);
  }
  else {
    if (out < dst1) return(dst1);
    if (out > dst2) return(dst2);
  }

  return(out);
}


int
Int_FitRange_Float(
  float curVal,
  float src1,
  float src2,
  float dst1,
  float dst2)
{

  int   out;
  float outF = curVal;
  float srcDist, dstDist, srcDstRatio;

  // IF src1 < src2 we need to swap the Max and Min values for both the src and dst.
  if (src1 < src2) {
    float temp;

    temp = src2;
    src2 = src1;
    src1 = temp;

    temp = dst2;
    dst2 = dst1;
    dst1 = temp;
  }

  if (fabs(dst1 - dst2) < .01)  return((int)ROUND_TOINT(dst2));
  if (fabs(src1 - src2) < .01)  return((int)ROUND_TOINT(((dst1 + dst2) / 2.0F)));

  if (curVal >= src1 - .01)     return((int)ROUND_TOINT(dst1));
  if (curVal <= src2 + .01)     return((int)ROUND_TOINT(dst2));



  // We need the distance between the source values as well as the distance between the destination values.
  srcDist = src1 - src2;
  if (dst1 > dst2) dstDist = dst1 - dst2;
  else             dstDist = dst2 - dst1;

  srcDstRatio = dstDist / srcDist;

  // Now take care of the cases of that can occur depending on which way the source values and the dest values run.
  if (src2 < src1 && dst2 < dst1)   outF = ((curVal - src2) * srcDstRatio) + dst2;
  if (src2 < src1 && dst2 > dst1)   outF = ((curVal - src2) * (0.0F - srcDstRatio)) + dst2;


  if (dst2 < dst1) {
    if (outF > dst1) return((int)ROUND_TOINT(dst1));
    if (outF < dst2) return((int)ROUND_TOINT(dst2));
  }
  else {
    if (outF < dst1) return((int)ROUND_TOINT(dst1));
    if (outF > dst2) return((int)ROUND_TOINT(dst2));
  }

  out = (int)ROUND_TOINT(outF);
  return(out);
}




float
Float_FitRange_Float_NoClip(
  float curVal,
  float src1,
  float src2,
  float dst1,
  float dst2)
{

  float srcDist, dstDist, srcDstRatio;
  float outF = curVal;

  // IF src1 < src2 we need to swap the Max and Min values for both the src and dst.
  if (src1 < src2) {
    float temp;

    temp = src2;
    src2 = src1;
    src1 = temp;

    temp = dst2;
    dst2 = dst1;
    dst1 = temp;
  }

  srcDist = (float)fabs(src1 - src2);
  dstDist = (float)fabs(dst1 - dst2);

  if (dstDist < .01)  return(dst2);
  if (srcDist < .01)  return((dst1 + dst2) / 2.0F);


  srcDstRatio = dstDist / srcDist;


  // Now take care of the 2 different cases of that can occur depending on which way the dest values run.
  if (src2 < src1 && dst2 < dst1)   outF = ((curVal - src2) * srcDstRatio) + dst2;
  if (src2 < src1 && dst2 > dst1)   outF = ((curVal - src2) * (0.0F - srcDstRatio)) + dst2;

  return(outF);
}




float
Float_FitRange_Float(
  float curVal,
  float src1,
  float src2,
  float dst1,
  float dst2)
{

  float srcDist, dstDist, srcDstRatio;
  float outF = curVal;

  // IF src1 < src2 we need to swap the Max and Min values for both the src and dst.
  if (src1 < src2) {
    float temp;

    temp = src2;
    src2 = src1;
    src1 = temp;

    temp = dst2;
    dst2 = dst1;
    dst1 = temp;
  }

  srcDist = (float)fabs(src1 - src2);
  dstDist = (float)fabs(dst1 - dst2);

  if (curVal >= src1)  return(dst1);
  if (curVal <= src2)  return(dst2);

  if (dstDist < .01)     return(dst2);
  if (srcDist < .01)     return((dst1 + dst2) / 2.0F);

  srcDstRatio = dstDist / srcDist;

  // Now take care of the 2 cases that can occur depending on which way the dest values run.
  if (src2 < src1 && dst2 < dst1)   outF = ((curVal - src2) * srcDstRatio) + dst2;
  if (src2 < src1 && dst2 > dst1)   outF = ((curVal - src2) * (0.0F - srcDstRatio)) + dst2;


  // Clip outF to dst2 and dst1
  if (dst2 < dst1) {
    if (outF > dst1)   return(dst1);
    if (outF < dst2)   return(dst2);
  }
  else {
    if (outF < dst1)   return(dst1);
    if (outF > dst2)   return(dst2);
  }

  return(outF);
}





// In a C program, pow always takes two double values and returns a double value.  Microsoft Visual Studio
int
Int_fitRangeGamma_Int(
  int   curVal,
  int   src2,
  int   src1,
  float gamma,
  int   dst2,
  int   dst1)
{

  float  cMsmin = (float)(curVal - src2);
  float  smaxMsmin = (float)(src1 - src2);
  float  pval = (float)powf((float)(cMsmin / smaxMsmin), (float)(1.0F / gamma));
  float  mval = (float)(dst1 - dst2);

  int   out = curVal;
  float outF = (float)curVal;
  float srcDstRatio;
  int   srcDist, dstDist;

  // IF src1 < src2 we need to swap the Max and Min values for both the src and dst.
  if (src1 < src2) {
    int temp;

    temp = src2;
    src2 = src1;
    src1 = temp;

    temp = dst2;
    dst2 = dst1;
    dst1 = temp;
  }


  srcDist = src1 - src2;
  dstDist = dst1 - dst2;

  if (curVal >= src1)   return(dst1);
  if (curVal <= src2)   return(dst2);
  if (dst1 == dst2)   return(dst2);
  if (srcDist == 0)     return((dst1 + dst2 + 1) >> 1);  // There are 3 reasonable choices. 
  //    1: return MIN  
  //    2: return MAX 
  //    3. return AVG  (dst1 + dst2 + 1)>>1)


  dstDist = dst1 - dst2;

  srcDstRatio = (float)dstDist / (float)srcDist;


  // Now take care of the 2 cases that can occur depending on which way the dest values run.
//  if (src2 < src1 && dst2 < dst1)   outF = ((curVal - src2) * srcDstRatio)          + dst2;
//  if (src2 < src1 && dst2 > dst1)   outF = ((curVal - src2) * (0.0F - srcDstRatio)) + dst2;



//cMsmin    = (float)(curVal-src2);
//smaxMsmin = (float)(src1-src2);
//pval      = (float)powf((float)(cMsmin / smaxMsmin), (float)(1.0F / gamma));
//mval      = (float)(dst1-dst2);


  cMsmin = (float)(curVal - src2);
  smaxMsmin = (float)(src1 - src2);

  pval = (float)powf((float)(cMsmin / smaxMsmin), (float)(1.0F / gamma));
  mval = (float)(dst1 - dst2);

  outF = pval * mval + (float)dst2;

  out = (int)ROUND_TOINT(outF);

  // Clip the return value if necessary.
  // dst2 and dst1 are ints they can be returned as is.  
  if (dst2 < dst1) {
    if (out > dst1) return(dst1);
    if (out < dst2) return(dst2);
  }
  else {
    if (out < dst1) return(dst1);
    if (out > dst2) return(dst2);
  }

  return(out);
}





float
Float_fitRangeGamma_Float(
  float curVal,
  float src2,
  float src1,
  float gamma,
  float dst2,
  float dst1)
{

  float out;
  float srcDist, dstDist;
  float cMsmin = curVal - src2;
  float smaxMsmin = src1 - src2;
  float pval;
  float mval;

  // IF src1 < src2 we need to swap the Max and Min values for both the src and dst.
  if (src1 < src2) {
    float temp;

    temp = src2;
    src2 = src1;
    src1 = temp;

    temp = dst2;
    dst2 = dst1;
    dst1 = temp;
  }

  srcDist = (float)fabs(src1 - src2);
  dstDist = (float)fabs(dst1 - dst2);

  if (dstDist < .01)  return(dst2);
  if (srcDist < .01)  return((dst1 + dst2) / 2.0F);


  if (curVal >= src1) return(dst1);
  if (curVal <= src2) return(dst2);

  cMsmin = curVal - src2;
  smaxMsmin = src1 - src2;

  if (smaxMsmin < .01F)  smaxMsmin = .01F;
  if (gamma < .01F)  gamma = .01F;
  pval = (float)pow((float)(cMsmin / smaxMsmin), (float)(1.0F / gamma));
  mval = dst1 - dst2;

  out = pval * mval + dst2;


  // Clip the return value if necessary.
  if (dst2 < dst1) {
    if (out > dst1) return(dst1);
    if (out < dst2) return(dst2);
  }
  else {
    if (out < dst1) return(dst1);
    if (out > dst2) return(dst2);
  }

  return(out);

}














/*

//Interpolation with Cubic Splines
//
//Adobe Photoshop, Gimp and many other graphic programs have a tool called Curves. You have several "knots" and while you move them, it computes a curve between. I always wondered, how is this curve computed.

  function CSPL(){};

  CSPL._gaussJ = {};
  CSPL._gaussJ.solve = function(A, x)  // in Matrix, out solutions
  {
    var m = A.length;
    for(var k=0; k<m; k++)  // column
    {
      // pivot for column
      var i_max = 0; var vali = Number.NEGATIVE_INFINITY;
      for(var i=k; i<m; i++) if(A[i][k]>vali) { i_max = i; vali = A[i][k];}
      CSPL._gaussJ.swapRows(A, k, i_max);

      if(A[i_max][k] == 0) console.log("matrix is singular!");

      // for all rows below pivot
      for(var i=k+1; i<m; i++)
      {
        for(var j=k+1; j<m+1; j++)
          A[i][j] = A[i][j] - A[k][j] * (A[i][k] / A[k][k]);
        A[i][k] = 0;
      }
    }

    for(var i=m-1; i>=0; i--)  // rows = columns
    {
      var v = A[i][m] / A[i][i];
      x[i] = v;
      for(var j=i-1; j>=0; j--)  // rows
      {
        A[j][m] -= A[j][i] * v;
        A[j][i] = 0;
      }
    }
  }
  CSPL._gaussJ.zerosMat = function(r,c) {var A = []; for(var i=0; i<r; i++) {A.push([]); for(var j=0; j<c; j++) A[i].push(0);} return A;}
  CSPL._gaussJ.printMat = function(A){ for(var i=0; i<A.length; i++) console.log(A[i]); }
  CSPL._gaussJ.swapRows = function(m, k, l) {var p = m[k]; m[k] = m[l]; m[l] = p;}


  CSPL.getNaturalKs = function(xs, ys, ks)  // in x values, in y values, out k values
  {
    var n = xs.length-1;
    var A = CSPL._gaussJ.zerosMat(n+1, n+2);

    for(var i=1; i<n; i++)  // rows
    {
      A[i][i-1] = 1/(xs[i] - xs[i-1]);

      A[i][i  ] = 2 * (1/(xs[i] - xs[i-1]) + 1/(xs[i+1] - xs[i])) ;

      A[i][i+1] = 1/(xs[i+1] - xs[i]);

      A[i][n+1] = 3*( (ys[i]-ys[i-1])/((xs[i] - xs[i-1])*(xs[i] - xs[i-1]))  +  (ys[i+1]-ys[i])/ ((xs[i+1] - xs[i])*(xs[i+1] - xs[i])) );
    }

    A[0][0  ] = 2/(xs[1] - xs[0]);
    A[0][1  ] = 1/(xs[1] - xs[0]);
    A[0][n+1] = 3 * (ys[1] - ys[0]) / ((xs[1]-xs[0])*(xs[1]-xs[0]));

    A[n][n-1] = 1/(xs[n] - xs[n-1]);
    A[n][n  ] = 2/(xs[n] - xs[n-1]);
    A[n][n+1] = 3 * (ys[n] - ys[n-1]) / ((xs[n]-xs[n-1])*(xs[n]-xs[n-1]));

    CSPL._gaussJ.solve(A, ks);
  }

  CSPL.evalSpline = function(x, xs, ys, ks)
  {
    var i = 1;
    while(xs[i]<x) i++;

    var t = (x - xs[i-1]) / (xs[i] - xs[i-1]);

    var a =  ks[i-1]*(xs[i]-xs[i-1]) - (ys[i]-ys[i-1]);
    var b = -ks[i  ]*(xs[i]-xs[i-1]) + (ys[i]-ys[i-1]);

    var q = (1-t)*ys[i-1] + t*ys[i] + t*(1-t)*(a*(1-t)+b*t);
    return q;
  }

*/
