/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the Bosch BMI260, backing both the I2C and the SPI interface
 * with one register file so the driver can be exercised without hardware.
 */

#define DT_DRV_COMPAT bosch_bmi260

#include <string.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/logging/log.h>

#include "bmi260_emul.h"
#include "bmi260_reg_map.h"

LOG_MODULE_DECLARE(bmi260, CONFIG_SENSOR_LOG_LEVEL);

#define BMI260_EMUL_NUM_REGS 0x80

struct bmi260_emul_data {
	uint8_t reg[BMI260_EMUL_NUM_REGS];
	/* Latched by the address phase of a read, consumed by the data phase. */
	uint8_t selected_reg;
	/*
	 * Counters a test reads to see what the driver did. They survive a
	 * soft reset, which clears only the register file.
	 */
	uint32_t soft_reset_count;
	/* Cleared by a soft reset, so it tells apart accesses made after one. */
	uint32_t chip_id_reads_since_reset;
	uint16_t write_count[BMI260_EMUL_NUM_REGS];
	/* Configuration upload bookkeeping. */
	uint32_t init_data_bytes;
	uint16_t last_init_addr;
	bool fail_config_load;
	/* When non-zero, the Nth write from now on fails, to test abort paths. */
	uint32_t fail_write_after;
};

void bmi260_emul_set_reg(const struct emul *target, uint8_t reg, const uint8_t *val, size_t len)
{
	struct bmi260_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg + len <= BMI260_EMUL_NUM_REGS);
	memcpy(&data->reg[reg], val, len);
}

void bmi260_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *val, size_t len)
{
	struct bmi260_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg + len <= BMI260_EMUL_NUM_REGS);
	memcpy(val, &data->reg[reg], len);
}

uint32_t bmi260_emul_get_soft_reset_count(const struct emul *target)
{
	struct bmi260_emul_data *data = target->data;

	return data->soft_reset_count;
}

uint32_t bmi260_emul_get_chip_id_reads_since_reset(const struct emul *target)
{
	struct bmi260_emul_data *data = target->data;

	return data->chip_id_reads_since_reset;
}

uint16_t bmi260_emul_get_write_count(const struct emul *target, uint8_t reg)
{
	struct bmi260_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < BMI260_EMUL_NUM_REGS);

	return data->write_count[reg];
}

uint32_t bmi260_emul_get_init_data_byte_count(const struct emul *target)
{
	struct bmi260_emul_data *data = target->data;

	return data->init_data_bytes;
}

uint16_t bmi260_emul_get_last_init_addr(const struct emul *target)
{
	struct bmi260_emul_data *data = target->data;

	return data->last_init_addr;
}

void bmi260_emul_set_fail_config_load(const struct emul *target, bool fail)
{
	struct bmi260_emul_data *data = target->data;

	data->fail_config_load = fail;
}

void bmi260_emul_fail_write_after(const struct emul *target, uint32_t writes)
{
	struct bmi260_emul_data *data = target->data;

	data->fail_write_after = writes;
}

static void bmi260_emul_reset(const struct emul *target)
{
	struct bmi260_emul_data *data = target->data;

	memset(data->reg, 0, sizeof(data->reg));

	/*
	 * Only the registers the driver reads before it has configured
	 * anything need a power-on value. The configuration file has not been
	 * uploaded yet, so the feature engine reports that it is not
	 * initialized.
	 */
	data->reg[BMI260_REG_CHIP_ID] = BMI260_CHIP_ID;
	data->reg[BMI260_REG_INTERNAL_STATUS] = BMI260_INST_MESSAGE_NOT_INIT;

	data->chip_id_reads_since_reset = 0U;
	data->init_data_bytes = 0U;
	data->last_init_addr = 0U;
}

static uint8_t bmi260_emul_read_reg(const struct emul *target, uint8_t reg)
{
	struct bmi260_emul_data *data = target->data;
	uint8_t val;

	if (reg >= BMI260_EMUL_NUM_REGS) {
		return 0;
	}

	val = data->reg[reg];

	if (reg == BMI260_REG_CHIP_ID) {
		data->chip_id_reads_since_reset++;
	}

	/*
	 * Real silicon raises data-ready at the output data rate. Reporting it
	 * on every read spares the tests from having to drive it, and a driver
	 * that waits for it never spins.
	 */
	if (reg == BMI260_REG_STATUS) {
		val |= BMI260_STATUS_DRDY_ACC | BMI260_STATUS_DRDY_GYR;
	}

	/*
	 * The interrupt status registers latch until they are read, which is
	 * what lets the device raise an edge and the driver find out why.
	 */
	if ((reg == BMI260_REG_INT_STATUS_0) || (reg == BMI260_REG_INT_STATUS_1)) {
		data->reg[reg] = 0U;
	}

	return val;
}

/*
 * The configuration write port does not advance: the device carries its own
 * address, taken from INIT_ADDR, and increments it internally. Letting a burst
 * to it walk forward would scribble over PWR_CONF, PWR_CTRL and CMD, which sit
 * just above it.
 */
static bool bmi260_emul_reg_advances(uint8_t reg)
{
	return reg != BMI260_REG_INIT_DATA;
}

static int bmi260_emul_write_reg(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct bmi260_emul_data *data = target->data;

	if (reg >= BMI260_EMUL_NUM_REGS) {
		return -EIO;
	}

	if (data->fail_write_after > 0U) {
		data->fail_write_after--;
		if (data->fail_write_after == 0U) {
			return -EIO;
		}
	}

	data->write_count[reg]++;

	if (reg == BMI260_REG_INIT_DATA) {
		/* Consumed by the feature engine rather than stored. */
		data->init_data_bytes++;
		return 0;
	}

	data->reg[reg] = val;

	if ((reg == BMI260_REG_CMD) && (val == BMI260_CMD_SOFT_RESET)) {
		bmi260_emul_reset(target);
		data->soft_reset_count++;
		return 0;
	}

	if (reg == BMI260_REG_INIT_ADDR_1) {
		/*
		 * The address is written as a pair, low nibble first, and
		 * counts 16-bit words.
		 */
		data->last_init_addr = (uint16_t)((data->reg[BMI260_REG_INIT_ADDR_1] << 4) |
						  (data->reg[BMI260_REG_INIT_ADDR_0] & 0x0FU));
	}

	if ((reg == BMI260_REG_INIT_CTRL) && (val == BMI260_COMPLETE_CONFIG_LOAD)) {
		bool complete = (data->init_data_bytes == BMI260_CONFIG_FILE_SIZE) &&
				!data->fail_config_load;

		data->reg[BMI260_REG_INTERNAL_STATUS] =
			complete ? BMI260_INST_MESSAGE_INIT_OK : BMI260_INST_MESSAGE_INIT_ERR;
	}

	return 0;
}

static int bmi260_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	struct bmi260_emul_data *data = target->data;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	for (int i = 0; i < num_msgs; i++) {
		struct i2c_msg *msg = &msgs[i];

		if ((msg->flags & I2C_MSG_READ) != 0) {
			for (uint32_t byte = 0; byte < msg->len; byte++) {
				msg->buf[byte] = bmi260_emul_read_reg(
					target, data->selected_reg + byte);
			}
			continue;
		}

		if (msg->len == 0U) {
			continue;
		}

		/*
		 * A write message opens with the register address. Any bytes
		 * after it are payload for consecutive registers, which is how
		 * the driver sends a burst.
		 */
		data->selected_reg = msg->buf[0];

		for (uint32_t byte = 1; byte < msg->len; byte++) {
			uint8_t reg = bmi260_emul_reg_advances(data->selected_reg)
					      ? (uint8_t)(data->selected_reg + (byte - 1U))
					      : data->selected_reg;
			int ret = bmi260_emul_write_reg(target, reg, msg->buf[byte]);

			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int bmi260_emul_io_spi(const struct emul *target, const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	const struct spi_buf *tx;
	uint8_t reg;
	int count;

	ARG_UNUSED(config);
	__ASSERT_NO_MSG((tx_bufs != NULL) || (rx_bufs != NULL));

	count = (tx_bufs != NULL) ? tx_bufs->count : rx_bufs->count;
	if (count < 1) {
		LOG_ERR("Invalid SPI buffer count %d", count);
		return -EIO;
	}

	tx = (tx_bufs != NULL) ? tx_bufs->buffers : NULL;
	if ((tx == NULL) || (tx->buf == NULL) || (tx->len < 1U)) {
		LOG_ERR("Missing SPI register address");
		return -EIO;
	}

	reg = *(const uint8_t *)tx->buf;

	if ((reg & BMI260_SPI_READ_BIT) != 0U) {
		const struct spi_buf *rxd;

		/*
		 * A read sends only the address, so its transmit set holds one
		 * buffer while the receive set holds two: the bytes to discard
		 * and the register contents. The counts are deliberately not
		 * compared against each other; the controller pads the shorter
		 * side.
		 */
		if ((rx_bufs == NULL) || (rx_bufs->count != 2) || (tx->len != 1U)) {
			LOG_ERR("Invalid SPI read transaction");
			return -EIO;
		}

		/*
		 * The driver throws away the first buffer, which covers the
		 * address slot and the device's padding byte, so only the
		 * second one carries register contents.
		 */
		rxd = &rx_bufs->buffers[1];
		if (rxd->buf == NULL) {
			return -EPERM;
		}

		reg &= BMI260_REG_MASK;
		for (uint32_t byte = 0; byte < rxd->len; byte++) {
			((uint8_t *)rxd->buf)[byte] = bmi260_emul_read_reg(target, reg + byte);
		}

		return 0;
	}

	/*
	 * A write puts the address on the wire followed by payload for
	 * consecutive registers. The byte stream is parsed independently of
	 * where the spi_buf boundaries happen to fall, because the driver
	 * sends the address and the payload as separate buffers.
	 */
	reg &= BMI260_REG_MASK;

	for (int buffer = 0; buffer < count; buffer++) {
		const struct spi_buf *msg = &tx_bufs->buffers[buffer];
		const uint8_t *buf = msg->buf;
		size_t first = (buffer == 0) ? 1U : 0U;

		if ((buf == NULL) && (msg->len > 0U)) {
			LOG_ERR("Missing SPI write data");
			return -EIO;
		}

		for (size_t byte = first; byte < msg->len; byte++) {
			int ret = bmi260_emul_write_reg(target, reg, buf[byte]);

			if (ret != 0) {
				return ret;
			}

			if (bmi260_emul_reg_advances(reg)) {
				reg++;
			}
		}
	}

	return 0;
}

static int bmi260_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	bmi260_emul_reset(target);

	return 0;
}

/*
 * The sensor backend below is what lets the generic sensor test drive this
 * device: it asks the emulator for a channel's range, plants values across it
 * and checks what the driver reports back. The emulator therefore has to hold
 * the exact inverse of the driver's conversion, in the same units.
 */

/* Full scale of the accelerometer in micro-m/s^2, from the range register. */
static int bmi260_emul_acc_full_scale(const struct bmi260_emul_data *data, int64_t *micro,
				      int8_t *shift)
{
	switch (data->reg[BMI260_REG_ACC_RANGE] & BMI260_ACC_RANGE_MSK) {
	case BMI260_ACC_RANGE_2G:
		*micro = 2LL * SENSOR_G;
		*shift = 5;
		return 0;
	case BMI260_ACC_RANGE_4G:
		*micro = 4LL * SENSOR_G;
		*shift = 6;
		return 0;
	case BMI260_ACC_RANGE_8G:
		*micro = 8LL * SENSOR_G;
		*shift = 7;
		return 0;
	case BMI260_ACC_RANGE_16G:
		*micro = 16LL * SENSOR_G;
		*shift = 8;
		return 0;
	default:
		return -EINVAL;
	}
}

/* Full scale of the gyroscope in micro-rad/s, from the range register. */
static int bmi260_emul_gyr_full_scale(const struct bmi260_emul_data *data, int64_t *micro,
				      int8_t *shift)
{
	int64_t dps;
	int8_t sh;

	switch (data->reg[BMI260_REG_GYR_RANGE] & BMI260_GYR_RANGE_MSK) {
	case BMI260_GYR_RANGE_2000DPS:
		dps = 2000;
		sh = 6;
		break;
	case BMI260_GYR_RANGE_1000DPS:
		dps = 1000;
		sh = 5;
		break;
	case BMI260_GYR_RANGE_500DPS:
		dps = 500;
		sh = 4;
		break;
	case BMI260_GYR_RANGE_250DPS:
		dps = 250;
		sh = 3;
		break;
	case BMI260_GYR_RANGE_125DPS:
		dps = 125;
		sh = 2;
		break;
	default:
		return -EINVAL;
	}

	*micro = (dps * SENSOR_PI) / 180LL;
	*shift = sh;

	return 0;
}

/* q31 at the given shift, to the micro-unit the driver's conversion uses. */
static int64_t bmi260_emul_q31_to_micro(q31_t value, int8_t shift)
{
	return ((int64_t)value * (1000000LL << shift)) / ((int64_t)INT32_MAX + 1);
}

/* The reverse, for reporting a range. */
static q31_t bmi260_emul_micro_to_q31(int64_t micro, int8_t shift)
{
	return (q31_t)((micro * ((int64_t)INT32_MAX + 1)) / (1000000LL << shift));
}

static void bmi260_emul_store_raw(struct bmi260_emul_data *data, uint8_t reg, int16_t raw)
{
	data->reg[reg] = (uint8_t)((uint16_t)raw & 0xFFU);
	data->reg[reg + 1] = (uint8_t)(((uint16_t)raw >> 8) & 0xFFU);
}

static int bmi260_emul_backend_set_channel(const struct emul *target, struct sensor_chan_spec ch,
					   const q31_t *value, int8_t shift)
{
	struct bmi260_emul_data *data = target->data;
	int64_t micro = bmi260_emul_q31_to_micro(*value, shift);
	int64_t full_scale;
	int64_t raw;
	int8_t unused;
	uint8_t reg;
	int ret;

	switch (ch.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		ret = bmi260_emul_acc_full_scale(data, &full_scale, &unused);
		if (ret != 0) {
			return ret;
		}
		reg = BMI260_REG_ACC_X_LSB + (ch.chan_type - SENSOR_CHAN_ACCEL_X) * 2;
		raw = (micro * INT16_MAX) / full_scale;
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		ret = bmi260_emul_gyr_full_scale(data, &full_scale, &unused);
		if (ret != 0) {
			return ret;
		}
		reg = BMI260_REG_GYR_X_LSB + (ch.chan_type - SENSOR_CHAN_GYRO_X) * 2;
		raw = (micro * INT16_MAX) / full_scale;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		reg = BMI260_REG_TEMPERATURE_0;
		raw = ((micro - BMI260_TEMP_CENTER_OFFSET) * BMI260_TEMP_LSB_PER_DEG) / 1000000LL;
		break;
	default:
		return -ENOTSUP;
	}

	/*
	 * The reported range is inclusive of both endpoints, so a request for
	 * exactly full scale can land one count past it once the division has
	 * rounded. Clamp rather than wrap into the opposite sign. INT16_MIN is
	 * excluded because the device spells it "no reading yet".
	 */
	raw = CLAMP(raw, INT16_MIN + 1, INT16_MAX);
	bmi260_emul_store_raw(data, reg, (int16_t)raw);

	return 0;
}

static int bmi260_emul_backend_get_sample_range(const struct emul *target,
						struct sensor_chan_spec ch, q31_t *lower,
						q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	struct bmi260_emul_data *data = target->data;
	int64_t full_scale;
	int ret;

	switch (ch.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		ret = bmi260_emul_acc_full_scale(data, &full_scale, shift);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		ret = bmi260_emul_gyr_full_scale(data, &full_scale, shift);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		/*
		 * The thermometer counts 512 per degree from a zero meaning
		 * 23 C, so a 16-bit count spans roughly -41 C to +87 C.
		 */
		*shift = 7;
		*upper = bmi260_emul_micro_to_q31(
			BMI260_TEMP_CENTER_OFFSET +
				((int64_t)INT16_MAX * 1000000LL) / BMI260_TEMP_LSB_PER_DEG,
			*shift);
		*lower = bmi260_emul_micro_to_q31(
			BMI260_TEMP_CENTER_OFFSET -
				((int64_t)INT16_MAX * 1000000LL) / BMI260_TEMP_LSB_PER_DEG,
			*shift);
		*epsilon = bmi260_emul_micro_to_q31(4LL * 1000000LL / BMI260_TEMP_LSB_PER_DEG,
						    *shift);
		return 0;
	default:
		return -ENOTSUP;
	}

	if (ret != 0) {
		return ret;
	}

	*upper = bmi260_emul_micro_to_q31(full_scale, *shift);
	*lower = -(*upper);
	/*
	 * Four counts, not one: the value is rounded twice on the way back,
	 * once converting to the register and once converting out of it.
	 */
	*epsilon = bmi260_emul_micro_to_q31((full_scale * 4) / INT16_MAX, *shift);

	return 0;
}

static const struct emul_sensor_driver_api bmi260_emul_backend_api = {
	.set_channel = bmi260_emul_backend_set_channel,
	.get_sample_range = bmi260_emul_backend_get_sample_range,
};

static const struct i2c_emul_api bmi260_emul_api_i2c = {
	.transfer = bmi260_emul_transfer_i2c,
};

static const struct spi_emul_api bmi260_emul_api_spi = {
	.io = bmi260_emul_io_spi,
};

#define BMI260_EMUL_DEFINE(inst, bus_api)                                                          \
	static struct bmi260_emul_data bmi260_emul_data_##inst;                                    \
                                                                                                   \
	EMUL_DT_INST_DEFINE(inst, bmi260_emul_init, &bmi260_emul_data_##inst, NULL, &bus_api,      \
			    &bmi260_emul_backend_api)

/*
 * EMUL_DT_INST_DEFINE picks the bus emulator type from the node's parent, so
 * the api handed to it has to match the bus the instance is on.
 */
#define BMI260_EMUL(inst)                                                                          \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (BMI260_EMUL_DEFINE(inst, bmi260_emul_api_spi)),    \
		    (BMI260_EMUL_DEFINE(inst, bmi260_emul_api_i2c)))

DT_INST_FOREACH_STATUS_OKAY(BMI260_EMUL)
