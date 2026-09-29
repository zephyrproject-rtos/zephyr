/*
 * Copyright (c) 2026 Ali Talha Yurtseven
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm20948

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "icm20948_emul.h"
#include "icm20948_reg.h"

LOG_MODULE_DECLARE(ICM20948, CONFIG_SENSOR_LOG_LEVEL);

/* The bank select register is reachable from every bank and so lives apart */
#define ICM20948_EMUL_BANKS    4
#define ICM20948_EMUL_REGS     0x7F
#define ICM20948_EMUL_RESET_MS 1

struct icm20948_emul_data {
	uint8_t reg[ICM20948_EMUL_BANKS][ICM20948_EMUL_REGS];
	uint8_t bank;
};

static void icm20948_emul_reset(const struct emul *target)
{
	struct icm20948_emul_data *data = target->data;

	memset(data->reg, 0, sizeof(data->reg));
	data->bank = 0;

	data->reg[0][FIELD_GET(REG_ADDRESS_MASK, REG_WHO_AM_I)] = WHO_AM_I_ICM20948;
}

void icm20948_emul_set_reg(const struct emul *target, uint16_t reg, const uint8_t *val, size_t len)
{
	struct icm20948_emul_data *data = target->data;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	uint8_t addr = FIELD_GET(REG_ADDRESS_MASK, reg);

	__ASSERT_NO_MSG(bank < ICM20948_EMUL_BANKS);
	__ASSERT_NO_MSG(addr + len <= ICM20948_EMUL_REGS);

	memcpy(&data->reg[bank][addr], val, len);
}

void icm20948_emul_get_reg(const struct emul *target, uint16_t reg, uint8_t *val, size_t len)
{
	struct icm20948_emul_data *data = target->data;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	uint8_t addr = FIELD_GET(REG_ADDRESS_MASK, reg);

	__ASSERT_NO_MSG(bank < ICM20948_EMUL_BANKS);
	__ASSERT_NO_MSG(addr + len <= ICM20948_EMUL_REGS);

	memcpy(val, &data->reg[bank][addr], len);
}

static void icm20948_emul_write(const struct emul *target, uint8_t addr, uint8_t val)
{
	struct icm20948_emul_data *data = target->data;

	if (addr == REG_BANK_SEL) {
		data->bank = FIELD_GET(MASK_USER_BANK, val);
		return;
	}

	__ASSERT_NO_MSG(addr < ICM20948_EMUL_REGS);

	/*
	 * A device reset restores every register, the selected bank among
	 * them, and is the one write whose effect outlives itself.
	 */
	if (data->bank == 0 && addr == FIELD_GET(REG_ADDRESS_MASK, REG_PWR_MGMT_1) &&
	    (val & BIT_DEVICE_RESET) != 0) {
		icm20948_emul_reset(target);
		return;
	}

	data->reg[data->bank][addr] = val;
}

static void icm20948_emul_read(const struct emul *target, uint8_t addr, uint8_t *buf, size_t len)
{
	struct icm20948_emul_data *data = target->data;

	__ASSERT_NO_MSG(addr + len <= ICM20948_EMUL_REGS);

	memcpy(buf, &data->reg[data->bank][addr], len);
}

/*
 * The device frames a transfer as one address byte, whose most significant
 * bit selects a read, followed by the data. A read leaves the first received
 * byte undefined, which the driver skips with a null receive buffer.
 */
static int icm20948_emul_io_spi(const struct emul *target, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	const struct spi_buf *tx;
	uint8_t addr;
	bool read;

	ARG_UNUSED(config);
	__ASSERT_NO_MSG(tx_bufs != NULL);

	tx = tx_bufs->buffers;
	__ASSERT_NO_MSG(tx != NULL && tx->len > 0);

	addr = ((uint8_t *)tx->buf)[0];
	read = (addr & REG_SPI_READ_BIT) != 0;
	addr &= ~REG_SPI_READ_BIT;

	if (read) {
		const struct spi_buf *rx;

		__ASSERT_NO_MSG(rx_bufs != NULL && rx_bufs->count > 1);
		rx = &rx_bufs->buffers[1];
		__ASSERT_NO_MSG(rx->buf != NULL && rx->len > 0);

		icm20948_emul_read(target, addr, rx->buf, rx->len);
	} else {
		__ASSERT_NO_MSG(tx->len > 1);
		icm20948_emul_write(target, addr, ((uint8_t *)tx->buf)[1]);
	}

	return 0;
}

static int icm20948_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	icm20948_emul_reset(target);

	return 0;
}

static const struct spi_emul_api icm20948_emul_spi_api = {
	.io = icm20948_emul_io_spi,
};

#define ICM20948_EMUL_DEFINE(inst)                                                                 \
	static struct icm20948_emul_data icm20948_emul_data_##inst;                                \
                                                                                                   \
	EMUL_DT_INST_DEFINE(inst, icm20948_emul_init, &icm20948_emul_data_##inst, NULL,            \
			    &icm20948_emul_spi_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(ICM20948_EMUL_DEFINE)
