/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_i2c_target_tmp103

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/target/regset_target_lib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/minmax.h>

LOG_MODULE_REGISTER(i2c_tmp103, CONFIG_I2C_LOG_LEVEL);

struct tmp103_target_data {
	struct regset_target_lib_data regset_data;
	const struct device *dev;
	struct k_work_delayable dwork;
};

struct tmp103_target_config {
	struct regset_target_lib_config regset_cfg;
	const struct device *sensor_dev;
};

REGSET_TARGET_LIB_STRUCT_CHECK(struct tmp103_target_config,
			       struct tmp103_target_data);

static void tmp103_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmp103_target_data *data = CONTAINER_OF(dwork, struct tmp103_target_data, dwork);
	const struct device *dev = data->dev;
	const struct tmp103_target_config *cfg = dev->config;
	struct sensor_value val;
	int ret;

	if (cfg->sensor_dev == NULL) {
		LOG_ERR("No sensor configured");
		return;
	}

	ret = sensor_sample_fetch_chan(cfg->sensor_dev, SENSOR_CHAN_DIE_TEMP);
	if (ret < 0) {
		LOG_ERR("sample_fetch error: %d", ret);
		return;
	}

	ret = sensor_channel_get(cfg->sensor_dev, SENSOR_CHAN_DIE_TEMP, &val);
	if (ret < 0) {
		LOG_ERR("channel_get error: %d", ret);
		return;
	}

	LOG_DBG("temp: %d", val.val1);

	int8_t temp_c = clamp(val.val1, 0, UINT8_MAX);

	regset_target_lib_write_data(dev, 0, &temp_c, sizeof(temp_c));

	k_work_schedule(&data->dwork, K_MSEC(500));
}

static void tmp103_change_cb(const struct device *dev, void *user_data)
{
	int8_t regs[4];

	regset_target_lib_read_data(dev, 0, regs, sizeof(regs));

	LOG_INF("Temp: %d Config: 0x%02x Thigh: %d Tlow: %d",
		regs[0], regs[1], regs[2], regs[3]);
}

static int tmp103_target_init(const struct device *dev)
{
	const struct tmp103_target_config *cfg = dev->config;
	struct tmp103_target_data *data = dev->data;

	data->dev = dev;

	if (cfg->sensor_dev != NULL && !device_is_ready(cfg->sensor_dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->sensor_dev);
		return -ENODEV;
	}

	regset_target_lib_set_changed_callback(dev, tmp103_change_cb, NULL);

	k_work_init_delayable(&data->dwork, tmp103_work_handler);

	if (cfg->sensor_dev != NULL) {
		k_work_schedule(&data->dwork, K_MSEC(500));
	}

	return regset_target_lib_init(dev);
}

#define I2C_TMP103_INIT(inst)								\
	REGSET_TARGET_LIB_DT_INST_BUILD_ASSERT(inst)					\
											\
	static struct tmp103_target_data tmp103_target_##inst##_dev_data;		\
											\
	static const struct tmp103_target_config tmp103_target_##inst##_cfg = {		\
		.regset_cfg = REGSET_TARGET_LIB_DT_INST_CONFIG_INIT(inst),		\
		.sensor_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, sensor)),	\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, &tmp103_target_init, NULL,				\
			      &tmp103_target_##inst##_dev_data,				\
			      &tmp103_target_##inst##_cfg,				\
			      POST_KERNEL, CONFIG_I2C_TARGET_INIT_PRIORITY,		\
			      &regset_target_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_TMP103_INIT)
