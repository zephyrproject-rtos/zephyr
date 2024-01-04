/*
 * Copyright (c) 2020-23, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_syscon
#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_utils.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <soc.h>
#include <fsl_clock.h>
#if defined(CONFIG_SOC_SERIES_LPC55XXX)
#include <clock_control_soc.h>
#else
/* Provide dummy definitions for SOC clock setpoint macros */
#define CLOCK_CONTROL_SETPOINT_DEFINE(node)
#define CLOCK_CONTROL_SETPOINT_GET(node) NULL
#endif

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

struct mcux_lpc_syscon_setpoint {
	/* Function to call to apply the setpoint */
	int (*setpoint)(void);
	uint8_t id;
};

struct mcux_lpc_syscon_config {
	struct mcux_lpc_syscon_setpoint *setpoints;
	uint32_t setpoint_count;
};

struct mcux_lpc_syscon_data {
	sys_slist_t callbacks;
};

static int mcux_lpc_syscon_clock_control_on(const struct device *dev,
			      clock_control_subsys_t sub_system)
{
#if defined(CONFIG_CAN_MCUX_MCAN)
	if ((uint32_t)sub_system == MCUX_MCAN_CLK) {
		CLOCK_EnableClock(kCLOCK_Mcan);
	}
#endif /* defined(CONFIG_CAN_MCUX_MCAN) */
#if defined(CONFIG_COUNTER_NXP_MRT)
	if ((uint32_t)sub_system == MCUX_MRT_CLK) {
#if defined(CONFIG_SOC_FAMILY_LPC)
		CLOCK_EnableClock(kCLOCK_Mrt);
#elif defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
		CLOCK_EnableClock(kCLOCK_Mrt0);
#endif
	}
#endif /* defined(CONFIG_COUNTER_NXP_MRT) */

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
		break;
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

#if (defined(FSL_FEATURE_SOC_SDIF_COUNT) && \
	(FSL_FEATURE_SOC_SDIF_COUNT)) && \
	CONFIG_MCUX_SDIF
	case MCUX_SDIF_CLK:
		*rate = CLOCK_GetSdioClkFreq();
		break;
#endif

#if defined(CONFIG_CAN_MCUX_MCAN)
	case MCUX_MCAN_CLK:
		*rate = CLOCK_GetMCanClkFreq();
		break;
#endif /* defined(CONFIG_CAN_MCUX_MCAN) */

#if defined(CONFIG_COUNTER_MCUX_CTIMER) || defined(CONFIG_PWM_MCUX_CTIMER)
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

#if defined(CONFIG_COUNTER_NXP_MRT)
	case MCUX_MRT_CLK:
#endif
#if defined(CONFIG_PWM_MCUX_SCTIMER)
	case MCUX_SCTIMER_CLK:
#endif
	case MCUX_BUS_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_BusClk);
		break;

#if defined(CONFIG_I3C_MCUX)
	case MCUX_I3C_CLK:
		*rate = CLOCK_GetI3cClkFreq();
		break;
#endif

#if defined(CONFIG_MIPI_DSI_MCUX_2L)
	case MCUX_MIPI_DSI_DPHY_CLK:
		*rate = CLOCK_GetMipiDphyClkFreq();
		break;
	case MCUX_MIPI_DSI_ESC_CLK:
		*rate = CLOCK_GetMipiDphyEscTxClkFreq();
		break;
	case MCUX_LCDIF_PIXEL_CLK:
		*rate = CLOCK_GetDcPixelClkFreq();
		break;
#endif
#if defined(CONFIG_AUDIO_DMIC_MCUX)
	case MCUX_DMIC_CLK:
		*rate = CLOCK_GetDmicClkFreq();
		break;
#endif
	case MCUX_SYSTEM_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_CoreSysClk);
		break;
	}

	return 0;
}

static int mcux_lpc_syscon_clock_control_select_setpoint(const struct device *dev,
							 uint32_t setpoint_id)
{
	const struct mcux_lpc_syscon_config *config = dev->config;
	struct mcux_lpc_syscon_data *data = dev->data;
	int ret;

	if (setpoint_id >= config->setpoint_count) {
		return -EINVAL;
	}
	for (uint8_t i = 0; i < config->setpoint_count; i++) {
		if (config->setpoints[i].id == setpoint_id) {
			ret = config->setpoints[i].setpoint();
			if (ret < 0) {
				return ret;
			}
			clock_control_fire_callbacks(&data->callbacks, dev);
			return ret;
		}
	}
	/* Setpoint was not found */
	return -ENOENT;
}

static int mcux_lpc_syscon_clock_control_add_cb(const struct device *dev,
						struct clock_control_callback *cb)
{
	struct mcux_lpc_syscon_data *data = dev->data;

	return clock_control_manage_callback(&data->callbacks, cb, true);
}

static int mcux_lpc_syscon_clock_control_remove_cb(const struct device *dev,
						   struct clock_control_callback *cb)
{
	struct mcux_lpc_syscon_data *data = dev->data;

	return clock_control_manage_callback(&data->callbacks, cb, false);
}


static const struct clock_control_driver_api mcux_lpc_syscon_api = {
	.on = mcux_lpc_syscon_clock_control_on,
	.off = mcux_lpc_syscon_clock_control_off,
	.get_rate = mcux_lpc_syscon_clock_control_get_subsys_rate,
	.select_setpoint = mcux_lpc_syscon_clock_control_select_setpoint,
	.add_callback = mcux_lpc_syscon_clock_control_add_cb,
	.remove_callback = mcux_lpc_syscon_clock_control_remove_cb,
};

#define LPC_CLOCK_DEFINE(node, prop, idx)	\
	CLOCK_CONTROL_SETPOINT_DEFINE(DT_PHANDLE_BY_IDX(node, prop, idx))

#define LPC_CLOCK_GET(node, prop, idx)		\
{									\
	.setpoint = CLOCK_CONTROL_SETPOINT_GET(DT_PHANDLE_BY_IDX(node, prop, idx)), \
	.id = UTIL_CAT(CLOCK_SETPOINT_, DT_STRING_UPPER_TOKEN_BY_IDX(node, setpoint_names, idx)) \
},

#define LPC_CLOCK_INIT(n) \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, setpoints),				\
	(DT_INST_FOREACH_PROP_ELEM(n, setpoints, LPC_CLOCK_DEFINE)))		\
	struct mcux_lpc_syscon_setpoint lpc_syscon_setpoints_##n[] = {		\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, setpoints),			\
		(DT_INST_FOREACH_PROP_ELEM(n, setpoints, LPC_CLOCK_GET)))	\
	};									\
	struct mcux_lpc_syscon_config lpc_syscon_config_##n = {			\
		.setpoints = lpc_syscon_setpoints_##n,				\
		.setpoint_count = DT_INST_PROP_LEN_OR(n, setpoints, 0),		\
	};									\
	struct mcux_lpc_syscon_data lpc_syscon_data_##n;			\
										\
	DEVICE_DT_INST_DEFINE(n, \
		    NULL, \
		    NULL, \
		    &lpc_syscon_data_##n, &lpc_syscon_config_##n, \
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, \
		    &mcux_lpc_syscon_api);

DT_INST_FOREACH_STATUS_OKAY(LPC_CLOCK_INIT)
