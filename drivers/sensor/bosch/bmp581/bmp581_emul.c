/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmp581

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "bmp581.h"
#include "bmp581_emul.h"

LOG_MODULE_DECLARE(bmp581, CONFIG_SENSOR_LOG_LEVEL);

/* Size of the emulated register file (highest register is CMD at 0x7E). */
#define BMP581_EMUL_NUM_REGS 0x80

struct bmp581_emul_data {
	uint8_t reg[BMP581_EMUL_NUM_REGS];
};

static void bmp581_emul_reset(const struct emul *target)
{
	struct bmp581_emul_data *data = target->data;

	memset(data->reg, 0, sizeof(data->reg));
	data->reg[BMP5_REG_CHIP_ID] = BMP5_CHIP_ID_PRIM;
	/* NVM ready, no NVM error. */
	data->reg[BMP5_REG_STATUS] = BMP5_INT_NVM_RDY;
}

static void bmp581_emul_set_reg24(struct bmp581_emul_data *data, uint8_t reg, uint32_t val)
{
	data->reg[reg] = (uint8_t)(val & 0xFF);
	data->reg[reg + 1] = (uint8_t)((val >> 8) & 0xFF);
	data->reg[reg + 2] = (uint8_t)((val >> 16) & 0xFF);
}

void bmp581_emul_set_temp_raw(const struct emul *target, int32_t raw_temp)
{
	bmp581_emul_set_reg24(target->data, BMP5_REG_TEMP_DATA_XLSB, (uint32_t)raw_temp);
}

void bmp581_emul_set_press_raw(const struct emul *target, uint32_t raw_press)
{
	bmp581_emul_set_reg24(target->data, BMP5_REG_PRESS_DATA_XLSB, raw_press);
}

static int bmp581_emul_handle_write(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct bmp581_emul_data *data = target->data;

	if (reg >= BMP581_EMUL_NUM_REGS) {
		return -EINVAL;
	}

	switch (reg) {
	case BMP5_REG_CMD:
		if (val == BMP5_SOFT_RESET_CMD) {
			bmp581_emul_reset(target);
			/* Signal soft-reset / power-on completion. */
			data->reg[BMP5_REG_INT_STATUS] = BMP5_INT_ASSERTED_POR_SOFTRESET_COMPLETE;
		}
		break;
	default:
		data->reg[reg] = val;
		break;
	}

	return 0;
}

static int bmp581_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	struct bmp581_emul_data *data = target->data;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (num_msgs < 1) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}
	if ((msgs[0].flags & I2C_MSG_READ) || msgs[0].len < 1) {
		LOG_ERR("Unexpected first message");
		return -EIO;
	}

	/*
	 * msgs[0] is always a write carrying the register address (the BMP581 bus
	 * may also split the written data into a separate follow-up write message).
	 * Subsequent messages are either the write payload or a read.
	 */
	uint8_t reg = msgs[0].buf[0];
	uint8_t write_off = 0;
	int ret;

	for (uint32_t i = 1; i < msgs[0].len; i++) {
		ret = bmp581_emul_handle_write(target, reg + write_off++, msgs[0].buf[i]);
		if (ret < 0) {
			return ret;
		}
	}

	for (int m = 1; m < num_msgs; m++) {
		struct i2c_msg *msg = &msgs[m];

		if (msg->flags & I2C_MSG_READ) {
			for (uint32_t i = 0; i < msg->len; i++) {
				uint8_t a = reg + i;

				msg->buf[i] = (a < BMP581_EMUL_NUM_REGS) ? data->reg[a] : 0;
			}
		} else {
			for (uint32_t i = 0; i < msg->len; i++) {
				ret = bmp581_emul_handle_write(target, reg + write_off++,
							       msg->buf[i]);
				if (ret < 0) {
					return ret;
				}
			}
		}
	}

	return 0;
}

static int bmp581_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	bmp581_emul_reset(target);

	return 0;
}

static int bmp581_emul_io_spi(const struct emul *target, const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct bmp581_emul_data *data = target->data;
	const struct spi_buf *tx, *txd, *rxd;
	uint8_t regn;
	int count;

	ARG_UNUSED(config);
	__ASSERT_NO_MSG(tx_bufs || rx_bufs);
	__ASSERT_NO_MSG(!tx_bufs || !rx_bufs || tx_bufs->count == rx_bufs->count);

	count = tx_bufs ? tx_bufs->count : rx_bufs->count;
	if (count != 2) {
		LOG_ERR("Unexpected SPI buffer count %d", count);
		return -EIO;
	}

	/* buffers[0] carries the 1-byte register address, buffers[1] the data. */
	tx = tx_bufs ? tx_bufs->buffers : NULL;
	txd = tx_bufs ? &tx_bufs->buffers[1] : NULL;
	rxd = rx_bufs ? &rx_bufs->buffers[1] : NULL;

	if (tx == NULL || tx->len != 1) {
		LOG_ERR("Expected a 1-byte register address");
		return -EIO;
	}

	regn = *(uint8_t *)tx->buf;

	if (regn & BMP5_SPI_RD_MASK) {
		/* Read transaction. */
		if (rxd == NULL) {
			return -EPERM;
		}
		regn &= ~BMP5_SPI_RD_MASK;
		for (uint32_t i = 0; i < rxd->len; i++) {
			uint8_t a = regn + i;

			((uint8_t *)rxd->buf)[i] = (a < BMP581_EMUL_NUM_REGS) ? data->reg[a] : 0;
		}
	} else {
		/* Write transaction. */
		if (txd == NULL) {
			return -EIO;
		}
		for (uint32_t i = 0; i < txd->len; i++) {
			int ret = bmp581_emul_handle_write(target, regn + i,
							   ((uint8_t *)txd->buf)[i]);

			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

static const struct i2c_emul_api bmp581_emul_api_i2c = {
	.transfer = bmp581_emul_transfer_i2c,
};

static struct spi_emul_api bmp581_emul_api_spi = {
	.io = bmp581_emul_io_spi,
};

#define BMP581_EMUL_DEFINE(n, bus_api)                                                             \
	static struct bmp581_emul_data bmp581_emul_data_##n;                                       \
	EMUL_DT_INST_DEFINE(n, bmp581_emul_init, &bmp581_emul_data_##n, NULL, &bus_api, NULL)

#define BMP581_EMUL_BUS_API(n)                                                                     \
	COND_CODE_1(DT_INST_ON_BUS(n, spi), (bmp581_emul_api_spi), (bmp581_emul_api_i2c))

/*
 * The I2C and SPI transports are emulated. I3C instances also report as being on
 * the I2C bus (I3C is backward compatible), so they are excluded explicitly.
 */
#define BMP581_EMUL(n)                                                                             \
	COND_CODE_1(DT_INST_ON_BUS(n, i3c), (), (BMP581_EMUL_DEFINE(n, BMP581_EMUL_BUS_API(n))))

DT_INST_FOREACH_STATUS_OKAY(BMP581_EMUL)
