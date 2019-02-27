/**
 ******************************************************************************
 * @file    stm32_wpan_common.h
 * @author  MCD Application Team
 * @brief   Common file to utilities
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32_WPAN_COMMON_H
#define __STM32_WPAN_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#if   defined ( __CC_ARM )
 #define __ASM            __asm                                      /*!< asm keyword for ARM Compiler          */
 #define __INLINE         __inline                                   /*!< inline keyword for ARM Compiler       */
 #define __STATIC_INLINE  static __inline
#elif defined ( __ICCARM__ )
 #define __ASM            __asm                                      /*!< asm keyword for IAR Compiler          */
 #define __INLINE         inline                                     /*!< inline keyword for IAR Compiler. Only available in High optimization mode! */
 #define __STATIC_INLINE  static inline
#elif defined ( __GNUC__ )
 #define __ASM            __asm                                      /*!< asm keyword for GNU Compiler          */
 #define __INLINE         inline                                     /*!< inline keyword for GNU Compiler       */
 #define __STATIC_INLINE  static inline
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// #include "core_cmFunc.h"

  /* -------------------------------- *
   *  Basic definitions               *
   * -------------------------------- */

#undef NULL
#define NULL                    0

#undef FALSE
#define FALSE                   0

#undef TRUE
#define TRUE                    (!0)

  /* -------------------------------- *
   *  Critical Section definition     *
   * -------------------------------- */
#undef BACKUP_PRIMASK
#define BACKUP_PRIMASK()    uint32_t primask_bit= __get_PRIMASK()

#undef DISABLE_IRQ
#define DISABLE_IRQ()       __disable_irq()

#undef RESTORE_PRIMASK
#define RESTORE_PRIMASK()   __set_PRIMASK(primask_bit)

  /* -------------------------------- *
   *  Macro delimiters                *
   * -------------------------------- */
#undef M_BEGIN
#define M_BEGIN     do {

#undef  M_END
#define M_END       } while(0)

  /* -------------------------------- *
   *  Some useful macro definitions   *
   * -------------------------------- */
#undef MAX
#define MAX( x, y )          (((x)>(y))?(x):(y))

#undef MIN
#define MIN( x, y )          (((x)<(y))?(x):(y))

#undef MODINC
#define MODINC( a, m )       M_BEGIN  (a)++;  if ((a)>=(m)) (a)=0;  M_END

#undef MODDEC
#define MODDEC( a, m )       M_BEGIN  if ((a)==0) (a)=(m);  (a)--;  M_END

#undef MODADD
#define MODADD( a, b, m )    M_BEGIN  (a)+=(b);  if ((a)>=(m)) (a)-=(m);  M_END

#undef MODSUB
#define MODSUB( a, b, m )    MODADD( a, (m)-(b), m )

#undef ALIGN
#ifdef WIN32
#define ALIGN(n)
#else
#define ALIGN(n)             __attribute__((aligned(n)))
#endif

#undef PAUSE
#define PAUSE( t )           M_BEGIN \
                               volatile int _i; \
                               for ( _i = t; _i > 0; _i -- ); \
                             M_END
#undef DIVF
#define DIVF( x, y )         ((x)/(y))

#undef DIVC
#define DIVC( x, y )         (((x)+(y)-1)/(y))

#undef DIVR
#define DIVR( x, y )         (((x)+((y)/2))/(y))

#undef SHRR
#define SHRR( x, n )         ((((x)>>((n)-1))+1)>>1)

#undef BITN
#define BITN( w, n )         (((w)[(n)/32] >> ((n)%32)) & 1)

#undef BITNSET
#define BITNSET( w, n, b )   M_BEGIN (w)[(n)/32] |= ((U32)(b))<<((n)%32); M_END

/* -------------------------------- *
 *  Section attribute               *
 * -------------------------------- */
#define PLACE_IN_SECTION( __x__ )  __attribute__((section (__x__)))

/* ----------------------------------- *
 *  Packed usage (compiler dependent)  *
 * ----------------------------------- */
#undef PACKED__
#undef PACKED_STRUCT

#if defined ( __CC_ARM )
  #if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
    #define PACKED__ __attribute__((packed))
    #define PACKED_STRUCT struct PACKED__
  #else
    #define PACKED__(TYPE) __packed TYPE
    #define PACKED_STRUCT PACKED__(struct)
  #endif
#elif defined   ( __GNUC__ )
  #define PACKED__ __attribute__((packed))
  #define PACKED_STRUCT struct PACKED__
#elif defined (__ICCARM__)
  #define PACKED_STRUCT __packed struct
#elif
  #define PACKED_STRUCT __packed struct
#endif

#ifdef __cplusplus
}
#endif

#endif /*__STM32_WPAN_COMMON_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
