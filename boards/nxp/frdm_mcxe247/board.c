/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <fsl_clock.h>
#include <soc.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ASSERT_WITHIN_RANGE(val, min, max, str) \
	BUILD_ASSERT(val >= min && val <= max, str)

#define ASSERT_ASYNC_CLK_DIV_VALID(val, str) \
	BUILD_ASSERT(val == 0 || val == 1 || val == 2 || val == 4 ||	\
		     val == 8 || val == 16 || val == 2 || val == 64, str)

#define kSCG_AsyncClkDivBy0 kSCG_AsyncClkDisable

#define TO_SYS_CLK_DIV(val) _DO_CONCAT(kSCG_SysClkDivBy, val)
#define TO_ASYNC_CLK_DIV(val) _DO_CONCAT(kSCG_AsyncClkDivBy, val)

#define SCG_CLOCK_NODE(name) DT_CHILD(DT_INST(0, nxp_kinetis_scg), name)
#define SCG_CLOCK_DIV(name) DT_PROP(SCG_CLOCK_NODE(name), clock_div)
#define SCG_CLOCK_MULT(name) DT_PROP(SCG_CLOCK_NODE(name), clock_mult)

/* System Clock configuration */
ASSERT_WITHIN_RANGE(SCG_CLOCK_DIV(slow_clk), 2, 8,
		    "Invalid SCG slow clock divider value");
ASSERT_WITHIN_RANGE(SCG_CLOCK_DIV(bus_clk), 1, 16,
		    "Invalid SCG bus clock divider value");
ASSERT_WITHIN_RANGE(SCG_CLOCK_DIV(core_clk), 1, 16,
		    "Invalid SCG core clock divider value");

/*******************************************************************************
 * Variables
 ******************************************************************************/

static const scg_sys_clk_config_t scg_sys_clk_config = {
	.divSlow = TO_SYS_CLK_DIV(SCG_CLOCK_DIV(slow_clk)),
	.divBus  = TO_SYS_CLK_DIV(SCG_CLOCK_DIV(bus_clk)),
	.divCore = TO_SYS_CLK_DIV(SCG_CLOCK_DIV(core_clk)),
#if DT_SAME_NODE(DT_CLOCKS_CTLR(SCG_CLOCK_NODE(core_clk)), SCG_CLOCK_NODE(sosc_clk))
	.src     = kSCG_SysClkSrcSysOsc,
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(SCG_CLOCK_NODE(core_clk)), SCG_CLOCK_NODE(sirc_clk))
	.src     = kSCG_SysClkSrcSirc,
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(SCG_CLOCK_NODE(core_clk)), SCG_CLOCK_NODE(firc_clk))
	.src     = kSCG_SysClkSrcFirc,
#elif DT_SAME_NODE(DT_CLOCKS_CTLR(SCG_CLOCK_NODE(core_clk)), SCG_CLOCK_NODE(spll_clk))
	.src     = kSCG_SysClkSrcSysPll,
#else
#error Invalid SCG core clock source
#endif
};

#if DT_NODE_HAS_STATUS_OKAY(SCG_CLOCK_NODE(sosc_clk))
/* System Oscillator (SOSC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(soscdiv1_clk),
		       "Invalid SCG SOSC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(soscdiv2_clk),
		       "Invalid SCG SOSC divider 2 value");
static const scg_sosc_config_t scg_sosc_config = {
	.freq        = DT_PROP(SCG_CLOCK_NODE(sosc_clk), clock_frequency),
	.monitorMode = kSCG_SysOscMonitorDisable,
	.enableMode  = kSCG_SysOscEnable,
	.div1        = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(soscdiv1_clk)),
	.div2        = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(soscdiv2_clk)),
	.workMode    = DT_PROP(DT_INST(0, nxp_kinetis_scg), sosc_mode)
};
#endif /* DT_NODE_HAS_STATUS_OKAY(SCG_CLOCK_NODE(sosc_clk)) */

/* Slow Internal Reference Clock (SIRC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(sircdiv1_clk),
		       "Invalid SCG SIRC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(sircdiv2_clk),
		       "Invalid SCG SIRC divider 2 value");
static const scg_sirc_config_t scg_sirc_config = {
	.enableMode = kSCG_SircEnable | kSCG_SircEnableInLowPower,
	.div1       = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(sircdiv1_clk)),
	.div2       = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(sircdiv2_clk)),
#if MHZ(8) == DT_PROP(SCG_CLOCK_NODE(sirc_clk), clock_frequency)
	.range      = kSCG_SircRangeHigh
#else
#error Invalid SCG SIRC clock frequency
#endif
};

/* Fast Internal Reference Clock (FIRC) configuration */
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(fircdiv1_clk),
		       "Invalid SCG FIRC divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(fircdiv2_clk),
		       "Invalid SCG FIRC divider 2 value");
static const scg_firc_config_t scg_firc_config = {
	.enableMode = kSCG_FircEnable,
	.div1       = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(fircdiv1_clk)),
	.div2       = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(fircdiv2_clk)),
#if MHZ(48) == DT_PROP(SCG_CLOCK_NODE(firc_clk), clock_frequency)
	.range      = kSCG_FircRange48M,
#else
#error Invalid SCG FIRC clock frequency
#endif
	.trimConfig = NULL
};

/* System Phase-Locked Loop (SPLL) configuration */
ASSERT_WITHIN_RANGE(SCG_CLOCK_DIV(spll_clk), 2, 2,
		    "Invalid SCG SPLL fixed divider value");
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(splldiv1_clk),
		    "Invalid SCG SPLL divider 1 value");
ASSERT_ASYNC_CLK_DIV_VALID(SCG_CLOCK_DIV(splldiv2_clk),
		    "Invalid SCG SPLL divider 2 value");
ASSERT_WITHIN_RANGE(SCG_CLOCK_DIV(pll), 1, 8,
		    "Invalid SCG PLL pre divider value");
ASSERT_WITHIN_RANGE(SCG_CLOCK_MULT(pll), 16, 47,
		    "Invalid SCG PLL multiplier value");
#if (DT_NODE_HAS_STATUS_OKAY(SCG_CLOCK_NODE(sosc_clk)) &&	\
		DT_NODE_HAS_STATUS_OKAY(SCG_CLOCK_NODE(spll_clk)))
static const scg_spll_config_t scg_spll_config = {
	.enableMode  = kSCG_SysPllEnable,
	.monitorMode = kSCG_SysPllMonitorDisable,
	.div1        = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(splldiv1_clk)),
	.div2        = TO_ASYNC_CLK_DIV(SCG_CLOCK_DIV(splldiv2_clk)),
#if !DT_SAME_NODE(DT_CLOCKS_CTLR(SCG_CLOCK_NODE(pll)), SCG_CLOCK_NODE(sosc_clk))
#error Invalid SCG PLL clock source
#endif
	.prediv      = (SCG_CLOCK_DIV(pll) - 1U),
	.mult        = (SCG_CLOCK_MULT(pll) - 16U)
};
#endif

__weak void clock_init(void)
{
	scg_sys_clk_config_t current;

	const scg_sys_clk_config_t scg_sys_clk_config_safe = {
		.divSlow = kSCG_SysClkDivBy4,
		.divBus  = kSCG_SysClkDivBy1,
		.divCore = kSCG_SysClkDivBy1,
		.src     = kSCG_SysClkSrcSirc
	};

#if DT_NODE_HAS_STATUS_OKAY(SCG_CLOCK_NODE(sosc_clk))
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

	#if (DT_NODE_HAS_STATUS_OKAY(SCG_CLOCK_NODE(sosc_clk)) &&	\
		 DT_NODE_HAS_STATUS_OKAY(SCG_CLOCK_NODE(spll_clk)))
	/* Configure System PLL only if system oscilator is initialized */
	/* as the oscillator is the only SPLL clock source.*/
	CLOCK_InitSysPll(&scg_spll_config);
	#endif

	/* Only RUN mode supported for now */
	CLOCK_SetRunModeSysClkConfig(&scg_sys_clk_config);
	do {
		CLOCK_GetCurSysClkConfig(&current);
	} while (current.src != scg_sys_clk_config.src);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0))
	CLOCK_SetIpSrc(kCLOCK_Lpuart0,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpuart0), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart1))
	CLOCK_SetIpSrc(kCLOCK_Lpuart1,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpuart1), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart2))
	CLOCK_SetIpSrc(kCLOCK_Lpuart2,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpuart2), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c0))
	CLOCK_SetIpSrc(kCLOCK_Lpi2c0,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpi2c0), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpi2c1))
	CLOCK_SetIpSrc(kCLOCK_Lpi2c1,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpi2c1), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpspi0))
	CLOCK_SetIpSrc(kCLOCK_Lpspi0,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpspi0), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpspi1))
	CLOCK_SetIpSrc(kCLOCK_Lpspi1,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpspi1), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpspi2))
	CLOCK_SetIpSrc(kCLOCK_Lpspi2,
		       DT_CLOCKS_CELL(DT_NODELABEL(lpspi2), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(adc0))
	CLOCK_SetIpSrc(kCLOCK_Adc0
		       DT_CLOCKS_CELL(DT_NODELABEL(adc0), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(adc1))
	CLOCK_SetIpSrc(kCLOCK_Adc1,
		       DT_CLOCKS_CELL(DT_NODELABEL(adc1), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ftm0))
	CLOCK_SetIpSrc(kCLOCK_Ftm0,
		       DT_CLOCKS_CELL(DT_NODELABEL(ftm0), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ftm1))
	CLOCK_SetIpSrc(kCLOCK_Ftm1,
		       DT_CLOCKS_CELL(DT_NODELABEL(ftm1), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ftm2))
	CLOCK_SetIpSrc(kCLOCK_Ftm2,
		       DT_CLOCKS_CELL(DT_NODELABEL(ftm2), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ftm3))
	CLOCK_SetIpSrc(kCLOCK_Ftm3,
		       DT_CLOCKS_CELL(DT_NODELABEL(ftm3), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ftm4))
	CLOCK_SetIpSrc(kCLOCK_Ftm4,
		       DT_CLOCKS_CELL(DT_NODELABEL(ftm4), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ftm5))
	CLOCK_SetIpSrc(kCLOCK_Ftm5,
		       DT_CLOCKS_CELL(DT_NODELABEL(ftm5), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ftm6))
	CLOCK_SetIpSrc(kCLOCK_Ftm6,
		       DT_CLOCKS_CELL(DT_NODELABEL(ftm6), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ftm7))
	CLOCK_SetIpSrc(kCLOCK_Ftm7,
		       DT_CLOCKS_CELL(DT_NODELABEL(ftm7), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ewm0))
	CLOCK_SetIpSrc(kCLOCK_Ewm0,
		       DT_CLOCKS_CELL(DT_NODELABEL(ewm0), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexio0))
	CLOCK_SetIpSrc(kCLOCK_Flexio0,
		       DT_CLOCKS_CELL(DT_NODELABEL(flexio0), ip_source));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(enet_ptp_clock))
	CLOCK_SetIpSrc(kCLOCK_Enet,
		       DT_CLOCKS_CELL(DT_NODELABEL(enet_ptp_clock), ip_source));
#endif
}

void board_early_init_hook(void)
{
#if !defined(CONFIG_ARM_MPU)
	uint32_t temp_reg;
#endif /* !CONFIG_ARM_MPU */

#if !defined(CONFIG_ARM_MPU)
	/*
	 * Disable memory protection and clear slave port errors.
	 * MCXE24x does not implement the optional ARMv7-M memory
	 * protection unit (MPU), specified by the architecture (PMSAv7), in the
	 * Cortex-M4 core.  Instead, the processor includes its own MPU module.
	 */
	temp_reg = SYSMPU->CESR;
	temp_reg &= ~SYSMPU_CESR_VLD_MASK;
	temp_reg |= SYSMPU_CESR_SPERR_MASK;
	SYSMPU->CESR = temp_reg;
#endif /* !CONFIG_ARM_MPU */

	clock_init();
}
