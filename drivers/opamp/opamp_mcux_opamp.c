/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/pm/device.h>
#if defined(CONFIG_SOC_FAMILY_LPC)
#include <zephyr/drivers/reset.h>
#endif
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_opamp, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_opamp

struct mcux_opamp_config {
	OPAMP_Type *base;
	uint8_t positive_reference;
	uint8_t operation_mode;
	uint8_t functional_mode;
	bool enable_measure_reference;
	bool enable_measure_output;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;
#if defined(CONFIG_SOC_FAMILY_LPC)
	const struct reset_dt_spec reset;
#endif
};

/*
 * OPAMP_CTR[NGAIN]/[PGAIN] encodings used by the follower default, from the
 * same field tables as the gain index switch in mcux_opamp_set_gain().
 */
enum {
	OPAMP_CTR_NGAIN_BUFFER_MODE = 0,
	OPAMP_CTR_PGAIN_NON_INVERT_1X = 1,
};

/* OPAMP_CTR[PREF] encoding used when no non-inverting reference is configured. */
#define OPAMP_CTR_PREF_VREFH4 3U

/*
 * The core keeps a device whose suspend failed in PM_DEVICE_STATE_ACTIVE, and
 * device runtime PM restores the usage count it had before the suspend, so a
 * suspend that cannot finish leaves the output driven as it found it.
 */
static void mcux_opamp_suspend_rollback(const struct device *dev)
{
	const struct mcux_opamp_config *config = dev->config;

	(void)pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	config->base->OPAMP_CTR |= OPAMP_OPAMP_CTR_EN_MASK;
}

static int mcux_opamp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct mcux_opamp_config *config = dev->config;
	int err;

	if (action == PM_DEVICE_ACTION_RESUME) {
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}

		config->base->OPAMP_CTR |= OPAMP_OPAMP_CTR_EN_MASK;
		return 0;
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		config->base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_EN_MASK;

		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			mcux_opamp_suspend_rollback(dev);
			return err;
		}

		return 0;
	}

	return -ENOTSUP;
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
	case OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL:
	case OPAMP_FUNCTIONAL_MODE_INVERTING:
	case OPAMP_FUNCTIONAL_MODE_NON_INVERTING:
		/* For differential, inverting and non-inverting mode,
		 * set Ngain to select different gains, and set Pgain
		 * to be the same as Ngain.
		 */
		config->base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_NGAIN_MASK) |
					   OPAMP_OPAMP_CTR_NGAIN(gain_index));
		config->base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PGAIN_MASK) |
					   OPAMP_OPAMP_CTR_PGAIN(gain_index));
		break;
	case OPAMP_FUNCTIONAL_MODE_FOLLOWER:
		config->base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_NGAIN_MASK) |
					   OPAMP_OPAMP_CTR_NGAIN(OPAMP_CTR_NGAIN_BUFFER_MODE));
		config->base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PGAIN_MASK) |
					   OPAMP_OPAMP_CTR_PGAIN(OPAMP_CTR_PGAIN_NON_INVERT_1X));
	default:
		break;
	}

	return 0;
}

static int mcux_opamp_configure(const struct device *dev)
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

	if (config->positive_reference != UINT8_MAX) {
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN
		base->OPAMP_CTR |= OPAMP_OPAMP_CTR_BUFEN_MASK;
#endif
		base->OPAMP_CTR = ((config->base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PREF_MASK) |
						OPAMP_OPAMP_CTR_PREF(config->positive_reference));
	} else {
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN
		base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_BUFEN_MASK;
#else
		base->OPAMP_CTR = ((base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PREF_MASK) |
						OPAMP_OPAMP_CTR_PREF(OPAMP_CTR_PREF_VREFH4));
#endif
	}

#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_OUTSW) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_OUTSW
	base->OPAMP_CTR |= OPAMP_OPAMP_CTR_OUTSW_MASK;
#endif

	switch (config->functional_mode) {
	case OPAMP_FUNCTIONAL_MODE_DIFFERENTIAL:
	case OPAMP_FUNCTIONAL_MODE_INVERTING:
	case OPAMP_FUNCTIONAL_MODE_NON_INVERTING:
		break;
	case OPAMP_FUNCTIONAL_MODE_FOLLOWER:
#if defined(FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN) && FSL_FEATURE_OPAMP_HAS_OPAMP_CTR_BUFEN
		base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_BUFEN_MASK;
#else
		base->OPAMP_CTR = ((base->OPAMP_CTR & ~OPAMP_OPAMP_CTR_PREF_MASK) |
				   OPAMP_OPAMP_CTR_PREF(OPAMP_CTR_PREF_VREFH4));
#endif
		break;
	default:
		LOG_ERR("Unsupported functional mode %d", config->functional_mode);
		return -ENOTSUP;
	}

	return 0;
}

int mcux_opamp_init(const struct device *dev)
{
	const struct mcux_opamp_config *config = dev->config;
	int ret;

	/* Enable OPAMP clock. */
	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device is not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);

	if (ret) {
		LOG_ERR("Device clock turn on failed");
		return ret;
	}

	/* Only LPC implemented RESET driver */
#if defined(CONFIG_SOC_FAMILY_LPC)
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("Reset device is not ready");
		return -ENODEV;
	}

	ret = reset_line_assert(config->reset.dev, config->reset.id);
	if (ret) {
		LOG_ERR("Device clock turn on failed");
		return ret;
	}
#endif

	ret = mcux_opamp_configure(dev);
	if (ret) {
		return ret;
	}

	return pm_device_driver_init(dev, mcux_opamp_pm_callback);
}

static DEVICE_API(opamp, api) = {
	.set_gain = mcux_opamp_set_gain,
};

#if defined(CONFIG_SOC_FAMILY_LPC)
#define GET_RESET_INFO(inst) .reset = RESET_DT_SPEC_INST_GET(inst),
#else
#define GET_RESET_INFO(inst)
#endif

#define OPAMP_MCUX_OPAMP_DEFINE(inst)								\
	PINCTRL_DT_INST_DEFINE(inst);								\
												\
	static const struct mcux_opamp_config _CONCAT(config, inst) = {				\
		.base = (OPAMP_Type *)DT_INST_REG_ADDR(inst),					\
		.positive_reference = COND_CODE_1(						\
			DT_INST_NODE_HAS_PROP(inst, non_inverting_reference),			\
				(DT_INST_ENUM_IDX(inst, non_inverting_reference)),		\
				(UINT8_MAX)),							\
		.operation_mode = DT_INST_ENUM_IDX(inst, operation_mode),			\
		.functional_mode = DT_INST_ENUM_IDX(inst, functional_mode),			\
		.enable_measure_reference = DT_INST_PROP(inst, enable_measure_reference),	\
		.enable_measure_output = DT_INST_PROP(inst, enable_measure_output),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),				\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),					\
		GET_RESET_INFO(inst)								\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, mcux_opamp_pm_callback);					\
												\
	DEVICE_DT_INST_DEFINE(inst, mcux_opamp_init,						\
			PM_DEVICE_DT_INST_GET(inst), NULL,					\
			&_CONCAT(config, inst), POST_KERNEL,					\
			CONFIG_OPAMP_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OPAMP_MCUX_OPAMP_DEFINE)
