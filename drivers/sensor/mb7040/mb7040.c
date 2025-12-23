/*
 * Copyright (c) 2025 Sabrina Simkhovich <sabrinasimkhovich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxbotix_mb7040

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#define RANGE_CMD 0x51

#define MB7040_HAS_STATUS_GPIO DT_ANY_INST_HAS_PROP_STATUS_OKAY(status_gpios)

LOG_MODULE_REGISTER(mb7040, CONFIG_SENSOR_LOG_LEVEL);

struct mb7040_data {
	uint16_t distance_cm;
	struct k_sem read_sem;
#if MB7040_HAS_STATUS_GPIO
	struct gpio_callback gpio_cb;
#endif
};

struct mb7040_config {
	struct i2c_dt_spec i2c;
	uint8_t i2c_addr;
#if MB7040_HAS_STATUS_GPIO
	struct gpio_dt_spec status_gpio;
#endif
};

#if MB7040_HAS_STATUS_GPIO
static void status_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct mb7040_data *data = CONTAINER_OF(cb, struct mb7040_data, gpio_cb);

	k_sem_give(&data->read_sem);
}
#endif

static int mb7040_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct mb7040_config *cfg = (struct mb7040_config *)dev->config;
	struct mb7040_data *data = (struct mb7040_data *)dev->data;
	uint8_t cmd = RANGE_CMD;
	int ret;
	uint8_t read_data[2];

	if (chan != SENSOR_CHAN_DISTANCE && chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Sensor only supports distance");
		return -EINVAL;
	}

	k_sem_reset(&data->read_sem);

#if MB7040_HAS_STATUS_GPIO
	/* Check if status_gpio port is present */
	if (cfg->status_gpio.port != NULL) {
		/* Enable interrupt before writing */
		ret = gpio_pin_interrupt_configure_dt(&cfg->status_gpio, GPIO_INT_EDGE_FALLING);
		if (ret != 0) {
			LOG_ERR("Failed to configure interrupt: %d", ret);
			return ret;
		}
	}
#endif

	/* Write range command to sensor */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, cfg->i2c_addr, cmd);

	if (ret != 0) {
		LOG_ERR("I2C write failed with error %d", ret);
#if MB7040_HAS_STATUS_GPIO
		/* Disable interrupt before returning error*/
		if (cfg->status_gpio.port != NULL) {
			gpio_pin_interrupt_configure_dt(&cfg->status_gpio, GPIO_INT_DISABLE);
		}
#endif
		return ret;
	}

	ret = k_sem_take(&data->read_sem, K_MSEC(CONFIG_MB7040_DELAY_MS));

#if MB7040_HAS_STATUS_GPIO
	/* Check if status_gpio port is present */
	if (cfg->status_gpio.port != NULL) {
		gpio_pin_interrupt_configure_dt(&cfg->status_gpio, GPIO_INT_DISABLE);
		if (ret != 0) {
			/* Success: interrupt fired and semaphore taken */
			return ret;
		}
	} else {
		if (ret != -EAGAIN) {
			/* Should not happen: semaphore only given by GPIO interrupt */
			return ret;
		}
	}
#else
	if (ret != -EAGAIN) {
		/* Should not happen: semaphore only given by GPIO interrupt */
		return ret;
	}
#endif
	/*
	 * Small wait due to device specific internal i2c timings. 10ms is a common
	 * wait to time to ensure ultrasonic sensors achieve stability and accuracy.
	 */
	k_msleep(10);

	ret = i2c_read_dt(&cfg->i2c, read_data, 2);

	if (ret != 0) {
		LOG_ERR("I2C read failed with error %d", ret);
#if MB7040_HAS_STATUS_GPIO
		if (cfg->status_gpio.port != NULL) {
			gpio_pin_interrupt_configure_dt(&cfg->status_gpio, GPIO_INT_DISABLE);
		}
#endif
		return ret;
	}

	/* Convert MSB/LSB to distance in cm */
	data->distance_cm = (read_data[0] << 8) | read_data[1];
	return 0;
}

static int mb7040_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct mb7040_data *data = (struct mb7040_data *)dev->data;

	if (chan != SENSOR_CHAN_DISTANCE) {
		LOG_ERR("Sensor only supports distance");
		return -ENOTSUP;
	}

	/* Meters */
	val->val1 = data->distance_cm / 100;
	/* Micrometers */
	val->val2 = (data->distance_cm % 100) * 10000;

	return 0;
}

static DEVICE_API(sensor, mb7040_api) = {
	.sample_fetch = mb7040_sample_fetch,
	.channel_get = mb7040_channel_get,
};

static int mb7040_init(const struct device *dev)
{
	const struct mb7040_config *cfg = (struct mb7040_config *)dev->config;
	struct mb7040_data *data = (struct mb7040_data *)dev->data;

	k_sem_init(&data->read_sem, 0, 1);

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C not ready!");
		return -ENODEV;
	}
	/* Initialize status GPIO if present */
#if MB7040_HAS_STATUS_GPIO
	if (cfg->status_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->status_gpio)) {
			LOG_ERR("Status GPIO not ready");
			return -ENODEV;
		}

		int ret = gpio_pin_configure_dt(&cfg->status_gpio, GPIO_INPUT);

		if (ret < 0) {
			LOG_ERR("Failed to configure status GPIO: %d", ret);
			return ret;
		}

		gpio_init_callback(&data->gpio_cb, status_gpio_callback, BIT(cfg->status_gpio.pin));
		ret = gpio_add_callback(cfg->status_gpio.port, &data->gpio_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add GPIO callback: %d", ret);
			return ret;
		}
		LOG_INF("MB7040 initialized with status GPIO");
	}
#else
	LOG_INF("MB7040 initialized");
#endif
	return 0;
}

#define MB7040_DEFINE(inst)                                                                        \
	static struct mb7040_data mb7040_data_##inst = {                                           \
		.distance_cm = 0,                                                                  \
	};                                                                                         \
	static const struct mb7040_config mb7040_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.i2c_addr = DT_INST_REG_ADDR(inst),                                                \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, status_gpios),                              \
		(.status_gpio = GPIO_DT_SPEC_INST_GET(inst, status_gpios),)) };                    \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mb7040_init, NULL, &mb7040_data_##inst,                 \
				     &mb7040_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &mb7040_api);

DT_INST_FOREACH_STATUS_OKAY(MB7040_DEFINE)
