/*
 * Copyright (c) 2025 RIT Launch Initiative <launchinitiative@rit.edu>
 * Copyright (c) 2025 Aaron Chan
 * Copyright (c) 2025 Richard Sommers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina260

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "ina260.h"

LOG_MODULE_REGISTER(INA260, CONFIG_SENSOR_LOG_LEVEL);

static int ina260_reg_read(const struct device *dev, uint8_t reg_addr, uint16_t *reg_data)
{
	const struct ina260_config *cfg = dev->config;
	uint8_t rx_buf[2] = {0};
	int rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), rx_buf, sizeof(rx_buf));

	if (rc == 0) {
		*reg_data = sys_get_be16(rx_buf);
	}
	return rc;
}

static int ina260_reg_write(const struct device *dev, uint8_t addr, uint16_t reg_data)
{
	const struct ina260_config *cfg = dev->config;
	uint8_t tx_buf[3] = {addr, 0, 0};

	sys_put_be16(reg_data, &tx_buf[1]);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int ina260_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ina260_config *cfg = dev->config;
	struct ina260_data *data = dev->data;
	uint16_t tmp = 0;
	int rc = 0;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_POWER &&
	    chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	if (cfg->mode < CONT_OFF) {
		LOG_ERR("Triggered Mode not supported");
		return -ENOTSUP;
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_VOLTAGE) {
		rc = ina260_reg_read(dev, INA260_REG_V_BUS, &tmp);
		if (rc) {
			LOG_ERR("Error reading bus voltage.");
			return rc;
		}
		data->v_bus = tmp;
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POWER) {
		rc = ina260_reg_read(dev, INA260_REG_POWER, &tmp);
		if (rc) {
			LOG_ERR("Error reading power register.");
			return rc;
		}
		data->power = tmp;
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_CURRENT) {
		rc = ina260_reg_read(dev, INA260_REG_CURRENT, &tmp);
		if (rc) {
			LOG_ERR("Error reading current register.");
			return rc;
		}
		data->current = tmp;
	}

	return 0;
}
static int ina260_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ina260_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		/* Full-scale range = 40.96 V (decimal = 32767); LSB = 1.25 mV. */
		return sensor_value_from_float(val, data->v_bus * INA260_VOLTS_PER_LSB);
	case SENSOR_CHAN_CURRENT:
		/* The LSB size for the current register is set to 1.25 mA */
		return sensor_value_from_float(val, data->current * INA260_AMPS_PER_LSB);
	case SENSOR_CHAN_POWER:
		/* The Power Register LSB is fixed to 10 mW. */
		return sensor_value_from_float(val, data->power * INA260_WATTS_PER_LSB);
	default:
		LOG_DBG("Channel not supported by device");
		return -ENOTSUP;
	}

	return 0;
}

static int ina260_init(const struct device *dev)
{
	const struct ina260_config *cfg = dev->config;
	int rc = 0;
	uint8_t avg_bits = 0;
	uint8_t vct_bits = 0;
	uint8_t ict_bits = 0;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	rc = ina260_reg_write(dev, INA260_REG_CONF, INA260_RST);
	if (rc) {
		LOG_ERR("Could not reset device.");
		return rc;
	}

	/* Allow time for reset to complete */
	k_sleep(K_USEC(1000));

	/* Convert numeric values to register bits */
	switch (cfg->average) {
	case 1:
		avg_bits = 0;
		break;
	case 4:
		avg_bits = 1;
		break;
	case 16:
		avg_bits = 2;
		break;
	case 64:
		avg_bits = 3;
		break;
	case 128:
		avg_bits = 4;
		break;
	case 256:
		avg_bits = 5;
		break;
	case 512:
		avg_bits = 6;
		break;
	case 1024:
		avg_bits = 7;
		break;
	default:
		avg_bits = 3; /* Default to 64 samples */
		break;
	}

	switch (cfg->voltage_conv_time) {
	case 140:
		vct_bits = 0;
		break;
	case 204:
		vct_bits = 1;
		break;
	case 332:
		vct_bits = 2;
		break;
	case 588:
		vct_bits = 3;
		break;
	case 1100:
		vct_bits = 4;
		break;
	case 2116:
		vct_bits = 5;
		break;
	case 4156:
		vct_bits = 6;
		break;
	case 8244:
		vct_bits = 7;
		break;
	default:
		vct_bits = 4; /* Default to 1100us */
		break;
	}

	switch (cfg->current_conv_time) {
	case 140:
		ict_bits = 0;
		break;
	case 204:
		ict_bits = 1;
		break;
	case 332:
		ict_bits = 2;
		break;
	case 588:
		ict_bits = 3;
		break;
	case 1100:
		ict_bits = 4;
		break;
	case 2116:
		ict_bits = 5;
		break;
	case 4156:
		ict_bits = 6;
		break;
	case 8244:
		ict_bits = 7;
		break;
	default:
		ict_bits = 4; /* Default to 1100us */
		break;
	}

	const uint16_t conf_reg = (INA260_CONF_REQUIRED_TOP_BITS << 12) | (avg_bits << 9) |
				  (vct_bits << 6) | (ict_bits << 3) | (cfg->mode);

	rc = ina260_reg_write(dev, INA260_REG_CONF, conf_reg);
	if (rc) {
		LOG_ERR("Could not set configuration data.");
		return rc;
	}

	return 0;
}

static DEVICE_API(sensor, ina260_api) = {
	.sample_fetch = ina260_sample_fetch,
	.channel_get = ina260_channel_get,
};

#define INA260_MODE_NAME_TO_ENUM(mode_name)                                                        \
	(((mode_name != NULL) && !strcmp(mode_name, "TRIG_OFF"))       ? TRIG_OFF                  \
	 : ((mode_name != NULL) && !strcmp(mode_name, "TRIG_CURRENT")) ? TRIG_CURRENT              \
	 : ((mode_name != NULL) && !strcmp(mode_name, "TRIG_VOLTAGE")) ? TRIG_VOLTAGE              \
	 : ((mode_name != NULL) && !strcmp(mode_name, "TRIG_BOTH"))    ? TRIG_BOTH                 \
	 : ((mode_name != NULL) && !strcmp(mode_name, "CONT_OFF"))     ? CONT_OFF                  \
	 : ((mode_name != NULL) && !strcmp(mode_name, "CONT_CURRENT")) ? CONT_CURRENT              \
	 : ((mode_name != NULL) && !strcmp(mode_name, "CONT_VOLTAGE")) ? CONT_VOLTAGE              \
								       : CONT_BOTH)

#define INA260_INIT(n)                                                                             \
	static struct ina260_data ina260_data_##n;                                                 \
                                                                                                   \
	static const struct ina260_config ina260_config_##n = {                                    \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.average = DT_PROP(DT_INST(n, DT_DRV_COMPAT), average),                            \
		.voltage_conv_time = DT_PROP(DT_INST(n, DT_DRV_COMPAT), v_conv_time),              \
		.current_conv_time = DT_PROP(DT_INST(n, DT_DRV_COMPAT), i_conv_time),              \
		.mode = INA260_MODE_NAME_TO_ENUM(DT_PROP(DT_INST(n, DT_DRV_COMPAT), mode))};       \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, ina260_init, NULL, &ina260_data_##n, &ina260_config_##n,   \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ina260_api);

DT_INST_FOREACH_STATUS_OKAY(INA260_INIT)
