/* Bosch BMI160 inertial measurement unit driver
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * http://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMI160-DS000-07.pdf
 */

#define DT_DRV_COMPAT bosch_bmi160

#include <init.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "bmi160.h"

LOG_MODULE_REGISTER(BMI160, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BMI160 driver enabled without any devices"
#endif

#if BMI160_BUS_SPI
static int bmi160_transceive(const struct device *dev, uint8_t reg,
			     bool write, void *buf, size_t length)
{
	const struct bmi160_cfg *cfg = to_config(dev);
	struct bmi160_data *data = to_data(dev);
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

		return spi_transceive(data->bus, cfg->bus_cfg.spi_cfg, &tx,
				      &rx);
	}

	return spi_write(data->bus, cfg->bus_cfg.spi_cfg, &tx);
}

int bmi160_read_spi(const struct device *dev,
		    const struct bmi160_bus_cfg *bus_config, uint8_t reg_addr,
		    void *buf, uint8_t len)
{
	return bmi160_transceive(dev, reg_addr | BMI160_REG_READ, false,
				 buf, len);
}

int bmi160_write_spi(const struct device *dev,
		     const struct bmi160_bus_cfg *bus_config,
		     uint8_t reg_addr, void *buf, uint8_t len)
{
	return bmi160_transceive(dev, reg_addr & BMI160_REG_MASK, true,
				 buf, len);
}

static const struct bmi160_reg_io bmi160_reg_io_spi = {
	.read = bmi160_read_spi,
	.write = bmi160_write_spi,
};
#endif /* BMI160_BUS_SPI */

#if BMI160_BUS_I2C
int bmi160_read_i2c(const struct device *dev,
		    const struct bmi160_bus_cfg *bus_config, uint8_t reg_addr,
		    void *buf, uint8_t len)
{
	struct bmi160_data *data = to_data(dev);

	return i2c_burst_read(data->bus, bus_config->i2c_addr, reg_addr, buf,
			      len);
}

int bmi160_write_i2c(const struct device *dev,
		     const struct bmi160_bus_cfg *bus_config,
		     uint8_t reg_addr, void *buf, uint8_t len)
{
	struct bmi160_data *data = to_data(dev);

	return i2c_burst_write(data->bus, bus_config->i2c_addr, reg_addr, buf,
			       len);
}

static const struct bmi160_reg_io bmi160_reg_io_i2c = {
	.read = bmi160_read_i2c,
	.write = bmi160_write_i2c,
};
#endif

int bmi160_read(const struct device *dev, uint8_t reg_addr, void *buf,
		uint8_t len)
{
	const struct bmi160_cfg *cfg = to_config(dev);

	return cfg->reg_io->read(dev, &cfg->bus_cfg, reg_addr, buf, len);
}

int bmi160_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte)
{
	return bmi160_read(dev, reg_addr, byte, 1);
}

static int bmi160_word_read(const struct device *dev, uint8_t reg_addr,
			    uint16_t *word)
{
	int rc;

	rc = bmi160_read(dev, reg_addr, word, 2);
	if (rc != 0) {
		return rc;
	}

	*word = sys_le16_to_cpu(*word);

	return 0;
}

int bmi160_write(const struct device *dev, uint8_t reg_addr, void *buf,
		 uint8_t len)
{
	const struct bmi160_cfg *cfg = to_config(dev);

	return cfg->reg_io->write(dev, &cfg->bus_cfg, reg_addr, buf, len);
}

int bmi160_byte_write(const struct device *dev, uint8_t reg_addr,
		      uint8_t byte)
{
	return bmi160_write(dev, reg_addr & BMI160_REG_MASK, &byte, 1);
}

int bmi160_word_write(const struct device *dev, uint8_t reg_addr,
		      uint16_t word)
{
	uint8_t tx_word[2] = {
		(uint8_t)(word & 0xff),
		(uint8_t)(word >> 8)
	};

	return bmi160_write(dev, reg_addr & BMI160_REG_MASK, tx_word, 2);
}

int bmi160_reg_field_update(const struct device *dev, uint8_t reg_addr,
			    uint8_t pos, uint8_t mask, uint8_t val)
{
	uint8_t old_val;

	if (bmi160_byte_read(dev, reg_addr, &old_val) < 0) {
		return -EIO;
	}

	return bmi160_byte_write(dev, reg_addr,
				 (old_val & ~mask) | ((val << pos) & mask));
}

static int bmi160_pmu_set(const struct device *dev,
			  union bmi160_pmu_status *pmu_sts)
{
	struct {
		uint8_t cmd;
		uint16_t delay_us; /* values taken from page 82 */
	} cmds[] = {
		{BMI160_CMD_PMU_MAG | pmu_sts->mag, 350},
		{BMI160_CMD_PMU_ACC | pmu_sts->acc, 3200},
		{BMI160_CMD_PMU_GYR | pmu_sts->gyr, 55000}
	};
	size_t i;

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		union bmi160_pmu_status sts;
		bool pmu_set = false;

		if (bmi160_byte_write(dev, BMI160_REG_CMD, cmds[i].cmd) < 0) {
			return -EIO;
		}

		/*
		 * Cannot use a timer here since this is called from the
		 * init function and the timeouts were not initialized yet.
		 */
		k_busy_wait(cmds[i].delay_us);

		/* make sure the PMU_STATUS was set, though */
		do {
			if (bmi160_byte_read(dev, BMI160_REG_PMU_STATUS,
					     &sts.raw) < 0) {
				return -EIO;
			}

			if (i == 0) {
				pmu_set = (pmu_sts->mag == sts.mag);
			} else if (i == 1) {
				pmu_set = (pmu_sts->acc == sts.acc);
			} else {
				pmu_set = (pmu_sts->gyr == sts.gyr);
			}

		} while (!pmu_set);
	}

	/* set the undersampling flag for accelerometer */
	return bmi160_reg_field_update(dev, BMI160_REG_ACC_CONF,
				       BMI160_ACC_CONF_US_POS, BMI160_ACC_CONF_US_MASK,
				       pmu_sts->acc != BMI160_PMU_NORMAL);
}

#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME) ||\
	defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
/*
 * Output data rate map with allowed frequencies:
 * freq = freq_int + freq_milli / 1000
 *
 * Since we don't need a finer frequency resolution than milliHz, use uint16_t
 * to save some flash.
 */
struct {
	uint16_t freq_int;
	uint16_t freq_milli; /* User should convert to uHz before setting the
			      * SENSOR_ATTR_SAMPLING_FREQUENCY attribute.
			      */
} bmi160_odr_map[] = {
	{0,    0  }, {0,     780}, {1,     562}, {3,    120}, {6,   250},
	{12,   500}, {25,    0  }, {50,    0  }, {100,  0  }, {200, 0  },
	{400,  0  }, {800,   0  }, {1600,  0  }, {3200, 0  },
};

static int bmi160_freq_to_odr_val(uint16_t freq_int, uint16_t freq_milli)
{
	size_t i;

	/* An ODR of 0 Hz is not allowed */
	if (freq_int == 0U && freq_milli == 0U) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(bmi160_odr_map); i++) {
		if (freq_int < bmi160_odr_map[i].freq_int ||
		    (freq_int == bmi160_odr_map[i].freq_int &&
		     freq_milli <= bmi160_odr_map[i].freq_milli)) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

#if defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
static int bmi160_acc_odr_set(const struct device *dev, uint16_t freq_int,
			      uint16_t freq_milli)
{
	struct bmi160_data *data = to_data(dev);
	int odr = bmi160_freq_to_odr_val(freq_int, freq_milli);

	if (odr < 0) {
		return odr;
	}

	/* some odr values cannot be set in certain power modes */
	if ((data->pmu_sts.acc == BMI160_PMU_NORMAL &&
	     odr < BMI160_ODR_25_2) ||
	    (data->pmu_sts.acc == BMI160_PMU_LOW_POWER &&
	    odr < BMI160_ODR_25_32) || odr > BMI160_ODR_1600) {
		return -ENOTSUP;
	}

	return bmi160_reg_field_update(dev, BMI160_REG_ACC_CONF,
				       BMI160_ACC_CONF_ODR_POS,
				       BMI160_ACC_CONF_ODR_MASK,
				       (uint8_t) odr);
}
#endif

static const struct bmi160_range bmi160_acc_range_map[] = {
	{2,	BMI160_ACC_RANGE_2G},
	{4,	BMI160_ACC_RANGE_4G},
	{8,	BMI160_ACC_RANGE_8G},
	{16,	BMI160_ACC_RANGE_16G},
};
#define BMI160_ACC_RANGE_MAP_SIZE	ARRAY_SIZE(bmi160_acc_range_map)

static const struct bmi160_range bmi160_gyr_range_map[] = {
	{2000,	BMI160_GYR_RANGE_2000DPS},
	{1000,	BMI160_GYR_RANGE_1000DPS},
	{500,	BMI160_GYR_RANGE_500DPS},
	{250,	BMI160_GYR_RANGE_250DPS},
	{125,	BMI160_GYR_RANGE_125DPS},
};
#define BMI160_GYR_RANGE_MAP_SIZE	ARRAY_SIZE(bmi160_gyr_range_map)

#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME) ||\
	defined(CONFIG_BMI160_GYRO_RANGE_RUNTIME)
static int32_t bmi160_range_to_reg_val(uint16_t range,
				       const struct bmi160_range *range_map,
				       uint16_t range_map_size)
{
	int i;

	for (i = 0; i < range_map_size; i++) {
		if (range <= range_map[i].range) {
			return range_map[i].reg_val;
		}
	}

	return -EINVAL;
}
#endif

static int32_t bmi160_reg_val_to_range(uint8_t reg_val,
				       const struct bmi160_range *range_map,
				       uint16_t range_map_size)
{
	int i;

	for (i = 0; i < range_map_size; i++) {
		if (reg_val == range_map[i].reg_val) {
			return range_map[i].range;
		}
	}

	return -EINVAL;
}

int32_t bmi160_acc_reg_val_to_range(uint8_t reg_val)
{
	return bmi160_reg_val_to_range(reg_val, bmi160_acc_range_map,
				       BMI160_ACC_RANGE_MAP_SIZE);
}

int32_t bmi160_gyr_reg_val_to_range(uint8_t reg_val)
{
	return bmi160_reg_val_to_range(reg_val, bmi160_gyr_range_map,
				       BMI160_GYR_RANGE_MAP_SIZE);
}

static int bmi160_do_calibration(const struct device *dev, uint8_t foc_conf)
{
	if (bmi160_byte_write(dev, BMI160_REG_FOC_CONF, foc_conf) < 0) {
		return -EIO;
	}

	if (bmi160_byte_write(dev, BMI160_REG_CMD, BMI160_CMD_START_FOC) < 0) {
		return -EIO;
	}

	k_busy_wait(250000); /* calibration takes a maximum of 250ms */

	return 0;
}

#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME)
static int bmi160_acc_range_set(const struct device *dev, int32_t range)
{
	struct bmi160_data *data = to_data(dev);
	int32_t reg_val = bmi160_range_to_reg_val(range,
						  bmi160_acc_range_map,
						  BMI160_ACC_RANGE_MAP_SIZE);

	if (reg_val < 0) {
		return reg_val;
	}

	if (bmi160_byte_write(dev, BMI160_REG_ACC_RANGE, reg_val & 0xff) < 0) {
		return -EIO;
	}

	data->scale.acc = BMI160_ACC_SCALE(range);

	return 0;
}
#endif

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
/*
 * Accelerometer offset scale, taken from pg. 79, converted to micro m/s^2:
 *	3.9 * 9.80665 * 1000
 */
#define BMI160_ACC_OFS_LSB		38246
static int bmi160_acc_ofs_set(const struct device *dev,
			      enum sensor_channel chan,
			      const struct sensor_value *ofs)
{
	uint8_t reg_addr[] = {
		BMI160_REG_OFFSET_ACC_X,
		BMI160_REG_OFFSET_ACC_Y,
		BMI160_REG_OFFSET_ACC_Z
	};
	int i;
	int32_t ofs_u;
	int8_t reg_val;

	/* we need the offsets for all axis */
	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	for (i = 0; i < BMI160_AXES; i++, ofs++) {
		/* convert ofset to micro m/s^2 */
		ofs_u = ofs->val1 * 1000000ULL + ofs->val2;
		reg_val = ofs_u / BMI160_ACC_OFS_LSB;

		if (bmi160_byte_write(dev, reg_addr[i], reg_val) < 0) {
			return -EIO;
		}
	}

	/* activate accel HW compensation */
	return bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN,
				       BMI160_ACC_OFS_EN_POS,
				       BIT(BMI160_ACC_OFS_EN_POS), 1);
}

static int  bmi160_acc_calibrate(const struct device *dev,
				 enum sensor_channel chan,
				 const struct sensor_value *xyz_calib_value)
{
	struct bmi160_data *data = to_data(dev);
	uint8_t foc_pos[] = {
		BMI160_FOC_ACC_X_POS,
		BMI160_FOC_ACC_Y_POS,
		BMI160_FOC_ACC_Z_POS,
	};
	int i;
	uint8_t reg_val = 0U;

	/* Calibration has to be done in normal mode. */
	if (data->pmu_sts.acc != BMI160_PMU_NORMAL) {
		return -ENOTSUP;
	}

	/*
	 * Hardware calibration is done knowing the expected values on all axis.
	 */
	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	for (i = 0; i < BMI160_AXES; i++, xyz_calib_value++) {
		int32_t accel_g;
		uint8_t accel_val;

		accel_g = sensor_ms2_to_g(xyz_calib_value);
		if (accel_g == 0) {
			accel_val = 3U;
		} else if (accel_g == 1) {
			accel_val = 1U;
		} else if (accel_g == -1) {
			accel_val = 2U;
		} else {
			accel_val = 0U;
		}
		reg_val |= (accel_val << foc_pos[i]);
	}

	if (bmi160_do_calibration(dev, reg_val) < 0) {
		return -EIO;
	}

	/* activate accel HW compensation */
	return bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN,
				       BMI160_ACC_OFS_EN_POS,
				       BIT(BMI160_ACC_OFS_EN_POS), 1);
}

static int bmi160_acc_config(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		return bmi160_acc_range_set(dev, sensor_ms2_to_g(val));
#endif
#if defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return bmi160_acc_odr_set(dev, val->val1, val->val2 / 1000);
#endif
	case SENSOR_ATTR_OFFSET:
		return bmi160_acc_ofs_set(dev, chan, val);
	case SENSOR_ATTR_CALIB_TARGET:
		return bmi160_acc_calibrate(dev, chan, val);
#if defined(CONFIG_BMI160_TRIGGER)
	case SENSOR_ATTR_SLOPE_TH:
	case SENSOR_ATTR_SLOPE_DUR:
		return bmi160_acc_slope_config(dev, attr, val);
#endif
	default:
		LOG_DBG("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND) */

#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME)
static int bmi160_gyr_odr_set(const struct device *dev, uint16_t freq_int,
			      uint16_t freq_milli)
{
	int odr = bmi160_freq_to_odr_val(freq_int, freq_milli);

	if (odr < 0) {
		return odr;
	}

	if (odr < BMI160_ODR_25 || odr > BMI160_ODR_3200) {
		return -ENOTSUP;
	}

	return bmi160_reg_field_update(dev, BMI160_REG_GYR_CONF,
				       BMI160_GYR_CONF_ODR_POS,
				       BMI160_GYR_CONF_ODR_MASK,
				       (uint8_t) odr);
}
#endif

#if defined(CONFIG_BMI160_GYRO_RANGE_RUNTIME)
static int bmi160_gyr_range_set(const struct device *dev, uint16_t range)
{
	struct bmi160_data *data = to_data(dev);
	int32_t reg_val = bmi160_range_to_reg_val(range,
						bmi160_gyr_range_map,
						BMI160_GYR_RANGE_MAP_SIZE);

	if (reg_val < 0) {
		return reg_val;
	}

	if (bmi160_byte_write(dev, BMI160_REG_GYR_RANGE, reg_val) < 0) {
		return -EIO;
	}

	data->scale.gyr = BMI160_GYR_SCALE(range);

	return 0;
}
#endif

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
/*
 * Gyro offset scale, taken from pg. 79, converted to micro rad/s:
 *		0.061 * (pi / 180) * 1000000, where pi = 3.141592
 */
#define BMI160_GYR_OFS_LSB		1065
static int bmi160_gyr_ofs_set(const struct device *dev,
			      enum sensor_channel chan,
			      const struct sensor_value *ofs)
{
	struct {
		uint8_t lsb_addr;
		uint8_t msb_pos;
	} ofs_desc[] = {
		{BMI160_REG_OFFSET_GYR_X, BMI160_GYR_MSB_OFS_X_POS},
		{BMI160_REG_OFFSET_GYR_Y, BMI160_GYR_MSB_OFS_Y_POS},
		{BMI160_REG_OFFSET_GYR_Z, BMI160_GYR_MSB_OFS_Z_POS},
	};
	int i;
	int32_t ofs_u;
	int16_t val;

	/* we need the offsets for all axis */
	if (chan != SENSOR_CHAN_GYRO_XYZ) {
		return -ENOTSUP;
	}

	for (i = 0; i < BMI160_AXES; i++, ofs++) {
		/* convert offset to micro rad/s */
		ofs_u = ofs->val1 * 1000000ULL + ofs->val2;

		val = ofs_u / BMI160_GYR_OFS_LSB;

		/*
		 * The gyro offset is a 10 bit two-complement value. Make sure
		 * the passed value is within limits.
		 */
		if (val < -512 || val > 512) {
			return -EINVAL;
		}

		/* write the LSB */
		if (bmi160_byte_write(dev, ofs_desc[i].lsb_addr,
				      val & 0xff) < 0) {
			return -EIO;
		}

		/* write the MSB */
		if (bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN,
					    ofs_desc[i].msb_pos,
					    0x3 << ofs_desc[i].msb_pos,
					    (val >> 8) & 0x3) < 0) {
			return -EIO;
		}
	}

	/* activate gyro HW compensation */
	return bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN,
				       BMI160_GYR_OFS_EN_POS,
				       BIT(BMI160_GYR_OFS_EN_POS), 1);
}

static int bmi160_gyr_calibrate(const struct device *dev,
				enum sensor_channel chan)
{
	struct bmi160_data *data = to_data(dev);

	ARG_UNUSED(chan);

	/* Calibration has to be done in normal mode. */
	if (data->pmu_sts.gyr != BMI160_PMU_NORMAL) {
		return -ENOTSUP;
	}

	if (bmi160_do_calibration(dev, BIT(BMI160_FOC_GYR_EN_POS)) < 0) {
		return -EIO;
	}

	/* activate gyro HW compensation */
	return bmi160_reg_field_update(dev, BMI160_REG_OFFSET_EN,
				       BMI160_GYR_OFS_EN_POS,
				       BIT(BMI160_GYR_OFS_EN_POS), 1);
}

static int bmi160_gyr_config(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	switch (attr) {
#if defined(CONFIG_BMI160_GYRO_RANGE_RUNTIME)
	case SENSOR_ATTR_FULL_SCALE:
		return bmi160_gyr_range_set(dev, sensor_rad_to_degrees(val));
#endif
#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return bmi160_gyr_odr_set(dev, val->val1, val->val2 / 1000);
#endif
	case SENSOR_ATTR_OFFSET:
		return bmi160_gyr_ofs_set(dev, chan, val);

	case SENSOR_ATTR_CALIB_TARGET:
		return bmi160_gyr_calibrate(dev, chan);

	default:
		LOG_DBG("Gyro attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}
#endif /* !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND) */

static int bmi160_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	switch (chan) {
#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		return bmi160_gyr_config(dev, chan, attr, val);
#endif
#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return bmi160_acc_config(dev, chan, attr, val);
#endif
	default:
		LOG_DBG("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int bmi160_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct bmi160_data *data = to_data(dev);
	uint8_t status;
	size_t i;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	status = 0;
	while ((status & BMI160_DATA_READY_BIT_MASK) == 0) {

		if (bmi160_byte_read(dev, BMI160_REG_STATUS, &status) < 0) {
			return -EIO;
		}
	}

	if (bmi160_read(dev, BMI160_SAMPLE_BURST_READ_ADDR, data->sample.raw,
			BMI160_BUF_SIZE) < 0) {
		return -EIO;
	}

	/* convert samples to cpu endianness */
	for (i = 0; i < BMI160_SAMPLE_SIZE; i += 2) {
		uint16_t *sample =
			(uint16_t *) &data->sample.raw[i];

		*sample = sys_le16_to_cpu(*sample);
	}

	return 0;
}

static void bmi160_to_fixed_point(int16_t raw_val, uint16_t scale,
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

static void bmi160_channel_convert(enum sensor_channel chan,
				   uint16_t scale,
				   uint16_t *raw_xyz,
				   struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_GYRO_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_GYRO_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop ; i++, val++) {
		bmi160_to_fixed_point(raw_xyz[i], scale, val);
	}
}

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
static inline void bmi160_gyr_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi160_data *data = to_data(dev);

	bmi160_channel_convert(chan, data->scale.gyr,
			       data->sample.gyr, val);
}
#endif

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
static inline void bmi160_acc_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi160_data *data = to_data(dev);

	bmi160_channel_convert(chan, data->scale.acc,
			       data->sample.acc, val);
}
#endif

static int bmi160_temp_channel_get(const struct device *dev,
				   struct sensor_value *val)
{
	uint16_t temp_raw = 0U;
	int32_t temp_micro = 0;
	struct bmi160_data *data = to_data(dev);

	if (data->pmu_sts.raw == 0U) {
		return -EINVAL;
	}

	if (bmi160_word_read(dev, BMI160_REG_TEMPERATURE0, &temp_raw) < 0) {
		return -EIO;
	}

	/* the scale is 1/2^9/LSB = 1953 micro degrees */
	temp_micro = BMI160_TEMP_OFFSET * 1000000ULL + temp_raw * 1953ULL;

	val->val1 = temp_micro / 1000000ULL;
	val->val2 = temp_micro % 1000000ULL;

	return 0;
}

static int bmi160_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	switch (chan) {
#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		bmi160_gyr_channel_get(dev, chan, val);
		return 0;
#endif
#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		bmi160_acc_channel_get(dev, chan, val);
		return 0;
#endif
	case SENSOR_CHAN_DIE_TEMP:
		return bmi160_temp_channel_get(dev, val);
	default:
		LOG_DBG("Channel not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api bmi160_api = {
	.attr_set = bmi160_attr_set,
#ifdef CONFIG_BMI160_TRIGGER
	.trigger_set = bmi160_trigger_set,
#endif
	.sample_fetch = bmi160_sample_fetch,
	.channel_get = bmi160_channel_get,
};

int bmi160_init(const struct device *dev)
{
	const struct bmi160_cfg *cfg = to_config(dev);
	struct bmi160_data *data = to_data(dev);
	uint8_t val = 0U;
	int32_t acc_range, gyr_range;

	data->bus = device_get_binding(cfg->bus_label);
	if (!data->bus) {
		LOG_DBG("SPI master controller not found: %s.", cfg->bus_label);
		return -EINVAL;
	}

	/* reboot the chip */
	if (bmi160_byte_write(dev, BMI160_REG_CMD, BMI160_CMD_SOFT_RESET) < 0) {
		LOG_DBG("Cannot reboot chip.");
		return -EIO;
	}

	k_busy_wait(1000);

	/* do a dummy read from 0x7F to activate SPI */
	if (bmi160_byte_read(dev, BMI160_SPI_START, &val) < 0) {
		LOG_DBG("Cannot read from 0x7F..");
		return -EIO;
	}

	k_busy_wait(100);

	if (bmi160_byte_read(dev, BMI160_REG_CHIPID, &val) < 0) {
		LOG_DBG("Failed to read chip id.");
		return -EIO;
	}

	if (val != BMI160_CHIP_ID) {
		LOG_DBG("Unsupported chip detected (0x%x)!", val);
		return -ENODEV;
	}

	/* set default PMU for gyro, accelerometer */
	data->pmu_sts.gyr = BMI160_DEFAULT_PMU_GYR;
	data->pmu_sts.acc = BMI160_DEFAULT_PMU_ACC;
	/* compass not supported, yet */
	data->pmu_sts.mag = BMI160_PMU_SUSPEND;

	/*
	 * The next command will take around 100ms (contains some necessary busy
	 * waits), but we cannot do it in a separate thread since we need to
	 * guarantee the BMI is up and running, before the app's main() is
	 * called.
	 */
	if (bmi160_pmu_set(dev, &data->pmu_sts) < 0) {
		LOG_DBG("Failed to set power mode.");
		return -EIO;
	}

	/* set accelerometer default range */
	if (bmi160_byte_write(dev, BMI160_REG_ACC_RANGE,
				BMI160_DEFAULT_RANGE_ACC) < 0) {
		LOG_DBG("Cannot set default range for accelerometer.");
		return -EIO;
	}

	acc_range = bmi160_acc_reg_val_to_range(BMI160_DEFAULT_RANGE_ACC);

	data->scale.acc = BMI160_ACC_SCALE(acc_range);

	/* set gyro default range */
	if (bmi160_byte_write(dev, BMI160_REG_GYR_RANGE,
			      BMI160_DEFAULT_RANGE_GYR) < 0) {
		LOG_DBG("Cannot set default range for gyroscope.");
		return -EIO;
	}

	gyr_range = bmi160_gyr_reg_val_to_range(BMI160_DEFAULT_RANGE_GYR);

	data->scale.gyr = BMI160_GYR_SCALE(gyr_range);

	if (bmi160_reg_field_update(dev, BMI160_REG_ACC_CONF,
				    BMI160_ACC_CONF_ODR_POS,
				    BMI160_ACC_CONF_ODR_MASK,
				    BMI160_DEFAULT_ODR_ACC) < 0) {
		LOG_DBG("Failed to set accel's default ODR.");
		return -EIO;
	}

	if (bmi160_reg_field_update(dev, BMI160_REG_GYR_CONF,
				    BMI160_GYR_CONF_ODR_POS,
				    BMI160_GYR_CONF_ODR_MASK,
				    BMI160_DEFAULT_ODR_GYR) < 0) {
		LOG_DBG("Failed to set gyro's default ODR.");
		return -EIO;
	}

#ifdef CONFIG_BMI160_TRIGGER
	if (bmi160_trigger_mode_init(dev) < 0) {
		LOG_DBG("Cannot set up trigger mode.");
		return -EINVAL;
	}
#endif

	return 0;
}

#if defined(CONFIG_BMI160_TRIGGER)
#define BMI160_TRIGGER_CFG(inst)					\
	.gpio_port = DT_INST_GPIO_LABEL(inst, int_gpios),		\
	.int_pin = DT_INST_GPIO_PIN(inst, int_gpios),			\
	.int_flags = DT_INST_GPIO_FLAGS(inst, int_gpios),
#else
#define BMI160_TRIGGER_CFG(inst)
#endif

#define BMI160_DEVICE_INIT(inst)					\
	DEVICE_DT_INST_DEFINE(inst, bmi160_init, NULL,			\
		&bmi160_data_##inst, &bmi160_cfg_##inst,		\
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
		&bmi160_api);

/* Instantiation macros used when a device is on a SPI bus */
#define BMI160_DEFINE_SPI(inst)						\
	static struct bmi160_data bmi160_data_##inst;			\
	static const struct bmi160_cfg bmi160_cfg_##inst = {		\
		BMI160_TRIGGER_CFG(inst)				\
		.reg_io = &bmi160_reg_io_spi,				\
		.bus_label = DT_INST_BUS_LABEL(inst),			\
		.bus_cfg = {						\
			.spi_cfg = (&(struct spi_config) {		\
				.operation = SPI_WORD_SET(8),		\
				.frequency = DT_INST_PROP(inst,		\
					spi_max_frequency),		\
				.slave = DT_INST_REG_ADDR(inst),	\
			}),						\
		},							\
	};								\
	BMI160_DEVICE_INIT(inst)

/* Instantiation macros used when a device is on an I2C bus */
#define BMI160_CONFIG_I2C(inst)						\
	{								\
		.bus_label = DT_INST_BUS_LABEL(inst),			\
		.reg_io = &bmi160_reg_io_i2c,				\
		.bus_cfg =  { .i2c_addr = DT_INST_REG_ADDR(inst), }	\
	}

#define BMI160_DEFINE_I2C(inst)						\
	static struct bmi160_data bmi160_data_##inst;			\
	static const struct bmi160_cfg bmi160_cfg_##inst =	\
		BMI160_CONFIG_I2C(inst);				\
	BMI160_DEVICE_INIT(inst)

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define BMI160_DEFINE(inst)						\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (BMI160_DEFINE_SPI(inst)),				\
		    (BMI160_DEFINE_I2C(inst)))

DT_INST_FOREACH_STATUS_OKAY(BMI160_DEFINE)
