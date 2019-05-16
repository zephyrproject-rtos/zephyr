/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Based on NXP k6x soc.c, which is:
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <fsl_clock.h>
#include <fsl_cache.h>
#include <cortex_m/exc.h>

/*
 * KE1xF flash configuration fields
 * These 16 bytes, which must be loaded to address 0x400, include default
 * protection, boot options and security settings.
 * They are loaded at reset to various Flash Memory module (FTFE) registers.
 */
u8_t __kinetis_flash_config_section __kinetis_flash_config[] = {
	/* Backdoor Comparison Key (unused) */
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	/* Program flash protection */
	0xFF, 0xFF, 0xFF, 0xFF,
	/*
	 * Flash security: Backdoor key disabled, Mass erase enabled,
	 *                 Factory access enabled, MCU is unsecure
	 */
	0xFE,
	/*
	 * Flash nonvolatile option: Boot from ROM with BOOTCFG0/NMI
	 *                           pin low, boot from flash with
	 *                           BOOTCFG0/NMI pin high, RESET_b
	 *                           pin dedicated, NMI enabled,
	 *                           normal boot
	 */
	0x7d,
	/* EEPROM protection */
	0xFF,
	/* Data flash protection */
	0xFF,
};

#define ASSERT_WITHIN_RANGE(val, min, max, str) \
	BUILD_ASSERT_MSG(val >= min && val <= max, str)

#define ASSERT_ASYNC_CLK_DIV_VALID(val, str) \
	BUILD_ASSERT_MSG(val == 0 || val == 1 || val == 2 || val == 4 || \
			 val == 8 || val == 16 || val == 2 || val == 64, str)

#define TO_SYS_CLK_DIV(val) _DO_CONCAT(kSCG_SysClkDivBy, val)

#define kSCG_AsyncClkDivBy0 kSCG_AsyncClkDisable
#define TO_ASYNC_CLK_DIV(val) _DO_CONCAT(kSCG_AsyncClkDivBy, val)

/* System Clock configuration */
ASSERT_WITHIN_RANGE(DT_NXP_KINETIS_SCG_0_CLK_DIVIDER_SLOW, 2, 8,
		    "Invalid SCG slow clock divider value");
ASSERT_WITHIN_RANGE(DT_NXP_KINETIS_SCG_0_CLK_DIVIDER_BUS, 1, 16,
		    "Invalid SCG bus clock divider value");
#if DT_NXP_KINETIS_SCG_0_CLK_SOURCE == KINETIS_SCG_SCLK_SRC_SPLL
/* Core divider range is 1 to 4 with SPLL as clock source */
ASSERT_WITHIN_RANGE(DT_NXP_KINETIS_SCG_0_CLK_DIVIDER_CORE, 1, 4,
		    "Invalid SCG core clock divider value");
#else
ASSERT_WITHIN_RANGE(DT_NXP_KINETIS_SCG_0_CLK_DIVIDER_CORE, 1, 16,
		    "Invalid SCG core clock divider value");
#endif
static const scg_sys_clk_config_t scg_sys_clk_config = {
	.divSlow = TO_SYS_CLK_DIV(DT_NXP_KINETIS_SCG_0_CLK_DIVIDER_SLOW),
	.divBus  = TO_SYS_CLK_DIV(DT_NXP_KINETIS_SCG_0_CLK_DIVIDER_BUS),
	.divCore = TO_SYS_CLK_DIV(DT_NXP_KINETIS_SCG_0_CLK_DIVIDER_CORE),
	.src     = DT_NXP_KINETIS_SCG_0_CLK_SOURCE
};

#ifdef DT_NXP_KINETIS_SCG_0_SOSC_FREQ
/* System Oscillator (SOSC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(DT_NXP_KINETIS_SCG_0_SOSC_DIVIDER_1,
		       "Invalid SCG SOSC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(DT_NXP_KINETIS_SCG_0_SOSC_DIVIDER_2,
		       "Invalid SCG SOSC divider 2 value");
static const scg_sosc_config_t scg_sosc_config = {
	.freq        = DT_NXP_KINETIS_SCG_0_SOSC_FREQ,
	.monitorMode = kSCG_SysOscMonitorDisable,
	.enableMode  = kSCG_SysOscEnable | kSCG_SysOscEnableInLowPower,
	.div1        = TO_ASYNC_CLK_DIV(DT_NXP_KINETIS_SCG_0_SOSC_DIVIDER_1),
	.div2        = TO_ASYNC_CLK_DIV(DT_NXP_KINETIS_SCG_0_SOSC_DIVIDER_2),
	.workMode    = DT_NXP_KINETIS_SCG_0_SOSC_MODE
};
#endif /* DT_NXP_KINETIS_SCG_0_SOSC_FREQ */

/* Slow Internal Reference Clock (SIRC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(DT_NXP_KINETIS_SCG_0_SIRC_DIVIDER_1,
		       "Invalid SCG SIRC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(DT_NXP_KINETIS_SCG_0_SIRC_DIVIDER_2,
		       "Invalid SCG SIRC divider 2 value");
static const scg_sirc_config_t scg_sirc_config = {
	.enableMode = kSCG_SircEnable,
	.div1       = TO_ASYNC_CLK_DIV(DT_NXP_KINETIS_SCG_0_SIRC_DIVIDER_1),
	.div2       = TO_ASYNC_CLK_DIV(DT_NXP_KINETIS_SCG_0_SIRC_DIVIDER_2),
#if MHZ(2) == DT_NXP_KINETIS_SCG_0_SIRC_RANGE
	.range      = kSCG_SircRangeLow
#elif MHZ(8) == DT_NXP_KINETIS_SCG_0_SIRC_RANGE
	.range      = kSCG_SircRangeHigh
#else
#error Invalid SCG SIRC range
#endif
};

/* Fast Internal Reference Clock (FIRC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(DT_NXP_KINETIS_SCG_0_FIRC_DIVIDER_1,
		       "Invalid SCG FIRC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(DT_NXP_KINETIS_SCG_0_FIRC_DIVIDER_2,
		       "Invalid SCG FIRC divider 2 value");
static const scg_firc_config_t scg_firc_config = {
	.enableMode = kSCG_FircEnable,
	.div1       = TO_ASYNC_CLK_DIV(DT_NXP_KINETIS_SCG_0_FIRC_DIVIDER_1),
	.div2       = TO_ASYNC_CLK_DIV(DT_NXP_KINETIS_SCG_0_FIRC_DIVIDER_2),
#if MHZ(48) == DT_NXP_KINETIS_SCG_0_FIRC_RANGE
	.range      = kSCG_FircRange48M,
#elif MHZ(52) == DT_NXP_KINETIS_SCG_0_FIRC_RANGE
	.range      = kSCG_FircRange52M,
#elif MHZ(56) == DT_NXP_KINETIS_SCG_0_FIRC_RANGE
	.range      = kSCG_FircRange56M,
#elif MHZ(60) == DT_NXP_KINETIS_SCG_0_FIRC_RANGE
	.range      = kSCG_FircRange60M,
#else
#error Invalid SCG FIRC range
#endif
	.trimConfig = NULL
};

/* System Phase-Locked Loop (SPLL) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(DT_NXP_KINETIS_SCG_0_SPLL_DIVIDER_1,
		       "Invalid SCG SPLL divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(DT_NXP_KINETIS_SCG_0_SPLL_DIVIDER_2,
		       "Invalid SCG SPLL divider 2 value");
ASSERT_WITHIN_RANGE(DT_NXP_KINETIS_SCG_0_SPLL_DIVIDER_PRE, 1, 8,
		    "Invalid SCG SPLL pre divider value");
ASSERT_WITHIN_RANGE(DT_NXP_KINETIS_SCG_0_SPLL_MULTIPLIER, 16, 47,
		    "Invalid SCG SPLL multiplier value");
static const scg_spll_config_t scg_spll_config = {
	.enableMode  = kSCG_SysPllEnable,
	.monitorMode = kSCG_SysPllMonitorDisable,
	.div1        = TO_ASYNC_CLK_DIV(DT_NXP_KINETIS_SCG_0_SPLL_DIVIDER_1),
	.div2        = TO_ASYNC_CLK_DIV(DT_NXP_KINETIS_SCG_0_SPLL_DIVIDER_2),
#if DT_NXP_KINETIS_SCG_0_SPLL_SOURCE == KINETIS_SCG_SPLL_SRC_SOSC
	.src         = kSCG_SysPllSrcSysOsc,
#elif DT_NXP_KINETIS_SCG_0_SPLL_SOURCE == KINETIS_SCG_SPLL_SRC_FIRC
	.src         = kSCG_SysPllSrcFirc,
#else
#error Invalid SCG SPLL source
#endif
	.prediv      = (DT_NXP_KINETIS_SCG_0_SPLL_DIVIDER_PRE - 1U),
	.mult        = (DT_NXP_KINETIS_SCG_0_SPLL_MULTIPLIER - 16U)
};

static ALWAYS_INLINE void clk_init(void)
{
	const scg_sys_clk_config_t scg_sys_clk_config_safe = {
		.divSlow = kSCG_SysClkDivBy4,
		.divBus  = kSCG_SysClkDivBy1,
		.divCore = kSCG_SysClkDivBy1,
		.src     = kSCG_SysClkSrcSirc
	};
	scg_sys_clk_config_t current;

#ifdef DT_NXP_KINETIS_SCG_0_SOSC_FREQ
	/* Optionally initialize system oscillator */
	CLOCK_InitSysOsc(&scg_sosc_config);
	CLOCK_SetXtal0Freq(scg_sosc_config.freq);
#endif
	/* Configure SIRC */
	CLOCK_InitSirc(&scg_sirc_config);

	/* Temporary switch to safe SIRC in order to configure FIRC */
	CLOCK_SetRunModeSysClkConfig(&scg_sys_clk_config_safe);
	do {
		CLOCK_GetCurSysClkConfig(&current);
	} while (current.src != scg_sys_clk_config_safe.src);
	CLOCK_InitFirc(&scg_firc_config);

	/* Configure System PLL */
	CLOCK_InitSysPll(&scg_spll_config);

	/* Only RUN mode supported for now */
	CLOCK_SetRunModeSysClkConfig(&scg_sys_clk_config);
	do {
		CLOCK_GetCurSysClkConfig(&current);
	} while (current.src != scg_sys_clk_config.src);

#ifdef CONFIG_UART_MCUX_LPUART_0
	CLOCK_SetIpSrc(kCLOCK_Lpuart0, kCLOCK_IpSrcFircAsync);
#endif
#ifdef CONFIG_UART_MCUX_LPUART_1
	CLOCK_SetIpSrc(kCLOCK_Lpuart1, kCLOCK_IpSrcFircAsync);
#endif
#ifdef CONFIG_UART_MCUX_LPUART_2
	CLOCK_SetIpSrc(kCLOCK_Lpuart2, kCLOCK_IpSrcFircAsync);
#endif
#ifdef DT_NXP_KINETIS_SCG_0_CLKOUT_SOURCE
	CLOCK_SetClkOutSel(DT_NXP_KINETIS_SCG_0_CLKOUT_SOURCE);
#endif
}

static int ke1xf_init(struct device *arg)

{
	ARG_UNUSED(arg);

	unsigned int old_level; /* old interrupt lock level */
#if !defined(CONFIG_ARM_MPU)
	u32_t temp_reg;
#endif /* !CONFIG_ARM_MPU */

	/* Disable interrupts */
	old_level = irq_lock();

#if !defined(CONFIG_ARM_MPU)
	/*
	 * Disable memory protection and clear slave port errors.
	 * Note that the KE1xF does not implement the optional ARMv7-M memory
	 * protection unit (MPU), specified by the architecture (PMSAv7), in the
	 * Cortex-M4 core.  Instead, the processor includes its own MPU module.
	 */
	temp_reg = SYSMPU->CESR;
	temp_reg &= ~SYSMPU_CESR_VLD_MASK;
	temp_reg |= SYSMPU_CESR_SPERR_MASK;
	SYSMPU->CESR = temp_reg;
#endif /* !CONFIG_ARM_MPU */

	/* Initialize system clocks and PLL */
	clk_init();

	/*
	 * Install default handler that simply resets the CPU if
	 * configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

#ifdef CONFIG_KINETIS_KE1XF_ENABLE_CODE_CACHE
	L1CACHE_EnableCodeCache();
#endif
	/* Restore interrupt state */
	irq_unlock(old_level);

	return 0;
}

void _WdogInit(void)
{
	/*
	 * NOTE: DO NOT SINGLE STEP THROUGH THIS FUNCTION!!! Watchdog
	 * reconfiguration must take place within 128 bus clocks from
	 * unlocking. Single stepping through the code will cause the
	 * watchdog to close the unlock window again.
	 */

	/* Unlock watchdog to enable reconfiguration after bootloader */
	WDOG->CNT = WDOG_UPDATE_KEY;
	while (!(WDOG->CS & WDOG_CS_ULK_MASK)) {
		;
	}

	/*
	 * Watchdog reconfiguration only takes effect after writing to
	 * both TOVAL and CS registers.
	 */
	WDOG->TOVAL = 1024;
	WDOG->CS = WDOG_CS_EN(0) | WDOG_CS_UPDATE(1);
	while (!(WDOG->CS & WDOG_CS_RCS_MASK)) {
		;
	}
}

SYS_INIT(ke1xf_init, PRE_KERNEL_1, 0);
