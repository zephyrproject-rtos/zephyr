/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina219

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include "ina219.h"

LOG_MODULE_REGISTER(INA219, CONFIG_SENSOR_LOG_LEVEL);

static int ina219_reg_read(const struct device *dev,
		uint8_t reg_addr,
		uint16_t *reg_data)
{
	const struct ina219_config *cfg = dev->config;
	uint8_t rx_buf[2];
	int rc;

	rc = i2c_write_read_dt(&cfg->bus,
			&reg_addr, sizeof(reg_addr),
			rx_buf, sizeof(rx_buf));

	*reg_data = sys_get_be16(rx_buf);

	return rc;
}

static int ina219_reg_write(const struct device *dev,
		uint8_t addr,
		uint16_t reg_data)
{
	const struct ina219_config *cfg = dev->config;
	uint8_t tx_buf[3];

	tx_buf[0] = addr;
	sys_put_be16(reg_data, &tx_buf[1]);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int ina219_reg_field_update(const struct device *dev,
		uint8_t addr,
		uint16_t mask,
		uint16_t field)
{
	uint16_t reg_data;
	int rc;

	rc = ina219_reg_read(dev, addr, &reg_data);
	if (rc) {
		return rc;
	}

	reg_data = (reg_data & ~mask) | field;

	return ina219_reg_write(dev, addr, reg_data);
}

static int ina219_set_msr_delay(const struct device *dev)
{
	const struct ina219_config *cfg = dev->config;
	struct ina219_data *data = dev->data;

	data->msr_delay = ina219_conv_delay(cfg->badc) +
		ina219_conv_delay(cfg->sadc);
	return 0;
}

static int ina219_set_config(const struct device *dev)
{
	const struct ina219_config *cfg = dev->config;
	uint16_t reg_data;

	reg_data = (cfg->brng & INA219_BRNG_MASK) << INA219_BRNG_SHIFT |
		(cfg->pg & INA219_PG_MASK) << INA219_PG_SHIFT |
		(cfg->badc & INA219_ADC_MASK) << INA219_BADC_SHIFT |
		(cfg->sadc & INA219_ADC_MASK) << INA219_SADC_SHIFT |
		(cfg->mode & INA219_MODE_NORMAL);

	return ina219_reg_write(dev, INA219_REG_CONF, reg_data);
}

static int ina219_set_calib(const struct device *dev)
{
	const struct ina219_config *cfg = dev->config;
	uint16_t cal;

	cal = INA219_SCALING_FACTOR / ((cfg->r_shunt) * (cfg->current_lsb));

	return ina219_reg_write(dev, INA219_REG_CALIB, cal);
}

static int ina219_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ina219_data *data = dev->data;
	uint16_t status;
	uint16_t tmp;
	int rc;

	if (chan != SENSOR_CHAN_ALL &&
		chan != SENSOR_CHAN_VOLTAGE &&
		chan != SENSOR_CHAN_POWER &&
		chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	/* Trigger measurement and wait for completion */
	rc = ina219_reg_field_update(dev,
				    INA219_REG_CONF,
				    INA219_MODE_MASK,
				    INA219_MODE_NORMAL);
	if (rc) {
		LOG_ERR("Failed to start measurement.");
		return rc;
	}

	k_sleep(K_USEC(data->msr_delay));

	rc = ina219_reg_read(dev, INA219_REG_V_BUS, &status);
	if (rc) {
		LOG_ERR("Failed to read device status.");
		return rc;
	}

	while (!(INA219_CNVR_RDY(status))) {
		rc = ina219_reg_read(dev, INA219_REG_V_BUS, &status);
		if (rc) {
			LOG_ERR("Failed to read device status.");
			return rc;
		}
		k_sleep(K_USEC(INA219_WAIT_MSR_RETRY));
	}

	/* Check for overflow */
	if (INA219_OVF_STATUS(status)) {
		LOG_WRN("Power and/or Current calculations are out of range.");
	}

	if (chan == SENSOR_CHAN_ALL ||
		chan == SENSOR_CHAN_VOLTAGE) {

		rc = ina219_reg_read(dev, INA219_REG_V_BUS, &tmp);
		if (rc) {
			LOG_ERR("Error reading bus voltage.");
			return rc;
		}
		data->v_bus = INA219_VBUS_GET(tmp);
	}

	if (chan == SENSOR_CHAN_ALL ||
		chan == SENSOR_CHAN_POWER)	{

		rc = ina219_reg_read(dev, INA219_REG_POWER, &tmp);
		if (rc) {
			LOG_ERR("Error reading power register.");
			return rc;
		}
		data->power = tmp;
	}

	if (chan == SENSOR_CHAN_ALL ||
		chan == SENSOR_CHAN_CURRENT) {

		rc = ina219_reg_read(dev, INA219_REG_CURRENT, &tmp);
		if (rc) {
			LOG_ERR("Error reading current register.");
			return rc;
		}
		data->current = tmp;
	}

	return rc;
}

static int ina219_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ina219_config *cfg = dev->config;
	struct ina219_data *data = dev->data;
	double tmp;
	int8_t sign = 1;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		tmp = data->v_bus * INA219_V_BUS_MUL;
		break;
	case SENSOR_CHAN_POWER:
		tmp = data->power * cfg->current_lsb * INA219_POWER_MUL * INA219_SI_MUL;
		break;
	case SENSOR_CHAN_CURRENT:
		if (INA219_SIGN_BIT(data->current)) {
			data->current = ~data->current + 1;
			sign = -1;
		}
		tmp = sign * data->current * cfg->current_lsb * INA219_SI_MUL;
		break;
	default:
		LOG_DBG("Channel not supported by device!");
		return -ENOTSUP;
	}

	return sensor_value_from_double(val, tmp);
}

#ifdef CONFIG_PM_DEVICE
static int ina219_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	uint16_t reg_val;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return ina219_init(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		reg_val = INA219_MODE_SLEEP;
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		reg_val = INA219_MODE_OFF;
		break;
	default:
		return -ENOTSUP;
	}

	return ina219_reg_field_update(dev,
				INA219_REG_CONF,
				INA219_MODE_MASK,
				reg_val);
}
#endif /* CONFIG_PM_DEVICE */

static int ina219_init(const struct device *dev)
{
	const struct ina219_config *cfg = dev->config;
	int rc;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	rc = ina219_reg_write(dev, INA219_REG_CONF, INA219_RST);
	if (rc) {
		LOG_ERR("Could not reset device.");
		return rc;
	}

	rc = ina219_set_config(dev);
	if (rc) {
		LOG_ERR("Could not set configuration data.");
		return rc;
	}

	rc = ina219_set_calib(dev);
	if (rc) {
		LOG_DBG("Could not set calibration data.");
		return rc;
	}

	/* Set measurement delay */
	ina219_set_msr_delay(dev);

	k_sleep(K_USEC(INA219_WAIT_STARTUP));

	return 0;
}

static DEVICE_API(sensor, ina219_api) = {
	.sample_fetch = ina219_sample_fetch,
	.channel_get = ina219_channel_get,
};


#define INA219_INIT(n)						\
	static struct ina219_data ina219_data_##n;		\
								\
	static const struct ina219_config ina219_config_##n = {	\
		.bus = I2C_DT_SPEC_INST_GET(n),			\
		.current_lsb = DT_INST_PROP(n, lsb_microamp),	\
		.r_shunt = DT_INST_PROP(n, shunt_milliohm),	\
		.brng = DT_INST_PROP(n, brng),			\
		.pg = DT_INST_PROP(n, pg),			\
		.badc = DT_INST_PROP(n, badc),			\
		.sadc = DT_INST_PROP(n, sadc),			\
		.mode = INA219_MODE_NORMAL			\
	};							\
								\
	PM_DEVICE_DT_INST_DEFINE(n, ina219_pm_action);		\
								\
	SENSOR_DEVICE_DT_INST_DEFINE(n,				\
			      ina219_init,			\
			      PM_DEVICE_DT_INST_GET(n),		\
			      &ina219_data_##n,			\
			      &ina219_config_##n,		\
			      POST_KERNEL,			\
			      CONFIG_SENSOR_INIT_PRIORITY,	\
			      &ina219_api);

DT_INST_FOREACH_STATUS_OKAY(INA219_INIT)
