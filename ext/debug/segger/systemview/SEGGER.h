/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2019 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER SystemView * Real-time application analysis           *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the RTT protocol and J-Link.                       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* conditions are met:                                                *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this list of conditions and the following disclaimer.    *
*                                                                    *
* o Redistributions in binary form must reproduce the above          *
*   copyright notice, this list of conditions and the following      *
*   disclaimer in the documentation and/or other materials provided  *
*   with the distribution.                                           *
*                                                                    *
* o Neither the name of SEGGER Microcontroller GmbH         *
*   nor the names of its contributors may be used to endorse or      *
*   promote products derived from this software without specific     *
*   prior written permission.                                        *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
*                                                                    *
*       SystemView version: V2.52h                                    *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef SEGGER_H            // Guard against multiple inclusion
#define SEGGER_H

#include <stdarg.h>
#include "Global.h"         // Type definitions: U8, U16, U32, I8, I16, I32

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Keywords/specifiers
*
**********************************************************************
*/

#ifndef INLINE
  #ifdef _WIN32
    //
    // Microsoft VC6 and newer.
    // Force inlining without cost checking.
    //
    #define INLINE  __forceinline
  #elif (defined(__ZEPHYR__))
    #include <toolchain.h>
  #else
    #if (defined(__GNUC__))
      //
      // Force inlining with GCC
      //
      #define INLINE inline __attribute__((always_inline))
    #elif (defined(__CC_ARM))
      //
      // Force inlining with ARMCC (Keil)
      //
      #define INLINE  __inline
    #elif (defined(__ICCARM__) || defined(__RX) || defined(__ICCRX__))
      //
      // Other known compilers.
      //
      #define INLINE  inline
    #else
      //
      // Unknown compilers.
      //
      #define INLINE
    #endif
  #endif
#endif

/*********************************************************************
*
*       Function-like macros
*
**********************************************************************
*/

#define SEGGER_COUNTOF(a)          (sizeof((a))/sizeof((a)[0]))
#define SEGGER_MIN(a,b)            (((a) < (b)) ? (a) : (b))
#define SEGGER_MAX(a,b)            (((a) > (b)) ? (a) : (b))

#ifndef   SEGGER_USE_PARA                   // Some compiler complain about unused parameters.
  #define SEGGER_USE_PARA(Para) (void)Para  // This works for most compilers.
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define SEGGER_PRINTF_FLAG_ADJLEFT    (1 << 0)
#define SEGGER_PRINTF_FLAG_SIGNFORCE  (1 << 1)
#define SEGGER_PRINTF_FLAG_SIGNSPACE  (1 << 2)
#define SEGGER_PRINTF_FLAG_PRECEED    (1 << 3)
#define SEGGER_PRINTF_FLAG_ZEROPAD    (1 << 4)
#define SEGGER_PRINTF_FLAG_NEGATIVE   (1 << 5)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
  char* pBuffer;
  int   BufferSize;
  int   Cnt;
} SEGGER_BUFFER_DESC;

typedef struct {
  unsigned int CacheLineSize;                         // 0: No Cache. Most Systems such as ARM9 use a 32 bytes cache line size.
  void (*pfDMB)       (void);                         // Optional DMB function for Data Memory Barrier to make sure all memory operations are completed.
  void (*pfClean)     (void *p, unsigned NumBytes);   // Optional clean function for cached memory.
  void (*pfInvalidate)(void *p, unsigned NumBytes);   // Optional invalidate function for cached memory.
} SEGGER_CACHE_CONFIG;

typedef struct SEGGER_SNPRINTF_CONTEXT_struct SEGGER_SNPRINTF_CONTEXT;

struct SEGGER_SNPRINTF_CONTEXT_struct {
  void*               pContext;                       // Application specific context.
  SEGGER_BUFFER_DESC* pBufferDesc;                    // Buffer descriptor to use for output.
  void (*pfFlush)(SEGGER_SNPRINTF_CONTEXT* pContext); // Callback executed once the buffer is full. Callback decides if the buffer gets cleared to store more or not.
};

typedef struct {
  void (*pfStoreChar)       (SEGGER_BUFFER_DESC* pBufferDesc, SEGGER_SNPRINTF_CONTEXT* pContext, char c);
  int  (*pfPrintUnsigned)   (SEGGER_BUFFER_DESC* pBufferDesc, SEGGER_SNPRINTF_CONTEXT* pContext, U32 v, unsigned Base, char Flags, int Width, int Precision);
  int  (*pfPrintInt)        (SEGGER_BUFFER_DESC* pBufferDesc, SEGGER_SNPRINTF_CONTEXT* pContext, I32 v, unsigned Base, char Flags, int Width, int Precision);
} SEGGER_PRINTF_API;

typedef void (*SEGGER_pFormatter)(SEGGER_BUFFER_DESC* pBufferDesc, SEGGER_SNPRINTF_CONTEXT* pContext, const SEGGER_PRINTF_API* pApi, va_list* pParamList, char Lead, int Width, int Precision);

typedef struct SEGGER_PRINTF_FORMATTER {
  struct SEGGER_PRINTF_FORMATTER* pNext;              // Pointer to next formatter.
  SEGGER_pFormatter               pfFormatter;        // Formatter function.
  char                            Specifier;          // Format specifier.
} SEGGER_PRINTF_FORMATTER;

/*********************************************************************
*
*       Utility functions
*
**********************************************************************
*/

//
// Memory operations.
//
void SEGGER_ARM_memcpy(void* pDest, const void* pSrc, int NumBytes);
void SEGGER_memcpy    (void* pDest, const void* pSrc, unsigned NumBytes);
void SEGGER_memxor    (void* pDest, const void* pSrc, unsigned NumBytes);

//
// String functions.
//
int      SEGGER_atoi      (const char* s);
int      SEGGER_isalnum   (int c);
int      SEGGER_isalpha   (int c);
unsigned SEGGER_strlen    (const char* s);
int      SEGGER_tolower   (int c);
int      SEGGER_strcasecmp(const char* sText1, const char* sText2);

//
// Buffer/printf related.
//
void SEGGER_StoreChar    (SEGGER_BUFFER_DESC* pBufferDesc, char c);
void SEGGER_PrintUnsigned(SEGGER_BUFFER_DESC* pBufferDesc, U32 v, unsigned Base, int Precision);
void SEGGER_PrintInt     (SEGGER_BUFFER_DESC* pBufferDesc, I32 v, unsigned Base, int Precision);
int  SEGGER_snprintf     (char* pBuffer, int BufferSize, const char* sFormat, ...);
int  SEGGER_vsnprintf    (char* pBuffer, int BufferSize, const char* sFormat, va_list ParamList);
int  SEGGER_vsnprintfEx  (SEGGER_SNPRINTF_CONTEXT* pContext, const char* sFormat, va_list ParamList);

int  SEGGER_PRINTF_AddFormatter      (SEGGER_PRINTF_FORMATTER* pFormatter, SEGGER_pFormatter pfFormatter, char c);
void SEGGER_PRINTF_AddDoubleFormatter(void);
void SEGGER_PRINTF_AddIPFormatter    (void);
void SEGGER_PRINTF_AddHTMLFormatter  (void);

#if defined(__cplusplus)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif                      // Avoid multiple inclusion

/*************************** End of file ****************************/
