/*
 * Copyright 2021 Leonard Pollak
 * Copyright 2025 Nova Dynamics LLC
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

static inline int ina219_conv_delay(uint8_t delay_idx)
{
	switch (delay_idx) {
	case 0:
		return 84;
	case 1:
		return 148;
	case 2:
		return 276;
	case 3:
		return 532;
	case 9:
		return 1060;
	case 10:
		return 2013;
	case 11:
		return 4260;
	case 12:
		return 8510;
	case 13:
		return 17020;
	case 14:
		return 34050;
	case 15:
		return 68100;
	default:
		return -EINVAL;
	}
}

static int ina219_reg_field_update(const struct device *dev,
		uint8_t addr,
		uint16_t mask,
		uint16_t field)
{
	const struct ina219_config *config = dev->config;
	const struct ina2xx_config *common = &config->common;
	uint16_t reg_data;
	int rc;

	rc = ina2xx_reg_read_16(&common->bus, addr, &reg_data);
	if (rc) {
		return rc;
	}

	reg_data = (reg_data & ~mask) | field;

	return ina2xx_reg_write(&common->bus, addr, reg_data);
}

static int ina219_set_msr_delay(const struct device *dev)
{
	const struct ina219_config *cfg = dev->config;
	struct ina219_data *data = dev->data;

	data->msr_delay = ina219_conv_delay(cfg->badc) +
		ina219_conv_delay(cfg->sadc);
	return 0;
}

static int ina219_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ina219_data *data = dev->data;
	const struct ina219_config *config = dev->config;
	const struct ina2xx_config *common = &config->common;
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

	rc = ina2xx_reg_read_16(&common->bus, INA219_REG_V_BUS, &status);
	if (rc) {
		LOG_ERR("Failed to read device status.");
		return rc;
	}

	while (!(INA219_CNVR_RDY(status))) {
		rc = ina2xx_reg_read_16(&common->bus, INA219_REG_V_BUS, &status);
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

		rc = ina2xx_reg_read_16(&common->bus, INA219_REG_V_BUS, &tmp);
		if (rc) {
			LOG_ERR("Error reading bus voltage.");
			return rc;
		}
		data->v_bus = INA219_VBUS_GET(tmp);
	}

	if (chan == SENSOR_CHAN_ALL ||
		chan == SENSOR_CHAN_POWER)	{

		rc = ina2xx_reg_read_16(&common->bus, INA219_REG_POWER, &tmp);
		if (rc) {
			LOG_ERR("Error reading power register.");
			return rc;
		}
		data->power = tmp;
	}

	if (chan == SENSOR_CHAN_ALL ||
		chan == SENSOR_CHAN_CURRENT) {

		rc = ina2xx_reg_read_16(&common->bus, INA219_REG_CURRENT, &tmp);
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
	const struct ina2xx_config *common = &cfg->common;
	struct ina219_data *data = dev->data;
	double tmp;
	int8_t sign = 1;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		tmp = data->v_bus * INA219_V_BUS_MUL;
		break;
	case SENSOR_CHAN_POWER:
		tmp = data->power * common->current_lsb * INA219_POWER_MUL * INA219_SI_MUL;
		break;
	case SENSOR_CHAN_CURRENT:
		if (INA219_SIGN_BIT(data->current)) {
			data->current = ~data->current + 1;
			sign = -1;
		}
		tmp = sign * data->current * common->current_lsb * INA219_SI_MUL;
		break;
	default:
		LOG_DBG("Channel not supported by device!");
		return -ENOTSUP;
	}

	return sensor_value_from_double(val, tmp);
}

static int ina219_init(const struct device *dev)
{
	int ret = ina2xx_init(dev);

	if (ret < 0) {
		return ret;
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

#define INA219_DT_CONFIG(inst)                                                  \
	(DT_INST_PROP(inst, brng) & INA219_BRNG_MASK) << INA219_BRNG_SHIFT |        \
	(DT_INST_PROP(inst, pg) & INA219_PG_MASK) << INA219_PG_SHIFT |              \
	(DT_INST_PROP(inst, badc) & INA219_ADC_MASK) << INA219_BADC_SHIFT |         \
	(DT_INST_PROP(inst, sadc) & INA219_ADC_MASK) << INA219_SADC_SHIFT |         \
	(INA219_MODE_NORMAL)

/* Original calculation (milliohms):
 * INA219_SCALING_FACTOR / ((shunt-milliohm) * (lsb-microamp))
 */
#define INA219_DT_CALIB(inst)                                                   \
	 INA219_SCALING_FACTOR / (DT_INST_PROP(inst, rshunt_micro_ohms) *           \
	 DT_INST_PROP(inst, current_lsb_microamps) / 1000)

#define INA219_INIT(inst)                                                       \
	static struct ina219_data ina219_data_##inst;                               \
	static const struct ina219_config ina219_config_##inst = {                  \
		.common = {                                                             \
			.bus = I2C_DT_SPEC_INST_GET(inst),                                  \
			.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),           \
			.config = INA219_DT_CONFIG(inst),                                   \
			.cal = INA219_DT_CALIB(inst),                                       \
			.id_reg = -1,                                                       \
			.config_reg = INA219_REG_CONF,                                      \
			.adc_config_reg = -1,                                               \
			.cal_reg = INA219_REG_CALIB,                                        \
		},                                                                      \
		.badc = DT_INST_PROP(inst, badc),                                       \
		.sadc = DT_INST_PROP(inst, sadc),                                       \
	};                                                                          \
	PM_DEVICE_DT_INST_DEFINE(inst, ina219_pm_action);                           \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                                          \
			      ina219_init,                                                  \
			      PM_DEVICE_DT_INST_GET(inst),                                  \
			      &ina219_data_##inst,                                          \
			      &ina219_config_##inst,                                        \
			      POST_KERNEL,                                                  \
			      CONFIG_SENSOR_INIT_PRIORITY,                                  \
			      &ina219_api);

DT_INST_FOREACH_STATUS_OKAY(INA219_INIT)
