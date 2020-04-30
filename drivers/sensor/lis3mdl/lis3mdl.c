/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis3mdl_magn

#include <drivers/i2c.h>
#include <init.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <string.h>
#include <logging/log.h>

#include "lis3mdl.h"

LOG_MODULE_REGISTER(LIS3MDL, CONFIG_SENSOR_LOG_LEVEL);

static void lis3mdl_convert(struct sensor_value *val, int16_t raw_val,
			    uint16_t divider)
{
	/* val = raw_val / divider */
	val->val1 = raw_val / divider;
	val->val2 = (((int64_t)raw_val % divider) * 1000000L) / divider;
}

static int lis3mdl_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lis3mdl_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_MAGN_XYZ) {
		/* magn_val = sample / mang_gain */
		lis3mdl_convert(val, drv_data->x_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
		lis3mdl_convert(val + 1, drv_data->y_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
		lis3mdl_convert(val + 2, drv_data->z_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_X) {
		lis3mdl_convert(val, drv_data->x_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_Y) {
		lis3mdl_convert(val, drv_data->y_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_Z) {
		lis3mdl_convert(val, drv_data->z_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else { /* chan == SENSOR_CHAN_DIE_TEMP */
		/* temp_val = 25 + sample / 8 */
		lis3mdl_convert(val, drv_data->temp_sample, 8);
		val->val1 += 25;
	}

	return 0;
}

int lis3mdl_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lis3mdl_data *drv_data = dev->data;
	int16_t buf[4];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* fetch magnetometer sample */
	if (i2c_burst_read(drv_data->i2c, DT_INST_REG_ADDR(0),
			   LIS3MDL_REG_SAMPLE_START, (uint8_t *)buf, 8) < 0) {
		LOG_DBG("Failed to fetch megnetometer sample.");
		return -EIO;
	}

	/*
	 * the chip doesn't allow fetching temperature data in
	 * the same read as magnetometer data, so do another
	 * burst read to fetch the temperature sample
	 */
	if (i2c_burst_read(drv_data->i2c, DT_INST_REG_ADDR(0),
			   LIS3MDL_REG_SAMPLE_START + 6,
			   (uint8_t *)(buf + 3), 2) < 0) {
		LOG_DBG("Failed to fetch temperature sample.");
		return -EIO;
	}

	drv_data->x_sample = sys_le16_to_cpu(buf[0]);
	drv_data->y_sample = sys_le16_to_cpu(buf[1]);
	drv_data->z_sample = sys_le16_to_cpu(buf[2]);
	drv_data->temp_sample = sys_le16_to_cpu(buf[3]);

	return 0;
}

static const struct sensor_driver_api lis3mdl_driver_api = {
#if CONFIG_LIS3MDL_TRIGGER
	.trigger_set = lis3mdl_trigger_set,
#endif
	.sample_fetch = lis3mdl_sample_fetch,
	.channel_get = lis3mdl_channel_get,
};

int lis3mdl_init(const struct device *dev)
{
	struct lis3mdl_data *drv_data = dev->data;
	uint8_t chip_cfg[6];
	uint8_t id, idx;

	drv_data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));

	if (drv_data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	/* check chip ID */
	if (i2c_reg_read_byte(drv_data->i2c, DT_INST_REG_ADDR(0),
			      LIS3MDL_REG_WHO_AM_I, &id) < 0) {
		LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	if (id != LIS3MDL_CHIP_ID) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	/* check if CONFIG_LIS3MDL_ODR is valid */
	for (idx = 0U; idx < ARRAY_SIZE(lis3mdl_odr_strings); idx++) {
		if (!strcmp(lis3mdl_odr_strings[idx], CONFIG_LIS3MDL_ODR)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(lis3mdl_odr_strings)) {
		LOG_ERR("Invalid ODR value.");
		return -EINVAL;
	}

	/* Configure sensor */
	chip_cfg[0] = LIS3MDL_REG_CTRL1;
	chip_cfg[1] = LIS3MDL_TEMP_EN_MASK | lis3mdl_odr_bits[idx];
	chip_cfg[2] = LIS3MDL_FS_IDX << LIS3MDL_FS_SHIFT;
	chip_cfg[3] = lis3mdl_odr_bits[idx] & LIS3MDL_FAST_ODR_MASK ?
		      LIS3MDL_MD_SINGLE : LIS3MDL_MD_CONTINUOUS;
	chip_cfg[4] = ((lis3mdl_odr_bits[idx] & LIS3MDL_OM_MASK) >>
		       LIS3MDL_OM_SHIFT) << LIS3MDL_OMZ_SHIFT;
	chip_cfg[5] = LIS3MDL_BDU_EN;

	if (i2c_write(drv_data->i2c,
		      chip_cfg, 6, DT_INST_REG_ADDR(0)) < 0) {
		LOG_DBG("Failed to configure chip.");
		return -EIO;
	}

#ifdef CONFIG_LIS3MDL_TRIGGER
	if (lis3mdl_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupts.");
		return -EIO;
	}
#endif

	return 0;
}

struct lis3mdl_data lis3mdl_driver;

DEVICE_AND_API_INIT(lis3mdl, DT_INST_LABEL(0), lis3mdl_init,
		    &lis3mdl_driver, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &lis3mdl_driver_api);
