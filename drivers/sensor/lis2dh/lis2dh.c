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
	default:
		return -ENOTSUP;
	}
	for (i = ofs_start; i <= ofs_end; i++, val++) {
		lis2dh_convert(lis2dh->sample.xyz[i], lis2dh->scale, val);
	}

	return 0;
}

static int lis2dh_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
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

	/* Fix LSM303AGR_ACCEL device scale values */
	if (cfg->is_lsm303agr_dev) {
		lis2dh_reg_val_to_scale[0] = ACCEL_SCALE(1563);
		lis2dh_reg_val_to_scale[1] = ACCEL_SCALE(3126);
		lis2dh_reg_val_to_scale[2] = ACCEL_SCALE(6252);
		lis2dh_reg_val_to_scale[3] = ACCEL_SCALE(18758);
	}

	if (cfg->disc_pull_up) {
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
		    cfg->bus_name, 1 << (LIS2DH_FS_IDX + 1),
		    LIS2DH_ODR_IDX, (uint8_t)LIS2DH_LP_EN_BIT, lis2dh->scale);

	/* enable accel measurements and set power mode and data rate */
	return lis2dh->hw_tf->write_reg(dev, LIS2DH_REG_CTRL1,
					LIS2DH_ACCEL_EN_BITS | LIS2DH_LP_EN_BIT |
					LIS2DH_ODR_BITS);
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LIS2DH driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by LIS2DH_DEFINE_SPI() and
 * LIS2DH_DEFINE_I2C().
 */

#define LIS2DH_DEVICE_INIT(inst)					\
	DEVICE_DT_INST_DEFINE(inst,					\
			    lis2dh_init,				\
			    device_pm_control_nop,			\
			    &lis2dh_data_##inst,			\
			    &lis2dh_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &lis2dh_driver_api);

#define IS_LSM303AGR_DEV(inst) \
	DT_NODE_HAS_COMPAT(DT_DRV_INST(inst), st_lsm303agr_accel)

#define DISC_PULL_UP(inst) \
	DT_INST_PROP(inst, disconnect_sdo_sa0_pull_up)

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#define LIS2DH_HAS_CS(inst) DT_INST_SPI_DEV_HAS_CS_GPIOS(inst)

#define LIS2DH_DATA_SPI_CS(inst)					\
	{ .cs_ctrl = {							\
		.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(inst),		\
		.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(inst),	\
		},							\
	}

#define LIS2DH_DATA_SPI(inst)						\
	COND_CODE_1(LIS2DH_HAS_CS(inst),				\
		    (LIS2DH_DATA_SPI_CS(inst)),				\
		    ({}))

#define LIS2DH_SPI_CS_PTR(inst)						\
	COND_CODE_1(LIS2DH_HAS_CS(inst),				\
		    (&(lis2dh_data_##inst.cs_ctrl)),			\
		    (NULL))

#define LIS2DH_SPI_CS_LABEL(inst)					\
	COND_CODE_1(LIS2DH_HAS_CS(inst),				\
		    (DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst)), (NULL))

#define LIS2DH_SPI_CFG(inst)						\
	(&(struct lis2dh_spi_cfg) {					\
		.spi_conf = {						\
			.frequency =					\
				DT_INST_PROP(inst, spi_max_frequency),	\
			.operation = (SPI_WORD_SET(8) |			\
				      SPI_OP_MODE_MASTER |		\
				      SPI_MODE_CPOL |			\
				      SPI_MODE_CPHA),			\
			.slave = DT_INST_REG_ADDR(inst),		\
			.cs = LIS2DH_SPI_CS_PTR(inst),			\
		},							\
		.cs_gpios_label = LIS2DH_SPI_CS_LABEL(inst),		\
	})

#ifdef CONFIG_LIS2DH_TRIGGER
#define LIS2DH_HAS_IRQ_IDX(inst, idx)					\
		DT_INST_PROP_HAS_IDX(inst, irq_gpios, idx)

#define LIS2DH_CFG_INT(inst) \
	.irq1_dev_name = COND_CODE_1(LIS2DH_HAS_IRQ_IDX(inst, 0),	\
		(DT_INST_GPIO_LABEL_BY_IDX(inst, irq_gpios, 0)),	\
		(NULL)),						\
	.irq1_pin = COND_CODE_1(LIS2DH_HAS_IRQ_IDX(inst, 0),		\
		(DT_INST_GPIO_PIN_BY_IDX(inst, irq_gpios, 0)),		\
		(0)),						\
	.irq1_flags = COND_CODE_1(LIS2DH_HAS_IRQ_IDX(inst, 0),		\
		(DT_INST_GPIO_FLAGS_BY_IDX(inst, irq_gpios, 0)),	\
		(0)),						\
	.irq2_dev_name = COND_CODE_1(LIS2DH_HAS_IRQ_IDX(inst, 1),	\
		(DT_INST_GPIO_LABEL_BY_IDX(inst, irq_gpios, 1)),	\
		(NULL)),						\
	.irq2_pin = COND_CODE_1(LIS2DH_HAS_IRQ_IDX(inst, 1),		\
		(DT_INST_GPIO_PIN_BY_IDX(inst, irq_gpios, 1)),		\
		(0)),						\
	.irq2_flags = COND_CODE_1(LIS2DH_HAS_IRQ_IDX(inst, 1),		\
		(DT_INST_GPIO_FLAGS_BY_IDX(inst, irq_gpios, 1)),	\
		(0)),
#else
#define LIS2DH_CFG_INT(inst)
#endif /* CONFIG_LIS2DH_TRIGGER */

#define LIS2DH_CONFIG_SPI(inst)						\
	{								\
		.bus_name = DT_INST_BUS_LABEL(inst),			\
		.bus_init = lis2dh_spi_init,				\
		.bus_cfg = { .spi_cfg = LIS2DH_SPI_CFG(inst)	},	\
		.is_lsm303agr_dev = IS_LSM303AGR_DEV(inst),		\
		.disc_pull_up = DISC_PULL_UP(inst),			\
		LIS2DH_CFG_INT(inst)					\
	}

#define LIS2DH_DEFINE_SPI(inst)						\
	static struct lis2dh_data lis2dh_data_##inst =			\
		LIS2DH_DATA_SPI(inst);					\
	static const struct lis2dh_config lis2dh_config_##inst =	\
		LIS2DH_CONFIG_SPI(inst);				\
	LIS2DH_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LIS2DH_CONFIG_I2C(inst)						\
	{								\
		.bus_name = DT_INST_BUS_LABEL(inst),			\
		.bus_init = lis2dh_i2c_init,				\
		.bus_cfg = { .i2c_slv_addr = DT_INST_REG_ADDR(inst), },	\
		.is_lsm303agr_dev = IS_LSM303AGR_DEV(inst),		\
		.disc_pull_up = DISC_PULL_UP(inst),			\
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
