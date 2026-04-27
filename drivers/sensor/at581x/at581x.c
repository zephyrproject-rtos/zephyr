/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT andar_at581x

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(at581x, CONFIG_SENSOR_LOG_LEVEL);

/* Register Definitions */
#define AT581X_REG_RESET        0x00
#define AT581X_REG_THRESHOLD_L  0x10
#define AT581X_REG_THRESHOLD_H  0x11
#define AT581X_REG_WIN_LEN      0x31
#define AT581X_REG_WIN_THR      0x32
#define AT581X_REG_SELF_CK_L    0x38
#define AT581X_REG_SELF_CK_H    0x39
#define AT581X_REG_BASE_TIME_L  0x3D
#define AT581X_REG_BASE_TIME_H  0x3E
#define AT581X_REG_OUT_CTRL     0x41
#define AT581X_REG_KEEP_TIME_L  0x42
#define AT581X_REG_KEEP_TIME_H  0x43
#define AT581X_REG_PROT_TIME_L  0x4E
#define AT581X_REG_PROT_TIME_H  0x4F
#define AT581X_REG_RF_CTRL      0x51
#define AT581X_REG_MAGIC        0x55
#define AT581X_REG_GAIN         0x5C

struct at581x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
	uint16_t threshold;
	uint8_t gain;
	uint16_t base_time_ms;
	uint16_t keep_time_ms;
};

struct at581x_data {
	uint16_t detection_value;

#ifdef CONFIG_AT581X_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t motion_handler;
	const struct sensor_trigger *motion_trigger;

#if defined(CONFIG_AT581X_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_AT581X_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_AT581X_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_AT581X_TRIGGER */
};

static int at581x_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct at581x_config *cfg = dev->config;
	uint8_t buf[2] = {reg, val};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}


#ifdef CONFIG_AT581X_TRIGGER
static void at581x_thread_cb(const struct device *dev)
{
	struct at581x_data *data = dev->data;

	if (data->motion_handler != NULL) {
		data->motion_handler(dev, data->motion_trigger);
	}
}

static void at581x_gpio_callback(const struct device *port,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct at581x_data *data = CONTAINER_OF(cb, struct at581x_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

#if defined(CONFIG_AT581X_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_AT581X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_AT581X_TRIGGER_OWN_THREAD)
static void at581x_thread(void *arg1, void *unused2, void *unused3)
{
	struct at581x_data *data = arg1;

	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		at581x_thread_cb(data->dev);
	}
}
#elif defined(CONFIG_AT581X_TRIGGER_GLOBAL_THREAD)
static void at581x_work_cb(struct k_work *work)
{
	struct at581x_data *data = CONTAINER_OF(work, struct at581x_data, work);

	at581x_thread_cb(data->dev);
}
#endif

static int at581x_trigger_set(const struct device *dev,
			      const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
	const struct at581x_config *cfg = dev->config;
	struct at581x_data *data = dev->data;
	int ret;

	if (trig->type != SENSOR_TRIG_MOTION || trig->chan != SENSOR_CHAN_PROX) {
		return -ENOTSUP;
	}

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	data->motion_handler = handler;
	data->motion_trigger = trig;

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					      handler ? GPIO_INT_EDGE_BOTH
						      : GPIO_INT_DISABLE);
	if (ret < 0) {
		return ret;
	}

	if (handler) {
		ret = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	} else {
		ret = gpio_remove_callback(cfg->int_gpio.port, &data->gpio_cb);
	}

	return ret;
}
#endif /* CONFIG_AT581X_TRIGGER */

static int at581x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct at581x_config *cfg = dev->config;
	struct at581x_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PROX) {
		return -ENOTSUP;
	}

	data->detection_value = gpio_pin_get_dt(&cfg->int_gpio);

	return 0;
}

static int at581x_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct at581x_data *data = dev->data;

	if (chan != SENSOR_CHAN_PROX) {
		return -ENOTSUP;
	}

	val->val1 = data->detection_value;
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, at581x_driver_api) = {
	.sample_fetch = at581x_sample_fetch,
	.channel_get = at581x_channel_get,
#ifdef CONFIG_AT581X_TRIGGER
	.trigger_set = at581x_trigger_set,
#endif
};

static int at581x_init(const struct device *dev)
{
	const struct at581x_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	/* Factory Demo Setup Sequence */
	ret = at581x_reg_write(dev, 0x68, 0x68);
	if (ret < 0) {
		LOG_ERR("Failed to write factory setup register");
		return ret;
	}

	ret = at581x_reg_write(dev, 0x67, 0x8B);
	if (ret < 0) {
		LOG_ERR("Failed to write factory setup register");
		return ret;
	}

	/* Set Detection Threshold */
	ret = at581x_reg_write(dev, AT581X_REG_THRESHOLD_L, cfg->threshold & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write threshold low register");
		return ret;
	}

	ret = at581x_reg_write(dev, AT581X_REG_THRESHOLD_H, (cfg->threshold >> 8) & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write threshold high register");
		return ret;
	}

	/* Set Gain */
	ret = at581x_reg_write(dev, AT581X_REG_GAIN, cfg->gain);
	if (ret < 0) {
		LOG_ERR("Failed to write gain register");
		return ret;
	}

	/* Set Base Time */
	ret = at581x_reg_write(dev, AT581X_REG_BASE_TIME_L, cfg->base_time_ms & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write base time low register");
		return ret;
	}

	ret = at581x_reg_write(dev, AT581X_REG_BASE_TIME_H, (cfg->base_time_ms >> 8) & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write base time high register");
		return ret;
	}

	/* Set Output Control */
	ret = at581x_reg_write(dev, AT581X_REG_OUT_CTRL, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to write output control register");
		return ret;
	}

	/* Set Keep Time */
	ret = at581x_reg_write(dev, AT581X_REG_KEEP_TIME_L, cfg->keep_time_ms & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write keep time low register");
		return ret;
	}

	ret = at581x_reg_write(dev, AT581X_REG_KEEP_TIME_H, (cfg->keep_time_ms >> 8) & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write keep time high register");
		return ret;
	}

	/* Magic Register (Required for stability) */
	ret = at581x_reg_write(dev, AT581X_REG_MAGIC, 0x04);
	if (ret < 0) {
		LOG_ERR("Failed to write magic register");
		return ret;
	}

	/* Soft Reset to apply settings */
	ret = at581x_reg_write(dev, AT581X_REG_RESET, 0x00);
	if (ret < 0) {
		LOG_ERR("Failed to reset device");
		return ret;
	}

	k_msleep(10);

	ret = at581x_reg_write(dev, AT581X_REG_RESET, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to complete reset sequence");
		return ret;
	}

#ifdef CONFIG_AT581X_TRIGGER
	if (cfg->int_gpio.port) {
		struct at581x_data *data = dev->data;

		if (!gpio_is_ready_dt(&cfg->int_gpio)) {
			LOG_ERR("Trigger GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure trigger GPIO: %d", ret);
			return ret;
		}

		data->dev = dev;

		gpio_init_callback(&data->gpio_cb, at581x_gpio_callback,
				   BIT(cfg->int_gpio.pin));

		ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
		if (ret < 0) {
			LOG_ERR("Failed to configure trigger GPIO interrupt: %d", ret);
			return ret;
		}

#if defined(CONFIG_AT581X_TRIGGER_OWN_THREAD)
		k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);

		k_thread_create(&data->thread, data->thread_stack,
				CONFIG_AT581X_THREAD_STACK_SIZE,
				at581x_thread, data, NULL, NULL,
				K_PRIO_COOP(CONFIG_AT581X_THREAD_PRIORITY),
				0, K_NO_WAIT);
#elif defined(CONFIG_AT581X_TRIGGER_GLOBAL_THREAD)
		data->work.handler = at581x_work_cb;
#endif
	}
#endif /* CONFIG_AT581X_TRIGGER */

	LOG_INF("AT581X radar sensor initialized");

	return 0;
}

#define AT581X_INT_GPIO_INIT(inst) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int_gpios), \
		(.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),), \
		())

#define AT581X_INIT(inst)							\
	static struct at581x_data at581x_data_##inst;				\
										\
	static const struct at581x_config at581x_config_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
		AT581X_INT_GPIO_INIT(inst)					\
		.threshold = DT_INST_PROP_OR(inst, threshold, 200),		\
		.gain = DT_INST_PROP_OR(inst, gain, 0x1B),			\
		.base_time_ms = DT_INST_PROP_OR(inst, base_time_ms, 500),	\
		.keep_time_ms = DT_INST_PROP_OR(inst, keep_time_ms, 1500),	\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, at581x_init, NULL,			\
				      &at581x_data_##inst,			\
				      &at581x_config_##inst, POST_KERNEL,	\
				      CONFIG_SENSOR_INIT_PRIORITY,		\
				      &at581x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AT581X_INIT)
