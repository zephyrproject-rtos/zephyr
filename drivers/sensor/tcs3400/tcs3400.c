/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_tcs3400

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tcs3400.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tcs3400, CONFIG_SENSOR_LOG_LEVEL);

#define TCS3400_ENABLE_REG 0x80
#define TCS3400_ENABLE_AIEN BIT(4)
#define TCS3400_ENABLE_AEN BIT(1)
#define TCS3400_ENABLE_PON BIT(0)

#define TCS3400_ATIME_REG 0x81

#define TCS3400_PERS_REG 0x8c

#define TCS3400_CONFIG_REG 0x8d

#define TCS3400_CONTROL_REG 0x8f

#define TCS3400_ID_REG 0x92
#define TCS3400_ID_1 0x90
#define TCS3400_ID_2 0x93

#define TCS3400_STATUS_REG 0x93
#define TCS3400_STATUS_AVALID BIT(0)

#define TCS3400_CDATAL_REG 0x94
#define TCS3400_CDATAH_REG 0x95
#define TCS3400_RDATAL_REG 0x96
#define TCS3400_RDATAH_REG 0x97
#define TCS3400_GDATAL_REG 0x98
#define TCS3400_GDATAH_REG 0x99
#define TCS3400_BDATAL_REG 0x9A
#define TCS3400_BDATAH_REG 0x9B

#define TCS3400_AICLEAR_REG 0xe7

/* Default values */
#define TCS3400_DEFAULT_ENABLE 0x00
#define TCS3400_DEFAULT_ATIME 0xff
#define TCS3400_DEFAULT_PERS 0x00
#define TCS3400_DEFAULT_CONFIG 0x00
#define TCS3400_DEFAULT_CONTROL 0x00
#define TCS3400_AICLEAR_RESET 0x00

struct tcs3400_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
};

struct tcs3400_data {
	struct gpio_callback gpio_cb;
	const struct device *dev;

	uint16_t sample_crgb[4];

	struct k_sem data_sem;
};

static void tcs3400_setup_int(const struct tcs3400_config *config, bool enable)
{
	unsigned int flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, flags);
}

static void tcs3400_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct tcs3400_data *data = CONTAINER_OF(cb, struct tcs3400_data,
						 gpio_cb);

	tcs3400_setup_int(data->dev->config, false);

	k_sem_give(&data->data_sem);
}

static int tcs3400_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	const struct tcs3400_config *cfg = dev->config;
	struct tcs3400_data *data = dev->data;
	int ret;
	uint8_t status;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	tcs3400_setup_int(cfg, true);

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TCS3400_ENABLE_REG,
				    TCS3400_ENABLE_AIEN | TCS3400_ENABLE_AEN |
				    TCS3400_ENABLE_PON);
	if (ret) {
		return ret;
	}

	k_sem_take(&data->data_sem, K_FOREVER);

	ret = i2c_reg_read_byte_dt(&cfg->i2c, TCS3400_STATUS_REG, &status);
	if (ret) {
		return ret;
	}

	if (status & TCS3400_STATUS_AVALID) {
		ret = i2c_burst_read_dt(&cfg->i2c, TCS3400_CDATAL_REG,
					(uint8_t *)&data->sample_crgb,
					sizeof(data->sample_crgb));
		if (ret) {
			return ret;
		}
	} else {
		LOG_ERR("Unexpected status: %02x", status);
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TCS3400_ENABLE_REG, 0);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TCS3400_AICLEAR_REG, 0);
	if (ret) {
		return ret;
	}

	return 0;
}

static int tcs3400_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tcs3400_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[0]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_RED:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[1]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[2]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[3]);
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int tcs3400_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	const struct tcs3400_config *cfg = dev->config;
	int ret;
	uint8_t reg_val;

	switch (attr) {
	case SENSOR_ATTR_TCS3400_INTEGRATION_CYCLES:
		if (!IN_RANGE(val->val1, 1, 256)) {
			return -EINVAL;
		}
		reg_val = UINT8_MAX - val->val1 + 1;
		ret = i2c_reg_write_byte_dt(&cfg->i2c,
					    TCS3400_ATIME_REG, reg_val);
		if (ret) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int tcs3400_sensor_setup(const struct device *dev)
{
	const struct tcs3400_config *cfg = dev->config;
	uint8_t chip_id;
	int ret;
	struct {
		uint8_t reg_addr;
		uint8_t value;
	} reset_regs[] = {
		{TCS3400_ENABLE_REG, TCS3400_DEFAULT_ENABLE},
		{TCS3400_AICLEAR_REG, TCS3400_AICLEAR_RESET},
		{TCS3400_ATIME_REG, TCS3400_DEFAULT_ATIME},
		{TCS3400_PERS_REG, TCS3400_DEFAULT_PERS},
		{TCS3400_CONFIG_REG, TCS3400_DEFAULT_CONFIG},
		{TCS3400_CONTROL_REG, TCS3400_DEFAULT_CONTROL},
	};

	ret = i2c_reg_read_byte_dt(&cfg->i2c, TCS3400_ID_REG, &chip_id);
	if (ret) {
		LOG_DBG("Failed to read chip id: %d", ret);
		return ret;
	}

	if (!((chip_id == TCS3400_ID_1) || (chip_id == TCS3400_ID_2))) {
		LOG_DBG("Invalid chip id: %02x", chip_id);
		return -EIO;
	}

	LOG_INF("chip id: 0x%x", chip_id);

	for (size_t i = 0; i < ARRAY_SIZE(reset_regs); i++) {
		ret = i2c_reg_write_byte_dt(&cfg->i2c, reset_regs[i].reg_addr,
					    reset_regs[i].value);
		if (ret) {
			LOG_ERR("Failed to set default register: %02x",
				reset_regs[i].reg_addr);
			return ret;
		}
	}

	return 0;
}

static const struct sensor_driver_api tcs3400_api = {
	.sample_fetch = tcs3400_sample_fetch,
	.channel_get = tcs3400_channel_get,
	.attr_set = tcs3400_attr_set,
};

static int tcs3400_init(const struct device *dev)
{
	const struct tcs3400_config *cfg = dev->config;
	struct tcs3400_data *data = dev->data;
	int ret;

	k_sem_init(&data->data_sem, 0, K_SEM_MAX_LIMIT);
	data->dev = dev;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	ret = tcs3400_sensor_setup(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup device");
		return ret;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("Interrupt GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt pin");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, tcs3400_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	ret = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set GPIO callback");
		return ret;
	}

	return 0;
}

#define TCS3400_INIT(n) \
	static struct tcs3400_data tcs3400_data_##n; \
	static const struct tcs3400_config tcs3400_config_##n = { \
		.i2c = I2C_DT_SPEC_INST_GET(n), \
		.int_gpio = GPIO_DT_SPEC_INST_GET(n, int_gpios), \
	}; \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &tcs3400_init, NULL, \
				     &tcs3400_data_##n, &tcs3400_config_##n, \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				     &tcs3400_api);

DT_INST_FOREACH_STATUS_OKAY(TCS3400_INIT)
