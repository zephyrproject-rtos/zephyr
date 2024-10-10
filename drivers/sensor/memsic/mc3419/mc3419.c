/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Linumiz
 */

#define DT_DRV_COMPAT memsic_mc3419

#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "mc3419.h"

LOG_MODULE_REGISTER(MC3419, CONFIG_SENSOR_LOG_LEVEL);

static const uint16_t mc3419_accel_sense_map[] = {1, 2, 4, 8, 6};
static struct mc3419_odr_map odr_map_table[] = {
	{25}, {50}, {62, 500}, {100},
	{125}, {250}, {500}, {1000}
};

static int mc3419_get_odr_value(uint16_t freq, uint16_t m_freq)
{
	for (int i = 0; i < ARRAY_SIZE(odr_map_table); i++) {
		if (odr_map_table[i].freq == freq &&
		    odr_map_table[i].mfreq == m_freq) {
			return i;
		}
	}

	return -EINVAL;
}

static inline int mc3419_set_op_mode(const struct mc3419_config *cfg,
				     enum mc3419_op_mode mode)
{
	return i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_OP_MODE, mode);
}

static int mc3419_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;
	struct mc3419_driver_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
	ret = i2c_burst_read_dt(&cfg->i2c, MC3419_REG_XOUT_L,
				(uint8_t *)data->samples,
				MC3419_SAMPLE_READ_SIZE);
	k_sem_give(&data->sem);
	return ret;
}

static int mc3419_to_sensor_value(double sensitivity, int16_t *raw_data,
				  struct sensor_value *val)
{
	double value = sys_le16_to_cpu(*raw_data);

	value *= sensitivity * SENSOR_GRAVITY_DOUBLE / 1000;

	return sensor_value_from_double(val, value);
}

static int mc3419_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	int ret = 0;
	struct mc3419_driver_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		ret = mc3419_to_sensor_value(data->sensitivity, &data->samples[0], val);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		ret = mc3419_to_sensor_value(data->sensitivity, &data->samples[1], val);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		ret = mc3419_to_sensor_value(data->sensitivity, &data->samples[2], val);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		ret = mc3419_to_sensor_value(data->sensitivity, &data->samples[0], &val[0]);
		ret |= mc3419_to_sensor_value(data->sensitivity, &data->samples[1], &val[1]);
		ret |= mc3419_to_sensor_value(data->sensitivity, &data->samples[2], &val[2]);
		break;
	default:
		LOG_ERR("Unsupported channel");
		ret = -ENOTSUP;
	}

	k_sem_give(&data->sem);
	return ret;
}

static int mc3419_set_accel_range(const struct device *dev, uint8_t range)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;
	struct mc3419_driver_data *data = dev->data;

	if (range >= MC3419_ACCL_RANGE_END) {
		LOG_ERR("Accel resolution is out of range");
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, MC3419_REG_RANGE_SELECT_CTRL,
				     MC3419_RANGE_MASK, range << 4);
	if (ret < 0) {
		LOG_ERR("Failed to set resolution (%d)", ret);
		return ret;
	}

	data->sensitivity = (double)(mc3419_accel_sense_map[range] *
				     SENSOR_GRAIN_VALUE);

	return 0;
}

static int mc3419_set_odr(const struct device *dev,
			  const struct sensor_value *val)
{
	int ret = 0;
	int data_rate = 0;
	const struct mc3419_config *cfg = dev->config;

	ret = mc3419_get_odr_value(val->val1, val->val2);
	if (ret < 0) {
		LOG_ERR("Failed to get odr value from odr map (%d)", ret);
		return ret;
	}

	data_rate = MC3419_BASE_ODR_VAL + ret;

	ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_SAMPLE_RATE,
				    data_rate);
	if (ret < 0) {
		LOG_ERR("Failed to set ODR (%d)", ret);
		return ret;
	}

	LOG_DBG("Set ODR Rate to 0x%x", data_rate);
	ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_SAMPLE_RATE_2, cfg->decimation_rate);
	if (ret < 0) {
		LOG_ERR("Failed to set decimation rate (%d)", ret);
		return ret;
	}

	return 0;
}

#if defined(CONFIG_MC3419_TRIGGER)
static int mc3419_set_anymotion_threshold(const struct device *dev,
					  const struct sensor_value *val)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;
	uint8_t buf[3] = {0};

	if (val->val1 > MC3419_ANY_MOTION_THRESH_MAX) {
		return -EINVAL;
	}

	buf[0] = MC3419_REG_ANY_MOTION_THRES;
	sys_put_le16((uint16_t)val->val1, &buf[1]);

	ret = i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to set anymotion threshold (%d)", ret);
		return ret;
	}

	return 0;
}

static int mc3419_trigger_set(const struct device *dev,
			      const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;
	struct mc3419_driver_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
	ret = mc3419_set_op_mode(cfg, MC3419_MODE_STANDBY);
	if (ret < 0) {
		goto exit;
	}

	ret = mc3419_configure_trigger(dev, trig, handler);
	if (ret < 0) {
		LOG_ERR("Failed to set trigger (%d)", ret);
	}

exit:
	mc3419_set_op_mode(cfg, MC3419_MODE_WAKE);

	k_sem_give(&data->sem);
	return ret;
}
#endif

static int mc3419_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	int ret = 0;
	struct mc3419_driver_data *data = dev->data;

	if (chan != SENSOR_CHAN_ACCEL_X &&
	    chan != SENSOR_CHAN_ACCEL_Y &&
	    chan != SENSOR_CHAN_ACCEL_Z &&
	    chan != SENSOR_CHAN_ACCEL_XYZ) {
		LOG_ERR("Not supported on this channel.");
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);
	ret = mc3419_set_op_mode(dev->config, MC3419_MODE_STANDBY);
	if (ret < 0) {
		goto exit;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		ret = mc3419_set_accel_range(dev, val->val1);
		break;
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = mc3419_set_odr(dev, val);
		break;
#if defined(CONFIG_MC3419_TRIGGER)
	case SENSOR_ATTR_SLOPE_TH:
		ret = mc3419_set_anymotion_threshold(dev, val);
		break;
#endif
	default:
		LOG_ERR("ACCEL attribute is not supported");
		ret = -EINVAL;
	}

exit:
	mc3419_set_op_mode(dev->config, MC3419_MODE_WAKE);

	k_sem_give(&data->sem);
	return ret;
}

static int mc3419_init(const struct device *dev)
{
	int ret = 0;
	struct mc3419_driver_data *data = dev->data;
	const struct mc3419_config *cfg = dev->config;

	if (!(i2c_is_ready_dt(&cfg->i2c))) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	k_sem_init(&data->sem, 1, 1);

#if defined(CONFIG_MC3419_TRIGGER)
	ret = mc3419_trigger_init(dev);
	if (ret < 0) {
		LOG_ERR("Could not initialize interrupts");
		return ret;
	}
#endif

	ret = i2c_reg_update_byte_dt(&cfg->i2c, MC3419_REG_RANGE_SELECT_CTRL, MC3419_LPF_MASK,
				     cfg->lpf_fc_sel);
	if (ret < 0) {
		LOG_ERR("Failed to configure LPF (%d)", ret);
		return ret;
	}
	/* Leave the sensor in default power on state, will be
	 * enabled by configure attr or setting trigger.
	 */

	LOG_INF("MC3419 Initialized");

	return ret;
}

static const struct sensor_driver_api mc3419_api = {
	.attr_set = mc3419_attr_set,
#if defined(CONFIG_MC3419_TRIGGER)
	.trigger_set = mc3419_trigger_set,
#endif
	.sample_fetch = mc3419_sample_fetch,
	.channel_get = mc3419_channel_get,
};

#if defined(CONFIG_MC3419_TRIGGER)
#define MC3419_CFG_IRQ(idx)						\
	.int_gpio = GPIO_DT_SPEC_INST_GET_OR(idx, int_gpios, { 0 }),	\
	.int_cfg  = DT_INST_PROP(idx, int_pin2),
#else
#define MC3419_CFG_IRQ(idx)
#endif

#define MC3419_DEFINE(idx)                                                                         \
                                                                                                   \
	static const struct mc3419_config mc3419_config_##idx = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(idx),                                                  \
		.lpf_fc_sel = DT_INST_PROP(idx, lpf_fc_sel),                                       \
		.decimation_rate = DT_INST_PROP(idx, decimation_rate),                             \
		MC3419_CFG_IRQ(idx)};                                                              \
	static struct mc3419_driver_data mc3419_data_##idx;                                        \
	SENSOR_DEVICE_DT_INST_DEFINE(idx, mc3419_init, NULL, &mc3419_data_##idx,                   \
				     &mc3419_config_##idx, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &mc3419_api);

DT_INST_FOREACH_STATUS_OKAY(MC3419_DEFINE)
