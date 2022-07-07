/*
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for NXP RT5XX platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the RT5XX platforms.
 */

#include <zephyr/init.h>
#include <soc.h>
#include "flash_clock_setup.h"
#include "fsl_power.h"
#include "fsl_clock.h"

/* Board System oscillator settling time in us */
#define BOARD_SYSOSC_SETTLING_US                        100U
/* Board xtal frequency in Hz */
#define BOARD_XTAL_SYS_CLK_HZ                      24000000U
/* Core clock frequency: 198000000Hz */
#define CLOCK_INIT_CORE_CLOCK                     198000000U

#define CTIMER_CLOCK_SOURCE(node_id) \
	TO_CTIMER_CLOCK_SOURCE(DT_CLOCKS_CELL(node_id, name), DT_PROP(node_id, clk_source))
#define TO_CTIMER_CLOCK_SOURCE(inst, val) TO_CLOCK_ATTACH_ID(inst, val)
#define TO_CLOCK_ATTACH_ID(inst, val) CLKCTL1_TUPLE_MUXA(CT32BIT##inst##FCLKSEL_OFFSET, val)
#define CTIMER_CLOCK_SETUP(node_id) CLOCK_AttachClk(CTIMER_CLOCK_SOURCE(node_id));

const clock_sys_pll_config_t g_sysPllConfig_clock_init = {
	/* OSC clock */
	.sys_pll_src = kCLOCK_SysPllXtalIn,
	/* Numerator of the SYSPLL0 fractional loop divider is 0 */
	.numerator = 0,
	/* Denominator of the SYSPLL0 fractional loop divider is 1 */
	.denominator = 1,
	/* Divide by 22 */
	.sys_pll_mult = kCLOCK_SysPllMult22
};

const clock_audio_pll_config_t g_audioPllConfig_clock_init = {
	/* OSC clock */
	.audio_pll_src = kCLOCK_AudioPllXtalIn,
	/* Numerator of the Audio PLL fractional loop divider is 0 */
	.numerator = 5040,
	/* Denominator of the Audio PLL fractional loop divider is 1 */
	.denominator = 27000,
	/* Divide by 22 */
	.audio_pll_mult = kCLOCK_AudioPllMult22
};

const clock_frg_clk_config_t g_frg0Config_clock_init = {
	.num = 0,
	.sfg_clock_src = kCLOCK_FrgPllDiv,
	.divider = 255U,
	.mult = 0
};

const clock_frg_clk_config_t g_frg12Config_clock_init = {
	.num = 12,
	.sfg_clock_src = kCLOCK_FrgMainClk,
	.divider = 255U,
	.mult = 167
};

/* System clock frequency. */
extern uint32_t SystemCoreClock;

#ifdef CONFIG_NXP_IMX_RT5XX_BOOT_HEADER
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
extern void sys_clock_isr(void);
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
#if defined(CONFIG_SYS_CLOCK_EXISTS) && \
		defined(CONFIG_CORTEX_M_SYSTICK_INSTALL_ISR)
	sys_clock_isr,						/* 0x3C */
#else
	z_arm_exc_spurious,
#endif
};
#endif /* CONFIG_NXP_IMX_RT5XX_BOOT_HEADER */

void z_arm_platform_init(void)
{
	/* This is provided by the SDK */
	SystemInit();
}

void clock_init(void)
{
	/* Configure LPOSC 1M */
	/* Power on LPOSC (1MHz) */
	POWER_DisablePD(kPDRUNCFG_PD_LPOSC);
	/* Wait until LPOSC stable */
	CLOCK_EnableLpOscClk();

	/* Configure FRO clock source */
	/* Power on FRO (192MHz or 96MHz) */
	POWER_DisablePD(kPDRUNCFG_PD_FFRO);
	/* FRO_DIV1 is always enabled and used as Main clock during PLL update. */
	/* Enable all FRO outputs */
	CLOCK_EnableFroClk(kCLOCK_FroAllOutEn);

	/*
	 * Call function flexspi_clock_safe_config() to move FlexSPI clock to a stable
	 * clock source to avoid instruction/data fetch issue when updating PLL and Main
	 * clock if XIP(execute code on FLEXSPI memory).
	 */
	flexspi_clock_safe_config();

	/* Let CPU run on FRO with divider 2 for safe switching. */
	CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 2);
	CLOCK_AttachClk(kFRO_DIV1_to_MAIN_CLK);

	/* Configure SYSOSC clock source. */
	/* Power on SYSXTAL */
	POWER_DisablePD(kPDRUNCFG_PD_SYSXTAL);
	/* Updated XTAL oscillator settling time */
	POWER_UpdateOscSettlingTime(BOARD_SYSOSC_SETTLING_US);
	/* Enable system OSC */
	CLOCK_EnableSysOscClk(true, true, BOARD_SYSOSC_SETTLING_US);
	/* Sets external XTAL OSC freq */
	CLOCK_SetXtalFreq(BOARD_XTAL_SYS_CLK_HZ);

	/* Configure SysPLL0 clock source. */
	CLOCK_InitSysPll(&g_sysPllConfig_clock_init);
	/* Enable MAIN PLL clock */
	CLOCK_InitSysPfd(kCLOCK_Pfd0, 24);
	/* Enable AUX0 PLL clock */
	CLOCK_InitSysPfd(kCLOCK_Pfd2, 24);

	/* Configure Audio PLL clock source. */
	CLOCK_InitAudioPll(&g_audioPllConfig_clock_init);
	/* Enable Audio PLL clock */
	CLOCK_InitAudioPfd(kCLOCK_Pfd0, 26);

	/* Set MAINPLLCLKDIV divider to value 5 */
	CLOCK_SetClkDiv(kCLOCK_DivMainPllClk, 5U);

	/* Set SYSCPUAHBCLKDIV divider to value 2 */
	CLOCK_SetClkDiv(kCLOCK_DivSysCpuAhbClk, 2U);

	/* Setup FRG0 clock */
	CLOCK_SetFRGClock(&g_frg0Config_clock_init);
	/* Setup FRG12 clock */
	CLOCK_SetFRGClock(&g_frg12Config_clock_init);

	/* Set up clock selectors - Attach clocks to the peripheries. */
	/* Switch MAIN_CLK to MAIN_PLL */
	CLOCK_AttachClk(kMAIN_PLL_to_MAIN_CLK);
	/* Switch SYSTICK_CLK to MAIN_CLK_DIV */
	CLOCK_AttachClk(kMAIN_CLK_DIV_to_SYSTICK_CLK);
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_usart, okay)
	/* Switch FLEXCOMM0 to FRG */
	CLOCK_AttachClk(kFRG_to_FLEXCOMM0);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_i2c, okay)
	/* Switch FLEXCOMM4 to FRO_DIV4 */
	CLOCK_AttachClk(kFRO_DIV4_to_FLEXCOMM4);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(hs_spi1), nxp_lpc_spi, okay)
	CLOCK_AttachClk(kFRO_DIV4_to_FLEXCOMM16);
#endif
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm12), nxp_lpc_usart, okay)
	/* Switch FLEXCOMM12 to FRG */
	CLOCK_AttachClk(kFRG_to_FLEXCOMM12);
#endif
	/* Switch CLKOUT to FRO_DIV2 */
	CLOCK_AttachClk(kFRO_DIV2_to_CLKOUT);

	DT_FOREACH_STATUS_OKAY(nxp_lpc_ctimer, CTIMER_CLOCK_SETUP)

	/* Set up dividers. */
	/* Set AUDIOPLLCLKDIV divider to value 15 */
	CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 15U);
	/* Set FRGPLLCLKDIV divider to value 11 */
	CLOCK_SetClkDiv(kCLOCK_DivPLLFRGClk, 11U);
	/* Set SYSTICKFCLKDIV divider to value 2 */
	CLOCK_SetClkDiv(kCLOCK_DivSystickClk, 2U);
	/* Set PFC0DIV divider to value 2 */
	CLOCK_SetClkDiv(kCLOCK_DivPfc0Clk, 2U);
	/* Set PFC1DIV divider to value 4 */
	CLOCK_SetClkDiv(kCLOCK_DivPfc1Clk, 4U);
	/* Set CLKOUTFCLKDIV divider to value 100 */
	CLOCK_SetClkDiv(kCLOCK_DivClockOut, 100U);

	/*
	 * Call function flexspi_setup_clock() to set user configured clock source/divider
	 * for FlexSPI.
	 */
	flexspi_setup_clock(FLEXSPI0, 0U, 2U);

	/* Set SystemCoreClock variable. */
	SystemCoreClock = CLOCK_INIT_CORE_CLOCK;

	/* Set main clock to FRO as deep sleep clock by default. */
	POWER_SetDeepSleepClock(kDeepSleepClk_Fro);
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

static int nxp_rt500_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* old interrupt lock level */
	unsigned int oldLevel;

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Initialize clocks with tool generated code */
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

SYS_INIT(nxp_rt500_init, PRE_KERNEL_1, 0);
