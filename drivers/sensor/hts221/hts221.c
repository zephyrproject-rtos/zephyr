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

struct hts221_data hts221_driver;
static u8_t addr =
		HTS221_REG_DATA_START | HTS221_AUTOINCREMENT_ADDR;

static int hts221_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct hts221_data *drv_data = dev->driver_data;
	s32_t conv_val;
	int err = 0;

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
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		conv_val = (s32_t)(drv_data->h1_rh_x2 - drv_data->h0_rh_x2) *
			   (drv_data->rh_sample - drv_data->h0_t0_out) /
			   (drv_data->h1_t0_out - drv_data->h0_t0_out) +
			   drv_data->h0_rh_x2;

		/* convert humidity x2 to percent */
		val->val1 = conv_val / 2;
		val->val2 = (conv_val % 2) * 500000;
	} else {
		err = -ENOTSUP;
	}

	drv_data->transaction.op.async_cli = NULL;

	return err;
}

static int hts221_sample_fetch_async(struct device *dev,
					enum sensor_channel chan,
					struct async_client *cli)
{
	struct hts221_data *drv_data = dev->driver_data;
	static const struct i2c_msg fetch_data_msgs[] = {
		I2C_MSG_TXRX(&addr, 1, &hts221_driver.rh_sample, 4, 0)
	};

	if (atomic_cas((atomic_t *)&drv_data->transaction.op.async_cli, 0,
			(atomic_val_t)cli) == false) {
		return -EBUSY;
	}

	drv_data->transaction.msgs = fetch_data_msgs;
	drv_data->transaction.num_msgs = ARRAY_SIZE(fetch_data_msgs);

	return i2c_schedule(drv_data->i2c, &drv_data->transaction);
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

static void hts221_read_conversion_data(struct hts221_data *drv_data, u8_t *buf)
{
	drv_data->h0_rh_x2 = buf[0];
	drv_data->h1_rh_x2 = buf[1];
	drv_data->t0_degc_x8 = sys_le16_to_cpu(buf[2] | ((buf[5] & 0x3) << 8));
	drv_data->t1_degc_x8 = sys_le16_to_cpu(buf[3] | ((buf[5] & 0xC) << 6));
	drv_data->h0_t0_out = sys_le16_to_cpu(buf[6] | (buf[7] << 8));
	drv_data->h1_t0_out = sys_le16_to_cpu(buf[10] | (buf[11] << 8));
	drv_data->t0_out = sys_le16_to_cpu(buf[12] | (buf[13] << 8));
	drv_data->t1_out = sys_le16_to_cpu(buf[14] | (buf[15] << 8));
}

static const struct sensor_driver_api hts221_driver_api = {
#if CONFIG_HTS221_TRIGGER
	.trigger_set = hts221_trigger_set,
#endif
	.sample_fetch = hts221_sample_fetch,
	.sample_fetch_async = IS_ENABLED(CONFIG_I2C_USE_MNGR) ?
				hts221_sample_fetch_async : NULL,
	.channel_get = hts221_channel_get,
};

int hts221_init(struct device *dev)
{
	struct hts221_data *drv_data = dev->driver_data;
	u8_t id;
	u8_t buf[16];

	drv_data->i2c = device_get_binding(DT_INST_0_ST_HTS221_BUS_NAME);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			    DT_INST_0_ST_HTS221_BUS_NAME);
		return -EINVAL;
	}

	int err;
	struct async_client client;
	struct z_i2c_with_mngr_data data;
	static const u8_t who_am_i_addr = HTS221_REG_WHO_AM_I;
	static const u8_t ctrl1_addr = HTS221_REG_CTRL1;
	static const u8_t ctrl1_val = (CONFIG_HTS221_ODR << HTS221_ODR_SHIFT) |
					HTS221_BDU_BIT | HTS221_PD_BIT;
	static const u8_t calib_addr = HTS221_REG_CONVERSION_START |
			   		HTS221_AUTOINCREMENT_ADDR;

	const struct i2c_msg init_msgs[] = {
		I2C_MSG_TXRX(&who_am_i_addr, 1, &id, 1, 0),
		I2C_MSG_TXTX(&ctrl1_addr, 1, &ctrl1_val, 1, 0),
		I2C_MSG_TXRX(&calib_addr, 1, buf, 16, 3)
	};

	k_sem_init(&data.sem, 0, 1);
	async_client_init_callback(&client, z_i2c_transaction_callback, &data);
	drv_data->transaction.addr = DT_INST_0_ST_HTS221_BASE_ADDRESS;
	drv_data->transaction.op.async_cli = &client;
	drv_data->transaction.msgs = init_msgs;
	drv_data->transaction.num_msgs = ARRAY_SIZE(init_msgs);

	err = i2c_schedule(drv_data->i2c, &drv_data->transaction);
	if (err == 0) {
		k_sem_take(&data.sem, K_FOREVER);
		err = data.res;
	}

	if (err < 0) {
		return err;
	}

	if (id != HTS221_CHIP_ID) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	hts221_read_conversion_data(drv_data, buf);


#ifdef CONFIG_HTS221_TRIGGER
	if (hts221_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

DEVICE_AND_API_INIT(hts221, DT_INST_0_ST_HTS221_LABEL, hts221_init, &hts221_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &hts221_driver_api);
