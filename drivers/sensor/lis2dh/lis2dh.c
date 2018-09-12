/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lis2dh.h"

#include <init.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#if defined(CONFIG_LIS2DH_BUS_SPI)
int lis2dh_spi_access(struct lis2dh_data *ctx, u8_t cmd,
		      void *data, size_t length)
{
	const struct spi_buf buf[2] = {
		{
			.buf = &cmd,
			.len = 1
		},
		{
			.buf = data,
			.len = length
		}
	};
	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = 2
	};

	if (cmd & LIS2DH_SPI_READ_BIT) {
		const struct spi_buf_set rx = {
			.buffers = buf,
			.count = 2
		};

		return spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx);
	}

	return spi_write(ctx->spi, &ctx->spi_cfg, &tx);
}
#endif

#if defined(CONFIG_LIS2DH_TRIGGER) || defined(CONFIG_LIS2DH_ACCEL_RANGE_RUNTIME)
int lis2dh_reg_field_update(struct device *dev, u8_t reg_addr,
			    u8_t pos, u8_t mask, u8_t val)
{
	int status;
	u8_t old_val;

	/* just to remove gcc warning */
	old_val = 0;

	status = lis2dh_reg_read_byte(dev, reg_addr, &old_val);
	if (status < 0) {
		return status;
	}

	return lis2dh_reg_write_byte(dev, reg_addr,
				     (old_val & ~mask) | ((val << pos) & mask));
}
#endif

static void lis2dh_convert(s16_t raw_val, u16_t scale,
			   struct sensor_value *val)
{
	s32_t converted_val;

	/*
	 * maximum converted value we can get is: max(raw_val) * max(scale)
	 *	max(raw_val) = +/- 2^15
	 *	max(scale) = 4785
	 *	max(converted_val) = 156794880 which is less than 2^31
	 */
	converted_val = raw_val * scale;
	val->val1 = converted_val / 1000000;
	val->val2 = converted_val % 1000000;

	/* normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1;
		val->val2 += 1000000;
	}
}

static int lis2dh_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct lis2dh_data *lis2dh = dev->driver_data;
	int ofs_start;
	int ofs_end;
	int i;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		ofs_start = ofs_end = 0;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		ofs_start = ofs_end = 1;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		ofs_start = ofs_end = 2;
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		ofs_start = 0;
		ofs_end = 2;
		break;
	default:
		return -ENOTSUP;
	}
	for (i = ofs_start; i <= ofs_end; i++, val++) {
		lis2dh_convert(lis2dh->sample.xyz[i], lis2dh->scale, val);
	}

	return 0;
}

static int lis2dh_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct lis2dh_data *lis2dh = dev->driver_data;
	size_t i;
	int status;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_ACCEL_XYZ);

	/*
	 * since status and all accel data register addresses are consecutive,
	 * a burst read can be used to read all the samples
	 */
	status = lis2dh_burst_read(dev, LIS2DH_REG_STATUS,
				   lis2dh->sample.raw,
				   sizeof(lis2dh->sample.raw));
	if (status < 0) {
		SYS_LOG_WRN("Could not read accel axis data");
		return status;
	}

	for (i = 0; i < (3 * sizeof(s16_t)); i += sizeof(s16_t)) {
		s16_t *sample =
			(s16_t *)&lis2dh->sample.raw[LIS2DH_DATA_OFS + 1 + i];

		*sample = sys_le16_to_cpu(*sample);
	}

	SYS_LOG_INF("status=0x%x x=%d y=%d z=%d", lis2dh->sample.status,
		    lis2dh->sample.xyz[0], lis2dh->sample.xyz[1],
		    lis2dh->sample.xyz[2]);

	if (lis2dh->sample.status & LIS2DH_STATUS_OVR_MASK) {
		return -EBADMSG;
	} else if (lis2dh->sample.status & LIS2DH_STATUS_DRDY_MASK) {
		return 0;
	}

	return -ENODATA;
}

#ifdef CONFIG_LIS2DH_ODR_RUNTIME
/* 1620 & 5376 are low power only */
static const u16_t lis2dh_odr_map[] = {0, 1, 10, 25, 50, 100, 200, 400, 1620,
				       1344, 5376};

static int lis2dh_freq_to_odr_val(u16_t freq)
{
	size_t i;

	/* An ODR of 0 Hz is not allowed */
	if (freq == 0) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(lis2dh_odr_map); i++) {
		if (freq == lis2dh_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lis2dh_acc_odr_set(struct device *dev, u16_t freq)
{
	struct lis2dh_data *lis2dh = dev->driver_data;
	int odr;
	int status;
	u8_t value;

	odr = lis2dh_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	status = lis2dh_reg_read_byte(dev, LIS2DH_REG_CTRL1, &value);
	if (status < 0) {
		return status;
	}

	/* some odr values cannot be set in certain power modes */
	if ((value & LIS2DH_LP_EN_BIT) == 0 && odr == LIS2DH_ODR_8) {
		return -ENOTSUP;
	}

	/* adjust odr index for LP enabled mode, see table above */
	if ((value & LIS2DH_LP_EN_BIT) == 1 && (odr == LIS2DH_ODR_9 + 1)) {
		odr--;
	}

	return lis2dh_reg_write_byte(dev, LIS2DH_REG_CTRL1,
				  (value & ~LIS2DH_ODR_MASK) |
				  LIS2DH_ODR_RATE(odr));
}
#endif

#ifdef CONFIG_LIS2DH_ACCEL_RANGE_RUNTIME
static const union {
	u32_t word_le32;
	u8_t fs_values[4];
} lis2dh_acc_range_map = { .fs_values = {2, 4, 8, 16} };

static int lis2dh_range_to_reg_val(u16_t range)
{
	int i;
	u32_t range_map;

	range_map = sys_le32_to_cpu(lis2dh_acc_range_map.word_le32);

	for (i = 0; range_map; i++, range_map >>= 1) {
		if (range == (range_map & 0xff)) {
			return i;
		}
	}

	return -EINVAL;
}

static int lis2dh_acc_range_set(struct device *dev, s32_t range)
{
	struct lis2dh_data *lis2dh = dev->driver_data;
	int fs;

	fs = lis2dh_range_to_reg_val(range);
	if (fs < 0) {
		return fs;
	}

	lis2dh->scale = LIS2DH_ACCEL_SCALE(range);

	return lis2dh_reg_field_update(dev, LIS2DH_REG_CTRL4,
				       LIS2DH_FS_SHIFT,
				       LIS2DH_FS_MASK,
				       fs);
}
#endif

static int lis2dh_acc_config(struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
#ifdef CONFIG_LIS2DH_ACCEL_RANGE_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		return lis2dh_acc_range_set(dev, sensor_ms2_to_g(val));
#endif
#ifdef CONFIG_LIS2DH_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2dh_acc_odr_set(dev, val->val1);
#endif
#if defined(CONFIG_LIS2DH_TRIGGER)
	case SENSOR_ATTR_SLOPE_TH:
	case SENSOR_ATTR_SLOPE_DUR:
		return lis2dh_acc_slope_config(dev, attr, val);
#endif
	default:
		SYS_LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2dh_attr_set(struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return lis2dh_acc_config(dev, chan, attr, val);
	default:
		SYS_LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lis2dh_driver_api = {
	.attr_set = lis2dh_attr_set,
#if CONFIG_LIS2DH_TRIGGER
	.trigger_set = lis2dh_trigger_set,
#endif
	.sample_fetch = lis2dh_sample_fetch,
	.channel_get = lis2dh_channel_get,
};

int lis2dh_init(struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->driver_data;
	int status;
	u8_t raw[LIS2DH_DATA_OFS + 6];

	status = lis2dh_bus_configure(dev);
	if (status < 0) {
		return status;
	}

	/* Initialize control register ctrl1 to ctrl 6 to default boot values
	 * to avoid warm start/reset issues as the accelerometer has no reset
	 * pin. Register values are retained if power is not removed.
	 * Default values see LIS2DH documentation page 30, chapter 6.
	 */
	(void)memset(raw, 0, sizeof(raw));
	raw[LIS2DH_DATA_OFS] = LIS2DH_ACCEL_EN_BITS;

	status = lis2dh_burst_write(dev, LIS2DH_REG_CTRL1, raw,
				    sizeof(raw));
	if (status < 0) {
		SYS_LOG_ERR("Failed to reset ctrl registers.");
		return status;
	}

	/* set full scale range and store it for later conversion */
	lis2dh->scale = LIS2DH_ACCEL_SCALE(1 << (LIS2DH_FS_IDX + 1));
	status = lis2dh_reg_write_byte(dev, LIS2DH_REG_CTRL4,
				       LIS2DH_FS_BITS);
	if (status < 0) {
		SYS_LOG_ERR("Failed to set full scale ctrl register.");
		return status;
	}

#ifdef CONFIG_LIS2DH_TRIGGER
	status = lis2dh_init_interrupt(dev);
	if (status < 0) {
		SYS_LOG_ERR("Failed to initialize interrupts.");
		return status;
	}
#endif

	SYS_LOG_INF("bus=%s fs=%d, odr=0x%x lp_en=0x%x scale=%d",
		    LIS2DH_BUS_DEV_NAME, 1 << (LIS2DH_FS_IDX + 1),
		    LIS2DH_ODR_IDX, (u8_t)LIS2DH_LP_EN_BIT, lis2dh->scale);

	/* enable accel measurements and set power mode and data rate */
	return lis2dh_reg_write_byte(dev, LIS2DH_REG_CTRL1,
				     LIS2DH_ACCEL_EN_BITS | LIS2DH_LP_EN_BIT |
				     LIS2DH_ODR_BITS);
}

static struct lis2dh_data lis2dh_driver;

DEVICE_AND_API_INIT(lis2dh, CONFIG_LIS2DH_NAME, lis2dh_init, &lis2dh_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &lis2dh_driver_api);
