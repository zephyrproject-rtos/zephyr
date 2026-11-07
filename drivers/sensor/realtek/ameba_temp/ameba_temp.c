/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_temp

#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ameba_temp, CONFIG_SENSOR_LOG_LEVEL);

struct ameba_temp_data {
	struct k_mutex mutex; /* serializes access to c_temp */
	float c_temp;         /* Celsius degree of float type */
};

static int ameba_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ameba_temp_data *data = dev->data;
	uint32_t tm;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	tm = TM_GetTempResult();
	if (tm == TM_INVALID_VALUE) {
		LOG_ERR("failed to read temperature");
		return -EIO;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	data->c_temp = TM_GetCdegree(tm);
	k_mutex_unlock(&data->mutex);

	return 0;
}

static int ameba_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct ameba_temp_data *data = dev->data;
	int rc;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);
	rc = sensor_value_from_float(val, data->c_temp);
	k_mutex_unlock(&data->mutex);

	return rc;
}

static DEVICE_API(sensor, ameba_temp_driver_api) = {
	.sample_fetch = ameba_temp_sample_fetch,
	.channel_get = ameba_temp_channel_get,
};

static int ameba_temp_init(const struct device *dev)
{
	struct ameba_temp_data *data = dev->data;
	TM_InitTypeDef TM_InitStruct;

	k_mutex_init(&data->mutex);

	TM_StructInit(&TM_InitStruct);
	TM_Init(&TM_InitStruct);

	/* disable high_pt & high_wt & low_wt */
	TM_INTConfig(TM_BIT_IMR_TM_HIGH_WT | TM_BIT_IMR_TM_LOW_WT, DISABLE);
	TM_HighPtConfig(0x0, DISABLE);
	TM_HighWtConfig(0x0, DISABLE);
	TM_LowWtConfig(0x0, DISABLE);

	return 0;
}

#define AMEBA_TEMP_DEFINE(inst)                                                                    \
	static struct ameba_temp_data ameba_temp_dev_data_##inst = {};                             \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ameba_temp_init, NULL, &ameba_temp_dev_data_##inst,     \
				     NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,               \
				     &ameba_temp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMEBA_TEMP_DEFINE)
