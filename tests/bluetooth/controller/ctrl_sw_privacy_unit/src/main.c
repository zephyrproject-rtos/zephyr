/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <stdio.h>
#include <stdlib.h>

#define CONFIG_BT_CTLR_FILTER_ACCEPT_LIST 1
#define CONFIG_BT_CTLR_PRIVACY 1
#define CONFIG_BT_CTLR_RL_SIZE 8
#define CONFIG_BT_CTLR_FAL_SIZE 8
#define CONFIG_BT_CTLR_RPA_CACHE_SIZE 4
#define CONFIG_BT_CTLR_TRPA_CACHE_SIZE 4
#define CONFIG_BT_CTLR_SW_DEFERRED_PRIVACY 1
#define CONFIG_BT_LOG_LEVEL 1

#include "ll_sw/ull_filter.c"

/*
 * Unit test of SW deferred privacy data structure and related methods
 * Tests the prpa and trpa cache functions (prpa_cache_add, prpa_cache_clear, prpa_cache_find
 * and trpa_cache_add, trpa_cache_clear, trpa_cache_find)
 */

#define BT_ADDR_INIT(P0, P1, P2, P3, P4, P5)                                   \
	(&(bt_addr_t){ { P0, P1, P2, P3, P4, P5 } })

void helper_privacy_clear(void)
{
	zassert_equal(newest_prpa, 0, "");
	for (uint8_t i = 0; i < CONFIG_BT_CTLR_RPA_CACHE_SIZE; i++) {
		zassert_equal(prpa_cache[i].taken, 0U, "");
	}
	zassert_equal(newest_trpa, 0, "");
	for (uint8_t i = 0; i < CONFIG_BT_CTLR_TRPA_CACHE_SIZE; i++) {
		zassert_equal(trpa_cache[i].rl_idx, FILTER_IDX_NONE, "");
	}
}

void helper_prpa_add(int skew)
{
	bt_addr_t a1, a2, a3, a4, a5;
	uint8_t pos, ex_pos;

	bt_addr_copy(&a1, BT_ADDR_INIT(0x12, 0x13, 0x14, 0x15, 0x16, 0x17));
	bt_addr_copy(&a2, BT_ADDR_INIT(0x22, 0x23, 0x24, 0x25, 0x26, 0x27));
	bt_addr_copy(&a3, BT_ADDR_INIT(0x32, 0x33, 0x34, 0x35, 0x36, 0x37));
	bt_addr_copy(&a4, BT_ADDR_INIT(0x42, 0x43, 0x44, 0x45, 0x46, 0x47));
	bt_addr_copy(&a5, BT_ADDR_INIT(0x52, 0x53, 0x54, 0x55, 0x56, 0x57));

	prpa_cache_add(&a1);
	pos = prpa_cache_find(&a1);
	ex_pos = (1 + skew) % CONFIG_BT_CTLR_RPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "%d == %d", pos, ex_pos);

	prpa_cache_add(&a2);
	pos = prpa_cache_find(&a2);
	ex_pos = (2 + skew) % CONFIG_BT_CTLR_RPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "");

	prpa_cache_add(&a3);
	pos = prpa_cache_find(&a3);
	ex_pos = (3 + skew) % CONFIG_BT_CTLR_RPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "");

	/* Adding this should cause wrap around */
	prpa_cache_add(&a4);
	pos = prpa_cache_find(&a4);
	ex_pos = (4 + skew) % CONFIG_BT_CTLR_RPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "");

	/* adding this should cause a1 to be dropped */
	prpa_cache_add(&a5);
	pos = prpa_cache_find(&a5);
	ex_pos = (1 + skew) % CONFIG_BT_CTLR_RPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "");

	/* check that a1 can no longer be found */
	pos = prpa_cache_find(&a1);
	zassert_equal(pos, FILTER_IDX_NONE, "");
}

void helper_trpa_add(int skew)
{
	bt_addr_t a1, a2, a3, a4, a5;
	uint8_t pos, ex_pos;

	bt_addr_copy(&a1, BT_ADDR_INIT(0x12, 0x13, 0x14, 0x15, 0x16, 0x17));
	bt_addr_copy(&a2, BT_ADDR_INIT(0x22, 0x23, 0x24, 0x25, 0x26, 0x27));
	bt_addr_copy(&a3, BT_ADDR_INIT(0x32, 0x33, 0x34, 0x35, 0x36, 0x37));
	bt_addr_copy(&a4, BT_ADDR_INIT(0x42, 0x43, 0x44, 0x45, 0x46, 0x47));
	bt_addr_copy(&a5, BT_ADDR_INIT(0x52, 0x53, 0x54, 0x55, 0x56, 0x57));

	trpa_cache_add(&a1, 0);
	pos = trpa_cache_find(&a1, 0);
	ex_pos = (1 + skew) % CONFIG_BT_CTLR_TRPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "%d == %d", pos, ex_pos);

	trpa_cache_add(&a2, 1);
	pos = trpa_cache_find(&a2, 1);
	ex_pos = (2 + skew) % CONFIG_BT_CTLR_TRPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "");

	trpa_cache_add(&a3, 2);
	pos = trpa_cache_find(&a3, 2);
	ex_pos = (3 + skew) % CONFIG_BT_CTLR_TRPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "");

	/* Adding this should cause wrap around */
	trpa_cache_add(&a4, 3);
	pos = trpa_cache_find(&a4, 3);
	ex_pos = (4 + skew) % CONFIG_BT_CTLR_TRPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "");

	/* adding this should cause a1 to be dropped */
	trpa_cache_add(&a5, 4);
	pos = trpa_cache_find(&a5, 4);
	ex_pos = (1 + skew) % CONFIG_BT_CTLR_TRPA_CACHE_SIZE;
	zassert_equal(pos, ex_pos, "");

	/* check that a1 can no longer be found */
	pos = trpa_cache_find(&a1, 0);
	zassert_equal(pos, FILTER_IDX_NONE, "");
}

static void before(void *data)
{
	ARG_UNUSED(data);

	/* Run before each test - clear the cache so we start fresh each time. */
	prpa_cache_clear();
	trpa_cache_clear();
}

ZTEST_SUITE(test_ctrl_sw_privacy_unit, NULL, NULL, before, NULL, NULL);

ZTEST(test_ctrl_sw_privacy_unit, test_privacy_clear)
{
	helper_privacy_clear();
}

ZTEST(test_ctrl_sw_privacy_unit, test_privacy_add)
{
	helper_prpa_add(0);
	helper_trpa_add(0);
}

ZTEST(test_ctrl_sw_privacy_unit, test_privacy_add_stress)
{
	bt_addr_t ar;

	for (uint8_t skew = 0; skew < CONFIG_BT_CTLR_RPA_CACHE_SIZE; skew++) {
		for (uint8_t i = 0; i < skew; i++) {
			bt_addr_copy(&ar,
				     BT_ADDR_INIT(0xde, 0xad, 0xbe,
						  0xef, 0xaa, 0xff));
			prpa_cache_add(&ar);
		}
		helper_prpa_add(skew);
		prpa_cache_clear();
	}

	for (uint8_t skew = 0; skew < CONFIG_BT_CTLR_TRPA_CACHE_SIZE; skew++) {
		for (uint8_t i = 0; i < skew; i++) {
			bt_addr_copy(&ar,
				     BT_ADDR_INIT(0xde, 0xad, 0xbe,
						  0xef, 0xaa, 0xff));
			trpa_cache_add(&ar, 0);
		}
		helper_trpa_add(skew);
		trpa_cache_clear();
	}
}
