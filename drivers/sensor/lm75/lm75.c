/*
 * Copyright (c) 2021 Leica Geosystems AG
 * Copyright (c) 2023 Centralp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lm75

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(LM75, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Only compile in trigger support if enabled in Kconfig and at least one
 * enabled lm75 devicetree node has the int-gpios property.
 */
#define LM75_TRIGGER_SUPPORT \
	(CONFIG_LM75_TRIGGER && DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios))

/* LM75 registers */
#define LM75_REG_TEMP	0x00
#define LM75_REG_CONFIG 0x01
#define LM75_REG_T_HYST 0x02
#define LM75_REG_T_OS	0x03

struct lm75_data {
	int16_t temp;
#if LM75_TRIGGER_SUPPORT
	const struct device *dev;
	struct k_work_q workq;
	struct k_work work;
	struct gpio_callback int_gpio_cb;
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t trigger_handler;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_LM75_TRIGGER_THREAD_STACK_SIZE);
#endif /* LM75_TRIGGER_SUPPORT */
};

/* LM75 configuration register bits */
union lm75_reg_config {
	uint8_t reg;
	struct {
		uint8_t shutdown: 1;
		uint8_t int_mode: 1;
		uint8_t int_pol: 1;
		uint8_t fault_queue: 2;
		uint8_t reserved: 3;
	} __packed;
};

struct lm75_config {
	struct i2c_dt_spec i2c;
	union lm75_reg_config config_dt;
#if LM75_TRIGGER_SUPPORT
	struct gpio_dt_spec int_gpio;
#endif /* LM75_TRIGGER_SUPPORT */
};

static inline int lm75_reg_read(const struct device *dev, uint8_t reg, uint8_t *buf, uint32_t size)
{
	const struct lm75_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, reg, buf, size);
}

static inline int lm75_reg_write(const struct device *dev, uint8_t reg, const uint8_t *buf,
				 uint32_t size)
{
	const struct lm75_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->i2c, reg, buf, size);
}

static inline int lm75_temp_read(const struct device *dev, uint8_t reg, int16_t *value)
{
	uint8_t buf[2];
	int ret;

	ret = lm75_reg_read(dev, reg, buf, sizeof(buf));
	if (ret) {
		LOG_ERR("Could not fetch temperature [%d]", ret);
		return -EIO;
	}

	*value = sys_get_be16(buf);

	return 0;
}

static int lm75_temp_write(const struct device *dev, uint8_t reg, int16_t value)
{
	uint8_t buf[2];

	sys_put_be16(value, buf);
	return lm75_reg_write(dev, reg, buf, sizeof(buf));
}

static void lm75_sensor_value_to_temp(const struct sensor_value *val, int16_t *temp)
{
	*temp = val->val1 * 10;
	*temp += val->val2 / 100000U;

	*temp = (*temp * 256) / 10;
}

static void lm75_temp_to_sensor_value(int16_t temp, struct sensor_value *val)
{
	/* shift right by 7, multiply by 10 to get 0.1° and divide by 2 to get °C */
	temp = (temp / 128) * 10 / 2;

	/* Integer part in degrees Celsius */
	val->val1 = temp / 10;

	/* Fractional part in micro degrees Celsius */
	val->val2 = (temp - val->val1 * 10) * 100000U;
}

static int lm75_attr_set(const struct device *dev, enum sensor_channel chan,
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
	case SENSOR_ATTR_ALERT:
		reg = LM75_REG_T_OS;
		break;
	case SENSOR_ATTR_HYSTERESIS:
		reg = LM75_REG_T_HYST;
		break;
	default:
		return -ENOTSUP;
	}

	lm75_sensor_value_to_temp(val, &temp);

	err = lm75_temp_write(dev, reg, temp);
	if (err < 0) {
		LOG_ERR("failed to write register 0x%02x (err %d)", reg, err);
		return err;
	}

	return 0;
}

static int lm75_attr_get(const struct device *dev, enum sensor_channel chan,
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
	case SENSOR_ATTR_ALERT:
		reg = LM75_REG_T_OS;
		break;
	case SENSOR_ATTR_HYSTERESIS:
		reg = LM75_REG_T_HYST;
		break;
	default:
		return -ENOTSUP;
	}

	err = lm75_temp_read(dev, reg, &temp);
	if (err < 0) {
		LOG_ERR("failed to read register 0x%02x (err %d)", reg, err);
		return err;
	}

	lm75_temp_to_sensor_value(temp, val);

	return 0;
}

#if LM75_TRIGGER_SUPPORT
static int lm75_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			    sensor_trigger_handler_t handler)
{
	const struct lm75_config *config = dev->config;
	struct lm75_data *data = dev->data;
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

static void lm75_trigger_work_handler(struct k_work *item)
{
	struct lm75_data *data = CONTAINER_OF(item, struct lm75_data, work);
	sensor_trigger_handler_t handler = data->trigger_handler;

	if (handler != NULL) {
		handler(data->dev, (struct sensor_trigger *)data->trigger);
	}
}

static void lm75_int_gpio_callback_handler(const struct device *port, struct gpio_callback *cb,
					   gpio_port_pins_t pins)
{
	struct lm75_data *data = CONTAINER_OF(cb, struct lm75_data, int_gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit_to_queue(&data->workq, &data->work);
}
#endif /* LM75_TRIGGER_SUPPORT */

static inline int lm75_fetch_temp(const struct device *dev)
{
	struct lm75_data *data = dev->data;
	int16_t temp;
	int ret;

	ret = lm75_temp_read(dev, LM75_REG_TEMP, &temp);
	if (ret) {
		LOG_ERR("Could not fetch temperature [%d]", ret);
		return -EIO;
	}

	data->temp = temp;

	return 0;
}

static int lm75_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	enum pm_device_state pm_state;
	int ret;

	(void)pm_device_state_get(dev, &pm_state);
	if (pm_state != PM_DEVICE_STATE_ACTIVE) {
		ret = -EIO;
		return ret;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
		ret = lm75_fetch_temp(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int lm75_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	struct lm75_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		lm75_temp_to_sensor_value(data->temp, val);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, lm75_driver_api) = {
	.attr_set = lm75_attr_set,
	.attr_get = lm75_attr_get,
#if LM75_TRIGGER_SUPPORT
	.trigger_set = lm75_trigger_set,
#endif /* LM75_TRIGGER_SUPPORT */
	.sample_fetch = lm75_sample_fetch,
	.channel_get = lm75_channel_get,
};

int lm75_init(const struct device *dev)
{
	const struct lm75_config *cfg = dev->config;
	int ret = 0;
#if LM75_TRIGGER_SUPPORT
	struct lm75_data *data = dev->data;
#endif /* LM75_TRIGGER_SUPPORT */

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C dev not ready");
		return -ENODEV;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_init_suspended(dev);

	ret = pm_device_runtime_enable(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		LOG_ERR("Failed to enable runtime power management");
		return ret;
	}
#endif

	ret = lm75_reg_write(dev, LM75_REG_CONFIG, &cfg->config_dt.reg, sizeof(cfg->config_dt.reg));
	if (ret < 0) {
		LOG_ERR("failed to write configuration (ret %d)", ret);
		return ret;
	}

#if LM75_TRIGGER_SUPPORT
	/** Even if Trigger support is enabled, there may be multiple
	 * instances. This handles those who may not have Trigger support.
	 */
	if (cfg->int_gpio.port != NULL) {

		data->dev = dev;
		k_work_queue_start(&data->workq, data->stack, K_THREAD_STACK_SIZEOF(data->stack),
				CONFIG_LM75_TRIGGER_THREAD_PRIO, NULL);
		k_thread_name_set(&data->workq.thread, "lm75_trigger");
		k_work_init(&data->work, lm75_trigger_work_handler);

		if (!device_is_ready(cfg->int_gpio.port)) {
			LOG_ERR("INT GPIO not ready");
			return -EINVAL;
		}

		ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("failed to configure INT GPIO (ret %d)", ret);
			return ret;
		}

		gpio_init_callback(&data->int_gpio_cb, lm75_int_gpio_callback_handler,
				   BIT(cfg->int_gpio.pin));

		ret = gpio_add_callback(cfg->int_gpio.port, &data->int_gpio_cb);
		if (ret < 0) {
			LOG_ERR("failed to add INT GPIO callback (ret %d)", ret);
			return ret;
		}
	}
#endif /* LM75_TRIGGER_SUPPORT */

	return ret;
}

#ifdef CONFIG_PM_DEVICE

static int lm75_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#endif

#if LM75_TRIGGER_SUPPORT
#define LM75_INT_GPIO_INIT(n) .int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, { 0 })
#else /* LM75_TRIGGER_SUPPORT */
#define LM75_INT_GPIO_INIT(n)
#endif /* ! LM75_TRIGGER_SUPPORT */

#define LM75_INST(inst)                                             \
static struct lm75_data lm75_data_##inst;                           \
static const struct lm75_config lm75_config_##inst = {              \
	.i2c = I2C_DT_SPEC_INST_GET(inst),                          \
	.config_dt = {						\
		.shutdown = 0,					\
		.int_mode = DT_INST_NODE_HAS_PROP(inst, int_gpios), \
		.int_pol = DT_INST_PROP(inst, int_inverted),	\
		.fault_queue = 0, \
		.reserved = 0,					\
	},							\
	LM75_INT_GPIO_INIT(inst)					\
};                                                                  \
PM_DEVICE_DT_INST_DEFINE(inst, lm75_pm_action);						\
SENSOR_DEVICE_DT_INST_DEFINE(inst, lm75_init, PM_DEVICE_DT_INST_GET(inst), &lm75_data_##inst,	\
		      &lm75_config_##inst, POST_KERNEL,             \
		      CONFIG_SENSOR_INIT_PRIORITY, &lm75_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LM75_INST)
