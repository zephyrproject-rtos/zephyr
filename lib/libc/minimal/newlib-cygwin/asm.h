/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 * Copyright (c) 2017  SiFive Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the FreeBSD License.   This program is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
 * including the implied warranties of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  A copy of this license is available at
 * http://www.opensource.org/licenses.
 */

#ifndef _SYS_ASM_H
#define _SYS_ASM_H

/**
 * Macros to handle different pointer/register sizes for 32/64-bit code
 */
#if __riscv_xlen == 64
#define PTRLOG 3
#define SZREG  8
#define REG_S  sd
#define REG_L  ld
#elif __riscv_xlen == 32
#define PTRLOG 2
#define SZREG  4
#define REG_S  sw
#define REG_L  lw
#else
#error __riscv_xlen must equal 32 or 64
#endif

#ifndef __riscv_float_abi_soft
/** For ABI uniformity, reserve 8 bytes for floats, even if double-precision
 * floating-point is not supported in hardware.
 */
#define SZFREG 8
#ifdef __riscv_float_abi_single
#define FREG_L flw
#define FREG_S fsw
#elif defined(__riscv_float_abi_double)
#define FREG_L fld
#define FREG_S fsd
#elif defined(__riscv_float_abi_quad)
#define FREG_L flq
#define FREG_S fsq
#else
#error unsupported FLEN
#endif
#endif

#endif /* sys/asm.h */
