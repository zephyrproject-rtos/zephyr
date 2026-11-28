/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "dma_ti_am62l_bcdma.h"

ZTEST(bcdma_desc, test_split_small)
{
	struct bcdma_piece p[2];

	zassert_equal(bcdma_split_block(1, p), 1);
	zassert_equal(p[0].icnt0, 1);
	zassert_equal(p[0].icnt1, 1);
	zassert_equal(p[0].offset, 0);

	zassert_equal(bcdma_split_block(0xFFFF, p), 1);
	zassert_equal(p[0].icnt0, 0xFFFF);
	zassert_equal(p[0].icnt1, 1);
}

ZTEST(bcdma_desc, test_split_rows)
{
	struct bcdma_piece p[2];

	/* 64 KiB: two rows of 32 KiB, one TR */
	zassert_equal(bcdma_split_block(0x10000, p), 1);
	zassert_equal(p[0].icnt0, 0x8000);
	zassert_equal(p[0].icnt1, 2);
}

ZTEST(bcdma_desc, test_split_remainder)
{
	struct bcdma_piece p[2];
	uint64_t size = 3U * 0x8000U + 100U;

	zassert_equal(bcdma_split_block(size, p), 2);
	zassert_equal(p[0].icnt0, 0x8000);
	zassert_equal(p[0].icnt1, 3);
	zassert_equal(p[1].offset, 3U * 0x8000U);
	zassert_equal(p[1].icnt0, 100);
	zassert_equal(p[1].icnt1, 1);
	zassert_equal((uint64_t)p[0].icnt0 * p[0].icnt1 + p[1].icnt0, size);
}

ZTEST(bcdma_desc, test_split_limits)
{
	struct bcdma_piece p[2];

	zassert_equal(bcdma_split_block(0, p), -EINVAL);
	zassert_equal(bcdma_split_block(BCDMA_BLOCK_MAX, p), 2);
	zassert_equal(p[0].icnt1, 0xFFFF);
	zassert_equal(p[1].icnt0, 0x7FFF);
	zassert_equal(bcdma_split_block(BCDMA_BLOCK_MAX + 1U, p), -EINVAL);
}

ZTEST(bcdma_desc, test_tr15_fields)
{
	struct bcdma_tr15 tr;
	struct bcdma_piece p = {.offset = 0x100, .icnt0 = 0x8000, .icnt1 = 3};
	const uint32_t *w = (const uint32_t *)&tr;

	bcdma_tr15_fill(&tr, 0x880000000ULL, 0x82000000ULL, &p, 15);

	/* Word positions from TRM Table 11-21 */
	zassert_equal(w[0], 15U, "FLAGS: type 15, no trigger, event at completion");
	zassert_equal(w[1], 0x8000U | (3U << 16), "ICNT0/ICNT1");
	zassert_equal(w[2], 0x80000100U, "ADDR low");
	zassert_equal(w[3], 0x000F0008U, "ADDR high with ASEL 15 in bits 51:48");
	zassert_equal(w[4], 0x8000U, "DIM1");
	zassert_equal(w[5], 1U | (1U << 16), "ICNT2/ICNT3");
	zassert_equal(w[8], 0U, "reserved");
	zassert_equal(w[9], 0x8000U, "DDIM1");
	zassert_equal(w[10], 0x82000100U, "DADDR low");
	zassert_equal(w[11], 0x000F0000U, "DADDR high with ASEL 15");
	zassert_equal(w[14], 0x8000U | (3U << 16), "DICNT0/DICNT1");
	zassert_equal(w[15], 1U | (1U << 16), "DICNT2/DICNT3");
}

ZTEST(bcdma_desc, test_trpd_layout)
{
	static uint8_t desc[1024] __aligned(64);
	uint32_t *w = (uint32_t *)desc;

	zassert_equal(sizeof(struct bcdma_tr15), 64);

	bcdma_trpd_init(desc, 4, false);
	zassert_equal(w[0], 0xC0000003U, "type 3, reload 0, last entry 3");
	zassert_equal(w[1], 2U << 24, "64-byte TR records");
	zassert_equal(w[2], 0U);
	zassert_equal(w[3], 0U);

	bcdma_trpd_init(desc, 1, true);
	zassert_equal(w[0], 0xC0000000U | (0x1FFU << 20), "perpetual reload, last entry 0");

	zassert_equal((uint8_t *)bcdma_trpd_tr(desc, 0) - desc, 64);
	zassert_equal((uint8_t *)bcdma_trpd_tr(desc, 2) - desc, 192);
	zassert_equal((volatile uint8_t *)bcdma_trpd_resp(desc, 4, 0) - desc, 320);
	zassert_equal((volatile uint8_t *)bcdma_trpd_resp(desc, 4, 3) - desc, 332);
	zassert_equal(bcdma_trpd_size(4), 336);
}

ZTEST_SUITE(bcdma_desc, NULL, NULL, NULL, NULL, NULL);
