/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RH850 system register access
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_SYSREG_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_SYSREG_H_

/** @cond INTERNAL_HIDDEN */

/* System register numbers: regID, selID */
#define RH850_SR_EIPC   0, 0
#define RH850_SR_EIPSW  1, 0
#define RH850_SR_FEPC   2, 0
#define RH850_SR_FEPSW  3, 0
#define RH850_SR_PSW    5, 0
#define RH850_SR_FPSR   6, 0
#define RH850_SR_FPEPC  7, 0
#define RH850_SR_FPST   8, 0
#define RH850_SR_FPCC   9, 0
#define RH850_SR_FPEC   11, 0
#define RH850_SR_EIIC   13, 0
#define RH850_SR_FEIC   14, 0
#define RH850_SR_CTPC   16, 0
#define RH850_SR_CTPSW  17, 0
#define RH850_SR_EIWR   28, 0
#define RH850_SR_FEWR   29, 0
#define RH850_SR_EBASE  3, 1
#define RH850_SR_INTBP  4, 1
#define RH850_SR_PEID   0, 2
#define RH850_SR_MEA    6, 2
#define RH850_SR_MEI    8, 2
#define RH850_SR_ISPR   10, 2
#define RH850_SR_INTCFG 13, 2
#define RH850_SR_PLMR   14, 2
#define RH850_SR_RBIP   18, 2
#define RH850_SR_ICTAGL 16, 4
#define RH850_SR_ICTAGH 17, 4
#define RH850_SR_ICDATL 18, 4
#define RH850_SR_ICDATH 19, 4
#define RH850_SR_ICERR  28, 4
#define RH850_SR_MCA    8, 5
#define RH850_SR_MCS    9, 5
#define RH850_SR_MCR    11, 5
#define RH850_SR_MCI    12, 5
#define RH850_SR_MPIDX  16, 5
#define RH850_SR_MPLA   20, 5
#define RH850_SR_MPUA   21, 5
#define RH850_SR_MPAT   22, 5
#define RH850_SR_FXSR   6, 10
#define RH850_SR_FXST   8, 10
#define RH850_SR_FXXC   12, 10
#define RH850_SR_FXXP   13, 10

/* PSW bits, without suffixes so that assembly can use them */
#define RH850_PSW_ID  0x00000020
#define RH850_PSW_EP  0x00000040
#define RH850_PSW_NP  0x00000080
#define RH850_PSW_EBV 0x00008000
#define RH850_PSW_CU0 0x00010000
#define RH850_PSW_CU1 0x00020000

/* PSW.EIMASK = 63: priorities 0-62 accepted */
#define RH850_PSW_EIMASK_ALL 0x03f00000

/* INTCFG.EPL: 64 interrupt priority levels; ULNR keeps its reset value 0FH */
#define RH850_INTCFG_EPL 0x000f0002
/* PLMR = 63: priorities 0-62 accepted */
#define RH850_PLMR_ALL   63

/* FPSR.FS: subnormal operands and results are flushed */
#define RH850_FPSR_FS      0x00020000
/* FPSR.XC: cause bits E, V, Z, O, U, I */
#define RH850_FPSR_XC_MASK 0x0000fc00
/* FPEC.FPIVD: an imprecise FPU exception (FPI) is reported and pending */
#define RH850_FPEC_FPIVD   0x00000001

/* FXSR: reserved bit 21 = 1, FS = 1 */
#define RH850_FXSR_INIT 0x00220000

#ifdef _ASMLANGUAGE

/* clang-format off */
.macro rh850_stsr regid, selid, reg
	stsr	\regid, \reg, \selid
.endm
/* clang-format on */

#endif /* _ASMLANGUAGE */

#ifndef _ASMLANGUAGE
#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#define Z_RH850_STSR(regid, selid)                                                                 \
	({                                                                                         \
		uint32_t _v;                                                                       \
                                                                                                   \
		__asm__ volatile("stsr " STRINGIFY(regid) ", %0, " STRINGIFY(selid) : "=r"(_v));   \
		_v;                                                                                \
	})

#define Z_RH850_LDSR(regid, selid, val)                                                            \
	__asm__ volatile("ldsr %0, " STRINGIFY(regid) ", " STRINGIFY(selid)::"r"(val) : "memory")

#define rh850_sysreg_read(sr)       Z_RH850_STSR(sr)
#define rh850_sysreg_write(sr, val) Z_RH850_LDSR(sr, val)

#endif /* _ASMLANGUAGE */

/** @endcond */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_SYSREG_H_ */
