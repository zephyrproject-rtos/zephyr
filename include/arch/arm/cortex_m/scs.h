/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM CORTEX-M System Control Space
 *
 * Define the System Control Space for the CORTEX-M series of processors and
 * provide an interface for miscellaneous SCS functionalities.
 *
 * All register and bit-field names come from the
 *
 *   Cortex-M3 Devices
 *   Generic User Guide
 *   ARM DUI 0552A (ID121610)
 *
 * and
 *
 *   Cortex-M3
 *   Revision r2p1
 *   Technical Reference Manual
 *   ARM DDI 0337I (ID072410)
 *
 * documents from ARM.
 *
 * The API does not account for all possible usages of the SCS, only the
 * functionalities needed by the kernel. It does not contain NVIC and
 * SCB functionalities either: these can be found in nvic.h and scb.h.
 *
 * MPU functionalities are not implemented.
 *
 * The same effect can be achieved by directly writing in the registers of the
 * SCS, using the __scs data structure (or hardcoded values), but the APIs found
 * here are less error-prone, especially for registers with multiple instances
 * to account for 16 exceptions.
 *
 * If access to a missing functionality is needed, directly writing to the
 * registers is the way to implement it.
 *
 * Note that the 'stir' register, even if not in the 'nvic' part of the SCB, is
 * still considered part of the NVIC and an API for it is provided in nvic.h.
 */

#ifndef _SCS__H_
#define _SCS__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

#include <arch/arm/cortex_m/memory_map.h>

#else /* _ASMLANGUAGE */

#include <kernel.h>
#include <arch/cpu.h>
#include <stdint.h>

union __ictr {
	uint32_t val;
	struct {
		uint32_t intlinesnum : 4 __packed;
		uint32_t rsvd__4_31 : 28 __packed;
	} bit;
};

union __actlr {
	uint32_t val;
	struct {
		uint32_t dismcycint : 1 __packed;
		uint32_t disdefwbuf : 1 __packed;
		uint32_t disfold : 1 __packed;
		uint32_t rsvd__3_31 : 28 __packed;
	} bit;
};

union __stcsr {
	uint32_t val;
	struct {
		uint8_t enable : 1 __packed;
		uint8_t tickint : 1 __packed;
		uint8_t clksource : 1 __packed;
		uint16_t rsvd__3_15 : 13 __packed;
		uint16_t countflag : 1 __packed;
		uint16_t rsvd__17_31 : 15 __packed;
	} bit;
};

union __cpuid {
	uint32_t val;
	struct {
		uint32_t revision : 4 __packed;
		uint32_t partno : 12 __packed;
		uint32_t constant : 4 __packed;
		uint32_t variant : 4 __packed;
		uint32_t implementer : 8 __packed;
	} bit;
};

union __icsr {
	uint32_t val;
	struct {
#if defined(CONFIG_ARMV6_M)
		uint32_t vectactive : 9 __packed;
		uint32_t rsvd__9_10_11 : 3 __packed;
		uint32_t vectpending : 9 __packed;
		uint32_t rsvd__21 : 1 __packed;
#elif defined(CONFIG_ARMV7_M)
		uint32_t vectactive : 10 __packed;
		uint32_t rsvd__10 : 1 __packed;
		uint32_t rettobase : 1 __packed;
		uint32_t vectpending : 10 __packed;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
		uint32_t isrpending : 1 __packed;
		uint32_t rsvd__23 : 1 __packed;
		uint32_t rsvd__24 : 1 __packed;
		uint32_t pendstclr : 1 __packed;
		uint32_t pendstset : 1 __packed;
		uint32_t pendsvclr : 1 __packed;
		uint32_t pendsvset : 1 __packed;
		uint32_t rsvd__29_30 : 2 __packed;
		uint32_t nmipendset : 1 __packed;
	} bit;
};

union __vtor {
	uint32_t val;
	struct {
		uint32_t rsvd__0_6 : 7 __packed;
		uint32_t tbloff : 22 __packed;
		uint32_t tblbase : 1 __packed;
		uint32_t rsvd__30_31 : 2 __packed;
	} bit;
};

union __aircr {
	uint32_t val;
	struct {
#if defined(CONFIG_ARMV6_M)
		uint32_t rsvd__0 : 1 __packed;
#elif defined(CONFIG_ARMV7_M)
		uint32_t vecreset : 1 __packed;      /* WO */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
		uint32_t vectclractive : 1 __packed; /* WO */
		uint32_t sysresetreq : 1 __packed;   /* WO */
#if defined(CONFIG_ARMV6_M)
		uint32_t rsvd__3_14 : 12 __packed;
#elif defined(CONFIG_ARMV7_M)
		uint32_t rsvd__3_7 : 5 __packed;
		uint32_t prigroup : 3 __packed;
		uint32_t rsvd__11_14 : 4 __packed;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
		uint32_t endianness : 1 __packed; /* RO */
		uint32_t vectkey : 16 __packed;
	} bit;
};

union __scr {
	uint32_t val;
	struct {
		uint32_t rsvd__0 : 1 __packed;
		uint32_t sleeponexit : 1 __packed;
		uint32_t sleepdeep : 1 __packed;
		uint32_t rsvd__3 : 1 __packed;
		uint32_t sevonpend : 1 __packed;
		uint32_t rsvd__5_31 : 27 __packed;
	} bit;
};

#define _SCB_CCR_DIV_0_TRP (1 << 4)
#define _SCB_CCR_UNALIGN_TRP (1 << 3)
union __ccr {
	uint32_t val;
	struct {
#if defined(CONFIG_ARMV6_M)
		uint32_t rsvd_0_2 : 3 __packed;
#elif defined(CONFIG_ARMV7_M)
		uint32_t nonbasethrdena : 1 __packed;
		uint32_t usersetmpend : 1 __packed;
		uint32_t rsvd__2 : 1 __packed;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
		uint32_t unalign_trp : 1 __packed;
#if defined(CONFIG_ARMV6_M)
		uint32_t rsvd_4_8 : 5 __packed;
#elif defined(CONFIG_ARMV7_M)
		uint32_t div_0_trp : 1 __packed;
		uint32_t rsvd__5_7 : 3 __packed;
		uint32_t bfhfnmign : 1 __packed;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
		uint32_t stkalign : 1 __packed;
		uint32_t rsvd__10_31 : 22 __packed;
	} bit;
};

union __shcsr {
	uint32_t val;
	struct {
		uint32_t memfaultact : 1 __packed;
		uint32_t busfaultact : 1 __packed;
		uint32_t rsvd__2 : 1 __packed;
		uint32_t usgfaultact : 1 __packed;
		uint32_t rsvd__4_6 : 3 __packed;
		uint32_t svcallact : 1 __packed;
		uint32_t monitoract : 1 __packed;
		uint32_t rsvd__9 : 1 __packed;
		uint32_t pendsvact : 1 __packed;
		uint32_t systickact : 1 __packed;
		uint32_t usgfaultpended : 1 __packed;
		uint32_t memfaultpended : 1 __packed;
		uint32_t busfaultpended : 1 __packed;
		uint32_t svcallpended : 1 __packed;
		uint32_t memfaultena : 1 __packed;
		uint32_t busfaultena : 1 __packed;
		uint32_t usgfaultena : 1 __packed;
		uint32_t rsvd__19_31 : 13 __packed;
	} bit;
};

union __cfsr {
	uint32_t val;
	struct {
		union {
			uint8_t val;
			struct {
				uint8_t iaccviol : 1 __packed;
				uint8_t daccviol : 1 __packed;
				uint8_t rsvd__2 : 1 __packed;
				uint8_t munstkerr : 1 __packed;
				uint8_t mstkerr : 1 __packed;
				uint8_t rsvd__5_6 : 2 __packed;
				uint8_t mmarvalid : 1 __packed;
			} bit;
		} mmfsr;
		union {
			uint8_t val;
			struct {
				uint8_t ibuserr : 1 __packed;
				uint8_t preciserr : 1 __packed;
				uint8_t impreciserr : 1 __packed;
				uint8_t unstkerr : 1 __packed;
				uint8_t stkerr : 1 __packed;
				uint8_t rsvd__5_6 : 2 __packed;
				uint8_t bfarvalid : 1 __packed;
			} bit;
		} bfsr;
		union {
			uint16_t val;
			struct {
				uint16_t undefinstr : 1 __packed;
				uint16_t invstate : 1 __packed;
				uint16_t invpc : 1 __packed;
				uint16_t nocp : 1 __packed;
				uint16_t rsvd__4_7 : 4 __packed;
				uint16_t unaligned : 1 __packed;
				uint16_t divbyzero : 1 __packed;
				uint16_t rsvd__10_15 : 6 __packed;
			} bit;
		} ufsr;
	} byte;
};

union __hfsr {
	uint32_t val;
	struct {
		uint32_t rsvd__0 : 1 __packed;
		uint32_t vecttbl : 1 __packed;
		uint32_t rsvd__2_29 : 28 __packed;
		uint32_t forced : 1 __packed;
		uint32_t debugevt : 1 __packed;
	} bit;
};

union __mpu_type {
	uint32_t val;
	struct {
		uint32_t separate : 1 __packed;
		uint32_t rsvd__1_7 : 7 __packed;
		uint32_t dregion : 8 __packed;
		uint32_t iregion : 8 __packed;
		uint32_t rsvd__24_31 : 8 __packed;
	} bit;
};

union __mpu_ctrl {
	uint32_t val;
	struct {
		uint32_t enable : 1 __packed;
		uint32_t hfnmiena : 1 __packed;
		uint32_t privdefena : 1 __packed;
		uint32_t rsvd__24_31 : 29 __packed;
	} bit;
};

union __mpu_rnr {
	uint32_t val;
	struct {
		uint32_t region : 8 __packed;
		uint32_t rsvd__24_31 : 24 __packed;
	} bit;
};

union __mpu_rbar {
	uint32_t val;
	struct {
		uint8_t region : 4 __packed;
		uint8_t valid : 1 __packed;
		uint32_t addr : 27 __packed;
	} bit;
};

union __mpu_rasr {
	uint32_t val;
	struct {
		uint32_t enable : 1 __packed;
		uint32_t size : 5 __packed;
		uint32_t rsvd__6_7 : 2 __packed;
		uint32_t srd : 8 __packed;
		uint32_t b : 1 __packed;
		uint32_t c : 1 __packed;
		uint32_t s : 1 __packed;
		uint32_t tex : 3 __packed;
		uint32_t rsvd__22_23 : 2 __packed;
		uint32_t ap : 3 __packed;
		uint32_t rsvd__27 : 1 __packed;
		uint32_t xn : 1 __packed;
		uint32_t rsvd__29_31 : 3 __packed;
	} bit;
};

union __cpacr {    /* Coprocessor Access Control Register */
	uint32_t val;
	struct {
		uint32_t rsvd__0_19 : 20 __packed;
		uint32_t cp10 : 2 __packed;
		uint32_t cp11 : 2 __packed;
		uint32_t rsvd__24_31 : 8 __packed;
	} bit;
};

/* CP10 Access Bits */
#define _SCS_CPACR_CP10_Pos          20U
#define _SCS_CPACR_CP10_Msk          (3UL << _SCS_CPACR_CP10_Pos)
#define _SCS_CPACR_CP10_NO_ACCESS    (0UL << _SCS_CPACR_CP10_Pos)
#define _SCS_CPACR_CP10_PRIV_ACCESS  (1UL << _SCS_CPACR_CP10_Pos)
#define _SCS_CPACR_CP10_RESERVED     (2UL << _SCS_CPACR_CP10_Pos)
#define _SCS_CPACR_CP10_FULL_ACCESS  (3UL << _SCS_CPACR_CP10_Pos)

/* CP11 Access Bits */
#define _SCS_CPACR_CP11_Pos          22U
#define _SCS_CPACR_CP11_Msk          (3UL << _SCS_CPACR_CP11_Pos)
#define _SCS_CPACR_CP11_NO_ACCESS    (0UL << _SCS_CPACR_CP11_Pos)
#define _SCS_CPACR_CP11_PRIV_ACCESS  (1UL << _SCS_CPACR_CP11_Pos)
#define _SCS_CPACR_CP11_RESERVED     (2UL << _SCS_CPACR_CP11_Pos)
#define _SCS_CPACR_CP11_FULL_ACCESS  (3UL << _SCS_CPACR_CP11_Pos)

union __fpu_ccr {      /* FPU Context Control Register */
	uint32_t val;
	struct {
		uint32_t lspact : 1 __packed;
		uint32_t user : 1 __packed;
		uint32_t rsvd__2 : 1 __packed;
		uint32_t thread : 1 __packed;
		uint32_t hfrdy : 1 __packed;
		uint32_t mmrdy : 1 __packed;
		uint32_t bfrdy : 1 __packed;
		uint32_t rsvd__7 : 1 __packed;
		uint32_t monrdy : 1 __packed;
		uint32_t rsvd__9_29 : 21 __packed;
		uint32_t lspen : 1 __packed;
		uint32_t aspen : 1 __packed;
	} bit;
};

#define _SCS_FPU_CCR_ASPEN_Pos      31U
#define _SCS_FPU_CCR_ASPEN_Msk      (1UL << _SCS_FPU_CCR_ASPEN_Pos)
#define _SCS_FPU_CCR_ASPEN_ENABLE   (1UL << _SCS_FPU_CCR_ASPEN_Pos)
#define _SCS_FPU_CCR_ASPEN_DISABLE  (0UL << _SCS_FPU_CCR_ASPEN_Pos)

#define _SCS_FPU_CCR_LSPEN_Pos      30U
#define _SCS_FPU_CCR_LSPEN_Msk      (1UL << _SCS_FPU_CCR_LSPEN_Pos)
#define _SCS_FPU_CCR_LSPEN_ENABLE   (1UL << _SCS_FPU_CCR_LSPEN_Pos)
#define _SCS_FPU_CCR_LSPEN_DISABLE  (0UL << _SCS_FPU_CCR_LSPEN_Pos)

union __fpu_car {      /* FPU Context Address Register */
	uint32_t val;
	struct {
		uint32_t rsvd__0_2 : 3 __packed;
		uint32_t address : 29 __packed;
	} bit;
};

union __fpu_scr {      /* FPU Status Control Register */
	uint32_t val;
	struct {
		uint32_t ioc : 1 __packed;
		uint32_t dzc : 1 __packed;
		uint32_t ofc : 1 __packed;
		uint32_t ufc : 1 __packed;
		uint32_t ixc : 1 __packed;
		uint32_t rsvd__5_6 : 2 __packed;
		uint32_t idc : 1 __packed;
		uint32_t rsvd__8_21 : 14 __packed;
		uint32_t rmode : 2 __packed;
		uint32_t fz : 1 __packed;
		uint32_t dn : 1 __packed;
		uint32_t ahp : 1 __packed;
		uint32_t rsvd__27 : 1 __packed;
		uint32_t v : 1 __packed;
		uint32_t c : 1 __packed;
		uint32_t z : 1 __packed;
		uint32_t n : 1 __packed;
	} bit;
};

union __fpu_dscr {     /* FPU Default Status Control Register */
	uint32_t val;
	struct {
		uint32_t rsvd__0_21 : 22 __packed;
		uint32_t rmode : 2 __packed;
		uint32_t fz : 1 __packed;
		uint32_t dn : 1 __packed;
		uint32_t ahp : 1 __packed;
		uint32_t rsvd__27_31 : 5 __packed;
	} bit;
};

struct __scs {
	uint32_t rsvd__MasterControlRegister;
	union __ictr ictr;   /* 0x004 Interrupt Controller Type Register */
	union __actlr actlr; /* 0x008 Auxiliary ConTroL Register */
	uint32_t rsvd__00c_00f;

	/* system ticker (SYSTICK) */
	struct {
		union __stcsr stcsr; /* 0x10 Control and Status Register */
		uint32_t strvr; /* 0x14	Reload Value Register (low 24 bits) */
		uint32_t stcvr; /* 0x18	Current Value Register (low 24 bits) */
		uint32_t stcr;  /* 0x1c Calibration value Register */
	} systick;		/* offset: 0x010, size 0x10 */

	uint32_t rsvd__020_0ff[(0x100 - 0x20) / 4];

	/* Nested Vector Interrupt Controller (NVIC)
	 *
	 * Each block of 8 32-bit words could in theory support 256 IRQs, but
	 * the architecture only supports IRQs 0 -> 239.
	 */
	struct {
		uint32_t iser[8]; /* 0x100 Interrupt Set-Enable Registers */
		uint32_t rsvd__120_17f[24];

		uint32_t icer[8]; /* 0x180 Interrupt Clear-Enable Registers */
		uint32_t rsvd__1a0_1ff[24];

		uint32_t ispr[8]; /* 0x200 Interrupt Set-Pending Registers */
		uint32_t rsvd__220_27f[24];

		uint32_t icpr[8]; /* 0x280 Interrupt Clear-Pending Registers */
		uint32_t rsvd__2a0_2ff[24];

		uint32_t iabr[8]; /* 0x300 Interrupt Active-Bit Registers */
		uint32_t rsvd__320_37f[24];

		uint32_t rsvd__380_3ff[32];
#if defined(CONFIG_ARMV6_M)
		uint32_t ipr[8];
		uint32_t rsvd__420_4ff[56];
#elif defined(CONFIG_ARMV7_M)
		uint8_t ipr[240]; /* 0x400 Interrupt Priority Registers */
		uint32_t rsvd__4f0_4ff[4];
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
	} nvic; /* offset: 0x100, size 0x400 */

	uint32_t rsvd__500_cff[(0xd00 - 0x500) / 4];

	/* System Control Block (SCB) */
	struct {
		union __cpuid cpuid; /* 0xd00 CPUID register */
		union __icsr icsr;   /* 0xd04 IRQ Control and Start Register */
#if defined(CONFIG_ARMV6_M)
		uint32_t rsvd_9_12;
#elif defined(CONFIG_ARMV7_M)
		union __vtor vtor;   /* 0xd08 Vector Table Offset Register */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
		union __aircr
			aircr;    /* 0xd0c App IRQ and Reset Control Register */
		union __scr scr;  /* 0xd10 System Control Register */
		union __ccr ccr;  /* 0xd14 Configuration and Control Register */
#if defined(CONFIG_ARMV6_M)
		uint32_t rsvd_24_27;
		uint32_t shpr[2];
#elif defined(CONFIG_ARMV7_M)
		uint8_t shpr[12]; /* 0xd18 System Handler Priority Registers
				   *   Use ('exception number' - 4) to
				   *   get index into array
				   */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
		union __shcsr
			shcsr;     /* 0xd24 Sys Handler Control and State Reg */
#if defined(CONFIG_ARMV6_M)
		uint32_t rsvd_40_63[6];
#elif defined(CONFIG_ARMV7_M)
		union __cfsr cfsr; /* 0xd28 Configurable Fault Status Register
				    */
		union __hfsr hfsr; /* 0xd2C Hard Fault Status Register */
		uint32_t dfsr;     /* 0xd30 Debug Fault Status Register */
		uint32_t mmfar;    /* 0xd34 MemManage Fault Address Register */
		uint32_t bfar;     /* 0xd38 BusFault Address Register */
		uint32_t afsr;     /* 0xd3C Aux Fault Status Register */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
	} scb;			   /* offset: 0xd00, size 0x040 */

	/*
	 * d40 -> d7f: processor feature ID registers (pp.778-779 in DDI0403D)
	 */
	uint32_t rsvd__d40_d87[(0xd88 - 0xd40) / 4];

	union __cpacr cpacr;       /* 0xd88 Coprocessor Access Control Reg */

	uint32_t rsvd__d8c_d8f;

	/* Memory Protection Unit (MPU) */
	struct { /* 0xD90-0xDA3 */
		union __mpu_type mpu_type; /* 0xd90 RO TYPE register */
		union __mpu_ctrl mpu_ctrl; /* 0xd94 RW ConTRoL register */
		union __mpu_rnr mpu_rnr;   /* 0xd98 RW Region Number Register */
		union __mpu_rbar mpu_rbar; /* 0xd9c RW Region Base Addr Reg. */
		union __mpu_rasr mpu_rasr; /* 0xda0 RW Region Attr and Size
					    * Reg.
					    */
		union __mpu_rbar mpu_rbar_a1; /* 0xda4 RW alias of mpu_rbar */
		union __mpu_rasr mpu_rasr_a1; /* 0xda8 RW alias of mpu_rasr */
		union __mpu_rbar mpu_rbar_a2; /* 0xdac RW alias of mpu_rbar */
		union __mpu_rasr mpu_rasr_a2; /* 0xdb0 RW alias of mpu_rasr */
		union __mpu_rbar mpu_rbar_a3; /* 0xdb4 RW alias of mpu_rbar */
		union __mpu_rasr mpu_rasr_a3; /* 0xdb8 RW alias of mpu_rasr */
	} mpu;				      /* offset: 0xd90, size: 0x02c */

	uint32_t rsvd__da4_eff[(0xf00 - 0xdbc) / 4];

	/* 0xf00 WO SW Trigger IRQ Reg. (bit 0-8/IRQ 0-239 only) */
	uint32_t stir;

	uint32_t rsvd__f04_f33[(0xf34 - 0xF04) / 4];

	struct { /* 0xF34-F3F */
		union __fpu_ccr  ccr;    /* 0xf34 Context Control Reg */
		union __fpu_car  car;    /* 0xf38 Context Address Reg */
		union __fpu_dscr dscr;   /* 0xf3c Default Status Control Reg */
	} fpu;                           /* offset: 0xf34, size: 0x0c */

	uint32_t rsvd__f40_fff[(0x1000 - 0xf40) / 4];
};

/* the linker always puts this object at 0xe000e000 */
extern volatile struct __scs __scs;

#if defined(CONFIG_ARMV6_M)
/* Interrupt Priorities are WORD accessible only under ARMv6M */
/* The following MACROS handle generation of the register offset and masks */
#define _PRIO_BIT_SHIFT(IRQn) (((((uint32_t)(IRQn))) & 0x03UL) * 8UL)
#define _PRIO_SHP_IDX(IRQn)   ((((((uint32_t)(IRQn)) & 0x0FUL)-8UL) >> 2UL))
#define _PRIO_IP_IDX(IRQn)    ((((uint32_t)(IRQn)) >> 2UL))
#elif defined(CONFIG_ARMV7_M)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SCS__H_ */
