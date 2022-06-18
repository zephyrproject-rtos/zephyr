/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 * Copyright (c) 2018 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_ccs811

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "ccs811.h"

LOG_MODULE_REGISTER(CCS811, CONFIG_SENSOR_LOG_LEVEL);

#if DT_INST_NODE_HAS_PROP(0, wake_gpios)
static void set_wake(const struct device *dev, bool enable)
{
	const struct ccs811_config *config = dev->config;

	gpio_pin_set_dt(&config->wake_gpio, enable);
	if (enable) {
		k_busy_wait(50);        /* t_WAKE = 50 us */
	} else {
		k_busy_wait(20);        /* t_DWAKE = 20 us */
	}
}
#else
#define set_wake(...)
#endif

/* Get STATUS register in low 8 bits, and if ERROR is set put ERROR_ID
 * in bits 8..15.  These registers are available in both boot and
 * application mode.
 */
static int fetch_status(const struct device *dev)
{
	const struct ccs811_config *config = dev->config;
	uint8_t status;
	int rv;

	if (i2c_reg_read_byte_dt(&config->i2c, CCS811_REG_STATUS, &status) < 0) {
		LOG_ERR("Failed to read Status register");
		return -EIO;
	}

	rv = status;
	if (status & CCS811_STATUS_ERROR) {
		uint8_t error_id;

		if (i2c_reg_read_byte_dt(&config->i2c, CCS811_REG_ERROR_ID, &error_id) < 0) {
			LOG_ERR("Failed to read ERROR_ID register");
			return -EIO;
		}

		rv |= (error_id << 8);
	}

	return rv;
}

static inline uint8_t error_from_status(int status)
{
	return status >> 8;
}

const struct ccs811_result_type *ccs811_result(const struct device *dev)
{
	struct ccs811_data *drv_data = dev->data;

	return &drv_data->result;
}

int ccs811_configver_fetch(const struct device *dev,
			   struct ccs811_configver_type *ptr)
{
	struct ccs811_data *drv_data = dev->data;
	const struct ccs811_config *config = dev->config;
	uint8_t cmd;
	int rc;

	if (!ptr) {
		return -EINVAL;
	}

	set_wake(dev, true);
	cmd = CCS811_REG_HW_VERSION;
	rc = i2c_write_read_dt(&config->i2c, &cmd, sizeof(cmd), &ptr->hw_version,
			       sizeof(ptr->hw_version));
	if (rc == 0) {
		cmd = CCS811_REG_FW_BOOT_VERSION;
		rc = i2c_write_read_dt(&config->i2c, &cmd, sizeof(cmd),
				       (uint8_t *)&ptr->fw_boot_version,
				       sizeof(ptr->fw_boot_version));
		ptr->fw_boot_version = sys_be16_to_cpu(ptr->fw_boot_version);
	}

	if (rc == 0) {
		cmd = CCS811_REG_FW_APP_VERSION;
		rc = i2c_write_read_dt(&config->i2c, &cmd, sizeof(cmd),
				       (uint8_t *)&ptr->fw_app_version,
				       sizeof(ptr->fw_app_version));
		ptr->fw_app_version = sys_be16_to_cpu(ptr->fw_app_version);
	}
	if (rc == 0) {
		LOG_INF("HW %x FW %x APP %x",
			ptr->hw_version, ptr->fw_boot_version,
			ptr->fw_app_version);
	}

	set_wake(dev, false);
	ptr->mode = drv_data->mode & CCS811_MODE_MSK;

	return rc;
}

int ccs811_baseline_fetch(const struct device *dev)
{
	const uint8_t cmd = CCS811_REG_BASELINE;
	const struct ccs811_config *config = dev->config;
	int rc;
	uint16_t baseline;

	set_wake(dev, true);

	rc = i2c_write_read_dt(&config->i2c, &cmd, sizeof(cmd), (uint8_t *)&baseline,
			       sizeof(baseline));
	set_wake(dev, false);
	if (rc <= 0) {
		rc = baseline;
	}

	return rc;
}

int ccs811_baseline_update(const struct device *dev,
			   uint16_t baseline)
{
	const struct ccs811_config *config = dev->config;
	uint8_t buf[1 + sizeof(baseline)];
	int rc;

	buf[0] = CCS811_REG_BASELINE;
	memcpy(buf + 1, &baseline, sizeof(baseline));
	set_wake(dev, true);
	rc = i2c_write_dt(&config->i2c, buf, sizeof(buf));
	set_wake(dev, false);
	return rc;
}

int ccs811_envdata_update(const struct device *dev,
			  const struct sensor_value *temperature,
			  const struct sensor_value *humidity)
{
	const struct ccs811_config *config = dev->config;
	int rc;
	uint8_t buf[5] = { CCS811_REG_ENV_DATA };

	/*
	 * Environment data are represented in a broken whole/fraction
	 * system that specified a 9-bit fractional part to represent
	 * milli-units.  Since 1000 is greater than 512, the device
	 * actually only pays attention to the top bit, treating it as
	 * indicating 0.5.  So we only write the first octet (7-bit
	 * while plus 1-bit half).
	 *
	 * Humidity is simple: scale it by two and round to the
	 * nearest half.  Assume the fractional part is not
	 * negative.
	 */
	if (humidity) {
		int value = 2 * humidity->val1;

		value += (250000 + humidity->val2) / 500000;
		if (value < 0) {
			value = 0;
		} else if (value > (2 * 100)) {
			value = 2 * 100;
		}
		LOG_DBG("HUM %d.%06d becomes %d",
			humidity->val1, humidity->val2, value);
		buf[1] = value;
	} else {
		buf[1] = 2 * 50;
	}

	/*
	 * Temperature is offset from -25 Cel.  Values below minimum
	 * store as zero.  Default is 25 Cel.  Again we round to the
	 * nearest half, complicated by Zephyr's signed representation
	 * of the fractional part.
	 */
	if (temperature) {
		int value = 2 * temperature->val1;

		if (temperature->val2 < 0) {
			value += (250000 + temperature->val2) / 500000;
		} else {
			value += (-250000 + temperature->val2) / 500000;
		}
		if (value < (2 * -25)) {
			value = 0;
		} else {
			value += 2 * 25;
		}
		LOG_DBG("TEMP %d.%06d becomes %d",
			temperature->val1, temperature->val2, value);
		buf[3] = value;
	} else {
		buf[3] = 2 * (25 + 25);
	}

	set_wake(dev, true);
	rc = i2c_write_dt(&config->i2c, buf, sizeof(buf));
	set_wake(dev, false);
	return rc;
}

static int ccs811_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ccs811_data *drv_data = dev->data;
	const struct ccs811_config *config = dev->config;
	struct ccs811_result_type *rp = &drv_data->result;
	const uint8_t cmd = CCS811_REG_ALG_RESULT_DATA;
	int rc;
	uint16_t buf[4] = { 0 };
	unsigned int status;

	set_wake(dev, true);
	rc = i2c_write_read_dt(&config->i2c, &cmd, sizeof(cmd), (uint8_t *)buf, sizeof(buf));
	set_wake(dev, false);
	if (rc < 0) {
		return -EIO;
	}

	rp->co2 = sys_be16_to_cpu(buf[0]);
	rp->voc = sys_be16_to_cpu(buf[1]);
	status = sys_le16_to_cpu(buf[2]); /* sic */
	rp->status = status;
	rp->error = error_from_status(status);
	rp->raw = sys_be16_to_cpu(buf[3]);

	/* APP FW 1.1 does not set DATA_READY, but it does set CO2 to
	 * zero while it's starting up.  Assume a non-zero CO2 with
	 * old firmware is valid for the purposes of claiming the
	 * fetch was fresh.
	 */
	if ((drv_data->app_fw_ver <= 0x11)
	    && (rp->co2 != 0)) {
		status |= CCS811_STATUS_DATA_READY;
	}
	return (status & CCS811_STATUS_DATA_READY) ? 0 : -EAGAIN;
}

static int ccs811_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ccs811_data *drv_data = dev->data;
	const struct ccs811_result_type *rp = &drv_data->result;
	uint32_t uval;

	switch (chan) {
	case SENSOR_CHAN_CO2:
		val->val1 = rp->co2;
		val->val2 = 0;

		break;
	case SENSOR_CHAN_VOC:
		val->val1 = rp->voc;
		val->val2 = 0;

		break;
	case SENSOR_CHAN_VOLTAGE:
		/*
		 * Raw ADC readings are contained in least significant 10 bits
		 */
		uval = ((rp->raw & CCS811_RAW_VOLTAGE_MSK)
			>> CCS811_RAW_VOLTAGE_POS) * CCS811_RAW_VOLTAGE_SCALE;
		val->val1 = uval / 1000000U;
		val->val2 = uval % 1000000;

		break;
	case SENSOR_CHAN_CURRENT:
		/*
		 * Current readings are contained in most
		 * significant 6 bits in microAmps
		 */
		uval = ((rp->raw & CCS811_RAW_CURRENT_MSK)
			>> CCS811_RAW_CURRENT_POS) * CCS811_RAW_CURRENT_SCALE;
		val->val1 = uval / 1000000U;
		val->val2 = uval % 1000000;

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api ccs811_driver_api = {
#ifdef CONFIG_CCS811_TRIGGER
	.attr_set = ccs811_attr_set,
	.trigger_set = ccs811_trigger_set,
#endif
	.sample_fetch = ccs811_sample_fetch,
	.channel_get = ccs811_channel_get,
};

static int switch_to_app_mode(const struct device *dev)
{
	const struct ccs811_config *config = dev->config;
	uint8_t buf;
	int status;

	LOG_DBG("Switching to Application mode...");

	status = fetch_status(dev);
	if (status < 0) {
		return -EIO;
	}

	/* Check for the application firmware */
	if (!(status & CCS811_STATUS_APP_VALID)) {
		LOG_ERR("No Application firmware loaded");
		return -EINVAL;
	}

	/* Check if already in application mode */
	if (status & CCS811_STATUS_FW_MODE) {
		LOG_DBG("CCS811 Already in application mode");
		return 0;
	}

	buf = CCS811_REG_APP_START;
	/* Set the device to application mode */
	if (i2c_write_dt(&config->i2c, &buf, 1) < 0) {
		LOG_ERR("Failed to set Application mode");
		return -EIO;
	}

	k_msleep(1);             /* t_APP_START */
	status = fetch_status(dev);
	if (status < 0) {
		return -EIO;
	}

	/* Check for application mode */
	if (!(status & CCS811_STATUS_FW_MODE)) {
		LOG_ERR("Failed to start Application firmware");
		return -EINVAL;
	}

	LOG_DBG("CCS811 Application firmware started!");

	return 0;
}

#ifdef CONFIG_CCS811_TRIGGER

int ccs811_mutate_meas_mode(const struct device *dev,
			    uint8_t set,
			    uint8_t clear)
{
	struct ccs811_data *drv_data = dev->data;
	const struct ccs811_config *config = dev->config;
	int rc = 0;
	uint8_t mode = set | (drv_data->mode & ~clear);

	/*
	 * Changing drive mode of a running system has preconditions.
	 * Only allow changing the interrupt generation.
	 */
	if ((set | clear) & ~(CCS811_MODE_DATARDY | CCS811_MODE_THRESH)) {
		return -EINVAL;
	}

	if (mode != drv_data->mode) {
		set_wake(dev, true);
		rc = i2c_reg_write_byte_dt(&config->i2c, CCS811_REG_MEAS_MODE, mode);
		LOG_DBG("CCS811 meas mode change %02x to %02x got %d",
			drv_data->mode, mode, rc);
		if (rc < 0) {
			LOG_ERR("Failed to set mode");
			rc = -EIO;
		} else {
			drv_data->mode = mode;
			rc = 0;
		}

		set_wake(dev, false);
	}

	return rc;
}

int ccs811_set_thresholds(const struct device *dev)
{
	struct ccs811_data *drv_data = dev->data;
	const struct ccs811_config *config = dev->config;
	const uint8_t buf[5] = {
		CCS811_REG_THRESHOLDS,
		drv_data->co2_l2m >> 8,
		drv_data->co2_l2m,
		drv_data->co2_m2h >> 8,
		drv_data->co2_m2h,
	};
	int rc;

	set_wake(dev, true);
	rc = i2c_write_dt(&config->i2c, buf, sizeof(buf));
	set_wake(dev, false);
	return rc;
}

#endif /* CONFIG_CCS811_TRIGGER */

static int ccs811_init(const struct device *dev)
{
	struct ccs811_data *drv_data = dev->data;
	const struct ccs811_config *config = dev->config;
	int ret = 0;
	int status;
	uint16_t fw_ver;
	uint8_t cmd;
	uint8_t hw_id;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

#if DT_INST_NODE_HAS_PROP(0, wake_gpios)
	if (!device_is_ready(config->wake_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	/*
	 * Wakeup pin should be pulled low before initiating
	 * any I2C transfer.  If it has been tied to GND by
	 * default, skip this part.
	 */
	gpio_pin_configure_dt(&config->wake_gpio, GPIO_OUTPUT_INACTIVE);

	set_wake(dev, true);
	k_msleep(1);
#endif
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	if (!device_is_ready(config->reset_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);

	k_msleep(1);
#endif

#if DT_INST_NODE_HAS_PROP(0, irq_gpios)
	if (!device_is_ready(config->irq_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}
#endif

	k_msleep(20);            /* t_START assuming recent power-on */

	/* Reset the device.  This saves having to deal with detecting
	 * and validating any errors or configuration inconsistencies
	 * after a reset that left the device running.
	 */
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	gpio_pin_set_dt(&config->reset_gpio, 1);
	k_busy_wait(15);        /* t_RESET */
	gpio_pin_set_dt(&config->reset_gpio, 0);
#else
	{
		static uint8_t const reset_seq[] = {
			0xFF, 0x11, 0xE5, 0x72, 0x8A,
		};

		if (i2c_write_dt(&config->i2c, reset_seq, sizeof(reset_seq)) < 0) {
			LOG_ERR("Failed to issue SW reset");
			ret = -EIO;
			goto out;
		}
	}
#endif
	k_msleep(2);             /* t_START after reset */

	/* Switch device to application mode */
	ret = switch_to_app_mode(dev);
	if (ret) {
		goto out;
	}

	/* Check Hardware ID */
	if (i2c_reg_read_byte_dt(&config->i2c, CCS811_REG_HW_ID, &hw_id) < 0) {
		LOG_ERR("Failed to read Hardware ID register");
		ret = -EIO;
		goto out;
	}

	if (hw_id != CCS881_HW_ID) {
		LOG_ERR("Hardware ID mismatch!");
		ret = -EINVAL;
		goto out;
	}

	/* Check application firmware version (first byte) */
	cmd = CCS811_REG_FW_APP_VERSION;
	if (i2c_write_read_dt(&config->i2c, &cmd, sizeof(cmd), &fw_ver, sizeof(fw_ver)) < 0) {
		LOG_ERR("Failed to read App Firmware Version register");
		ret = -EIO;
		goto out;
	}
	fw_ver = sys_be16_to_cpu(fw_ver);
	LOG_INF("App FW %04x", fw_ver);
	drv_data->app_fw_ver = fw_ver >> 8U;

	/* Configure measurement mode */
	uint8_t meas_mode = CCS811_MODE_IDLE;
#ifdef CONFIG_CCS811_DRIVE_MODE_1
	meas_mode = CCS811_MODE_IAQ_1SEC;
#elif defined(CONFIG_CCS811_DRIVE_MODE_2)
	meas_mode = CCS811_MODE_IAQ_10SEC;
#elif defined(CONFIG_CCS811_DRIVE_MODE_3)
	meas_mode = CCS811_MODE_IAQ_60SEC;
#elif defined(CONFIG_CCS811_DRIVE_MODE_4)
	meas_mode = CCS811_MODE_IAQ_250MSEC;
#endif
	if (i2c_reg_write_byte_dt(&config->i2c, CCS811_REG_MEAS_MODE, meas_mode) < 0) {
		LOG_ERR("Failed to set Measurement mode");
		ret = -EIO;
		goto out;
	}
	drv_data->mode = meas_mode;

	/* Check for error */
	status = fetch_status(dev);
	if (status < 0) {
		ret = -EIO;
		goto out;
	}

	if (status & CCS811_STATUS_ERROR) {
		LOG_ERR("CCS811 Error %02x during sensor configuration",
			error_from_status(status));
		ret = -EINVAL;
		goto out;
	}

#ifdef CONFIG_CCS811_TRIGGER
	ret = ccs811_init_interrupt(dev);
	LOG_DBG("CCS811 interrupt init got %d", ret);
#endif

out:
	set_wake(dev, false);
	return ret;
}

static struct ccs811_data ccs811_data_inst;

static const struct ccs811_config ccs811_config_inst = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
	IF_ENABLED(CONFIG_CCS811_TRIGGER,
		   (.irq_gpio = GPIO_DT_SPEC_INST_GET(0, irq_gpios),))
	IF_ENABLED(DT_INST_NODE_HAS_PROP(0, reset_gpios),
		   (.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),))
	IF_ENABLED(DT_INST_NODE_HAS_PROP(0, wake_gpios),
		   (.wake_gpio = GPIO_DT_SPEC_INST_GET(0, wake_gpios),))
};

DEVICE_DT_INST_DEFINE(0, ccs811_init, NULL,
		      &ccs811_data_inst, &ccs811_config_inst,
		      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		      &ccs811_driver_api);
