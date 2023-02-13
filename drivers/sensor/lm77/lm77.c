/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lm77

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(lm77, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Only compile in trigger support if enabled in Kconfig and at least one
 * enabled lm77 devicetree node has the int-gpios property.
 */
#define LM77_TRIGGER_SUPPORT_INST(inst) DT_INST_NODE_HAS_PROP(inst, int_gpios) ||
#define LM77_TRIGGER_SUPPORT \
	(CONFIG_LM77_TRIGGER && (DT_INST_FOREACH_STATUS_OKAY(LM77_TRIGGER_SUPPORT_INST) 0))

/* LM77 registers */
#define LM77_REG_TEMP   0x00U
#define LM77_REG_CONFIG 0x01U
#define LM77_REG_THYST  0x02U
#define LM77_REG_TCRIT  0x03U
#define LM77_REG_TLOW   0x04U
#define LM77_REG_THIGH  0x05U

/* LM77 configuration register bits */
union lm77_reg_config {
	uint8_t reg;
	struct {
		uint8_t shutdown : 1;
		uint8_t int_mode : 1;
		uint8_t tcrita_pol : 1;
		uint8_t int_pol : 1;
		uint8_t fault_queue : 1;
		uint8_t reserved : 3;
	} __packed;
};

struct lm77_config {
	struct i2c_dt_spec i2c;
	union lm77_reg_config config_dt;
#if LM77_TRIGGER_SUPPORT
	struct gpio_dt_spec int_gpio;
#endif /* LM77_TRIGGER_SUPPORT */
};

struct lm77_data {
	int16_t temp;
#if LM77_TRIGGER_SUPPORT
	const struct device *dev;
	struct k_work_q workq;
	struct k_work work;
	struct gpio_callback int_gpio_cb;
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t trigger_handler;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_LM77_TRIGGER_THREAD_STACK_SIZE);
#endif /* LM77_TRIGGER_SUPPORT */
};

static int lm77_write_config(const struct device *dev, uint8_t value)
{
	const struct lm77_config *config = dev->config;
	uint8_t buf[2] = { LM77_REG_CONFIG, value };

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static int lm77_read_temp(const struct device *dev, uint8_t reg, int16_t *value)
{
	const struct lm77_config *config = dev->config;
	uint8_t buf[2];
	int err;

	err = i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), &buf, sizeof(buf));
	if (err < 0) {
		LOG_ERR("failed to read temperature register 0x%02x (err %d)", reg, err);
		return err;
	}

	*value = sys_get_be16(buf);

	return 0;
}

static int lm77_write_temp(const struct device *dev, uint8_t reg, int16_t value)
{
	const struct lm77_config *config = dev->config;
	uint8_t buf[3] = { reg, value >> 8, value };

	return i2c_write_dt(&config->i2c, buf, sizeof(buf));
}

static void lm77_sensor_value_to_temp(const struct sensor_value *val, int16_t *temp)
{
	/* Integer part in degrees Celsius (LSB = 0.5 degrees Celsius) */
	*temp = val->val1 << 1;

	/* Fractional part in micro degrees Celsius */
	*temp += (val->val2 * 2) / 1000000;

	/* Shift up to include "status" bits */
	*temp <<= 3;
}

static void lm77_temp_to_sensor_value(int16_t temp, struct sensor_value *val)
{
	/* Shift down to remove "status" bits (LSB = 0.5 degrees Celsius) */
	temp = (temp >> 3) * 5;

	/* Integer part in degrees Celsius */
	val->val1 = temp / 10;

	/* Fractional part in micro degrees Celsius */
	val->val2 = temp % 10;
	val->val2 *= 100000;
}

static int lm77_attr_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	int16_t temp = 0;
	uint8_t reg;
	int err;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		reg = LM77_REG_TLOW;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		reg = LM77_REG_THIGH;
		break;
	case SENSOR_ATTR_ALERT:
		reg = LM77_REG_TCRIT;
		break;
	case SENSOR_ATTR_HYSTERESIS:
		reg = LM77_REG_THYST;
		break;
	default:
		return -ENOTSUP;
	}

	lm77_sensor_value_to_temp(val, &temp);

	err = lm77_write_temp(dev, reg, temp);
	if (err < 0) {
		LOG_ERR("failed to write register 0x%02x (err %d)", reg, err);
		return err;
	}

	return 0;
}

static int lm77_attr_get(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, struct sensor_value *val)
{
	int16_t temp;
	uint8_t reg;
	int err;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		reg = LM77_REG_TLOW;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		reg = LM77_REG_THIGH;
		break;
	case SENSOR_ATTR_ALERT:
		reg = LM77_REG_TCRIT;
		break;
	case SENSOR_ATTR_HYSTERESIS:
		reg = LM77_REG_THYST;
		break;
	default:
		return -ENOTSUP;
	}

	err = lm77_read_temp(dev, reg, &temp);
	if (err < 0) {
		LOG_ERR("failed to read register 0x%02x (err %d)", reg, err);
		return err;
	}

	lm77_temp_to_sensor_value(temp, val);

	return 0;
}

#if LM77_TRIGGER_SUPPORT
static int lm77_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler)
{
	const struct lm77_config *config = dev->config;
	struct lm77_data *data = dev->data;
	gpio_flags_t flags;
	int err;

	__ASSERT_NO_MSG(trig != NULL);

	if (trig->type != SENSOR_TRIG_THRESHOLD || trig->chan != SENSOR_CHAN_AMBIENT_TEMP ||
	    config->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	if (handler != NULL) {
		flags = GPIO_INT_EDGE_TO_ACTIVE;
	} else {
		flags = GPIO_INT_DISABLE;
	}

	err = gpio_pin_interrupt_configure_dt(&config->int_gpio, flags);
	if (err < 0) {
		LOG_ERR("failed to configure INT GPIO IRQ (err %d)", err);
		return err;
	}

	data->trigger = trig;
	data->trigger_handler = handler;

	return 0;
}

static void lm77_trigger_work_handler(struct k_work *item)
{
	struct lm77_data *data = CONTAINER_OF(item, struct lm77_data, work);
	sensor_trigger_handler_t handler = data->trigger_handler;

	if (handler != NULL) {
		handler(data->dev, (struct sensor_trigger *)data->trigger);
	}
}

static void lm77_int_gpio_callback_handler(const struct device *port,
					   struct gpio_callback *cb,
					   gpio_port_pins_t pins)
{
	struct lm77_data *data = CONTAINER_OF(cb, struct lm77_data, int_gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit_to_queue(&data->workq, &data->work);
}
#endif /* LM77_TRIGGER_SUPPORT */

static int lm77_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lm77_data *data = dev->data;
	int16_t temp;
	int err;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	err = lm77_read_temp(dev, LM77_REG_TEMP, &temp);
	if (err < 0) {
		LOG_ERR("failed to read temperature (err %d)", err);
		return err;
	}

	data->temp = temp;

	return 0;
}

static int lm77_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	struct lm77_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	lm77_temp_to_sensor_value(data->temp, val);

	return 0;
}

static const struct sensor_driver_api lm77_driver_api = {
	.attr_set = lm77_attr_set,
	.attr_get = lm77_attr_get,
#if LM77_TRIGGER_SUPPORT
	.trigger_set = lm77_trigger_set,
#endif /* LM77_TRIGGER_SUPPORT */
	.sample_fetch = lm77_sample_fetch,
	.channel_get = lm77_channel_get,
};

static int lm77_init(const struct device *dev)
{
	const struct lm77_config *config = dev->config;
	int err;
#if LM77_TRIGGER_SUPPORT
	struct lm77_data *data = dev->data;
#endif /* LM77_TRIGGER_SUPPORT */

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2c bus not ready");
		return -EINVAL;
	}

	err = lm77_write_config(dev, config->config_dt.reg);
	if (err < 0) {
		LOG_ERR("failed to write configuration (err %d)", err);
		return err;
	}

#if LM77_TRIGGER_SUPPORT
	data->dev = dev;
	k_work_queue_start(&data->workq, data->stack, K_THREAD_STACK_SIZEOF(data->stack),
			   CONFIG_LM77_TRIGGER_THREAD_PRIO, NULL);
	k_thread_name_set(&data->workq.thread, "lm77_trigger");
	k_work_init(&data->work, lm77_trigger_work_handler);

	if (config->int_gpio.port != NULL) {
		if (!device_is_ready(config->int_gpio.port)) {
			LOG_ERR("INT GPIO not ready");
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("failed to configure INT GPIO (err %d)", err);
			return err;
		}

		gpio_init_callback(&data->int_gpio_cb, lm77_int_gpio_callback_handler,
				   BIT(config->int_gpio.pin));

		err = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
		if (err < 0) {
			LOG_ERR("failed to add INT GPIO callback (err %d)", err);
			return err;
		}
	}
#endif /* LM77_TRIGGER_SUPPORT */

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lm77_pm_action(const struct device *dev,
			  enum pm_device_action action)
{
	const struct lm77_config *config = dev->config;
	union lm77_reg_config creg = config->config_dt;
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		creg.shutdown = 1;
		break;
	case PM_DEVICE_ACTION_RESUME:
		creg.shutdown = 0;
		break;
	default:
		return -ENOTSUP;
	}

	err = lm77_write_config(dev, creg.reg);
	if (err < 0) {
		LOG_ERR("failed to write configuration (err %d)", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#if LM77_TRIGGER_SUPPORT
#define LM77_INT_GPIO_INIT(n) .int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, { 0 })
#else /* LM77_TRIGGER_SUPPORT */
#define LM77_INT_GPIO_INIT(n)
#endif /* ! LM77_TRIGGER_SUPPORT */

#define LM77_INIT(n)							\
	static struct lm77_data lm77_data_##n;				\
									\
	static const struct lm77_config lm77_config_##n = {		\
		.i2c = I2C_DT_SPEC_INST_GET(n),				\
		.config_dt = {						\
			.shutdown = 0,					\
			.int_mode = DT_INST_NODE_HAS_PROP(n, int_gpios), \
			.tcrita_pol = DT_INST_PROP(n, tcrita_inverted), \
			.int_pol = DT_INST_PROP(n, int_inverted),	\
			.fault_queue = DT_INST_PROP(n, enable_fault_queue), \
			.reserved = 0,					\
		},							\
		LM77_INT_GPIO_INIT(n)					\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(n, lm77_pm_action);			\
									\
	SENSOR_DEVICE_DT_INST_DEFINE(n, lm77_init,			\
			      PM_DEVICE_DT_INST_GET(n),			\
			      &lm77_data_##n,				\
			      &lm77_config_##n, POST_KERNEL,		\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &lm77_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LM77_INIT)
