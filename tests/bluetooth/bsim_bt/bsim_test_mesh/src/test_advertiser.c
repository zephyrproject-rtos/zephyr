/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mesh_test.h"

#define LOG_MODULE_NAME test_adv

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 60 /*seconds*/

static const struct bt_mesh_test_cfg adv_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};

static void test_adv_init(void)
{
	bt_mesh_test_cfg_set(&adv_cfg, WAIT_TIME);
}

static void test_adv_tx_cb(void)
{
	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "adv_" #role "_" #name,             \
		.test_descr = description,                     \
		.test_pre_init_f = test_##role##_init,         \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_adv[] = {
	TEST_CASE(adv, tx_cb,  "ADV: tx complete callbacks"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_adv_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_adv);
	return tests;
}
