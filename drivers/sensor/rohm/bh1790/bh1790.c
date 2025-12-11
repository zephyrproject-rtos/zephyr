/*
 * Copyright (c) 2025, Magpie Embedded
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bh1790

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(BH1790, CONFIG_SENSOR_LOG_LEVEL);

/* Register addresses as in the datasheet */
#define BH1790_REG_ADDR_MANUFACTURER_ID  (0x0F)
#define BH1790_REG_ADDR_PART_ID          (0x10)
#define BH1790_REG_ADDR_MEASURE_CONTROL1 (0x41)
#define BH1790_REG_ADDR_MEASURE_CONTROL2 (0x42)
#define BH1790_REG_ADDR_MEASURE_START    (0x43)
#define BH1790_REG_ADDR_DATAOUT          (0x54)

/* ID values as in the datasheet */
#define BH1790_MANUFACTURER_ID (0xE0)
#define BH1790_PART_ID         (0x0D)

/* Constant value to write to registers as in the datasheet */
#define BH1790_MEASURE_CONTROL_1_DEFAULT_VALUE (0x82)
#define BH1790_MEASURE_CONTROL_2_DEFAULT_VALUE (0x0C)
#define BH1790_MEASURE_START                   (0x01)

struct bh1790_dev_config {
	struct i2c_dt_spec bus;
};

struct bh1790_data {
	uint16_t led_off_data;
	uint16_t led_on_data;
};

static int bh1790_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct bh1790_dev_config *cfg = dev->config;
	struct bh1790_data *drv_data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_GREEN && chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	uint8_t read_buffer[4];

	const uint8_t value_read_start_address = BH1790_REG_ADDR_DATAOUT;

	int res = i2c_write_read_dt(&cfg->bus, &value_read_start_address, 1U, read_buffer, 4U);

	if (res < 0) {
		LOG_ERR("I2C error: %d", res);
		return -EIO;
	}

	drv_data->led_off_data = sys_get_le16(&read_buffer[0]);
	drv_data->led_on_data = sys_get_le16(&read_buffer[2]);

	return 0;
}

static int bh1790_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bh1790_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_GREEN:
		val->val1 = drv_data->led_on_data;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_LIGHT:
		val->val1 = drv_data->led_off_data;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bh1790_init(const struct device *dev)
{
	const struct bh1790_dev_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	/* Check Manufacturer ID and Part ID as specified in the Register Map of the datasheet */
	uint8_t manufacturer_id_byte = 0x00;

	if (i2c_reg_read_byte_dt(&cfg->bus, BH1790_REG_ADDR_MANUFACTURER_ID,
				 &manufacturer_id_byte)) {
		LOG_ERR("Could not read manufacturer id");
		return -EIO;
	}

	if (manufacturer_id_byte != BH1790_MANUFACTURER_ID) {
		LOG_ERR("Incorrect manufacturer id (%d)", manufacturer_id_byte);
		return -EIO;
	}

	uint8_t part_id_byte = 0x00;

	if (i2c_reg_read_byte_dt(&cfg->bus, BH1790_REG_ADDR_PART_ID, &part_id_byte)) {
		LOG_ERR("Could not read part id");
		return -EIO;
	}

	if (part_id_byte != BH1790_PART_ID) {
		LOG_ERR("Incorrect part id (%d)", part_id_byte);
		return -EIO;
	}

	/* Set control registers to perform measurements with default values */
	if (i2c_reg_write_byte_dt(&cfg->bus, BH1790_REG_ADDR_MEASURE_CONTROL1,
				  BH1790_MEASURE_CONTROL_1_DEFAULT_VALUE)) {
		LOG_ERR("Could not write to MEAS_CONTROL1");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&cfg->bus, BH1790_REG_ADDR_MEASURE_CONTROL2,
				  BH1790_MEASURE_CONTROL_2_DEFAULT_VALUE)) {
		LOG_ERR("Could not write to MEAS_CONTROL2");
		return -EIO;
	}

	/* Trigger first measurement, additional reads triggered by sensor fetch
	 * See Measurement Sequence in the datasheet, page 9
	 */
	if (i2c_reg_write_byte_dt(&cfg->bus, BH1790_REG_ADDR_MEASURE_START, BH1790_MEASURE_START)) {
		LOG_ERR("Could not start initial measurement");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(sensor, bh1790_driver_api) = {.sample_fetch = bh1790_sample_fetch,
						.channel_get = bh1790_channel_get};

#define DEFINE_BH1790(_num)                                                                        \
	static struct bh1790_data bh1790_data_##_num;                                              \
	static const struct bh1790_dev_config bh1790_config_##_num = {                             \
		.bus = I2C_DT_SPEC_INST_GET(_num)};                                                \
	SENSOR_DEVICE_DT_INST_DEFINE(_num, bh1790_init, NULL, &bh1790_data_##_num,                 \
				     &bh1790_config_##_num, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &bh1790_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BH1790)
