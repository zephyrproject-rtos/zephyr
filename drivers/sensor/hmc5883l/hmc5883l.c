/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT honeywell_hmc5883l

#include <drivers/i2c.h>
#include <init.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <string.h>
#include <logging/log.h>

#include "hmc5883l.h"

LOG_MODULE_REGISTER(HMC5883L, CONFIG_SENSOR_LOG_LEVEL);

static void hmc5883l_convert(struct sensor_value *val, int16_t raw_val,
			     uint16_t divider)
{
	/* val = raw_val / divider */
	val->val1 = raw_val / divider;
	val->val2 = (((int64_t)raw_val % divider) * 1000000L) / divider;
}

static int hmc5883l_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct hmc5883l_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_MAGN_X) {
		hmc5883l_convert(val, drv_data->x_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
	} else if (chan == SENSOR_CHAN_MAGN_Y) {
		hmc5883l_convert(val, drv_data->y_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
	} else if (chan == SENSOR_CHAN_MAGN_Z) {
		hmc5883l_convert(val, drv_data->z_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
	} else { /* chan == SENSOR_CHAN_MAGN_XYZ */
		hmc5883l_convert(val, drv_data->x_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
		hmc5883l_convert(val + 1, drv_data->y_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
		hmc5883l_convert(val + 2, drv_data->z_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
	}

	return 0;
}

static int hmc5883l_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct hmc5883l_data *drv_data = dev->data;
	int16_t buf[3];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* fetch magnetometer sample */
	if (i2c_burst_read(drv_data->i2c,
			   DT_INST_REG_ADDR(0),
			   HMC5883L_REG_DATA_START, (uint8_t *)buf, 6) < 0) {
		LOG_ERR("Failed to fetch megnetometer sample.");
		return -EIO;
	}

	drv_data->x_sample = sys_be16_to_cpu(buf[0]);
	drv_data->z_sample = sys_be16_to_cpu(buf[1]);
	drv_data->y_sample = sys_be16_to_cpu(buf[2]);

	return 0;
}

static const struct sensor_driver_api hmc5883l_driver_api = {
#if CONFIG_HMC5883L_TRIGGER
	.trigger_set = hmc5883l_trigger_set,
#endif
	.sample_fetch = hmc5883l_sample_fetch,
	.channel_get = hmc5883l_channel_get,
};

int hmc5883l_init(const struct device *dev)
{
	struct hmc5883l_data *drv_data = dev->data;
	uint8_t chip_cfg[3], id[3], idx;

	drv_data->i2c = device_get_binding(
		DT_INST_BUS_LABEL(0));
	if (drv_data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device.",
			DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	/* check chip ID */
	if (i2c_burst_read(drv_data->i2c,
			   DT_INST_REG_ADDR(0),
			   HMC5883L_REG_CHIP_ID, id, 3) < 0) {
		LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	if (id[0] != HMC5883L_CHIP_ID_A || id[1] != HMC5883L_CHIP_ID_B ||
	    id[2] != HMC5883L_CHIP_ID_C) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	/* check if CONFIG_HMC5883L_FS is valid */
	for (idx = 0U; idx < ARRAY_SIZE(hmc5883l_fs_strings); idx++) {
		if (!strcmp(hmc5883l_fs_strings[idx], CONFIG_HMC5883L_FS)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(hmc5883l_fs_strings)) {
		LOG_ERR("Invalid full-scale range value.");
		return -EINVAL;
	}

	drv_data->gain_idx = idx;

	/* check if CONFIG_HMC5883L_ODR is valid */
	for (idx = 0U; idx < ARRAY_SIZE(hmc5883l_odr_strings); idx++) {
		if (!strcmp(hmc5883l_odr_strings[idx], CONFIG_HMC5883L_ODR)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(hmc5883l_odr_strings)) {
		LOG_ERR("Invalid ODR value.");
		return -EINVAL;
	}

	/* configure device */
	chip_cfg[0] = idx << HMC5883L_ODR_SHIFT;
	chip_cfg[1] = drv_data->gain_idx << HMC5883L_GAIN_SHIFT;
	chip_cfg[2] = HMC5883L_MODE_CONTINUOUS;

	if (i2c_burst_write(drv_data->i2c,
			    DT_INST_REG_ADDR(0),
			    HMC5883L_REG_CONFIG_A, chip_cfg, 3) < 0) {
		LOG_ERR("Failed to configure chip.");
		return -EIO;
	}

#ifdef CONFIG_HMC5883L_TRIGGER
	if (hmc5883l_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupts.");
		return -EIO;
	}
#endif

	return 0;
}

struct hmc5883l_data hmc5883l_driver;

DEVICE_DT_INST_DEFINE(0, hmc5883l_init, device_pm_control_nop,
		    &hmc5883l_driver, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &hmc5883l_driver_api);
