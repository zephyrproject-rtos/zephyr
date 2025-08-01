/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_opamp.h>
#include <zephyr/drivers/opamp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_opamp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_opamp

struct mcux_opamp_config {
	OPAMP_Type *base;
	opamp_positive_ref_voltage_t positive_ref_source;
	uint8_t mode;
	uint8_t functional_mode;
	bool enable_measure_reference;
	bool enable_measure_output;
};

static int mcux_opamp_enable(const struct device *dev)
{
	const struct mcux_opamp_config *config = dev->config;
	OPAMP_Type *base = config->base;

	base->OPAMP_CTR |= OPAMP_OPAMP_CTR_EN_MASK;

	return 0;
}

static int mcux_opamp_disable(const struct device *dev)
{
	const struct mcux_opamp_config *config = dev->config;
	OPAMP_Type *base = config->base;

	base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_EN_MASK;

	return 0;
}

static int mcux_opamp_set_gain(const struct device *dev, const struct opamp_gain_cfg *cfg)
{
	const struct mcux_opamp_config *config = dev->config;

	const uint8_t gain_map[] = {
		[1] = 1, [2] = 2, [4] = 3, [8] = 4, [16] = 5, [33] = 6, [64] = 7};

	if (cfg->inverting_gain < 1 || cfg->inverting_gain > 64 ||
	    gain_map[cfg->inverting_gain] == 0) {
		LOG_ERR("Invalid gain: %d", cfg->inverting_gain);
		return -EINVAL;
	}

	if (cfg->non_inverting_gain < 1 || cfg->non_inverting_gain > 64 ||
	    gain_map[cfg->non_inverting_gain] == 0) {
		LOG_ERR("Invalid non-inverting gain: %d", cfg->non_inverting_gain);
		return -EINVAL;
	}

	OPAMP_DoNegGainConfig(config->base, gain_map[cfg->inverting_gain]);
	OPAMP_DoPosGainConfig(config->base, gain_map[cfg->non_inverting_gain]);

	return 0;
}

static DEVICE_API(opamp, api) = {
	.enable = mcux_opamp_enable,
	.disable = mcux_opamp_disable,
	.set_gain = mcux_opamp_set_gain,
};

int mcux_opamp_init(const struct device *dev)
{
	const struct mcux_opamp_config *config = dev->config;
	OPAMP_Type *base = config->base;

	base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PREF_MASK) |
			   OPAMP_OPAMP_CTR_PREF(config->positive_ref_source));

	base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_MODE_MASK) |
			   OPAMP_OPAMP_CTR_MODE(config->mode));

#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_OUTSW) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_OUTSW
	base->OPAMP_CTR |= OPAMP_OPAMP_CTR_OUTSW_MASK;
#endif /* FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_OUTSW */

#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN
	base->OPAMP_CTR |= OPAMP_OPAMP_CTR_BUFEN_MASK;
#endif /* FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN */

#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW1) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW1
	if (config->enable_measure_output) {
		base->OPAMP_CTR |= OPAMP_OPAMP_CTR_ADCSW1_MASK;
	}
#endif /* FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW1 */

#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW2) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW2
	if (config->enable_measure_reference) {
		base->OPAMP_CTR |= OPAMP_OPAMP_CTR_ADCSW2_MASK;
	}
#else
	if (config->enable_measure_reference) {
		base->OPAMP_CTR |= OPAMP_OPAMP_CTR_ADCSW_MASK;
	}
#endif /* FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW2 */

	switch (config->functional_mode) {
	case OPAMP_PGA_MODE:           /* No more software configuration. */
	case OPAMP_INVERTING_MODE:     /* No more software configuration. */
	case OPAMP_NON_INVERTING_MODE: /* No more software configuration. */
		break;
	case OPAMP_FOLLOWER_MODE:
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN
		base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_BUFEN_MASK;
#else
		base->OPAMP_CTR = ((base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PREF_MASK) |
				   OPAMP_OPAMP_CTR_PREF(kOPAMP_PosRefVoltVrefh4));
#endif /*  */
		break;
	default:
		LOG_ERR("Unsupported functional mode %d", config->functional_mode);
		return -ENOTSUP;
	}

	return 0;
}

#define OPAMP_MCUX_OPAMP_DEFINE(inst)                                                              \
	static const struct mcux_opamp_config _CONCAT(config, inst) = {                            \
		.base = (OPAMP_Type *)DT_INST_REG_ADDR(inst),                                      \
		.positive_ref_source = DT_INST_ENUM_IDX(inst, non_inverting_reference_source),     \
		.mode = DT_INST_ENUM_IDX(inst, mode),                                              \
		.functional_mode = DT_INST_ENUM_IDX(inst, functional_mode),                        \
		.enable_measure_reference = DT_INST_PROP(inst, enable_measure_reference),          \
		.enable_measure_output = DT_INST_PROP(inst, enable_measure_output),                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mcux_opamp_init, NULL, NULL, &_CONCAT(config, inst),           \
			      POST_KERNEL, CONFIG_OPAMP_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OPAMP_MCUX_OPAMP_DEFINE)
