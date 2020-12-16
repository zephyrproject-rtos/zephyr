/*
 * Copyright (c) 2020 Richard Osterloh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_vcnl4040

#include "vcnl4040.h"
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(vcnl4040, CONFIG_SENSOR_LOG_LEVEL);

int vcnl4040_read(const struct device *dev, uint8_t reg, uint16_t *out)
{
	const struct vcnl4040_config *config = dev->config;
	struct vcnl4040_data *data = dev->data;
	uint8_t buff[2] = { 0 };
	int ret = 0;

	ret = i2c_write_read(data->i2c, config->i2c_address,
			     &reg, sizeof(reg), buff, sizeof(buff));

	if (!ret)
		*out = sys_get_le16(buff);

	return ret;
}

int vcnl4040_write(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct vcnl4040_config *config = dev->config;
	struct vcnl4040_data *data = dev->data;
	struct i2c_msg msg;
	int ret;
	uint8_t buff[3];

	sys_put_le16(value, &buff[1]);

	buff[0] = reg;

	msg.buf = buff;
	msg.flags = 0;
	msg.len = sizeof(buff);

	ret = i2c_transfer(data->i2c, &msg, 1, config->i2c_address);

	if (ret < 0) {
		LOG_ERR("write block failed");
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
	k_sem_take(&data->sem, K_FOREVER);

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
	k_sem_give(&data->sem);

	return ret;
}

static int vcnl4040_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct vcnl4040_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->sem, K_FOREVER);

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

	k_sem_give(&data->sem);

	return ret;
}

static int vcnl4040_proxy_setup(const struct device *dev)
{
	const struct vcnl4040_config *config = dev->config;
	uint16_t conf = 0;

	if (vcnl4040_read(dev, VCNL4040_REG_PS_MS, &conf)) {
		LOG_ERR("Could not read proximity config");
		return -EIO;
	}

	/* Set LED current */
	conf |= config->led_i << VCNL4040_LED_I_POS;

	if (vcnl4040_write(dev, VCNL4040_REG_PS_MS, conf)) {
		LOG_ERR("Could not write proximity config");
		return -EIO;
	}

	if (vcnl4040_read(dev, VCNL4040_REG_PS_CONF, &conf)) {
		LOG_ERR("Could not read proximity config");
		return -EIO;
	}

	/* Set PS_HD */
	conf |= VCNL4040_PS_HD_MASK;
	/* Set duty cycle */
	conf |= config->led_dc << VCNL4040_PS_DUTY_POS;
	/* Set integration time */
	conf |= config->proxy_it << VCNL4040_PS_IT_POS;
	/* Clear proximity shutdown */
	conf &= ~VCNL4040_PS_SD_MASK;

	if (vcnl4040_write(dev, VCNL4040_REG_PS_CONF, conf)) {
		LOG_ERR("Could not write proximity config");
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_VCNL4040_ENABLE_ALS
static int vcnl4040_ambient_setup(const struct device *dev)
{
	const struct vcnl4040_config *config = dev->config;
	struct vcnl4040_data *data = dev->data;
	uint16_t conf = 0;

	if (vcnl4040_read(dev, VCNL4040_REG_ALS_CONF, &conf)) {
		LOG_ERR("Could not read proximity config");
		return -EIO;
	}

	/* Set ALS integration time */
	conf |= config->als_it << VCNL4040_ALS_IT_POS;
	/* Clear ALS shutdown */
	conf &= ~VCNL4040_ALS_SD_MASK;

	if (vcnl4040_write(dev, VCNL4040_REG_ALS_CONF, conf)) {
		LOG_ERR("Could not write proximity config");
		return -EIO;
	}

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

	return 0;
}
#endif

#ifdef CONFIG_PM_DEVICE
static int vcnl4040_device_ctrl(const struct device *dev,
				uint32_t ctrl_command, void *context,
				device_pm_cb cb, void *arg)
{
	int ret = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		uint32_t device_pm_state = *(uint32_t *)context;
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
		if (device_pm_state == DEVICE_PM_ACTIVE_STATE) {
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
		} else {
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
					     als_conf)
			if (ret < 0)
				return ret;
#endif
		}

	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = DEVICE_PM_ACTIVE_STATE;
	}

	if (cb) {
		cb(dev, ret, context, arg);
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
	data->i2c = device_get_binding(config->i2c_name);
	if (data->i2c == NULL) {
		LOG_ERR("Could not find I2C device");
		return -EINVAL;
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

	if (vcnl4040_proxy_setup(dev)) {
		LOG_ERR("Failed to setup proximity functionality");
		return -EIO;
	}

#ifdef CONFIG_VCNL4040_ENABLE_ALS
	if (vcnl4040_ambient_setup(dev)) {
		LOG_ERR("Failed to setup ambient light functionality");
		return -EIO;
	}
#endif

	k_sem_init(&data->sem, 0, UINT_MAX);

#if CONFIG_VCNL4040_TRIGGER
	if (vcnl4040_trigger_init(dev)) {
		LOG_ERR("Could not initialise interrupts");
		return -EIO;
	}
#endif

	k_sem_give(&data->sem);

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

static const struct vcnl4040_config vcnl4040_config = {
	.i2c_name = DT_INST_BUS_LABEL(0),
	.i2c_address = DT_INST_REG_ADDR(0),
#ifdef CONFIG_VCNL4040_TRIGGER
#if DT_INST_NODE_HAS_PROP(0, int_gpios)
	.gpio_name = DT_INST_GPIO_LABEL(0, int_gpios),
	.gpio_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.gpio_flags = DT_INST_GPIO_FLAGS(0, int_gpios),
#else
	.gpio_name = NULL,
	.gpio_pin = 0,
	.gpio_flags = 0,
#endif
#endif
	.led_i = DT_ENUM_IDX(DT_DRV_INST(0), led_current),
	.led_dc = DT_ENUM_IDX(DT_DRV_INST(0), led_duty_cycle),
	.als_it = DT_ENUM_IDX(DT_DRV_INST(0), als_it),
	.proxy_it = DT_ENUM_IDX(DT_DRV_INST(0), proximity_it),
	.proxy_type = DT_ENUM_IDX(DT_DRV_INST(0), proximity_trigger),
};

static struct vcnl4040_data vcnl4040_data;

DEVICE_DT_INST_DEFINE(0, vcnl4040_init,
	      vcnl4040_device_ctrl, &vcnl4040_data, &vcnl4040_config,
	      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &vcnl4040_driver_api);
