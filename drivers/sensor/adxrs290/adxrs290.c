/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <spi.h>

#include "adxrs290.h"

static struct adxrs290_data adxrs290_data;

/**
 * Read from or write to register
 */
static int adxrs290_read_reg(struct device *dev, u8_t address, u8_t *reg)
{
	struct adxrs290_data *data = dev->driver_data;
	int ret;

	/* now combine the address and the command into one byte */
	data->spi_tx_buf[0] = address | ADXRS290_READ;

	spi_slave_select(data->spi, data->spi_slave);

	ret = spi_transceive(data->spi, data->spi_tx_buf, 2,
			     data->spi_rx_buf, 2);
	if (ret) {
		SYS_LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	*reg = data->spi_rx_buf[1];

	return 0;
}

static int adxrs290_read_regs(struct device *dev, u8_t address,
			      u8_t *buffer,
			      int count)
{
	struct adxrs290_data *data = dev->driver_data;
	int ret;

	/* now combine the address and the command into one byte */
	data->spi_tx_buf[0] = address | ADXRS290_READ;

	spi_slave_select(data->spi, data->spi_slave);

	ret = spi_transceive(data->spi, data->spi_tx_buf, 1 + count,
			     data->spi_rx_buf, 1 + count);
	if (ret) {
		SYS_LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	memcpy(buffer, data->spi_rx_buf + 1, count);

	return 0;
}

static int adxrs290_write_reg(struct device *dev, u8_t address,
			      u8_t value)
{
	struct adxrs290_data *data = dev->driver_data;
	int ret;

	/* now combine the register address and the command into one byte: */
	data->spi_tx_buf[0] = address & ADXRS290_WRITE;
	data->spi_tx_buf[1] = value;

	spi_slave_select(data->spi, data->spi_slave);

	ret = spi_transceive(data->spi, data->spi_tx_buf, 2,
			     data->spi_rx_buf, 2);
	if (ret) {
		SYS_LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	return 0;
}

/*****************************************************************************/
/*                                 Gyroscope                                 */
/*****************************************************************************/


/*****************************************************************************/
/*                Methods that can be used in standby Mode                   */
/*****************************************************************************/


static int adxrs290_read_x(struct device *dev, s16_t *x)
{
	u8_t lsb;
	u8_t msb;
	u16_t data;
	int ret;

	ret = adxrs290_read_reg(dev, ADXRS290_GYR_X_L, &lsb);
	if (ret) {
		return ret;
	}

	ret = adxrs290_read_reg(dev, ADXRS290_GYR_X_H, &msb);
	if (ret) {
		return ret;
	}

	data = lsb;
	data |= msb << 8;

	*x = (s16_t)data;

	return 0;
}

static int adxrs290_read_y(struct device *dev, s16_t *y)
{
	u8_t lsb = 0;
	u8_t msb = 0;
	u16_t data = 0;
	int ret;

	ret = adxrs290_read_reg(dev, ADXRS290_GYR_Y_L, &lsb);
	if (ret) {
		return ret;
	}

	ret = adxrs290_read_reg(dev, ADXRS290_GYR_Y_H, &msb);
	if (ret) {
		return ret;
	}

	data = lsb;
	data |= msb << 8;

	*y = (s16_t)data;

	return 0;
}

static int adxrs290_standby_read_temperature(struct device *dev, s16_t *temp)
{
	u8_t lsb;
	u8_t msb;
	u16_t data;
	int ret;

	/* In standby we can read one byte at the time */
	ret = adxrs290_read_reg(dev, ADXRS290_TEMP_L, &lsb);
	if (ret) {
		return ret;
	}

	ret = adxrs290_read_reg(dev, ADXRS290_TEMP_H, &msb);
	if (ret) {
		return ret;
	}

	data = lsb;
	data |= (0xFF & msb) << 8;

	/* CONVERT 12 bit, into 2 complement */
	*temp = (s16_t)data;
	/* Decode Temp measurement */
	*temp /= ADXRS290_TEMP_SCALE_FACT;

	return 0;
}

static int adxrs290_standby_read_xy(struct device *dev, s16_t *x,
				    s16_t *y)
{
	/* In standby we can read one byte at the time
	 * sensitivity 1/200 per LSB
	 */
	int ret;

	ret = adxrs290_read_x(dev, x);
	if (ret) {
		return ret;
	}

	ret = adxrs290_read_y(dev, y);
	if (ret) {
		return ret;
	}

	return 0;
}

static bool adxrs290_check_id(struct device *dev)
{
	int i = 0;
	u32_t sn;
	u8_t reg;
	int ret;

	ret = adxrs290_read_reg(dev, ADXRS290_ANALOG_ID, &reg);
	if (ret || reg != ADXRS290_ANALOG_ID_RETURN) {
		/* Wrong Device ID */
		return false;
	}

	ret = adxrs290_read_reg(dev, ADXRS290_MEMS_ID, &reg);
	if (ret || reg != ADXRS290_MEMS_ID_RETURN) {
		/* Wrong MEMS ID */
		return false;
	}

	ret = adxrs290_read_reg(dev, ADXRS290_DEV_ID, &reg);
	if (ret || reg != ADXRS290_DEV_ID_RETURN) {
		/* Wrong DEV ID */
		return false;
	}

	ret = adxrs290_read_reg(dev, ADXRS290_REV_NUM, &reg);
	if (ret || reg == 0) {
		return false;
	}

	/* reading Serial Number */
	sn = 0;
	for (i = ADXRS290_SERIALNUM_START; i <= ADXRS290_SERIALNUM_END; ++i) {
		ret = adxrs290_read_reg(dev, i, &reg);
		if (ret) {
			return false;
		}
		sn |=  reg << (8 * (i - ADXRS290_SERIALNUM_START));
	}
	if (sn == 0) {
		return false;
	}

	return true;
}

static int adxrs290_set_standby(struct device *dev, u8_t mode)
{
	u8_t data;
	u8_t change = false;
	int ret;

	adxrs290_data.standby = mode;

	ret = adxrs290_read_reg(dev, ADXRS290_POW_CTRL_REG, &data);
	if (ret) {
		return ret;
	}
	if (data & ADXRS290_POW_CTRL_STDBY_MASK) {
		/* current status on -> setting standby */
		if (mode) {
			data &= ~ADXRS290_POW_CTRL_STDBY_MASK;
			change = true;
		}
	} else {
		/* current status off -> setting measurement mode */
		if (!mode) {
			data |= ADXRS290_POW_CTRL_STDBY_MASK;
			change = true;
		}
	}

	if (change) {
		ret = adxrs290_write_reg(dev, ADXRS290_POW_CTRL_REG, data);
		if (ret) {
			return ret;
		}
		k_sleep(K_MSEC(100));
	}

	return 0;
}

static int adxrs290_interrupt_mode_enable(struct device *dev,
					  u8_t activate)
{
	u8_t data;
	int ret;

	ret = adxrs290_read_reg(dev, ADXRS290_DATA_READY_REG, &data);
	if (ret) {
		return ret;
	}

	data &= ~ADXRS290_DATA_READY_INT_MASK;

	if (activate) {
		data |=  0x01;
	}

	ret = adxrs290_write_reg(dev, ADXRS290_DATA_READY_REG, data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxrs290_set_low_pass_filter(struct device *dev, int lf_pole)
{
	u8_t data;
	int ret;

	ret = adxrs290_read_reg(dev, ADXRS290_BANDPASS_FILTER, &data);
	if (ret) {
		return ret;
	}

	data &= ~ADXRS290_BPF_LPF_MASK;
	data |= (lf_pole & ADXRS290_BPF_LPF_MASK);

	ret = adxrs290_write_reg(dev, ADXRS290_BANDPASS_FILTER, data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxrs290_set_high_pass_filter(struct device *dev, int hf_pole)
{
	u8_t data;
	int ret;

	ret = adxrs290_read_reg(dev, ADXRS290_BANDPASS_FILTER, &data);
	if (ret) {
		return ret;
	}

	data &= ~ADXRS290_BPF_HPF_MASK;
	data |= ((hf_pole << ADXRS290_BPF_HPF_OFFSET) &
		 ADXRS290_BPF_HPF_MASK);

	ret = adxrs290_write_reg(dev, ADXRS290_BANDPASS_FILTER, data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxrs290_temp_sensor_enable(struct device *dev, u8_t enable)
{
	u8_t data;
	int ret;

	ret = adxrs290_read_reg(dev, ADXRS290_POW_CTRL_REG, &data);
	if (ret) {
		return ret;
	}

	data &= ~ADXRS290_POW_CTRL_TEMP_EN_MASK;

	if (enable) {
		data |=  ADXRS290_POW_CTRL_TEMP_EN_MASK;
	}

	ret = adxrs290_write_reg(dev, ADXRS290_POW_CTRL_REG, data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxrs290_read_xy(struct device *dev, s16_t *x, s16_t *y)
{
	u8_t buf[4];
	int ret;

	if (adxrs290_data.standby) {
		ret = adxrs290_standby_read_xy(dev, x, y);
		if (ret) {
			return ret;
		}
	}

	ret = adxrs290_read_regs(dev, ADXRS290_GYR_X_L, buf, 4);
	if (ret) {
		return ret;
	}

	*x = buf[2] | (buf[3] << 8);
	*y = buf[0] | (buf[1] << 8);

	return 0;
}

static int adxrs290_read_temperature(struct device *dev, s16_t *temp)
{
	u8_t buf[2];
	int ret;

	if (adxrs290_data.standby) {
		return adxrs290_standby_read_temperature(dev, temp);
	}

	ret = adxrs290_read_regs(dev, ADXRS290_TEMP_L, buf, 2);
	if (ret) {
		return ret;
	}

	*temp = buf[0] | ((0x0F & buf[1]) << 8);

	return 0;
}

static int adxrs290_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct adxrs290_data *data = dev->driver_data;
	int ret;

	ret = adxrs290_read_xy(dev, &data->x, &data->y);
	if (ret) {
		return ret;
	}

	ret = adxrs290_read_temperature(dev, &data->temp);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adxrs290_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct adxrs290_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		/* Angular velocity around the X axis, in radians/s. */
		val->val1 = (s32_t)data->x / ADXRS290_GYR_SCALE_FACT;
		val->val2 = ((s32_t)data->x % ADXRS290_GYR_SCALE_FACT)
			* 1000000;
		break;
	case SENSOR_CHAN_GYRO_Y:
		/* Angular velocity around the Y axis, in radians/s. */
		val->val1 = (s32_t)data->y / ADXRS290_GYR_SCALE_FACT;
		val->val2 = ((s32_t)data->y % ADXRS290_GYR_SCALE_FACT)
			* 1000000;
		break;
	case SENSOR_CHAN_TEMP:
		/* Temperature in degrees Celsius. */
		val->val1 = (s32_t)data->temp / ADXRS290_TEMP_SCALE_FACT;
		val->val2 = ((s32_t)data->temp % ADXRS290_TEMP_SCALE_FACT)
			* 1000000;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api adxrs290_api_funcs = {
	.sample_fetch = adxrs290_sample_fetch,
	.channel_get  = adxrs290_channel_get,
};

static int adxrs290_init(struct device *dev)
{
	struct adxrs290_data *data = dev->driver_data;
	struct spi_config spi_config;
	int ret;

	data->spi = device_get_binding(CONFIG_ADXRS290_SPI_DEV_NAME);
	if (!data->spi) {
		SYS_LOG_DBG("spi device not found: %s",
			    CONFIG_ADXRS290_SPI_DEV_NAME);
		return -EINVAL;
	}

	spi_config.config = SPI_WORD(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL |
			    SPI_MODE_CPHA;
	spi_config.max_sys_freq = 4;
	if (spi_configure(data->spi, &spi_config)) {
		SYS_LOG_DBG("SPI configuration error %s\n",
			    CONFIG_ADXRS290_SPI_DEV_NAME);
		return -EINVAL;
	}

	data->spi_slave = CONFIG_ADXRS290_SPI_DEV_SLAVE;

	if (adxrs290_check_id(dev) == false) {
		return -EINVAL;
	}

	ret = adxrs290_set_low_pass_filter(dev, ADXRS_LPF_80_HZ);
	if (ret) {
		return -EINVAL;
	}

	ret = adxrs290_set_high_pass_filter(dev, ADXRS_HPF_0_350_HZ);
	if (ret) {
		return -EINVAL;
	}

	ret = adxrs290_interrupt_mode_enable(dev, false);
	if (ret) {
		return -EINVAL;
	}

	ret = adxrs290_temp_sensor_enable(dev, true);
	if (ret) {
		return -EINVAL;
	}

	ret = adxrs290_set_standby(dev, false);
	if (ret) {
		return -EINVAL;
	}

	dev->driver_api = &adxrs290_api_funcs;

	return 0;
}

DEVICE_INIT(adxrs290, CONFIG_ADXRS290_DEV_NAME, adxrs290_init, &adxrs290_data,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
