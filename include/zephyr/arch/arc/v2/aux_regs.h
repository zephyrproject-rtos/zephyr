/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 auxiliary registers definitions
 *
 *
 * Definitions for auxiliary registers.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_AUX_REGS_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_AUX_REGS_H_

#include <zephyr/sys/util_macro.h>

#define _ARC_V2_LP_START 0x002
#define _ARC_V2_LP_END 0x003
#define _ARC_V2_IDENTITY 0x004
#define _ARC_V2_SEC_STAT 0x09
#define _ARC_V2_STATUS32 0x00a
#define _ARC_V2_STATUS32_P0 0x00b
#define _ARC_V2_USER_SP	0x00d
#define _ARC_V2_AUX_IRQ_CTRL 0x00e
#define _ARC_V2_IC_IVIC 0x010
#define _ARC_V2_IC_CTRL 0x011
#define _ARC_V2_IC_LIL 0x013
#define _ARC_V2_IC_IVIL 0x019
#define _ARC_V2_IC_DATA 0x01d
#define _ARC_V2_TMR0_COUNT 0x021
#define _ARC_V2_TMR0_CONTROL 0x022
#define _ARC_V2_TMR0_LIMIT 0x023
#define _ARC_V2_IRQ_VECT_BASE    0x025
#define _ARC_V2_IRQ_VECT_BASE_S 0x26
#define _ARC_V2_KERNEL_SP 0x38
#define _ARC_V2_SEC_U_SP 0x39
#define _ARC_V2_SEC_K_SP 0x3a
#define _ARC_V2_AUX_IRQ_ACT 0x043
#define _ARC_V2_DC_IVDC 0x047
#define _ARC_V2_DC_CTRL 0x048
#define _ARC_V2_DC_LDL 0x049
#define _ARC_V2_DC_IVDL 0x04a
#define _ARC_V2_DC_FLSH 0x04b
#define _ARC_V2_DC_FLDL 0x04c
#define _ARC_V2_EA_BUILD 0x065
#define _ARC_V2_VECBASE_AC_BUILD 0x068
#define _ARC_V2_FP_BUILD 0x06b
#define _ARC_V2_DPFP_BUILD 0x06c
#define _ARC_V2_MPU_BUILD 0x06d
#define _ARC_V2_RF_BUILD 0x06e
#define _ARC_V2_MMU_BUILD 0x06f
#define _ARC_V2_VECBASE_BUILD 0x071
#define _ARC_V2_D_CACHE_BUILD 0x072
#define _ARC_V2_DCCM_BUILD 0x074
#define _ARC_V2_TIMER_BUILD 0x075
#define _ARC_V2_AP_BUILD 0x076
#define _ARC_V2_I_CACHE_BUILD 0x077
#define _ARC_V2_ICCM_BUILD 0x078
#define _ARC_V2_MULTIPLY_BUILD 0x07b
#define _ARC_V2_SWAP_BUILD 0x07c
#define _ARC_V2_NORM_BUILD 0x07d
#define _ARC_V2_MINMAX_BUILD 0x07e
#define _ARC_V2_BARREL_BUILD 0x07f
#define _ARC_V2_ISA_CONFIG 0x0c1
#define _ARC_V2_SEP_BUILD 0x0c7
#define _ARC_V2_LPB_BUILD 0x0e9
#define _ARC_V2_LPB_CTRL 0x488
#define _ARC_V2_IRQ_BUILD 0x0f3
#define _ARC_V2_PCT_BUILD 0x0f5
#define _ARC_V2_CC_BUILD 0x0f6
#define _ARC_V2_TMR1_COUNT 0x100
#define _ARC_V2_TMR1_CONTROL 0x101
#define _ARC_V2_TMR1_LIMIT 0x102
#define _ARC_V2_S_TMR0_COUNT 0x106
#define _ARC_V2_S_TMR0_CONTROL 0x107
#define _ARC_V2_S_TMR0_LIMIT 0x108
#define _ARC_V2_S_TMR1_COUNT 0x109
#define _ARC_V2_S_TMR1_CONTROL 0x10a
#define _ARC_V2_S_TMR1_LIMIT 0x10b
#define _ARC_V2_IRQ_PRIO_PEND 0x200
#define _ARC_V2_AUX_IRQ_HINT 0x201
#define _ARC_V2_IRQ_PRIORITY 0x206
#define _ARC_V2_USTACK_TOP 0x260
#define _ARC_V2_USTACK_BASE 0x261
#define _ARC_V2_S_USTACK_TOP 0x262
#define _ARC_V2_S_USTACK_BASE 0x263
#define _ARC_V2_KSTACK_TOP 0x264
#define _ARC_V2_KSTACK_BASE 0x265
#define _ARC_V2_S_KSTACK_TOP 0x266
#define _ARC_V2_S_KSTACK_BASE 0x267
#define _ARC_V2_NSC_TABLE_TOP 0x268
#define _ARC_V2_NSC_TABLE_BASE 0x269
#define _ARC_V2_JLI_BASE 0x290
#define _ARC_V2_LDI_BASE 0x291
#define _ARC_V2_EI_BASE 0x292
#define _ARC_V2_ERET 0x400
#define _ARC_V2_ERSTATUS 0x402
#define _ARC_V2_ECR 0x403
#define _ARC_V2_EFA 0x404
#define _ARC_V2_ERSEC_STAT 0x406
#define _ARC_V2_ICAUSE 0x40a
#define _ARC_V2_IRQ_SELECT 0x40b
#define _ARC_V2_IRQ_ENABLE 0x40c
#define _ARC_V2_IRQ_TRIGGER 0x40d
#define _ARC_V2_IRQ_STATUS 0x40f
#define _ARC_V2_IRQ_PULSE_CANCEL 0x415
#define _ARC_V2_IRQ_PENDING 0x416
#define _ARC_V2_FPU_CTRL 0x300
#define _ARC_V2_FPU_STATUS 0x301
#define _ARC_V2_FPU_DPFP1L 0x302
#define _ARC_V2_FPU_DPFP1H 0x303
#define _ARC_V2_FPU_DPFP2L 0x304
#define _ARC_V2_FPU_DPFP2H 0x305
#define _ARC_V2_MPU_EN 0x409
#define _ARC_V2_MPU_RDB0 0x422
#define _ARC_V2_MPU_RDP0 0x423
#define _ARC_V2_MPU_INDEX 0x448
#define _ARC_V2_MPU_RSTART 0x449
#define _ARC_V2_MPU_REND 0x44A
#define _ARC_V2_MPU_RPER 0x44B
#define _ARC_V2_MPU_PROBE 0x44C
#define _ARC_V2_ACC0_GHI 0x583
#define _ARC_V2_ACC0_HI 0x582
#define _ARC_V2_ACC0_GLO 0x581
#define _ARC_V2_ACC0_LO 0x580
#define _ARC_V2_DSP_BUILD 0x7A
#define _ARC_V2_DSP_CTRL 0x59f
#define _ARC_V2_DSP_BFLY0 0x598
#define _ARC_V2_DSP_FFT_CTRL 0x59e
#define _ARC_V2_AGU_BUILD 0xcc
#define _ARC_V2_AGU_AP0 0x5c0
#define _ARC_V2_AGU_AP1 0x5c1
#define _ARC_V2_AGU_AP2 0x5c2
#define _ARC_V2_AGU_AP3 0x5c3
#define _ARC_V2_AGU_AP4 0x5c4
#define _ARC_V2_AGU_AP5 0x5c5
#define _ARC_V2_AGU_AP6 0x5c6
#define _ARC_V2_AGU_AP7 0x5c7
#define _ARC_V2_AGU_AP8 0x5c8
#define _ARC_V2_AGU_AP9 0x5c9
#define _ARC_V2_AGU_AP10 0x5ca
#define _ARC_V2_AGU_AP11 0x5cb
#define _ARC_V2_AGU_OS0 0x5d0
#define _ARC_V2_AGU_OS1 0x5d1
#define _ARC_V2_AGU_OS2 0x5d2
#define _ARC_V2_AGU_OS3 0x5d3
#define _ARC_V2_AGU_OS4 0x5d4
#define _ARC_V2_AGU_OS5 0x5d5
#define _ARC_V2_AGU_OS6 0x5d6
#define _ARC_V2_AGU_OS7 0x5d7
#define _ARC_V2_AGU_MOD0 0x5e0
#define _ARC_V2_AGU_MOD1 0x5e1
#define _ARC_V2_AGU_MOD2 0x5e2
#define _ARC_V2_AGU_MOD3 0x5e3
#define _ARC_V2_AGU_MOD4 0x5e4
#define _ARC_V2_AGU_MOD5 0x5e5
#define _ARC_V2_AGU_MOD6 0x5e6
#define _ARC_V2_AGU_MOD7 0x5e7
#define _ARC_V2_AGU_MOD8 0x5e8
#define _ARC_V2_AGU_MOD9 0x5e9
#define _ARC_V2_AGU_MOD10 0x5ea
#define _ARC_V2_AGU_MOD11 0x5eb
#define _ARC_V2_AGU_MOD12 0x5ec
#define _ARC_V2_AGU_MOD13 0x5ed
#define _ARC_V2_AGU_MOD14 0x5ee
#define _ARC_V2_AGU_MOD15 0x5ef
#define _ARC_V2_AGU_MOD16 0x5f0
#define _ARC_V2_AGU_MOD17 0x5f1
#define _ARC_V2_AGU_MOD18 0x5f2
#define _ARC_V2_AGU_MOD19 0x5f3
#define _ARC_V2_AGU_MOD20 0x5f4
#define _ARC_V2_AGU_MOD21 0x5f5
#define _ARC_V2_AGU_MOD22 0x5f6
#define _ARC_V2_AGU_MOD23 0x5f7
#define _ARC_HW_PF_BUILD 0xf70
#define _ARC_HW_PF_CTRL 0x4f

/* _ARC_HW_PF_CTRL bits */
#define _ARC_HW_PF_CTRL_ENABLE BIT(0)

/* STATUS32/STATUS32_P0 bits */
#define _ARC_V2_STATUS32_H (1 << 0)
#define Z_ARC_V2_STATUS32_E(x) ((x) << 1)
#define _ARC_V2_STATUS32_AE_BIT 5
#define _ARC_V2_STATUS32_AE (1 << _ARC_V2_STATUS32_AE_BIT)
#define _ARC_V2_STATUS32_DE (1 << 6)
#define _ARC_V2_STATUS32_U_BIT 7
#define _ARC_V2_STATUS32_U (1 << _ARC_V2_STATUS32_U_BIT)
#define _ARC_V2_STATUS32_V (1 << 8)
#define _ARC_V2_STATUS32_C (1 << 9)
#define _ARC_V2_STATUS32_N (1 << 10)
#define _ARC_V2_STATUS32_Z (1 << 11)
#define _ARC_V2_STATUS32_L (1 << 12)
#define _ARC_V2_STATUS32_DZ_BIT 13
#define _ARC_V2_STATUS32_DZ (1 << _ARC_V2_STATUS32_DZ_BIT)
#define _ARC_V2_STATUS32_SC_BIT 14
#define _ARC_V2_STATUS32_SC (1 << _ARC_V2_STATUS32_SC_BIT)
#define _ARC_V2_STATUS32_ES (1 << 15)
#define _ARC_V2_STATUS32_RB(x) ((x) << 16)
#define _ARC_V2_STATUS32_AD_BIT 19
#define _ARC_V2_STATUS32_AD (1 << _ARC_V2_STATUS32_AD_BIT)
#define _ARC_V2_STATUS32_US_BIT 20
#define _ARC_V2_STATUS32_US (1 << _ARC_V2_STATUS32_US_BIT)
#define _ARC_V2_STATUS32_S_BIT 21
#define _ARC_V2_STATUS32_S (1 << _ARC_V2_STATUS32_US_BIT)
#define _ARC_V2_STATUS32_IE (1 << 31)

/* SEC_STAT bits */
#define _ARC_V2_SEC_STAT_SSC_BIT 0
#define _ARC_V2_SEC_STAT_SSC (1 << _ARC_V2_SEC_STAT_SSC_BIT)
#define _ARC_V2_SEC_STAT_NSRT_BIT 1
#define _ARC_V2_SEC_STAT_NSRT (1 << _ARC_V2_SEC_STAT_NSRT_BIT)
#define _ARC_V2_SEC_STAT_NSRU_BIT 2
#define _ARC_V2_SEC_STAT_NSRU (1 << _ARC_V2_SEC_STAT_NSRU_BIT)
#define _ARC_V2_SEC_STAT_IRM_BIT 3
#define _ARC_V2_SEC_STAT_IRM (1 << _ARC_V2_SEC_STAT_IRM_BIT)
#define _ARC_V2_SEC_STAT_SUE_BIT 4
#define _ARC_V2_SEC_STAT_SUE (1 << _ARC_V2_SEC_STAT_SUE_BIT)
#define _ARC_V2_SEC_STAT_NIC_BIT 5
#define _ARC_V2_SEC_STAT_NIC (1 << _ARC_V2_SEC_STAT_NIC_BIT)

/* interrupt related bits */
#define _ARC_V2_IRQ_PRIORITY_SECURE 0x100

/* exception cause register masks */
#define Z_ARC_V2_ECR_VECTOR(X) ((X & 0xff0000) >> 16)
#define Z_ARC_V2_ECR_CODE(X) ((X & 0xff00) >> 8)
#define Z_ARC_V2_ECR_PARAMETER(X) (X & 0xff)

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#if defined(__CCAC__)

#define z_arc_v2_aux_reg_read(reg) _lr((volatile uint32_t)reg)
#define z_arc_v2_aux_reg_write(reg, val) \
	_sr((unsigned int)val, (volatile uint32_t)reg)

#else /* ! __CCAC__ */

#define z_arc_v2_aux_reg_read(reg) __builtin_arc_lr((volatile uint32_t)reg)
#define z_arc_v2_aux_reg_write(reg, val) \
	__builtin_arc_sr((unsigned int)val, (volatile uint32_t)reg)

#endif /* __CCAC__ */
#endif /* _ASMLANGUAGE */

#define z_arc_v2_core_id() \
	({                                               \
		unsigned int __ret;                      \
		__asm__ __volatile__("lr %0, [%1]\n" \
				     "xbfu %0, %0, 0xe8\n" \
				     : "=r"(__ret)       \
				     : "i"(_ARC_V2_IDENTITY));        \
		__ret;                                   \
	})

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_AUX_REGS_H_ */
