/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************
 * FILE NAME
 *
 *       rpmsg_compiler.h
 *
 * DESCRIPTION
 *
 *       This file defines compiler-specific macros.
 *
 ***************************************************************************/
#ifndef _RPMSG_COMPILER_H_
#define _RPMSG_COMPILER_H_

/* IAR ARM build tools */
#if defined(__ICCARM__)

#include <intrinsics.h>

#define MEM_BARRIER() __DSB()

#ifndef RL_PACKED_BEGIN
#define RL_PACKED_BEGIN __packed
#endif

#ifndef RL_PACKED_END
#define RL_PACKED_END
#endif

/* ARM GCC */
#elif defined(__CC_ARM) || (defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))

#if (defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
#include <arm_compat.h>
#endif

#define MEM_BARRIER() __schedule_barrier()

#ifndef RL_PACKED_BEGIN
#define RL_PACKED_BEGIN _Pragma("pack(1U)")
#endif

#ifndef RL_PACKED_END
#define RL_PACKED_END _Pragma("pack()")
#endif

/* XCC HiFi4 */
#elif defined(__XCC__)

/*
 * The XCC HiFi4 compiler is compatible with GNU compiler, with restrictions.
 * For ARM __schedule_barrier, there's no identical intrinsic in HiFi4.
 * A complete synchronization barrier would require initialize and wait ops.
 * Here use NOP instead, similar to ARM __nop.
*/
#define MEM_BARRIER() __asm__ __volatile__("nop" : : : "memory")

#ifndef RL_PACKED_BEGIN
#define RL_PACKED_BEGIN
#endif

#ifndef RL_PACKED_END
#define RL_PACKED_END __attribute__((__packed__))
#endif

/* GNUC */
#elif defined(__GNUC__)

#define MEM_BARRIER() __asm__ volatile("dsb" : : : "memory")

#ifndef RL_PACKED_BEGIN
#define RL_PACKED_BEGIN
#endif

#ifndef RL_PACKED_END
#define RL_PACKED_END __attribute__((__packed__))
#endif

#else
/* There is no default definition here to avoid wrong structures packing in case of not supported compiler */
#error Please implement the structure packing macros for your compiler here!
#endif

#endif /* _RPMSG_COMPILER_H_ */
