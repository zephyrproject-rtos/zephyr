/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mesh_test.h"
#include "mesh/net.h"

#define LOG_MODULE_NAME test_ivi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 60 /*seconds*/
#define TEST_IV_IDX  100
#define BCN_IV_IN_PROGRESS true
#define BCN_IV_IN_IDLE false

extern struct bt_mesh_net bt_mesh;

static const struct bt_mesh_test_cfg ivu_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};

static void async_send_end(int err, void *data)
{
	struct k_sem *sem = data;

	if (sem) {
		k_sem_give(sem);
	}
}

static const struct bt_mesh_send_cb async_send_cb = {
	.end = async_send_end,
};

static void test_ivu_init(void)
{
	bt_mesh_test_cfg_set(&ivu_cfg, WAIT_TIME);
}

static void emulate_recovery_timeout(void)
{
	k_work_cancel_delayable(&bt_mesh.ivu_timer);
	bt_mesh.ivu_duration = 2 * BT_MESH_IVU_MIN_HOURS;
}

static void test_ivu_recovery(void)
{
	bt_mesh_test_setup();

	bt_mesh.iv_index = TEST_IV_IDX;

	atomic_set_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS);

	/* Already in IV Update in Progress state */
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX, BCN_IV_IN_PROGRESS));

	/* Out of sync */
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX - 1, BCN_IV_IN_IDLE));
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX + 43, BCN_IV_IN_IDLE));

	/* Start recovery */
	ASSERT_TRUE(bt_mesh_net_iv_update(TEST_IV_IDX + 2, BCN_IV_IN_IDLE));
	ASSERT_EQUAL(TEST_IV_IDX + 2, bt_mesh.iv_index);
	ASSERT_FALSE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));

	/* Start recovery before minimum delay */
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX + 4, BCN_IV_IN_IDLE));

	emulate_recovery_timeout();
	bt_mesh.iv_index = TEST_IV_IDX;

	atomic_clear_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS);

	/* Already in IV normal mode */
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX + 1, BCN_IV_IN_IDLE));

	/* Out of sync */
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX - 1, BCN_IV_IN_IDLE));
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX + 43, BCN_IV_IN_IDLE));

	/* Start recovery */
	ASSERT_TRUE(bt_mesh_net_iv_update(TEST_IV_IDX + 2, BCN_IV_IN_IDLE));
	ASSERT_EQUAL(TEST_IV_IDX + 2, bt_mesh.iv_index);
	ASSERT_FALSE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));

	/* Start recovery before minimum delay */
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX + 4, BCN_IV_IN_IDLE));

	PASS();
}

static void test_ivu_normal(void)
{
	bt_mesh_test_setup();
	bt_mesh.iv_index = TEST_IV_IDX;
	atomic_set_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS);

	/* update before minimum duration */
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX, BCN_IV_IN_IDLE));
	/* moving back into the normal mode */
	bt_mesh.ivu_duration = BT_MESH_IVU_MIN_HOURS;
	ASSERT_TRUE(bt_mesh_net_iv_update(TEST_IV_IDX, BCN_IV_IN_IDLE));
	ASSERT_FALSE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_EQUAL(TEST_IV_IDX, bt_mesh.iv_index);
	ASSERT_EQUAL(0, bt_mesh.seq);

	atomic_clear_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS);

	bt_mesh.seq = 100;
	/* update before minimum duration */
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX + 1, BCN_IV_IN_PROGRESS));
	/* moving into the IV update mode */
	bt_mesh.ivu_duration = BT_MESH_IVU_MIN_HOURS;
	ASSERT_TRUE(bt_mesh_net_iv_update(TEST_IV_IDX + 1, BCN_IV_IN_PROGRESS));
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_EQUAL(TEST_IV_IDX + 1, bt_mesh.iv_index);
	ASSERT_EQUAL(100, bt_mesh.seq);

	PASS();
}

static void test_ivu_deferring(void)
{
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);
	bt_mesh_test_setup();
	bt_mesh.iv_index = TEST_IV_IDX;
	atomic_set_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS);
	bt_mesh.ivu_duration = BT_MESH_IVU_MIN_HOURS;

	ASSERT_OK(bt_mesh_test_send_async(0x0002, 20, FORCE_SEGMENTATION, &async_send_cb, &sem));
	ASSERT_FALSE(bt_mesh_net_iv_update(TEST_IV_IDX, BCN_IV_IN_IDLE));
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));

	ASSERT_OK(k_sem_take(&sem, K_SECONDS(10)));
	ASSERT_FALSE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));

	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "ivi_" #role "_" #name,             \
		.test_descr = description,                     \
		.test_pre_init_f = test_##role##_init,         \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_ivi[] = {
	TEST_CASE(ivu, recovery,  "IVI: IV recovery procedure"),
	TEST_CASE(ivu, normal,    "IVI: IV update procedure"),
	TEST_CASE(ivu, deferring, "IVI: deferring of the IV update procedure"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_ivi_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_ivi);
	return tests;
}
