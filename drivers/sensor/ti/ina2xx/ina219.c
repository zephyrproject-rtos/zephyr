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

#include "ina2xx_common.h"

LOG_MODULE_DECLARE(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

/* Device register addresses */
#define INA219_REG_CONF		0x00
#define INA219_REG_V_SHUNT	0x01
#define INA219_REG_V_BUS	0x02
#define INA219_REG_POWER	0x03
#define INA219_REG_CURRENT	0x04
#define INA219_REG_CALIB	0x05

/* Config register shifts and masks */
#define INA219_RST		BIT(15)
#define INA219_BRNG_MASK	0x1
#define INA219_BRNG_SHIFT	13
#define INA219_PG_MASK		0x3
#define INA219_PG_SHIFT		11
#define INA219_ADC_MASK		0xF
#define INA219_BADC_SHIFT	7
#define INA219_SADC_SHIFT	3
#define INA219_MODE_MASK	0x7

/* Bus voltage register */
#define INA219_VBUS_GET(x)	((x) >> 3) & 0x3FFF
#define INA219_CNVR_RDY(x)	((x) >> 1) & 0x1
#define INA219_OVF_STATUS(x)	x & 0x1

/* Mode fields */
#define INA219_MODE_NORMAL	0x3 /* shunt and bus, triggered */
#define INA219_MODE_SLEEP	0x4 /* ADC off */
#define INA219_MODE_OFF		0x0 /* Power off */

/* Others */
#define INA219_SIGN_BIT(x)	((x) >> 15) & 0x1
#define INA219_V_BUS_DIV	250
#define INA219_SI_MUL		0.00001
#define INA219_POWER_MUL	200 /* 20 * 1e-5 * 1e6 */
#define INA219_WAIT_STARTUP	40
#define INA219_WAIT_MSR_RETRY	100
#define INA219_SCALING_FACTOR	4096000

INA2XX_REG_DEFINE(ina219_config, INA219_REG_CONF, 16, 0);
INA2XX_REG_DEFINE(ina219_cal, INA219_REG_CALIB, 16, 0);

/* Note: we are converting from milli to micro to match the newer sensors */
INA2XX_CHANNEL_DEFINE(ina219_bus_voltage, INA219_REG_V_BUS, 16, 0, 1000000, INA219_V_BUS_DIV);
INA2XX_CHANNEL_DEFINE(ina219_power, INA219_REG_POWER, 16, 0, INA219_POWER_MUL, 1);

/* Note: We are using custom scaling for current */
INA2XX_CHANNEL_DEFINE(ina219_current, INA219_REG_CURRENT, 16, 0, 0, 0);

static struct ina2xx_channels ina219_channels = {
	.bus_voltage = &ina219_bus_voltage,
	.shunt_voltage = NULL,
	.current = &ina219_current,
	.power = &ina219_power,
	.die_temp = NULL,
	.energy = NULL,
	.charge = NULL,
};
struct ina219_data {
	struct ina2xx_data common;
	uint32_t msr_delay;
};

struct ina219_config {
	struct ina2xx_config common;
	uint8_t badc;
	uint8_t sadc;
};

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
	const struct ina2xx_config *config = dev->config;
	uint16_t status;
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

	rc = ina2xx_reg_read_16(&config->bus, INA219_REG_V_BUS, &status);
	if (rc) {
		LOG_ERR("Failed to read device status.");
		return rc;
	}

	while (!(INA219_CNVR_RDY(status))) {
		rc = ina2xx_reg_read_16(&config->bus, INA219_REG_V_BUS, &status);
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

	return ina2xx_sample_fetch(dev, chan);
}

static int ina219_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ina2xx_config *config = dev->config;
	struct ina2xx_data *data = dev->data;
	double tmp;
	int8_t sign = 1;

	if (chan == SENSOR_CHAN_CURRENT) {
		if (INA219_SIGN_BIT(data->current)) {
			data->current = ~data->current + 1;
			sign = -1;
		}

		tmp = sign * data->current * config->current_lsb * INA219_SI_MUL;
		return sensor_value_from_double(val, tmp);
	}

	return ina2xx_channel_get(dev, chan, val);
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

#define INA219_DT_CALIB(inst)                                                   \
	 INA219_SCALING_FACTOR / (DT_INST_PROP(inst, rshunt_micro_ohms) *           \
	 DT_INST_PROP(inst, current_lsb_microamps) / 1000)

#define INA219_INIT(inst)                                                      \
	static struct ina219_data ina219_data_##inst;                              \
	static const struct ina219_config ina219_config_##inst = {                 \
		.common = {                                                            \
			.bus = I2C_DT_SPEC_INST_GET(inst),                                 \
			.current_lsb = DT_INST_PROP(inst, current_lsb_microamps),          \
			.config = INA219_DT_CONFIG(inst),                                  \
			.cal = INA219_DT_CALIB(inst),                                      \
			.id_reg = NULL,                                                    \
			.config_reg = &ina219_config,                                      \
			.adc_config_reg = NULL,                                            \
			.cal_reg = &ina219_cal,                                            \
			.channels = &ina219_channels,                                      \
		},                                                                     \
		.badc = DT_INST_PROP(inst, badc),                                      \
		.sadc = DT_INST_PROP(inst, sadc),                                      \
	};                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, ina219_pm_action);                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                                         \
			      ina219_init,                                                 \
			      PM_DEVICE_DT_INST_GET(inst),                                 \
			      &ina219_data_##inst,                                         \
			      &ina219_config_##inst,                                       \
			      POST_KERNEL,                                                 \
			      CONFIG_SENSOR_INIT_PRIORITY,                                 \
			      &ina219_api);

DT_INST_FOREACH_STATUS_OKAY(INA219_INIT)
