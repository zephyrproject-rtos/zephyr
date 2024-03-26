/*
 * Copyright (c) 2021 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si7210

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(SI7210, CONFIG_SENSOR_LOG_LEVEL);

/* Register addresses */
#define SI7210_REG_CHIPID		0xC0
#define SI7210_REG_DSPSIGM		0xC1
#define SI7210_REG_DSPSIGL		0xC2
#define SI7210_REG_DSPSIGSEL		0xC3
#define SI7210_REG_POWER_CTRL		0xC4
#define SI7210_REG_ARAUTOINC		0xC5
#define SI7210_REG_CTRL1		0xC6
#define SI7210_REG_CTRL2		0xC7
#define SI7210_REG_SLTIME		0xC8
#define SI7210_REG_CTRL3		0xC9
#define SI7210_REG_A0			0xCA
#define SI7210_REG_A1			0xCB
#define SI7210_REG_A2			0xCC
#define SI7210_REG_CTRL4		0xCD
#define SI7210_REG_A3			0xCE
#define SI7210_REG_A4			0xCF
#define SI7210_REG_A5			0xD0
#define SI7210_REG_OTP_ADDR		0xE1
#define SI7210_REG_OTP_DATA		0xE2
#define SI7210_REG_OTP_CTRL		0xE3
#define SI7210_REG_TM_FG		0xE4

/* Registers bits */
#define SI7210_BIT_DSPSIGSEL_MAG	0x00
#define SI7210_BIT_DSPSIGSEL_TEMP	0x01
#define SI7210_BIT_POWER_CTRL_MEAS	0x80
#define SI7210_BIT_POWER_CTRL_USESTORE	0x08
#define SI7210_BIT_POWER_CTRL_ONEBURST	0x04
#define SI7210_BIT_POWER_CTRL_STOP	0x02
#define SI7210_BIT_POWER_CTRL_SLEEP	0x01
#define SI7210_BIT_CTRL3_SLTIMEENA	0x01
#define SI7210_BIT_CTRL3_SLTFAST	0x02
#define SI7210_BIT_OTP_CTRL_READEN	0x02
#define SI7210_BIT_OTP_CTRL_BUSY	0x01

/* OT registers */
#define SI7210_OTPREG_DEF_CTRL1		0x04
#define SI7210_OTPREG_DEF_CTRL2		0x05
#define SI7210_OTPREG_DEF_SLTIME	0x06
#define SI7210_OTPREG_DEF_CTRL3		0x08
#define SI7210_OTPREG_DEF_A0		0x09
#define SI7210_OTPREG_DEF_A1		0x0A
#define SI7210_OTPREG_DEF_A2		0x0B
#define SI7210_OTPREG_DEF_CTRL4		0x0C
#define SI7210_OTPREG_DEF_A3		0x0D
#define SI7210_OTPREG_DEF_A4		0x0E
#define SI7210_OTPREG_DEF_A5		0x0F
#define SI7210_OTPREG_PART_BASE		0x14
#define SI7210_OTPREG_PART_VARIANT	0x15
#define SI7210_OTPREG_SN1		0x18
#define SI7210_OTPREG_SN2		0x19
#define SI7210_OTPREG_SN3		0x1A
#define SI7210_OTPREG_SN4		0x1B
#define SI7210_OTPREG_TEMP_OFFSET	0x1D
#define SI7210_OTPREG_TEMP_GAIN		0x1E
#define SI7210_OTPREG_200G_SCALE_A0	0x21
#define SI7210_OTPREG_200G_SCALE_A1	0x22
#define SI7210_OTPREG_200G_SCALE_A2	0x23
#define SI7210_OTPREG_200G_SCALE_A3	0x24
#define SI7210_OTPREG_200G_SCALE_A4	0x25
#define SI7210_OTPREG_200G_SCALE_A5	0x26
#define SI7210_OTPREG_2000G_SCALE_A0	0x27
#define SI7210_OTPREG_2000G_SCALE_A1	0x28
#define SI7210_OTPREG_2000G_SCALE_A2	0x29
#define SI7210_OTPREG_2000G_SCALE_A3	0x30
#define SI7210_OTPREG_2000G_SCALE_A4	0x31
#define SI7210_OTPREG_2000G_SCALE_A5	0x32

enum si7210_scale {
	si7210_scale_200G,
	si7210_scale_2000G,
};

struct si7210_config {
	struct i2c_dt_spec bus;
};

struct si7210_data {
	int8_t otp_temp_offset;
	int8_t otp_temp_gain;

	enum si7210_scale scale;

	uint8_t reg_dspsigsel;
	uint8_t reg_arautoinc;

	uint16_t mag_sample;
	uint16_t temp_sample;
};

/* Put the chip into sleep mode */
static int si7210_sleep(const struct device *dev)
{
	const struct si7210_config *config = dev->config;
	struct si7210_data *data = dev->data;
	int rc;

	/*
	 * Disable measurements during sleep. This overrides the other fields of
	 * the register, but they get reloaded from OTP during wake-up.
	 */
	rc = i2c_reg_write_byte_dt(&config->bus, SI7210_REG_CTRL3, 0);
	if (rc < 0) {
		LOG_ERR("Failed to disable SLTIME");
		return rc;
	}

	/* Go to sleep mode */
	rc = i2c_reg_write_byte_dt(&config->bus, SI7210_REG_POWER_CTRL,
				   SI7210_BIT_POWER_CTRL_SLEEP);
	if (rc < 0) {
		LOG_ERR("Failed to go to sleep mode");
		return rc;
	}

	/* Going to sleep mode resets some registers */
	data->reg_dspsigsel = 0x00;
	data->reg_arautoinc = 0x00;

	return 0;
}

/* Wake a chip from idle or sleep mode */
static int si7210_wakeup(const struct device *dev)
{
	const struct si7210_config *config = dev->config;
	uint8_t val;
	int rc;

	/*
	 * Read one byte from the chip to wake it up. The shorter alternative
	 * is to write a zero byte length message, but it might not be
	 * supported by all I2C controllers.
	 */
	rc = i2c_read_dt(&config->bus, &val, 1);
	if (rc < 0) {
		LOG_ERR("Failed to go wake-up chip");
		return rc;
	}

	return 0;
}

/*
 * The Si7210 device do not have a reset function, but most of the registers
 * are reloaded when exiting from sleep mode.
 */
static int si7210_reset(const struct device *dev)
{
	int rc;

	rc = si7210_sleep(dev);
	if (rc < 0) {
		return rc;
	}

	rc = si7210_wakeup(dev);

	return rc;
}

static int si7210_otp_reg_read_byte(const struct device *dev,
				    uint8_t otp_reg_addr, uint8_t *value)
{
	const struct si7210_config *config = dev->config;
	int rc;

	rc = i2c_reg_write_byte_dt(&config->bus, SI7210_REG_OTP_ADDR, otp_reg_addr);
	if (rc) {
		LOG_ERR("Failed to write OTP address register");
		return rc;
	}

	rc = i2c_reg_write_byte_dt(&config->bus, SI7210_REG_OTP_CTRL, SI7210_BIT_OTP_CTRL_READEN);
	if (rc) {
		LOG_ERR("Failed to write OTP control register");
		return rc;
	}

	/*
	 * No need to check for the data availability (SI7210_REG_OTP_CTRL, bit
	 * !BUSY), as the I2C bus timing ensure the data is available (see
	 * datasheet).
	 */
	rc = i2c_reg_read_byte_dt(&config->bus, SI7210_REG_OTP_DATA, value);
	if (rc) {
		LOG_ERR("Failed to read OTP data register");
		return rc;
	}

	return 0;
}

static int si7210_read_sn(const struct device *dev, uint32_t *sn)
{
	uint32_t val = 0;

	for (int reg = SI7210_OTPREG_SN1; reg <= SI7210_OTPREG_SN4; reg++) {
		uint8_t val8;
		int rc;

		rc = si7210_otp_reg_read_byte(dev, reg, &val8);
		if (rc < 0) {
			return rc;
		}

		val = (val << 8) | val8;
	}

	*sn = val;

	return 0;
}

/* Set the DSPSIGSEL register unless it already has the correct value */
static int si7210_set_dspsigsel(const struct device *dev, uint8_t value)
{
	const struct si7210_config *config = dev->config;
	struct si7210_data *data = dev->data;
	int rc;

	if (data->reg_dspsigsel == value) {
		return 0;
	}

	rc = i2c_reg_write_byte_dt(&config->bus, SI7210_REG_DSPSIGSEL, value);
	if (rc < 0) {
		LOG_ERR("Failed to select channel");
		return rc;
	}

	data->reg_dspsigsel = value;

	return 0;
}

/* Set the ARAUTOINC register unless it already has the correct value */
static int si7210_set_arautoinc(const struct device *dev, uint8_t value)
{
	const struct si7210_config *config = dev->config;
	struct si7210_data *data = dev->data;
	int rc;

	if (data->reg_arautoinc == value) {
		return 0;
	}

	rc = i2c_reg_write_byte_dt(&config->bus, SI7210_REG_ARAUTOINC, value);
	if (rc < 0) {
		LOG_ERR("Failed to set the auto increment register");
		return rc;
	}

	data->reg_arautoinc = value;

	return 0;
}

static int si7210_sample_fetch_one(const struct device *dev, uint8_t channel, uint16_t *dspsig)
{
	const struct si7210_config *config = dev->config;
	uint16_t val;
	int rc;

	/* Select the channel */
	rc = si7210_set_dspsigsel(dev, channel);
	if (rc < 0) {
		return rc;
	}

	/* Enable auto increment to be able to read DSPSIGM and DSPSIGL sequentially */
	rc = si7210_set_arautoinc(dev, 1);
	if (rc < 0) {
		return rc;
	}

	/* Start a single conversion */
	rc = i2c_reg_write_byte_dt(&config->bus, SI7210_REG_POWER_CTRL,
				   SI7210_BIT_POWER_CTRL_ONEBURST);
	if (rc < 0) {
		LOG_ERR("Failed to write power control register");
		return rc;
	}

	/*
	 * No need to wait for the conversion to end, the I2C bus guarantees
	 * the timing (even at 400kHz)
	 */

	/* Read DSPSIG in one burst as auto increment is enabled */
	rc = i2c_burst_read_dt(&config->bus, SI7210_REG_DSPSIGM, (uint8_t *)&val, sizeof(val));
	if (rc < 0) {
		LOG_ERR("Failed to read sample data");
		return rc;
	}

	*dspsig = sys_be16_to_cpu(val) & 0x7fff;

	return 0;
}

static int si7210_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct si7210_data *data = dev->data;
	uint16_t dspsig;
	int rc;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_AMBIENT_TEMP ||
			chan == SENSOR_CHAN_MAGN_Z);

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;
	(void)pm_device_state_get(dev, &state);
	/* Do not allow sample fetching from suspended state */
	if (state == PM_DEVICE_STATE_SUSPENDED)
		return -EIO;
#endif

	/* Prevent going into suspend in the middle of the conversion */
	pm_device_busy_set(dev);

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP) {
		rc = si7210_sample_fetch_one(dev, SI7210_BIT_DSPSIGSEL_TEMP, &dspsig);
		if (rc < 0) {
			return rc;
		}

		data->temp_sample = dspsig >> 3;

	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_MAGN_Z) {
		rc = si7210_sample_fetch_one(dev, SI7210_BIT_DSPSIGSEL_MAG, &dspsig);
		if (rc < 0) {
			return rc;
		}

		data->mag_sample = dspsig;
	}

	pm_device_busy_clear(dev);

	return 0;
}


static int si7210_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct si7210_data *data = dev->data;
	int64_t tmp;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* type conversion */
		tmp = data->temp_sample;

		/* temperature_raw = -3.83e-6 * value^2 + 0.16094 * value - 279.80 */
		tmp = (-383 * tmp * tmp) / 100 + (160940 * tmp) - 279800000;

		/* temperature = (1 + gain / 2048) * temperature_raw + offset / 16 */
		tmp = (tmp * (2048 + data->otp_temp_gain)) / 2048;
		tmp = tmp + (data->otp_temp_offset * 62500);

		/*
		 * Additional offset of -0.222 x VDD. The datasheet recommends
		 * to use VDD = 3.3V if not known.
		 */
		tmp -= 732600;

		val->val1 = tmp / 1000000;
		val->val2 = tmp % 1000000;
		break;
	case SENSOR_CHAN_MAGN_Z:
		/* type conversion */
		tmp = data->mag_sample;

		if (data->scale == si7210_scale_200G) {
			/*
			 * Formula in mT (datasheet) for the 20mT scale: (value - 16384) * 0.00125
			 * => Formula in G for the 200G scale: (value - 16384) * 0.0125
			 */
			tmp = (tmp - 16384) * 12500;
		} else {
			/*
			 * Formula in mT (datasheet) for the 200mT scale: (value - 16384) * 0.0125
			 * => Formula in G for the 2000G scale: (value - 16384) * 0.125
			 */
			tmp = (tmp - 16384) * 1250;
		}

		val->val1 = tmp / 1000000;
		val->val2 = tmp % 1000000;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api si7210_api_funcs = {
	.sample_fetch = si7210_sample_fetch,
	.channel_get = si7210_channel_get,
};

#ifdef CONFIG_PM_DEVICE
static int si7210_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int rc;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Wake-up the chip */
		rc = si7210_wakeup(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Put the chip into sleep mode */
		rc = si7210_sleep(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return rc;
}
#endif /* CONFIG_PM_DEVICE */

static int si7210_init(const struct device *dev)
{
	const struct si7210_config *config = dev->config;
	struct si7210_data *data = dev->data;
	uint8_t chipid, part_base, part_variant;
	uint32_t sn;
	char rev;
	int rc;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C bus %s not ready!", config->bus.bus->name);
		return -ENODEV;
	}

	/* Possibly wakeup device */
	rc = si7210_wakeup(dev);
	if (rc < 0) {
		LOG_ERR("Failed to wake-up device");
		return rc;
	}

	/* Read chip ID */
	rc = i2c_reg_read_byte_dt(&config->bus, SI7210_REG_CHIPID, &chipid);
	if (rc < 0) {
		LOG_ERR("Failed to read chip ID");
		return rc;
	}

	if ((chipid & 0xf0) != 0x10) {
		LOG_ERR("Unsupported device ID");
		return -EINVAL;
	}
	switch (chipid & 0x0f) {
	case 0x04:
		rev = 'B';
		break;
	default:
		LOG_WRN("Unknown revision %d", chipid & 0x0f);
		rev = '.';
	}

	/* Read part number */
	rc = si7210_otp_reg_read_byte(dev, SI7210_OTPREG_PART_BASE, &part_base);
	if (rc < 0) {
		return rc;
	}
	rc = si7210_otp_reg_read_byte(dev, SI7210_OTPREG_PART_VARIANT, &part_variant);
	if (rc < 0) {
		return rc;
	}

	/* Read serial number */
	rc = si7210_read_sn(dev, &sn);
	if (rc < 0) {
		return rc;
	}

	LOG_INF("Found Si72%02d-%c-%02d S/N %08x, at I2C address 0x%x",
		part_base, rev, part_variant, sn, config->bus.addr);

	/* Set default scale depending on the part variant */
	data->scale = (part_variant == 5 || part_variant == 15) ?
		      si7210_scale_2000G : si7210_scale_200G;

	/* Read temperature adjustment values */
	rc = si7210_otp_reg_read_byte(dev, SI7210_OTPREG_TEMP_OFFSET, &data->otp_temp_offset);
	if (rc < 0) {
		return rc;
	}
	rc = si7210_otp_reg_read_byte(dev, SI7210_OTPREG_TEMP_GAIN, &data->otp_temp_gain);
	if (rc < 0) {
		return rc;
	}

	/* Reset the device */
	rc = si7210_reset(dev);
	if (rc < 0) {
		LOG_ERR("Failed to reset the device");
		return rc;
	}

	return 0;
}

/* Main instantiation macro */
#define DEFINE_SI7210(inst) \
	static struct si7210_data si7210_data_##inst; \
	static const struct si7210_config si7210_config_##inst = { \
		.bus = I2C_DT_SPEC_INST_GET(inst), \
	}; \
	PM_DEVICE_DT_INST_DEFINE(inst, si7210_pm_action); \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, si7210_init, PM_DEVICE_DT_INST_GET(inst), \
		&si7210_data_##inst, &si7210_config_##inst, POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, &si7210_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SI7210)
