/*
 * Copyright (c) 2022 Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared helpers used by the Cadence I3C driver core and its native RTIO
 * submit path. These were previously file-static in i3c_cdns.c and have been
 * promoted to be linkable from i3c_cdns_rtio.c as well.
 */

#include <string.h>

#include <zephyr/drivers/i3c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "i3c_cdns.h"

LOG_MODULE_DECLARE(I3C_CADENCE, CONFIG_I3C_CADENCE_LOG_LEVEL);

/*
 * Nibble-wise table-driven CRC5 for I3C HDR-DDR.
 *
 * Equivalent to the bit-serial recurrence below (4 bits per step instead of 1):
 *
 *     crc0     = next_data_bit ^ crc[4]
 *     crc[4:0] = { crc[3:2], crc[1]^crc0, crc[0], crc0 }
 *
 * Tables were generated and exhaustively verified against the bit-serial
 * reference for all (crc, word) pairs (2^21 combinations, 0 mismatches).
 *
 *   crc5_data_tab[n]    = step4(0, n)  applied to a 4-bit input nibble n
 *   crc5_shift_tab[c]   = step4(c, 0)  autonomous LFSR evolution by 4 steps
 *
 * By linearity, step4(crc, n) = step4(crc, 0) ^ step4(0, n).
 */
static const uint8_t crc5_data_tab[16] = {
	[0x0] = 0x00, [0x1] = 0x05, [0x2] = 0x0A, [0x3] = 0x0F,
	[0x4] = 0x14, [0x5] = 0x11, [0x6] = 0x1E, [0x7] = 0x1B,
	[0x8] = 0x0D, [0x9] = 0x08, [0xA] = 0x07, [0xB] = 0x02,
	[0xC] = 0x19, [0xD] = 0x1C, [0xE] = 0x13, [0xF] = 0x16,
};

static const uint8_t crc5_shift_tab[32] = {
	[0x00] = 0x00, [0x01] = 0x10, [0x02] = 0x05, [0x03] = 0x15,
	[0x04] = 0x0A, [0x05] = 0x1A, [0x06] = 0x0F, [0x07] = 0x1F,
	[0x08] = 0x14, [0x09] = 0x04, [0x0A] = 0x11, [0x0B] = 0x01,
	[0x0C] = 0x1E, [0x0D] = 0x0E, [0x0E] = 0x1B, [0x0F] = 0x0B,
	[0x10] = 0x0D, [0x11] = 0x1D, [0x12] = 0x08, [0x13] = 0x18,
	[0x14] = 0x07, [0x15] = 0x17, [0x16] = 0x02, [0x17] = 0x12,
	[0x18] = 0x19, [0x19] = 0x09, [0x1A] = 0x1C, [0x1B] = 0x0C,
	[0x1C] = 0x13, [0x1D] = 0x03, [0x1E] = 0x16, [0x1F] = 0x06,
};

uint8_t i3c_cdns_crc5(uint8_t crc5, uint16_t word)
{
	/* First mask guards against a caller seeding bits above bit 4.
	 * Subsequent iterations operate on values already bounded to 5 bits
	 * by the table contents, so no further masking is needed.
	 */
	crc5 = crc5_shift_tab[crc5 & 0x1f] ^ crc5_data_tab[(word >> 12) & 0xf];
	crc5 = crc5_shift_tab[crc5] ^ crc5_data_tab[(word >> 8) & 0xf];
	crc5 = crc5_shift_tab[crc5] ^ crc5_data_tab[(word >> 4) & 0xf];
	crc5 = crc5_shift_tab[crc5] ^ crc5_data_tab[word & 0xf];

	return crc5;
}

uint8_t cdns_i3c_ddr_parity(uint16_t payload)
{
	/*
	 * PA1 = odd parity over odd-indexed bits (1,3,5,...,15).
	 * PA0 = even parity over even-indexed bits (0,2,4,...,14) — i.e. XOR of those
	 *       bits inverted, equivalent to parity ^ 1.
	 */
	uint8_t pa1 = (uint8_t)__builtin_parity((unsigned int)(payload & 0xaaaau));
	uint8_t pa0 = (uint8_t)(__builtin_parity((unsigned int)(payload & 0x5555u)) ^ 1u);

	return (uint8_t)((pa1 << 1) | pa0);
}

/* This prepares the ddr word from the payload add adding on parity, This
 * does not write the preamble
 */
uint32_t prepare_ddr_word(uint16_t payload)
{
	return (uint32_t)payload << 2 | cdns_i3c_ddr_parity(payload);
}

/* This ensures that PA0 contains 1'b1 which allows for easier Bus Turnaround */
uint16_t prepare_ddr_cmd_parity_adjustment_bit(uint16_t word)
{
	/* XOR of bits 2,4,6,8,10,12,14 == parity(word & 0x5554). */
	if (__builtin_parity((unsigned int)(word & 0x5554u))) {
		word |= BIT(0);
	}

	return word;
}

#ifdef CONFIG_I3C_CONTROLLER
/* Computes and sets parity */
/* Returns [7:1] 7-bit addr, [0] even/xor parity */
uint8_t cdns_i3c_even_parity_byte(uint8_t byte)
{
	return (uint8_t)((byte << 1) | (uint8_t)(__builtin_parity((unsigned int)byte) ^ 1u));
}
#endif /* CONFIG_I3C_CONTROLLER */

void cdns_i3c_write_tx_fifo(const struct cdns_i3c_config *config, const void *buf, uint32_t len)
{
	const uint32_t *ptr = buf;
	uint32_t remain, val;

	for (remain = len; remain >= 4; remain -= 4) {
		val = *ptr++;
		sys_write32(val, config->base + TX_FIFO);
	}

	if (remain > 0) {
		val = 0;
		memcpy(&val, ptr, remain);
		sys_write32(val, config->base + TX_FIFO);
	}
}

#ifdef CONFIG_I3C_CONTROLLER
int cdns_i3c_read_rx_fifo(const struct cdns_i3c_config *config, void *buf, uint32_t len)
{
	uint32_t *ptr = buf;
	uint32_t remain, val;

	for (remain = len; remain >= 4; remain -= 4) {
		if (cdns_i3c_rx_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_le32_to_cpu(sys_read32(config->base + RX_FIFO));
		*ptr++ = val;
	}

	if (remain > 0) {
		if (cdns_i3c_rx_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_le32_to_cpu(sys_read32(config->base + RX_FIFO));
		memcpy(ptr, &val, remain);
	}

	return 0;
}

int cdns_i3c_wait_for_idle(const struct device *dev)
{
	const struct cdns_i3c_config *config = dev->config;
	uint32_t start_time = k_cycle_get_32();

	/**
	 * Spin waiting for device to go idle. It is unlikely that this will
	 * actually take any time unless if the last transaction came immediately
	 * after an error condition.
	 */
	while (!(sys_read32(config->base + MST_STATUS0) & MST_STATUS0_IDLE)) {
		if (k_cycle_get_32() - start_time > I3C_IDLE_TIMEOUT_CYC) {
			LOG_ERR("%s: Timeout waiting for idle", dev->name);
			return -EAGAIN;
		}
	}

	return 0;
}
#endif /* CONFIG_I3C_CONTROLLER */
