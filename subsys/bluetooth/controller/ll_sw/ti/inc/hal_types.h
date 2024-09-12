/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/******************************************************************************

 @file  _hal_types.h

 @brief Describe the purpose and contents of the file.

 *****************************************************************************/

#ifndef _HAL_TYPES_H
#define _HAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------------------------------------
 *                                               Types
 * ------------------------------------------------------------------------------------------------
 */
typedef signed   char   int8;
typedef unsigned char   uint8;

typedef signed   short  int16;
typedef unsigned short  uint16;

typedef signed   long   int32;
typedef unsigned long   uint32;

typedef uint32          halDataAlign_t;

/* ------------------------------------------------------------------------------------------------
 *                                        Compiler Macros
 * ------------------------------------------------------------------------------------------------
 */
/* ----------- IAR Compiler ----------- */
#ifdef __IAR_SYSTEMS_ICC__
#define ASM_NOP    asm("NOP")
#define NO_INIT    __no_init

/* ----------- KEIL Compiler ----------- */
#elif defined __KEIL__
#define ASM_NOP   __nop()

/* ----------- CCS Compiler ----------- */
#elif defined __TI_COMPILER_VERSION || defined __TI_COMPILER_VERSION__
#define ASM_NOP    asm(" NOP")
#define NO_INIT    __attribute__((noinit))

/* ----------- GNU Compiler ----------- */
#elif defined __GNUC__
#define ASM_NOP __asm__ __volatile__ ("nop")

/* ---------- MSVC compiler ---------- */
#elif _MSC_VER
#define ASM_NOP __asm NOP

/* ----------- Unrecognized Compiler ----------- */
#else
#error "ERROR: Unknown compiler."
#endif


/* ------------------------------------------------------------------------------------------------
 *                                        Standard Defines
 * ------------------------------------------------------------------------------------------------
 */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef UTRUE
#define UTRUE 1U
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef UFALSE
#define UFALSE 0U
#endif

#ifndef NULL
#define NULL 0L
#endif


/* ------------------------------------------------------------------------------------------------
 *                                       Memory Attributes
 * ------------------------------------------------------------------------------------------------
 */

#if defined (__IAR_SYSTEMS_ICC__)
#define XDATA
#define CODE
#define DATA_ALIGN(x)                   _Pragma data_alignment=(x)
#define ALIGNED
#define PACKED                          __packed
#define PACKED_STRUCT                   PACKED struct
#define PACKED_TYPEDEF_STRUCT           PACKED typedef struct
#define PACKED_TYPEDEF_CONST_STRUCT     PACKED typedef const struct
#define PACKED_TYPEDEF_UNION            PACKED typedef union
#define PACKED_ALIGNED                  PACKED
#define PACKED_ALIGNED_TYPEDEF_STRUCT   PACKED_TYPEDEF_STRUCT

#elif defined __TI_COMPILER_VERSION || defined __TI_COMPILER_VERSION__
#define XDATA
#define CODE
#define DATA
#define NEARFUNC
#define ALIGNED
#define PACKED                              __attribute__((packed))
#define PACKED_STRUCT                       struct PACKED
#define PACKED_TYPEDEF_STRUCT               typedef struct PACKED
#define PACKED_TYPEDEF_CONST_STRUCT         typedef const struct PACKED
#define PACKED_TYPEDEF_UNION                typedef union PACKED
#define PACKED_ALIGNED                      __attribute__((packed,aligned(4)))
#define PACKED_ALIGNED_TYPEDEF_STRUCT       typedef struct PACKED_ALIGNED

#elif defined (__GNUC__)
#if defined (__clang__)
#define ALIGNED                             __attribute__((aligned(4)))
#else
#define ALIGNED
#endif
#ifdef CC33xx
#define PACKED                              __attribute__((aligned(1)))  __attribute__((packed))
#else
#define PACKED                              __attribute__((__packed__))
#endif
#define PACKED_STRUCT                       struct PACKED
#define PACKED_TYPEDEF_STRUCT               typedef struct PACKED
#define PACKED_TYPEDEF_STRUCT               typedef struct PACKED
#define PACKED_TYPEDEF_CONST_STRUCT         typedef const struct PACKED
#define PACKED_TYPEDEF_UNION                typedef union PACKED
#define PACKED_ALIGNED                      __attribute__((packed,aligned(4)))
#define PACKED_ALIGNED_TYPEDEF_STRUCT       typedef struct PACKED_ALIGNED
#endif

/**************************************************************************************************
 */
#endif
