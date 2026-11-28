/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/pbuf.h>
#include <zephyr/ztest.h>

ZTEST(test_pbuf_initializers, test_cfg_init)
{
	static uint8_t memory_area[64] __aligned(32);
	const struct pbuf_cfg configs[] = {
		PBUF_CFG_INIT(memory_area, sizeof(memory_area), 0, false),
		PBUF_CFG_INIT(memory_area, sizeof(memory_area), 0, true),
		PBUF_CFG_INIT(memory_area, sizeof(memory_area), 32, false),
		PBUF_CFG_INIT(memory_area, sizeof(memory_area), 32, true),
	};
	const uint32_t write_offsets[] = {4, 8, 32, 32};
	const uint32_t alignments[] = {0, 0, 32, 32};

	for (size_t i = 0; i < ARRAY_SIZE(configs); i++) {
		const struct pbuf_cfg *cfg = &configs[i];
		volatile uint32_t *handshake = i % 2 == 0 ? NULL : (uint32_t *)(memory_area + 4);

		zassert_equal_ptr(cfg->rd_idx_loc, memory_area);
		zassert_equal_ptr(cfg->handshake_loc, handshake);
		zassert_equal_ptr(cfg->wr_idx_loc, memory_area + write_offsets[i]);
		zassert_equal(cfg->dcache_alignment, alignments[i]);
		zassert_equal(cfg->len, sizeof(memory_area) - write_offsets[i] - 4);
		zassert_equal_ptr(cfg->data_loc, memory_area + write_offsets[i] + 4);
	}
}

ZTEST_SUITE(test_pbuf_initializers, NULL, NULL, NULL, NULL, NULL);
