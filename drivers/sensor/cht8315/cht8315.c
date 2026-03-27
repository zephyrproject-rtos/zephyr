/*
 * Copyright (c) 2026 MASSDRIVER EI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensylink_cht8315

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define CHT8315_REG_TEMP		0x0
#define CHT8315_REG_HUMIDITY		0x1
#define CHT8315_REG_STATUS		0x2
#define CHT8315_REG_CONFIGURE		0x3
#define CHT8315_REG_CONVERSION_RATE	0x4
#define CHT8315_REG_LIM_HIGH_TEMP	0x5
#define CHT8315_REG_LIM_LOW_TEMP	0x6
#define CHT8315_REG_LIM_HIGH_HUMI	0x7
#define CHT8315_REG_LIM_LOW_HUMI	0x8
#define CHT8315_REG_ONESHOT		0xF
#define CHT8315_REG_SOFTRESET		0xFC
#define CHT8315_REG_MANUFID		0xFE
#define CHT8315_REG_DEVICEID		0xFF

#define CHT8315_MANUFID			0x5959
#define CHT8315_DEVID			0x8315

#define CHT8315_CFG_ALERT_MSK		BIT(15)
#define CHT8315_CFG_STANDBY		BIT(14)
#define CHT8315_CFG_HYSTMODE		BIT(13)
#define CHT8315_CFG_14BITS		BIT(12)
#define CHT8315_CFG_VALID_BIT		BIT(11)
#define CHT8315_CFG_ALERT_FAST		BIT(9)
#define CHT8315_CFG_HEATER		BIT(8)
#define CHT8315_CFG_BUS_TIMEOUT		BIT(7)
#define CHT8315_CFG_ALERT_POLARITY	BIT(5)
#define CHT8315_CFG_ALERT_SRC1		BIT(4)
#define CHT8315_CFG_ALERT_SRC0		BIT(3)
#define CHT8315_CFG_ALERT_FAULT1	BIT(2)
#define CHT8315_CFG_ALERT_FAULT0	BIT(1)
#define CHT8315_CFG_ALERTMODE		BIT(0)

#define CHT8315_REG_STATUS_BUSY		BIT(15)
#define CHT8315_REG_STATUS_THIGH	BIT(14)
#define CHT8315_REG_STATUS_TLOW		BIT(13)
#define CHT8315_REG_STATUS_HHIGH	BIT(12)
#define CHT8315_REG_STATUS_HLOW		BIT(11)

#define CHT8315_DUMMY			0xAAAA
#define CHT8315_TIMEOUT			K_MSEC(30)
#define CHT8315_RATE_CNT		8

#define CHT8315_TYP_CONV_TIME		K_MSEC(11)
#define CHT8315_CONV_SAFETY_MARGIN	K_MSEC(9)

/* Value is 3 bits, 0 to 7, 120s to 125ms time between samples*/
static const struct sensor_value cht8315_sample_rates[CHT8315_RATE_CNT] = {
	{ 0, 1000000/120 },
	{ 0, 1000000/60 },
	{ 0, 1000000/10 },
	{ 0, 1000000/5 },
	{ 1, 0 },
	{ 2, 0 },
	{ 4, 0 },
	{ 8, 0 },
};

struct cht8315_config {
	struct i2c_dt_spec i2c;
	bool oneshot_mode;
	bool heater;
	bool precision_mode;

#ifdef CONFIG_CHT8315_TRIGGER
	struct gpio_dt_spec alert;
#endif
};

struct cht8315_data {
	int16_t t_sample;
	uint16_t rh_sample;

#ifdef CONFIG_CHT8315_TRIGGER
	struct gpio_callback gpio_cb;
	const struct device *dev;
	sensor_trigger_handler_t rh_cb;
	const struct sensor_trigger *rh_trig;
	bool rh_triggered;
	sensor_trigger_handler_t t_cb;
	const struct sensor_trigger *t_trig;
	bool t_triggered;
	struct k_work work;
#endif
};

LOG_MODULE_REGISTER(CHT8315, CONFIG_SENSOR_LOG_LEVEL);

static int read16(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t *out)
{
	int ret = i2c_write_read_dt(i2c, &reg, 1, out, 2);
	*out = (*out >> 8) | ((*out & 0xff) << 8);
	return ret;
}

static int write16(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t val)
{
	uint8_t buf[3];

	buf[0] = reg;
	buf[1] = val >> 8;
	buf[2] = val & 0xff;

	return i2c_write_dt(i2c, buf, 3);
}

static int cht8315_wait_busy(const struct device *dev)
{
	const struct cht8315_config *cfg = dev->config;
	k_timepoint_t timeout = sys_timepoint_calc(CHT8315_TIMEOUT);
	uint16_t tmp;

	do {
		read16(&cfg->i2c, CHT8315_REG_STATUS, &tmp);
	} while ((tmp & CHT8315_REG_STATUS_BUSY) && !sys_timepoint_expired(timeout));

	if (sys_timepoint_expired(timeout)) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int cht8315_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct cht8315_data *data = dev->data;
	const struct cht8315_config *cfg = dev->config;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL
			|| chan == SENSOR_CHAN_AMBIENT_TEMP
			|| chan == SENSOR_CHAN_HUMIDITY);

	if (cfg->oneshot_mode) {
		ret = cht8315_wait_busy(dev);
		if (ret != 0) {
			LOG_ERR("Sensor is busy");
			return ret;
		}

		ret = write16(&cfg->i2c, CHT8315_REG_ONESHOT, CHT8315_DUMMY);
		if (ret != 0) {
			LOG_ERR("Failed to start oneshot: %d", ret);
			return ret;
		}

		/* This should be unlikely to not wait enough time */
		k_sleep(K_TIMEOUT_SUM(CHT8315_TYP_CONV_TIME, CHT8315_CONV_SAFETY_MARGIN));
	}

	ret = read16(&cfg->i2c, CHT8315_REG_TEMP, &data->t_sample);
	if (ret != 0) {
		LOG_ERR("Failed to get temperature: %d", ret);
		return ret;
	}

	ret = read16(&cfg->i2c, CHT8315_REG_HUMIDITY, &data->rh_sample);
	if (ret != 0) {
		LOG_ERR("Failed to get relative humidity: %d", ret);
		return ret;
	}

	return 0;
}

static struct sensor_value cht8315_temp_get(int16_t t)
{
	struct sensor_value val;

	/* val = (sample[13:0] >> 2) * 0.03125 */
	val.val1 = (int32_t)t / 128;
	val.val2 = (((int64_t)t * 1000000) / 128)
			- ((int64_t)val.val1 * 1000000);

	return val;
}

static struct sensor_value cht8315_humi_get(uint16_t rh)
{
	struct sensor_value val;

	/* val = 100 * sample[14:0] / 32768 */
	if (rh >= 0x8000) {
		val.val1 = 100;
		val.val2 = 0;
	} else {
		val.val1 = (100U * (uint32_t)rh) / 0x8000;
		val.val2 = (((uint64_t)rh * 100000000U) / 0x8000)
			- ((uint64_t)val.val1 * 1000000U);
	}

	return val;
}

#ifdef CONFIG_CHT8315_TRIGGER

static uint16_t cht8315_temp_set(const struct sensor_value *val, bool precision_mode)
{
	const int64_t mul = precision_mode ? 128 : 256;
	int16_t tmp;

	tmp = val->val1 * mul + ((int64_t)val->val2 * mul) / 1000000;

	return *(uint16_t *)&tmp;
}


static uint16_t cht8315_humi_set(const struct sensor_value *val)
{
	if (val->val1 >= 100) {
		return 0x8000;
	}

	return ((uint32_t)val->val1 * 0x8000) / 100U + ((uint64_t)val->val2 * 0x8000) / 100000000U;
}

static void cht8315_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct cht8315_data *data =
		CONTAINER_OF(cb, struct cht8315_data, gpio_cb);
	const struct cht8315_config *cfg = data->dev->config;

	if ((pins & BIT(cfg->alert.pin)) == 0U) {
		return;
	}

	k_work_submit(&data->work);
}

static void cht8315_work_handler(struct k_work *work)
{
	struct cht8315_data *data = CONTAINER_OF(work, struct cht8315_data, work);
	const struct cht8315_config *cfg = data->dev->config;
	int ret;
	uint16_t tmp;

	ret = read16(&cfg->i2c, CHT8315_REG_STATUS, &tmp);
	if (ret != 0) {
		LOG_ERR("Failed to fetch status: %d", ret);
	}

	if (data->t_cb != NULL && !data->t_triggered
	    && ((tmp & CHT8315_REG_STATUS_THIGH) != 0 || (tmp & CHT8315_REG_STATUS_TLOW) != 0)) {
		data->t_triggered = true;
		data->t_cb(data->dev, data->t_trig);
	}

	if (data->rh_cb != NULL && !data->rh_triggered
	    && ((tmp & CHT8315_REG_STATUS_HHIGH) != 0 || (tmp & CHT8315_REG_STATUS_HLOW) != 0)) {
		data->rh_triggered = true;
		data->rh_cb(data->dev, data->rh_trig);
	}

	/* Interrupt is automatically masked after trigger and read and is shared, unmask it */
	if ((data->rh_cb != NULL && !data->rh_triggered)
	    || (data->t_cb != NULL && !data->t_triggered)) {
		ret = read16(&cfg->i2c, CHT8315_REG_CONFIGURE, &tmp);
		if (ret != 0) {
			LOG_ERR("Failed to fetch configuration: %d", ret);
		}
		tmp &= ~CHT8315_CFG_ALERT_MSK;
		ret = write16(&cfg->i2c, CHT8315_REG_CONFIGURE, tmp);
		if (ret != 0) {
			LOG_ERR("Failed to clear interrupt mask: %d", ret);
		}
	}
}

#endif

static int cht8315_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct cht8315_config *cfg = dev->config;
	struct cht8315_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		*val = cht8315_temp_get(cfg->precision_mode ? data->t_sample : data->t_sample / 2);
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		*val = cht8315_humi_get(data->rh_sample);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int cht8315_configure(const struct device *dev)
{
	const struct cht8315_config *cfg = dev->config;
	int ret;
	uint16_t tmp;
	uint16_t s_cfg = CHT8315_CFG_BUS_TIMEOUT
		| CHT8315_CFG_VALID_BIT
		| CHT8315_CFG_ALERT_MSK;

	s_cfg |= cfg->heater ? CHT8315_CFG_HEATER : 0;
	s_cfg |= cfg->precision_mode ? CHT8315_CFG_14BITS : 0;

	if (cfg->oneshot_mode) {
		s_cfg |= CHT8315_CFG_STANDBY;
	}

	ret = write16(&cfg->i2c, CHT8315_REG_CONFIGURE, s_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure: %d", ret);
		return ret;
	}

	ret = read16(&cfg->i2c, CHT8315_REG_CONFIGURE, &tmp);
	if (ret != 0) {
		LOG_ERR("Failed to check configuration: %d", ret);
		return ret;
	}
	if (tmp != s_cfg) {
		LOG_ERR("Configuration not set: %x vs %x", s_cfg, tmp);
		if ((tmp & CHT8315_CFG_VALID_BIT) == 0) {
			LOG_ERR("Valid bit is not set, power cycle the sensor");
		}
		return -EIO;
	}

	return 0;
}

static int cht8315_init(const struct device *dev)
{
	const struct cht8315_config *cfg = dev->config;
	int ret;
	uint16_t tmp;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	ret = read16(&cfg->i2c, CHT8315_REG_MANUFID, &tmp);
	if (ret != 0) {
		LOG_ERR("Failed to get manufacturer ID: %d", ret);
		return ret;
	}
	if (tmp != CHT8315_MANUFID) {
		LOG_ERR("Invalid manufacturer ID: %x", tmp);
		return -EINVAL;
	}

	ret = read16(&cfg->i2c, CHT8315_REG_DEVICEID, &tmp);
	if (ret != 0) {
		LOG_ERR("Failed to get device ID: %d", ret);
		return ret;
	}
	if (tmp != CHT8315_DEVID) {
		LOG_ERR("Invalid device ID: %x", tmp);
		return -EINVAL;
	}

	ret = cht8315_configure(dev);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_CHT8315_TRIGGER
	if (cfg->alert.port && !cfg->oneshot_mode) {
		struct cht8315_data *data = dev->data;

		data->dev = dev;

		/* setup alert gpio interrupt */
		if (!gpio_is_ready_dt(&cfg->alert)) {
			LOG_ERR("%s: device %s is not ready", dev->name,
					cfg->alert.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->alert, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Failed to configure pin: %d", ret);
			return ret;
		}

		gpio_init_callback(&data->gpio_cb,
				cht8315_gpio_callback,
				BIT(cfg->alert.pin));

		if (gpio_add_callback(cfg->alert.port, &data->gpio_cb) < 0) {
			LOG_DBG("Failed to set GPIO callback");
			return -EIO;
		}

		k_work_init(&data->work, cht8315_work_handler);

		gpio_pin_interrupt_configure_dt(&cfg->alert, GPIO_INT_DISABLE);
	}
#endif

	LOG_INF("Initialized device successfully");

	return 0;
}

#ifdef CONFIG_CHT8315_TRIGGER

int cht8315_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	const struct cht8315_config *cfg = dev->config;
	struct cht8315_data *data = dev->data;
	int ret;
	uint16_t tmp;
	uint16_t s_cfg;

	if (cfg->oneshot_mode) {
		LOG_ERR("Oneshot mode is not supported with trigger");
		return -ENODEV;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_ALL
	    && trig->chan != SENSOR_CHAN_AMBIENT_TEMP
	    && trig->chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	ret = read16(&cfg->i2c, CHT8315_REG_CONFIGURE, &s_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to fetch configuration: %d", ret);
		return ret;
	}

	if (trig->chan == SENSOR_CHAN_AMBIENT_TEMP || trig->chan == SENSOR_CHAN_ALL) {
		data->t_cb = handler;
		data->t_trig = trig;
		data->t_triggered = false;
	}

	if (trig->chan == SENSOR_CHAN_HUMIDITY || trig->chan == SENSOR_CHAN_ALL) {
		data->rh_cb = handler;
		data->rh_trig = trig;
		data->rh_triggered = false;
	}

	/* Only alert for relative humidity *and* temperature together, this would never be used
	 * in a practical case but is equivalent to clearing the specific selection here before
	 * enabling the specifically relevant alerts (as alerts until now have been masked)
	 */
	s_cfg |= CHT8315_CFG_ALERT_SRC1 | CHT8315_CFG_ALERT_SRC0;

	/* When both are cleared, either can trigger the alert, but when only one is cleared,
	 * only the relevant threshold triggers the alert pin.
	 */
	if (data->rh_cb != NULL) {
		s_cfg &= ~CHT8315_CFG_ALERT_SRC0;
	}
	if (data->t_cb != NULL) {
		s_cfg &= ~CHT8315_CFG_ALERT_SRC1;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->alert, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = write16(&cfg->i2c, CHT8315_REG_CONFIGURE, s_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure: %d", ret);
		return ret;
	}

	/* Clear interrupts */
	ret = read16(&cfg->i2c, CHT8315_REG_STATUS, &tmp);
	if (ret != 0) {
		LOG_ERR("Failed to fetch status: %d", ret);
	}

	/* Unmask the alert for the interrupt pin */
	s_cfg &= ~CHT8315_CFG_ALERT_MSK;

	ret = write16(&cfg->i2c, CHT8315_REG_CONFIGURE, s_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure: %d", ret);
		return ret;
	}

	return ret;
}

#endif /* CONFIG_CHT8315_TRIGGER */

int cht8315_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	const struct cht8315_config *cfg = dev->config;
	uint16_t tmp_val;
	uint8_t reg = CHT8315_REG_CONVERSION_RATE;

	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		for (uint16_t i = 0; i < CHT8315_RATE_CNT; i++) {
			if (val->val1 == cht8315_sample_rates[i].val1
			    && val->val2 == cht8315_sample_rates[i].val2) {
				tmp_val = i << 8;
				return write16(&cfg->i2c, reg, tmp_val);
			}
		}
		LOG_ERR("Invalid sample rate, only the following are valid:");
		for (uint16_t i = 0; i < CHT8315_RATE_CNT; i++) {
			LOG_ERR("%f", sensor_value_to_double(&cht8315_sample_rates[i]));
		}
		return -EINVAL;
	}

#ifdef CONFIG_CHT8315_TRIGGER

	if (chan != SENSOR_CHAN_AMBIENT_TEMP && chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	if (attr != SENSOR_ATTR_LOWER_THRESH && attr != SENSOR_ATTR_UPPER_THRESH) {
		return -ENOTSUP;
	}

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		if (attr == SENSOR_ATTR_LOWER_THRESH) {
			reg = CHT8315_REG_LIM_LOW_TEMP;
		} else {
			reg = CHT8315_REG_LIM_HIGH_TEMP;
		}
		tmp_val = cht8315_temp_set(val, cfg->precision_mode);
	} else {
		if (attr == SENSOR_ATTR_LOWER_THRESH) {
			reg = CHT8315_REG_LIM_LOW_HUMI;
		} else {
			reg = CHT8315_REG_LIM_HIGH_HUMI;
		}
		tmp_val = cht8315_humi_set(val);
	}

	LOG_DBG("Writing 0x%02X to limit reg %x", tmp_val, reg);

	return write16(&cfg->i2c, reg, tmp_val);
#else
	return -ENOTSUP;
#endif
}

int cht8315_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     struct sensor_value *val)
{
	const struct cht8315_config *cfg = dev->config;
	uint16_t tmp_val;
	int ret;

	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		ret = read16(&cfg->i2c, CHT8315_REG_CONVERSION_RATE, &tmp_val);
		if (ret != 0) {
			LOG_ERR("Failed to fetch conversion rate: %d", ret);
			return ret;
		}

		tmp_val = tmp_val >> 8;
		if (tmp_val >= CHT8315_RATE_CNT) {
			return -EIO;
		}
		*val = cht8315_sample_rates[tmp_val];
		return 0;
	}

#ifdef CONFIG_CHT8315_TRIGGER

	if (chan != SENSOR_CHAN_AMBIENT_TEMP && chan != SENSOR_CHAN_HUMIDITY) {
		return -ENOTSUP;
	}

	if (attr != SENSOR_ATTR_LOWER_THRESH && attr != SENSOR_ATTR_UPPER_THRESH) {
		return -ENOTSUP;
	}

	{
		uint8_t reg = CHT8315_REG_CONVERSION_RATE;

		if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
			if (attr == SENSOR_ATTR_LOWER_THRESH) {
				reg = CHT8315_REG_LIM_LOW_TEMP;
			} else {
				reg = CHT8315_REG_LIM_HIGH_TEMP;
			}
		} else {
			if (attr == SENSOR_ATTR_LOWER_THRESH) {
				reg = CHT8315_REG_LIM_LOW_HUMI;
			} else {
				reg = CHT8315_REG_LIM_HIGH_HUMI;
			}
		}

		ret = read16(&cfg->i2c, reg, &tmp_val);
		if (ret != 0) {
			LOG_ERR("Failed to fetch configuration: %d", ret);
			return ret;
		}
	}
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		*val = cht8315_temp_get(cfg->precision_mode ? tmp_val : tmp_val / 2);
	} else {
		*val = cht8315_humi_get(tmp_val);
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}

static DEVICE_API(sensor, cht8315_driver_api) = {
	.sample_fetch = cht8315_sample_fetch,
	.channel_get = cht8315_channel_get,
	.attr_get = cht8315_attr_get,
	.attr_set = cht8315_attr_set,
#ifdef CONFIG_CHT8315_TRIGGER
	.trigger_set = cht8315_trigger_set,
#endif
};

#ifdef CONFIG_PM_DEVICE
static int cht8315_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct cht8315_config *cfg = dev->config;
	uint16_t tmp;
	int ret;

	if (action != PM_DEVICE_ACTION_RESUME && action != PM_DEVICE_ACTION_SUSPEND) {
		return -ENOTSUP;
	}

	ret = read16(&cfg->i2c, CHT8315_REG_CONFIGURE, &tmp);
	if (ret != 0) {
		LOG_ERR("Failed to fetch configuration: %d", ret);
		return ret;
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		tmp |= CHT8315_CFG_STANDBY;
	} else {
		tmp &= ~CHT8315_CFG_STANDBY;
	}

	ret = write16(&cfg->i2c, CHT8315_REG_CONFIGURE, tmp);
	if (ret != 0) {
		LOG_ERR("Failed to apply configuration: %d", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_CHT8315_TRIGGER
#define CHT8315_TRIGGER(inst) .alert = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),
#else
#define CHT8315_TRIGGER(inst)
#endif

#define CHT8315_DEFINE(inst)									\
	static struct cht8315_data cht8315_data_##inst = {0};					\
												\
	static const struct cht8315_config cht8315_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.oneshot_mode = DT_INST_PROP(inst, oneshot_mode),				\
		.heater = DT_INST_PROP(inst, heater),						\
		.precision_mode = DT_INST_PROP(inst, precision_mode),				\
		CHT8315_TRIGGER(inst)								\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, cht8315_pm_action);					\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, cht8315_init, PM_DEVICE_DT_INST_GET(inst),		\
			      &cht8315_data_##inst, &cht8315_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &cht8315_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(CHT8315_DEFINE)
