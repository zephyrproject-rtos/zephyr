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
#include <zephyr/pm/policy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_opamp, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_opamp

/*
 * With CONFIG_PM_DEVICE_RUNTIME the consumer owns the amplifier's lifetime. An
 * amplifier output has no completion event the driver could hang a
 * pm_device_runtime_put() on, so the driver takes no runtime reference of its
 * own: call pm_device_runtime_get() before opamp_set_gain() and keep the
 * reference for as long as the output is needed.
 *
 * A node that names power states in zephyr,disabling-power-states gets those
 * states blocked for as long as the device is active, so a consumer that holds
 * a runtime reference does not have to know which system states would break
 * the output it asked for.
 */

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

struct mcux_opamp_data {
	/* Zero until opamp_set_gain() selects a gain; the encoding starts at 1. */
	uint8_t gain_index;
};

static int mcux_opamp_configure(const struct device *dev);

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

/*
 * Only an active device holds its clock. A suspended device has OPAMP_CTR[EN]
 * cleared and its clock gated, and one the core has turned off has lost the
 * contents of its register block. Without CONFIG_PM_DEVICE
 * pm_device_state_get() reports the device as active and this check passes.
 */
static bool mcux_opamp_is_active(const struct device *dev)
{
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);

	return state == PM_DEVICE_STATE_ACTIVE;
}

static void mcux_opamp_apply_gain(const struct device *dev, uint8_t gain_index)
{
	const struct mcux_opamp_config *config = dev->config;

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
}

static int mcux_opamp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct mcux_opamp_config *config = dev->config;
	struct mcux_opamp_data *data = dev->data;
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * A state that power gates the block leaves OPAMP_CTR and the
		 * clock gate at their reset values, and driver init does not
		 * re-run on the way back. Ungate the clock, rebuild the
		 * devicetree configuration, then gate it again: the core
		 * records the device as suspended once this returns, and a
		 * suspended device holds no clock. That holds for a failed
		 * rebuild too, which the core also records as suspended, so
		 * every path out of here gates the clock.
		 */
		err = clock_control_on(config->clock_dev, config->clock_subsys);
		if (err) {
			return err;
		}

		/* Only LPC implemented RESET driver */
#if defined(CONFIG_SOC_FAMILY_LPC)
		err = reset_line_assert(config->reset.dev, config->reset.id);
		if (err) {
			LOG_ERR("Failed to assert reset line");
			(void)clock_control_off(config->clock_dev, config->clock_subsys);
			return err;
		}
#endif

		err = mcux_opamp_configure(dev);
		if (err) {
			(void)clock_control_off(config->clock_dev, config->clock_subsys);
			return err;
		}

		/* mcux_opamp_configure() leaves the gain fields alone. */
		if (data->gain_index != 0) {
			mcux_opamp_apply_gain(dev, data->gain_index);
		}

		return clock_control_off(config->clock_dev, config->clock_subsys);

	case PM_DEVICE_ACTION_RESUME:
		err = clock_control_on(config->clock_dev, config->clock_subsys);
		if (err) {
			return err;
		}

		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			(void)clock_control_off(config->clock_dev, config->clock_subsys);
			return err;
		}

		config->base->OPAMP_CTR |= OPAMP_OPAMP_CTR_EN_MASK;

		/*
		 * The amplifier drives its output from here until the next
		 * suspend. Block the states the node declares in
		 * zephyr,disabling-power-states for that long.
		 */
		pm_policy_device_power_lock_get(dev);

		return 0;

	case PM_DEVICE_ACTION_SUSPEND:
		config->base->OPAMP_CTR &= ~OPAMP_OPAMP_CTR_EN_MASK;

		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			mcux_opamp_suspend_rollback(dev);
			return err;
		}

		/*
		 * The amplifier is off from here until the next resume, and a
		 * gated clock leaves OPAMP_CTR unreachable in the meantime.
		 */
		err = clock_control_off(config->clock_dev, config->clock_subsys);
		if (err) {
			mcux_opamp_suspend_rollback(dev);
			return err;
		}

		/*
		 * Nothing drives the output until the next resume. Release the
		 * lock last, so a suspend that fails has none to reclaim.
		 */
		pm_policy_device_power_lock_put(dev);

		return 0;

	case PM_DEVICE_ACTION_TURN_OFF:
		/* The core runs this only on a suspended device, whose clock is gated. */
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int mcux_opamp_set_gain(const struct device *dev, enum opamp_gain gain)
{
	struct mcux_opamp_data *data = dev->data;
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

	if (!mcux_opamp_is_active(dev)) {
		LOG_ERR("device is not active");
		return -EBUSY;
	}

	data->gain_index = gain_index;

	mcux_opamp_apply_gain(dev, gain_index);

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

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device is not ready");
		return -ENODEV;
	}

	/* Only LPC implemented RESET driver */
#if defined(CONFIG_SOC_FAMILY_LPC)
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("Reset device is not ready");
		return -ENODEV;
	}
#endif

	/*
	 * TURN_ON brings up the clock gate, the reset line and OPAMP_CTR.
	 * pm_device_driver_init() runs it here, and the core runs it again
	 * every time the block comes back from a power cycle.
	 */
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
	static struct mcux_opamp_data _CONCAT(data, inst);					\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, mcux_opamp_pm_callback);					\
												\
	DEVICE_DT_INST_DEFINE(inst, mcux_opamp_init,						\
			PM_DEVICE_DT_INST_GET(inst), &_CONCAT(data, inst),			\
			&_CONCAT(config, inst), POST_KERNEL,					\
			CONFIG_OPAMP_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OPAMP_MCUX_OPAMP_DEFINE)
