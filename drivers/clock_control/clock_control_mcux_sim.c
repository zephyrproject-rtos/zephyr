/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_sim
#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/kinetis_sim.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

struct kinetis_sim_spec {
	uint32_t name;
	uint32_t offset;
	uint32_t bits;
};

static int kinetis_sim_get_spec(clock_control_subsys_t subsys, struct kinetis_sim_spec *spec)
{
	struct clock_control_dt_spec *dt_spec = subsys;

	if (dt_spec->len != 3) {
		return -EINVAL;
	}

	spec->name = dt_spec->cells[0];
	spec->offset = dt_spec->cells[1];
	spec->bits = dt_spec->cells[2];

	return 0;
}

static int kinetis_sim_get_key(clock_control_subsys_t subsys, uint32_t *key)
{
	struct kinetis_sim_spec spec;
	int ret;

	ret = kinetis_sim_get_spec(subsys, &spec);
	if (ret) {
		return ret;
	}

	*key = CLK_GATE_DEFINE(spec.offset, spec.bits);

	return 0;
}

static int mcux_sim_on(const struct device *dev,
		       clock_control_subsys_t sub_system)
{
	clock_ip_name_t clock_ip_name = (clock_ip_name_t) sub_system;

#ifdef CONFIG_ETH_NXP_ENET
	if ((uint32_t)sub_system == KINETIS_SIM_ENET_CLK) {
		clock_ip_name = kCLOCK_Enet0;
	}
#endif

	CLOCK_EnableClock(clock_ip_name);

	return 0;
}

static int mcux_sim_off(const struct device *dev,
			clock_control_subsys_t sub_system)
{
	clock_ip_name_t clock_ip_name = (clock_ip_name_t) sub_system;

	CLOCK_DisableClock(clock_ip_name);

	return 0;
}

static int mcux_sim_get_subsys_rate(const struct device *dev,
				    clock_control_subsys_t sub_system,
				    uint32_t *rate)
{
	clock_name_t clock_name;

	switch ((uint32_t) sub_system) {
	case KINETIS_SIM_LPO_CLK:
		clock_name = kCLOCK_LpoClk;
		break;
	case KINETIS_SIM_ENET_CLK:
		clock_name = kCLOCK_CoreSysClk;
		break;
	case KINETIS_SIM_ENET_1588_CLK:
		clock_name = kCLOCK_Osc0ErClk;
		break;
	default:
		clock_name = (clock_name_t) sub_system;
		break;
	}

	*rate = CLOCK_GetFreq(clock_name);

	return 0;
}

#if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_kinetis_ke1xf_sim))
#define NXP_KINETIS_SIM_NODE DT_INST(0, nxp_kinetis_ke1xf_sim)
#if DT_NODE_HAS_PROP(DT_INST(0, nxp_kinetis_ke1xf_sim), clkout_source)
	#define NXP_KINETIS_SIM_CLKOUT_SOURCE \
			DT_PROP(DT_INST(0, nxp_kinetis_ke1xf_sim), clkout_source)
#endif
#if DT_NODE_HAS_PROP(DT_INST(0, nxp_kinetis_ke1xf_sim), clkout_divider)
	#define NXP_KINETIS_SIM_CLKOUT_DIVIDER \
			DT_PROP(DT_INST(0, nxp_kinetis_ke1xf_sim), clkout_divider)
#endif
#else
#define NXP_KINETIS_SIM_NODE DT_INST(0, nxp_kinetis_sim)
#if DT_NODE_HAS_PROP(DT_INST(0, nxp_kinetis_sim), clkout_source)
	#define NXP_KINETIS_SIM_CLKOUT_SOURCE \
		DT_PROP(DT_INST(0, nxp_kinetis_sim), clkout_source)
#endif
#if DT_NODE_HAS_PROP(DT_INST(0, nxp_kinetis_sim), clkout_divider)
	#define NXP_KINETIS_SIM_CLKOUT_DIVIDER \
		DT_PROP(DT_INST(0, nxp_kinetis_sim), clkout_divider)
#endif
#endif

static int mcux_sim_init(const struct device *dev)
{
#ifdef NXP_KINETIS_SIM_CLKOUT_DIVIDER
	SIM->CHIPCTL = (SIM->CHIPCTL & ~SIM_CHIPCTL_CLKOUTDIV_MASK)
		| SIM_CHIPCTL_CLKOUTDIV(NXP_KINETIS_SIM_CLKOUT_DIVIDER);
#endif
#ifdef NXP_KINETIS_SIM_CLKOUT_SOURCE
	SIM->CHIPCTL = (SIM->CHIPCTL & ~SIM_CHIPCTL_CLKOUTSEL_MASK)
		| SIM_CHIPCTL_CLKOUTSEL(NXP_KINETIS_SIM_CLKOUT_SOURCE);
#endif

	return 0;
}

static DEVICE_API(clock_control, mcux_sim_driver_api) = {
	.on = mcux_sim_on,
	.off = mcux_sim_off,
	.get_rate = mcux_sim_get_subsys_rate,
};

DEVICE_DT_DEFINE(NXP_KINETIS_SIM_NODE,
		    mcux_sim_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &mcux_sim_driver_api);
