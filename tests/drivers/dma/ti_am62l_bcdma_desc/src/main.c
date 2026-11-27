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

ZTEST(bcdma_desc, test_trpd_layout_32)
{
	static uint8_t desc[1024] __aligned(64);
	const uint32_t *w = (const uint32_t *)desc;

	zassert_equal((uint8_t *)bcdma_trpd_tr_rec(desc, 0, BCDMA_TR_REC_32) - desc, 32);
	zassert_equal((volatile uint8_t *)bcdma_trpd_resp_rec(desc, 1, 0, BCDMA_TR_REC_32) - desc,
		      64);
	zassert_equal(bcdma_trpd_size_rec(1, BCDMA_TR_REC_32), 68);
	bcdma_trpd_init_rec(desc, 1, false, BCDMA_TR_REC_32);
	zassert_equal(w[1], 1U << 24, "TR size code 1");
}

ZTEST(bcdma_desc, test_tr1_fields)
{
	struct bcdma_tr15 tr;
	struct bcdma_piece p = {.offset = 0x40, .icnt0 = 0x200, .icnt1 = 1};
	const uint32_t *w = (const uint32_t *)&tr;

	(void)memset(&tr, 0xA5, sizeof(tr));
	bcdma_tr1_fill(&tr, 0x82000000ULL, &p, 0, false);

	zassert_equal(w[0], 1U, "FLAGS: type 1, no trigger, event at completion");
	zassert_equal(w[1], 0x200U | (1U << 16), "ICNT0/ICNT1");
	zassert_equal(w[2], 0x82000040U, "ADDR low");
	zassert_equal(w[3], 0U, "ADDR high, ASEL 0");
	zassert_equal(w[4], 0x200U, "DIM1");
	for (int i = 5; i < 16; i++) {
		zassert_equal(w[i], 0U, "word %d cleared", i);
	}

	bcdma_tr1_fill(&tr, 0x82000000ULL, &p, 0, true);
	zassert_equal(w[0], 1U | BIT(31), "FLAGS: type 1 with EOP");
}

ZTEST(bcdma_desc, test_static_xy)
{
	uint32_t xy = 0U;

	zassert_ok(bcdma_static_xy(2, 4, false, &xy));
	zassert_equal(xy, (1U << 24) | 2U, "16-bit samples, two per event");
	zassert_ok(bcdma_static_xy(3, 6, false, &xy));
	zassert_equal(xy, (2U << 24) | 2U, "24-bit samples, two per event");
	zassert_ok(bcdma_static_xy(4, 8, true, &xy));
	zassert_equal(xy, BIT(31) | BIT(30) | (3U << 24) | 2U, "32-bit, ACC32, BURST");
	zassert_ok(bcdma_static_xy(1, 1, false, &xy));
	zassert_equal(xy, 1U);
	zassert_ok(bcdma_static_xy(8, 8, false, &xy));
	zassert_equal(xy, (4U << 24) | 1U);
	zassert_ok(bcdma_static_xy(4, 4 * 4095, false, &xy));
	zassert_equal(xy, (3U << 24) | 4095U);

	zassert_equal(bcdma_static_xy(4, 4 * 4096, false, &xy), -EINVAL, "Y overflow");
	zassert_equal(bcdma_static_xy(4, 6, false, &xy), -EINVAL, "partial sample");
	zassert_equal(bcdma_static_xy(4, 0, false, &xy), -EINVAL, "no burst");
	zassert_equal(bcdma_static_xy(5, 5, false, &xy), -EINVAL, "no 40-bit code");
	zassert_equal(bcdma_static_xy(0, 4, false, &xy), -EINVAL);
	zassert_equal(bcdma_static_xy(16, 16, false, &xy), -EINVAL);
}

ZTEST_SUITE(bcdma_desc, NULL, NULL, NULL, NULL, NULL);
