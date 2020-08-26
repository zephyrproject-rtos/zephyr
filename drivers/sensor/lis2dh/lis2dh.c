/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh


#include <init.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(lis2dh, CONFIG_SENSOR_LOG_LEVEL);
#include "lis2dh.h"

#define ACCEL_SCALE(sensitivity)			\
	((SENSOR_G * (sensitivity) >> 14) / 100)

/*
 * Use values for low-power mode in DS "Mechanical (Sensor) characteristics",
 * multiplied by 100.
 */
static const uint32_t lis2dh_reg_val_to_scale[] = {
#if DT_NODE_HAS_STATUS(DT_INST(0, st_lsm303agr_accel), okay)
	ACCEL_SCALE(1563),
	ACCEL_SCALE(3126),
	ACCEL_SCALE(6252),
	ACCEL_SCALE(18758),
#else
	ACCEL_SCALE(1600),
	ACCEL_SCALE(3200),
	ACCEL_SCALE(6400),
	ACCEL_SCALE(19200),
#endif
};

static void lis2dh_convert(int16_t raw_val, uint32_t scale,
			   struct sensor_value *val)
{
	int32_t converted_val;

	/*
	 * maximum converted value we can get is: max(raw_val) * max(scale)
	 *	max(raw_val >> 4) = +/- 2^11
	 *	max(scale) = 114921
	 *	max(converted_val) = 235358208 which is less than 2^31
	 */
	converted_val = (raw_val >> 4) * scale;
	val->val1 = converted_val / 1000000;
	val->val2 = converted_val % 1000000;
}

static int lis2dh_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct lis2dh_data *lis2dh = dev->data;
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
	struct lis2dh_data *lis2dh = dev->data;
	size_t i;
	int status;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_ACCEL_XYZ);

	/*
	 * since status and all accel data register addresses are consecutive,
	 * a burst read can be used to read all the samples
	 */
	status = lis2dh->hw_tf->read_data(dev, LIS2DH_REG_STATUS,
					  lis2dh->sample.raw,
					  sizeof(lis2dh->sample.raw));
	if (status < 0) {
		LOG_WRN("Could not read accel axis data");
		return status;
	}

	for (i = 0; i < (3 * sizeof(int16_t)); i += sizeof(int16_t)) {
		int16_t *sample =
			(int16_t *)&lis2dh->sample.raw[1 + i];

		*sample = sys_le16_to_cpu(*sample);
	}

	if (lis2dh->sample.status & LIS2DH_STATUS_DRDY_MASK) {
		return 0;
	}

	return -ENODATA;
}

#ifdef CONFIG_LIS2DH_ODR_RUNTIME
/* 1620 & 5376 are low power only */
static const uint16_t lis2dh_odr_map[] = {0, 1, 10, 25, 50, 100, 200, 400, 1620,
				       1344, 5376};

static int lis2dh_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	/* An ODR of 0 Hz is not allowed */
	if (freq == 0U) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(lis2dh_odr_map); i++) {
		if (freq == lis2dh_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lis2dh_acc_odr_set(struct device *dev, uint16_t freq)
{
	int odr;
	int status;
	uint8_t value;
	struct lis2dh_data *data = dev->data;

	odr = lis2dh_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	status = data->hw_tf->read_reg(dev, LIS2DH_REG_CTRL1, &value);
	if (status < 0) {
		return status;
	}

	/* some odr values cannot be set in certain power modes */
	if ((value & LIS2DH_LP_EN_BIT_MASK) == 0U && odr == LIS2DH_ODR_8) {
		return -ENOTSUP;
	}

	/* adjust odr index for LP enabled mode, see table above */
	if (((value & LIS2DH_LP_EN_BIT_MASK) == LIS2DH_LP_EN_BIT_MASK) &&
		(odr == LIS2DH_ODR_9 + 1)) {
		odr--;
	}

	return data->hw_tf->write_reg(dev, LIS2DH_REG_CTRL1,
				      (value & ~LIS2DH_ODR_MASK) |
				      LIS2DH_ODR_RATE(odr));
}
#endif

#ifdef CONFIG_LIS2DH_ACCEL_RANGE_RUNTIME

#define LIS2DH_RANGE_IDX_TO_VALUE(idx)		(1 << ((idx) + 1))
#define LIS2DH_NUM_RANGES			4

static int lis2dh_range_to_reg_val(uint16_t range)
{
	int i;

	for (i = 0; i < LIS2DH_NUM_RANGES; i++) {
		if (range == LIS2DH_RANGE_IDX_TO_VALUE(i)) {
			return i;
		}
	}

	return -EINVAL;
}

static int lis2dh_acc_range_set(struct device *dev, int32_t range)
{
	struct lis2dh_data *lis2dh = dev->data;
	int fs;

	fs = lis2dh_range_to_reg_val(range);
	if (fs < 0) {
		return fs;
	}

	lis2dh->scale = lis2dh_reg_val_to_scale[fs];

	return lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL4,
					 LIS2DH_FS_MASK,
					 (fs << LIS2DH_FS_SHIFT));
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
		LOG_DBG("Accel attribute not supported.");
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
		LOG_WRN("attr_set() not supported on this channel.");
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
	struct lis2dh_data *lis2dh = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	int status;
	uint8_t id;
	uint8_t raw[6];

	lis2dh->bus = device_get_binding(cfg->bus_name);
	if (!lis2dh->bus) {
		LOG_ERR("master not found: %s", cfg->bus_name);
		return -EINVAL;
	}

	cfg->bus_init(dev);

	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_WAI, &id);
	if (status < 0) {
		LOG_ERR("Failed to read chip id.");
		return status;
	}

	if (id != LIS2DH_CHIP_ID) {
		LOG_ERR("Invalid chip ID: %02x\n", id);
		return -EINVAL;
	}

	if (IS_ENABLED(DT_INST_PROP(0, disconnect_sdo_sa0_pull_up))) {
		status = lis2dh->hw_tf->update_reg(dev, LIS2DH_REG_CTRL0,
						   LIS2DH_SDO_PU_DISC_MASK,
						   LIS2DH_SDO_PU_DISC_MASK);
		if (status < 0) {
			LOG_ERR("Failed to disconnect SDO/SA0 pull-up.");
			return status;
		}
	}

	/* Initialize control register ctrl1 to ctrl 6 to default boot values
	 * to avoid warm start/reset issues as the accelerometer has no reset
	 * pin. Register values are retained if power is not removed.
	 * Default values see LIS2DH documentation page 30, chapter 6.
	 */
	(void)memset(raw, 0, sizeof(raw));
	raw[0] = LIS2DH_ACCEL_EN_BITS;

	status = lis2dh->hw_tf->write_data(dev, LIS2DH_REG_CTRL1, raw,
					   sizeof(raw));

	if (status < 0) {
		LOG_ERR("Failed to reset ctrl registers.");
		return status;
	}

	/* set full scale range and store it for later conversion */
	lis2dh->scale = lis2dh_reg_val_to_scale[LIS2DH_FS_IDX];
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL4,
					  LIS2DH_FS_BITS | LIS2DH_HR_BIT);
	if (status < 0) {
		LOG_ERR("Failed to set full scale ctrl register.");
		return status;
	}

#ifdef CONFIG_LIS2DH_TRIGGER
	status = lis2dh_init_interrupt(dev);
	if (status < 0) {
		LOG_ERR("Failed to initialize interrupts.");
		return status;
	}
#endif

	LOG_INF("bus=%s fs=%d, odr=0x%x lp_en=0x%x scale=%d",
		    LIS2DH_BUS_DEV_NAME, 1 << (LIS2DH_FS_IDX + 1),
		    LIS2DH_ODR_IDX, (uint8_t)LIS2DH_LP_EN_BIT, lis2dh->scale);

	/* enable accel measurements and set power mode and data rate */
	return lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL1,
					LIS2DH_ACCEL_EN_BITS | LIS2DH_LP_EN_BIT |
					LIS2DH_ODR_BITS);
}

static struct lis2dh_data lis2dh_data;

static const struct lis2dh_config lis2dh_config = {
	.bus_name = DT_INST_BUS_LABEL(0),
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	.bus_init = lis2dh_spi_init,
	.spi_conf.frequency = DT_INST_PROP(0, spi_max_frequency),
	.spi_conf.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
			       SPI_MODE_CPHA | SPI_WORD_SET(8) |
			       SPI_LINES_SINGLE),
	.spi_conf.slave     = DT_INST_REG_ADDR(0),
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.gpio_cs_port	    = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.cs_gpio	    = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.cs_gpio_flags	    = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0),
	.spi_conf.cs        =  &lis2dh_data.cs_ctrl,
#else
	.spi_conf.cs        = NULL,
#endif
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	.bus_init = lis2dh_i2c_init,
	.i2c_slv_addr = DT_INST_REG_ADDR(0),
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

DEVICE_AND_API_INIT(lis2dh, DT_INST_LABEL(0), lis2dh_init, &lis2dh_data,
		    &lis2dh_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &lis2dh_driver_api);
