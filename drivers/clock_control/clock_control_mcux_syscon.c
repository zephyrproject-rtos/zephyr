/*
 * Copyright (c) 2020-22, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_syscon
#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

static int mcux_lpc_syscon_clock_control_on(const struct device *dev,
			      clock_control_subsys_t sub_system)
{
#if defined(CONFIG_CAN_MCUX_MCAN)
	uint32_t clock_name = (uint32_t)sub_system;

	if (clock_name == MCUX_MCAN_CLK) {
		CLOCK_EnableClock(kCLOCK_Mcan);
	}
#endif /* defined(CONFIG_CAN_MCUX_MCAN) */

	return 0;
}

static int mcux_lpc_syscon_clock_control_off(const struct device *dev,
			       clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_lpc_syscon_clock_control_get_subsys_rate(
					const struct device *dev,
				    clock_control_subsys_t sub_system,
				    uint32_t *rate)
{
	uint32_t clock_name = (uint32_t) sub_system;

	switch (clock_name) {

#if defined(CONFIG_I2C_MCUX_FLEXCOMM) || \
		defined(CONFIG_SPI_MCUX_FLEXCOMM) || \
		defined(CONFIG_UART_MCUX_FLEXCOMM)
	case MCUX_FLEXCOMM0_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(0);
		break;
	case MCUX_FLEXCOMM1_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(1);
		break;
	case MCUX_FLEXCOMM2_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(2);
		break;
	case MCUX_FLEXCOMM3_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(3);
		break;
	case MCUX_FLEXCOMM4_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(4);
		break;
	case MCUX_FLEXCOMM5_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(5);
		break;
	case MCUX_FLEXCOMM6_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(6);
		break;
	case MCUX_FLEXCOMM7_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(7);
		break;
	case MCUX_FLEXCOMM8_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(8);
		break;
	case MCUX_FLEXCOMM9_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(9);
		break;
	case MCUX_FLEXCOMM10_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(10);
		break;
	case MCUX_FLEXCOMM11_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(11);
		break;
	case MCUX_FLEXCOMM12_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(12);
		break;
	case MCUX_FLEXCOMM13_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(13);
		break;
	case MCUX_PMIC_I2C_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(15);
		break;
	case MCUX_HS_SPI_CLK:
#if defined(SYSCON_HSLSPICLKSEL_SEL_MASK)
		*rate = CLOCK_GetHsLspiClkFreq();
#else
		*rate = CLOCK_GetFlexCommClkFreq(14);
#endif
	case MCUX_HS_SPI1_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(16);
		break;
#endif

#if (defined(FSL_FEATURE_SOC_USDHC_COUNT) && FSL_FEATURE_SOC_USDHC_COUNT)
	case MCUX_USDHC1_CLK:
		*rate = CLOCK_GetSdioClkFreq(0);
		break;
	case MCUX_USDHC2_CLK:
		*rate = CLOCK_GetSdioClkFreq(1);
		break;
#endif

#if defined(CONFIG_CAN_MCUX_MCAN)
	case MCUX_MCAN_CLK:
		*rate = CLOCK_GetMCanClkFreq();
		break;
#endif /* defined(CONFIG_CAN_MCUX_MCAN) */

#if defined(CONFIG_COUNTER_MCUX_CTIMER)
	case (MCUX_CTIMER0_CLK + MCUX_CTIMER_CLK_OFFSET):
		*rate = CLOCK_GetCTimerClkFreq(0);
		break;
	case (MCUX_CTIMER1_CLK + MCUX_CTIMER_CLK_OFFSET):
		*rate = CLOCK_GetCTimerClkFreq(1);
		break;
	case (MCUX_CTIMER2_CLK + MCUX_CTIMER_CLK_OFFSET):
		*rate = CLOCK_GetCTimerClkFreq(2);
		break;
	case (MCUX_CTIMER3_CLK + MCUX_CTIMER_CLK_OFFSET):
		*rate = CLOCK_GetCTimerClkFreq(3);
		break;
	case (MCUX_CTIMER4_CLK + MCUX_CTIMER_CLK_OFFSET):
		*rate = CLOCK_GetCTimerClkFreq(4);
		break;
#endif

	case MCUX_BUS_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_BusClk);
		break;
	}

	return 0;
}

static int mcux_lpc_syscon_clock_control_init(const struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api mcux_lpc_syscon_api = {
	.on = mcux_lpc_syscon_clock_control_on,
	.off = mcux_lpc_syscon_clock_control_off,
	.get_rate = mcux_lpc_syscon_clock_control_get_subsys_rate,
};

#define LPC_CLOCK_INIT(n) \
	\
DEVICE_DT_INST_DEFINE(n, \
		    &mcux_lpc_syscon_clock_control_init, \
		    NULL, \
		    NULL, NULL, \
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, \
		    &mcux_lpc_syscon_api);

DT_INST_FOREACH_STATUS_OKAY(LPC_CLOCK_INIT)
