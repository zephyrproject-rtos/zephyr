/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/* Copyright (c) 2009 - 2015 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/

#ifndef __CORE_RISCV32_H__
#define __CORE_RISCV32_H__

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

#define RISCV32

#if defined ( __GNUC__ )
  #define __ASM            __asm                                      /*!< asm keyword for GNU Compiler */
  #define __INLINE         inline                                     /*!< inline keyword for GNU Compiler */
  #define __STATIC_INLINE  static inline

#else
  #error Unknown compiler
#endif

#if defined ( __GNUC__ )

#define __BKPT(x) __ASM("ebreak")

__attribute__((always_inline)) __STATIC_INLINE void __NOP(void)
{
  __ASM volatile ("nop");
}

__attribute__((always_inline)) __STATIC_INLINE void __DSB(void)
{
  __ASM volatile ("nop");
}

__attribute__((always_inline)) __STATIC_INLINE void __ISB(void)
{
  __ASM volatile ("nop");
}

__attribute__((always_inline)) __STATIC_INLINE void __WFI(void)
{
  __ASM volatile ("wfi");
}

__attribute__((always_inline)) __STATIC_INLINE void __WFE(void)
{
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __enable_irq(void)
{
  __ASM volatile ("csrsi mstatus, 8");
}

__attribute__( ( always_inline ) ) __STATIC_INLINE void __disable_irq(void)
{
  __ASM volatile ("csrci mstatus, 8");
}

__attribute__((always_inline)) __STATIC_INLINE uint32_t __REV(uint32_t value)
{
  return __builtin_bswap32(value);
}

__attribute__((always_inline)) __STATIC_INLINE uint32_t __REV16(uint32_t value)
{
  return __builtin_bswap16(value);
}

#else
  #error Unknown compiler
#endif

#ifdef __cplusplus
  #define   __I     volatile             /*!< Defines 'read only' permissions */
#else
  #define   __I     volatile const       /*!< Defines 'read only' permissions */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions */
#define     __IO    volatile             /*!< Defines 'read / write' permissions */

/* following defines should be used for structure members */
#define     __IM     volatile const      /*! Defines 'read only' structure member permissions */
#define     __OM     volatile            /*! Defines 'write only' structure member permissions */
#define     __IOM    volatile            /*! Defines 'read / write' structure member permissions */

#ifdef __cplusplus
}
#endif

#endif /* __CORE_RISCV32_H__ */
