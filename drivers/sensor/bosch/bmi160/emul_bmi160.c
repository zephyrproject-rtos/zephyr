/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the Bosch BMI160 accelerometer / gyro. This supports basic
 * init and reading of canned samples. It supports both I2C and SPI buses.
 */

#define DT_DRV_COMPAT bosch_bmi160

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bosch_bmi160);

#include <zephyr/sys/byteorder.h>
#include <bmi160.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/sys/util.h>

/** Run-time data used by the emulator */
struct bmi160_emul_data {
	uint8_t pmu_status;
	/** Current register to read (address) */
	uint32_t cur_reg;
};

/** Static configuration for the emulator */
struct bmi160_emul_cfg {
	/** Chip registers */
	uint8_t *reg;
	union {
		/** Unit address (chip select ordinal) of emulator */
		uint16_t chipsel;
		/** I2C address of emulator */
		uint16_t addr;
	};
};

/* Names for the PMU components */
static const char *const pmu_name[] = {"acc", "gyr", "mag", "INV"};

int emul_bmi160_get_reg_value(const struct emul *target, int reg_number, uint8_t *out, size_t count)
{
	const struct bmi160_emul_cfg *cfg = target->cfg;

	if (reg_number < 0 || reg_number + count > BMI160_REG_COUNT) {
		return -EINVAL;
	}

	memcpy(out, cfg->reg + reg_number, count);
	return 0;
}

static void reg_write(const struct emul *target, int regn, int val)
{
	struct bmi160_emul_data *data = target->data;
	const struct bmi160_emul_cfg *cfg = target->cfg;

	LOG_DBG("write %x = %x", regn, val);
	cfg->reg[regn] = val;
	switch (regn) {
	case BMI160_REG_ACC_CONF:
		LOG_DBG("   * acc conf");
		break;
	case BMI160_REG_ACC_RANGE:
		LOG_DBG("   * acc range");
		break;
	case BMI160_REG_GYR_CONF:
		LOG_DBG("   * gyr conf");
		break;
	case BMI160_REG_GYR_RANGE:
		LOG_DBG("   * gyr range");
		break;
	case BMI160_REG_CMD:
		switch (val) {
		case BMI160_CMD_SOFT_RESET:
			LOG_DBG("   * soft reset");
			break;
		default:
			if ((val & BMI160_CMD_PMU_BIT) == BMI160_CMD_PMU_BIT) {
				int which = (val & BMI160_CMD_PMU_MASK) >> BMI160_CMD_PMU_SHIFT;
				int shift;
				int pmu_val = val & BMI160_CMD_PMU_VAL_MASK;

				switch (which) {
				case 0:
					shift = BMI160_PMU_STATUS_ACC_POS;
					break;
				case 1:
					shift = BMI160_PMU_STATUS_GYR_POS;
					break;
				case 2:
				default:
					shift = BMI160_PMU_STATUS_MAG_POS;
					break;
				}
				data->pmu_status &= 3 << shift;
				data->pmu_status |= pmu_val << shift;
				LOG_DBG("   * pmu %s = %x, new status %x", pmu_name[which], pmu_val,
					data->pmu_status);
			} else {
				LOG_DBG("Unknown command %x", val);
			}
			break;
		}
		break;
	default:
		LOG_DBG("Unknown write %x", regn);
	}
}

static int reg_read(const struct emul *target, int regn)
{
	struct bmi160_emul_data *data = target->data;
	const struct bmi160_emul_cfg *cfg = target->cfg;
	int val;

	LOG_DBG("read %x =", regn);
	val = cfg->reg[regn];
	switch (regn) {
	case BMI160_REG_CHIPID:
		LOG_DBG("   * get chipid");
		break;
	case BMI160_REG_PMU_STATUS:
		LOG_DBG("   * get pmu");
		val = data->pmu_status;
		break;
	case BMI160_REG_STATUS:
		LOG_DBG("   * status");
		val |= BMI160_DATA_READY_BIT_MASK;
		break;
	case BMI160_REG_ACC_CONF:
		LOG_DBG("   * acc conf");
		break;
	case BMI160_REG_GYR_CONF:
		LOG_DBG("   * gyr conf");
		break;
	case BMI160_SPI_START:
		LOG_DBG("   * Bus start");
		break;
	case BMI160_REG_ACC_RANGE:
		LOG_DBG("   * acc range");
		break;
	case BMI160_REG_GYR_RANGE:
		LOG_DBG("   * gyr range");
		break;
	default:
		LOG_DBG("Unknown read %x", regn);
	}
	LOG_DBG("       = %x", val);

	return val;
}

#if BMI160_BUS_SPI
static int bmi160_emul_io_spi(const struct emul *target, const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct bmi160_emul_data *data;
	const struct spi_buf *tx, *txd, *rxd;
	unsigned int regn, val;
	int count;

	ARG_UNUSED(config);

	data = target->data;

	__ASSERT_NO_MSG(tx_bufs || rx_bufs);
	__ASSERT_NO_MSG(!tx_bufs || !rx_bufs || tx_bufs->count == rx_bufs->count);
	count = tx_bufs ? tx_bufs->count : rx_bufs->count;

	if (count != 2) {
		LOG_DBG("Unknown tx_bufs->count %d", count);
		return -EIO;
	}
	tx = tx_bufs->buffers;
	txd = &tx_bufs->buffers[1];
	rxd = rx_bufs ? &rx_bufs->buffers[1] : NULL;

	if (tx->len != 1) {
		LOG_DBG("Unknown tx->len %d", tx->len);
		return -EIO;
	}

	regn = *(uint8_t *)tx->buf;
	if ((regn & BMI160_REG_READ) && rxd == NULL) {
		LOG_ERR("Cannot read without rxd");
		return -EPERM;
	}

	if (txd->len == 1) {
		if (regn & BMI160_REG_READ) {
			regn &= BMI160_REG_MASK;
			val = reg_read(target, regn);
			*(uint8_t *)rxd->buf = val;
		} else {
			val = *(uint8_t *)txd->buf;
			reg_write(target, regn, val);
		}
	} else {
		if (regn & BMI160_REG_READ) {
			regn &= BMI160_REG_MASK;
			for (int i = 0; i < txd->len; ++i) {
				((uint8_t *)rxd->buf)[i] = reg_read(target, regn + i);
			}
		} else {
			LOG_ERR("Unknown sample write");
			return -EIO;
		}
	}

	return 0;
}
#endif

#if BMI160_BUS_I2C
static int bmi160_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	struct bmi160_emul_data *data;

	data = target->data;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		if (msgs->len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msgs->len);
			return -EIO;
		}
		data->cur_reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			for (int i = 0; i < msgs->len; ++i) {
				msgs->buf[i] = reg_read(target, data->cur_reg + i);
			}
		} else {
			if (msgs->len != 1) {
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
			}
			reg_write(target, data->cur_reg, msgs->buf[0]);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}
#endif

/* Device instantiation */

#if BMI160_BUS_SPI
static struct spi_emul_api bmi160_emul_api_spi = {
	.io = bmi160_emul_io_spi,
};
#endif

#if BMI160_BUS_I2C
static struct i2c_emul_api bmi160_emul_api_i2c = {
	.transfer = bmi160_emul_transfer_i2c,
};
#endif

static int bmi160_emul_backend_set_channel(const struct emul *target, enum sensor_channel ch,
					   const q31_t *value, int8_t shift)
{
	const struct bmi160_emul_cfg *cfg = target->cfg;
	int64_t intermediate = *value;
	q31_t scale;
	int8_t scale_shift = 0;
	int reg_lsb;

	switch (ch) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		reg_lsb = BMI160_REG_DATA_ACC_X + (ch - SENSOR_CHAN_ACCEL_X) * 2;
		scale = 0x4e7404ea;

		switch (FIELD_GET(GENMASK(3, 0), cfg->reg[BMI160_REG_ACC_RANGE])) {
		case BMI160_ACC_RANGE_4G:
			scale_shift = 6;
			break;
		case BMI160_ACC_RANGE_8G:
			scale_shift = 7;
			break;
		case BMI160_ACC_RANGE_16G:
			scale_shift = 8;
			break;
		default:
			scale_shift = 5;
			break;
		}
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		reg_lsb = BMI160_REG_DATA_GYR_X + (ch - SENSOR_CHAN_GYRO_X) * 2;
		scale = 0x45d02bea;

		switch (FIELD_GET(GENMASK(2, 0), cfg->reg[BMI160_REG_GYR_RANGE])) {
		case BMI160_GYR_RANGE_2000DPS:
			scale_shift = 6;
			break;
		case BMI160_GYR_RANGE_1000DPS:
			scale_shift = 5;
			break;
		case BMI160_GYR_RANGE_500DPS:
			scale_shift = 4;
			break;
		case BMI160_GYR_RANGE_250DPS:
			scale_shift = 3;
			break;
		case BMI160_GYR_RANGE_125DPS:
			scale_shift = 2;
			break;
		default:
			return -EINVAL;
		}
		break;
	case SENSOR_CHAN_DIE_TEMP:
		reg_lsb = BMI160_REG_TEMPERATURE0;
		scale = 0x8000;
		scale_shift = 7;
		break;
	default:
		return -EINVAL;
	}

	if (shift < scale_shift) {
		/* Original value doesn't have enough int bits, fix it */
		intermediate >>= scale_shift - shift;
	} else if (shift > 0 && shift > scale_shift) {
		/* Original value might be out-of-bounds, fix it (we're going to lose precision) */
		intermediate <<= shift - scale_shift;
	}

	if (ch == SENSOR_CHAN_DIE_TEMP) {
		/* Need to subtract 23C */
		intermediate -= INT64_C(23) << (31 - scale_shift);
	}

	intermediate =
		CLAMP(DIV_ROUND_CLOSEST(intermediate * INT16_MAX, scale), INT16_MIN, INT16_MAX);

	cfg->reg[reg_lsb] = FIELD_GET(GENMASK64(7, 0), intermediate);
	cfg->reg[reg_lsb + 1] = FIELD_GET(GENMASK64(15, 8), intermediate);
	return 0;
}

static int bmi160_emul_backend_get_sample_range(const struct emul *target, enum sensor_channel ch,
						q31_t *lower, q31_t *upper, q31_t *epsilon,
						int8_t *shift)
{
	const struct bmi160_emul_cfg *cfg = target->cfg;

	switch (ch) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ: {
		uint8_t acc_range = cfg->reg[BMI160_REG_ACC_RANGE];

		switch (acc_range) {
		case BMI160_ACC_RANGE_2G:
			*shift = 5;
			break;
		case BMI160_ACC_RANGE_4G:
			*shift = 6;
			break;
		case BMI160_ACC_RANGE_8G:
			*shift = 7;
			break;
		case BMI160_ACC_RANGE_16G:
			*shift = 8;
			break;
		default:
			return -EINVAL;
		}
		int64_t intermediate = ((int64_t)(2 * 9.80665 * INT32_MAX)) >> 5;

		*upper = intermediate;
		*lower = -(*upper);
		*epsilon = intermediate * 2 / (1 << (16 - *shift));
		return 0;
	}
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ: {
		uint8_t gyro_range = cfg->reg[BMI160_REG_GYR_RANGE];

		switch (gyro_range) {
		case BMI160_GYR_RANGE_125DPS:
			*shift = 2;
			break;
		case BMI160_GYR_RANGE_250DPS:
			*shift = 3;
			break;
		case BMI160_GYR_RANGE_500DPS:
			*shift = 4;
			break;
		case BMI160_GYR_RANGE_1000DPS:
			*shift = 5;
			break;
		case BMI160_GYR_RANGE_2000DPS:
			*shift = 6;
			break;
		default:
			return -EINVAL;
		}

		int64_t intermediate = (int64_t)(125 * 3.141592654 * INT32_MAX / 180) >> 2;

		*upper = intermediate;
		*lower = -(*upper);
		*epsilon = intermediate * 2 / (1 << (16 - *shift));
		return 0;
	}
	default:
		return -EINVAL;
	}
}

static int bmi160_emul_backend_set_offset(const struct emul *target, enum sensor_channel ch,
					  const q31_t *values, int8_t shift)
{
	if (ch != SENSOR_CHAN_ACCEL_XYZ && ch != SENSOR_CHAN_GYRO_XYZ) {
		return -EINVAL;
	}

	const struct bmi160_emul_cfg *cfg = target->cfg;
	q31_t scale;
	int8_t scale_shift = 0;

	if (values[0] == 0 && values[1] == 0 && values[2] == 0) {
		if (ch == SENSOR_CHAN_ACCEL_XYZ) {
			cfg->reg[BMI160_REG_OFFSET_EN] &= ~BIT(BMI160_ACC_OFS_EN_POS);
		} else {
			cfg->reg[BMI160_REG_OFFSET_EN] &= ~BIT(BMI160_GYR_OFS_EN_POS);
		}
	} else {
		if (ch == SENSOR_CHAN_ACCEL_XYZ) {
			cfg->reg[BMI160_REG_OFFSET_EN] |= BIT(BMI160_ACC_OFS_EN_POS);
		} else {
			cfg->reg[BMI160_REG_OFFSET_EN] |= BIT(BMI160_GYR_OFS_EN_POS);
		}
	}

	if (ch == SENSOR_CHAN_ACCEL_XYZ) {
		/*
		 * bits = (values[i]mps2 / 9.80665g/mps2) / 0.0039g
		 *      = values[i] / 0.038245935mps2/bit
		 * 0.038245935 in Q31 format is 0x4e53e28 with shift 0
		 */
		scale = 0x4e53e28;
	} else {
		/*
		 * bits = (values[i]rad/s * 180 / pi) / 0.061deg/s
		 *      = values[i] / 0.001064651rad/s
		 */
		scale = 0x22e2f0;
	}

	for (int i = 0; i < 3; ++i) {
		int64_t intermediate = values[i];

		if (shift > scale_shift) {
			/* Input uses a bigger scale, we need to increase its value to match */
			intermediate <<= (shift - scale_shift);
		} else if (shift < scale_shift) {
			/* Scale uses a bigger shift, we need to decrease its value to match */
			scale >>= (scale_shift - shift);
		}

		int64_t reg_value = intermediate / scale;

		__ASSERT_NO_MSG(ch != SENSOR_CHAN_ACCEL_XYZ ||
				(reg_value >= INT8_MIN && reg_value <= INT8_MAX));
		__ASSERT_NO_MSG(ch != SENSOR_CHAN_GYRO_XYZ ||
				(reg_value >= -0x1ff - 1 && reg_value <= 0x1ff));
		if (ch == SENSOR_CHAN_ACCEL_XYZ) {
			cfg->reg[BMI160_REG_OFFSET_ACC_X + i] = reg_value & 0xff;
		} else {
			cfg->reg[BMI160_REG_OFFSET_GYR_X + i] = reg_value & 0xff;
			cfg->reg[BMI160_REG_OFFSET_EN] =
				(cfg->reg[BMI160_REG_OFFSET_EN] & ~GENMASK(i * 2 + 1, i * 2)) |
				(reg_value & GENMASK(9, 8));
		}
	}

	return 0;
}

static int bmi160_emul_backend_set_attribute(const struct emul *target, enum sensor_channel ch,
					     enum sensor_attribute attribute, const void *value)
{
	if (attribute == SENSOR_ATTR_OFFSET &&
	    (ch == SENSOR_CHAN_ACCEL_XYZ || ch == SENSOR_CHAN_GYRO_XYZ)) {
		const struct sensor_three_axis_attribute *attribute_value = value;

		return bmi160_emul_backend_set_offset(target, ch, attribute_value->values,
						      attribute_value->shift);
	}
	return -EINVAL;
}

static int bmi160_emul_backend_get_attribute_metadata(const struct emul *target,
						      enum sensor_channel ch,
						      enum sensor_attribute attribute, q31_t *min,
						      q31_t *max, q31_t *increment, int8_t *shift)
{
	ARG_UNUSED(target);
	switch (ch) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attribute == SENSOR_ATTR_OFFSET) {
			/* Offset uses 3.9mg per bit in an 8 bit register:
			 *   0.0039g * 9.8065m/s2: yields the increment in SI units
			 *   * INT8_MIN (or MAX) : yields the minimum (or maximum) values
			 *   * INT32_MAX >> 3    : converts to q31 format within range [-8, 8]
			 */
			*min = (q31_t)((int64_t)(0.0039 * 9.8065 * INT8_MIN * INT32_MAX) >> 3);
			*max = (q31_t)((int64_t)(0.0039 * 9.8065 * INT8_MAX * INT32_MAX) >> 3);
			*increment = (q31_t)((int64_t)(0.0039 * 9.8065 * INT32_MAX) >> 3);
			*shift = 3;
			return 0;
		}
		return -EINVAL;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attribute == SENSOR_ATTR_OFFSET) {
			/* Offset uses 0.061deg/s per bit in an 10 bit register:
			 *   0.061deg/s * pi / 180: yields the increment in SI units
			 *   * INT10_MIN (or MAX) : yields the minimum (or maximum) values
			 *   * INT32_MAX          : converts to q31 format within range [-1, 1]
			 */
			*min = (q31_t)(0.061 * 3.141593 / 180.0 * -512 * INT32_MAX);
			*max = (q31_t)(0.061 * 3.141593 / 180.0 * 511 * INT32_MAX);
			*increment = (q31_t)(0.061 * 3.141593 / 180.0 * INT32_MAX);
			*shift = 0;
			return 0;
		}
		return -EINVAL;
	default:
		return -EINVAL;
	}
}

static const struct emul_sensor_backend_api backend_api = {
	.set_channel = bmi160_emul_backend_set_channel,
	.get_sample_range = bmi160_emul_backend_get_sample_range,
	.set_attribute = bmi160_emul_backend_set_attribute,
	.get_attribute_metadata = bmi160_emul_backend_get_attribute_metadata,
};

static int emul_bosch_bmi160_init(const struct emul *target, const struct device *parent)
{
	const struct bmi160_emul_cfg *cfg = target->cfg;
	struct bmi160_emul_data *data = target->data;
	uint8_t *reg = cfg->reg;

	ARG_UNUSED(parent);

	data->pmu_status = 0;

	reg[BMI160_REG_CHIPID] = BMI160_CHIP_ID;

	return 0;
}

#define BMI160_EMUL_DATA(n)                                                                        \
	static uint8_t bmi160_emul_reg_##n[BMI160_REG_COUNT];                                      \
	static struct bmi160_emul_data bmi160_emul_data_##n;

#define BMI160_EMUL_DEFINE(n, bus_api)                                                             \
	EMUL_DT_INST_DEFINE(n, emul_bosch_bmi160_init, &bmi160_emul_data_##n,                      \
			    &bmi160_emul_cfg_##n, &bus_api, &backend_api)

/* Instantiation macros used when a device is on a SPI bus */
#define BMI160_EMUL_SPI(n)                                                                         \
	BMI160_EMUL_DATA(n)                                                                        \
	static const struct bmi160_emul_cfg bmi160_emul_cfg_##n = {                                \
		.reg = bmi160_emul_reg_##n, .chipsel = DT_INST_REG_ADDR(n)};                       \
	BMI160_EMUL_DEFINE(n, bmi160_emul_api_spi)

#define BMI160_EMUL_I2C(n)                                                                         \
	BMI160_EMUL_DATA(n)                                                                        \
	static const struct bmi160_emul_cfg bmi160_emul_cfg_##n = {.reg = bmi160_emul_reg_##n,     \
								   .addr = DT_INST_REG_ADDR(n)};   \
	BMI160_EMUL_DEFINE(n, bmi160_emul_api_i2c)

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define BMI160_EMUL(n)                                                                             \
	COND_CODE_1(DT_INST_ON_BUS(n, spi), (BMI160_EMUL_SPI(n)), (BMI160_EMUL_I2C(n)))

DT_INST_FOREACH_STATUS_OKAY(BMI160_EMUL)
