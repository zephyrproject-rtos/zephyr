/*
 * Copyright (c) 2021 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(TI_HDC20XX, CONFIG_SENSOR_LOG_LEVEL);

/* Register addresses */
#define TI_HDC20XX_REG_TEMP		0x00
#define TI_HDC20XX_REG_HUMIDITY		0x02
#define TI_HDC20XX_REG_INT_EN		0x07
#define TI_HDC20XX_REG_CONFIG		0x0E
#define TI_HDC20XX_REG_MEAS_CFG		0x0F
#define TI_HDC20XX_REG_MANUFACTURER_ID	0xFC
#define TI_HDC20XX_REG_DEVICE_ID	0xFE

/* Register values */
#define TI_HDC20XX_MANUFACTURER_ID	0x5449
#define TI_HDC20XX_DEVICE_ID		0x07D0

/* Register bits */
#define TI_HDC20XX_BIT_INT_EN_DRDY_EN		0x80
#define TI_HDC20XX_BIT_CONFIG_SOFT_RES		0x80
#define TI_HDC20XX_BIT_CONFIG_DRDY_INT_EN	0x04

/* Reset time: not in the datasheet, but found by trial and error */
#define TI_HDC20XX_RESET_TIME		K_MSEC(1)

/* Conversion time for 14-bit resolution. Temperature needs 660us and humidity 610us */
#define TI_HDC20XX_CONVERSION_TIME      K_MSEC(2)

/* Temperature and humidity scale and factors from the datasheet ("Register Maps" section) */
#define TI_HDC20XX_RH_SCALE		100U
#define TI_HDC20XX_TEMP_OFFSET		-2654208	/* = -40.5 * 2^16 */
#define TI_HDC20XX_TEMP_SCALE		165U

struct ti_hdc20xx_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec gpio_int;
};

struct ti_hdc20xx_data {
	struct gpio_callback cb_int;
	struct k_sem sem_int;

	uint16_t t_sample;
	uint16_t rh_sample;
};

static void ti_hdc20xx_int_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct ti_hdc20xx_data *data = CONTAINER_OF(cb, struct ti_hdc20xx_data, cb_int);

	ARG_UNUSED(pins);

	k_sem_give(&data->sem_int);
}

static int ti_hdc20xx_sample_fetch(const struct device *dev,
				   enum sensor_channel chan)
{
	const struct ti_hdc20xx_config *config = dev->config;
	struct ti_hdc20xx_data *data = dev->data;
	uint16_t buf[2];
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* start conversion of both temperature and humidity with the default accuracy (14 bits) */
	rc = i2c_reg_write_byte_dt(&config->bus, TI_HDC20XX_REG_MEAS_CFG, 0x01);
	if (rc < 0) {
		LOG_ERR("Failed to write measurement configuration register");
		return rc;
	}

	/* wait for the conversion to finish */
	if (config->gpio_int.port) {
		k_sem_take(&data->sem_int, K_FOREVER);
	} else {
		k_sleep(TI_HDC20XX_CONVERSION_TIME);
	}

	/* temperature and humidity registers are consecutive, read them in the same burst */
	rc = i2c_burst_read_dt(&config->bus, TI_HDC20XX_REG_TEMP, (uint8_t *)buf, sizeof(buf));
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
		/* val = -40.5 + 165 * sample / 2^16 */
		tmp = data->t_sample * TI_HDC20XX_TEMP_SCALE + TI_HDC20XX_TEMP_OFFSET;
		val->val1 = tmp >> 16;
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
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api ti_hdc20xx_api_funcs = {
	.sample_fetch = ti_hdc20xx_sample_fetch,
	.channel_get = ti_hdc20xx_channel_get,
};

static int ti_hdc20xx_reset(const struct device *dev)
{
	const struct ti_hdc20xx_config *config = dev->config;
	int rc;

	rc = i2c_reg_write_byte_dt(&config->bus, TI_HDC20XX_REG_CONFIG,
				   TI_HDC20XX_BIT_CONFIG_SOFT_RES);
	if (rc < 0) {
		LOG_ERR("Failed to soft-reset device");
		return rc;
	}
	k_sleep(TI_HDC20XX_RESET_TIME);

	return 0;
}

static int ti_hdc20xx_init(const struct device *dev)
{
	const struct ti_hdc20xx_config *config = dev->config;
	struct ti_hdc20xx_data *data = dev->data;
	uint16_t buf[2];
	int rc;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	/* manufacturer and device ID registers are consecutive, read them in the same burst */
	rc = i2c_burst_read_dt(&config->bus, TI_HDC20XX_REG_MANUFACTURER_ID,
			       (uint8_t *)buf, sizeof(buf));
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

	/* Soft-reset the device to bring all registers in a known and consistent state */
	rc = ti_hdc20xx_reset(dev);
	if (rc < 0) {
		return rc;
	}

	/* Configure the interrupt GPIO if available */
	if (config->gpio_int.port) {
		if (!gpio_is_ready_dt(&config->gpio_int)) {
			LOG_ERR("Cannot get pointer to gpio interrupt device");
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
		if (rc) {
			LOG_ERR("Failed to configure interrupt pin");
			return rc;
		}

		gpio_init_callback(&data->cb_int, ti_hdc20xx_int_callback,
				   BIT(config->gpio_int.pin));

		rc = gpio_add_callback(config->gpio_int.port, &data->cb_int);
		if (rc) {
			LOG_ERR("Failed to set interrupt callback");
			return rc;
		}

		rc = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (rc) {
			LOG_ERR("Failed to configure interrupt");
			return rc;
		}

		/* Initialize the semaphore */
		k_sem_init(&data->sem_int, 0, K_SEM_MAX_LIMIT);

		/* Enable the data ready interrupt */
		rc = i2c_reg_write_byte_dt(&config->bus, TI_HDC20XX_REG_INT_EN,
					   TI_HDC20XX_BIT_INT_EN_DRDY_EN);
		if (rc) {
			LOG_ERR("Failed to enable the data ready interrupt");
			return rc;
		}

		/* Enable the interrupt pin with level sensitive active low polarity */
		rc = i2c_reg_write_byte_dt(&config->bus, TI_HDC20XX_REG_CONFIG,
					   TI_HDC20XX_BIT_CONFIG_DRDY_INT_EN);
		if (rc) {
			LOG_ERR("Failed to enable the interrupt pin");
			return rc;
		}
	}

	return 0;
}

/* Main instantiation macro */
#define TI_HDC20XX_DEFINE(inst, compat)							\
	static struct ti_hdc20xx_data ti_hdc20xx_data_##compat##inst;			\
	static const struct ti_hdc20xx_config ti_hdc20xx_config_##compat##inst = {	\
		.bus = I2C_DT_SPEC_GET(DT_INST(inst, compat)),				\
		.gpio_int = GPIO_DT_SPEC_GET_OR(DT_INST(inst, compat), int_gpios, {0}),	\
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
