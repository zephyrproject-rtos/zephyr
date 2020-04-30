/*
 * Copyright (c) 2020, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_lpc55s69 platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_lpc55s69 platform.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/uart.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <aarch32/cortex_m/exc.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_device_registers.h>

#define SYSTEM_IS_XIP_FLEXSPI() \
	((((uint32_t)nxp_rt600_init >= 0x08000000U) &&		\
	  ((uint32_t)nxp_rt600_init < 0x10000000U)) ||		\
	 (((uint32_t)nxp_rt600_init >= 0x18000000U) &&		\
	  ((uint32_t)nxp_rt600_init < 0x20000000U)))

#ifdef CONFIG_INIT_SYS_PLL
const clock_sys_pll_config_t g_sysPllConfig = {
	.sys_pll_src  = kCLOCK_SysPllXtalIn,
	.numerator	  = 0,
	.denominator  = 1,
	.sys_pll_mult = kCLOCK_SysPllMult22
};
#endif

#ifdef CONFIG_INIT_AUDIO_PLL
const clock_audio_pll_config_t g_audioPllConfig = {
	.audio_pll_src	= kCLOCK_AudioPllXtalIn,
	.numerator		= 5040,
	.denominator	= 27000,
	.audio_pll_mult = kCLOCK_AudioPllMult22
};
#endif

#ifdef CONFIG_NXP_IMX_RT6XX_BOOT_HEADER
extern char z_main_stack[];
extern char _flash_used[];

extern void z_arm_reset(void);
extern void z_arm_nmi(void);
extern void z_arm_hard_fault(void);
extern void z_arm_mpu_fault(void);
extern void z_arm_bus_fault(void);
extern void z_arm_usage_fault(void);
extern void z_arm_secure_fault(void);
extern void z_arm_svc(void);
extern void z_arm_debug_monitor(void);
extern void z_arm_pendsv(void);
extern void z_clock_isr(void);
extern void z_arm_exc_spurious(void);

__imx_boot_ivt_section void (* const image_vector_table[])(void)  = {
	(void (*)())(z_main_stack + CONFIG_MAIN_STACK_SIZE),  /* 0x00 */
	z_arm_reset,				/* 0x04 */
	z_arm_nmi,					/* 0x08 */
	z_arm_hard_fault,			/* 0x0C */
	z_arm_mpu_fault,			/* 0x10 */
	z_arm_bus_fault,			/* 0x14 */
	z_arm_usage_fault,			/* 0x18 */
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	z_arm_secure_fault,			/* 0x1C */
#else
	z_arm_exc_spurious,
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
	(void (*)())_flash_used,	/* 0x20, imageLength. */
	0,				/* 0x24, imageType (Plain Image) */
	0,				/* 0x28, authBlockOffset/crcChecksum */
	z_arm_svc,		/* 0x2C */
	z_arm_debug_monitor,	/* 0x30 */
	(void (*)())image_vector_table,		/* 0x34, imageLoadAddress. */
	z_arm_pendsv,						/* 0x38 */
#if defined(CONFIG_SYS_CLOCK_EXISTS)
	z_clock_isr,						/* 0x3C */
#else
	z_arm_exc_spurious,
#endif
};
#endif /* CONFIG_NXP_IMX_RT6XX_BOOT_HEADER */

/**
 *
 * @brief Initialize the system clock
 *
 * @return N/A
 *
 */

static ALWAYS_INLINE void clock_init(void)
{
#ifdef CONFIG_SOC_MIMXRT685S_CM33
	/* Configure LPOSC clock*/
	if ((SYSCTL0->PDRUNCFG0 & SYSCTL0_PDRUNCFG0_LPOSC_PD_MASK) != 0) {
		POWER_DisablePD(kPDRUNCFG_PD_LPOSC);
	}

	/* Configure FFRO clock */
	if ((SYSCTL0->PDRUNCFG0 & SYSCTL0_PDRUNCFG0_FFRO_PD_MASK) != 0) {
		POWER_DisablePD(kPDRUNCFG_PD_FFRO);
		CLOCK_EnableFfroClk(kCLOCK_Ffro48M);
	}
	if ((SYSCTL0->PDRUNCFG0 & SYSCTL0_PDRUNCFG0_SFRO_PD_MASK) != 0) {
		/* Configure SFRO clock */
		POWER_DisablePD(kPDRUNCFG_PD_SFRO);
		CLOCK_EnableSfroClk();
	}

	if ((SYSCTL0->PDRUNCFG0 & SYSCTL0_PDRUNCFG0_SYSXTAL_PD_MASK) != 0) {
		/* Configure SYSOSC clock source */
		POWER_DisablePD(kPDRUNCFG_PD_SYSXTAL);
		CLOCK_EnableSysOscClk(true, true, BOARD_SYSOSC_SETTLING_US);
	}
	CLOCK_SetXtalFreq(BOARD_XTAL_SYS_CLK_HZ);

	/* Let CPU and AHB bus run on FFRO 48MHz for safe switching. */
	CLOCK_AttachClk(kFFRO_to_MAIN_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 1U);

#ifdef CONFIG_INIT_SYS_PLL
	/* Configure SysPLL0 clock source */
	CLOCK_InitSysPll(&g_sysPllConfig);
	CLOCK_InitSysPfd(kCLOCK_Pfd0, 19);
	CLOCK_InitSysPfd(kCLOCK_Pfd2, 24);
#endif

	/* Set FRGPLLCLKDIV divider to value 12 */
	CLOCK_SetClkDiv(kCLOCK_DivPllFrgClk, 12U);

#ifdef CONFIG_INIT_AUDIO_PLL
	/* Configure Audio PLL clock source */
	CLOCK_InitAudioPll(&g_audioPllConfig);
	CLOCK_InitAudioPfd(kCLOCK_Pfd0, 26);
	CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 15U);
#endif

	CLOCK_AttachClk(kSFRO_to_FLEXCOMM0);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_i2c, okay)
	CLOCK_AttachClk(kSFRO_to_FLEXCOMM2);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm5), nxp_lpc_spi, okay)
	CLOCK_AttachClk(kFFRO_to_FLEXCOMM5);
#endif

#endif /* CONFIG_SOC_MIMXRT685S_CM33 */
}

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int nxp_rt600_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* old interrupt lock level */
	unsigned int oldLevel;

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Enable cache to accelerate boot. */
	if (SYSTEM_IS_XIP_FLEXSPI() && (CACHE64_POLSEL->POLSEL == 0)) {
		/*
		 * Set command to invalidate all ways and write GO bit
		 * to initiate command
		 */
		CACHE64->CCR = (CACHE64_CTRL_CCR_INVW1_MASK |
					CACHE64_CTRL_CCR_INVW0_MASK);
		CACHE64->CCR |= CACHE64_CTRL_CCR_GO_MASK;
		/* Wait until the command completes */
		while (CACHE64->CCR & CACHE64_CTRL_CCR_GO_MASK) {
		}
		/* Enable cache, enable write buffer */
		CACHE64->CCR = (CACHE64_CTRL_CCR_ENWRBUF_MASK |
						CACHE64_CTRL_CCR_ENCACHE_MASK);

		/* Set whole FlexSPI0 space to write through. */
		CACHE64_POLSEL->REG0_TOP = 0x07FFFC00U;
		CACHE64_POLSEL->REG1_TOP = 0x0U;
		CACHE64_POLSEL->POLSEL = 0x1U;

		__ISB();
		__DSB();
	}

	/* Initialize clock */
	clock_init();

	/*
	 * install default handler that simply resets the CPU if configured in
	 * the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);

	return 0;
}

SYS_INIT(nxp_rt600_init, PRE_KERNEL_1, 0);
