/*
 * Copyright (c) 2018 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <gpio.h>
#include <i2c.h>
#include <kernel.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <sensor.h>
#include <misc/__assert.h>

#include "ccs811.h"

static int ccs811_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct ccs811_data *drv_data = dev->driver_data;
	int tries = 11;
	u16_t buf[4];
	u8_t status;

	/* Check data ready flag for the measurement interval of 1 seconds */
	while (tries-- > 0) {
		if (i2c_reg_read_byte(drv_data->i2c, CONFIG_CCS811_I2C_ADDR,
				      CCS811_REG_STATUS, &status) < 0) {
			SYS_LOG_ERR("Failed to read Status register");
			return -EIO;
		}

		if ((status & CCS811_STATUS_DATA_READY) || tries == 0) {
			break;
		}

		k_sleep(100);
	}

	if (!(status & CCS811_STATUS_DATA_READY)) {
		SYS_LOG_ERR("Sensor data not available");
		return -EIO;
	}

	if (i2c_burst_read(drv_data->i2c, CONFIG_CCS811_I2C_ADDR,
			   CCS811_REG_ALG_RESULT_DATA, (u8_t *)buf, 8) < 0) {
		SYS_LOG_ERR("Failed to read conversion data.");
		return -EIO;
	}

	drv_data->co2 = sys_be16_to_cpu(buf[0]);
	drv_data->voc = sys_be16_to_cpu(buf[1]);
	drv_data->status = buf[2] & 0xff;
	drv_data->error = buf[2] >> 8;
	drv_data->resistance = sys_be16_to_cpu(buf[3]);

	return 0;
}

static int ccs811_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct ccs811_data *drv_data = dev->driver_data;
	u32_t uval;

	switch (chan) {
	case SENSOR_CHAN_CO2:
		val->val1 = drv_data->co2;
		val->val2 = 0;

		break;
	case SENSOR_CHAN_VOC:
		val->val1 = drv_data->voc;
		val->val2 = 0;

		break;
	case SENSOR_CHAN_VOLTAGE:
		/*
		 * Raw ADC readings are contained in least significant 10 bits
		 */
		uval = (drv_data->resistance & CCS811_VOLTAGE_MASK)
					* CCS811_VOLTAGE_SCALE;
		val->val1 = uval / 1000000;
		val->val2 = uval % 1000000;

		break;
	case SENSOR_CHAN_CURRENT:
		/*
		 * Current readings are contained in most
		 * significant 6 bits in microAmps
		 */
		uval = drv_data->resistance >> 10;
		val->val1 = uval / 1000000;
		val->val2 = uval % 1000000;

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api ccs811_driver_api = {
	.sample_fetch = ccs811_sample_fetch,
	.channel_get = ccs811_channel_get,
};

static int switch_to_app_mode(struct device *i2c)
{
	u8_t status, buf;

	SYS_LOG_DBG("Switching to Application mode...");

	if (i2c_reg_read_byte(i2c, CONFIG_CCS811_I2C_ADDR,
			      CCS811_REG_STATUS, &status) < 0) {
		SYS_LOG_ERR("Failed to read Status register");
		return -EIO;
	}

	/* Check for the application firmware */
	if (!(status & CCS811_STATUS_APP_VALID)) {
		SYS_LOG_ERR("No Application firmware loaded");
		return -EINVAL;
	}

	buf = CCS811_REG_APP_START;
	/* Set the device to application mode */
	if (i2c_write(i2c, &buf, 1, CONFIG_CCS811_I2C_ADDR) < 0) {
		SYS_LOG_ERR("Failed to set Application mode");
		return -EIO;
	}

	if (i2c_reg_read_byte(i2c, CONFIG_CCS811_I2C_ADDR,
			      CCS811_REG_STATUS, &status) < 0) {
		SYS_LOG_ERR("Failed to read Status register");
		return -EIO;
	}

	/* Check for application mode */
	if (!(status & CCS811_STATUS_FW_MODE)) {
		SYS_LOG_ERR("Failed to start Application firmware");
		return -EINVAL;
	}

	SYS_LOG_DBG("CCS811 Application firmware started!");

	return 0;
}

int ccs811_init(struct device *dev)
{
	struct ccs811_data *drv_data = dev->driver_data;
	int ret;
	u8_t hw_id, status;

	drv_data->i2c = device_get_binding(CONFIG_CCS811_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device!",
			    CONFIG_CCS811_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

#if defined(CONFIG_CCS811_GPIO_WAKEUP) || defined(CONFIG_CCS811_GPIO_RESET)
	drv_data->gpio = device_get_binding(CONFIG_CCS811_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device!",
			    CONFIG_CCS811_GPIO_DEV_NAME);
		return -EINVAL;
	}
#endif

#ifdef CONFIG_CCS811_GPIO_RESET
	gpio_pin_configure(drv_data->gpio, CONFIG_CCS811_GPIO_RESET_PIN_NUM,
			   GPIO_DIR_OUT);
	gpio_pin_write(drv_data->gpio, CONFIG_CCS811_GPIO_RESET_PIN_NUM, 1);

	k_sleep(1);
#endif

	/*
	 * Wakeup pin should be pulled low before initiating any I2C transfer.
	 * If it has been tied to GND by default, skip this part.
	 */
#ifdef CONFIG_CCS811_GPIO_WAKEUP
	gpio_pin_configure(drv_data->gpio, CONFIG_CCS811_GPIO_WAKEUP_PIN_NUM,
			   GPIO_DIR_OUT);
	gpio_pin_write(drv_data->gpio, CONFIG_CCS811_GPIO_WAKEUP_PIN_NUM, 0);

	k_sleep(1);
#endif

	/* Switch device to application mode */
	ret = switch_to_app_mode(drv_data->i2c);
	if (ret) {
		return ret;
	}

	/* Check Hardware ID */
	if (i2c_reg_read_byte(drv_data->i2c, CONFIG_CCS811_I2C_ADDR,
			      CCS811_REG_HW_ID, &hw_id) < 0) {
		SYS_LOG_ERR("Failed to read Hardware ID register");
		return -EIO;
	}

	if (hw_id != CCS881_HW_ID) {
		SYS_LOG_ERR("Hardware ID mismatch!");
		return -EINVAL;
	}

	/* Set Measurement mode for 1 second */
	if (i2c_reg_write_byte(drv_data->i2c, CONFIG_CCS811_I2C_ADDR,
			       CCS811_REG_MEAS_MODE,
			       CCS811_MODE_IAQ_1SEC) < 0) {
		SYS_LOG_ERR("Failed to set Measurement mode");
		return -EIO;
	}

	/* Check for error */
	if (i2c_reg_read_byte(drv_data->i2c, CONFIG_CCS811_I2C_ADDR,
			      CCS811_REG_STATUS, &status) < 0) {
		SYS_LOG_ERR("Failed to read Status register");
		return -EIO;
	}

	if (status & CCS811_STATUS_ERROR) {
		SYS_LOG_ERR("Error occurred during sensor configuration");
		return -EINVAL;
	}

	return 0;
}

static struct ccs811_data ccs811_driver;

DEVICE_AND_API_INIT(ccs811, CONFIG_CCS811_NAME, ccs811_init, &ccs811_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &ccs811_driver_api);
