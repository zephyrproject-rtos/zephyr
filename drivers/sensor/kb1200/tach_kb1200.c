/*
 * Copyright (c) 2021 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_tach

/**
 * @file
 * @brief ENE KB1200 tachometer sensor module driver
 *
 * This file contains a driver for the tachometer sensor.
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <soc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tach_kb1200, CONFIG_SENSOR_LOG_LEVEL);

/* Device config */
struct tach_kb1200_config {
	/* tachometer controller base address */
	uintptr_t base;
	/* selected port of tachometer */
	int port;
	/* number of pulses (holes) per round of tachometer's input (encoder) */
	int pulses_per_round;
};

/* Driver data */
struct tach_kb1200_data {
	/* Input clock for tachometers */
	uint32_t input_clk;
	/* Captured counts of tachometer */
	uint32_t capture;
};

/* TACH api functions */
int tach_kb1200_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);
	//struct tach_kb1200_data *const data = dev->data;

	return 0;
}

static int tach_kb1200_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	//const struct tach_kb1200_config *const config = dev->config;
	//struct tach_kb1200_data *const data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	val->val1 = 0U;
	val->val2 = 0U;

	return 0;
}


/* TACH driver registration */
static int tach_kb1200_init(const struct device *dev)
{
	//const struct tach_kb1200_config *const config = dev->config;
	//struct tach_kb1200_data *const data = dev->data;

	return 0;
}

static const struct sensor_driver_api tach_kb1200_driver_api = {
	.sample_fetch = tach_kb1200_sample_fetch,
	.channel_get = tach_kb1200_channel_get,
};

#define KB1200_TACH_INIT(inst)                                                   \
	static const struct tach_kb1200_config tach_cfg_##inst = {               \
		.base = DT_INST_REG_ADDR(inst),                                \
		.port = DT_INST_PROP(inst, port),                              \
	};                                                                     \
	static struct tach_kb1200_data tach_data_##inst;                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                                     \
			      tach_kb1200_init,                                  \
			      NULL,                                            \
			      &tach_data_##inst,                               \
			      &tach_cfg_##inst,                                \
			      POST_KERNEL,                                     \
			      CONFIG_SENSOR_INIT_PRIORITY,                     \
			      &tach_kb1200_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KB1200_TACH_INIT)
