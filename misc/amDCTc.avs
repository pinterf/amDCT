###
#  This file contains the amDCTc() script routine.
#
# Changelog:
#   (20241128) fix sharpTAmt typo

###
#  REQUIRED ROUTINES
#  
#  aWarpSharp2.dll  available  from http://sites.google.com/site/pavelsx2/
#  MaskTools2.dll   available  from http://avisynth.org/mediawiki/MaskTools2
#
###
#  RESTRICTIONS
#  
#  The Input clip must be evenly divisible by 16 in both height and width.

###
#  ARGUMENTS
#
#  amDCTc() provides a wrapper around amDCT() that takes a clip with the luma deblocked and uses amDCTc() to deblock the chroma.
#              The arguments quant, shift, matrix, and qtype  are the same as those in amDCT() and have the same defaults.
#              The arguments Cpr, cWarpToLuma and downSize are specific to amDCTc().
#
#
#              cWarpToLuma (0...8) uses aWarpSharp2() to warp the chroma to the luma. Default 0 no warping 8 strong warping.
#                                  This will move the chroma from high luma areas toward strong low luma edges which is
#                                  useful to reduce chroma bleeding.
#                                  Places where the chroma has been moved from may be visibly desaturated.  
#                                  NOTE: The amDCTc() argument Cpr will have NO effect on chroma desaturation caused by
#                                  cWarpToLuma. 
#
#              downSize (1...4) This specifies how much the image should be shrunk before deblocking.
#                               Default 1 no downsizing.  
#                                       2 reduce image to 1/2 it's original size before processing.
#                                       3 reduce image to 1/3 it's original size before processing.
#                                       4 reduce image to 1/4 it's original size before processing.
#                               This can be useful if you have really bad chroma blocking. 
#

###
#  GETTING STARTED
#
#  First all parameters default to reasonable values so amDCTc() should work, though it will provide only very weak deblocking.  
#  The most important parameter is quant, which is the primary control of deblocking strength.  
#  If you can not get as much deblocking as desired then try using matrix 8.
#  If you still can't get enough deblocking then try qtype=2 or 4.
#  If you still can't get enough deblocking then increase downSize. 
#  When you get enough deblocking find an area, usually in a darker part of the picture, that the chroma has washed out and start increasing expand
#  until the chroma returns.    
#  When you have gotten this far and you have chroma bleeding around strong luma edges then you can try increasing cWarpToLuma 
#  to see if it will help reduce it.  Note that cWarpToLuma especially at higher settings will tend to washout the chroma around the edges.




###
#  This routine takes a clip that has already had the luma deblocked and uses amDCTc to deblock the chroma.
# Defaults amDCTc(quant=3, adapt=3, quality=4, shift=5, matrix=8, qtype=1, expand=0,  sharpWPos=5, sharpTPos=7, sharpWAmt=0,  sharpTAmt=0, cWarpToLuma=0, downSize=1, ncpu=1)
Function amDCTc(clip clp, int "quant", int "adapt", int "quality", int "shift", int "matrix", int "qtype", int "expand", int  "sharpWPos", int "sharpTPos", int "sharpWAmt", int "sharpTAmt", int "cWarpToLuma", int "downSize", int "ncpu") {
quant       = default( quant,        3 )  
adapt       = default( adapt,    quant ) 
quality     = default( quality,      4 )  
shift       = default( shift,        5 )  
matrix      = default( matrix,       4 )  
qtype       = default( qtype,        1 )  
expand      = default( expand,       0 )  
sharpWPos   = default( sharpWPos,    0 )  
sharpTPos   = default( sharpTPos,    0 )  
sharpWAmt   = default( sharpWAmt,    0 )  
sharpTAmt   = default( sharpTAmt,    0 )  
cWarpToLuma = default( cWarpToLuma,  0 ) 
downSize    = default( downSize,     1 ) 
ncpu        = default( ncpu,         1 ) 


Assert( ( quant       >= 0  &&  quant       <=  31 ) ? true : false, chr(10) + "'quant' wrong value! [0-31]" + chr(10))
Assert( ( adapt       >= 0  &&  adapt       <=  31 ) ? true : false, chr(10) + "'adapt' wrong value! [0-31]" + chr(10))
Assert( ( quality     >= 0  &&  quality     <=   4 ) ? true : false, chr(10) + "'quality' wrong value! [0-4]" + chr(10))
Assert( ( shift       >= 1  &&  shift       <=   5 ) ? true : false, chr(10) + "'shift' wrong value! [1-4]" + chr(10))
Assert( ( matrix      >= 1  &&  matrix      <= 110 ) ? true : false, chr(10) + "'matrix' wrong value! [1-110]" + chr(10))
Assert( ( qtype       >= 0  &&  qtype       <=   4 ) ? true : false, chr(10) + "'qtype' wrong value! [0-4]" + chr(10))
Assert( ( expand      >= 0  &&  expand      <=  31 ) ? true : false, chr(10) + "'expand' wrong value! [0-31]" + chr(10))
Assert( ( sharpWPos   >= 0  &&  sharpWPos   <=   5 ) ? true : false, chr(10) + "'sharpWPos' wrong value! [5-7]" + chr(10))
Assert( ( sharpTPos   >= 0  &&  sharpTPos   <=   7 ) ? true : false, chr(10) + "'sharpTPos' wrong value! [5-7]" + chr(10))
Assert( ( sharpWAmt   >= 0  &&  sharpWAmt   <=  31 ) ? true : false, chr(10) + "'sharpWAmt' wrong value! [0-31]" + chr(10))
Assert( ( sharpTAmt   >= 0  &&  sharpTAmt   <=  31 ) ? true : false, chr(10) + "'sharpTAmt' wrong value! [0-31]" + chr(10))
Assert( ( cWarpToLuma >= 0  &&  cWarpToLuma <=   8 ) ? true : false, chr(10) + "'cWarpToLuma' wrong value! [0-8]" + chr(10))
Assert( ( downSize    >= 1  &&  downSize    <=   5 ) ? true : false, chr(10) + "'downSize' wrong value! [1-4]" + chr(10))
Assert( ( ncpu        >= 1  &&  ncpu        <=   4 ) ? true : false, chr(10) + "'ncpu' wrong value! [1-4]" + chr(10))
Assert( ( clp.width   == m16(clp.width)            ) ? true : false, chr(10) + "'clip width'  must be a multiple of 16" + chr(10))
Assert( ( clp.height  == m16(clp.height)           ) ? true : false, chr(10) + "'clip height' must be a multiple of 16" + chr(10))

   ow  = clp.width
   oh  = clp.height


   sd1  = clp 
   clp2 = clp
   sd1  = (downSize    > 1) ? sd1.Lanczos4Resize(m32(ow/downSize), m32(oh/downSize)) : sd1
   sd1  = (cWarpToLuma > 0) ? sd1.colorWarpToLuma(strength=cWarpToLuma)              : sd1

   u2  = sd1.UToY()
   v2  = sd1.VToY()

   cw   = u2.width
   ch   = u2.height


   u2s  = u2.amDCT(quant=quant, adapt=adapt, quality=quality, shift=shift, matrix=matrix, qtype=qtype, ncpu=ncpu, expand=expand,  sharpWpos=sharpWpos, sharpTpos=sharpTpos, sharpWAmt=sharpWAmt,  sharpTAmt=sharpTAmt) 
   v2s  = v2.amDCT(quant=quant, adapt=adapt, quality=quality, shift=shift, matrix=matrix, qtype=qtype, ncpu=ncpu, expand=expand,  sharpWpos=sharpWpos, sharpTpos=sharpTpos, sharpWAmt=sharpWAmt,  sharpTAmt=sharpTAmt) 


   u2 = (downSize > 1) ? u2s.Lanczos4Resize(ow/2, oh/2) : u2s
   v2 = (downSize > 1) ? v2s.Lanczos4Resize(ow/2, oh/2) : v2s

#   u2 = (downSize > 1) ? u2.bicubicResize(ow/2, oh/2) : u2
#   v2 = (downSize > 1) ? v2.bicubicResize(ow/2, oh/2) : v2

   sd1 = YToUV(u2, v2).MergeLuma(clp2)


   return(sd1)
}


###
# m4  "mod-4"   see "Crop restrictions" in http://avisynth.org/mediawiki/Crop
#               m4 is used in may scripts that do resizing.
# m8  "mod-8"   is used so that cropping will occur on luma block (8x8) boundaries
#               and that resizing will produce an integer number of luma blocks.
# m16 "mod-16"  is used so that cropping will occur on YV12 chroma block (16x16) boundaries
#               and that resizing will produce an integer number of chroma blocks.
#function  m4(float x) {return(x<16?16:int(round(x/4.0) *4))}
#function  m8(float x) {return(x<16?16:int(round(x/8.0) *8))}
function m16(float x) {return(x<32?32:int(round(x/16.0)*16))}
function m32(float x) {return(x<64?64:int(round(x/32.0)*32))}





###
# 
#  This routine moves the color information toward the edges in the luma
#
# Defaults colorWarpToLuma(strength=1)
function colorWarpToLuma(clip clp, int "strength") {
strength = default( strength, 1 )

  thresh = (strength ==  0)  ?    1                   
\        : (strength ==  1)  ?   24
\        : (strength ==  2)  ?   32 
\        : (strength ==  3)  ?   48  
\        : (strength ==  4)  ?   64  
\        : (strength ==  5)  ?   96  
\        : (strength ==  6)  ?  128  
\        : (strength ==  7)  ?  192  
\        :                      255  

    blur = strength + 2
    warpedColor = clp.aWarpSharp2(thresh=thresh, blur=blur, type=1, depth=14, chroma=4)

    mt_merge(clp, clp, warpedColor, y=2, u=5, v=5) 

    return(last)
}



