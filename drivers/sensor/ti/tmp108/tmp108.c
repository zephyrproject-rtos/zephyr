/*
 * Copyright (c) 2021 Jimmy Johnson <catch22@fastmail.net>
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp108

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "tmp108.h"

LOG_MODULE_REGISTER(TMP108, CONFIG_SENSOR_LOG_LEVEL);

int tmp108_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct tmp108_config *cfg = dev->config;
	int result;

	result = i2c_burst_read_dt(&cfg->i2c_spec, reg, (uint8_t *) val, 2);

	if (result < 0) {
		return result;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

int tmp108_reg_write(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct tmp108_config *cfg = dev->config;
	uint8_t tx_buf[3];
	int result;

	tx_buf[0] = reg;
	sys_put_be16(val, &tx_buf[1]);

	result = i2c_write_dt(&cfg->i2c_spec, tx_buf, sizeof(tx_buf));

	if (result < 0) {
		return result;
	}

	return 0;
}

int tmp108_write_config(const struct device *dev, uint16_t mask, uint16_t conf)
{
	uint16_t config = 0;
	int result;

	result = tmp108_reg_read(dev, TI_TMP108_REG_CONF, &config);

	if (result < 0) {
		return result;
	}

	config &= mask;
	config |= conf;

	result = tmp108_reg_write(dev, TI_TMP108_REG_CONF, config);

	if (result < 0) {
		return result;
	}

	return 0;
}

int ti_tmp108_read_temp(const struct device *dev)
{
	struct tmp108_data *drv_data = dev->data;
	int result;

	/* clear previous temperature readings */

	drv_data->sample = 0U;

	/* Get the most recent temperature measurement */

	result = tmp108_reg_read(dev, TI_TMP108_REG_TEMP, &drv_data->sample);

	if (result < 0) {
		return result;
	}

	return 0;
}

static int tmp108_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct tmp108_data *drv_data = dev->data;
	int result;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* If one shot mode is set, query chip for reading
	 * should be finished 30 ms later
	 */
	if (drv_data->one_shot_mode == true) {

		result = tmp108_write_config(dev,
					     TI_TMP108_MODE_MASK(dev),
					     TI_TMP108_MODE_ONE_SHOT(dev));

		if (result < 0) {
			return result;
		}

		/* Schedule read to start in 30 ms if mode change was successful
		 * the typical wakeup time given in the data sheet is 27
		 */
		result = k_work_schedule(&drv_data->scheduled_work,
					 K_MSEC(TMP108_WAKEUP_TIME_IN_MS(dev)));

		if (result < 0) {
			return result;
		}

		return 0;
	}

	result =  ti_tmp108_read_temp(dev);

	if (result < 0) {
		return result;
	}

	return 0;
}

static int tmp108_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tmp108_data *drv_data = dev->data;
	int32_t uval;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	uval = ((int32_t)(drv_data->sample) * TMP108_TEMP_MULTIPLIER(dev)) >> 4U;
	val->val1 = uval / 1000000U;
	val->val2 = uval % 1000000U;

	return 0;
}

static int tmp108_attr_get(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   struct sensor_value *val)
{
	int result;
	uint16_t tmp_val;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch ((int) attr) {
	case SENSOR_ATTR_CONFIGURATION:
		result =  tmp108_reg_read(dev,
					  TI_TMP108_REG_CONF,
					  &tmp_val);
		val->val1 = tmp_val;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return result;
}

static int tmp108_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	struct tmp108_data *drv_data = dev->data;
	uint16_t mode = 0;
	uint16_t reg_value = 0;
	int result = 0;
	int32_t uval;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch ((int) attr) {
	case SENSOR_ATTR_HYSTERESIS:
		if (TI_TMP108_HYSTER_0_C(dev) == TI_TMP108_CONF_NA) {
			LOG_WRN("AS621x Series lacks Hysterisis setttings");
			return -ENOTSUP;
		}
		if (val->val1 < 1) {
			mode = TI_TMP108_HYSTER_0_C(dev);
		} else if (val->val1 < 2) {
			mode = TI_TMP108_HYSTER_1_C(dev);
		} else if (val->val1 < 4) {
			mode = TI_TMP108_HYSTER_2_C(dev);
		} else {
			mode = TI_TMP108_HYSTER_4_C(dev);
		}

		result = tmp108_write_config(dev,
					     TI_TMP108_HYSTER_MASK(dev),
					     mode);
		break;

	case SENSOR_ATTR_ALERT:
		/* Spec Sheet Errata: TM is set on reset not cleared */
		if (val->val1 == 1) {
			mode = TI_TMP108_CONF_TM_INT(dev);
		} else {
			mode = TI_TMP108_CONF_TM_CMP(dev);
		}

		result = tmp108_write_config(dev,
					     TI_TMP108_CONF_TM_MASK(dev),
					     mode);
		break;

	case SENSOR_ATTR_LOWER_THRESH:
		uval = val->val1 * 1000000 + val->val2;
		reg_value = (uval << 4U) / TMP108_TEMP_MULTIPLIER(dev);
		result = tmp108_reg_write(dev,
					  TI_TMP108_REG_LOW_LIMIT,
					  reg_value);
		break;

	case SENSOR_ATTR_UPPER_THRESH:
		uval = val->val1 * 1000000 + val->val2;
		reg_value = (uval << 4U) / TMP108_TEMP_MULTIPLIER(dev);
		result = tmp108_reg_write(dev,
					  TI_TMP108_REG_HIGH_LIMIT,
					  reg_value);
		break;

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->val1 < 1) {
			mode = TI_TMP108_FREQ_4_SECS(dev);
		} else if (val->val1 < 4) {
			mode = TI_TMP108_FREQ_1_HZ(dev);
		} else if (val->val1 < 16) {
			mode = TI_TMP108_FREQ_4_HZ(dev);
		} else {
			mode = TI_TMP108_FREQ_16_HZ(dev);
		}
		result = tmp108_write_config(dev,
					     TI_TMP108_FREQ_MASK(dev),
					     mode);
		break;

	case SENSOR_ATTR_TMP108_SHUTDOWN_MODE:
		result = tmp108_write_config(dev,
					     TI_TMP108_MODE_MASK(dev),
					     TI_TMP108_MODE_SHUTDOWN(dev));
		drv_data->one_shot_mode = false;
		break;

	case SENSOR_ATTR_TMP108_CONTINUOUS_CONVERSION_MODE:
		result = tmp108_write_config(dev,
					     TI_TMP108_MODE_MASK(dev),
					     TI_TMP108_MODE_CONTINUOUS(dev));
		drv_data->one_shot_mode = false;
		break;

	case SENSOR_ATTR_TMP108_ONE_SHOT_MODE:
		result = tmp108_write_config(dev,
					     TI_TMP108_MODE_MASK(dev),
					     TI_TMP108_MODE_ONE_SHOT(dev));
		drv_data->one_shot_mode = true;
		break;

	case SENSOR_ATTR_TMP108_ALERT_POLARITY:
		if (val->val1 == 1) {
			mode = TI_TMP108_CONF_POL_HIGH(dev);
		} else {
			mode = TI_TMP108_CONF_POL_LOW(dev);
		}
		result = tmp108_write_config(dev,
					     TI_TMP108_CONF_POL_MASK(dev),
					     mode);
		break;

	default:
		return -ENOTSUP;
	}

	if (result < 0) {
		return result;
	}

	return 0;
}

static const struct sensor_driver_api tmp108_driver_api = {
	.attr_set = tmp108_attr_set,
	.attr_get = tmp108_attr_get,
	.sample_fetch = tmp108_sample_fetch,
	.channel_get = tmp108_channel_get,
	.trigger_set = tmp_108_trigger_set,
};

#ifdef CONFIG_TMP108_ALERT_INTERRUPTS
static int setup_interrupts(const struct device *dev)
{
	struct tmp108_data *drv_data = dev->data;
	const struct tmp108_config *config = dev->config;
	const struct gpio_dt_spec *alert_gpio = &config->alert_gpio;
	int result;

	if (!device_is_ready(alert_gpio->port)) {
		LOG_ERR("tmp108: gpio controller %s not ready",
			alert_gpio->port->name);
		return -ENODEV;
	}

	result = gpio_pin_configure_dt(alert_gpio, GPIO_INPUT);

	if (result < 0) {
		return result;
	}

	gpio_init_callback(&drv_data->temp_alert_gpio_cb,
			   tmp108_trigger_handle_alert,
			   BIT(alert_gpio->pin));

	result = gpio_add_callback(alert_gpio->port,
				   &drv_data->temp_alert_gpio_cb);

	if (result < 0) {
		return result;
	}

	result = gpio_pin_interrupt_configure_dt(alert_gpio,
						 GPIO_INT_EDGE_BOTH);

	if (result < 0) {
		return result;
	}

	return 0;
}
#endif

static int tmp108_init(const struct device *dev)
{
	const struct tmp108_config *cfg = dev->config;
	struct tmp108_data *drv_data = dev->data;
	int result = 0;

	if (!device_is_ready(cfg->i2c_spec.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->i2c_spec.bus->name);
		return -ENODEV;
	}

	drv_data->scheduled_work.work.handler = tmp108_trigger_handle_one_shot;

	/* save this driver instance for passing to other functions */
	drv_data->tmp108_dev = dev;

#ifdef CONFIG_TMP108_ALERT_INTERRUPTS
	result = setup_interrupts(dev);

	if (result < 0) {
		return result;
	}
#endif
	/* clear and set configuration registers back to default values */
	result = tmp108_write_config(dev,
				     0x0000,
				     TMP108_CONF_RST(dev));
	return result;
}

#define TMP108_DEFINE(inst, t)						   \
	static struct tmp108_data tmp108_prv_data_##inst##t;		   \
	static const struct tmp108_config tmp108_config_##inst##t = {	   \
		.i2c_spec = I2C_DT_SPEC_INST_GET(inst),			   \
		.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst,		   \
						       alert_gpios, { 0 }),\
		.reg_def = t##_CONF					   \
	};								   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				   \
			      &tmp108_init,				   \
			      NULL,					   \
			      &tmp108_prv_data_##inst##t,		   \
			      &tmp108_config_##inst##t,			   \
			      POST_KERNEL,				   \
			      CONFIG_SENSOR_INIT_PRIORITY,		   \
			      &tmp108_driver_api);

#define TMP108_INIT(n) TMP108_DEFINE(n, TI_TMP108)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_tmp108
DT_INST_FOREACH_STATUS_OKAY(TMP108_INIT)

#define AS6212_INIT(n) TMP108_DEFINE(n, AMS_AS6212)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ams_as6212
DT_INST_FOREACH_STATUS_OKAY(AS6212_INIT)
