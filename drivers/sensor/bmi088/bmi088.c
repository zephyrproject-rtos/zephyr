/* Bosch BMI088 inertial measurement unit driver
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * http://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMI088-DS000-07.pdf
 */

#define DT_DRV_COMPAT bosch_bmi088

#include <init.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "bmi088.h"

LOG_MODULE_REGISTER(BMI088, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BMI088 driver enabled without any devices"
#endif

#if BMI088_BUS_SPI
static int bmi088_transceive(const struct device *dev, uint8_t reg,
			     bool write, void *buf, size_t length)
{
	const struct bmi088_cfg *cfg = to_config(dev);
	const struct spi_buf tx_buf[2] = {
		{
			.buf = &reg,
			.len = 1
		},
		{
			.buf = buf,
			.len = length
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = buf ? 2 : 1
	};

	if (!write) {
		const struct spi_buf_set rx = {
			.buffers = tx_buf,
			.count = 2
		};

		return spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	}

	return spi_write_dt(&cfg->bus.spi, &tx);
}

bool bmi088_bus_ready_spi(const struct device *dev)
{
	return spi_is_ready(&to_config(dev)->bus.spi);
}

int bmi088_read_spi(const struct device *dev,
		    uint8_t reg_addr, void *buf, uint8_t len)
{
	return bmi088_transceive(dev, reg_addr | BMI088_REG_READ, false,
				 buf, len);
}

int bmi088_write_spi(const struct device *dev,
		     uint8_t reg_addr, void *buf, uint8_t len)
{
	return bmi088_transceive(dev, reg_addr & BMI088_REG_MASK, true,
				 buf, len);
}


#endif /* BMI088_BUS_SPI */


int bmi088_read(const struct device *dev, uint8_t reg_addr, void *buf,
		uint8_t len)
{
	const struct bmi088_cfg *cfg = to_config(dev);

	return cfg->bus_io->read(dev, reg_addr, buf, len);
}

int bmi088_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte)
{
	return bmi088_read(dev, reg_addr, byte, 1);
}

static int bmi088_word_read(const struct device *dev, uint8_t reg_addr,
			    uint16_t *word)
{
	int rc;

	rc = bmi088_read(dev, reg_addr, word, 2);
	if (rc != 0) {
		return rc;
	}

	*word = sys_le16_to_cpu(*word);

	return 0;
}

int bmi088_write(const struct device *dev, uint8_t reg_addr, void *buf,
		 uint8_t len)
{
	const struct bmi088_cfg *cfg = to_config(dev);

	return cfg->bus_io->write(dev, reg_addr, buf, len);
}

int bmi088_byte_write(const struct device *dev, uint8_t reg_addr,
		      uint8_t byte)
{
	return bmi088_write(dev, reg_addr & BMI088_REG_MASK, &byte, 1);
}

int bmi088_word_write(const struct device *dev, uint8_t reg_addr,
		      uint16_t word)
{
	uint8_t tx_word[2] = {
		(uint8_t)(word & 0xff),
		(uint8_t)(word >> 8)
	};

	return bmi088_write(dev, reg_addr & BMI088_REG_MASK, tx_word, 2);
}

int bmi088_reg_field_update(const struct device *dev, uint8_t reg_addr,
			    uint8_t pos, uint8_t mask, uint8_t val)
{
	uint8_t old_val;

	if (bmi088_byte_read(dev, reg_addr, &old_val) < 0) {
		return -EIO;
	}

	return bmi088_byte_write(dev, reg_addr,
				 (old_val & ~mask) | ((val << pos) & mask));
}

static int bmi088_do_calibration(const struct device *dev, uint8_t foc_conf)
{
	if (bmi088_byte_write(dev, BMI088_REG_FOC_CONF, foc_conf) < 0) {
		return -EIO;
	}

	if (bmi088_byte_write(dev, BMI088_REG_CMD, BMI088_CMD_START_FOC) < 0) {
		return -EIO;
	}

	k_busy_wait(250000); /* calibration takes a maximum of 250ms */

	return 0;
}

static int bmi088_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	return -ENOTSUP;
}

static int bmi088_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct bmi088_data *data = to_data(dev);
	uint8_t status;
	size_t i;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	status = 0;
	while ((status & BMI088_DATA_READY_BIT_MASK) == 0) {

		if (bmi088_byte_read(dev, BMI088_REG_STATUS, &status) < 0) {
			return -EIO;
		}
	}

	if (bmi088_read(dev, BMI088_SAMPLE_BURST_READ_ADDR, data->sample.raw,
			BMI088_BUF_SIZE) < 0) {
		return -EIO;
	}

	/* convert samples to cpu endianness */
	for (i = 0; i < BMI088_SAMPLE_SIZE; i += 2) {
		uint16_t *sample =
			(uint16_t *) &data->sample.raw[i];

		*sample = sys_le16_to_cpu(*sample);
	}

	return 0;
}

static void bmi088_to_fixed_point(int16_t raw_val, uint16_t scale,
				  struct sensor_value *val)
{
	int32_t converted_val;

	/*
	 * maximum converted value we can get is: max(raw_val) * max(scale)
	 *	max(raw_val) = +/- 2^15
	 *	max(scale) = 4785
	 *	max(converted_val) = 156794880 which is less than 2^31
	 */
	converted_val = raw_val * scale;
	val->val1 = converted_val / 1000000;
	val->val2 = converted_val % 1000000;
}

static void bmi088_channel_convert(enum sensor_channel chan,
				   uint16_t scale,
				   uint16_t *raw_xyz,
				   struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_GYRO_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_GYRO_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop ; i++, val++) {
		bmi088_to_fixed_point(raw_xyz[i], scale, val);
	}
}

#if !defined(CONFIG_BMI088_GYRO_PMU_SUSPEND)
static inline void bmi088_gyr_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi088_data *data = to_data(dev);

	bmi088_channel_convert(chan, data->scale.gyr,
			       data->sample.gyr, val);
}
#endif


static int bmi088_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	switch (chan) {
#if !defined(CONFIG_BMI088_GYRO_PMU_SUSPEND)
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		bmi088_gyr_channel_get(dev, chan, val);
		return 0;
#endif

	default:
		LOG_DBG("Channel not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api bmi088_api = {
	.attr_set = bmi088_attr_set,
	.sample_fetch = bmi088_sample_fetch,
	.channel_get = bmi088_channel_get,
};

int bmi088_init(const struct device *dev)
{
	const struct bmi088_cfg *cfg = to_config(dev);
	struct bmi088_data *data = to_data(dev);
	uint8_t val = 0U;
	int32_t acc_range, gyr_range;

	if (!cfg->bus_io->ready(dev)) {
		LOG_ERR("Bus not ready");
		return -EINVAL;
	}

	/* reboot the chip */
	if (bmi088_byte_write(dev, BMI088_REG_CMD, BMI088_CMD_SOFT_RESET) < 0) {
		LOG_DBG("Cannot reboot chip.");
		return -EIO;
	}

	k_busy_wait(1000);

	/* do a dummy read from 0x7F to activate SPI */
	if (bmi088_byte_read(dev, BMI088_SPI_START, &val) < 0) {
		LOG_DBG("Cannot read from 0x7F..");
		return -EIO;
	}

	k_busy_wait(100);

	if (bmi088_byte_read(dev, BMI088_REG_CHIPID, &val) < 0) {
		LOG_DBG("Failed to read chip id.");
		return -EIO;
	}

	if (val != BMI088_CHIP_ID) {
		LOG_DBG("Unsupported chip detected (0x%x)!", val);
		return -ENODEV;
	}

	/* set gyro default range */
	if (bmi088_byte_write(dev, BMI088_REG_GYR_RANGE,
			      BMI088_DEFAULT_RANGE_GYR) < 0) {
		LOG_DBG("Cannot set default range for gyroscope.");
		return -EIO;
	}

	gyr_range = bmi088_gyr_reg_val_to_range(BMI088_DEFAULT_RANGE_GYR);

	data->scale.gyr = BMI088_GYR_SCALE(gyr_range);

	if (bmi088_reg_field_update(dev, BMI088_REG_ACC_CONF,
				    BMI088_ACC_CONF_ODR_POS,
				    BMI088_ACC_CONF_ODR_MASK,
				    BMI088_DEFAULT_ODR_ACC) < 0) {
		LOG_DBG("Failed to set accel's default ODR.");
		return -EIO;
	}

	if (bmi088_reg_field_update(dev, BMI088_REG_GYR_CONF,
				    BMI088_GYR_CONF_ODR_POS,
				    BMI088_GYR_CONF_ODR_MASK,
				    BMI088_DEFAULT_ODR_GYR) < 0) {
		LOG_DBG("Failed to set gyro's default ODR.");
		return -EIO;
	}
	return 0;
}

#define BMI088_DEVICE_INIT(inst)					\
	DEVICE_DT_INST_DEFINE(inst, bmi088_init, NULL,			\
			      &bmi088_data_##inst, &bmi088_cfg_##inst,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
			      &bmi088_api);

/* Instantiation macros used when a device is on a SPI bus */
#define BMI088_DEFINE_SPI(inst)						   \
	static struct bmi088_data bmi088_data_##inst;			   \
	static const struct bmi088_cfg bmi088_cfg_##inst = {		   \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0), \
	};								   \
	BMI088_DEVICE_INIT(inst)


/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define BMI088_DEFINE(inst)						\
		    (BMI088_DEFINE_SPI(inst))			\

DT_INST_FOREACH_STATUS_OKAY(BMI088_DEFINE)
