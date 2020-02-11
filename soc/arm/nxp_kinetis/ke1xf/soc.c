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
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#define ASSERT_WITHIN_RANGE(val, min, max, str) \
	BUILD_ASSERT_MSG(val >= min && val <= max, str)

#define ASSERT_ASYNC_CLK_DIV_VALID(val, str) \
	BUILD_ASSERT_MSG(val == 0 || val == 1 || val == 2 || val == 4 || \
			 val == 8 || val == 16 || val == 2 || val == 64, str)

#define TO_SYS_CLK_DIV(val) _DO_CONCAT(kSCG_SysClkDivBy, val)

#define kSCG_AsyncClkDivBy0 kSCG_AsyncClkDisable
#define TO_ASYNC_CLK_DIV(val) _DO_CONCAT(kSCG_AsyncClkDivBy, val)

/* System Clock configuration */
ASSERT_WITHIN_RANGE(DT_INST_0_NXP_KINETIS_SCG_CLK_DIVIDER_SLOW, 2, 8,
		    "Invalid SCG slow clock divider value");
ASSERT_WITHIN_RANGE(DT_INST_0_NXP_KINETIS_SCG_CLK_DIVIDER_BUS, 1, 16,
		    "Invalid SCG bus clock divider value");
#if DT_INST_0_NXP_KINETIS_SCG_CLK_SOURCE == KINETIS_SCG_SCLK_SRC_SPLL
/* Core divider range is 1 to 4 with SPLL as clock source */
ASSERT_WITHIN_RANGE(DT_INST_0_NXP_KINETIS_SCG_CLK_DIVIDER_CORE, 1, 4,
		    "Invalid SCG core clock divider value");
#else
ASSERT_WITHIN_RANGE(DT_INST_0_NXP_KINETIS_SCG_CLK_DIVIDER_CORE, 1, 16,
		    "Invalid SCG core clock divider value");
#endif
static const scg_sys_clk_config_t scg_sys_clk_config = {
	.divSlow = TO_SYS_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_CLK_DIVIDER_SLOW),
	.divBus  = TO_SYS_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_CLK_DIVIDER_BUS),
	.divCore = TO_SYS_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_CLK_DIVIDER_CORE),
	.src     = DT_INST_0_NXP_KINETIS_SCG_CLK_SOURCE
};

#ifdef DT_INST_0_NXP_KINETIS_SCG_SOSC_FREQ
/* System Oscillator (SOSC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_SCG_SOSC_DIVIDER_1,
		       "Invalid SCG SOSC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_SCG_SOSC_DIVIDER_2,
		       "Invalid SCG SOSC divider 2 value");
static const scg_sosc_config_t scg_sosc_config = {
	.freq        = DT_INST_0_NXP_KINETIS_SCG_SOSC_FREQ,
	.monitorMode = kSCG_SysOscMonitorDisable,
	.enableMode  = kSCG_SysOscEnable | kSCG_SysOscEnableInLowPower,
	.div1        = TO_ASYNC_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_SOSC_DIVIDER_1),
	.div2        = TO_ASYNC_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_SOSC_DIVIDER_2),
	.workMode    = DT_INST_0_NXP_KINETIS_SCG_SOSC_MODE
};
#endif /* DT_INST_0_NXP_KINETIS_SCG_SOSC_FREQ */

/* Slow Internal Reference Clock (SIRC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_SCG_SIRC_DIVIDER_1,
		       "Invalid SCG SIRC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_SCG_SIRC_DIVIDER_2,
		       "Invalid SCG SIRC divider 2 value");
static const scg_sirc_config_t scg_sirc_config = {
	.enableMode = kSCG_SircEnable,
	.div1       = TO_ASYNC_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_SIRC_DIVIDER_1),
	.div2       = TO_ASYNC_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_SIRC_DIVIDER_2),
#if MHZ(2) == DT_INST_0_NXP_KINETIS_SCG_SIRC_RANGE
	.range      = kSCG_SircRangeLow
#elif MHZ(8) == DT_INST_0_NXP_KINETIS_SCG_SIRC_RANGE
	.range      = kSCG_SircRangeHigh
#else
#error Invalid SCG SIRC range
#endif
};

/* Fast Internal Reference Clock (FIRC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_SCG_FIRC_DIVIDER_1,
		       "Invalid SCG FIRC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_SCG_FIRC_DIVIDER_2,
		       "Invalid SCG FIRC divider 2 value");
static const scg_firc_config_t scg_firc_config = {
	.enableMode = kSCG_FircEnable,
	.div1       = TO_ASYNC_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_FIRC_DIVIDER_1),
	.div2       = TO_ASYNC_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_FIRC_DIVIDER_2),
#if MHZ(48) == DT_INST_0_NXP_KINETIS_SCG_FIRC_RANGE
	.range      = kSCG_FircRange48M,
#elif MHZ(52) == DT_INST_0_NXP_KINETIS_SCG_FIRC_RANGE
	.range      = kSCG_FircRange52M,
#elif MHZ(56) == DT_INST_0_NXP_KINETIS_SCG_FIRC_RANGE
	.range      = kSCG_FircRange56M,
#elif MHZ(60) == DT_INST_0_NXP_KINETIS_SCG_FIRC_RANGE
	.range      = kSCG_FircRange60M,
#else
#error Invalid SCG FIRC range
#endif
	.trimConfig = NULL
};

/* System Phase-Locked Loop (SPLL) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_SCG_SPLL_DIVIDER_1,
		       "Invalid SCG SPLL divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(DT_INST_0_NXP_KINETIS_SCG_SPLL_DIVIDER_2,
		       "Invalid SCG SPLL divider 2 value");
ASSERT_WITHIN_RANGE(DT_INST_0_NXP_KINETIS_SCG_SPLL_DIVIDER_PRE, 1, 8,
		    "Invalid SCG SPLL pre divider value");
ASSERT_WITHIN_RANGE(DT_INST_0_NXP_KINETIS_SCG_SPLL_MULTIPLIER, 16, 47,
		    "Invalid SCG SPLL multiplier value");
static const scg_spll_config_t scg_spll_config = {
	.enableMode  = kSCG_SysPllEnable,
	.monitorMode = kSCG_SysPllMonitorDisable,
	.div1        = TO_ASYNC_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_SPLL_DIVIDER_1),
	.div2        = TO_ASYNC_CLK_DIV(DT_INST_0_NXP_KINETIS_SCG_SPLL_DIVIDER_2),
#if DT_INST_0_NXP_KINETIS_SCG_SPLL_SOURCE == KINETIS_SCG_SPLL_SRC_SOSC
	.src         = kSCG_SysPllSrcSysOsc,
#elif DT_INST_0_NXP_KINETIS_SCG_SPLL_SOURCE == KINETIS_SCG_SPLL_SRC_FIRC
	.src         = kSCG_SysPllSrcFirc,
#else
#error Invalid SCG SPLL source
#endif
	.prediv      = (DT_INST_0_NXP_KINETIS_SCG_SPLL_DIVIDER_PRE - 1U),
	.mult        = (DT_INST_0_NXP_KINETIS_SCG_SPLL_MULTIPLIER - 16U)
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

#ifdef DT_INST_0_NXP_KINETIS_SCG_SOSC_FREQ
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

#ifdef DT_ALIAS_UART_0_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Lpuart0,
		       DT_ALIAS_UART_0_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_UART_1_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Lpuart1,
		       DT_ALIAS_UART_1_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_UART_2_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Lpuart2,
		       DT_ALIAS_UART_2_CLOCK_IP_SOURCE);
#endif
#ifdef DT_NXP_IMX_LPI2C_I2C_0_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Lpi2c0, DT_NXP_IMX_LPI2C_I2C_0_CLOCK_IP_SOURCE);
#endif
#ifdef DT_NXP_IMX_LPI2C_I2C_1_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Lpi2c1, DT_NXP_IMX_LPI2C_I2C_1_CLOCK_IP_SOURCE);
#endif
#ifdef DT_NXP_IMX_LPSPI_SPI_0_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Lpspi0, DT_NXP_IMX_LPSPI_SPI_0_CLOCK_IP_SOURCE);
#endif
#ifdef DT_NXP_IMX_LPSPI_SPI_1_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Lpspi1, DT_NXP_IMX_LPSPI_SPI_1_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_ADC_0_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Adc0, DT_ALIAS_ADC_0_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_ADC_1_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Adc1, DT_ALIAS_ADC_1_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_ADC_2_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Adc2, DT_ALIAS_ADC_2_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_PWM_0_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Ftm0, DT_ALIAS_PWM_0_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_PWM_1_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Ftm1, DT_ALIAS_PWM_1_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_PWM_2_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Ftm2, DT_ALIAS_PWM_2_CLOCK_IP_SOURCE);
#endif
#ifdef DT_ALIAS_PWM_3_CLOCK_IP_SOURCE
	CLOCK_SetIpSrc(kCLOCK_Ftm3, DT_ALIAS_PWM_3_CLOCK_IP_SOURCE);
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

void z_arm_watchdog_init(void)
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
#ifdef CONFIG_WDOG_ENABLE_AT_BOOT
	WDOG->TOVAL = CONFIG_WDOG_INITIAL_TIMEOUT >> 1;
	WDOG->CS = WDOG_CS_PRES(1) | WDOG_CS_CLK(1) | WDOG_CS_WAIT(1) |
		   WDOG_CS_EN(1) | WDOG_CS_UPDATE(1);
#else /* !CONFIG_WDOG_ENABLE_AT_BOOT */
	WDOG->TOVAL = 1024;
	WDOG->CS = WDOG_CS_EN(0) | WDOG_CS_UPDATE(1);
#endif /* !CONFIG_WDOG_ENABLE_AT_BOOT */

	while (!(WDOG->CS & WDOG_CS_RCS_MASK)) {
		;
	}
}

SYS_INIT(ke1xf_init, PRE_KERNEL_1, 0);
