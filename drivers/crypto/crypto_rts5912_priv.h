/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Realtek Semiconductor Corporation, SIBG-SD7
 *
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_RTS5912_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_RTS5912_PRIV_H_

struct rts5912_sha256_context {
	uint32_t total[2];
	uint32_t state[8];
	uint8_t buffer[64];
	uint8_t sha2_data_in_sram[1024];
	struct k_mutex crypto_rts5912_in_use;
	bool in_use;
	bool is224;
};

struct rts5912_sha_config {
	volatile struct sha2_type *cfg_sha2_regs;
	volatile struct sha2dma_type *cfg_sha2dma_regs;
};

const uint32_t rts5912_sha224_digest[] = {0xC1059ED8, 0x367CD507, 0x3070DD17, 0xF70E5939,
					  0xFFC00B31, 0x68581511, 0x64F98FA7, 0xBEFA4FA4};

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_RTS5912_PRIV_H_ */
