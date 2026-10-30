/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/policy.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_opamp_fast, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_opamp_fast

/*
 * With CONFIG_PM_DEVICE_RUNTIME the consumer owns the amplifier's lifetime. An
 * amplifier output has no completion event the driver could hang a
 * pm_device_runtime_put() on, so the driver takes no runtime reference of its
 * own: call pm_device_runtime_get() and keep the reference for as long as the
 * output is needed. The driver does not read is-reference-source: an instance
 * that only provides the reference input of other amplifiers needs its own
 * pm_device_runtime_get() from whoever uses those amplifiers, because no
 * consumer node names the provider.
 *
 * A node that names power states in zephyr,disabling-power-states gets those
 * states blocked for as long as the device is active, so a consumer that holds
 * a runtime reference does not have to know which system states would break
 * the output it asked for.
 */

struct mcux_opamp_fast_config {
	OPAMP_Type *base;
	uint8_t bias_current;
	uint8_t functional_mode;
	uint8_t compensation_capcitor;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;
};

static void mcux_opamp_fast_configure(const struct device *dev);

/*
 * The core keeps a device whose suspend failed in PM_DEVICE_STATE_ACTIVE, and
 * device runtime PM restores the usage count it had before the suspend, so a
 * suspend that cannot finish leaves the output driven as it found it.
 */
static void mcux_opamp_fast_suspend_rollback(const struct device *dev)
{
	const struct mcux_opamp_fast_config *config = dev->config;

	(void)pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	config->base->OPAMP_CTRL |= OPAMP_OPAMP_CTRL_OPA_EN_MASK;
}

static int mcux_opamp_fast_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct mcux_opamp_fast_config *config = dev->config;
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * A state that power gates the block leaves OPAMP_CTRL and the
		 * clock gate at their reset values, and driver init does not
		 * re-run on the way back. Ungate the clock, rebuild the
		 * devicetree configuration, then gate it again: the core
		 * records the device as suspended once this returns, and a
		 * suspended device holds no clock.
		 */
		err = clock_control_on(config->clock_dev, config->clock_subsys);
		if (err) {
			return err;
		}

		mcux_opamp_fast_configure(dev);

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

		config->base->OPAMP_CTRL |= OPAMP_OPAMP_CTRL_OPA_EN_MASK;

		/*
		 * The amplifier drives its output from here until the next
		 * suspend. Block the states the node declares in
		 * zephyr,disabling-power-states for that long.
		 */
		pm_policy_device_power_lock_get(dev);

		return 0;

	case PM_DEVICE_ACTION_SUSPEND:
		config->base->OPAMP_CTRL &= ~OPAMP_OPAMP_CTRL_OPA_EN_MASK;

		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			mcux_opamp_fast_suspend_rollback(dev);
			return err;
		}

		/*
		 * The amplifier is off from here until the next resume, and a
		 * gated clock leaves OPAMP_CTRL unreachable in the meantime.
		 */
		err = clock_control_off(config->clock_dev, config->clock_subsys);
		if (err) {
			mcux_opamp_fast_suspend_rollback(dev);
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

static int mcux_opamp_fast_set_gain(const struct device *dev, enum opamp_gain gain)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(gain);

	return -ENOTSUP;
}

static DEVICE_API(opamp, api) = {
	.set_gain = mcux_opamp_fast_set_gain,
};

static void mcux_opamp_fast_configure(const struct device *dev)
{
	const struct mcux_opamp_fast_config *config = dev->config;
	OPAMP_Type *base = config->base;

	base->OPAMP_CTRL = ((base->OPAMP_CTRL & ~OPAMP_OPAMP_CTRL_OPA_BC_SEL_MASK) |
			    OPAMP_OPAMP_CTRL_OPA_BC_SEL(config->bias_current));
	base->OPAMP_CTRL = ((base->OPAMP_CTRL & ~OPAMP_OPAMP_CTRL_OPA_CC_SEL_MASK) |
			    OPAMP_OPAMP_CTRL_OPA_CC_SEL(config->compensation_capcitor));
}

int mcux_opamp_fast_init(const struct device *dev)
{
	const struct mcux_opamp_fast_config *config = dev->config;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device is not ready");
		return -ENODEV;
	}

	/*
	 * TURN_ON brings up the clock gate and the register block.
	 * pm_device_driver_init() runs it here, and the core runs it again
	 * every time the block comes back from a power cycle.
	 */
	return pm_device_driver_init(dev, mcux_opamp_fast_pm_callback);
}

#define OPAMP_MCUX_OPAMP_FAST_DEFINE(inst)						\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static const struct mcux_opamp_fast_config _CONCAT(config, inst) = {		\
		.base = (OPAMP_Type *)DT_INST_REG_ADDR(inst),				\
		.bias_current = DT_INST_ENUM_IDX(inst, bias_current),			\
		.compensation_capcitor = DT_INST_ENUM_IDX(inst,				\
				compensation_capcitor),					\
		.functional_mode = DT_INST_ENUM_IDX(inst, functional_mode),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
	};										\
											\
	PM_DEVICE_DT_INST_DEFINE(inst, mcux_opamp_fast_pm_callback);			\
											\
	DEVICE_DT_INST_DEFINE(inst, mcux_opamp_fast_init,				\
			PM_DEVICE_DT_INST_GET(inst), NULL,				\
			&_CONCAT(config, inst), POST_KERNEL,				\
			CONFIG_OPAMP_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OPAMP_MCUX_OPAMP_FAST_DEFINE)
