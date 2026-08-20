/*
 * Copyright (c) 2026 Chaogui Deng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmi323

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "bmi323.h"
#include "bmi323_emul.h"

#define BMI323_EMUL_NUM_REGS 0x80
#define BMI323_I2C_DUMMY_BYTES 2
#define BMI323_SPI_READ_BIT BIT(7)

struct bmi323_emul_data {
	uint16_t reg[BMI323_EMUL_NUM_REGS];
};

static void bmi323_emul_reset(const struct emul *target)
{
	struct bmi323_emul_data *data = target->data;

	memset(data->reg, 0, sizeof(data->reg));
	data->reg[IMU_BOSCH_BMI323_REG_CHIP_ID] = 0x43;
	data->reg[IMU_BOSCH_BMI323_REG_ACC_DATA_X] = 0x8000U;
	data->reg[IMU_BOSCH_BMI323_REG_ACC_DATA_Y] = 0x8000U;
	data->reg[IMU_BOSCH_BMI323_REG_ACC_DATA_Z] = 0x8000U;
	data->reg[IMU_BOSCH_BMI323_REG_GYRO_DATA_X] = 0x8000U;
	data->reg[IMU_BOSCH_BMI323_REG_GYRO_DATA_Y] = 0x8000U;
	data->reg[IMU_BOSCH_BMI323_REG_GYRO_DATA_Z] = 0x8000U;
	data->reg[IMU_BOSCH_BMI323_REG_TEMP_DATA] = 0x8000U;
	data->reg[IMU_BOSCH_BMI323_REG_ACC_CONF] =
		IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, ODR, HZ100) |
		IMU_BOSCH_BMI323_REG_VALUE(ACC_CONF, RANGE, G8);
	data->reg[IMU_BOSCH_BMI323_REG_GYRO_CONF] =
		IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, ODR, HZ100) |
		IMU_BOSCH_BMI323_REG_VALUE(GYRO_CONF, RANGE, DPS2000);
}

static int bmi323_emul_read_words(const struct emul *target, uint8_t reg, uint8_t *buf,
				  size_t len, size_t dummy_bytes)
{
	struct bmi323_emul_data *data = target->data;
	size_t words;

	if (buf == NULL || len < dummy_bytes ||
	    ((len - dummy_bytes) % sizeof(uint16_t)) != 0) {
		return -EIO;
	}

	words = (len - dummy_bytes) / sizeof(uint16_t);
	if ((size_t)reg + words > ARRAY_SIZE(data->reg)) {
		return -EINVAL;
	}

	memset(buf, 0, dummy_bytes);
	for (size_t i = 0; i < words; i++) {
		sys_put_le16(data->reg[reg + i], &buf[dummy_bytes + i * sizeof(uint16_t)]);
	}

	return 0;
}

static int bmi323_emul_write_words(const struct emul *target, uint8_t reg, const uint8_t *buf,
				   size_t len)
{
	struct bmi323_emul_data *data = target->data;
	size_t words;

	if (buf == NULL || len == 0 || (len % sizeof(uint16_t)) != 0) {
		return -EIO;
	}

	words = len / sizeof(uint16_t);
	if ((size_t)reg + words > ARRAY_SIZE(data->reg)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < words; i++) {
		uint16_t value = sys_get_le16(&buf[i * sizeof(uint16_t)]);

		if (reg + i == IMU_BOSCH_BMI323_REG_CMD &&
		    value == IMU_BOSCH_BMI323_REG_CMD_CMD_VAL_SOFT_RESET) {
			bmi323_emul_reset(target);
		} else {
			data->reg[reg + i] = value;
		}
	}

	return 0;
}

void bmi323_emul_set_accel_raw(const struct emul *target, int16_t x, int16_t y, int16_t z)
{
	struct bmi323_emul_data *data = target->data;

	data->reg[IMU_BOSCH_BMI323_REG_ACC_DATA_X] = (uint16_t)x;
	data->reg[IMU_BOSCH_BMI323_REG_ACC_DATA_Y] = (uint16_t)y;
	data->reg[IMU_BOSCH_BMI323_REG_ACC_DATA_Z] = (uint16_t)z;
}

void bmi323_emul_set_gyro_raw(const struct emul *target, int16_t x, int16_t y, int16_t z)
{
	struct bmi323_emul_data *data = target->data;

	data->reg[IMU_BOSCH_BMI323_REG_GYRO_DATA_X] = (uint16_t)x;
	data->reg[IMU_BOSCH_BMI323_REG_GYRO_DATA_Y] = (uint16_t)y;
	data->reg[IMU_BOSCH_BMI323_REG_GYRO_DATA_Z] = (uint16_t)z;
}

void bmi323_emul_set_temperature_raw(const struct emul *target, int16_t temperature)
{
	struct bmi323_emul_data *data = target->data;

	data->reg[IMU_BOSCH_BMI323_REG_TEMP_DATA] = (uint16_t)temperature;
}

#ifdef CONFIG_BMI323_BUS_I2C
static int bmi323_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				    int num_msgs, int addr)
{
	ARG_UNUSED(addr);

	if (msgs == NULL || num_msgs < 1 || msgs[0].buf == NULL ||
	    (msgs[0].flags & I2C_MSG_READ) || msgs[0].len < 1) {
		return -EIO;
	}

	uint8_t reg = msgs[0].buf[0];

	if (num_msgs == 1) {
		return bmi323_emul_write_words(target, reg, &msgs[0].buf[1], msgs[0].len - 1);
	}

	if (num_msgs == 2 && (msgs[1].flags & I2C_MSG_READ) && msgs[0].len == 1) {
		return bmi323_emul_read_words(target, reg, msgs[1].buf, msgs[1].len,
					      BMI323_I2C_DUMMY_BYTES);
	}

	return -EIO;
}
#endif /* CONFIG_BMI323_BUS_I2C */

#ifdef CONFIG_BMI323_BUS_SPI
static int bmi323_emul_io_spi(const struct emul *target, const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	const struct spi_buf *address;
	uint8_t reg;

	ARG_UNUSED(config);

	if (tx_bufs == NULL || tx_bufs->count < 1) {
		return -EIO;
	}

	address = &tx_bufs->buffers[0];
	if (address->buf == NULL || address->len < 1) {
		return -EIO;
	}

	reg = *(uint8_t *)address->buf;
	if (reg & BMI323_SPI_READ_BIT) {
		if (tx_bufs->count != 1 || address->len != 2 || rx_bufs == NULL ||
		    rx_bufs->count != 2 || rx_bufs->buffers[1].buf == NULL) {
			return -EIO;
		}

		reg &= ~BMI323_SPI_READ_BIT;
		return bmi323_emul_read_words(target, reg, rx_bufs->buffers[1].buf,
					      rx_bufs->buffers[1].len, 0);
	}

	if (tx_bufs->count != 2 || address->len != 1 || tx_bufs->buffers[1].buf == NULL) {
		return -EIO;
	}

	return bmi323_emul_write_words(target, reg, tx_bufs->buffers[1].buf,
					       tx_bufs->buffers[1].len);
}
#endif /* CONFIG_BMI323_BUS_SPI */

static int bmi323_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	bmi323_emul_reset(target);

	return 0;
}

#ifdef CONFIG_BMI323_BUS_I2C
static const struct i2c_emul_api bmi323_emul_api_i2c = {
	.transfer = bmi323_emul_transfer_i2c,
};
#endif /* CONFIG_BMI323_BUS_I2C */

#ifdef CONFIG_BMI323_BUS_SPI
static const struct spi_emul_api bmi323_emul_api_spi = {
	.io = bmi323_emul_io_spi,
};
#endif /* CONFIG_BMI323_BUS_SPI */

#define BMI323_EMUL_DEFINE(n, bus_api)                                                             \
	static struct bmi323_emul_data bmi323_emul_data_##n;                                       \
	EMUL_DT_INST_DEFINE(n, bmi323_emul_init, &bmi323_emul_data_##n, NULL, &bus_api, NULL)

#define BMI323_EMUL_BUS_API(n)                                                                     \
	COND_CODE_1(DT_INST_ON_BUS(n, spi), (bmi323_emul_api_spi), (bmi323_emul_api_i2c))

#define BMI323_EMUL(n) BMI323_EMUL_DEFINE(n, BMI323_EMUL_BUS_API(n))

DT_INST_FOREACH_STATUS_OKAY(BMI323_EMUL)
