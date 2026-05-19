/*
 * Copyright 2017, 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_sim
#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/kinetis_sim.h>
#include <fsl_clock.h>

static inline uint32_t mcux_sim_subsys_id(clock_control_subsys_t sub_system)
{
	return (uint32_t)(uintptr_t)sub_system;
}

#if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nxp_kinetis_ke1xf_sim))
#define NXP_KINETIS_SIM_NODE DT_INST(0, nxp_kinetis_ke1xf_sim)
#else
#define NXP_KINETIS_SIM_NODE DT_INST(0, nxp_kinetis_sim)
#endif

static int mcux_sim_validate_gate_offset(uint32_t gate_offset)
{
	if (gate_offset == 0U) {
		return 0;
	}

	if (((gate_offset & 0x3U) != 0U) ||
	    (gate_offset >= DT_REG_SIZE(NXP_KINETIS_SIM_NODE))) {
		return -EINVAL;
	}

	return 0;
}

static int mcux_sim_on(const struct device *dev,
		       clock_control_subsys_t sub_system)
{
	uint32_t clk = mcux_sim_subsys_id(sub_system);
	uint32_t gate_offset = KINETIS_SIM_CLOCK_DECODE_GATE_OFFSET(clk);
	uint32_t gate_bit = KINETIS_SIM_CLOCK_DECODE_GATE_BIT(clk);

	int ret;

	ARG_UNUSED(dev);

	ret = mcux_sim_validate_gate_offset(gate_offset);
	if (ret) {
		return ret;
	}

	if (gate_offset == 0U) {
		return 0;
	}

	CLOCK_EnableClock((clock_ip_name_t)CLK_GATE_DEFINE(gate_offset, gate_bit));
	return 0;
}

static int mcux_sim_off(const struct device *dev,
			clock_control_subsys_t sub_system)
{
	uint32_t clk = mcux_sim_subsys_id(sub_system);
	uint32_t gate_offset = KINETIS_SIM_CLOCK_DECODE_GATE_OFFSET(clk);
	uint32_t gate_bit = KINETIS_SIM_CLOCK_DECODE_GATE_BIT(clk);

	int ret;

	ARG_UNUSED(dev);

	ret = mcux_sim_validate_gate_offset(gate_offset);
	if (ret) {
		return ret;
	}

	if (gate_offset == 0U) {
		return 0;
	}

	CLOCK_DisableClock((clock_ip_name_t)CLK_GATE_DEFINE(gate_offset, gate_bit));
	return 0;
}

static int mcux_sim_get_subsys_rate(const struct device *dev,
				    clock_control_subsys_t sub_system,
				    uint32_t *rate)
{
	uint32_t clk = mcux_sim_subsys_id(sub_system);
	uint32_t clock_name = KINETIS_SIM_CLOCK_DECODE_NAME(clk);

	ARG_UNUSED(dev);

	if (clock_name == KINETIS_SIM_LPO_CLK) {
		*rate = CLOCK_GetFreq(kCLOCK_LpoClk);
	} else {
		*rate = CLOCK_GetFreq((clock_name_t)clock_name);
	}

	return 0;
}


#if DT_NODE_HAS_PROP(NXP_KINETIS_SIM_NODE, clkout_source)
	#define NXP_KINETIS_SIM_CLKOUT_SOURCE \
		DT_PROP(NXP_KINETIS_SIM_NODE, clkout_source)
#endif
#if DT_NODE_HAS_PROP(NXP_KINETIS_SIM_NODE, clkout_divider)
	#define NXP_KINETIS_SIM_CLKOUT_DIVIDER \
		DT_PROP(NXP_KINETIS_SIM_NODE, clkout_divider)
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
