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
	uint8_t operation_mode;
	uint8_t functional_mode;
	bool enable_measure_reference;
	bool enable_non_inverting_reference;
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

static int mcux_opamp_set_gain(const struct device *dev, enum opamp_gain gain)
{
	const struct mcux_opamp_config *config = dev->config;
	uint8_t gain_index;

	switch (gain) {
	case OPAMP_GAIN_1:
		gain_index = 1;
		break;
	case OPAMP_GAIN_2:
		gain_index = 2;
		break;
	case OPAMP_GAIN_4:
		gain_index = 3;
		break;
	case OPAMP_GAIN_8:
		gain_index = 4;
		break;
	case OPAMP_GAIN_16:
		gain_index = 5;
		break;
	case OPAMP_GAIN_33:
		gain_index = 6;
		break;
	case OPAMP_GAIN_64:
		gain_index = 7;
		break;
	default:
		LOG_ERR("Invalid gain value: %d", gain);
		return -EINVAL;
	}

	switch (config->functional_mode) {
	case OPAMP_DIFFERENTIAL_MODE:
	case OPAMP_INVERTING_MODE:
	case OPAMP_NON_INVERTING_MODE:
		/* For differential, inverting and non-inverting mode,
		 * set Ngain to select different gains, and set Pgain
		 * to be the same as Ngain.
		 */
		OPAMP_DoNegGainConfig(config->base, gain_index);
		OPAMP_DoPosGainConfig(config->base, gain_index);
		break;
	case OPAMP_FOLLOWER_MODE:
		OPAMP_DoNegGainConfig(config->base, kOPAMP_NegGainBufferMode);
		OPAMP_DoPosGainConfig(config->base, kOPAMP_PosGainNonInvert1X);
	default:
		break;
	}

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

	base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_MODE_MASK) |
			   OPAMP_OPAMP_CTR_MODE(config->operation_mode));

	if (config->enable_measure_reference) {
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW2) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW2
		base->OPAMP_CTR |= OPAMP_OPAMP_CTR_ADCSW2_MASK;
#else
		base->OPAMP_CTR |= OPAMP_OPAMP_CTR_ADCSW_MASK;
#endif
	} else {
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW2) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW2
		base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_ADCSW2_MASK;
#else
		base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_ADCSW_MASK;
#endif
	}

#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW1) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_ADCSW1
	if (config->enable_measure_output) {
		base->OPAMP_CTR |= OPAMP_OPAMP_CTR_ADCSW1_MASK;
	} else {
		base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_ADCSW1_MASK;
	}
#endif

	if (config->enable_non_inverting_reference) {
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN
		base->OPAMP_CTR |= OPAMP_OPAMP_CTR_BUFEN_MASK;
#endif
		base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PREF_MASK) |
						OPAMP_OPAMP_CTR_PREF(config->positive_ref_source));
	} else {
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN
		base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_BUFEN_MASK;
#else
		base->OPAMP_CTR = ((base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PREF_MASK) |
						OPAMP_OPAMP_CTR_PREF(kOPAMP_PosRefVoltVrefh4));
#endif
	}

#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_OUTSW) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_OUTSW
	base->OPAMP_CTR |= OPAMP_OPAMP_CTR_OUTSW_MASK;
#endif

	switch (config->functional_mode) {
	case OPAMP_DIFFERENTIAL_MODE:
	case OPAMP_INVERTING_MODE:
	case OPAMP_NON_INVERTING_MODE:
		break;
	case OPAMP_FOLLOWER_MODE:
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN
		base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_BUFEN_MASK;
#else
		base->OPAMP_CTR = ((base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PREF_MASK) |
				   OPAMP_OPAMP_CTR_PREF(kOPAMP_PosRefVoltVrefh4));
#endif
		break;
	default:
		LOG_ERR("Unsupported functional mode %d", config->functional_mode);
		return -ENOTSUP;
	}

	return 0;
}

#define OPAMP_MCUX_OPAMP_DEFINE(inst)						\
	static const struct mcux_opamp_config _CONCAT(config, inst) = {		\
		.base = (OPAMP_Type *)DT_INST_REG_ADDR(inst),			\
		.positive_ref_source = DT_INST_ENUM_IDX(inst,			\
				non_inverting_reference_source),		\
		.operation_mode = DT_INST_ENUM_IDX(inst, operation_mode),	\
		.functional_mode = DT_INST_ENUM_IDX(inst, functional_mode),	\
		.enable_measure_reference = DT_INST_PROP(inst,			\
				enable_measure_reference),			\
		.enable_measure_output = DT_INST_PROP(inst,			\
				enable_measure_output),				\
		.enable_non_inverting_reference = DT_INST_PROP(inst,		\
				enable_non_inverting_reference),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, mcux_opamp_init, NULL,			\
			NULL, &_CONCAT(config, inst), POST_KERNEL,		\
			CONFIG_OPAMP_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OPAMP_MCUX_OPAMP_DEFINE)
