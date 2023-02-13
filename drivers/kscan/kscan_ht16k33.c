/*
 * Copyright (c) 2019 - 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT holtek_ht16k33_keyscan

/**
 * @file
 * @brief Keyscan driver for the HT16K33 I2C LED driver
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/drivers/led/ht16k33.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(kscan_ht16k33, CONFIG_KSCAN_LOG_LEVEL);

BUILD_ASSERT(CONFIG_KSCAN_INIT_PRIORITY > CONFIG_LED_INIT_PRIORITY,
	"HT16K33 keyscan driver must be initialized after HT16K33 LED driver");

struct kscan_ht16k33_cfg {
	const struct device *parent;
};

static int kscan_ht16k33_config(const struct device *dev,
				kscan_callback_t callback)
{
	const struct kscan_ht16k33_cfg *config = dev->config;

	return ht16k33_register_keyscan_callback(config->parent, dev, callback);
}

static int kscan_ht16k33_init(const struct device *dev)
{
	const struct kscan_ht16k33_cfg *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("HT16K33 parent device not ready");
		return -EINVAL;
	}

	return 0;
}

static const struct kscan_driver_api kscan_ht16k33_api = {
	.config = kscan_ht16k33_config,
};

#define KSCAN_HT16K33_DEVICE(id)					\
	static const struct kscan_ht16k33_cfg kscan_ht16k33_##id##_cfg = { \
		.parent = DEVICE_DT_GET(DT_INST_BUS(id)),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(id, &kscan_ht16k33_init,			\
			      NULL, NULL,				\
			      &kscan_ht16k33_##id##_cfg, POST_KERNEL,	\
			      CONFIG_KSCAN_INIT_PRIORITY,		\
			      &kscan_ht16k33_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_HT16K33_DEVICE)
