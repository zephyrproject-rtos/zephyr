/*
 * SPDX-FileCopyrightText: <text>Copyright 2021-2022, 2024 Arm Limited and/or its
 * affiliates <open-source-office@arm.com></text>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <ethosu_driver.h>

#include "ethos_u_common.h"

#define DT_DRV_COMPAT arm_ethos_u
LOG_MODULE_REGISTER(arm_ethos_u, CONFIG_ETHOS_U_LOG_LEVEL);

void ethosu_zephyr_irq_handler(const struct device *dev)
{
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;

	ethosu_irq_handler(drv);
}

static int ethosu_zephyr_init(const struct device *dev)
{
	const struct ethosu_dts_info *config = dev->config;
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;
	struct ethosu_driver_version version;

	LOG_DBG("Ethos-U DTS info. base_address=0x%p, secure_enable=%u, privilege_enable=%u",
		config->base_addr, config->secure_enable, config->privilege_enable);

	ethosu_get_driver_version(&version);

	LOG_DBG("Version. major=%u, minor=%u, patch=%u", version.major, version.minor,
		version.patch);

	if (ethosu_init(drv, config->base_addr, NULL, 0, config->secure_enable,
			config->privilege_enable)) {
		LOG_ERR("Failed to initialize NPU with ethosu_init().");
		return -EINVAL;
	}

	config->irq_config();

	return 0;
}

#define ETHOSU_DEVICE_INIT(n)                                                                      \
	static struct ethosu_data ethosu_data_##n;                                                 \
                                                                                                   \
	static void ethosu_zephyr_irq_config_##n(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ethosu_zephyr_irq_handler,  \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ethosu_dts_info ethosu_dts_info_##n = {                                \
		.base_addr = (void *)DT_INST_REG_ADDR(n),                                          \
		.secure_enable = DT_INST_PROP(n, secure_enable),                                   \
		.privilege_enable = DT_INST_PROP(n, privilege_enable),                             \
		.irq_config = &ethosu_zephyr_irq_config_##n,                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ethosu_zephyr_init, NULL, &ethosu_data_##n, &ethosu_dts_info_##n, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(ETHOSU_DEVICE_INIT);
