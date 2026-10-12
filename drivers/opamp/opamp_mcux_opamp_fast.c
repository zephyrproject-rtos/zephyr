/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/opamp.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_opamp_fast, CONFIG_OPAMP_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_opamp_fast

struct mcux_opamp_fast_config {
	OPAMP_Type *base;
	uint8_t bias_current;
	uint8_t functional_mode;
	uint8_t compensation_capcitor;
	bool is_reference_source;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;
};

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

	if (action == PM_DEVICE_ACTION_RESUME) {
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}

		config->base->OPAMP_CTRL |= OPAMP_OPAMP_CTRL_OPA_EN_MASK;
		return 0;
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		config->base->OPAMP_CTRL &= ~OPAMP_OPAMP_CTRL_OPA_EN_MASK;

		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			mcux_opamp_fast_suspend_rollback(dev);
			return err;
		}

		return 0;
	}

	return -ENOTSUP;
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

int mcux_opamp_fast_init(const struct device *dev)
{
	const struct mcux_opamp_fast_config *config = dev->config;
	OPAMP_Type *base = config->base;
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

	base->OPAMP_CTRL = ((base->OPAMP_CTRL & ~OPAMP_OPAMP_CTRL_OPA_BC_SEL_MASK) |
			    OPAMP_OPAMP_CTRL_OPA_BC_SEL(config->bias_current));
	base->OPAMP_CTRL = ((base->OPAMP_CTRL & ~OPAMP_OPAMP_CTRL_OPA_CC_SEL_MASK) |
			    OPAMP_OPAMP_CTRL_OPA_CC_SEL(config->compensation_capcitor));

	if (config->is_reference_source) {
		base->OPAMP_CTRL |= OPAMP_OPAMP_CTRL_OPA_EN_MASK;
	}

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
		.is_reference_source = DT_INST_PROP(inst, is_reference_source),		\
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
