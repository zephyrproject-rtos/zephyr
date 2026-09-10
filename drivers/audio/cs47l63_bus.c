/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_bus.c
 * @brief CS47L63 32-bit register access over SPI
 */

#include "cs47l63_bus.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "cs47l63_priv.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(cs47l63);

/** Marks the address word of a read transaction. */
#define CS47L63_READ_FLAG 0x80000000U

/** Address, padding and data are each one 32-bit word on the wire. */
#define CS47L63_WORD_BYTES 4

/**
 * @brief Wire layout of every register transaction: address, padding, data.
 *
 * The padding word is not optional and it is not read-only: the vendor BSP
 * declares `spi_pad_len = 4` (modules/hal/cirrus-logic/cs47l63/bsp/bsp_cs47l63.c)
 * and its transport clocks that many zero bytes between the address and the
 * data on writes exactly as it does on reads (common/platform_bsp/eestm32int/
 * platform_bsp.c, bsp_spi_write()). A write that omits it is not a corrupt
 * write - the part takes the data word for the padding, sees the frame end
 * before any data arrives and discards the whole transaction, silently and
 * with the SPI transfer itself reporting success.
 */
#define CS47L63_FRAME_BYTES (3 * CS47L63_WORD_BYTES)

/** Byte offsets of the three words in a transaction. */
#define CS47L63_ADDR_OFFS 0
#define CS47L63_PAD_OFFS  CS47L63_WORD_BYTES
#define CS47L63_DATA_OFFS (2 * CS47L63_WORD_BYTES)

bool cs47l63_bus_is_ready(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;

	return spi_is_ready_dt(&cfg->bus);
}

int cs47l63_bus_write_reg(const struct device *dev, uint32_t addr, uint32_t val)
{
	const struct cs47l63_config *cfg = dev->config;
	uint8_t tx[CS47L63_FRAME_BYTES] = {0};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	int ret;

	sys_put_be32(addr, &tx[CS47L63_ADDR_OFFS]);
	/* tx[CS47L63_PAD_OFFS] stays zero - the padding word. */
	sys_put_be32(val, &tx[CS47L63_DATA_OFFS]);

	ret = spi_write_dt(&cfg->bus, &tx_set);
	if (ret < 0) {
		LOG_ERR("Failed to write reg 0x%05x: %d", addr, ret);
	}

	return ret;
}

int cs47l63_bus_read_reg(const struct device *dev, uint32_t addr, uint32_t *val)
{
	const struct cs47l63_config *cfg = dev->config;
	uint8_t tx[2 * CS47L63_WORD_BYTES] = {0};
	uint8_t rx[CS47L63_WORD_BYTES];
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	/* Address then padding on the way out; the first buffer has no
	 * destination, so both are clocked out and dropped rather than landing
	 * in front of the data.
	 */
	const struct spi_buf rx_bufs[] = {
		{.buf = NULL, .len = sizeof(tx)},
		{.buf = rx, .len = sizeof(rx)},
	};
	const struct spi_buf_set rx_set = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};
	int ret;

	if (val == NULL) {
		return -EINVAL;
	}

	/* The read marker is OR'd in, so an address that already has its top
	 * bit set is not corrupted - it is simply not an addressable register.
	 */
	sys_put_be32(addr | CS47L63_READ_FLAG, &tx[CS47L63_ADDR_OFFS]);

	ret = spi_transceive_dt(&cfg->bus, &tx_set, &rx_set);
	if (ret < 0) {
		LOG_ERR("Failed to read reg 0x%05x: %d", addr, ret);
		return ret;
	}

	*val = sys_get_be32(rx);

	return 0;
}

int cs47l63_bus_update_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t val)
{
	uint32_t cur;
	uint32_t next;
	int ret;

	ret = cs47l63_bus_read_reg(dev, addr, &cur);
	if (ret < 0) {
		return ret;
	}

	next = (cur & ~mask) | (val & mask);
	if (next == cur) {
		return 0;
	}

	return cs47l63_bus_write_reg(dev, addr, next);
}

int cs47l63_bus_poll_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t expected,
			 uint32_t interval_ms, uint32_t max_polls)
{
	uint32_t val;
	int ret;

	for (uint32_t i = 0; i < max_polls; i++) {
		ret = cs47l63_bus_read_reg(dev, addr, &val);
		if (ret < 0) {
			return ret;
		}

		if ((val & mask) == expected) {
			return 0;
		}

		k_msleep(interval_ms);
	}

	LOG_ERR("Reg 0x%05x never reached 0x%08x under mask 0x%08x", addr, expected, mask);

	return -ETIMEDOUT;
}
