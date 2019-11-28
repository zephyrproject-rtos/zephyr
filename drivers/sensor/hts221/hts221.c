/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <init.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <string.h>
#include <logging/log.h>

#include "hts221.h"

LOG_MODULE_REGISTER(HTS221, CONFIG_SENSOR_LOG_LEVEL);

static const char * const hts221_odr_strings[] = {
	"1", "7", "12.5"
};

static int hts221_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct hts221_data *drv_data = dev->driver_data;
	s32_t conv_val;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP ||
			chan == SENSOR_CHAN_HUMIDITY);

	/*
	 * see "Interpreting humidity and temperature readings" document
	 * for more details
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		conv_val = (s32_t)(drv_data->t1_degc_x8 -
				     drv_data->t0_degc_x8) *
			   (drv_data->t_sample - drv_data->t0_out) /
			   (drv_data->t1_out - drv_data->t0_out) +
			   drv_data->t0_degc_x8;

		/* convert temperature x8 to degrees Celsius */
		val->val1 = conv_val / 8;
		val->val2 = (conv_val % 8) * (1000000 / 8);
	} else { /* SENSOR_CHAN_HUMIDITY */
		conv_val = (s32_t)(drv_data->h1_rh_x2 - drv_data->h0_rh_x2) *
			   (drv_data->rh_sample - drv_data->h0_t0_out) /
			   (drv_data->h1_t0_out - drv_data->h0_t0_out) +
			   drv_data->h0_rh_x2;

		/* convert humidity x2 to percent */
		val->val1 = conv_val / 2;
		val->val2 = (conv_val % 2) * 500000;
	}

	return 0;
}

static int hts221_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct hts221_data *drv_data = dev->driver_data;
	u8_t buf[4];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (i2c_burst_read(drv_data->i2c, DT_INST_0_ST_HTS221_BASE_ADDRESS,
			   HTS221_REG_DATA_START | HTS221_AUTOINCREMENT_ADDR,
			   buf, 4) < 0) {
		LOG_ERR("Failed to fetch data sample.");
		return -EIO;
	}

	drv_data->rh_sample = sys_le16_to_cpu(buf[0] | (buf[1] << 8));
	drv_data->t_sample = sys_le16_to_cpu(buf[2] | (buf[3] << 8));

	return 0;
}

static int hts221_read_conversion_data(struct hts221_data *drv_data)
{
	u8_t buf[16];

	if (i2c_burst_read(drv_data->i2c, DT_INST_0_ST_HTS221_BASE_ADDRESS,
			   HTS221_REG_CONVERSION_START |
			   HTS221_AUTOINCREMENT_ADDR, buf, 16) < 0) {
		LOG_ERR("Failed to read conversion data.");
		return -EIO;
	}

	drv_data->h0_rh_x2 = buf[0];
	drv_data->h1_rh_x2 = buf[1];
	drv_data->t0_degc_x8 = sys_le16_to_cpu(buf[2] | ((buf[5] & 0x3) << 8));
	drv_data->t1_degc_x8 = sys_le16_to_cpu(buf[3] | ((buf[5] & 0xC) << 6));
	drv_data->h0_t0_out = sys_le16_to_cpu(buf[6] | (buf[7] << 8));
	drv_data->h1_t0_out = sys_le16_to_cpu(buf[10] | (buf[11] << 8));
	drv_data->t0_out = sys_le16_to_cpu(buf[12] | (buf[13] << 8));
	drv_data->t1_out = sys_le16_to_cpu(buf[14] | (buf[15] << 8));

	return 0;
}

static const struct sensor_driver_api hts221_driver_api = {
#if CONFIG_HTS221_TRIGGER
	.trigger_set = hts221_trigger_set,
#endif
	.sample_fetch = hts221_sample_fetch,
	.channel_get = hts221_channel_get,
};

int hts221_init(struct device *dev)
{
	struct hts221_data *drv_data = dev->driver_data;
	u8_t id, idx;

	drv_data->i2c = device_get_binding(DT_INST_0_ST_HTS221_BUS_NAME);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			    DT_INST_0_ST_HTS221_BUS_NAME);
		return -EINVAL;
	}

	/* check chip ID */
	if (i2c_reg_read_byte(drv_data->i2c, DT_INST_0_ST_HTS221_BASE_ADDRESS,
			      HTS221_REG_WHO_AM_I, &id) < 0) {
		LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	if (id != HTS221_CHIP_ID) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	/* check if CONFIG_HTS221_ODR is valid */
	for (idx = 0U; idx < ARRAY_SIZE(hts221_odr_strings); idx++) {
		if (!strcmp(hts221_odr_strings[idx], CONFIG_HTS221_ODR)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(hts221_odr_strings)) {
		LOG_ERR("Invalid ODR value.");
		return -EINVAL;
	}

	if (i2c_reg_write_byte(drv_data->i2c, DT_INST_0_ST_HTS221_BASE_ADDRESS,
			       HTS221_REG_CTRL1,
			       (idx + 1) << HTS221_ODR_SHIFT | HTS221_BDU_BIT |
			       HTS221_PD_BIT) < 0) {
		LOG_ERR("Failed to configure chip.");
		return -EIO;
	}

	/*
	 * the device requires about 2.2 ms to download the flash content
	 * into the volatile mem
	 */
	k_sleep(K_MSEC(3));

	if (hts221_read_conversion_data(drv_data) < 0) {
		LOG_ERR("Failed to read conversion data.");
		return -EINVAL;
	}

#ifdef CONFIG_HTS221_TRIGGER
	if (hts221_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

struct hts221_data hts221_driver;

DEVICE_AND_API_INIT(hts221, DT_INST_0_ST_HTS221_LABEL, hts221_init, &hts221_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &hts221_driver_api);
