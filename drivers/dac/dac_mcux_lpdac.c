/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright 2023, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_lpdac

#include <zephyr/kernel.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <fsl_dac.h>

LOG_MODULE_REGISTER(dac_mcux_lpdac, CONFIG_DAC_LOG_LEVEL);

/*
 * With CONFIG_PM_DEVICE_RUNTIME the consumer owns the output's lifetime. A DAC
 * output voltage has no completion event the driver could hang a
 * pm_device_runtime_put() on, so the driver takes no runtime reference of its
 * own: call pm_device_runtime_get() before dac_channel_setup() and keep the
 * reference for as long as the output is needed.
 *
 * A suspend switches the analog output buffer off and a resume restores the last
 * value written, so a consumer that keeps its reference does not have to call
 * dac_channel_setup() again.
 *
 * A state that power gates the DAC resets its register block. TURN_ON rebuilds
 * the devicetree configuration of an instance that was already set up, so a
 * resume after a power cycle restores the output the same way.
 *
 * An API call that arrives while the device is not active returns -EBUSY rather
 * than writing a register block that is gated or has lost its contents.
 *
 * A node that names power states in zephyr,disabling-power-states gets those
 * states blocked for as long as its output is on, so a consumer that holds a
 * runtime reference does not have to know which system states would break the
 * output it asked for.
 */

struct mcux_lpdac_config {
	LPDAC_Type *base;
	const struct pinctrl_dev_config *pincfg;
	dac_reference_voltage_source_t ref_voltage;
	bool low_power;
#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	bool pm_device_constraints;
#endif
};

struct mcux_lpdac_data {
	bool configured;
	bool output_on;
	uint32_t value;
#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	bool power_locked;
#endif
};

/*
 * Block the power states the node declares in zephyr,disabling-power-states for
 * exactly as long as the output is on and the device is active: taken when a
 * value is first written, dropped when the device suspends, and taken again when
 * a resume restores the output.
 */
static void mcux_lpdac_power_lock_get(const struct device *dev)
{
#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	const struct mcux_lpdac_config *config = dev->config;
	struct mcux_lpdac_data *data = dev->data;

	if (config->pm_device_constraints && !data->power_locked) {
		pm_policy_device_power_lock_get(dev);
		data->power_locked = true;
	}
#endif
}

static void mcux_lpdac_power_lock_put(const struct device *dev)
{
#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	struct mcux_lpdac_data *data = dev->data;

	if (data->power_locked) {
		pm_policy_device_power_lock_put(dev);
		data->power_locked = false;
	}
#endif
}

/*
 * Write the devicetree configuration into the register block. DAC_Init()
 * releases the peripheral reset, resets the logic and the FIFO, then writes
 * GCR in full, which leaves GCR[DACEN] clear: the block is configured but not
 * converting when this returns.
 */
static void mcux_lpdac_configure(const struct device *dev)
{
	const struct mcux_lpdac_config *config = dev->config;
	dac_config_t dac_config;

	DAC_GetDefaultConfig(&dac_config);
	dac_config.referenceVoltageSource = config->ref_voltage;
#if defined(FSL_FEATURE_LPDAC_HAS_GCR_BUF_SPD_CTRL) && FSL_FEATURE_LPDAC_HAS_GCR_BUF_SPD_CTRL
	dac_config.enableLowerLowPowerMode = config->low_power;
#else
	dac_config.enableLowPowerMode = config->low_power;
#endif
	DAC_Init(config->base, &dac_config);
}

/*
 * A device the core has suspended has its analog output buffer off, and one it
 * has turned off has lost the contents of its register block, so GCR and DATA
 * written here would either never reach a pin or be thrown away by the next
 * power cycle. Without CONFIG_PM_DEVICE the state reads back as active and this
 * always passes.
 */
static bool mcux_lpdac_is_active(const struct device *dev)
{
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);

	return state == PM_DEVICE_STATE_ACTIVE;
}

static int mcux_lpdac_channel_setup(const struct device *dev,
				    const struct dac_channel_cfg *channel_cfg)
{
	struct mcux_lpdac_data *data = dev->data;

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != 12) {
		LOG_ERR("unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	if (!mcux_lpdac_is_active(dev)) {
		LOG_ERR("device is not active");
		return -EBUSY;
	}

	mcux_lpdac_configure(dev);
	data->configured = true;
	data->output_on = false;
	data->value = 0U;

	/* DAC_Init() left GCR[DACEN] clear, so the output this was holding is gone. */
	mcux_lpdac_power_lock_put(dev);

	return 0;
}

static int mcux_lpdac_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct mcux_lpdac_config *config = dev->config;
	struct mcux_lpdac_data *data = dev->data;

	if (!data->configured) {
		LOG_ERR("channel not initialized");
		return -EINVAL;
	}

	if (channel != 0) {
		LOG_ERR("unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (value >= 4096) {
		LOG_ERR("unsupported value %d", value);
		return -EINVAL;
	}

	if (!mcux_lpdac_is_active(dev)) {
		LOG_ERR("device is not active");
		return -EBUSY;
	}

	mcux_lpdac_power_lock_get(dev);

	DAC_Enable(config->base, true);
	DAC_SetData(config->base, value);

	data->value = value;
	data->output_on = true;

	return 0;
}

static int mcux_lpdac_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct mcux_lpdac_config *config = dev->config;
	struct mcux_lpdac_data *data = dev->data;
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * The register block may have just come back from a power loss
		 * with GCR and DATA at their reset values, and nothing re-runs
		 * driver init on that path. Rebuild the configuration, but only
		 * for an instance the application has already set up: an
		 * untouched DAC keeps the reset state it had before this ran.
		 *
		 * The block is left disabled, which is the state the core
		 * records for the device once this returns.
		 */
		if (data->configured) {
			mcux_lpdac_configure(dev);
		}

		return 0;

	case PM_DEVICE_ACTION_RESUME:
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}

		/*
		 * Restore the output the consumer last asked for, and only
		 * that: a DAC whose value was never written stays disabled
		 * rather than starting to drive a pin at 0 V.
		 */
		if (data->output_on) {
			mcux_lpdac_power_lock_get(dev);
			DAC_Enable(config->base, true);
			DAC_SetData(config->base, data->value);
		}

		return 0;

	case PM_DEVICE_ACTION_SUSPEND:
		/*
		 * Clearing GCR[DACEN] switches the analog output buffer off, so
		 * the output really stops driving. Relying on the SoC to gate
		 * the DAC clock instead would leave the buffer holding whatever
		 * level it had.
		 */
		DAC_Enable(config->base, false);

		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			return err;
		}

		/*
		 * The output is off from here, so stop blocking the states that
		 * would have broken it. Under runtime PM this runs when the last
		 * consumer reference is dropped, which is the point where nothing
		 * wants the output any more.
		 */
		mcux_lpdac_power_lock_put(dev);

		return 0;

	case PM_DEVICE_ACTION_TURN_OFF:
		/*
		 * Nothing to do: the core suspends the device before it turns
		 * it off, so the output buffer is already off.
		 */
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int mcux_lpdac_init(const struct device *dev)
{
	return pm_device_driver_init(dev, mcux_lpdac_pm_callback);
}

static DEVICE_API(dac, mcux_lpdac_driver_api) = {
	.channel_setup = mcux_lpdac_channel_setup,
	.write_value = mcux_lpdac_write_value,
};

#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
#define MCUX_LPDAC_PM_DEVICE_CONSTRAINTS_INIT(n)                                                   \
	.pm_device_constraints = DT_INST_NODE_HAS_PROP(n, zephyr_disabling_power_states),
#else
#define MCUX_LPDAC_PM_DEVICE_CONSTRAINTS_INIT(n)
#endif

#define MCUX_LPDAC_INIT(n)                                                                         \
	static struct mcux_lpdac_data mcux_lpdac_data_##n;                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, mcux_lpdac_pm_callback);                                       \
                                                                                                   \
	static const struct mcux_lpdac_config mcux_lpdac_config_##n = {                            \
		.base = (LPDAC_Type *)DT_INST_REG_ADDR(n),                                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.ref_voltage = DT_INST_PROP(n, voltage_reference),                                 \
		.low_power = DT_INST_PROP(n, low_power_mode),                                      \
		MCUX_LPDAC_PM_DEVICE_CONSTRAINTS_INIT(n)                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mcux_lpdac_init, PM_DEVICE_DT_INST_GET(n),                        \
			      &mcux_lpdac_data_##n, &mcux_lpdac_config_##n,                        \
			      POST_KERNEL, CONFIG_DAC_INIT_PRIORITY,                               \
			      &mcux_lpdac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_LPDAC_INIT)
