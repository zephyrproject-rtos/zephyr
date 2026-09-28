/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/bluetooth/mesh.h>

#include "mesh.h"
#include "net.h"
#include "settings.h"

#define STORE_TIMEOUT_MS (CONFIG_BT_MESH_STORE_TIMEOUT * MSEC_PER_SEC)
#define RPL_TIMEOUT_MS	 (CONFIG_BT_MESH_RPL_STORE_TIMEOUT * MSEC_PER_SEC)

/* Slack around each deadline, large enough to absorb workqueue latency and small
 * enough that STORE_TIMEOUT_MS and RPL_TIMEOUT_MS stay clearly separated.
 */
#define MARGIN_MS 300

static uint32_t store_cnt[BT_MESH_SETTINGS_FLAG_COUNT];

static const enum bt_mesh_settings_flag no_wait_flags[] = {
	BT_MESH_SETTINGS_NET_PENDING,
	BT_MESH_SETTINGS_IV_PENDING,
	BT_MESH_SETTINGS_SEQ_PENDING,
	BT_MESH_SETTINGS_CDB_PENDING,
};

static const enum bt_mesh_settings_flag rpl_flags[] = {
	BT_MESH_SETTINGS_RPL_PENDING,
	BT_MESH_SETTINGS_SRPL_PENDING,
};

static const enum bt_mesh_settings_flag generic_flags[] = {
	BT_MESH_SETTINGS_RPL_RESET_PENDING,
	BT_MESH_SETTINGS_NET_KEYS_PENDING,
	BT_MESH_SETTINGS_APP_KEYS_PENDING,
	BT_MESH_SETTINGS_DEV_KEY_CAND_PENDING,
	BT_MESH_SETTINGS_HB_PUB_PENDING,
	BT_MESH_SETTINGS_CFG_PENDING,
	BT_MESH_SETTINGS_COMP_PENDING,
	BT_MESH_SETTINGS_MOD_PENDING,
	BT_MESH_SETTINGS_VA_PENDING,
	BT_MESH_SETTINGS_SSEQ_PENDING,
	BT_MESH_SETTINGS_BRG_PENDING,
};

/**** Mocked functions ****/

struct bt_mesh_net bt_mesh;

bool bt_is_ready(void)
{
	return false;
}

struct bt_mesh_subnet *bt_mesh_subnet_next(struct bt_mesh_subnet *sub)
{
	return NULL;
}

int bt_mesh_start(void)
{
	return 0;
}

void bt_mesh_net_settings_commit(void)
{
}

void bt_mesh_model_settings_commit(void)
{
}

#define MOCK_STORE(_func, _flag)                                                                   \
	void bt_mesh_##_func(void)                                                                 \
	{                                                                                          \
		store_cnt[BT_MESH_SETTINGS_##_flag##_PENDING]++;                                   \
	}

/* Serves both replay list flags, and clears them the way the real handler does. */
void bt_mesh_rpl_pending_store_all_nodes(void)
{
	bt_mesh_settings_store_cancel(BT_MESH_SETTINGS_RPL_PENDING);
	bt_mesh_settings_store_cancel(BT_MESH_SETTINGS_RPL_RESET_PENDING);
	store_cnt[BT_MESH_SETTINGS_RPL_PENDING]++;
}
MOCK_STORE(srpl_pending_store, SRPL)
MOCK_STORE(net_pending_net_store, NET)
MOCK_STORE(net_pending_iv_store, IV)
MOCK_STORE(net_pending_seq_store, SEQ)
MOCK_STORE(cdb_pending_store, CDB)
MOCK_STORE(subnet_pending_store, NET_KEYS)
MOCK_STORE(app_key_pending_store, APP_KEYS)
MOCK_STORE(net_pending_dev_key_cand_store, DEV_KEY_CAND)
MOCK_STORE(hb_pub_pending_store, HB_PUB)
MOCK_STORE(cfg_pending_store, CFG)
MOCK_STORE(comp_data_pending_clear, COMP)
MOCK_STORE(model_pending_store, MOD)
MOCK_STORE(va_pending_store, VA)
MOCK_STORE(sseq_pending_store, SSEQ)
MOCK_STORE(brg_cfg_pending_store, BRG)

/**** Helper functions ****/

static uint32_t total_stores(void)
{
	uint32_t total = 0;

	for (int i = 0; i < BT_MESH_SETTINGS_FLAG_COUNT; i++) {
		total += store_cnt[i];
	}

	return total;
}

/* Both replay list flags are served by one handler, so they share a counter. */
static enum bt_mesh_settings_flag store_slot(enum bt_mesh_settings_flag flag)
{
	return flag == BT_MESH_SETTINGS_RPL_RESET_PENDING ? BT_MESH_SETTINGS_RPL_PENDING : flag;
}

static void assert_only_stored(const enum bt_mesh_settings_flag *flags, size_t cnt)
{
	for (size_t i = 0; i < cnt; i++) {
		zassert_equal(store_cnt[store_slot(flags[i])], 1, "flag %d stored %u times",
			      flags[i], store_cnt[store_slot(flags[i])]);
	}

	zassert_equal(total_stores(), cnt, "%u unexpected stores", total_stores() - cnt);
}

static void assert_stored(enum bt_mesh_settings_flag flag)
{
	zassert_equal(store_cnt[store_slot(flag)], 1, "flag %d stored %u times", flag,
		      store_cnt[store_slot(flag)]);
}

static void assert_not_stored(enum bt_mesh_settings_flag flag)
{
	zassert_equal(store_cnt[store_slot(flag)], 0, "flag %d stored too early", flag);
}

static void *suite_setup(void)
{
	bt_mesh_settings_init();

	return NULL;
}

static void test_setup(void *f)
{
	/* Drop anything a previous test left armed, so its deadline cannot
	 * expire in the middle of this one.
	 */
	bt_mesh_settings_store_pending();
	memset(store_cnt, 0, sizeof(store_cnt));
}

/**** Tests ****/

/** No-wait flags are stored without waiting for any deadline. */
ZTEST(bt_mesh_settings_store, test_no_wait_stored_immediately)
{
	for (size_t i = 0; i < ARRAY_SIZE(no_wait_flags); i++) {
		memset(store_cnt, 0, sizeof(store_cnt));

		bt_mesh_settings_store_schedule(no_wait_flags[i]);
		k_sleep(K_MSEC(MARGIN_MS));

		assert_only_stored(&no_wait_flags[i], 1);
	}
}

/** Generic flags are stored on CONFIG_BT_MESH_STORE_TIMEOUT, not before. */
ZTEST(bt_mesh_settings_store, test_generic_stored_on_store_timeout)
{
	for (size_t i = 0; i < ARRAY_SIZE(generic_flags); i++) {
		memset(store_cnt, 0, sizeof(store_cnt));

		bt_mesh_settings_store_schedule(generic_flags[i]);
		k_sleep(K_MSEC(STORE_TIMEOUT_MS - MARGIN_MS));
		assert_not_stored(generic_flags[i]);

		k_sleep(K_MSEC(2 * MARGIN_MS));
		assert_only_stored(&generic_flags[i], 1);
	}
}

/** Replay list flags are stored on CONFIG_BT_MESH_RPL_STORE_TIMEOUT, not before. */
ZTEST(bt_mesh_settings_store, test_rpl_stored_on_rpl_timeout)
{
	for (size_t i = 0; i < ARRAY_SIZE(rpl_flags); i++) {
		memset(store_cnt, 0, sizeof(store_cnt));

		bt_mesh_settings_store_schedule(rpl_flags[i]);
		k_sleep(K_MSEC(RPL_TIMEOUT_MS - MARGIN_MS));
		assert_not_stored(rpl_flags[i]);

		k_sleep(K_MSEC(2 * MARGIN_MS));
		assert_only_stored(&rpl_flags[i], 1);
	}
}

/** A generic store pending alongside RPL must not pull the replay list forward, nor be
 *  delayed to the replay list deadline itself.
 */
ZTEST(bt_mesh_settings_store, test_generic_does_not_pull_rpl)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
	k_sleep(K_MSEC(MARGIN_MS));

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);

	k_sleep(K_MSEC(STORE_TIMEOUT_MS + MARGIN_MS));
	assert_stored(BT_MESH_SETTINGS_MOD_PENDING);
	assert_not_stored(BT_MESH_SETTINGS_RPL_PENDING);

	k_sleep(K_MSEC(RPL_TIMEOUT_MS));
	assert_stored(BT_MESH_SETTINGS_RPL_PENDING);
}

/** A no-wait store pending alongside RPL must not pull the replay list forward. */
ZTEST(bt_mesh_settings_store, test_no_wait_does_not_pull_rpl)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SRPL_PENDING);
	k_sleep(K_MSEC(MARGIN_MS));

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_IV_PENDING);

	k_sleep(K_MSEC(MARGIN_MS));
	assert_stored(BT_MESH_SETTINGS_IV_PENDING);
	assert_not_stored(BT_MESH_SETTINGS_SRPL_PENDING);

	k_sleep(K_MSEC(RPL_TIMEOUT_MS));
	assert_stored(BT_MESH_SETTINGS_SRPL_PENDING);
}

/** Both replay lists share one deadline, and each is stored only when flagged. */
ZTEST(bt_mesh_settings_store, test_rpl_and_srpl_share_deadline)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
	k_sleep(K_MSEC(STORE_TIMEOUT_MS));

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SRPL_PENDING);

	k_sleep(K_MSEC(RPL_TIMEOUT_MS - STORE_TIMEOUT_MS - MARGIN_MS));
	assert_not_stored(BT_MESH_SETTINGS_RPL_PENDING);
	assert_not_stored(BT_MESH_SETTINGS_SRPL_PENDING);

	k_sleep(K_MSEC(2 * MARGIN_MS));
	assert_only_stored(rpl_flags, ARRAY_SIZE(rpl_flags));
}

/** A no-wait flag flushes the generic flags with it: they share one work item. */
ZTEST(bt_mesh_settings_store, test_no_wait_flushes_generic)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_NET_PENDING);

	k_sleep(K_MSEC(MARGIN_MS));
	assert_stored(BT_MESH_SETTINGS_MOD_PENDING);
	assert_stored(BT_MESH_SETTINGS_NET_PENDING);
}

/** A cancelled flag is not stored when its deadline expires. */
ZTEST(bt_mesh_settings_store, test_cancel_prevents_store)
{
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_RPL_PENDING);
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_MOD_PENDING);

	bt_mesh_settings_store_cancel(BT_MESH_SETTINGS_RPL_PENDING);
	bt_mesh_settings_store_cancel(BT_MESH_SETTINGS_MOD_PENDING);

	k_sleep(K_MSEC(RPL_TIMEOUT_MS + MARGIN_MS));
	zassert_equal(total_stores(), 0, "cancelled flags were stored");
}

/** bt_mesh_settings_store_pending() flushes every group without waiting. */
ZTEST(bt_mesh_settings_store, test_store_pending_flushes_all_groups)
{
	for (int i = 0; i < BT_MESH_SETTINGS_FLAG_COUNT; i++) {
		bt_mesh_settings_store_schedule(i);
	}

	bt_mesh_settings_store_pending();

	for (int i = 0; i < BT_MESH_SETTINGS_FLAG_COUNT; i++) {
		assert_stored(i);
	}

	/* Nothing may remain armed to fire afterwards. The two replay list flags
	 * share one handler, so they account for a single store.
	 */
	k_sleep(K_MSEC(RPL_TIMEOUT_MS + MARGIN_MS));
	zassert_equal(total_stores(), BT_MESH_SETTINGS_FLAG_COUNT - 1, "flag stored twice");
}

/** Every flag of every group completes exactly once when all are pending together. */
ZTEST(bt_mesh_settings_store, test_all_groups_complete)
{
	for (int i = 0; i < BT_MESH_SETTINGS_FLAG_COUNT; i++) {
		bt_mesh_settings_store_schedule(i);
	}

	k_sleep(K_MSEC(MARGIN_MS));
	/* RPL_RESET_PENDING is generic, so only the solicitation list is still due. */
	assert_not_stored(BT_MESH_SETTINGS_SRPL_PENDING);

	k_sleep(K_MSEC(RPL_TIMEOUT_MS));
	for (int i = 0; i < BT_MESH_SETTINGS_FLAG_COUNT; i++) {
		assert_stored(i);
	}
}

ZTEST_SUITE(bt_mesh_settings_store, NULL, suite_setup, test_setup, NULL, NULL);
