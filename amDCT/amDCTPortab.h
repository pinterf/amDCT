#ifndef _AMDCTPORTAB_H_
#define _AMDCTPORTAB_H_



#ifndef  FALSE
#define FALSE    0
#endif
#ifndef  TRUE
#define TRUE     1
#endif



#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif


#ifndef ROUND_TOINT
#define ROUND_TOINT(X)  (int)(((X > 0.0)?(X + 0.5):(X - 0.5)))
#endif

#ifndef ROUND_TO_CURVAL
#define ROUND_TO_CURVAL(CURV, X)  (int)(((X > CURV)?(X - 0.5):(X + 0.5)))
#endif


// Macroes from xvid global.h  file
#ifndef MIN
#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#endif

#ifndef MAX
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#endif




#endif
// _AMDCTPORTAB_H_
