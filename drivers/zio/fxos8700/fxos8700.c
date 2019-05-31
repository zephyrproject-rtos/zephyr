/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fxos8700.h"
#include <misc/util.h>
#include <misc/__assert.h>
#include <logging/log.h>
#include <stdlib.h>

#include <zio.h>

#define LOG_LEVEL CONFIG_ZIO_LOG_LEVEL
LOG_MODULE_REGISTER(FXOS8700);

int fxos8700_set_odr(struct device *dev)
{
	const struct fxos8700_config *config = dev->config->config_info;
	struct fxos8700_data *data = dev->driver_data;
	u8_t dr = FXOS8700_CTRLREG1_DR_RATE_400;

	LOG_DBG("Set ODR to 0x%x", dr);

	return i2c_reg_update_byte(data->i2c, config->i2c_address,
				   FXOS8700_REG_CTRLREG1,
				   FXOS8700_CTRLREG1_DR_MASK, (u8_t)dr);
}

static int fxos8700_trigger(struct device *dev)
{
	const struct fxos8700_config *config = dev->config->config_info;
	struct fxos8700_data *data = dev->driver_data;
	struct fxos8700_datum datum;
	int ret = 0;

	/* Read all the channels in one I2C transaction. The number of bytes to
	 * read and the starting register address depend on the mode
	 * configuration (accel-only, mag-only, or hybrid).
	 */
	u32_t num_bytes = config->num_channels
		* FXOS8700_BYTES_PER_CHANNEL_NORMAL;

	__ASSERT(num_bytes <= sizeof(datum), "Too many bytes to read");

	if (i2c_burst_read(data->i2c, config->i2c_address, config->start_addr,
			   (u8_t *)(&datum), num_bytes)) {
		LOG_ERR("Could not fetch sample");
		ret = -EIO;
		goto exit;
	}

#ifdef CONFIG_ZIO_FXOS8700_TEMP
	if (i2c_reg_read_byte(data->i2c, config->i2c_address, FXOS8700_REG_TEMP,
			      &datum->temp)) {
		LOG_ERR("Could not fetch temperature");
		ret = -EIO;
		goto exit;
	}
#endif

	zio_fifo_buf_push(&data->fifo, datum);

exit:
	return ret;
}

int fxos8700_get_power(struct device *dev, enum fxos8700_power *power)
{
	const struct fxos8700_config *config = dev->config->config_info;
	struct fxos8700_data *data = dev->driver_data;
	u8_t val = *power;

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_CTRLREG1,
			      &val)) {
		LOG_ERR("Could not get power setting");
		return -EIO;
	}
	val &= FXOS8700_M_CTRLREG1_MODE_MASK;
	*power = val;

	return 0;
}

int fxos8700_set_power(struct device *dev, enum fxos8700_power power)
{
	const struct fxos8700_config *config = dev->config->config_info;
	struct fxos8700_data *data = dev->driver_data;

	return i2c_reg_update_byte(data->i2c, config->i2c_address,
				   FXOS8700_REG_CTRLREG1,
				   FXOS8700_CTRLREG1_ACTIVE_MASK,
				   power);
}

static int fxos8700_init(struct device *dev)
{
	const struct fxos8700_config *config = dev->config->config_info;
	struct fxos8700_data *data = dev->driver_data;
	struct device *rst;

	/* Get the I2C device */
	data->i2c = device_get_binding(config->i2c_name);
	if (data->i2c == NULL) {
		LOG_ERR("Could not find I2C device");
		return -EINVAL;
	}

	if (config->reset_name) {
		/* Pulse RST pin high to perform a hardware reset of
		 * the sensor.
		 */
		rst = device_get_binding(config->reset_name);
		if (!rst) {
			LOG_ERR("Could not find reset GPIO device");
			return -EINVAL;
		}

		gpio_pin_configure(rst, config->reset_pin, GPIO_DIR_OUT);
		gpio_pin_write(rst, config->reset_pin, 1);
		/* The datasheet does not mention how long to pulse
		 * the RST pin high in order to reset. Stay on the
		 * safe side and pulse for 1 millisecond.
		 */
		k_busy_wait(USEC_PER_MSEC);
		gpio_pin_write(rst, config->reset_pin, 0);
	} else {
		/* Software reset the sensor. Upon issuing a software
		 * reset command over the I2C interface, the sensor
		 * immediately resets and does not send any
		 * acknowledgment (ACK) of the written byte to the
		 * master. Therefore, do not check the return code of
		 * the I2C transaction.
		 */
		i2c_reg_write_byte(data->i2c, config->i2c_address,
				   FXOS8700_REG_CTRLREG2,
				   FXOS8700_CTRLREG2_RST_MASK);
	}

	/* The sensor requires us to wait 1 ms after a reset before
	 * attempting further communications.
	 */
	k_busy_wait(USEC_PER_MSEC);

	/*
	 * Read the WHOAMI register to make sure we are talking to FXOS8700 or
	 * compatible device and not some other type of device that happens to
	 * have the same I2C address.
	 */
	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXOS8700_REG_WHOAMI, &data->whoami)) {
		LOG_ERR("Could not get WHOAMI value");
		return -EIO;
	}

	switch (data->whoami) {
	case WHOAMI_ID_MMA8451:
	case WHOAMI_ID_MMA8652:
	case WHOAMI_ID_MMA8653:
		if (config->mode != FXOS8700_MODE_ACCEL) {
			LOG_ERR("Device 0x%x supports only "
				    "accelerometer mode",
				    data->whoami);
			return -EIO;
		}
		break;
	case WHOAMI_ID_FXOS8700:
		LOG_DBG("Device ID 0x%x", data->whoami);
		break;
	default:
		LOG_ERR("Unknown Device ID 0x%x", data->whoami);
		return -EIO;
	}

	if (fxos8700_set_odr(dev)) {
		LOG_ERR("Could not set default data rate");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_CTRLREG2,
				FXOS8700_CTRLREG2_MODS_MASK,
				config->power_mode)) {
		LOG_ERR("Could not set power scheme");
		return -EIO;
	}

	/* Set the mode (accel-only, mag-only, or hybrid) */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_M_CTRLREG1,
				FXOS8700_M_CTRLREG1_MODE_MASK,
				config->mode)) {
		LOG_ERR("Could not set mode");
		return -EIO;
	}

	/* Set hybrid autoincrement so we can read accel and mag channels in
	 * one I2C transaction.
	 */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_M_CTRLREG2,
				FXOS8700_M_CTRLREG2_AUTOINC_MASK,
				FXOS8700_M_CTRLREG2_AUTOINC_MASK)) {
		LOG_ERR("Could not set hybrid autoincrement");
		return -EIO;
	}

	/* Set the full-scale range */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXOS8700_REG_XYZ_DATA_CFG,
				FXOS8700_XYZ_DATA_CFG_FS_MASK,
				config->range)) {
		LOG_ERR("Could not set range");
		return -EIO;
	}

#if CONFIG_ZIO_FXOS8700_TRIGGER
	if (fxos8700_trigger_init(dev)) {
		LOG_ERR("Could not initialize interrupts");
		return -EIO;
	}
#endif

	/* Set active */
	if (fxos8700_set_power(dev, FXOS8700_POWER_ACTIVE)) {
		LOG_ERR("Could not set active");
		return -EIO;
	}

	LOG_DBG("Init complete");

	return 0;
}

static int fxos8700_attach_buf(struct device *dev, struct zio_buf *buf)
{
	struct fxos8700_data *drv_data = dev->driver_data;

	return zio_fifo_buf_attach(&drv_data->fifo, buf);
}

static int fxos8700_detach_buf(struct device *dev)
{
	struct fxos8700_data *drv_data = dev->driver_data;

	return zio_fifo_buf_detach(&drv_data->fifo);
}

static const struct zio_dev_api fxos8700_driver_api = {
	.trigger = fxos8700_trigger,
	.attach_buf = fxos8700_attach_buf,
	.detach_buf = fxos8700_detach_buf,
};

static const struct fxos8700_config fxos8700_config = {
	.i2c_name = DT_NXP_FXOS8700_0_BUS_NAME,
	.i2c_address = DT_NXP_FXOS8700_0_BASE_ADDRESS,
#ifdef DT_NXP_FXOS8700_0_RESET_GPIOS_CONTROLLER
	.reset_name = DT_NXP_FXOS8700_0_RESET_GPIOS_CONTROLLER,
	.reset_pin = DT_NXP_FXOS8700_0_RESET_GPIOS_PIN,
#else
	.reset_name = NULL,
	.reset_pin = 0,
#endif
#ifdef CONFIG_ZIO_FXOS8700_MODE_ACCEL
	.mode = FXOS8700_MODE_ACCEL,
	.start_addr = FXOS8700_REG_OUTXMSB,
	.start_channel = FXOS8700_CHANNEL_ACCEL_X,
	.num_channels = FXOS8700_NUM_ACCEL_CHANNELS,
#elif CONFIG_ZIO_FXOS8700_MODE_MAGN
	.mode = FXOS8700_MODE_MAGN,
	.start_addr = FXOS8700_REG_M_OUTXMSB,
	.start_channel = FXOS8700_CHANNEL_MAGN_X,
	.num_channels = FXOS8700_NUM_MAG_CHANNELS,
#else
	.mode = FXOS8700_MODE_HYBRID,
	.start_addr = FXOS8700_REG_OUTXMSB,
	.start_channel = FXOS8700_CHANNEL_ACCEL_X,
	.num_channels = FXOS8700_NUM_HYBRID_CHANNELS,
#endif
#if CONFIG_ZIO_FXOS8700_PM_NORMAL
	.power_mode = FXOS8700_PM_NORMAL,
#elif CONFIG_ZIO_FXOS8700_PM_LOW_NOISE_LOW_POWER
	.power_mode = FXOS8700_PM_LOW_NOISE_LOW_POWER,
#elif CONFIG_ZIO_FXOS8700_PM_HIGH_RESOLUTION
	.power_mode = FXOS8700_PM_HIGH_RESOLUTION,
#else
	.power_mode = FXOS8700_PM_LOW_POWER,
#endif
#if CONFIG_ZIO_FXOS8700_RANGE_8G
	.range = FXOS8700_RANGE_8G,
#elif CONFIG_ZIO_FXOS8700_RANGE_4G
	.range = FXOS8700_RANGE_4G,
#else
	.range = FXOS8700_RANGE_2G,
#endif
#ifdef CONFIG_ZIO_FXOS8700_TRIGGER
#ifdef CONFIG_ZIO_FXOS8700_DRDY_INT1
	.gpio_name = DT_NXP_FXOS8700_0_INT1_GPIOS_CONTROLLER,
	.gpio_pin = DT_NXP_FXOS8700_0_INT1_GPIOS_PIN,
#else
	.gpio_name = DT_NXP_FXOS8700_0_INT2_GPIOS_CONTROLLER,
	.gpio_pin = DT_NXP_FXOS8700_0_INT2_GPIOS_PIN,
#endif
#endif
#ifdef CONFIG_ZIO_FXOS8700_PULSE
	.pulse_cfg = CONFIG_ZIO_FXOS8700_PULSE_CFG,
	.pulse_ths[0] = CONFIG_ZIO_FXOS8700_PULSE_THSX,
	.pulse_ths[1] = CONFIG_ZIO_FXOS8700_PULSE_THSY,
	.pulse_ths[2] = CONFIG_ZIO_FXOS8700_PULSE_THSZ,
	.pulse_tmlt = CONFIG_ZIO_FXOS8700_PULSE_TMLT,
	.pulse_ltcy = CONFIG_ZIO_FXOS8700_PULSE_LTCY,
	.pulse_wind = CONFIG_ZIO_FXOS8700_PULSE_WIND,
#endif
};

static struct fxos8700_data fxos8700_data = {
	.fifo = ZIO_FIFO_BUF_INITIALIZER(fxos8700_data.fifo,
			struct fxos8700_datum, CONFIG_ZIO_FXOS8700_FIFO_SIZE),
};

DEVICE_AND_API_INIT(fxos8700, DT_NXP_FXOS8700_0_LABEL, fxos8700_init,
		    &fxos8700_data, &fxos8700_config,
		    POST_KERNEL, CONFIG_ZIO_INIT_PRIORITY,
		    &fxos8700_driver_api);
