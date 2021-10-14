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
			      uint8_t *data, size_t length)
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

	return bq35100_bus_access(dev, BQ35100_REG_WRITE(reg_addr), buf, 2);
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

bool BQ35100::setSecurityMode(bq35100_security_t new_security) {
    bool success = false;
    char data[4];

    if (new_security == _security_mode) {
        return true; // We are already in this mode
    }

    if (new_security == SECURITY_UNKNOWN) {
        tr_error("Invalid access mode");
        return false;
    }

    // For reasons that aren't clear, the BQ35100 sometimes refuses
    // to change security mode if a previous security mode change
    // happend only a few seconds ago, hence the retry here

	//seal & unseal-gauge start & stop
    for (auto x = 0; (x < MBED_CONF_BQ35100_RETRY) && !success; x++) {
        data[0] = CMD_MAC;

        switch (new_security) {
            case SECURITY_SEALED:
                tr_debug("Setting security to SEALED");
                sendCntl(CNTL_SEAL);
                break;

            case SECURITY_FULL_ACCESS: {//**
                // Unseal first if in Sealed mode
                if (_security_mode == SECURITY_SEALED && !setSecurityMode(SECURITY_UNSEALED)) {
                    return false;
                }

                if (!readExtendedData(0X41D0, data, sizeof(data)/*4*/)) {//**cntrl_reg_read
                    tr_error("Could not get full access codes");
                    return false;
                }

                uint32_t full_access_codes = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];

                tr_debug("Setting security to FULL ACCESS");

                // Send the full access code with endianness conversion
                // in TWO writes
                data[2] = (full_access_codes >> 24) & 0xFF;
                data[1] = (full_access_codes >> 16) & 0xFF;

                if (write(data, 3)) {
                    data[2] = (full_access_codes >> 8) & 0xFF;
                    data[1] = full_access_codes & 0xFF;
                    write(data, 3);
                }
            }
            break;

            case SECURITY_UNSEALED: {
                // Seal first if in Full Access mode
                if (_security_mode == SECURITY_FULL_ACCESS && !setSecurityMode(SECURITY_SEALED)) {
                    return false;
                }

                tr_debug("Setting security to UNSEALED");

                data[2] = (_seal_codes >> 24) & 0xFF;
                data[1] = (_seal_codes >> 16) & 0xFF;

                if (write(data, 3)) {
                    data[2] = (_seal_codes >> 8) & 0xFF;
                    data[1] = _seal_codes & 0xFF;

                    write(data, 3);
                }
            }
            break;

            case SECURITY_UNKNOWN:
            default:
                MBED_ASSERT(false);

                break;
        }

        ThisThread::sleep_for(40ms); // always wait after writing codes
        _security_mode = getSecurityMode();

        if (_security_mode == new_security) {
            success = true;
            tr_info("Security mode set");

        } else {
            tr_error("Security mode set failed (wanted 0x%02X, got 0x%02X), will retry", new_security, _security_mode);
            ThisThread::sleep_for(40ms);
        }
    }

    return success;
}
/**
 * Enables End-Of-Service (EOS) Mode. 
 * This function follows the steps in bq35100.pdf section of 8.2.2.2.1.2
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */


static int bq35100_enable_EOS_mode(const struct device *dev)
{
	int status;

	/* Assumed steps 1 & 2 have already done */
	status = bq35100_gauge_start(dev); //Step 3.
	if (status < 0) {
		LOG_ERR("Unable to start gauge");
		return -EIO;
	}

	status = bq35100_gauge_stop(dev);  //Step 4.
	if (status < 0) {
		LOG_ERR("Unable to stop gauge");
		return -EIO;
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
	int ret;

	ret = bq35100_control_reg_write(dev,
				     BQ35100_CTRL_GAUGE_START);//reg_control_reg_read to check

	return ret;
}

/**
  * Triggers the device to stop gauging and complete all
	outstanding tasks.
  * @param dev - The device structure.
  * @return 0 in case of success, negative error code otherwise. 
  */
int bq35100_gauge_stop(const struct device *dev)
{
	int ret;

	ret = bq35100_control_reg_write(dev,
				     BQ35100_CTRL_GAUGE_STOP);

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