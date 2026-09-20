/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * CPU frequency scaling P-state driver for the i.MX9 Cortex-A cores whose clock
 * Zephyr programs itself, for example, i.MX91 and i.MX93.
 *
 * A P-state maps to a core frequency, which is applied by setting the
 * IMX_CCM_ARM_PLL_CLK clock of the CCM driver. The frequencies that can be
 * requested are the operating points described by the ARM PLL devicetree node.
 *
 * Only the core frequency is changed. VDD_SOC is left at the level configured
 * by the boot loader, so every frequency described in devicetree must be usable
 * at that voltage.
 */

#include <errno.h>

#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/imx_ccm_rev2.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(imx9_cpu_freq_scale_ccm, CONFIG_CPU_FREQ_LOG_LEVEL);

#define CCM_NODE     DT_INST(0, nxp_imx_ccm_rev2)
#define ARM_PLL_NODE DT_NODELABEL(arm_pll)

/* SoC specific P-state configuration. */
struct arm_pll_pstate {
	uint32_t frequency;
};

/*
 * Counts the ARM PLL operating points that produce a frequency, so that a
 * P-state asking for a frequency the PLL cannot be programmed to fails to
 * build.
 */
#define ARM_PLL_OPP_MATCHES(opp_node, freq) || (DT_PROP(opp_node, clock_frequency) == (freq))

#define ARM_PLL_FREQ_SUPPORTED(freq)                                                               \
	(0 DT_FOREACH_CHILD_STATUS_OKAY_VARGS(ARM_PLL_NODE, ARM_PLL_OPP_MATCHES, freq))

#define DEFINE_ARM_PLL_PSTATE(node_id)                                                             \
	BUILD_ASSERT(ARM_PLL_FREQ_SUPPORTED(DT_PROP(node_id, clock_frequency)) != 0,               \
		     "cpu_freq: no ARM PLL operating point for this clock-frequency");             \
	static const struct arm_pll_pstate CONCAT(arm_pll_pstate_, node_id) = {                    \
		.frequency = DT_PROP(node_id, clock_frequency),                                    \
	};                                                                                         \
	PSTATE_DT_DEFINE(node_id, &CONCAT(arm_pll_pstate_, node_id))

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states), DEFINE_ARM_PLL_PSTATE)

int cpu_freq_pstate_set(const struct pstate *state)
{
	const struct device *ccm = DEVICE_DT_GET(CCM_NODE);
	const struct arm_pll_pstate *cfg;
	int ret;

	if (state == NULL) {
		LOG_ERR("P-state is NULL");
		return -EINVAL;
	}

	cfg = (const struct arm_pll_pstate *)state->config;
	if (cfg == NULL) {
		LOG_ERR("P-state vendor config is invalid");
		return -EINVAL;
	}

	if (!device_is_ready(ccm)) {
		LOG_ERR("CCM device is not ready");
		return -ENODEV;
	}

	LOG_DBG("Setting core clock to %u Hz (threshold=%u%%)", cfg->frequency,
		state->load_threshold);

	ret = clock_control_set_rate(ccm, UINT_TO_POINTER(IMX_CCM_ARM_PLL_CLK),
				     UINT_TO_POINTER(cfg->frequency));
	if (ret != 0) {
		LOG_ERR("Failed to set core clock to %u Hz: %d", cfg->frequency, ret);
	}

	return ret;
}
