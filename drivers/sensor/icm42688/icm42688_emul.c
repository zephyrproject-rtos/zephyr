/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm42688

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/logging/log.h>

#include <icm42688_reg.h>

LOG_MODULE_DECLARE(ICM42688, CONFIG_SENSOR_LOG_LEVEL);

#define NUM_REGS (UINT8_MAX >> 1)

struct icm42688_emul_data {
	uint8_t reg[NUM_REGS];
};

struct icm42688_emul_cfg {
};

void icm42688_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val,
			   size_t count)
{
	struct icm42688_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count < NUM_REGS);
	memcpy(data->reg + reg_addr, val, count);
}

void icm42688_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val, size_t count)
{
	struct icm42688_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count < NUM_REGS);
	memcpy(val, data->reg + reg_addr, count);
}

static int icm42688_emul_io_spi(const struct emul *target, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct icm42688_emul_data *data = target->data;
	const struct spi_buf *tx, *rx;
	uint8_t regn;
	bool is_read;

	ARG_UNUSED(config);
	__ASSERT_NO_MSG(tx_bufs != NULL);

	tx = tx_bufs->buffers;
	__ASSERT_NO_MSG(tx != NULL);
	__ASSERT_NO_MSG(tx->len > 0);

	regn = *(uint8_t *)tx->buf;
	is_read = FIELD_GET(REG_SPI_READ_BIT, regn);
	regn &= GENMASK(6, 0);
	if (is_read) {
		__ASSERT_NO_MSG(rx_bufs != NULL);
		__ASSERT_NO_MSG(rx_bufs->count > 1);

		rx = &rx_bufs->buffers[1];
		__ASSERT_NO_MSG(rx->buf != NULL);
		__ASSERT_NO_MSG(rx->len > 0);
		for (uint16_t i = 0; i < rx->len; ++i) {
			((uint8_t *)rx->buf)[i] = data->reg[regn + i];
		}
	}

	return 0;
}

static int icm42688_emul_init(const struct emul *target, const struct device *parent)
{
	return 0;
}

static const struct spi_emul_api icm42688_emul_spi_api = {
	.io = icm42688_emul_io_spi,
};

#define ICM42688_EMUL_DEFINE(n, api)                                                               \
	EMUL_DT_INST_DEFINE(n, icm42688_emul_init, &icm42688_emul_data_##n,                        \
			    &icm42688_emul_cfg_##n, &api, NULL)

#define ICM42688_EMUL_SPI(n)                                                                       \
	static struct icm42688_emul_data icm42688_emul_data_##n;                                   \
	static const struct icm42688_emul_cfg icm42688_emul_cfg_##n;                               \
	ICM42688_EMUL_DEFINE(n, icm42688_emul_spi_api)

DT_INST_FOREACH_STATUS_OKAY(ICM42688_EMUL_SPI)
