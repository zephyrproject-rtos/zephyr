/*
 * Copyright 2025 CodeWrights GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_vrefbuf

#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>
#include <stm32_ll_system.h>

LOG_MODULE_REGISTER(regulator_stm32_vrefbuf, CONFIG_REGULATOR_LOG_LEVEL);

/*
 * Do we need to compile support code for VREFBUF without active scale change?
 * For now, support only configurations where all instances either have it or not.
 * This can be changed in the future if necessary by saving the property's value
 * in the device config and replacing compile-time with runtime checks.
 *
 * Note: NASC = Non-Active Scale Change
 */
#if DT_ALL_INST_HAS_BOOL_STATUS_OKAY(st_has_active_scale_change)
#define NEEDS_NASC_SUPPORT 0
#else /* DT_ALL_INST_HAS_BOOL_STATUS_OKAY(st_has_active_scale_change) */
BUILD_ASSERT(UTIL_NOT(DT_ANY_INST_HAS_BOOL_STATUS_OKAY(st_has_active_scale_change)),
	     "All instances must have the same value for property st,has-active-scale-change");
#define NEEDS_NASC_SUPPORT 1
#endif /* DT_ALL_INST_HAS_BOOL_STATUS_OKAY(st_has_active_scale_change) */

struct regulator_stm32_vrefbuf_voltage {
	uint32_t vrs; /* LL_VREFBUF_VOLTAGE_SCALE<X> */
	int32_t uv;   /* VREFBUF output in uV with this scale */
};

struct regulator_stm32_vrefbuf_data {
	struct regulator_common_data common;
};

struct regulator_stm32_vrefbuf_config {
	struct regulator_common_config common;
	const struct stm32_pclken pclken[1];
	const struct reset_dt_spec reset;
	const struct regulator_stm32_vrefbuf_voltage *ref_voltages;
	bool vrefp_output_enable;
	uint8_t ref_voltage_count;
};

static int regulator_stm32_vrefbuf_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	LL_VREFBUF_Enable();

	return 0;
}

static int regulator_stm32_vrefbuf_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	LL_VREFBUF_Disable();

	return 0;
}

static int regulator_stm32_vrefbuf_list_voltage(const struct device *dev, unsigned int idx,
						int32_t *volt_uv)
{
	const struct regulator_stm32_vrefbuf_config *config = dev->config;

	if (idx >= config->ref_voltage_count) {
		return -EINVAL;
	}

	*volt_uv = config->ref_voltages[idx].uv;

	return 0;
}

static unsigned int regulator_stm32_vrefbuf_count_voltages(const struct device *dev)
{
	const struct regulator_stm32_vrefbuf_config *config = dev->config;

	return config->ref_voltage_count;
}

static int regulator_stm32_vrefbuf_set_voltage(const struct device *dev, int32_t min_uv,
					       int32_t max_uv)
{
	const struct regulator_stm32_vrefbuf_config *config = dev->config;
	uint32_t best_vrs = 0;
	int32_t best_voltage = INT32_MAX;
	bool regulator_enabled;
	int res = -EINVAL;

	if (NEEDS_NASC_SUPPORT) {
		/*
		 * On some series, the regulator must be disabled to change voltage scale.
		 *
		 * Enable Hi-Z to switch the regulator in "Hold mode" if the VREF+ output
		 * is enabled to maintain VREF+ pin voltage while we temporarily disable
		 * the regulator. Otherwise, the output isn't used (Hi-Z already enabled)
		 * so disabling the regulator doesn't really matter...
		 *
		 * Note that we perform the operations unconditionally to reduce code size
		 * because they are idempotent (no effect if already in the desired state).
		 * Also, there is no LL function to check if the regulator is enabled so
		 * we have to do it using a raw register access.
		 */
		regulator_enabled = (VREFBUF->CSR & VREFBUF_CSR_ENVR) != 0U;
		LL_VREFBUF_EnableHIZ();
		LL_VREFBUF_Disable();
	}

	/* Search among supported voltages for the closest one at least equal
	 * to minimum requested by caller. Note that all configurations must
	 * be checked as the range order is not consistent across series
	 * (i.e., whether VREFBUF0 < VREFBUF1 or VREFBUF0 > VREFBUF1).
	 */
	for (uint8_t i = 0; i < config->ref_voltage_count; i++) {
		if (IN_RANGE(config->ref_voltages[i].uv, min_uv, max_uv) &&
		    config->ref_voltages[i].uv < best_voltage) {
			best_voltage = config->ref_voltages[i].uv;
			best_vrs = config->ref_voltages[i].vrs;
		}
	}

	if (best_voltage != INT32_MAX) {
		LL_VREFBUF_SetVoltageScaling(best_vrs);
		res = 0;
	}

	if (NEEDS_NASC_SUPPORT) {
		/*
		 * Regardless of whether we changed scale,
		 * undo changes we have done in the prologue.
		 */
		if (config->vrefp_output_enable) {
			LL_VREFBUF_DisableHIZ();
		}
		if (regulator_enabled) {
			LL_VREFBUF_Enable();
		}
	}

	return res;
}

static int regulator_stm32_vrefbuf_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_stm32_vrefbuf_config *config = dev->config;

	uint32_t vrs = LL_VREFBUF_GetVoltageScaling();

	for (uint8_t i = 0; i < config->ref_voltage_count; i++) {
		if (config->ref_voltages[i].vrs == vrs) {
			*volt_uv = config->ref_voltages[i].uv;
			return 0;
		}
	}

	return -EIO;
}

static int regulator_stm32_vrefbuf_init(const struct device *dev)
{
	const struct regulator_stm32_vrefbuf_config *config = dev->config;
	int res;

	regulator_common_data_init(dev);

	if (config->reset.dev != NULL) {
		const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

		res = clock_control_on(clk, (clock_control_subsys_t)&config->pclken[0]);
		if (res != 0) {
			LOG_ERR("Could not enable clock: %d", res);
			return res;
		}

		if (!device_is_ready(config->reset.dev)) {
			LOG_ERR("Reset controller not ready");
			return -ENODEV;
		}

		res = reset_line_deassert_dt(&config->reset);
		if (res != 0) {
			LOG_ERR("Could not deassert reset line: %d", res);
			return res;
		}
	}

	if (config->vrefp_output_enable) {
		LL_VREFBUF_DisableHIZ();
	} else {
		LL_VREFBUF_EnableHIZ();
	}

	return regulator_common_init(dev, false);
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_stm32_vrefbuf_enable,
	.disable = regulator_stm32_vrefbuf_disable,
	.count_voltages = regulator_stm32_vrefbuf_count_voltages,
	.list_voltage = regulator_stm32_vrefbuf_list_voltage,
	.set_voltage = regulator_stm32_vrefbuf_set_voltage,
	.get_voltage = regulator_stm32_vrefbuf_get_voltage,
};

#define VREFBUF_VOLTAGE_ELEM(node_id, prop, idx)                                                   \
	{                                                                                          \
		.vrs = CONCAT(LL_VREFBUF_VOLTAGE_SCALE, idx),                                      \
		.uv = DT_PROP_BY_IDX(node_id, prop, idx)                                           \
	},

#define REGULATOR_STM32_VREFBUF_DEFINE(inst)                                                       \
                                                                                                   \
	static struct regulator_stm32_vrefbuf_data data_##inst;                                    \
	static const struct regulator_stm32_vrefbuf_voltage ref_voltages_##inst[] = {              \
		DT_FOREACH_PROP_ELEM(DT_DRV_INST(inst), ref_voltages, VREFBUF_VOLTAGE_ELEM)        \
	};                                                                                         \
                                                                                                   \
	static const struct regulator_stm32_vrefbuf_config config_##inst = {                       \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst),                              \
		.pclken = STM32_DT_INST_CLOCKS(inst),                                              \
		.reset = RESET_DT_SPEC_INST_GET_OR(inst, {0}),                                     \
		.vrefp_output_enable = DT_INST_PROP(inst, vrefp_output_enable),                    \
		.ref_voltages = ref_voltages_##inst,                                               \
		.ref_voltage_count = ARRAY_SIZE(ref_voltages_##inst),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_stm32_vrefbuf_init, NULL, &data_##inst,              \
			      &config_##inst, POST_KERNEL,                                         \
			      CONFIG_REGULATOR_STM32_VREFBUF_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_STM32_VREFBUF_DEFINE)
