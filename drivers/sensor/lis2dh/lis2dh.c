/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh


#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(lis2dh, CONFIG_SENSOR_LOG_LEVEL);
#include "lis2dh.h"

#define ACCEL_SCALE(sensitivity)			\
	((SENSOR_G * (sensitivity) >> 14) / 100)

/*
 * Use values for low-power mode in DS "Mechanical (Sensor) characteristics",
 * multiplied by 100.
 */
static uint32_t lis2dh_reg_val_to_scale[] = {
	ACCEL_SCALE(1600),
	ACCEL_SCALE(3200),
	ACCEL_SCALE(6400),
	ACCEL_SCALE(19200),
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

static int lis2dh_sample_fetch_temp(const struct device *dev)
{
	int ret = -ENOTSUP;

#ifdef CONFIG_LIS2DH_MEASURE_TEMPERATURE
	struct lis2dh_data *lis2dh = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	uint8_t raw[sizeof(uint16_t)];

	ret = lis2dh->hw_tf->read_data(dev, cfg->temperature.dout_addr, raw,
				       sizeof(raw));

	if (ret < 0) {
		LOG_WRN("Failed to fetch raw temperature sample");
		ret = -EIO;
	} else {
		/*
		 * The result contains a delta value for the
		 * temperature that must be added to the reference temperature set
		 * for your board to return an absolute temperature in Celsius.
		 *
		 * The data is left aligned.  Fixed point after first 8 bits.
		 */
		lis2dh->temperature.val1 = (int32_t)((int8_t)raw[1]);
		if (cfg->temperature.fractional_bits == 0) {
			lis2dh->temperature.val2 = 0;
		} else {
			lis2dh->temperature.val2 =
				(raw[0] >> (8 - cfg->temperature.fractional_bits));
			lis2dh->temperature.val2 = (lis2dh->temperature.val2 * 1000000);
			lis2dh->temperature.val2 >>= cfg->temperature.fractional_bits;
			if (lis2dh->temperature.val1 < 0) {
				lis2dh->temperature.val2 *= -1;
			}
		}
	}
#else
	LOG_WRN("Temperature measurement disabled");
#endif

	return ret;
}

static int lis2dh_channel_get(const struct device *dev,
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
#ifdef CONFIG_LIS2DH_MEASURE_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP:
		memcpy(val, &lis2dh->temperature, sizeof(*val));
		return 0;
#endif
	default:
		return -ENOTSUP;
	}

	for (i = ofs_start; i <= ofs_end; i++, val++) {
		lis2dh_convert(lis2dh->sample.xyz[i], lis2dh->scale, val);
	}

	return 0;
}

static int lis2dh_fetch_xyz(const struct device *dev,
			       enum sensor_channel chan)
{
	struct lis2dh_data *lis2dh = dev->data;
	int status = -ENODATA;
	size_t i;
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
		status = 0;
	}

	return status;
}

static int lis2dh_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	int status = -ENODATA;

	if (chan == SENSOR_CHAN_ALL) {
		status = lis2dh_fetch_xyz(dev, chan);
#ifdef CONFIG_LIS2DH_MEASURE_TEMPERATURE
		if (status == 0) {
			status = lis2dh_sample_fetch_temp(dev);
		}
#endif
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		status = lis2dh_fetch_xyz(dev, chan);
	} else if (chan == SENSOR_CHAN_DIE_TEMP) {
		status = lis2dh_sample_fetch_temp(dev);
	} else {
		__ASSERT(false, "Invalid sensor channel in fetch");
	}

	return status;
}

#ifdef CONFIG_LIS2DH_ODR_RUNTIME
/* 1620 & 5376 are low power only */
static const uint16_t lis2dh_odr_map[] = {0, 1, 10, 25, 50, 100, 200, 400, 1620,
				       1344, 5376};

static int lis2dh_freq_to_odr_val(uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lis2dh_odr_map); i++) {
		if (freq == lis2dh_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lis2dh_acc_odr_set(const struct device *dev, uint16_t freq)
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

static int lis2dh_acc_range_set(const struct device *dev, int32_t range)
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

static int lis2dh_acc_config(const struct device *dev,
			     enum sensor_channel chan,
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

static int lis2dh_attr_set(const struct device *dev, enum sensor_channel chan,
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

int lis2dh_init(const struct device *dev)
{
	struct lis2dh_data *lis2dh = dev->data;
	const struct lis2dh_config *cfg = dev->config;
	int status;
	uint8_t id;
	uint8_t raw[6];

	status = cfg->bus_init(dev);
	if (status < 0) {
		return status;
	}

	status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_WAI, &id);
	if (status < 0) {
		LOG_ERR("Failed to read chip id.");
		return status;
	}

	if (id != LIS2DH_CHIP_ID) {
		LOG_ERR("Invalid chip ID: %02x\n", id);
		return -EINVAL;
	}

	/* Fix LSM303AGR_ACCEL device scale values */
	if (cfg->hw.is_lsm303agr_dev) {
		lis2dh_reg_val_to_scale[0] = ACCEL_SCALE(1563);
		lis2dh_reg_val_to_scale[1] = ACCEL_SCALE(3126);
		lis2dh_reg_val_to_scale[2] = ACCEL_SCALE(6252);
		lis2dh_reg_val_to_scale[3] = ACCEL_SCALE(18758);
	}

	if (cfg->hw.disc_pull_up) {
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
#ifdef CONFIG_LIS2DH_BLOCK_DATA_UPDATE
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL4,
					  LIS2DH_FS_BITS | LIS2DH_HR_BIT | LIS2DH_CTRL4_BDU_BIT);
#else
	status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL4, LIS2DH_FS_BITS | LIS2DH_HR_BIT);
#endif

	if (status < 0) {
		LOG_ERR("Failed to set full scale ctrl register.");
		return status;
	}

#ifdef CONFIG_LIS2DH_MEASURE_TEMPERATURE
	status = lis2dh->hw_tf->update_reg(dev, cfg->temperature.cfg_addr,
					   cfg->temperature.enable_mask,
					   cfg->temperature.enable_mask);

	if (status < 0) {
		LOG_ERR("Failed to enable temperature measurement");
		return status;
	}
#endif

#ifdef CONFIG_LIS2DH_TRIGGER
	if (cfg->gpio_drdy.port != NULL || cfg->gpio_int.port != NULL) {
		status = lis2dh_init_interrupt(dev);
		if (status < 0) {
			LOG_ERR("Failed to initialize interrupts.");
			return status;
		}
	}
#endif

	LOG_INF("fs=%d, odr=0x%x lp_en=0x%x scale=%d", 1 << (LIS2DH_FS_IDX + 1), LIS2DH_ODR_IDX,
		(uint8_t)LIS2DH_LP_EN_BIT, lis2dh->scale);

	/* enable accel measurements and set power mode and data rate */
	return lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL1,
					LIS2DH_ACCEL_EN_BITS | LIS2DH_LP_EN_BIT |
					LIS2DH_ODR_BITS);
}

#ifdef CONFIG_PM_DEVICE
static int lis2dh_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int status;
	struct lis2dh_data *lis2dh = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Resume previous mode. */
		status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL1,
						  lis2dh->reg_ctrl1_active_val);
		if (status < 0) {
			LOG_ERR("failed to write reg_crtl1");
			return status;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Store current mode, suspend. */
		status = lis2dh->hw_tf->read_reg(dev, LIS2DH_REG_CTRL1,
						 &lis2dh->reg_ctrl1_active_val);
		if (status < 0) {
			LOG_ERR("failed to read reg_crtl1");
			return status;
		}
		status = lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL1,
						  LIS2DH_SUSPEND);
		if (status < 0) {
			LOG_ERR("failed to write reg_crtl1");
			return status;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LIS2DH driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by LIS2DH_DEFINE_SPI() and
 * LIS2DH_DEFINE_I2C().
 */

#define LIS2DH_DEVICE_INIT(inst)					\
	PM_DEVICE_DT_INST_DEFINE(inst, lis2dh_pm_action);		\
	DEVICE_DT_INST_DEFINE(inst,					\
			    lis2dh_init,				\
			    PM_DEVICE_DT_INST_GET(inst),		\
			    &lis2dh_data_##inst,			\
			    &lis2dh_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &lis2dh_driver_api);

#define IS_LSM303AGR_DEV(inst) \
	DT_NODE_HAS_COMPAT(DT_DRV_INST(inst), st_lsm303agr_accel)

#define DISC_PULL_UP(inst) \
	DT_INST_PROP(inst, disconnect_sdo_sa0_pull_up)

#define ANYM_ON_INT1(inst) \
	DT_INST_PROP(inst, anym_on_int1)

#define ANYM_LATCH(inst) \
	!DT_INST_PROP(inst, anym_no_latch)

#define ANYM_MODE(inst) \
	DT_INST_PROP(inst, anym_mode)

#ifdef CONFIG_LIS2DH_TRIGGER
#define GPIO_DT_SPEC_INST_GET_BY_IDX_COND(id, prop, idx)		\
	COND_CODE_1(DT_INST_PROP_HAS_IDX(id, prop, idx),		\
		    (GPIO_DT_SPEC_INST_GET_BY_IDX(id, prop, idx)),	\
		    ({.port = NULL, .pin = 0, .dt_flags = 0}))

#define LIS2DH_CFG_INT(inst)				\
	.gpio_drdy =							\
	    COND_CODE_1(ANYM_ON_INT1(inst),		\
		({.port = NULL, .pin = 0, .dt_flags = 0}),                  \
		(GPIO_DT_SPEC_INST_GET_BY_IDX_COND(inst, irq_gpios, 0))),	\
	.gpio_int =								\
	    COND_CODE_1(ANYM_ON_INT1(inst),		\
		(GPIO_DT_SPEC_INST_GET_BY_IDX_COND(inst, irq_gpios, 0)),	\
		(GPIO_DT_SPEC_INST_GET_BY_IDX_COND(inst, irq_gpios, 1))),
#else
#define LIS2DH_CFG_INT(inst)
#endif /* CONFIG_LIS2DH_TRIGGER */

#ifdef CONFIG_LIS2DH_MEASURE_TEMPERATURE
/* The first 8 bits are the integer portion of the temperature.
 * The result is left justified.  The remainder of the bits are
 * the fractional part.
 *
 * LIS2DH has 8 total bits.
 * LIS2DH12/LIS3DH have 10 bits unless they are in lower power mode.
 * compat(lis2dh) cannot be used here because it is the base part.
 */
#define FRACTIONAL_BITS(inst)	\
	(DT_NODE_HAS_COMPAT(DT_DRV_INST(inst), st_lis2dh12) ||				\
	 DT_NODE_HAS_COMPAT(DT_DRV_INST(inst), st_lis3dh)) ?				\
		      (IS_ENABLED(CONFIG_LIS2DH_OPER_MODE_LOW_POWER) ? 0 : 2) : \
		      0

#define LIS2DH_CFG_TEMPERATURE(inst)	\
	.temperature = { .cfg_addr = 0x1F,	\
			 .enable_mask = 0xC0,		\
			 .dout_addr = 0x0C,			\
			 .fractional_bits = FRACTIONAL_BITS(inst) },
#else
#define LIS2DH_CFG_TEMPERATURE(inst)
#endif /* CONFIG_LIS2DH_MEASURE_TEMPERATURE */

#define LIS2DH_CONFIG_SPI(inst)						\
	{								\
		.bus_init = lis2dh_spi_init,				\
		.bus_cfg = { .spi = SPI_DT_SPEC_INST_GET(inst,		\
					SPI_WORD_SET(8) |		\
					SPI_OP_MODE_MASTER |		\
					SPI_MODE_CPOL |			\
					SPI_MODE_CPHA,			\
					0) },				\
		.hw = { .is_lsm303agr_dev = IS_LSM303AGR_DEV(inst),	\
			.disc_pull_up = DISC_PULL_UP(inst),		\
			.anym_on_int1 = ANYM_ON_INT1(inst),		\
			.anym_latch = ANYM_LATCH(inst),			\
			.anym_mode = ANYM_MODE(inst), },		\
		LIS2DH_CFG_TEMPERATURE(inst)				\
		LIS2DH_CFG_INT(inst)					\
	}

#define LIS2DH_DEFINE_SPI(inst)						\
	static struct lis2dh_data lis2dh_data_##inst;			\
	static const struct lis2dh_config lis2dh_config_##inst =	\
		LIS2DH_CONFIG_SPI(inst);				\
	LIS2DH_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LIS2DH_CONFIG_I2C(inst)						\
	{								\
		.bus_init = lis2dh_i2c_init,				\
		.bus_cfg = { .i2c = I2C_DT_SPEC_INST_GET(inst), },	\
		.hw = { .is_lsm303agr_dev = IS_LSM303AGR_DEV(inst),	\
			.disc_pull_up = DISC_PULL_UP(inst),		\
			.anym_on_int1 = ANYM_ON_INT1(inst),		\
			.anym_latch = ANYM_LATCH(inst),			\
			.anym_mode = ANYM_MODE(inst), },		\
		LIS2DH_CFG_TEMPERATURE(inst)				\
		LIS2DH_CFG_INT(inst)					\
	}

#define LIS2DH_DEFINE_I2C(inst)						\
	static struct lis2dh_data lis2dh_data_##inst;			\
	static const struct lis2dh_config lis2dh_config_##inst =	\
		LIS2DH_CONFIG_I2C(inst);				\
	LIS2DH_DEVICE_INIT(inst)
/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LIS2DH_DEFINE(inst)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (LIS2DH_DEFINE_SPI(inst)),				\
		    (LIS2DH_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(LIS2DH_DEFINE)
