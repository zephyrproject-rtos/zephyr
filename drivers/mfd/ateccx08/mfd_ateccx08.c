/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Microchip ATECCX08 I2C MFD.
 */
#include <zephyr/kernel.h>

#include "atecc_priv.h"
LOG_MODULE_REGISTER(ateccx08);

static int mfd_ateccx08_init(const struct device *dev)
{
	const struct ateccx08_config *config = dev->config;
	struct ateccx08_data *data = dev->data;

	k_mutex_init(&data->lock);

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("parent bus device not ready");
		return -EINVAL;
	}

	atecc_update_lock(dev);

	LOG_DBG("Config lock status: %d", data->is_locked_config);
	LOG_DBG("Data lock status: %d", data->is_locked_data);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int eeprom_ateccx08_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		(void)atecc_wakeup(dev);
		return atecc_sleep(dev);

	case PM_DEVICE_ACTION_RESUME:
		(void)atecc_wakeup(dev);
		return atecc_idle(dev);

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define INST_DT_ATECCX08(inst, t) DT_INST(inst, microchip_atecc##t)

#define MFD_ATECCX08_DEVICE(n, t)                                                                  \
	static const struct ateccx08_config atecc##t##_config_##n = {                              \
		.i2c = I2C_DT_SPEC_GET(INST_DT_ATECCX08(n, t)),                                    \
		.wakedelay = DT_PROP(INST_DT_ATECCX08(n, t), wake_delay),                          \
		.retries = DT_PROP(INST_DT_ATECCX08(n, t), retries)};                              \
                                                                                                   \
	static struct ateccx08_data atecc##t##_data_##n;                                           \
                                                                                                   \
	PM_DEVICE_DT_DEFINE(INST_DT_ATECCX08(n, t), eeprom_ateccx08_pm_action);                    \
                                                                                                   \
	DEVICE_DT_DEFINE(INST_DT_ATECCX08(n, t), &mfd_ateccx08_init,                               \
			 PM_DEVICE_DT_GET(INST_DT_ATECCX08(n, t)), &atecc##t##_data_##n,           \
			 &atecc##t##_config_##n, POST_KERNEL, CONFIG_MFD_ATECCX08_INIT_PRIORITY,   \
			 NULL)

#define MFD_ATECC608_DEVICE(n) MFD_ATECCX08_DEVICE(n, 608)
#define MFD_ATECC508_DEVICE(n) MFD_ATECCX08_DEVICE(n, 508)

#define CALL_WITH_ARG(arg, expr) expr(arg);

#define INST_DT_ATECCX08_FOREACH(t, inst_expr)                                                     \
	LISTIFY(DT_NUM_INST_STATUS_OKAY(microchip_atecc##t), CALL_WITH_ARG, (), inst_expr)

#ifdef CONFIG_MFD_ATECC608
INST_DT_ATECCX08_FOREACH(608, MFD_ATECC608_DEVICE);
#endif

#ifdef CONFIG_MFD_ATECC508
INST_DT_ATECCX08_FOREACH(508, MFD_ATECC508_DEVICE);
#endif
