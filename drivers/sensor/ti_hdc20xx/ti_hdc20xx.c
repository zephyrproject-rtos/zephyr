/*
 * Copyright (c) 2021 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(TI_HDC20XX, CONFIG_SENSOR_LOG_LEVEL);

/* Register addresses */
#define TI_HDC20XX_REG_TEMP		0x00
#define TI_HDC20XX_REG_HUMIDITY		0x02
#define TI_HDC20XX_REG_MEAS_CFG		0x0F
#define TI_HDC20XX_REG_MANUFACTURER_ID	0xFC
#define TI_HDC20XX_REG_DEVICE_ID	0xFE

/* Register values */
#define TI_HDC20XX_MANUFACTURER_ID	0x5449
#define TI_HDC20XX_DEVICE_ID		0x07D0

/* Conversion time for 14-bit resolution. Temperature needs 660us and humidity 610us */
#define TI_HDC20XX_CONVERSION_TIME      K_MSEC(2)

/* Temperature and humidity scale and factors from the datasheet ("Register Maps" section) */
#define TI_HDC20XX_RH_SCALE		100U
#define TI_HDC20XX_TEMP_OFFSET		-40U
#define TI_HDC20XX_TEMP_SCALE		165U

struct ti_hdc20xx_config {
	const struct device *bus;
	uint16_t i2c_addr;
};

struct ti_hdc20xx_data {
	int32_t t_sample;
	int32_t rh_sample;
};

static int ti_hdc20xx_sample_fetch(const struct device *dev,
				   enum sensor_channel chan)
{
	const struct ti_hdc20xx_config *config = dev->config;
	struct ti_hdc20xx_data *data = dev->data;
	uint16_t buf[2];
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* start conversion of both temperature and humidity with the default accuracy (14 bits) */
	rc = i2c_reg_write_byte(config->bus, config->i2c_addr, TI_HDC20XX_REG_MEAS_CFG, 0x01);
	if (rc < 0) {
		LOG_ERR("Failed to write measurement configuration register");
		return rc;
	}

	/* wait for the conversion to finish */
	k_sleep(TI_HDC20XX_CONVERSION_TIME);

	/* temperature and humidity registers are consecutive, read them in the same burst */
	rc = i2c_burst_read(config->bus, config->i2c_addr,
			    TI_HDC20XX_REG_TEMP, (uint8_t *)buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to read sample data");
		return rc;
	}

	data->t_sample = sys_le16_to_cpu(buf[0]);
	data->rh_sample = sys_le16_to_cpu(buf[1]);

	return 0;
}


static int ti_hdc20xx_channel_get(const struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct ti_hdc20xx_data *data = dev->data;
	int32_t tmp;

	/* See datasheet "Register Maps" section for more details on processing sample data. */
	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* val = -40 + 165 * sample / 2^16 */
		tmp = data->t_sample * TI_HDC20XX_TEMP_SCALE;
		val->val1 = TI_HDC20XX_TEMP_OFFSET + (tmp >> 16);
		/* x * 1000000 / 2^16 = x * 15625 / 2^10 */
		val->val2 = ((tmp & 0xFFFF) * 15625U) >> 10;
		break;
	case SENSOR_CHAN_HUMIDITY:
		/* val = 100 * sample / 2^16 */
		tmp = data->rh_sample * TI_HDC20XX_RH_SCALE;
		val->val1 = tmp >> 16;
		/* x * 1000000 / 2^16 = x * 15625 / 2^10 */
		val->val2 = ((tmp & 0xFFFF) * 15625U) >> 10;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api ti_hdc20xx_api_funcs = {
	.sample_fetch = ti_hdc20xx_sample_fetch,
	.channel_get = ti_hdc20xx_channel_get,
};

static int ti_hdc20xx_init(const struct device *dev)
{
	const struct ti_hdc20xx_config *config = dev->config;
	uint16_t buf[2];
	int rc;

	if (!device_is_ready(config->bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus->name);
		return -ENODEV;
	}

	/* manufacturer and device ID registers are consecutive, read them in the same burst */
	rc = i2c_burst_read(config->bus, config->i2c_addr,
			    TI_HDC20XX_REG_MANUFACTURER_ID, (uint8_t *)buf, sizeof(buf));
	if (rc < 0) {
		LOG_ERR("Failed to read manufacturer and device IDs");
		return rc;
	}

	if (sys_le16_to_cpu(buf[0]) != TI_HDC20XX_MANUFACTURER_ID) {
		LOG_ERR("Failed to get correct manufacturer ID");
		return -EINVAL;
	}
	if (sys_le16_to_cpu(buf[1]) != TI_HDC20XX_DEVICE_ID) {
		LOG_ERR("Unsupported device ID");
		return -EINVAL;
	}

	return 0;
}

/* Main instantiation macro */
#define TI_HDC20XX_DEFINE(inst, compat)							\
	static struct ti_hdc20xx_data ti_hdc20xx_data_##compat##inst;			\
	static const struct ti_hdc20xx_config ti_hdc20xx_config_##compat##inst = {	\
		.bus = DEVICE_DT_GET(DT_BUS(DT_INST(inst, compat))),			\
		.i2c_addr = DT_REG_ADDR(DT_INST(inst, compat))				\
	};										\
	DEVICE_DT_DEFINE(DT_INST(inst, compat),						\
			ti_hdc20xx_init,						\
			NULL,								\
			&ti_hdc20xx_data_##compat##inst,				\
			&ti_hdc20xx_config_##compat##inst,				\
			POST_KERNEL,							\
			CONFIG_SENSOR_INIT_PRIORITY,					\
			&ti_hdc20xx_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
#define TI_HDC20XX_FOREACH_STATUS_OKAY(compat, fn)	\
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),	\
		    (UTIL_CAT(DT_FOREACH_OKAY_INST_,	\
			      compat)(fn)),		\
		    ())

/*
 * HDC2010 Low-Power Humidity and Temperature Digital Sensors
 */
#define TI_HDC2010_DEFINE(inst) TI_HDC20XX_DEFINE(inst, ti_hdc2010)
TI_HDC20XX_FOREACH_STATUS_OKAY(ti_hdc2010, TI_HDC2010_DEFINE)

/*
 * HDC2021 High-Accuracy, Low-Power Humidity and Temperature Sensor
 * With Assembly Protection Cover
 */
#define TI_HDC2021_DEFINE(inst) TI_HDC20XX_DEFINE(inst, ti_hdc2021)
TI_HDC20XX_FOREACH_STATUS_OKAY(ti_hdc2021, TI_HDC2021_DEFINE)

/*
 * HDC2022 High-Accuracy, Low-Power Humidity and Temperature Sensor
 * With IP67 Rated Water and Dust Protection Cover
 */
#define TI_HDC2022_DEFINE(inst) TI_HDC20XX_DEFINE(inst, ti_hdc2022)
TI_HDC20XX_FOREACH_STATUS_OKAY(ti_hdc2022, TI_HDC2022_DEFINE)

/*
 * HDC2080 Low-Power Humidity and Temperature Digital Sensor
 */
#define TI_HDC2080_DEFINE(inst) TI_HDC20XX_DEFINE(inst, ti_hdc2080)
TI_HDC20XX_FOREACH_STATUS_OKAY(ti_hdc2080, TI_HDC2080_DEFINE)
