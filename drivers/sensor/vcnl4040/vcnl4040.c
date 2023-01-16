/*
 * Copyright (c) 2020 Richard Osterloh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_vcnl4040

#include "vcnl4040.h"
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(vcnl4040, CONFIG_SENSOR_LOG_LEVEL);

int vcnl4040_read(const struct device *dev, uint8_t reg, uint16_t *out)
{
	const struct vcnl4040_config *config = dev->config;
	uint8_t buff[2] = { 0 };
	int ret = 0;

	ret = i2c_write_read_dt(&config->i2c,
			     &reg, sizeof(reg), buff, sizeof(buff));

	if (ret == 0) {
		*out = sys_get_le16(buff);
	}

	return ret;
}

int vcnl4040_write(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct vcnl4040_config *config = dev->config;
	uint8_t buf[3];
	int ret;

	buf[0] = reg;
	sys_put_le16(value, &buf[1]);

	ret = i2c_write_dt(&config->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("write[%02X]: %u", reg, ret);
		return ret;
	}

	return 0;
}

static int vcnl4040_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct vcnl4040_data *data = dev->data;
	int ret = 0;

#ifdef CONFIG_VCNL4040_ENABLE_ALS
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_PROX ||
			chan == SENSOR_CHAN_LIGHT);
#else
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_PROX);
#endif
	k_mutex_lock(&data->mutex, K_FOREVER);

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_PROX) {
		ret = vcnl4040_read(dev, VCNL4040_REG_PS_DATA,
				    &data->proximity);
		if (ret < 0) {
			LOG_ERR("Could not fetch proximity");
			goto exit;
		}
	}
#ifdef CONFIG_VCNL4040_ENABLE_ALS
	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT) {
		ret = vcnl4040_read(dev, VCNL4040_REG_ALS_DATA,
				    &data->light);
		if (ret < 0) {
			LOG_ERR("Could not fetch ambient light");
		}
	}
#endif
exit:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int vcnl4040_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct vcnl4040_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	switch (chan) {
	case SENSOR_CHAN_PROX:
		val->val1 = data->proximity;
		val->val2 = 0;
		break;

#ifdef CONFIG_VCNL4040_ENABLE_ALS
	case SENSOR_CHAN_LIGHT:
		val->val1 = data->light * data->sensitivity;
		val->val2 = 0;
		break;
#endif

	default:
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->mutex);

	return ret;
}

static int vcnl4040_reg_setup(const struct device *dev)
{
	const struct vcnl4040_config *config = dev->config;
	uint16_t value[VCNL4040_RW_REG_COUNT] = { 0 };
	uint8_t reg;
	int ret = 0;

#ifdef CONFIG_VCNL4040_ENABLE_ALS
	struct vcnl4040_data *data = dev->data;

	/* Set ALS integration time */
	value[VCNL4040_REG_ALS_CONF] = config->als_it << VCNL4040_ALS_IT_POS;
	/* Clear ALS shutdown */
	value[VCNL4040_REG_ALS_CONF] &= ~VCNL4040_ALS_SD_MASK;

	/*
	 * scale the lux depending on the value of the integration time
	 * see page 8 of the VCNL4040 application note:
	 * https://www.vishay.com/docs/84307/designingvcnl4040.pdf
	 */
	switch (config->als_it) {
	case VCNL4040_AMBIENT_INTEGRATION_TIME_80MS:
		data->sensitivity = 0.12;
		break;
	case VCNL4040_AMBIENT_INTEGRATION_TIME_160MS:
		data->sensitivity = 0.06;
		break;
	case VCNL4040_AMBIENT_INTEGRATION_TIME_320MS:
		data->sensitivity = 0.03;
		break;
	case VCNL4040_AMBIENT_INTEGRATION_TIME_640MS:
		data->sensitivity = 0.015;
		break;
	default:
		data->sensitivity = 1.0;
		LOG_WRN("Cannot set ALS sensitivity from ALS_IT=%d",
			config->als_it);
		break;
	}
#else
	value[VCNL4040_REG_ALS_CONF] = VCNL4040_ALS_SD_MASK; /* default */
#endif

	/* Set PS_HD */
	value[VCNL4040_REG_PS_CONF] = VCNL4040_PS_HD_MASK;
	/* Set duty cycle */
	value[VCNL4040_REG_PS_CONF] |= config->led_dc << VCNL4040_PS_DUTY_POS;
	/* Set integration time */
	value[VCNL4040_REG_PS_CONF] |= config->proxy_it << VCNL4040_PS_IT_POS;
	/* Clear proximity shutdown */
	value[VCNL4040_REG_PS_CONF] &= ~VCNL4040_PS_SD_MASK;

	/* Set LED current */
	value[VCNL4040_REG_PS_MS] = config->led_i << VCNL4040_LED_I_POS;

	for (reg = 0; reg < ARRAY_SIZE(value); reg++) {
		ret |= vcnl4040_write(dev, reg, value[reg]);
	}

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int vcnl4040_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	int ret = 0;
	uint16_t ps_conf;

	ret = vcnl4040_read(dev, VCNL4040_REG_PS_CONF, &ps_conf);
	if (ret < 0)
		return ret;
#ifdef CONFIG_VCNL4040_ENABLE_ALS
	uint16_t als_conf;

	ret = vcnl4040_read(dev, VCNL4040_REG_ALS_CONF, &als_conf);
	if (ret < 0)
		return ret;
#endif
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Clear proximity shutdown */
		ps_conf &= ~VCNL4040_PS_SD_MASK;

		ret = vcnl4040_write(dev, VCNL4040_REG_PS_CONF,
					ps_conf);
		if (ret < 0)
			return ret;
#ifdef CONFIG_VCNL4040_ENABLE_ALS
		/* Clear als shutdown */
		als_conf &= ~VCNL4040_ALS_SD_MASK;

		ret = vcnl4040_write(dev, VCNL4040_REG_ALS_CONF,
					als_conf);
		if (ret < 0)
			return ret;
#endif
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Set proximity shutdown bit 0 */
		ps_conf |= VCNL4040_PS_SD_MASK;

		ret = vcnl4040_write(dev, VCNL4040_REG_PS_CONF,
					ps_conf);
		if (ret < 0)
			return ret;
#ifdef CONFIG_VCNL4040_ENABLE_ALS
		/* Clear als shutdown bit 0 */
		als_conf |= VCNL4040_ALS_SD_MASK;

		ret = vcnl4040_write(dev, VCNL4040_REG_ALS_CONF,
					als_conf);
		if (ret < 0)
			return ret;
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

static int vcnl4040_init(const struct device *dev)
{
	const struct vcnl4040_config *config = dev->config;
	struct vcnl4040_data *data = dev->data;
	uint16_t id;

	/* Get the I2C device */
	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	/* Check device id */
	if (vcnl4040_read(dev, VCNL4040_REG_DEVICE_ID, &id)) {
		LOG_ERR("Could not read device id");
		return -EIO;
	}

	if (id != VCNL4040_DEFAULT_ID) {
		LOG_ERR("Incorrect device id (%d)", id);
		return -EIO;
	}

	if (vcnl4040_reg_setup(dev)) {
		LOG_ERR("register setup");
		return -EIO;
	}

	k_mutex_init(&data->mutex);

#if CONFIG_VCNL4040_TRIGGER
	if (vcnl4040_trigger_init(dev)) {
		LOG_ERR("Could not initialise interrupts");
		return -EIO;
	}
#endif

	LOG_DBG("Init complete");

	return 0;
}

static const struct sensor_driver_api vcnl4040_driver_api = {
	.sample_fetch = vcnl4040_sample_fetch,
	.channel_get = vcnl4040_channel_get,
#ifdef CONFIG_VCNL4040_TRIGGER
	.attr_set = vcnl4040_attr_set,
	.trigger_set = vcnl4040_trigger_set,
#endif
};

#define VCNL4040_DEFINE(inst)									\
	static struct vcnl4040_data vcnl4040_data_##inst;					\
												\
	static const struct vcnl4040_config vcnl4040_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_VCNL4040_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),))	\
		.led_i = DT_INST_ENUM_IDX(inst, led_current),					\
		.led_dc = DT_INST_ENUM_IDX(inst, led_duty_cycle),				\
		.als_it = DT_INST_ENUM_IDX(inst, als_it),					\
		.proxy_it = DT_INST_ENUM_IDX(inst, proximity_it),				\
		.proxy_type = DT_INST_ENUM_IDX(inst, proximity_trigger),			\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, vcnl4040_pm_action);					\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, vcnl4040_init, PM_DEVICE_DT_INST_GET(inst),		\
			      &vcnl4040_data_##inst, &vcnl4040_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &vcnl4040_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(VCNL4040_DEFINE)
