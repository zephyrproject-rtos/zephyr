/* Bosch BMI160 inertial measurement unit driver
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * http://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMI160-DS000-07.pdf
 */

#include <init.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "bm160.h"

LOG_MODULE_REGISTER(BM160, CONFIG_SENSOR_LOG_LEVEL);

static int bmi160_transceive_spi(const struct device *dev, uint8_t reg,
				 bool write, void *data, size_t length)
{
	struct bmi160_device_data *bmi160 = dev->data;
	const struct spi_buf buf[2] = {
		{
			.buf = &reg,
			.len = 1
		},
		{
			.buf = data,
			.len = length
		}
	};
	const struct spi_buf_set tx = {
		.buffers = buf,
		.count = data ? 2 : 1
	};

	if (!write) {
		const struct spi_buf_set rx = {
			.buffers = buf,
			.count = 2
		};

		return spi_transceive(bmi160->spi, &bmi160->spi_cfg, &tx, &rx);
	}

	return spi_write(bmi160->spi, &bmi160->spi_cfg, &tx);
}

static int bmi160_transceive_i2c(const struct device *dev, uint8_t reg,
				 bool write, void *data, size_t len)
{
	struct bmi160_device_data *bmi160 = dev->data;

	if (!write) {
		return i2c_burst_read(bmi160->i2c, bmi160->i2c_addr,
				reg, data, len);
	}

	return i2c_burst_write(bmi160->i2c, bmi160->i2c_addr, reg, data, len);
}


static int bmi160_transceive(const struct device *dev, uint8_t reg,
			     bool write, void *data, size_t length)
{
	struct bmi160_device_data *bmi160 = dev->data;

	if (bmi160->i2c) {
		return bmi160_transceive_i2c(dev, reg, write, data, length);
	} else {
		return bmi160_transceive_spi(dev, reg, write, data, length);
	}
}

int bmi160_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	return bmi160_transceive(dev, reg_addr | BIT(7), false, data, len);
}

int bmi160_byte_read(const struct device *dev, uint8_t reg_addr,
		     uint8_t *byte)
{
	return bmi160_transceive(dev, reg_addr | BIT(7), false, byte, 1);
}

static int bmi160_word_read(const struct device *dev, uint8_t reg_addr,
			    uint16_t *word)
{
	if (bmi160_transceive(dev, reg_addr | BIT(7), false, word, 2) != 0) {
		return -EIO;
	}

	*word = sys_le16_to_cpu(*word);

	return 0;
}

int bmi160_byte_write(const struct device *dev, uint8_t reg_addr,
		      uint8_t byte)
{
	return bmi160_transceive(dev, reg_addr & 0x7F, true, &byte, 1);
}

int bmi160_word_write(const struct device *dev, uint8_t reg_addr,
		      uint16_t word)
{
	uint8_t tx_word[2] = {
		(uint8_t)(word & 0xff),
		(uint8_t)(word >> 8)
	};

	return bmi160_transceive(dev, reg_addr & 0x7F, true, tx_word, 2);
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
				       BMI160_ACC_CONF_US, BMI160_ACC_CONF_US,
				       pmu_sts->acc != BMI160_PMU_NORMAL);
}


#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME) ||\
	defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME) ||\
	defined(CONFIG_BMX160_MAG_ODR_RUNTIME)
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
	struct bmi160_device_data *bmi160 = dev->data;
	int odr = bmi160_freq_to_odr_val(freq_int, freq_milli);

	if (odr < 0) {
		return odr;
	}

	/* some odr values cannot be set in certain power modes */
	if ((bmi160->pmu_sts.acc == BMI160_PMU_NORMAL &&
	     odr < BMI160_ODR_25_2) ||
	    (bmi160->pmu_sts.acc == BMI160_PMU_LOW_POWER &&
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
	struct bmi160_device_data *bmi160 = dev->data;
	int32_t reg_val = bmi160_range_to_reg_val(range,
						  bmi160_acc_range_map,
						  BMI160_ACC_RANGE_MAP_SIZE);

	if (reg_val < 0) {
		return reg_val;
	}

	if (bmi160_byte_write(dev, BMI160_REG_ACC_RANGE, reg_val & 0xff) < 0) {
		return -EIO;
	}

	bmi160->scale.acc = BMI160_ACC_SCALE(range);

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

	for (i = 0; i < 3; i++, ofs++) {
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
	struct bmi160_device_data *bmi160 = dev->data;
	uint8_t foc_pos[] = {
		BMI160_FOC_ACC_X_POS,
		BMI160_FOC_ACC_Y_POS,
		BMI160_FOC_ACC_Z_POS,
	};
	int i;
	uint8_t reg_val = 0U;

	/* Calibration has to be done in normal mode. */
	if (bmi160->pmu_sts.acc != BMI160_PMU_NORMAL) {
		return -ENOTSUP;
	}

	/*
	 * Hardware calibration is done knowing the expected values on all axis.
	 */
	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	for (i = 0; i < 3; i++, xyz_calib_value++) {
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

#if defined(CONFIG_BMX160_MAG_ODR_RUNTIME)
static int bmx160_magn_odr_set(const struct device *dev, uint16_t freq_int,
			      uint16_t freq_milli)
{
	int odr = bmi160_freq_to_odr_val(freq_int, freq_milli);

	if (odr < 0) {
		return odr;
	}

	if (odr < BMI160_ODR_25_32 || odr > BMI160_ODR_800) {
		return -ENOTSUP;
	}

	return bmi160_reg_field_update(dev, BMI160_REG_MAG_CONF,
				       BMI160_MAG_CONF_ODR_POS,
				       BMI160_MAG_CONF_ODR_MASK,
				       (uint8_t) odr);
}

static int bmx160_magn_config(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return bmx160_magn_odr_set(dev, val->val1, val->val2 / 1000);

	default:
		LOG_DBG("Mag attribute not supported.");
		return -ENOTSUP;
	}
}
#endif /* defined(CONFIG_BMX160_MAG_ODR_RUNTIME) */

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
	struct bmi160_device_data *bmi160 = dev->data;
	int32_t reg_val = bmi160_range_to_reg_val(range,
						bmi160_gyr_range_map,
						BMI160_GYR_RANGE_MAP_SIZE);

	if (reg_val < 0) {
		return reg_val;
	}

	if (bmi160_byte_write(dev, BMI160_REG_GYR_RANGE, reg_val) < 0) {
		return -EIO;
	}

	bmi160->scale.gyr = BMI160_GYR_SCALE(range);

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

	for (i = 0; i < 3; i++, ofs++) {
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
	struct bmi160_device_data *bmi160 = dev->data;

	ARG_UNUSED(chan);

	/* Calibration has to be done in normal mode. */
	if (bmi160->pmu_sts.gyr != BMI160_PMU_NORMAL) {
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

int bmi160_attr_set(const struct device *dev, enum sensor_channel chan,
		    enum sensor_attribute attr, const struct sensor_value *val)
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
#if defined(CONFIG_BMX160_MAG_ODR_RUNTIME)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		return bmx160_magn_config(dev, chan, attr, val);
#endif

	default:
		LOG_DBG("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

#if defined(CONFIG_BMX160_MAG)
#	define BMX160_MAG_DRDY			BIT(5)
#else
#	define BMX160_MAG_DRDY			(0)
#endif

#if defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
#	define BMI160_SAMPLE_BURST_READ_ADDR	BMI160_REG_DATA_ACC_X
#	define BMI160_DATA_READY_BIT_MASK	((1 << 7) | BMX160_MAG_DRDY)
#elif !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
#	define BMI160_SAMPLE_BURST_READ_ADDR	BMI160_REG_DATA_GYR_X
#	define BMI160_DATA_READY_BIT_MASK	((1 << 6) | BMX160_MAG_DRDY)
#else
#	define BMI160_SAMPLE_BURST_READ_ADDR	BMI160_REG_DATA_MAG_X
#	define BMI160_DATA_READY_BIT_MASK	BMX160_MAG_DRDY
#endif

int bmi160_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmi160_device_data *bmi160 = dev->data;
	size_t i;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	bmi160->sample.raw[0] = 0U;

	while ((bmi160->sample.raw[0] & BMI160_DATA_READY_BIT_MASK) == 0U) {
		if (bmi160_transceive(dev, BMI160_REG_STATUS | (1 << 7), false,
				      bmi160->sample.raw, 1) < 0) {
			return -EIO;
		}
	}

	if (bmi160_transceive(dev, BMI160_SAMPLE_BURST_READ_ADDR | (1 << 7),
			     false, &bmi160->sample.raw[BMX160_MAG_SAMPLE_SIZE],
			     BMI160_BUF_SIZE - BMX160_MAG_SAMPLE_SIZE) < 0) {
		return -EIO;
	}

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND) || \
	!defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	if (bmi160_transceive(dev, BMI160_REG_DATA_MAG_X | (1 << 7),
			      false, &bmi160->sample.raw,
			      BMX160_MAG_SAMPLE_SIZE) < 0) {
		return -EIO;
	}
#endif

	/* convert samples to cpu endianness */
	for (i = 0; i < BMI160_SAMPLE_SIZE; i += 2) {
		uint16_t *sample =
		   (uint16_t *) &bmi160->sample.raw[i + BMX160_MAG_SAMPLE_SIZE];

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
	struct bmi160_device_data *bmi160 = dev->data;

	bmi160_channel_convert(chan, bmi160->scale.gyr,
			       bmi160->sample.gyr, val);
}
#endif

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
static inline void bmi160_acc_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi160_device_data *bmi160 = dev->data;

	bmi160_channel_convert(chan, bmi160->scale.acc,
			       bmi160->sample.acc, val);
}
#endif

#if defined(CONFIG_BMX160_MAG)

/* bmx160 chip has bmm150 magnetometer built-in, so the below computation part
 * of mangetomer are based on reference from bmm150 driver.
 */
static int32_t bmx160_compensate_mag_xy(struct bmx_magn_trim_regs *tregs,
				      int16_t xy, uint16_t rhall, bool is_x)
{
	int8_t txy1, txy2;
	int16_t val;
	uint16_t prevalue;
	int32_t temp1, temp2, temp3;

	if (xy == BMX160_XY_OVERFLOW_VAL) {
		return INT32_MIN;
	}

	if (!rhall) {
		rhall = tregs->xyz1;
	}

	if (is_x) {
		txy1 = tregs->x1;
		txy2 = tregs->x2;
	} else {
		txy1 = tregs->y1;
		txy2 = tregs->y2;
	}

	prevalue = (uint16_t)((((int32_t)tregs->xyz1) << 14) / rhall);

	val = (int16_t)((prevalue) - ((uint16_t)0x4000));

	temp1 = (((int32_t)tregs->xy2) *
		((((int32_t)val) * ((int32_t)val)) >> 7));

	temp2 = ((int32_t)val) * ((int32_t)(((int16_t)tregs->xy1) << 7));

	temp3 = (((((temp1 + temp2) >> 9) +
		((int32_t)0x100000)) * ((int32_t)(((int16_t)txy2) +
		((int16_t)0xA0)))) >> 12);

	val = ((int16_t)((((int32_t)xy) * temp3) >> 13)) +
		(((int16_t)txy1) << 3);

	return (int32_t)val;
}

static int32_t bmx160_compensate_z(struct bmx_magn_trim_regs *tregs,
				 int16_t z, uint16_t rhall)
{
	int32_t val, temp1, temp2;
	int16_t temp3;

	if (z == BMX160_Z_OVERFLOW_VAL) {
		return INT32_MIN;
	}

	temp1 = (((int32_t)(z - tregs->z4)) << 15);

	temp2 = ((((int32_t)tregs->z3) *
		((int32_t)(((int16_t)rhall) - ((int16_t)tregs->xyz1)))) >> 2);

	temp3 = ((int16_t)(((((int32_t)tregs->z1) *
		((((int16_t)rhall) << 1))) + (1 << 15)) >> 16));

	val = ((temp1 - temp2) / (tregs->z2 + temp3));

	return val;
}

static void bmx160_convert(struct sensor_value *val, int raw_val)
{
	/* val = raw_val / 1600 */
	val->val1 = raw_val / 1600;
	val->val2 = ((int32_t)raw_val * (1000000 / 1600)) % 1000000;
}

static inline void bmx160_magn_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct bmi160_device_data *bmi160 = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		val->val1 = bmx160_compensate_mag_xy(&bmi160->tregs,
					bmi160->sample.mag[0],
					bmi160->sample.rhall, true);
		bmx160_convert(val, val->val1);
		return;

	case SENSOR_CHAN_MAGN_Y:
		val->val1 = bmx160_compensate_mag_xy(&bmi160->tregs,
					 bmi160->sample.mag[1],
					 bmi160->sample.rhall, false);
		bmx160_convert(val, val->val1);
		return;

	case SENSOR_CHAN_MAGN_Z:
		val->val1 = bmx160_compensate_z(&bmi160->tregs,
					 bmi160->sample.mag[2],
					 bmi160->sample.rhall);
		bmx160_convert(val, val->val1);
		return;

	default:
		val[0].val1 = bmx160_compensate_mag_xy(&bmi160->tregs,
					bmi160->sample.mag[0],
					bmi160->sample.rhall, true);
		bmx160_convert(val, val[0].val1);
		val[1].val1 = bmx160_compensate_mag_xy(&bmi160->tregs,
					bmi160->sample.mag[1],
					bmi160->sample.rhall, false);
		bmx160_convert(val + 1, val[1].val1);
		val[2].val1 = bmx160_compensate_z(&bmi160->tregs,
					bmi160->sample.mag[2],
					bmi160->sample.rhall);
		bmx160_convert(val + 2, val[2].val1);
		return;
	}
}
#endif

static int bmi160_temp_channel_get(const struct device *dev, struct sensor_value *val)
{
	uint16_t temp_raw = 0U;
	int32_t temp_micro = 0;
	struct bmi160_device_data *bmi160 = dev->data;

	if (bmi160->pmu_sts.raw == 0U) {
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

int bmi160_channel_get(const struct device *dev,
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
#if defined(CONFIG_BMX160_MAG)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		bmx160_magn_channel_get(dev, chan, val);
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

#if defined(CONFIG_BMX160_MAG)

/* reference from bmm150 driver */
int bmi160_magn_treg_read(const struct device *dev)
{
	uint8_t i, rval = BMI160_STATUS_MAG_MAN_OP;
	struct bmi160_device_data *bmi160 = dev->data;
	struct bmx_magn_trim_regs *tregs = &bmi160->tregs;
	uint8_t *ptr;

	ptr = (uint8_t *)tregs;
	for (i = 0; i < sizeof(*tregs); ptr++, i++) {
		if (bmi160_byte_write(dev, BMX160_REG_MAG_IF1,
					BMX160_REG_TRIM_START + i) < 0){
			LOG_ERR("failed to read trim regs");
			return -1;
		}

		while ((rval & BMI160_STATUS_MAG_MAN_OP) == 0U) {
			if (bmi160_byte_read(dev, BMI160_REG_STATUS,
						&rval) < 0) {
				return -EIO;
			}
		}

		if (bmi160_byte_read(dev, BMI160_REG_DATA_MAG_X, ptr) < 0) {
			return -EIO;
		}
	}

	tregs->xyz1 = sys_le16_to_cpu(tregs->xyz1);
	tregs->z1 = sys_le16_to_cpu(tregs->z1);
	tregs->z2 = sys_le16_to_cpu(tregs->z2);
	tregs->z3 = sys_le16_to_cpu(tregs->z3);
	tregs->z4 = sys_le16_to_cpu(tregs->z4);

	return 0;
}

/* write magnetometer reg & val over magif */
int bmi160_magn_indirect_write(const struct device *dev, uint8_t reg, uint8_t magval)
{
	uint8_t rval = BMI160_STATUS_MAG_MAN_OP;

	if (bmi160_byte_write(dev, BMX160_REG_MAG_IF3, magval) < 0) {
		return -EIO;
	}

	if (bmi160_byte_write(dev, BMX160_REG_MAG_IF2, reg) < 0) {
		return -EIO;
	}

	while ((rval & BMI160_STATUS_MAG_MAN_OP) == 0U) {
		if (bmi160_byte_read(dev, BMI160_REG_STATUS, &rval) < 0) {
			return -EIO;
		}
	}

	return 0;
}

int bmi160_setup_magnif(const struct device *dev)
{
	struct bmi160_device_data *ddata = dev->data;

	if (ddata->pmu_sts.mag != BMI160_PMU_NORMAL) {
		if (bmi160_byte_write(dev, BMI160_REG_CMD,
			(BMI160_PMU_NORMAL | BMI160_CMD_PMU_MAG)) < 0) {
			LOG_ERR("Cannot set mag to normal mode.");
			return -EIO;
		}
		k_busy_wait(650);
	}

	if (bmi160_reg_field_update(dev, BMX160_REG_MAG_IF0, 0,
				BMI160_MAG_MANUAL_EN, BMI160_MAG_MANUAL_EN)) {
		LOG_ERR("Cannot set mag setup mode.");
		return -EIO;
	}

	if (bmi160_magn_indirect_write(dev, BMI160_MAG_REG_POWER_CTRL,
				       BMI160_MAG_SLEEP_MODE)) {
		LOG_ERR("Cannot set mag sleep mode.");
		return -EIO;
	}

	if (bmi160_magn_indirect_write(dev, BMI160_MAG_REG_PRESET_XY,
				       BMX160_DEFAULT_XY_PRESET)) {
		LOG_ERR("Cannot set mag xy preset.");
		return -EIO;
	}

	if (bmi160_magn_indirect_write(dev, BMI160_MAG_REG_PRESET_Z,
				       BMX160_DEFAULT_Z_PRESET)) {
		LOG_ERR("Cannot set mag xy preset.");
		return -EIO;
	}

	if (bmi160_magn_treg_read(dev) < 0) {
		LOG_ERR("Cannot change to data mode.");
		return -EIO;
	}

	if (bmi160_magn_indirect_write(dev, BMI160_MAG_REG_DATA_MODE,
				       BMI160_MAG_DATA_MODE)) {
		LOG_ERR("Cannot change to data mode.");
		return -EIO;
	}

	if (bmi160_byte_write(dev, BMX160_REG_MAG_IF1, 0x42) < 0) {
		LOG_ERR("Cannot set mag setup mode.");
		return -EIO;
	}

	if (bmi160_reg_field_update(dev, BMI160_REG_MAG_CONF,
				    BMI160_MAG_CONF_ODR_POS,
				    BMI160_MAG_CONF_ODR_MASK,
				    BMX160_DEFAULT_ODR_MAG) < 0) {
		LOG_ERR("Failed to set mag's default ODR.");
		return -EIO;
	}

	if (bmi160_reg_field_update(dev, BMX160_REG_MAG_IF0,
			    0, BMI160_MAG_MANUAL_EN, 0)) {
		LOG_ERR("Cannot disable mag manual mode.");
		return -EIO;
	}

	if (ddata->pmu_sts.mag == BMI160_PMU_LOW_POWER) {
		if (bmi160_byte_write(dev, BMI160_REG_CMD,
			(BMI160_CMD_PMU_MAG | ddata->pmu_sts.mag)) < 0) {
			LOG_ERR("Cannot set mag to normal mode.");
			return -EIO;
		}
	}

	return 0;
}
#endif /* CONFIG_BMX160_MAG */

int bm160_device_init(const struct device *dev)
{
	uint8_t val = 0U;
	int32_t acc_range, gyr_range;
	struct bmi160_device_data *ddata = dev->data;
	const struct bmi160_device_config *cfg = dev->config;

	/* reboot the chip */
	if (bmi160_byte_write(dev, BMI160_REG_CMD, BMI160_CMD_SOFT_RESET) < 0) {
		LOG_DBG("Cannot reboot chip.");
		return -EIO;
	}

	k_busy_wait(1000);

	/* do a dummy read from 0x7F to activate SPI */
	if (bmi160_byte_read(dev, 0x7F, &val) < 0) {
		LOG_DBG("Cannot read from 0x7F..");
		return -EIO;
	}

	k_busy_wait(100);

	if (bmi160_byte_read(dev, BMI160_REG_CHIPID, &val) < 0) {
		LOG_DBG("Failed to read chip id.");
		return -EIO;
	}

	if (val != cfg->chipid) {
		LOG_DBG("Unsupported chip detected (0x%x)!", val);
		return -ENODEV;
	}

	/* set default PMU for gyro, accelerometer */
	ddata->pmu_sts.gyr = BMI160_DEFAULT_PMU_GYR;
	ddata->pmu_sts.acc = BMI160_DEFAULT_PMU_ACC;
	ddata->pmu_sts.mag = BMX160_DEFAULT_PMU_MAG;

	/*
	 * The next command will take around 100ms (contains some necessary busy
	 * waits), but we cannot do it in a separate thread since we need to
	 * guarantee the BMI is up and running, before the app's main() is
	 * called.
	 */
	if (bmi160_pmu_set(dev, &ddata->pmu_sts) < 0) {
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

	ddata->scale.acc = BMI160_ACC_SCALE(acc_range);

	/* set gyro default range */
	if (bmi160_byte_write(dev, BMI160_REG_GYR_RANGE,
			      BMI160_DEFAULT_RANGE_GYR) < 0) {
		LOG_DBG("Cannot set default range for gyroscope.");
		return -EIO;
	}

	gyr_range = bmi160_gyr_reg_val_to_range(BMI160_DEFAULT_RANGE_GYR);

	ddata->scale.gyr = BMI160_GYR_SCALE(gyr_range);

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

#if defined(CONFIG_BMX160_MAG)
	if (ddata->pmu_sts.mag != BMI160_PMU_SUSPEND) {
		if (bmi160_setup_magnif(dev)) {
			LOG_DBG("Failed to setup magnif.");
			return -EIO;
		}
	}
#endif

#ifdef CONFIG_BMI160_TRIGGER
	if (bmi160_trigger_mode_init(dev) < 0) {
		LOG_DBG("Cannot set up trigger mode.");
		return -EINVAL;
	}
#endif

	return 0;
}
