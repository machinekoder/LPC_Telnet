
//----------------------------------------------------------------------------------------------------
// type.h
//----------------------------------------------------------------------------------------------------

#ifndef _TYPE_H_
#define _TYPE_H_

//----------------------------------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------------------------------




#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

//----------------------------------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------------------------------

typedef unsigned char  BOOLEAN;
typedef unsigned char  INT8U;                    // Unsigned  8 bit quantity
typedef signed   char  INT8S;                    // Signed    8 bit quantity
typedef unsigned short INT16U;                   // Unsigned 16 bit quantity
typedef signed   short INT16S;                   // Signed   16 bit quantity
typedef unsigned int   INT32U;                   // Unsigned 32 bit quantity
typedef signed   int   INT32S;                   // Signed   32 bit quantity
typedef float          FP32;                     // Single precision floating point
typedef double         FP64;                     // Double precision floating point

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   BOOL;

//----------------------------------------------------------------------------------------------------
// EOF
//----------------------------------------------------------------------------------------------------

#endif // _TYPE_H_


