#ifndef __VAR_TYPE_H__
#define __VAR_TYPE_H__

//==============================================================================
// HAL data type
//==============================================================================
typedef struct Time         Date_t;     // related rtc.h
typedef struct Config       Config_t;   // related common.h

typedef unsigned char       BOOL;
typedef unsigned char       byte;
typedef unsigned char       uchar;
typedef unsigned int        word;
typedef unsigned long       dword;
typedef unsigned long long  qword;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned long       uint32;
typedef unsigned long long  uint64;

typedef uint32              event32_t;

typedef signed char         int8;
typedef signed short        int16;
typedef signed long         int32;
typedef signed long long    int64;

#ifdef NULL
    #undef NULL
#endif

#ifdef FAIL
    #undef FAIL
#endif

#define NULL                0
#define TRUE                1
#define FALSE               0
#define SUCCESS             1
#define FAIL                0
#define ON                  1
#define OFF                 0

#define BM(x)               (1<<x)

#define SWAP32(x)           ((((x) >> 24) & 0x000000FF) | \
                             (((x) >> 8)  & 0x0000FF00) | \
                             (((x) << 8)  & 0x00FF0000) | \
                             (((x) << 24) & 0xFF000000))

#define SWAP16(x)           ((((x) >> 8) & 0x00FF) | \
                             (((x) << 8) & 0xFF00))

#define HI_UINT16(a)        (((a) >> 8) & 0xFF)
#define LO_UINT16(a)        ((a) & 0xFF)
#define HI_UINT8(a)         (((a) >> 4) & 0x0F)
#define LO_UINT8(a)         ((a) & 0x0F)

#define BUILD_UINT32(b0, b1, b2, b3)    ((uint32)((uint32)((b0) & 0x00FF) \
                                        + ((uint32)((b1) & 0x00FF) << 8) \
                                        + ((uint32)((b2) & 0x00FF) << 16) \
                                        + ((uint32)((b3) & 0x00FF) << 24)))
#define BUILD_UINT16(lo, hi)            ((uint16)(((lo) & 0x00FF) + (((hi) & 0x00FF) << 8)))
#define BUILD_UINT8(hi, lo)             ((uint8)(((lo) & 0x0F) + (((hi) & 0x0F) << 4)))
/* takes a byte out of a uint32 : var - uint32,  ByteNum - byte to take out (0 - 3) */
#define BREAK_UINT32( var, ByteNum )    (byte)((uint32)(((var) >>((ByteNum) * 8)) & 0x00FF))

#ifndef ABS
# define ABS(n)     (((n) < 0) ? -(n) : (n))
#endif

#endif
