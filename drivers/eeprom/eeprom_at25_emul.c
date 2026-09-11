/*
 * Copyright (c) 2026 Axon Enterprise, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_at25

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(atmel_at25);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>

/* AT25 instruction set, mirrors drivers/eeprom/eeprom_at2x.c */
#define EEPROM_AT25_WRITE 0x02U
#define EEPROM_AT25_READ  0x03U
#define EEPROM_AT25_WRDI  0x04U
#define EEPROM_AT25_RDSR  0x05U
#define EEPROM_AT25_WREN  0x06U

/*
 * In 9-bit addressing mode the 9th (most significant) address bit is not
 * carried in the address bytes: it is OR'd into bit 3 of the opcode byte.
 */
#define EEPROM_AT25_OPCODE_ADDR_BIT8 BIT(3)

/** Static configuration for the emulator */
struct at25_emul_cfg {
	/** EEPROM data contents */
	uint8_t *buf;
	/** Size of EEPROM in bytes */
	uint32_t size;
};

/**
 * Emulate a SPI transfer to an AT25 chip
 *
 * This handles RDSR, WREN, WRDI and simple reads and writes, including the
 * 9-bit addressing mode where the top address bit travels in the opcode.
 *
 * @param target SPI emulation information
 * @param config Not used
 * @param tx_bufs Buffers describing the request written by the driver
 * @param rx_bufs Buffers to be filled in with the emulated response
 * @retval 0 If successful
 * @retval -EIO General input / output error
 */
static int at25_emul_io(const struct emul *target, const struct spi_config *config,
			const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	const struct at25_emul_cfg *cfg = target->cfg;
	const struct spi_buf *cmd_buf;
	const struct spi_buf *data_buf;
	const uint8_t *cmd;
	uint8_t opcode;
	uint16_t offset;
	size_t i;

	ARG_UNUSED(config);

	if (!tx_bufs || tx_bufs->count == 0) {
		return -EIO;
	}

	cmd_buf = &tx_bufs->buffers[0];
	cmd = cmd_buf->buf;
	opcode = cmd[0] & ~EEPROM_AT25_OPCODE_ADDR_BIT8;

	switch (opcode) {
	case EEPROM_AT25_WREN:
	case EEPROM_AT25_WRDI:
		return 0;

	case EEPROM_AT25_RDSR:
		if (rx_bufs && rx_bufs->count > 0 && rx_bufs->buffers[0].len >= 2) {
			/* Never busy; write-protection is not modeled */
			((uint8_t *)rx_bufs->buffers[0].buf)[1] = 0;
		}
		return 0;

	case EEPROM_AT25_READ:
	case EEPROM_AT25_WRITE:
		break;

	default:
		LOG_ERR("Unknown opcode 0x%02x", opcode);
		return -EIO;
	}

	if (cmd_buf->len < 2) {
		LOG_ERR("Command too short for read/write (%zu)", cmd_buf->len);
		return -EIO;
	}

	offset = cmd[1];
	if (cmd[0] & EEPROM_AT25_OPCODE_ADDR_BIT8) {
		offset |= BIT(8);
	}

	if (opcode == EEPROM_AT25_READ) {
		if (!rx_bufs || rx_bufs->count < 2) {
			LOG_ERR("Missing rx data buffer for read");
			return -EIO;
		}

		data_buf = &rx_bufs->buffers[1];
		for (i = 0; i < data_buf->len && (offset + i) < cfg->size; i++) {
			((uint8_t *)data_buf->buf)[i] = cfg->buf[offset + i];
		}
	} else {
		if (tx_bufs->count < 2) {
			LOG_ERR("Missing tx data buffer for write");
			return -EIO;
		}

		data_buf = &tx_bufs->buffers[1];
		for (i = 0; i < data_buf->len && (offset + i) < cfg->size; i++) {
			cfg->buf[offset + i] = ((const uint8_t *)data_buf->buf)[i];
		}
	}

	return 0;
}

/* Device instantiation */

static struct spi_emul_api bus_api = {
	.io = at25_emul_io,
};

/**
 * Set up a new AT25 emulator
 *
 * This should be called for each AT25 device that needs to be emulated. It
 * registers it with the SPI emulation controller.
 *
 * @param target Emulation information
 * @param parent Device to emulate (must use AT25 driver)
 * @return 0 indicating success (always)
 */
static int emul_atmel_at25_init(const struct emul *target, const struct device *parent)
{
	const struct at25_emul_cfg *cfg = target->cfg;

	ARG_UNUSED(parent);

	/* Start with an erased EEPROM, assuming all 0xff */
	memset(cfg->buf, 0xff, cfg->size);

	return 0;
}

#define EEPROM_AT25_EMUL(n)                                                                        \
	static uint8_t at25_emul_buf_##n[DT_INST_PROP(n, size)];                                   \
	static const struct at25_emul_cfg at25_emul_cfg_##n = {                                    \
		.buf = at25_emul_buf_##n,                                                          \
		.size = DT_INST_PROP(n, size),                                                     \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_atmel_at25_init, NULL, &at25_emul_cfg_##n, &bus_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(EEPROM_AT25_EMUL)
