/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_atm90e32as

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "atm90e32as.h"
#include "emul_atm90e32as.h"

LOG_MODULE_REGISTER(atm90e32as_emul, CONFIG_ENERGY_METER_LOG_LEVEL);

#define REG_COUNT 256U

struct atm90e32as_emul_data {
	uint16_t regs[REG_COUNT];
	uint32_t reset_count;
	uint32_t corrupt_reads;
	uint32_t drop_writes;
	bool absent;
};

static const struct atm90e32as_reg_val atm90e32as_emul_defaults[] = {
	{ATM90E32AS_REG_CHANNEL_MAP_I, 0x0210},
	{ATM90E32AS_REG_CHANNEL_MAP_U, 0x0654},
	{ATM90E32AS_REG_SAG_PEAK_DET, 0x143F},
	{ATM90E32AS_REG_OV_TH, 0xC000},
	{ATM90E32AS_REG_ZX_CONFIG, 0x0001},
	{ATM90E32AS_REG_SAG_TH, 0x1000},
	{ATM90E32AS_REG_PHASE_LOSS_TH, 0x0400},
	{ATM90E32AS_REG_IN_WARN_TH, 0xFFFF},
	{ATM90E32AS_REG_OI_TH, 0xC000},
	{ATM90E32AS_REG_FREQ_LO_TH, 0x1324},
	{ATM90E32AS_REG_FREQ_HI_TH, 0x13EC},
	{ATM90E32AS_REG_PM_PWR_CTRL, 0x010F},
	{ATM90E32AS_REG_F0, 5000},
	{ATM90E32AS_REG_T0, 25},
	{ATM90E32AS_REG_PL_CONST_H, 0x0861},
	{ATM90E32AS_REG_PL_CONST_L, 0xC468},
	{ATM90E32AS_REG_MMODE0, 0x0087},
	{ATM90E32AS_REG_U_GAIN_A, 0x8000},
	{ATM90E32AS_REG_I_GAIN_A, 0x8000},
	{ATM90E32AS_REG_U_GAIN_A + ATM90E32AS_MEAS_CAL_STRIDE, 0x8000},
	{ATM90E32AS_REG_I_GAIN_A + ATM90E32AS_MEAS_CAL_STRIDE, 0x8000},
	{ATM90E32AS_REG_U_GAIN_A + 2 * ATM90E32AS_MEAS_CAL_STRIDE, 0x8000},
	{ATM90E32AS_REG_I_GAIN_A + 2 * ATM90E32AS_MEAS_CAL_STRIDE, 0x8000},
};

static void atm90e32as_emul_load_defaults(struct atm90e32as_emul_data *data)
{
	/* Measurement registers are not touched by a reset of the model */
	for (size_t i = 0; i < ATM90E32AS_REG_AP_ENERGY_T; i++) {
		data->regs[i] = 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(atm90e32as_emul_defaults); i++) {
		data->regs[atm90e32as_emul_defaults[i].addr] = atm90e32as_emul_defaults[i].val;
	}
}

static bool atm90e32as_emul_is_energy(uint16_t addr)
{
	return ((addr >= ATM90E32AS_REG_AP_ENERGY_T) && (addr < ATM90E32AS_REG_SA_ENERGY_T + 4U)) ||
	       ((addr >= ATM90E32AS_REG_AP_ENERGY_TF) && (addr < ATM90E32AS_REG_AN_ENERGY_TH + 4U));
}

static uint16_t atm90e32as_emul_read(struct atm90e32as_emul_data *data, uint16_t addr)
{
	uint16_t val;

	if (data->absent) {
		return 0xFFFF;
	}

	if (addr >= REG_COUNT) {
		return 0;
	}

	val = data->regs[addr];
	data->regs[ATM90E32AS_REG_LAST_SPI_DATA] = val;

	if (atm90e32as_emul_is_energy(addr)) {
		data->regs[addr] = 0;
	}

	if (data->corrupt_reads > 0U) {
		/* A different pattern on every read, so two corrupted copies never match */
		val ^= (uint16_t)(0x5A5AU + data->corrupt_reads);
		data->corrupt_reads--;
	}

	return val;
}

static void atm90e32as_emul_write(struct atm90e32as_emul_data *data, uint16_t addr, uint16_t val)
{
	if (data->absent || (addr >= REG_COUNT)) {
		return;
	}

	if (data->drop_writes > 0U) {
		data->drop_writes--;
		return;
	}

	data->regs[ATM90E32AS_REG_LAST_SPI_DATA] = val;

	switch (addr) {
	case ATM90E32AS_REG_SOFT_RESET:
		if (val == ATM90E32AS_SOFT_RESET_CMD) {
			atm90e32as_emul_load_defaults(data);
			data->reset_count++;
		}
		return;
	case ATM90E32AS_REG_EMM_INT_STATE0:
	case ATM90E32AS_REG_EMM_INT_STATE1:
		data->regs[addr] &= ~val;
		return;
	case ATM90E32AS_REG_CFG_REG_ACC_EN:
		data->regs[addr] = val;
		return;
	default:
		break;
	}

	if (addr < ATM90E32AS_REG_SOFT_RESET) {
		/* Configuration registers are only writable with access enabled */
		if (data->regs[ATM90E32AS_REG_CFG_REG_ACC_EN] == ATM90E32AS_CFG_ACCESS_ENABLE) {
			data->regs[addr] = val;
		}
		return;
	}

	if ((addr == ATM90E32AS_REG_EMM_INT_EN0) || (addr == ATM90E32AS_REG_EMM_INT_EN1)) {
		data->regs[addr] = val;
	}
}

static int atm90e32as_emul_io(const struct emul *target, const struct spi_config *config,
			      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct atm90e32as_emul_data *data = target->data;
	const uint8_t *tx;
	uint16_t cmd;
	uint16_t addr;

	ARG_UNUSED(config);

	if ((tx_bufs == NULL) || (tx_bufs->count != 1U) || (tx_bufs->buffers[0].len != 4U)) {
		LOG_ERR("Unsupported transfer");
		return -EIO;
	}

	tx = tx_bufs->buffers[0].buf;
	cmd = sys_get_be16(&tx[0]);
	addr = cmd & ATM90E32AS_SPI_ADDR_MASK;

	if ((cmd & ATM90E32AS_SPI_READ) != 0U) {
		uint8_t *rx;

		if ((rx_bufs == NULL) || (rx_bufs->count != 1U) ||
		    (rx_bufs->buffers[0].len != 4U)) {
			LOG_ERR("Read without a receive buffer");
			return -EIO;
		}

		rx = rx_bufs->buffers[0].buf;
		rx[0] = 0;
		rx[1] = 0;
		sys_put_be16(atm90e32as_emul_read(data, addr), &rx[2]);
	} else {
		atm90e32as_emul_write(data, addr, sys_get_be16(&tx[2]));
	}

	return 0;
}

static const struct spi_emul_api atm90e32as_emul_spi_api = {
	.io = atm90e32as_emul_io,
};

void atm90e32as_emul_set_reg(const struct emul *target, uint8_t reg, uint16_t val)
{
	struct atm90e32as_emul_data *data = target->data;

	data->regs[reg] = val;
}

uint16_t atm90e32as_emul_get_reg(const struct emul *target, uint8_t reg)
{
	struct atm90e32as_emul_data *data = target->data;

	return data->regs[reg];
}

void atm90e32as_emul_reset(const struct emul *target)
{
	struct atm90e32as_emul_data *data = target->data;

	atm90e32as_emul_load_defaults(data);
}

uint32_t atm90e32as_emul_reset_count(const struct emul *target)
{
	struct atm90e32as_emul_data *data = target->data;

	return data->reset_count;
}

void atm90e32as_emul_corrupt_reads(const struct emul *target, uint32_t count)
{
	struct atm90e32as_emul_data *data = target->data;

	data->corrupt_reads = count;
}

void atm90e32as_emul_drop_writes(const struct emul *target, uint32_t count)
{
	struct atm90e32as_emul_data *data = target->data;

	data->drop_writes = count;
}

void atm90e32as_emul_set_present(const struct emul *target, bool present)
{
	struct atm90e32as_emul_data *data = target->data;

	data->absent = !present;
}

static int atm90e32as_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	atm90e32as_emul_load_defaults(target->data);

	return 0;
}

#define ATM90E32AS_EMUL(inst)                                                                      \
	static struct atm90e32as_emul_data atm90e32as_emul_data_##inst;                            \
	EMUL_DT_INST_DEFINE(inst, atm90e32as_emul_init, &atm90e32as_emul_data_##inst, NULL,        \
			    &atm90e32as_emul_spi_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(ATM90E32AS_EMUL)
