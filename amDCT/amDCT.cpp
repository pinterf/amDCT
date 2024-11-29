
#include "windows.h"
#include "avisynth.h"

#include "amDCT.h"

#define ALIGN 16


/****************************
 * The following is the header definitions.
 * For larger projects, move this into a .h file
 * that can be included.
 ****************************/



class amDCT : public GenericVideoFilter {
  // amDCT defines the name of your filter class.
  // This name is only used internally, and does not affect the name of your filter or similar.
  // This filter extends GenericVideoFilter, which incorporates basic functionality.
  // All functions present in the filter must also be present here.

  // define the parameter variable
  PClip pf1;
  PClip bf1;
  int   quant;
  int   adapt;
  int   shift;
  int   matrix;
  int   qtype;
  int  sharpWPos;
  int  sharpWAmt;
  int  expand;
  int   sharpTPos;
  int   sharpTAmt;
  int   quality;
  int   brightStart;
  int  brightAmt;
  int   darkStart;
  int  darkAmt;
  int   showMask;
  int   T2;
  int   ncpu;

public:
  // This defines that these functions are present in your class.
  // These functions must be that same as those actually implemented.
  // Since the functions are "public" they are accessible to other classes.
  // Otherwise they can only be called from functions within the class itself.
  //  env->AddFunction("amDCT", "c[pf1]c[bf1]c[quant]i[adapt]i[shift]i[matrix]i[qtype]i[sharpWPos]i[sharpWAmt]i[expand]i[sharpTPos]i[sharpTAmt]i[quality]i[brightStart]i[brightAmt]i[darkStart]i[darkAmt]i[showMask]i[T2]i[ncpu]i", Create_amDCT, 0);

  amDCT(PClip _child, const PClip _pf1, const PClip _bf1, const int _quant, const int _adapt, const int _shift, const int _matrix, const int _qtype, const int _sharpWPos, const int _sharpWAmt, const int _expand, const int _sharpTPos, const int _sharpTAmt, const int _quality, const int _brightStart, const int _brightAmt, const int _darkStart, const int _darkAmt, const int _showMask, const int _T2, const int _ncpu, IScriptEnvironment* env);
  // This is the constructor. It does not return any value, and is always used,
  //  when an instance of the class is created.
  // Since there is no code in this, this is the definition.

  ~amDCT();
  // The is the destructor definition. This is called when the filter is destroyed.


  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  // This is the function that AviSynth calls to get a given frame.
  // So when this functions gets called, the filter is supposed to return frame n.
};

/***************************
 * The following is the implementation
 * of the defined functions.
 ***************************/

 //Here is the actual constructor code used
amDCT::amDCT(PClip _child, PClip _pf1, PClip _bf1, const int _quant, const int _adapt, const int _shift, const int _matrix, const int _qtype, const int _sharpWPos, const int _sharpWAmt, const int _expand, const int _sharpTPos, const int _sharpTAmt, const int _quality, const int _brightStart, const int _brightAmt, const int _darkStart, const int _darkAmt, const int _showMask, const int _T2, const int _ncpu, IScriptEnvironment* env) :
  GenericVideoFilter(_child), pf1(_pf1), bf1(_bf1), quant(_quant), adapt(_adapt), shift(_shift), matrix(_matrix), qtype(_qtype), sharpWPos(_sharpWPos), sharpWAmt(_sharpWAmt), expand(_expand), sharpTPos(_sharpTPos), sharpTAmt(_sharpTAmt), quality(_quality), brightStart(_brightStart), brightAmt(_brightAmt), darkStart(_darkStart), darkAmt(_darkAmt), showMask(_showMask), T2(_T2), ncpu(_ncpu) {

  // This is the implementation of the constructor.
  // The child clip (source clip) is inherited by the GenericVideoFilter,
  //  where the following variables gets defined:
  //   PClip child;   // Contains the source clip.
  //   VideoInfo vi;  // Contains video-info on the source clip.
  if (!vi.IsYV12()) // is input planar?
    env->ThrowError("amDCT: input to filter must be in YV12");

  if ((quant < 0) || (quant > 31)) // was 31
    env->ThrowError("amDCT: quant out of range (0-31)");

  if ((adapt < 0) || (adapt > 31))
    env->ThrowError("amDCT: adapt out of range (0-31)");

  if ((shift < 0) || (shift > 5)) // This will change to 5 after testing
    env->ThrowError("amDCT: shift out of range (0-5)");

  if ((matrix < 0) || (matrix > 255))
    env->ThrowError("amDCT: matrix out of range (0-255)");

  if ((qtype < 1) || (qtype > 24))
    //  if ((qtype<1) || (qtype>14))
    env->ThrowError("amDCT: qtype out of range (1-4, 11-14)");

  if (((sharpWPos < 0) || (sharpWPos > 7)) && (sharpWPos != 255))
    env->ThrowError("amDCT: sharpWPos out of range (0-7)");

  if (((sharpWAmt < 0) || (sharpWAmt > 31)) && (sharpWAmt != 255))
    env->ThrowError("amDCT: sharpWAmt out of range (0-31)");

  if ((expand < 0) || (expand > 31))
    env->ThrowError("amDCT: expand out of range (0 to 31)");

  if (((sharpTPos < 0) || (sharpTPos > 7)) && (sharpTPos != 255))
    env->ThrowError("amDCT: sharpTPos out of range (0-7)");

  if (((sharpTAmt < 0) || (sharpTAmt > 31)) && (sharpTAmt != 255))
    env->ThrowError("amDCT: sharpTAmt out of range (0-31)");

  if ((quality < 1) || (quality > 6))
    env->ThrowError("amDCT: quality out of range (1 to 6 default=2)");

  if ((sharpTPos != 0 && sharpTPos != 255) && sharpWPos != 0 && (sharpWPos != 255) && (sharpWPos > sharpTPos))
    env->ThrowError("amDCT: sharpTPos must be 0 or >= sharpWPos");

  if ((brightStart < 0) || (brightStart > 255))
    env->ThrowError("amDCT: brightStart out of range (0-255)");

  if ((brightAmt < 0) || (brightAmt > 31))
    env->ThrowError("amDCT: brightAmt out of range (0-31)");

  if ((darkStart < 0) || (darkStart > 255))
    env->ThrowError("amDCT: darkStart out of range (0-255)");

  if ((darkAmt < 0) || (darkAmt > 31))
    env->ThrowError("amDCT: darkAmt out of range (0-31)");

  if ((showMask < 0) || (showMask > 255))
    env->ThrowError("amDCT: showMask out of range (0-255)");

  if ((T2 < 0) || (T2 > 255))
    env->ThrowError("amDCT: T2 out of range (0-255)");

  if ((ncpu < 1) || (ncpu > 4))
    env->ThrowError("amDCT: ncpu out of range (1-4)");

}

// This is where any actual destructor code used goes
amDCT::~amDCT() {
  // This is where you can deallocate any memory you might have used.
}


PVideoFrame __stdcall amDCT::GetFrame(int n, IScriptEnvironment* env) {
  // This is the implementation of the GetFrame function.
  // See the header definition for further info.

  PVideoFrame src = child->GetFrame(n, env);

  //  PVideoFrame pf1Src = NULL;  // WAS src
  //  if (pf1 != NULL) pf1Src = pf1->GetFrame(n, env);
  //  PVideoFrame bf1Src = NULL;
  //  if (bf1 != NULL) bf1Src = bf1->GetFrame(n, env);


     // Request frame 'n' from the child (source) clip.
  PVideoFrame dst = env->NewVideoFrame(vi);
  // Construct a frame based on the information of the current frame
  // contained in the "vi" struct.

  const unsigned char* srcp = src->GetReadPtr();

  //  const unsigned char* pf1Srcp = NULL; // WAS srcp
  //  const unsigned char* bf1Srcp = NULL;
  //  if (pf1 != NULL)  pf1Srcp = pf1Src->GetReadPtr();
  //  if (bf1 != NULL)  bf1Srcp = bf1Src->GetReadPtr();

    //const unsigned char* QmaskSrcp  = srcp; //QmaskSrc->GetReadPtr();
    // Request a Read pointer from the source frame.
    // This will return the position of the upper-left pixel in YUY2 images,
    // and return the lower-left pixel in RGB.
    // RGB images are stored upside-down in memory.
    // You should still process images from line 0 to height.

  unsigned char* dstp = dst->GetWritePtr();
  // Request a Write pointer from the newly created destination image.
  // You can request a writepointer to images that have just been
  // created by NewVideoFrame. If you receive a frame from PClip->GetFrame(...)
  // you must call env->MakeWritable(&frame) to receive a valid write pointer.

  const int dst_pitch = dst->GetPitch();
  // Requests pitch (length of a line) of the destination image.
  // For more information on pitch see: http://www.avisynth.org/index.php?page=WorkingWithImages
  // (short version - pitch is always equal to or greater than width to allow for seriously fast assembly code)

  const int dst_width = dst->GetRowSize();
  // Requests row-size (number of used bytes in a line.
  // See the link above for more information.

  const int dst_height = dst->GetHeight();
  // Requests the height of the destination image.

  const    int src_pitch = src->GetPitch();
  unsigned int src_width = src->GetRowSize();
  unsigned int src_height = src->GetHeight();


  // We want pf1 to be the size of the src frame that was passed in.
//         int pf1_pitch  = 0; // was src_pitch;
//  unsigned int pf1_width  = 0; // src_width;
//  unsigned int pf1_height = 0; // src_height;
//  if (pf1 != NULL) {
//    pf1_pitch  = pf1Src->GetPitch();
//    pf1_width  = pf1Src->GetRowSize();
//    pf1_height = pf1Src->GetHeight();
//  }
//
//          int bf1_pitch  = 0; // src_pitch;
//  unsigned int bf1_width  = 0; // src_width;
//  unsigned int bf1_height = 0; // src_height;
//  if (bf1 != NULL) {
//    bf1_pitch  = bf1Src->GetPitch();
//    bf1_width  = bf1Src->GetRowSize();
//    bf1_height = bf1Src->GetHeight();
//  }

  // We check the src frame to make sure it is mod16 width.
  // Mod8 is required for the core amDCT algorithm to work.
  // Mod16 keeps the start of each row aligned at multiples of 16 bytes to optimize mmx
  unsigned int w, h;
  //w = src_width;
  //if (src_width != (w>>4)<<4) src_width = ((src_width + 15)>>4)<<4;
  src_width = (src_width + 15) & ~15;  // 15 = ALIGN-1  // ALIGN must be a power of 2

  // check parameters for valid values
  if ((quant < 0) || (quant > 31))
    env->ThrowError("amDCT: quant out of range");

  if ((adapt < 0) || (adapt > 31))
    env->ThrowError("amDCT: adapt out of range");

  if ((shift < 0) || (shift > 5))
    env->ThrowError("amDCT: shift out of range");

  if ((matrix < 0) || (matrix > 255))
    env->ThrowError("amDCT: matrix out of range");

  if ((qtype < 1) || (qtype > 24))
    //  if ((qtype<1) || (qtype>14))
    env->ThrowError("amDCT: qtype out of range (1-4, 11-14)");

  if (((sharpWPos < 0) || (sharpWPos > 7)) && (sharpWPos != 255))
    env->ThrowError("amDCT: sharpWPos out of range (0-7)");

  if (((sharpWAmt < 0) || (sharpWAmt > 31)) && (sharpWAmt != 255))
    env->ThrowError("amDCT: sharpWAmt out of range (0-31)");

  if ((expand < 0) || (expand > 31))
    env->ThrowError("amDCT: expand out of range (0 to 31)");

  if (((sharpTPos < 0) || (sharpTPos > 7)) && (sharpTPos != 255))
    env->ThrowError("amDCT: sharpTPos out of range (0-7)");

  if (((sharpTAmt < 0) || (sharpTAmt > 31)) && (sharpTAmt != 255))
    env->ThrowError("amDCT: sharpTAmt out of range (0-31)");

  if ((quality < 1) || (quality > 6))
    env->ThrowError("amDCT: quality out of range (1 to 6 default=2)");

  if ((sharpTPos != 0 && sharpTPos != 255) && sharpWPos != 0 && (sharpWPos != 255) && (sharpWPos > sharpTPos))
    env->ThrowError("amDCT: sharpTPos must be 0 or >= sharpWPos");

  if ((brightStart < 0) || (brightStart > 255))
    env->ThrowError("amDCT: brightStart out of range (0-255)");

  if ((brightAmt < 0) || (brightAmt > 31))
    env->ThrowError("amDCT: brightAmt out of range (0-31)");

  if ((darkStart < 0) || (darkStart > 255))
    env->ThrowError("amDCT: darkStart out of range (0-255)");

  if ((darkAmt < 0) || (darkAmt > 31))
    env->ThrowError("amDCT: darkAmt out of range (0-31)");

  if ((showMask < 0) || (showMask > 255))
    env->ThrowError("amDCT: showMask out of range (0-255)");

  if ((T2 < 0) || (T2 > 255))
    env->ThrowError("amDCT: T2 out of range (0-255)");

  if ((ncpu < 1) || (ncpu > 4))
    env->ThrowError("amDCT: ncpu out of range (1-4)");




  // Set the "dependency default values".
  // Dependency default values depend on the values of other arguments.
  // The "AVSValue __cdecl Create_amDCT() constructor" at the bottom of this file
  // has set the dependency default values to an impossible value and the used values are computed here.

  // Set the default sharpWPos and sharpTPos if only sharpWAmt or sharpTAmt is specified.
  if (sharpWPos == 255 && sharpTPos != 255) sharpWPos = sharpTPos;
  if (sharpTPos == 255 && sharpWPos != 255) sharpTPos = sharpWPos;

  // If neither sharpWPos or sharpTPos have been set then use their defaults.
  if (sharpWPos == 255 && sharpTPos == 255 && sharpWAmt != 255)  sharpWPos = 5;
  if (sharpWPos == 255 && sharpTPos == 255 && sharpTAmt != 255)  sharpTPos = 7;



  if (sharpWAmt == 255 && sharpTAmt != 255) sharpWAmt = sharpTAmt;
  if (sharpTAmt == 255 && sharpWAmt != 255) sharpTAmt = sharpWAmt;

  if (sharpWAmt == 255 && sharpTAmt == 255 ||
    sharpWPos == 255 && sharpTPos == 255) {
    //  if (sharpWAmt == 255 || sharpTAmt == 255 ||
    //      sharpWPos == 255 || sharpTPos == 255) {
    sharpWAmt = 0;
    sharpTAmt = 0;
    sharpWPos = 0;
    sharpTPos = 0;
  }

  if (sharpTPos == 255 && sharpWPos != 255) {
    sharpTPos = sharpWPos;
    sharpTAmt = sharpWAmt;
  }

  if (sharpTPos != 255 && sharpWPos == 255) {
    sharpWPos = sharpTPos;
    sharpWAmt = sharpTAmt;
  }


  if (brightStart >= 255) brightAmt = 0;
  if (darkStart <= 0)   darkAmt = 0;



  //  if (pf1 != NULL && ((pf1_width != src_width) || (pf1_height != src_height)))// {
  //    env->ThrowError("pf1 size != Src size");
  //    return src;
  //    }

  //  if (bf1 != NULL && ((bf1_width != src_width) || (bf1_height != src_height)))// {
  //    env->ThrowError("bf1 size != Src size");
  //    return src;
  //    }


  //  PClip AsClip() const { _ASSERTE(IsClip()); return IsClip()?clip:0; }
  //{ _ASSERTE(1); return (dst) ? dst : 0; }
  //const { _ASSERTE(1)}; return 1 ? 1 : 0;



  if (vi.IsYV12()) {

    //////// AviSynth sample code comment
    // This code deals with YV12 colourspace where the Y, U and V information are
    // stored in completely separate memory areas
    //
    // This colourspace is the most memory efficient but usually requires 3 separate loops
    // However, it can actually be easier to deal with than YUY2 depending on your filter algorithm
    //
    //for (h=0; h < src_height;h++) {       // Loop from top line to bottom line (Sames as YUY2.
    //  for (w = 0; w < src_width; w++)     // Loop from left side of the image to the right side.
    //    *(dstp + w) = *(srcp + w);      // Copy each byte from source to destination.
    //  srcp = srcp + src_pitch;            // Add the pitch (note use of pitch and not width) of one line (in bytes) to the source image.
    //  dstp = dstp + dst_pitch;            // Add the pitch of one line (in bytes) to the destination.
    //}
    //////// end AviSynth sample code comment

  // So first of all deal with the Y Plane
    srcp = src->GetReadPtr();
    dstp = dst->GetWritePtr();


    unsigned int   height = src_height;
    unsigned int   width = src_width;




    /*
    //      FUTURE TRY MULTIPLE MOTION COMPENSATED FRAMES from MVTools2
        unsigned char       *pf1Buf  = NULL;
        unsigned char       *pf1Bufp = NULL;
        unsigned char       *ppf1    = NULL;

        unsigned char       *bf1Buf  = NULL;
        unsigned char       *bf1Bufp = NULL;
        unsigned char       *pbf1    = NULL;
        if (pf1 != NULL) {
          pf1Buf  = (unsigned char *)_aligned_malloc(frameSize, ALIGN);
          pf1Bufp = pf1Buf;
          ppf1    = pf1Bufp;
          memset(pf1Buf,  0, frameSize);
        }

        if (bf1 != NULL) {
          bf1Buf  = (unsigned char *)_aligned_malloc(frameSize, ALIGN);
          bf1Bufp = bf1Buf;
          pbf1    = bf1Bufp;
          memset(bf1Buf,  0, frameSize);
        }
    */



    unsigned char* psrc = (unsigned char*)srcp;
    height = src_height;
    width = src_width;

    //    ppf1 = pf1Bufp;  // For testing the use of motion compensated frames.
    //    pbf1 = bf1Bufp;
        //if (aaDCT(psrc, ppf1, pbf1, dstp, height, width, dst_height, dst_pitch, quant, adapt, shift, matrix, qtype, expand, sharpWPos, sharpWAmt, sharpTPos, sharpTAmt, quality, brightStart, brightAmt, showMask, T2, ncpu) > 0) {
    if (amDCTmain(psrc, dstp, height, width, src_pitch, dst_height, dst_width, dst_pitch, quant, adapt, shift, matrix, qtype, expand, sharpWPos, sharpWAmt, sharpTPos, sharpTAmt, quality, brightStart, brightAmt, darkStart, darkAmt, showMask, T2, ncpu) == 1) {
      env->ThrowError("amDCT: out of memory");

    }

    // end of Y plane Code



    // This section of code deals with the U and V planes of planar formats (e.g. YV12)
    // So first of all we have to get the additional info on the U and V planes

    const unsigned int dst_pitchUV = dst->GetPitch(PLANAR_U);   // The pitch,height and width information
    const unsigned int dst_widthUV = dst->GetRowSize(PLANAR_U); // is guaranteed to be the same for both
    const unsigned int dst_heightUV = dst->GetHeight(PLANAR_U);   // the U and V planes so we only the U
    const unsigned int src_pitchUV = src->GetPitch(PLANAR_U);   // plane values and use them for V as
    const unsigned int src_widthUV = src->GetRowSize(PLANAR_U); // well
    const unsigned int src_heightUV = src->GetHeight(PLANAR_U);   //

    //Copy U plane src to dst
    srcp = src->GetReadPtr(PLANAR_U);
    dstp = dst->GetWritePtr(PLANAR_U);

    for (h = 0; h < src_heightUV; h++) {
      for (w = 0; w < src_widthUV; w++)
        //        *(dstp + w) = 128;   // Debug This returns greyscale
        *(dstp + w) = *(srcp + w);
      srcp = srcp + src_pitchUV;
      dstp = dstp + dst_pitchUV;
    }
    // end copy U plane src to dst



    //Copy V plane src to dst
    srcp = src->GetReadPtr(PLANAR_V);
    dstp = dst->GetWritePtr(PLANAR_V);

    for (h = 0; h < src_heightUV; h++) {
      for (w = 0; w < src_widthUV; w++)
        //        *(dstp + w) = 128;   // debug This returns greyscale
        *(dstp + w) = *(srcp + w);
      srcp = srcp + src_pitchUV;
      dstp = dstp + dst_pitchUV;
    }
    // end copy V plane src to dst




//  if (pf1 != NULL) _aligned_free(pf1Buf);
//  if (bf1 != NULL) _aligned_free(bf1Buf);

  }

  // As we now are finished processing the image, we return the destination image.
  return dst;
}


// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,
// based on the arguments received.

//  Parameters:
//  pf1         = NULL        // Motion Compensated previous frame.  NOT CURRENTLY IMPLIMENTED
//  bf1         = NULL        // Motion Compensated next frame.      NOT CURRENTLY IMPLIMENTED
//  quant       = 0   [0..31]    // Strength of smoothing processing
//  adapt       = 0   [0..31]    // Max amount increase to quant amDCT will add depending on the amount of blocking detected.
//  shift       = 3   [0..5]     // 0 does no re sampling, 1 does 4 , 2 does 8, 3 does 16, 4 does 32, 5 does 64 (max possible)
//  matrix      = 1   [1..110]   // 1-4 are standard and increase in strength. Above that are for fun.
//  qtype       = 1   [1..4]   // quant dequant type, 1=mpeg intra, 2=h263 intra, 3=mpeg inter, 4= h263 inter
//  sharpWPos   = 0   [5..7]     // width of sharpening
//  sharpWAmt   = 0   [0..31]     // strength at the width specified by sharpWPos.
//  expand      = 0   [0..31]    // 0 does nothing, >=1 sharpens the image by expanding the luma range during quant dequant processing.
//  sharpTPos   = 0   [5..7]   // width of sharpening
//  sharpTAmt   = 0   [1..31]   // strength at the width specified by sharpTPos.
//  quality     = 2   [1..6]     // This is a speed vs quality trade off. 0 uses default. 1 is fastest  x is slowest.
//  brightStart = 255 [0..255]   // Extra smoothing in brightest areas is decreased proportionally from luma 255 to this value.
//  brightAmt   = 0   [0..31]   // The maximum amount extra smoothing that is applied to the bright areas.
//  darkStart   = 255 [0..255]   // Extra smoothing in brightest areas is decreased proportionally from luma 255 to this value.
//  darkAmt     = 0   [0..31]   // The maximum amount extra smoothing that is applied to the bright areas.
//  ncpu        = 1   [0..3]   // The number of CPUs to use. Actually number of threads to run.
//  showMask    = 0   [0..70]    // This will return any of the following internally used masks. 0 returns processed frame.

//  do              = 0             // 32 bit int used as a Bitmask. This allows the specifications of which specific internal filters to apply. 0 use automatic mode.
AVSValue __cdecl Create_amDCT(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return new amDCT(args[0].AsClip(),
    args[1].AsClip(),    // Our 20th parameter pf1          default NULL     NOT IMPLIMENTED YET
    args[2].AsClip(),    // Our 21th parameter bf1          default NULL     NOT IMPLIMENTED YET
    args[3].AsInt(0),      // Our  1st parameter quant        default 0
    args[4].AsInt(0),      // Our  2st parameter adapt        default 0
    args[5].AsInt(3),      // Our  3nd parameter shift          default 3    16 shifts. Best speed vs. quality trade off.
    args[6].AsInt(1),      // Our  4rd parameter matrix      default 1
    args[7].AsInt(1),      // Our  5th parameter qtype        default 1    mpeg intra
    args[8].AsInt(255),  // Our  6th parameter sharpWPos       default 255
    args[9].AsInt(255),  // Our  7th parameter sharpWAmt       default 255   Do nothing
    args[10].AsInt(0),      // Our  8th parameter expand          default 0
    args[11].AsInt(255),  // Our  9th parameter sharpTPos       default 255
    args[12].AsInt(255),  // Our 10th parameter sharpTAmt       default 255   Do nothing
    args[13].AsInt(2),      // Our 11th parameter quality        default 2
    args[14].AsInt(255),  // Our 13th parameter brightStart     default 255   No bright luma masking
    args[15].AsInt(0),      // Our 14th parameter brightAmt       default 0    No bright luma masking
    args[16].AsInt(0),      // Our 15th parameter darkStart       default 0   No dark   luma masking
    args[17].AsInt(0),      // Our 16th parameter darkAmt         default 0    No dark   luma masking
    args[18].AsInt(0),    // Our 17th parameter showMask      default 0    Displays various internal frames
    args[19].AsInt(0),   // Our 18th parameter T2          default 0    Test Val
    args[20].AsInt(1),    // Our 19th parameter ncpu          default 1   1 thread
    env);
  // Calls the constructor with the arguments provided.
}


// The following function is the function that actually registers the filter in AviSynth
// It is called automatically, when the plug in is loaded to see which functions this filter contains.
/* New 2.6 requirement!!! */
// Declare and initialise server pointers static storage.
const AVS_Linkage* AVS_linkage = 0;
// DLL entry point called from LoadPlugin() to setup a user plugin.
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
  // Save the server pointers.
  AVS_linkage = vectors;

  env->AddFunction("amDCT", "c[pf1]c[bf1]c[quant]i[adapt]i[shift]i[matrix]i[qtype]i[sharpWPos]i[sharpWAmt]i[expand]i[sharpTPos]i[sharpTAmt]i[quality]i[brightStart]i[brightAmt]i[darkStart]i[darkAmt]i[showMask]i[T2]i[ncpu]i", Create_amDCT, 0);

  // The AddFunction has the following parameters:
  // AddFunction(Filtername , Arguments, Function to call,0);

  // Arguments is a string that defines the types and optional names of the arguments for you filter.
  // c - Video Clip
  // i - Integer number
  // f - Float number
  // s - String
  // b - Boolean

   // The word inside the [ ] lets you used named parameters in your script
   // e.g last=localEdge(last,size=100).
   // but last=localEdge(last,100) will also work auto magically

  return "`amDCT' amDCT plugin";
  // A free-form name of the plug-in.
}



