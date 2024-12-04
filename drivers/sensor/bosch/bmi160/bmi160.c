/* Bosch BMI160 inertial measurement unit driver
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * http://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMI160-DS000-07.pdf
 */

#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>

#include "bmi160.h"

LOG_MODULE_REGISTER(BMI160, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BMI160 driver enabled without any devices"
#endif

#if BMI160_BUS_SPI
static int bmi160_transceive(const struct device *dev, uint8_t reg,
			     bool write, void *buf, size_t length)
{
	const struct bmi160_cfg *cfg = dev->config;
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

bool bmi160_bus_ready_spi(const struct device *dev)
{
	const struct bmi160_cfg *cfg = dev->config;

	return spi_is_ready_dt(&cfg->bus.spi);
}

int bmi160_read_spi(const struct device *dev,
		    uint8_t reg_addr, void *buf, uint8_t len)
{
	return bmi160_transceive(dev, reg_addr | BMI160_REG_READ, false,
				 buf, len);
}

int bmi160_write_spi(const struct device *dev,
		     uint8_t reg_addr, void *buf, uint8_t len)
{
	return bmi160_transceive(dev, reg_addr & BMI160_REG_MASK, true,
				 buf, len);
}

static const struct bmi160_bus_io bmi160_bus_io_spi = {
	.ready = bmi160_bus_ready_spi,
	.read = bmi160_read_spi,
	.write = bmi160_write_spi,
};
#endif /* BMI160_BUS_SPI */

#if BMI160_BUS_I2C

bool bmi160_bus_ready_i2c(const struct device *dev)
{
	const struct bmi160_cfg *cfg = dev->config;

	return device_is_ready(cfg->bus.i2c.bus);
}

int bmi160_read_i2c(const struct device *dev,
		    uint8_t reg_addr, void *buf, uint8_t len)
{
	const struct bmi160_cfg *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus.i2c, reg_addr, buf, len);
}

int bmi160_write_i2c(const struct device *dev,
		     uint8_t reg_addr, void *buf, uint8_t len)
{
	const struct bmi160_cfg *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->bus.i2c, reg_addr, buf, len);
}

static const struct bmi160_bus_io bmi160_bus_io_i2c = {
	.ready = bmi160_bus_ready_i2c,
	.read = bmi160_read_i2c,
	.write = bmi160_write_i2c,
};
#endif

int bmi160_read(const struct device *dev, uint8_t reg_addr, void *buf,
		uint8_t len)
{
	const struct bmi160_cfg *cfg = dev->config;

	return cfg->bus_io->read(dev, reg_addr, buf, len);
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
	const struct bmi160_cfg *cfg = dev->config;

	return cfg->bus_io->write(dev, reg_addr, buf, len);
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
	{0,    0  }, {0,     781}, {1,     562}, {3,    125}, {6,   250},
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
	int odr = bmi160_freq_to_odr_val(freq_int, freq_milli);

	if (odr < 0) {
		return odr;
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
	{125,	BMI160_GYR_RANGE_125DPS},
	{250,	BMI160_GYR_RANGE_250DPS},
	{500,	BMI160_GYR_RANGE_500DPS},
	{1000,	BMI160_GYR_RANGE_1000DPS},
	{2000,	BMI160_GYR_RANGE_2000DPS},
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
static int bmi160_acc_range_set(const struct device *dev, const struct sensor_value *val)
{
	int32_t range_g = sensor_ms2_to_g(val);
	struct bmi160_data *data = dev->data;
	int32_t reg_val = bmi160_range_to_reg_val(range_g,
						  bmi160_acc_range_map,
						  BMI160_ACC_RANGE_MAP_SIZE);

	if (reg_val < 0) {
		return reg_val;
	}

	switch (reg_val & 0xff) {
	case BMI160_ACC_RANGE_2G:
		range_g = 2;
		break;
	case BMI160_ACC_RANGE_4G:
		range_g = 4;
		break;
	case BMI160_ACC_RANGE_8G:
		range_g = 8;
		break;
	case BMI160_ACC_RANGE_16G:
		range_g = 16;
		break;
	}

	if (bmi160_byte_write(dev, BMI160_REG_ACC_RANGE, reg_val & 0xff) < 0) {
		return -EIO;
	}

	data->scale.acc_numerator = BMI160_ACC_SCALE_NUMERATOR(range_g);
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
	int32_t reg_val;

	/* we need the offsets for all axis */
	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	for (i = 0; i < BMI160_AXES; i++, ofs++) {
		/* convert offset to micro m/s^2 */
		reg_val =
			CLAMP(sensor_value_to_micro(ofs) / BMI160_ACC_OFS_LSB, INT8_MIN, INT8_MAX);

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
	struct bmi160_data *data = dev->data;
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
		return bmi160_acc_range_set(dev, val);
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
static int bmi160_gyr_range_set(const struct device *dev, const struct sensor_value *val)
{
	uint16_t range = sensor_rad_to_degrees(val);
	struct bmi160_data *data = dev->data;
	int32_t reg_val = bmi160_range_to_reg_val(range,
						bmi160_gyr_range_map,
						BMI160_GYR_RANGE_MAP_SIZE);

	if (reg_val < 0) {
		return reg_val;
	}
	switch (reg_val) {
	case BMI160_GYR_RANGE_125DPS:
		range = 125;
		break;
	case BMI160_GYR_RANGE_250DPS:
		range = 250;
		break;
	case BMI160_GYR_RANGE_500DPS:
		range = 500;
		break;
	case BMI160_GYR_RANGE_1000DPS:
		range = 1000;
		break;
	case BMI160_GYR_RANGE_2000DPS:
		range = 2000;
		break;
	}

	if (bmi160_byte_write(dev, BMI160_REG_GYR_RANGE, reg_val) < 0) {
		return -EIO;
	}

	data->scale.gyr_numerator = BMI160_GYR_SCALE_NUMERATOR(range);

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

		val = CLAMP(ofs_u / BMI160_GYR_OFS_LSB, -512, 511);

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
	struct bmi160_data *data = dev->data;

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
		return bmi160_gyr_range_set(dev, val);
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

static int bmi160_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	int rc;

	if (attr == SENSOR_ATTR_OFFSET) {
		if (chan != SENSOR_CHAN_ACCEL_XYZ && chan != SENSOR_CHAN_GYRO_XYZ) {
			return -EINVAL;
		}

		int8_t data[7];

		rc = bmi160_read(dev, BMI160_REG_OFFSET_ACC_X, data, 7);
		if (rc != 0) {
			return rc;
		}

		if ((chan == SENSOR_CHAN_ACCEL_XYZ &&
		     FIELD_GET(BIT(BMI160_ACC_OFS_EN_POS), data[6]) == 0) ||
		    (chan == SENSOR_CHAN_GYRO_XYZ &&
		     FIELD_GET(BIT(BMI160_GYR_OFS_EN_POS), data[6]) == 0)) {
			for (int i = 0; i < 3; ++i) {
				val[i].val1 = 0;
				val[i].val2 = 0;
			}
		} else {
			for (int i = 0; i < 3; ++i) {
				if (chan == SENSOR_CHAN_ACCEL_XYZ) {
					int32_t ug = data[i] * INT32_C(3900);

					sensor_ug_to_ms2(ug, &val[i]);
				} else {
					int32_t udeg =
						(FIELD_GET(GENMASK((2 * i) + 1, 2 * i), data[6])
						 << 8) |
						data[3 + i];

					udeg |= 0 - (udeg & 0x200);
					udeg *= 61000;
					sensor_10udegrees_to_rad(udeg / 10, &val[i]);
				}
			}
		}
		return 0;
	}
	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		if (chan == SENSOR_CHAN_ACCEL_XYZ) {
			int64_t rate_uhz;
			uint8_t acc_odr;

			if (IS_ENABLED(CONFIG_BMI160_ACCEL_ODR_RUNTIME)) {
				/* Read the register */
				rc = bmi160_byte_read(dev, BMI160_REG_ACC_CONF, &acc_odr);
				if (rc != 0) {
					return rc;
				}
				acc_odr = FIELD_GET(BMI160_ACC_CONF_ODR_MASK, acc_odr);
			} else {
				acc_odr = BMI160_DEFAULT_ODR_ACC;
			}

			rate_uhz = INT64_C(100000000) * BIT(acc_odr) / 256;
			val->val1 = rate_uhz / 1000000;
			val->val2 = rate_uhz - val->val1 * 1000000;
			return 0;
		} else if (chan == SENSOR_CHAN_GYRO_XYZ) {
			int64_t rate_uhz;
			uint8_t gyr_ord;

			if (IS_ENABLED(CONFIG_BMI160_GYRO_ODR_RUNTIME)) {
				/* Read the register */
				rc = bmi160_byte_read(dev, BMI160_REG_GYR_CONF, &gyr_ord);
				if (rc != 0) {
					return rc;
				}
				gyr_ord = FIELD_GET(BMI160_GYR_CONF_ODR_MASK, gyr_ord);
			} else {
				gyr_ord = BMI160_DEFAULT_ODR_GYR;
			}

			rate_uhz = INT64_C(100000000) * BIT(gyr_ord) / 256;
			val->val1 = rate_uhz / 1000000;
			val->val2 = rate_uhz - val->val1 * 1000000;
			return 0;

		}
		return -EINVAL;

	}
	if (attr == SENSOR_ATTR_FULL_SCALE) {
		if (chan == SENSOR_CHAN_ACCEL_XYZ) {
			uint8_t acc_range;

			if (IS_ENABLED(CONFIG_BMI160_ACCEL_RANGE_RUNTIME)) {
				rc = bmi160_byte_read(dev, BMI160_REG_ACC_RANGE, &acc_range);
				if (rc != 0) {
					return rc;
				}
			} else {
				acc_range = BMI160_DEFAULT_RANGE_ACC;
			}
			sensor_g_to_ms2(bmi160_acc_reg_val_to_range(acc_range), val);
			return 0;
		} else if (chan == SENSOR_CHAN_GYRO_XYZ) {
			uint8_t gyr_range;

			if (IS_ENABLED(CONFIG_BMI160_GYRO_RANGE_RUNTIME)) {
				rc = bmi160_byte_read(dev, BMI160_REG_GYR_RANGE, &gyr_range);
				if (rc != 0) {
					return rc;
				}
			} else {
				gyr_range = BMI160_DEFAULT_RANGE_GYR;
			}
			sensor_degrees_to_rad(bmi160_gyr_reg_val_to_range(gyr_range), val);
			return 0;
		}
		return -EINVAL;
	}
	return -EINVAL;
}

static int bmi160_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct bmi160_data *data = dev->data;
	uint8_t status;
	size_t i;
	int ret = 0;
	enum pm_device_state pm_state;

	(void)pm_device_state_get(dev, &pm_state);
	if (pm_state != PM_DEVICE_STATE_ACTIVE) {
		LOG_DBG("Device is suspended, fetch is unavailable");
		ret = -EIO;
		goto out;
	}

	if (chan == SENSOR_CHAN_DIE_TEMP) {
		/* Die temperature is only valid when at least one measurement is active */
		if (data->pmu_sts.raw == 0U) {
			return -EINVAL;
		}
		return bmi160_word_read(dev, BMI160_REG_TEMPERATURE0, &data->sample.temperature);
	}

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	status = 0;
	while ((status & BMI160_DATA_READY_BIT_MASK) == 0) {

		if (bmi160_byte_read(dev, BMI160_REG_STATUS, &status) < 0) {
			ret = -EIO;
			goto out;
		}
	}

	if (bmi160_read(dev, BMI160_SAMPLE_BURST_READ_ADDR, data->sample.raw,
			BMI160_BUF_SIZE) < 0) {
		ret = -EIO;
		goto out;
	}

	/* convert samples to cpu endianness */
	for (i = 0; i < BMI160_SAMPLE_SIZE; i += 2) {
		uint16_t *sample =
			(uint16_t *) &data->sample.raw[i];

		*sample = sys_le16_to_cpu(*sample);
	}

out:
	return ret;
}

static void bmi160_to_fixed_point(int16_t raw_val, int64_t scale_numerator,
				  uint32_t scale_denominator, struct sensor_value *val)
{
	int64_t converted_val = (int64_t)raw_val * scale_numerator / scale_denominator;

	val->val1 = converted_val / 1000000;
	val->val2 = converted_val % 1000000;
}

static void bmi160_channel_convert(enum sensor_channel chan,
				   int64_t scale_numerator,
				   uint32_t scale_denominator,
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
		bmi160_to_fixed_point(raw_xyz[i], scale_numerator, scale_denominator, val);
	}
}

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
static inline void bmi160_gyr_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi160_data *data = dev->data;

	bmi160_channel_convert(chan, data->scale.gyr_numerator, BMI160_GYR_SCALE_DENOMINATOR,
			       data->sample.gyr, val);
}
#endif

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
static inline void bmi160_acc_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi160_data *data = dev->data;

	bmi160_channel_convert(chan, data->scale.acc_numerator, BMI160_ACC_SCALE_DENOMINATOR,
			       data->sample.acc, val);
}
#endif

static int bmi160_temp_channel_get(const struct device *dev,
				   struct sensor_value *val)
{
	struct bmi160_data *data = dev->data;

	/* the scale is 1/2^9/LSB = 1953 micro degrees */
	int32_t temp_micro = BMI160_TEMP_OFFSET * 1000000ULL + data->sample.temperature * 1953ULL;

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

static DEVICE_API(sensor, bmi160_api) = {
	.attr_set = bmi160_attr_set,
	.attr_get = bmi160_attr_get,
#ifdef CONFIG_BMI160_TRIGGER
	.trigger_set = bmi160_trigger_set,
#endif
	.sample_fetch = bmi160_sample_fetch,
	.channel_get = bmi160_channel_get,
};


static inline int bmi160_resume(const struct device *dev)
{
	struct bmi160_data *data = dev->data;

	return bmi160_pmu_set(dev, &data->pmu_sts);
}

static inline int bmi160_suspend(const struct device *dev)
{
	struct bmi160_data *data = dev->data;

	/* Suspend everything */
	union bmi160_pmu_status st = {
		.acc = BMI160_PMU_SUSPEND,
		.gyr = BMI160_PMU_SUSPEND,
		.mag = BMI160_PMU_SUSPEND,
	};

	int ret = bmi160_pmu_set(dev, &st);

	if (ret == 0) {
		memset(data->sample.raw, 0, sizeof(data->sample.raw));
	}
	return ret;
}

int bmi160_init(const struct device *dev)
{
	const struct bmi160_cfg *cfg = dev->config;
	struct bmi160_data *data = dev->data;
	uint8_t val = 0U;
	int32_t acc_range, gyr_range;

	if (!cfg->bus_io->ready(dev)) {
		LOG_ERR("Bus not ready");
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

	k_busy_wait(150);

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

	/* Start in a suspended state (never turning on the mems sensors) if
	 * PM_DEVICE_RUNTIME is enabled.
	 */
#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_init_suspended(dev);

	int ret = pm_device_runtime_enable(dev);

	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to enabled runtime power management");
		return -EIO;
	}
#else

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
#endif

	/* set accelerometer default range */
	if (bmi160_byte_write(dev, BMI160_REG_ACC_RANGE,
				BMI160_DEFAULT_RANGE_ACC) < 0) {
		LOG_DBG("Cannot set default range for accelerometer.");
		return -EIO;
	}

	acc_range = bmi160_acc_reg_val_to_range(BMI160_DEFAULT_RANGE_ACC);

	data->scale.acc_numerator = BMI160_ACC_SCALE_NUMERATOR(acc_range);

	/* set gyro default range */
	if (bmi160_byte_write(dev, BMI160_REG_GYR_RANGE,
			      BMI160_DEFAULT_RANGE_GYR) < 0) {
		LOG_DBG("Cannot set default range for gyroscope.");
		return -EIO;
	}

	gyr_range = bmi160_gyr_reg_val_to_range(BMI160_DEFAULT_RANGE_GYR);

	data->scale.gyr_numerator = BMI160_GYR_SCALE_NUMERATOR(gyr_range);

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

int bmi160_pm(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		bmi160_resume(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		bmi160_suspend(dev);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

#if defined(CONFIG_BMI160_TRIGGER)
#define BMI160_TRIGGER_CFG(inst) \
	.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),
#else
#define BMI160_TRIGGER_CFG(inst)
#endif

#define BMI160_DEVICE_INIT(inst)								\
	IF_ENABLED(CONFIG_PM_DEVICE_RUNTIME, (PM_DEVICE_DT_INST_DEFINE(inst, bmi160_pm)));	\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmi160_init,						\
		COND_CODE_1(CONFIG_PM_DEVICE_RUNTIME, (PM_DEVICE_DT_INST_GET(inst)), (NULL)),	\
		&bmi160_data_##inst, &bmi160_cfg_##inst,					\
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,					\
		&bmi160_api);

/* Instantiation macros used when a device is on a SPI bus */
#define BMI160_DEFINE_SPI(inst)						   \
	static struct bmi160_data bmi160_data_##inst;			   \
	static const struct bmi160_cfg bmi160_cfg_##inst = {		   \
		.bus.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0), \
		.bus_io = &bmi160_bus_io_spi,				   \
		BMI160_TRIGGER_CFG(inst)				   \
	};								   \
	BMI160_DEVICE_INIT(inst)

/* Instantiation macros used when a device is on an I2C bus */
#define BMI160_CONFIG_I2C(inst)			       \
	{					       \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.bus_io = &bmi160_bus_io_i2c,	       \
		BMI160_TRIGGER_CFG(inst)	       \
	}

#define BMI160_DEFINE_I2C(inst)							    \
	static struct bmi160_data bmi160_data_##inst;				    \
	static const struct bmi160_cfg bmi160_cfg_##inst = BMI160_CONFIG_I2C(inst); \
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
