/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_atm90e26

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#ifdef CONFIG_ATM90E26_BUS_SPI
#include <zephyr/drivers/spi_emul.h>
#endif
#ifdef CONFIG_ATM90E26_BUS_UART
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/uart_emul.h>
#endif

#include "atm90e26.h"
#include "emul_atm90e26.h"

LOG_MODULE_REGISTER(atm90e26_emul, CONFIG_ENERGY_METER_LOG_LEVEL);

#define REG_COUNT 0x80U

struct atm90e26_emul_data {
	uint16_t regs[REG_COUNT];
	uint32_t reset_count;
	uint32_t corrupt_reads;
	uint32_t drop_writes;
	bool absent;
	uint8_t frame[5];
	size_t frame_len;
};

struct atm90e26_emul_default {
	uint8_t addr;
	uint16_t val;
};

static const struct atm90e26_emul_default atm90e26_emul_defaults[] = {
	{ATM90E26_REG_FUNC_EN, 0x000C},
	{ATM90E26_REG_SAG_TH, 0x1D6A},
	{ATM90E26_REG_CAL_START, 0x6886},
	{0x21, 0x0015},
	{0x22, 0xD174},
	{0x27, 0x08BD},
	{0x29, 0x0AEC},
	{ATM90E26_REG_MMODE, 0x9422},
	{ATM90E26_REG_ADJ_START, 0x6886},
	{ATM90E26_REG_U_GAIN, 0x6720},
	{ATM90E26_REG_U_GAIN + 1, 0x7A13},
	{ATM90E26_REG_U_GAIN + 2, 0x7530},
};

static void atm90e26_emul_load_block_defaults(struct atm90e26_emul_data *data, uint8_t first,
					      uint8_t last)
{
	for (uint8_t addr = first; addr <= last; addr++) {
		data->regs[addr] = 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(atm90e26_emul_defaults); i++) {
		uint8_t addr = atm90e26_emul_defaults[i].addr;

		if ((addr >= first) && (addr <= last)) {
			data->regs[addr] = atm90e26_emul_defaults[i].val;
		}
	}
}

static void atm90e26_emul_load_defaults(struct atm90e26_emul_data *data)
{
	/* Measurement registers are not touched by a reset of the model */
	atm90e26_emul_load_block_defaults(data, 0, ATM90E26_REG_CS2);
}

static bool atm90e26_emul_read_clears(uint8_t addr)
{
	return (addr == ATM90E26_REG_SYS_STATUS) ||
	       ((addr >= ATM90E26_REG_AP_ENERGY) && (addr <= ATM90E26_REG_RT_ENERGY));
}

/* Returns the value the chip sends; *wire is what reaches the host */
static uint16_t atm90e26_emul_read(struct atm90e26_emul_data *data, uint8_t addr, uint16_t *wire)
{
	uint16_t val;

	if (data->absent) {
		*wire = 0xFFFF;
		return 0xFFFF;
	}

	val = (addr < REG_COUNT) ? data->regs[addr] : 0U;
	data->regs[ATM90E26_REG_LAST_DATA] = val;

	if (atm90e26_emul_read_clears(addr)) {
		data->regs[addr] = 0;
	}

	*wire = val;
	if (data->corrupt_reads > 0U) {
		/* A different pattern on every read, so two corrupted copies never match */
		*wire ^= (uint16_t)(0x5A5AU + data->corrupt_reads);
		data->corrupt_reads--;
	}

	return val;
}

static void atm90e26_emul_check_block(struct atm90e26_emul_data *data, uint8_t first, size_t len,
				      uint8_t cs_reg, uint16_t err_mask)
{
	if (atm90e26_checksum(&data->regs[first], len) == data->regs[cs_reg]) {
		data->regs[ATM90E26_REG_SYS_STATUS] &= ~err_mask;
	} else {
		data->regs[ATM90E26_REG_SYS_STATUS] |= err_mask;
	}
}

static void atm90e26_emul_start_cmd(struct atm90e26_emul_data *data, uint8_t addr, uint16_t val)
{
	bool meter = (addr == ATM90E26_REG_CAL_START);
	uint8_t first = meter ? ATM90E26_REG_PL_CONST_H : ATM90E26_REG_U_GAIN;
	size_t len = meter ? ATM90E26_METER_CAL_LEN : ATM90E26_MEAS_CAL_LEN;
	uint8_t cs_reg = meter ? ATM90E26_REG_CS1 : ATM90E26_REG_CS2;
	uint16_t err = meter ? ATM90E26_SYS_STATUS_CAL_ERR : ATM90E26_SYS_STATUS_ADJ_ERR;

	data->regs[addr] = val;

	if (val == ATM90E26_CAL_START_CMD) {
		atm90e26_emul_load_block_defaults(data, first, cs_reg);
	} else if (val == ATM90E26_CAL_CHECK_CMD) {
		atm90e26_emul_check_block(data, first, len, cs_reg, err);
	}
}

static void atm90e26_emul_write(struct atm90e26_emul_data *data, uint8_t addr, uint16_t val)
{
	if (data->absent || (addr >= REG_COUNT)) {
		return;
	}

	if (data->drop_writes > 0U) {
		data->drop_writes--;
		return;
	}

	data->regs[ATM90E26_REG_LAST_DATA] = val;

	switch (addr) {
	case ATM90E26_REG_SOFT_RESET:
		if (val == ATM90E26_SOFT_RESET_CMD) {
			atm90e26_emul_load_defaults(data);
			data->reset_count++;
		}
		return;
	case ATM90E26_REG_FUNC_EN:
	case ATM90E26_REG_SAG_TH:
		data->regs[addr] = val;
		return;
	case ATM90E26_REG_CAL_START:
	case ATM90E26_REG_ADJ_START:
		atm90e26_emul_start_cmd(data, addr, val);
		return;
	default:
		break;
	}

	/* Calibration registers are only writable after the start command */
	if ((addr > ATM90E26_REG_CAL_START) && (addr <= ATM90E26_REG_CS1) &&
	    (data->regs[ATM90E26_REG_CAL_START] == ATM90E26_CAL_START_CMD)) {
		data->regs[addr] = val;
	} else if ((addr > ATM90E26_REG_ADJ_START) && (addr <= ATM90E26_REG_CS2) &&
		   (data->regs[ATM90E26_REG_ADJ_START] == ATM90E26_CAL_START_CMD)) {
		data->regs[addr] = val;
	}
}

#ifdef CONFIG_ATM90E26_BUS_SPI
static int atm90e26_emul_spi_io(const struct emul *target, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct atm90e26_emul_data *data = target->data;
	const uint8_t *tx;
	uint8_t addr;

	ARG_UNUSED(config);

	if ((tx_bufs == NULL) || (tx_bufs->count != 1U) || (tx_bufs->buffers[0].len != 3U)) {
		LOG_ERR("Unsupported transfer");
		return -EIO;
	}

	tx = tx_bufs->buffers[0].buf;
	addr = tx[0] & ATM90E26_ADDR_MASK;

	if ((tx[0] & ATM90E26_READ) != 0U) {
		uint8_t *rx;
		uint16_t wire;

		if ((rx_bufs == NULL) || (rx_bufs->count != 1U) ||
		    (rx_bufs->buffers[0].len != 3U)) {
			LOG_ERR("Read without a receive buffer");
			return -EIO;
		}

		rx = rx_bufs->buffers[0].buf;
		(void)atm90e26_emul_read(data, addr, &wire);
		rx[0] = 0;
		sys_put_be16(wire, &rx[1]);
	} else {
		atm90e26_emul_write(data, addr, sys_get_be16(&tx[1]));
	}

	return 0;
}

static const struct spi_emul_api atm90e26_emul_spi_api = {
	.io = atm90e26_emul_spi_io,
};
#endif /* CONFIG_ATM90E26_BUS_SPI */

#ifdef CONFIG_ATM90E26_BUS_UART
static void atm90e26_emul_uart_frame(const struct device *uart, struct atm90e26_emul_data *data)
{
	const uint8_t *f = data->frame;
	uint8_t addr = f[1] & ATM90E26_ADDR_MASK;

	if ((f[1] & ATM90E26_READ) != 0U) {
		uint16_t wire;
		uint16_t val;
		uint8_t answer[3];

		/* A frame with a bad checksum is dropped without an answer */
		if (f[2] != f[1]) {
			return;
		}

		val = atm90e26_emul_read(data, addr, &wire);
		if (data->absent) {
			return;
		}

		answer[0] = (uint8_t)(wire >> 8);
		answer[1] = (uint8_t)wire;
		answer[2] = (uint8_t)((val >> 8) + val);
		(void)uart_emul_put_rx_data(uart, answer, sizeof(answer));
	} else {
		uint8_t checksum = (uint8_t)(f[1] + f[2] + f[3]);

		/* Bad frames, dropped writes and a missing chip get no answer */
		if ((f[4] != checksum) || data->absent) {
			return;
		}

		if (data->drop_writes > 0U) {
			data->drop_writes--;
			return;
		}

		atm90e26_emul_write(data, addr, ((uint16_t)f[2] << 8) | f[3]);
		(void)uart_emul_put_rx_data(uart, &checksum, 1);
	}
}

static void atm90e26_emul_uart_tx_ready(const struct device *uart, size_t size,
					const struct emul *target)
{
	struct atm90e26_emul_data *data = target->data;
	uint8_t byte;

	ARG_UNUSED(size);

	while (uart_emul_get_tx_data(uart, &byte, 1) == 1U) {
		if ((data->frame_len == 0U) && (byte != ATM90E26_UART_START)) {
			continue;
		}

		data->frame[data->frame_len++] = byte;

		if ((data->frame_len == 3U) && ((data->frame[1] & ATM90E26_READ) != 0U)) {
			atm90e26_emul_uart_frame(uart, data);
			data->frame_len = 0;
		} else if (data->frame_len == 5U) {
			atm90e26_emul_uart_frame(uart, data);
			data->frame_len = 0;
		}
	}
}

static const struct uart_emul_device_api atm90e26_emul_uart_api = {
	.tx_data_ready = atm90e26_emul_uart_tx_ready,
};
#endif /* CONFIG_ATM90E26_BUS_UART */

void atm90e26_emul_set_reg(const struct emul *target, uint8_t reg, uint16_t val)
{
	struct atm90e26_emul_data *data = target->data;

	data->regs[reg] = val;
}

uint16_t atm90e26_emul_get_reg(const struct emul *target, uint8_t reg)
{
	struct atm90e26_emul_data *data = target->data;

	return data->regs[reg];
}

uint32_t atm90e26_emul_reset_count(const struct emul *target)
{
	struct atm90e26_emul_data *data = target->data;

	return data->reset_count;
}

void atm90e26_emul_corrupt_reads(const struct emul *target, uint32_t count)
{
	struct atm90e26_emul_data *data = target->data;

	data->corrupt_reads = count;
}

void atm90e26_emul_drop_writes(const struct emul *target, uint32_t count)
{
	struct atm90e26_emul_data *data = target->data;

	data->drop_writes = count;
}

void atm90e26_emul_set_present(const struct emul *target, bool present)
{
	struct atm90e26_emul_data *data = target->data;

	data->absent = !present;
}

static int atm90e26_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	atm90e26_emul_load_defaults(target->data);

	return 0;
}

#define ATM90E26_EMUL_BUS_API(inst)                                                                \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), (&atm90e26_emul_spi_api),                           \
		    (&atm90e26_emul_uart_api))

#define ATM90E26_EMUL(inst)                                                                        \
	static struct atm90e26_emul_data atm90e26_emul_data_##inst;                                \
	EMUL_DT_INST_DEFINE(inst, atm90e26_emul_init, &atm90e26_emul_data_##inst, NULL,            \
			    ATM90E26_EMUL_BUS_API(inst), NULL)

DT_INST_FOREACH_STATUS_OKAY(ATM90E26_EMUL)
