/* bq35100.c - Driver for the Texas Instruments BQ35100 */

/*
 * Copyright (c) 2021 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq35100

#include <device.h>
#include <sys/util.h>
#include <logging/log.h>

#include "bq35100.h"
#include "drivers/sensor/bq35100.h"


LOG_MODULE_REGISTER(BQ35100, CONFIG_SENSOR_LOG_LEVEL);

/**
 * Read/Write from device.
 * @param dev - The device structure.
 * @param reg - The register address. Use BQ35100_REG_READ(x) or
 *				BQ35100_REG_WRITE(x).
 * @param data - The register data.
 * @param length - Number of bytes being read
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_bus_access(const struct device *dev, uint8_t reg,
			      uint8_t *data)
{
	const struct bq35100_config *cfg = dev->config;

	if (reg & BQ35100_READ) {
		return i2c_burst_read(cfg->bus,
				      cfg->i2c_addr,
				      BQ35100_TO_I2C_REG(reg),
				      data, length);
	} else {
		if (length != 2) {
			return -EINVAL;
		}

		uint8_t buf[3];

		buf[0] = BQ35100_TO_I2C_REG(reg);
		memcpy(buf + 1, data, sizeof(uint16_t));

		return i2c_write(cfg->bus, buf,
				 sizeof(buf), cfg->i2c_addr);
	}
}

/**
 * Read 8, 16 or 32 Bit from device.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param reg_data - The register data.
 * @param length - Number of bytes being read
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_reg_read(const struct device *dev,
			    uint8_t reg_addr,
			    uint32_t *reg_data,
			    size_t length)
{
	uint8_t buf[length];
	int ret;

	ret = bq35100_bus_access(dev, BQ35100_REG_READ(reg_addr), buf, length);

	/* Little Endian */
	switch (length) {
	case 1:
		*reg_data = buf[0];
		break;
	case 2:
		*reg_data = ((uint16_t)buf[1] << 8) | (uint16_t)buf[0];
		break;
	case 4:
		*reg_data = ((uint32_t)buf[3] << 24) | ((uint32_t)buf[2] << 16) |
			    ((uint32_t)buf[1] << 8) | (uint32_t)buf[0];
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

/**
 * Write (16 Bit) to device.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param reg_data - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_reg_write(const struct device *dev,
			     uint8_t reg_addr,
			     uint16_t reg_data)
{
	LOG_DBG("[0x%x] = 0x%x", reg_addr, reg_data);

	uint8_t buf[2];

	/* Little Endian */
	buf[0] = (uint8_t)reg_data;
	buf[1] = (uint8_t)(reg_data >> 8);

	return bq35100_bus_access(dev, BQ35100_REG_WRITE(reg_addr), buf);
}

/**
 * Write a subcommand to the device.
 * @param dev - The device structure.
 * @param subcommand - the subcommand register address
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_control_reg_write(const struct device *dev,
				     uint16_t subcommand)
{
	LOG_DBG("[0x%x] = 0x%x", BQ35100_CMD_MAC_CONTROL, subcommand);

	uint8_t buf[2];

	/* Little Endian */
	buf[0] = (uint8_t)subcommand;
	buf[1] = (uint8_t)(subcommand >> 8);

	return bq35100_bus_access(dev, BQ35100_REG_WRITE(BQ35100_CMD_MAC_CONTROL),
				  buf, 2);
}

/**
 * Read the response data of the previous subcommand from the device.
 * @param dev - The device structure.
 * @param data - The data response from the previous subcommand
 * @param length - Number of bytes being read
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_control_reg_read(const struct device *dev,
				    uint16_t *data)
{
	uint8_t buf[2];
	int ret;

	ret = bq35100_bus_access(dev, BQ35100_REG_READ(BQ35100_CMD_MAC_DATA),
				 buf, 2);

	/* Little Endian */
	*data = ((uint16_t)buf[1] << 8) | buf[0];
	
	return ret;
}

/**
 * Change the security mode.
 * @param dev - The device structure.
 * @param security_mode - The security mode wanted to be set.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_set_security_mode(const struct device *dev, enum bq35100_security security_mode)
{
	const struct bq35100_config *cfg = dev->config;
	uint8_t buf[4], status;
	buf[0] = BQ35100_CMD_MAC_CONTROL;

	switch (security_mode) {
	case BQ35100_SECURITY_UNKNOWN:
		LOG_ERR("Unkown mode");
		bq35100_current_security_mode = BQ35100_SECURITY_UNKNOWN;
			return -EIO;
		break;
	case  BQ35100_SECURITY_FULL_ACCESS:
		if ( security_mode == BQ35100_SECURITY_SEALED && !bq35100_set_security_mode(dev, BQ35100_SECURITY_UNSEALED) ) {
			LOG_ERR("Unseal first if in Sealed mode");
			return -EIO;
		}

		uint32_t full_access_codes = bq35100_reg_read(dev, BQ35100_FLASH_FULL_UNSEAL_STEP1, buf, 4);
		if ( full_access_codes < 0 ) {
			LOG_ERR("Unable to read from DataFlash");
			return -EIO;
		}

		full_access_codes = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
			
		buf[2] = (full_access_codes >> 24) & 0xFF;
        buf[1] = (full_access_codes >> 16) & 0xFF;

		status = bq35100_reg_write(dev, cfg->i2c_addr, buf);
		if ( !(status < 0) ) {
			buf[2] = (full_access_codes >> 8) & 0xFF;
			buf[1] = full_access_codes & 0xFF;
			bq35100_reg_write(dev, cfg->i2c_addr, buf);
		}

		bq35100_current_security_mode = BQ35100_SECURITY_FULL_ACCESS;
		break;
	case BQ35100_SECURITY_UNSEALED:
		if ( security_mode == BQ35100_SECURITY_FULL_ACCESS && !bq35100_set_security_mode(dev, BQ35100_SECURITY_SEALED) ){
			LOG_ERR("Seal first if in Full Access mode");
			return -EIO;
		}

		buf[2] = (BQ35100_DEFAULT_SEAL_CODES >> 24) & 0xFF;
		buf[1] = (BQ35100_DEFAULT_SEAL_CODES >> 16) & 0xFF;

		status = bq35100_reg_write(dev, cfg->i2c_addr, buf);
		if ( !(status < 0) ) {
			buf[2] = (BQ35100_DEFAULT_SEAL_CODES >> 8) & 0xFF;
			buf[1] = BQ35100_DEFAULT_SEAL_CODES & 0xFF;
			bq35100_reg_write(dev, cfg->i2c_addr, buf);
		}
		bq35100_current_security_mode = BQ35100_SECURITY_UNSEALED;
		break;
	case BQ35100_SECURITY_SEALED:
		buf[1] = 0x20; // First byte of SEALED sub-command (0x20)
		buf[2] = 0x00;  // Second byte of SEALED sub-command (0x00) (register address will auto-increment)
		bq35100_reg_write(dev, cfg->i2c_addr, buf);
		bq35100_current_security_mode = BQ35100_SECURITY_SEALED;
		break;
	default:
		LOG_ERR("Invalid mode");
			return -EIO;
		break;
	}
	k_sleep(K_MSEC(100));
	return 0;
}

/**
 * Set Gauge Mode.
 * @param dev - The device structure.
 * @param gauge_mode - The Gauge mode wanted to be set
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_set_gauge_mode(const struct device *dev, enum bq35100_gauge_mode gauge_mode)
{
	int status;
	uint8_t buf[1], data[34];

	if ( bq35100_current_security_mode == BQ35100_UNKNOWN_MODE || bq35100_current_security_mode == BQ35100_SECURITY_SEALED 
														&& !bq35100_set_security_mode(dev, BQ35100_SECURITY_UNSEALED) ) {
		LOG_ERR("Wrong Security Mode");
			return -EIO;
	}

	 //Operation Config A
	 //read extend data?
	status = bq35100_reg_read(dev, BQ35100_FLASH_OPERATION_CFG_A, buf, 1);
	if ( status < 0 ) {
		LOG_ERR("Unable to read Operation Config A");
			return -EIO;
	}

	if ( (bq35100_gauge_mode)(buf[0] & 0b11) != gauge_mode ) { //GMSEL 1:0
		buf[0] = buf[0] & ~0b11;
		buf[0] = buf[0] | (uint8_t)gauge_mode;

		k_sleep(K_MSEC(100));

		status = bq35100_reg_write(dev, BQ35100_FLASH_OPERATION_CFG_A, buf);
		if ( status < 0 ) {
			LOG_ERR("Unable to write Operation Config A");
			return -EIO;
		}
	}

	return 0;
}

 /**
  * Triggers the device to enter ACTIVE mode.
  * @param dev - The device structure.
  * @return 0 in case of success, negative error code otherwise. 
  */
int bq35100_gauge_start(const struct device *dev)
{
	int ret, status;

	ret = bq35100_control_reg_write(dev, BQ35100_CTRL_GAUGE_START);

	status = bq35100_control_reg_read(dev, BQ35100_CTRL_GAUGE_START);

	if ( status < 0) {
		LOG_ERR("Unable to start gauge");
		return -EIO;
	}

	return ret;
}

/**
  * Triggers the device to stop gauging and complete all outstanding tasks.
  * @param dev - The device structure.
  * @return 0 in case of success, negative error code otherwise. 
  */
int bq35100_gauge_stop(const struct device *dev)
{
	int ret, status;

	ret = bq35100_control_reg_write(dev, BQ35100_CTRL_GAUGE_STOP);

	status = bq35100_control_reg_read(dev, BQ35100_CTRL_GAUGE_STOP);

	if ( status < 0) {
		LOG_ERR("Unable to stop gauge");
		return -EIO;
	}

	return ret;
}

/**
 * Get the temperature register data
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_get_temp(const struct device *dev)
{
	struct bq35100_data *data = dev->data;

	return bq35100_reg_read(dev, BQ35100_CMD_TEMPERATURE,
				(uint16_t *)&data->temperature, 2);
}

/**
 * Get the voltage register data
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_get_voltage(const struct device *dev)
{
	struct bq35100_data *data = dev->data;

	return bq35100_reg_read(dev, BQ35100_CMD_VOLTAGE,
				(uint16_t *)&data->voltage, 2);
}

/**
 * Get the current register data
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_get_avg_current(const struct device *dev)
{
	struct bq35100_data *data = dev->data;

	return bq35100_reg_read(dev, BQ35100_CMD_CURRENT,
				(int16_t *)&data->avg_current, 2);
}

/**
 * Get the state of health register data
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_get_state_of_health(const struct device *dev)
{
	struct bq35100_data *data = dev->data;

	return bq35100_reg_read(dev, BQ35100_CMD_SOH,
				(uint8_t *)&data->state_of_health, 1);
}

/**
 * Get the accumulated capacity register data
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_get_acc_capacity(const struct device *dev)
{
	struct bq35100_data *data = dev->data;
	int status;

	status = bq35100_control_reg_write(dev, BQ35100_CTRL_CONTROL_STATUS);
	if (status < 0) {
		LOG_ERR("Unable to set [GA] pin");
		return -EIO;
	}

	status = bq35100_gauge_stop(dev);
	if (status < 0) {
		LOG_ERR("Unable to write Accumulated Capacity");
		return -EIO;
	}

	return bq35100_reg_read(dev, BQ35100_CMD_ACCUMULATED_CAPACITY,
				(uint32_t *)&data->acc_capacity, 4);
}

#ifdef CONFIG_PM_DEVICE

/**
 * Gauge Enable/disable
 * @param dev - The device structure.
 * @param enable - True = enable ge pin, false = disable ge pin
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_set_gauge_enable(const struct device *dev, bool enable)
{
	const struct bq35100_config *cfg = dev->config;
	int ret = 0;

	gpio_pin_set(cfg->ge_gpio, cfg->ge_pin, enable);

	return ret;
}

/**
 * Set the Device Power Management State.
 * @param dev - The device structure.
 * @param pm_state - power management state
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_device_pm_ctrl(const struct device *dev,
				  enum pm_device_action action)
{
	int ret;
	struct bq35100_data *data = dev->data;
	const struct bq35100_config *cfg = dev->config;
	enum pm_device_state curr_state;

	(void)pm_device_state_get(dev, &curr_state);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (curr_state == PM_DEVICE_STATE_OFF) {
			ret = bq35100_set_gauge_enable(dev, true);
			k_sleep(K_MSEC(1000));
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = -ENOTSUP;
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		if (cfg->ge_gpio->name) {
			ret = bq35100_set_gauge_enable(dev, false);
		} else {
			LOG_ERR("GE pin not defined");
			ret = -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

/**
 * Set attributes for the device.
 * @param dev - The device structure.
 * @param chan - The sensor channel type.
 * @param attr - The sensor attribute.
 * @param value - The sensor attribute value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	/* here you could set parameters at runtime through main. See
	   other drivers as example. Add new attributes in
	   enum sensor_attribute_bq35100 */
	return -ENOTSUP;
}

/**
 * Read sensor data from the device.
 * @param dev - The device structure.
 * @param cap_data - The sensor value data.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_get_sensor_data(const struct device *dev)
{
	bq35100_get_temp(dev);
	bq35100_get_voltage(dev);
	bq35100_get_avg_current(dev);
	bq35100_get_state_of_health(dev);
	bq35100_get_acc_capacity(dev);

	return 0;
}

/**
 * Read sensor data from the device.
 * @param dev - The device structure.
 * @param chan - The sensor channel type.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
#ifdef CONFIG_PM_DEVICE
	struct bq35100_data *data = dev->data;
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		LOG_ERR("Sample fetch failed, device is not in active mode");
		return -ENXIO;
	}
#endif

	return bq35100_get_sensor_data(dev);
}

/**
 * Get the sensor channel value which was fetched by bq35100_sample_fetch()
 * @param dev - The device structure.
 * @param chan - The sensor channel type.
 * @param val - The sensor channel value.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	// const struct bq35100_config *cfg = dev->config;
	struct bq35100_data *data = dev->data;

	switch ((int16_t)chan) {
	case SENSOR_CHAN_GAUGE_TEMP:
		val->val1 = ((uint16_t)(data->temperature) - 2731) / 10;
		val->val2 = ((uint16_t)((data->temperature) - 2731) % 10 * 100000);
		break;
	case SENSOR_CHAN_GAUGE_VOLTAGE:
		val->val1 = (uint16_t)data->voltage / 1000;
		val->val2 = ((uint16_t)data->voltage % 1000 * 1000);
		break;
	case SENSOR_CHAN_GAUGE_AVG_CURRENT:
		val->val1 = (int16_t)data->avg_current;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GAUGE_STATE_OF_CHARGE:
		val->val1 = (uint8_t)data->state_of_health;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GAUGE_ACCUMULATED_CAPACITY:
		val->val1 = (uint32_t)data->acc_capacity;
		val->val2 = 0;
		break;
	default:
		LOG_ERR("Channel type not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api bq35100_api_funcs = {
	.attr_set = bq35100_attr_set,
	.sample_fetch = bq35100_sample_fetch,
	.channel_get = bq35100_channel_get
};

/**
 * Probe device (Check if it is the correct device by reading the
 * BQ35100_CTRL_DEVICE_TYPE register).
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_probe(const struct device *dev)
{
	int status;
	uint16_t device_type;

	status = bq35100_control_reg_write(dev, BQ35100_CTRL_DEVICE_TYPE);
	if (status < 0) {
		LOG_ERR("Unable to write control register");
		return -EIO;
	}

	k_sleep(K_MSEC(100));
	status = bq35100_control_reg_read(dev, &device_type);
	if (status < 0) {
		LOG_ERR("Unable to read register");
		return -EIO;
	}

	if (device_type != BQ35100_DEVICE_TYPE_ID) {
		LOG_ERR("Wrong device type. Should be 0x%x, but is 0x%x",
			BQ35100_DEVICE_TYPE_ID, device_type);
		return -ENODEV;
	}

	return 0;
}

/**
 * Initialize the Gauge Enable Pin.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_init_ge_pin(const struct device *dev)
{
	const struct bq35100_config *cfg = dev->config;

	if (!device_is_ready(cfg->ge_gpio)) {
		LOG_ERR("%s: ge_gpio device not ready", cfg->ge_gpio->name);
		return -ENODEV;
	}

	gpio_pin_configure(cfg->ge_gpio, cfg->ge_pin,
			   GPIO_OUTPUT_ACTIVE | cfg->ge_flags);

	return 0;
}

/**
 * Initialization of the device.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int bq35100_init(const struct device *dev)
{
	const struct bq35100_config *cfg = dev->config;

	if (cfg->ge_gpio->name) {
		if (bq35100_init_ge_pin(dev) < 0) {
			return -ENODEV;
		}
		k_sleep(K_MSEC(1000));
	}

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("%s: bq35100 device not ready", dev->name);
		return -ENODEV;
	}

	if (bq35100_probe(dev) < 0) {
		return -ENODEV;
	}

	return 0;
}

#define BQ35100_GE_PROPS(n)						  \
	.ge_gpio = DEVICE_DT_GET(DT_GPIO_CTLR(DT_DRV_INST(n), ge_gpios)), \
	.ge_pin = DT_INST_GPIO_PIN(n, ge_gpios),			  \
	.ge_flags = DT_INST_GPIO_FLAGS(n, ge_gpios),			  \

#define BQ35100_GE(n)				       \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, ge_gpios), \
		   (BQ35100_GE_PROPS(n)))

#define BQ35100_INIT(n)						  \
	static struct bq35100_data bq35100_data_##n;		  \
								  \
	static const struct bq35100_config bq35100_config_##n = { \
		.bus = DEVICE_DT_GET(DT_INST_BUS(n)),		  \
		.i2c_addr = DT_INST_REG_ADDR(n),		  \
		BQ35100_GE(n)					  \
	};							  \
								  \
	DEVICE_DT_INST_DEFINE(n,				  \
			      bq35100_init,			  \
			      bq35100_device_pm_ctrl,		  \
			      &bq35100_data_##n,		  \
			      &bq35100_config_##n,		  \
			      POST_KERNEL,			  \
			      CONFIG_SENSOR_INIT_PRIORITY,	  \
			      &bq35100_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(BQ35100_INIT)