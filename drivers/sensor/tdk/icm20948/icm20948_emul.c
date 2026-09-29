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
#define ICM20948_EMUL_BANKS 4
#define ICM20948_EMUL_REGS  0x7F

/*
 * The magnetometer is a separate die on an auxiliary I2C bus the device
 * masters itself, so it needs a register file of its own.
 */
#define AK09916_EMUL_ADDR 0x0C
#define AK09916_EMUL_REGS 0x33

#define AK09916_EMUL_REG_WIA2  0x01
#define AK09916_EMUL_REG_ST1   0x10
#define AK09916_EMUL_REG_CNTL2 0x31
#define AK09916_EMUL_REG_CNTL3 0x32

#define AK09916_EMUL_DEVICE_ID 0x09
#define BIT_AK09916_EMUL_SRST  BIT(0)

struct icm20948_emul_data {
	uint8_t reg[ICM20948_EMUL_BANKS][ICM20948_EMUL_REGS];
	uint8_t magn[AK09916_EMUL_REGS];
	uint8_t bank;
};

static void icm20948_emul_reset(const struct emul *target)
{
	struct icm20948_emul_data *data = target->data;

	memset(data->reg, 0, sizeof(data->reg));
	data->bank = 0;

	data->reg[0][FIELD_GET(REG_ADDRESS_MASK, REG_WHO_AM_I)] = WHO_AM_I_ICM20948;
}

static void ak09916_emul_reset(const struct emul *target)
{
	struct icm20948_emul_data *data = target->data;

	memset(data->magn, 0, sizeof(data->magn));
	data->magn[AK09916_EMUL_REG_WIA2] = AK09916_EMUL_DEVICE_ID;
}

void icm20948_emul_set_magn_reg(const struct emul *target, uint8_t reg, const uint8_t *val,
				size_t len)
{
	struct icm20948_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg + len <= AK09916_EMUL_REGS);

	memcpy(&data->magn[reg], val, len);
}

void icm20948_emul_get_magn_reg(const struct emul *target, uint8_t reg, uint8_t *val, size_t len)
{
	struct icm20948_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg + len <= AK09916_EMUL_REGS);

	memcpy(val, &data->magn[reg], len);
}

/*
 * Slave 4 carries one byte at a time and reports its own completion, which
 * is what the driver uses while it brings the magnetometer up. Writing the
 * control register is what starts the transfer, so carry it out there and
 * report it as finished straight away.
 */
static void icm20948_emul_run_slv4(const struct emul *target)
{
	struct icm20948_emul_data *data = target->data;
	uint8_t addr = data->reg[3][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_SLV4_ADDR)];
	uint8_t reg = data->reg[3][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_SLV4_REG)];
	uint8_t status = BIT_I2C_SLV4_DONE;

	if ((addr & ~BIT_I2C_SLV_RNW) != AK09916_EMUL_ADDR || reg >= AK09916_EMUL_REGS) {
		/* Nothing answers at that address on the auxiliary bus */
		data->reg[0][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_MST_STATUS)] =
			BIT_I2C_SLV4_DONE | BIT_I2C_SLV4_NACK;
		return;
	}

	if ((addr & BIT_I2C_SLV_RNW) != 0) {
		data->reg[3][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_SLV4_DI)] = data->magn[reg];
	} else {
		uint8_t val = data->reg[3][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_SLV4_DO)];

		if (reg == AK09916_EMUL_REG_CNTL3 && (val & BIT_AK09916_EMUL_SRST) != 0) {
			ak09916_emul_reset(target);
		} else {
			data->magn[reg] = val;
		}
	}

	data->reg[0][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_MST_STATUS)] = status;
}

/*
 * Slave 0 is left running once armed and the master copies the registers it
 * points at into the external sensor registers on every cycle. Refresh them
 * as they are read, which is the only moment the copy can be observed.
 */
static void icm20948_emul_refresh_slv0(const struct emul *target)
{
	struct icm20948_emul_data *data = target->data;
	uint8_t ctrl = data->reg[3][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_SLV0_CTRL)];
	uint8_t addr = data->reg[3][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_SLV0_ADDR)];
	uint8_t reg = data->reg[3][FIELD_GET(REG_ADDRESS_MASK, REG_I2C_SLV0_REG)];
	uint8_t len = FIELD_GET(MASK_I2C_SLV_LENG, ctrl);
	uint8_t ext = FIELD_GET(REG_ADDRESS_MASK, REG_EXT_SLV_SENS_DATA_00);

	if ((ctrl & BIT_I2C_SLV_EN) == 0 || (addr & ~BIT_I2C_SLV_RNW) != AK09916_EMUL_ADDR) {
		return;
	}

	for (uint8_t i = 0; i < len && reg + i < AK09916_EMUL_REGS; i++) {
		data->reg[0][ext + i] = data->magn[reg + i];
	}
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

	if (data->bank == 3 && addr == FIELD_GET(REG_ADDRESS_MASK, REG_I2C_SLV4_CTRL) &&
	    (val & BIT_I2C_SLV_EN) != 0) {
		/* The enable bit clears itself once the transfer is done */
		data->reg[3][addr] &= ~BIT_I2C_SLV_EN;
		icm20948_emul_run_slv4(target);
	}
}

static void icm20948_emul_read(const struct emul *target, uint8_t addr, uint8_t *buf, size_t len)
{
	struct icm20948_emul_data *data = target->data;
	uint8_t status = FIELD_GET(REG_ADDRESS_MASK, REG_I2C_MST_STATUS);

	__ASSERT_NO_MSG(addr + len <= ICM20948_EMUL_REGS);

	if (data->bank == 0) {
		icm20948_emul_refresh_slv0(target);
	}

	memcpy(buf, &data->reg[data->bank][addr], len);

	/* The master status register clears itself as it is read */
	if (data->bank == 0 && addr <= status && status < addr + len) {
		data->reg[0][status] = 0;
	}
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
	ak09916_emul_reset(target);

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
