/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT asahi_kasei_ak09940a

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ak09940a_emul.h"
#include "ak09940a_reg.h"

LOG_MODULE_DECLARE(AK09940A, CONFIG_SENSOR_LOG_LEVEL);

#define NUM_REGS (AK09940A_REG_I2CDIS + 1)

#define MAGN_MIN_RAW        (-131072)
#define MAGN_MAX_RAW        131070
#define MICRO_GAUSS_PER_LSB 100
#define MAGN_SHIFT          4
#define TEMP_SHIFT          7

struct ak09940a_emul_data {
	uint8_t reg[NUM_REGS];
};

void ak09940a_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val,
			   size_t count)
{
	struct ak09940a_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count <= NUM_REGS);
	memcpy(&data->reg[reg_addr], val, count);
}

void ak09940a_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val, size_t count)
{
	struct ak09940a_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count <= NUM_REGS);
	memcpy(val, &data->reg[reg_addr], count);
}

void ak09940a_emul_reset(const struct emul *target)
{
	struct ak09940a_emul_data *data = target->data;

	memset(data->reg, 0, sizeof(data->reg));
	data->reg[AK09940A_REG_WIA1] = AK099XX_WIA1_AKM;
	data->reg[AK09940A_REG_WIA2] = AK09940A_WIA2;
	data->reg[AK09940A_REG_CNTL2] = AK09940A_CNTL2_TEM;
}

/* Register auto-increment with the FIFO disabled */
static uint8_t ak09940a_emul_next_reg(uint8_t reg)
{
	switch (reg) {
	case AK09940A_REG_RSV2:
		return AK09940A_REG_ST1;
	case AK09940A_REG_ST2:
		return AK09940A_REG_WIA1;
	case AK09940A_REG_CNTL4:
		return AK09940A_REG_CNTL1;
	default:
		return reg + 1U;
	}
}

static uint8_t ak09940a_emul_read(const struct emul *target, uint8_t reg, bool *data_read)
{
	struct ak09940a_emul_data *data = target->data;

	if (reg >= NUM_REGS) {
		return 0;
	}

	if ((reg >= AK09940A_REG_HXL) && (reg <= AK09940A_REG_ST2)) {
		*data_read = true;
	}

	if (reg == AK09940A_REG_ST) {
		return (data->reg[AK09940A_REG_ST1] & AK099XX_ST1_DRDY) |
		       ((data->reg[AK09940A_REG_ST2] & AK09940A_ST2_DOR) << 1);
	}

	return data->reg[reg];
}

static void ak09940a_emul_read_done(const struct emul *target, bool data_read)
{
	struct ak09940a_emul_data *data = target->data;

	/* Reading measurement data or ST2 clears DRDY */
	if (data_read) {
		data->reg[AK09940A_REG_ST1] &= ~AK099XX_ST1_DRDY;
	}
}

static void ak09940a_emul_write(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct ak09940a_emul_data *data = target->data;
	uint8_t mode;

	switch (reg) {
	case AK09940A_REG_CNTL1:
	case AK09940A_REG_CNTL2:
	case AK09940A_REG_I2CDIS:
		data->reg[reg] = val;
		break;
	case AK09940A_REG_CNTL3:
		/* Measurements complete instantly on the data set by the backend API */
		mode = FIELD_GET(AK09940A_CNTL3_MODE, val);
		if (mode != AK099XX_MODE_POWER_DOWN) {
			data->reg[AK09940A_REG_ST1] |= AK099XX_ST1_DRDY;
		}
		if ((mode == AK099XX_MODE_SINGLE) || (mode == AK099XX_MODE_SELF_TEST)) {
			val &= ~AK09940A_CNTL3_MODE;
		}
		data->reg[reg] = val;
		break;
	case AK09940A_REG_CNTL4:
		if ((val & AK09940A_CNTL4_SRST) != 0U) {
			ak09940a_emul_reset(target);
		}
		break;
	default:
		LOG_WRN("Write to read-only register 0x%02x", reg);
		break;
	}
}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
static int ak09940a_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				      int addr)
{
	struct ak09940a_emul_data *data = target->data;
	bool data_read = false;
	uint8_t reg;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (data->reg[AK09940A_REG_I2CDIS] == AK09940A_I2CDIS_DISABLE) {
		return -EIO;
	}

	if ((num_msgs < 1) || ((msgs[0].flags & I2C_MSG_READ) != 0U) || (msgs[0].len < 1U)) {
		LOG_ERR("Unexpected I2C transfer");
		return -EIO;
	}

	reg = msgs[0].buf[0];

	if (num_msgs == 1) {
		for (uint32_t i = 1; i < msgs[0].len; i++) {
			ak09940a_emul_write(target, reg, msgs[0].buf[i]);
			reg = ak09940a_emul_next_reg(reg);
		}
		return 0;
	}

	if ((num_msgs != 2) || ((msgs[1].flags & I2C_MSG_READ) == 0U)) {
		LOG_ERR("Unexpected I2C transfer");
		return -EIO;
	}

	for (uint32_t i = 0; i < msgs[1].len; i++) {
		msgs[1].buf[i] = ak09940a_emul_read(target, reg, &data_read);
		reg = ak09940a_emul_next_reg(reg);
	}
	ak09940a_emul_read_done(target, data_read);

	return 0;
}

static const struct i2c_emul_api ak09940a_emul_api_i2c = {
	.transfer = ak09940a_emul_transfer_i2c,
};
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
/* Byte at offset @p pos of the concatenated buffers, NULL if absent or not backed */
static uint8_t *ak09940a_emul_spi_byte(const struct spi_buf_set *bufs, size_t pos)
{
	if (bufs == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < bufs->count; i++) {
		if (pos < bufs->buffers[i].len) {
			return (bufs->buffers[i].buf == NULL)
				       ? NULL
				       : (uint8_t *)bufs->buffers[i].buf + pos;
		}
		pos -= bufs->buffers[i].len;
	}

	return NULL;
}

static size_t ak09940a_emul_spi_len(const struct spi_buf_set *bufs)
{
	size_t len = 0;

	for (size_t i = 0; (bufs != NULL) && (i < bufs->count); i++) {
		len += bufs->buffers[i].len;
	}

	return len;
}

static int ak09940a_emul_io_spi(const struct emul *target, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	size_t len = MAX(ak09940a_emul_spi_len(tx_bufs), ak09940a_emul_spi_len(rx_bufs));
	const uint8_t *cmd = ak09940a_emul_spi_byte(tx_bufs, 0);
	bool data_read = false;
	uint8_t reg;

	ARG_UNUSED(config);

	if ((cmd == NULL) || (len < 2U)) {
		LOG_ERR("Unexpected SPI transfer");
		return -EIO;
	}

	reg = *cmd & ~AK09940A_SPI_READ;

	if ((*cmd & AK09940A_SPI_READ) == 0U) {
		const uint8_t *val = ak09940a_emul_spi_byte(tx_bufs, 1);

		if (val == NULL) {
			return -EIO;
		}
		ak09940a_emul_write(target, reg, *val);
		return 0;
	}

	for (size_t i = 1; i < len; i++) {
		uint8_t *out = ak09940a_emul_spi_byte(rx_bufs, i);
		uint8_t val = ak09940a_emul_read(target, reg, &data_read);

		if (out != NULL) {
			*out = val;
		}
		reg = ak09940a_emul_next_reg(reg);
	}
	ak09940a_emul_read_done(target, data_read);

	return 0;
}

static const struct spi_emul_api ak09940a_emul_api_spi = {
	.io = ak09940a_emul_io_spi,
};
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

static int ak09940a_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	ak09940a_emul_reset(target);

	return 0;
}

static int64_t ak09940a_emul_q31_to_micro(q31_t value, int8_t shift)
{
	int64_t shifted = (shift < 0) ? ((int64_t)value >> -shift) : ((int64_t)value << shift);

	return (shifted * 1000000) / ((int64_t)INT32_MAX + 1);
}

static q31_t ak09940a_emul_micro_to_q31(int64_t micro, int8_t shift)
{
	return (q31_t)((micro * (INT64_C(1) << (31 - shift))) / 1000000);
}

static int ak09940a_emul_backend_set_channel(const struct emul *target, struct sensor_chan_spec ch,
					     const q31_t *value, int8_t shift)
{
	struct ak09940a_emul_data *data = target->data;
	int64_t micro = ak09940a_emul_q31_to_micro(*value, shift);
	int32_t raw;
	uint8_t reg;

	if (ch.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (ch.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		raw = CLAMP(DIV_ROUND_CLOSEST(micro, MICRO_GAUSS_PER_LSB), MAGN_MIN_RAW,
			    MAGN_MAX_RAW);
		reg = AK09940A_REG_HXL + (3 * (ch.chan_type - SENSOR_CHAN_MAGN_X));
		/* 18-bit two's complement, HxH bits 7:1 repeat the sign bit */
		sys_put_le24((uint32_t)raw, &data->reg[reg]);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		/* Temperature = 30 - TMPS / 1.7 */
		raw = CLAMP(DIV_ROUND_CLOSEST((INT64_C(30000000) - micro) * 17, INT64_C(10000000)),
			    INT8_MIN, INT8_MAX);
		data->reg[AK09940A_REG_TMPS] = (uint8_t)(int8_t)raw;
		break;
	default:
		return -ENOTSUP;
	}

	data->reg[AK09940A_REG_ST1] |= AK099XX_ST1_DRDY;

	return 0;
}

static int ak09940a_emul_backend_get_sample_range(const struct emul *target,
						  struct sensor_chan_spec ch, q31_t *lower,
						  q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	ARG_UNUSED(target);

	if (ch.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (ch.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		*shift = MAGN_SHIFT;
		*lower = ak09940a_emul_micro_to_q31(MAGN_MIN_RAW * MICRO_GAUSS_PER_LSB, MAGN_SHIFT);
		*upper = ak09940a_emul_micro_to_q31(MAGN_MAX_RAW * MICRO_GAUSS_PER_LSB, MAGN_SHIFT);
		*epsilon = ak09940a_emul_micro_to_q31(MICRO_GAUSS_PER_LSB, MAGN_SHIFT);
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		/* TMPS 127 to -128, one LSB is 1 / 1.7 degree */
		*shift = TEMP_SHIFT;
		*lower = ak09940a_emul_micro_to_q31(INT64_C(30000000) - INT64_C(1270000000) / 17,
						    TEMP_SHIFT);
		*upper = ak09940a_emul_micro_to_q31(INT64_C(30000000) + INT64_C(1280000000) / 17,
						    TEMP_SHIFT);
		*epsilon = ak09940a_emul_micro_to_q31(INT64_C(10000000) / 17, TEMP_SHIFT);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static const struct emul_sensor_driver_api ak09940a_emul_sensor_driver_api = {
	.set_channel = ak09940a_emul_backend_set_channel,
	.get_sample_range = ak09940a_emul_backend_get_sample_range,
};

#define AK09940A_EMUL(n)                                                                           \
	static struct ak09940a_emul_data ak09940a_emul_data_##n;                                   \
	EMUL_DT_INST_DEFINE(n, ak09940a_emul_init, &ak09940a_emul_data_##n, NULL,                  \
			    COND_CODE_1(DT_INST_ON_BUS(n, spi), (&ak09940a_emul_api_spi),          \
					(&ak09940a_emul_api_i2c)),                                 \
			    &ak09940a_emul_sensor_driver_api)

DT_INST_FOREACH_STATUS_OKAY(AK09940A_EMUL)
