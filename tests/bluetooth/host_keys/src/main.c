/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/keys.h>
#include <host/settings.h>
#include <zephyr/sys/byteorder.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U

#define NUM_PEERS 2U

static const bt_addr_le_t peers[NUM_PEERS] = {
	{
		.type = BT_ADDR_LE_PUBLIC,
		.a.val = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
	},
	{
		.type = BT_ADDR_LE_PUBLIC,
		.a.val = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16},
	},
};

/* Third peer: the pool holds CONFIG_BT_MAX_PAIRED (2) entries, so a third
 * address is needed for the full-pool test cases.
 */
static const bt_addr_le_t peer_extra = {
	.type = BT_ADDR_LE_PUBLIC,
	.a.val = {0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
};

static uint8_t rl_adds[NUM_PEERS];
static uint8_t rl_rejects[NUM_PEERS];

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void le_rand(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_le_rand *rp;
	static uint8_t rand_value;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;

	for (size_t i = 0U; i < ARRAY_SIZE(rp->rand); i++) {
		rp->rand[i] = rand_value++;
	}
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE supported and BR/EDR not supported. */
	rp->features[4] = BIT(5) | BIT(6);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	rp->features[0] = BIT(BT_LE_FEAT_BIT_PRIVACY);
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(27U);
	rp->le_max_num = 1U;
}

static void le_read_rl_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			    uint16_t opcode)
{
	struct bt_hci_rp_le_read_rl_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->rl_size = CONFIG_TEST_RL_SIZE;
}

static void le_add_dev_to_rl(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			     uint16_t opcode)
{
	struct bt_hci_cp_le_add_dev_to_rl *cp = (void *)buf->data;
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;

	for (size_t i = 0U; i < NUM_PEERS; i++) {
		if (!bt_addr_le_eq(&cp->peer_id_addr, &peers[i])) {
			continue;
		}

		rl_adds[i]++;

		/* The Controller rejects an entry that is already in the list. */
		if (rl_adds[i] > 1U) {
			rl_rejects[i]++;
			rp->status = BT_HCI_ERR_INVALID_PARAM;
		}
	}
}

static const struct cmd_handler cmds[] = {
	{
		BT_HCI_OP_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_read_local_features),
		read_local_features,
	},
	{
		BT_HCI_OP_READ_SUPPORTED_COMMANDS,
		sizeof(struct bt_hci_rp_read_supported_commands),
		read_supported_commands,
	},
	{
		BT_HCI_OP_READ_BD_ADDR,
		sizeof(struct bt_hci_rp_read_bd_addr),
		read_bd_addr,
	},
	{
		BT_HCI_OP_LE_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_le_read_local_features),
		le_read_local_features,
	},
	{
		BT_HCI_OP_LE_READ_BUFFER_SIZE,
		sizeof(struct bt_hci_rp_le_read_buffer_size),
		le_read_buffer_size,
	},
	{
		BT_HCI_OP_LE_READ_SUPP_STATES,
		sizeof(struct bt_hci_rp_le_read_supp_states),
		le_read_supp_states,
	},
	{
		BT_HCI_OP_LE_RAND,
		sizeof(struct bt_hci_rp_le_rand),
		le_rand,
	},
	{
		BT_HCI_OP_LE_READ_RL_SIZE,
		sizeof(struct bt_hci_rp_le_read_rl_size),
		le_read_rl_size,
	},
	{
		BT_HCI_OP_LE_ADD_DEV_TO_RL,
		sizeof(struct bt_hci_evt_cc_status),
		le_add_dev_to_rl,
	},
};

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	for (size_t i = 0U; i < ARRAY_SIZE(cmds); i++) {
		if (cmds[i].opcode == opcode) {
			cmds[i].handler(cmd, &evt, cmds[i].len, opcode);
			bt_hci_recv(dev, evt);
			return;
		}
	}

	generic_success(cmd, &evt, GENERIC_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected buffer type %u", type);
	cmd_handle(dev, buf);
	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void store_bond(size_t idx)
{
	struct bt_keys *keys;

	keys = bt_keys_get_type(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[idx]);
	zassert_not_null(keys, "No room for peer %zu's keys", idx);
	(void)memset(keys->irk.val, 0x5A + idx, sizeof(keys->irk.val));
	zassert_ok(bt_keys_store(keys), "Failed to store peer %zu's keys", idx);
}

static void *keys_setup(void)
{
	int err;

	err = bt_enable(NULL);
	zassert_true(err == 0 || err == -EALREADY, "bt_enable failed (%d)", err);

	store_bond(0U);
	/* Drop the in-RAM copy so that the first settings_load() has to restore it. */
	bt_keys_reset();

	return NULL;
}

static void load_and_advertise(void)
{
	zassert_ok(settings_load(), "settings_load() failed");
	zassert_ok(bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0, NULL, 0), "Advertising failed");
	zassert_ok(bt_le_adv_stop(), "Stopping advertising failed");
}

ZTEST_SUITE(bt_keys_commit, NULL, keys_setup, NULL, NULL, NULL);

ZTEST(bt_keys_commit, test_settings_reload_does_not_add_irk_twice)
{
	const uint8_t expected_adds = (CONFIG_TEST_RL_SIZE > 0) ? 1U : 0U;
	struct bt_keys *keys;

	zassert_is_null(bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]),
			"The peer's keys survived bt_keys_reset()");

	load_and_advertise();

	keys = bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]);
	zassert_not_null(keys, "The peer's keys were not restored");
	zassert_true((keys->state & BT_KEYS_ID_ADDED) != 0U, "The peer's IRK was never added");
	zassert_equal(bt_dev.le.rl_entries, 1U, "rl_entries %u after the first load",
		      bt_dev.le.rl_entries);
	zassert_equal(rl_adds[0], expected_adds, "%u adds after the first load", rl_adds[0]);

	store_bond(1U);

	load_and_advertise();

	zassert_equal(rl_adds[0], expected_adds, "%u adds of the first peer after the second load",
		      rl_adds[0]);
	zassert_equal(rl_adds[1], expected_adds, "%u adds of the second peer after the second load",
		      rl_adds[1]);
	zassert_equal(rl_rejects[0], 0U, "%u adds of the first peer rejected", rl_rejects[0]);
	zassert_equal(rl_rejects[1], 0U, "%u adds of the second peer rejected", rl_rejects[1]);
	zassert_equal(bt_dev.le.rl_entries, 2U, "rl_entries %u after the second load",
		      bt_dev.le.rl_entries);
}

/* ZTest does not guarantee test execution order within a suite, so reset
 * the key pool before every test to keep them independent.
 */
static void reset_key_pool(void *fixture)
{
	ARG_UNUSED(fixture);

	bt_keys_reset();
}

ZTEST_SUITE(bt_keys_pool, NULL, NULL, reset_key_pool, NULL, NULL);

/* bt_keys_get_addr(): allocate a slot and find it again. */
static ZTEST(bt_keys_pool, test_get_addr_allocates_slot)
{
	const struct bt_keys *keys = bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]);

	zassert_not_null(keys, "No slot for the first peer");
	zassert_equal_ptr(keys, bt_keys_find_addr(BT_ID_DEFAULT, &peers[0]),
			  "Allocated slot not findable");
	zassert_equal(keys->keys, 0U, "New slot must have no key types");
}

/* bt_keys_get_addr(): a full pool rejects new addresses, but an existing
 * entry must still be returned — the find-first path must not be confused
 * with allocation.
 */
static ZTEST(bt_keys_pool, test_get_addr_returns_existing_when_full)
{
	const struct bt_keys *keys;

	if (IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)) {
		/* Under CONFIG_BT_KEYS_OVERWRITE_OLDEST a full pool evicts the
		 * least recently used entry instead of rejecting; that behavior
		 * is covered by the bt_keys_oldest suite.
		 */
		ztest_test_skip();
	}

	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]));
	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[1]));

	keys = bt_keys_get_addr(BT_ID_DEFAULT, &peer_extra);
	zassert_is_null(keys, "Pool full, but a slot was still handed out");

	zassert_equal_ptr(bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]),
			  bt_keys_find_addr(BT_ID_DEFAULT, &peers[0]),
			  "Existing entry lost when the pool filled up");
}

/* bt_keys_get_type(): same ref on repeated calls, key type bit set, and no
 * slot handed out from a full pool.
 */
static ZTEST(bt_keys_pool, test_get_type_existing_and_full)
{
	const struct bt_keys *keys;

	if (IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)) {
		/* Under CONFIG_BT_KEYS_OVERWRITE_OLDEST a full pool evicts the
		 * least recently used entry instead of rejecting; that behavior
		 * is covered by the bt_keys_oldest suite.
		 */
		ztest_test_skip();
	}

	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]));
	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[1]));

	keys = bt_keys_get_type(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]);
	zassert_not_null(keys, "Existing key type not found");
	zassert_equal_ptr(keys, bt_keys_get_type(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]),
			  "Repeated get_type returned a different ref");
	zassert_true((keys->keys & BT_KEYS_IRK) != 0U, "IRK bit not set");

	zassert_is_null(bt_keys_get_type(BT_KEYS_IRK, BT_ID_DEFAULT, &peer_extra),
			"Full pool must not hand out more slots");
}

static void bond_count_cb(const struct bt_bond_info *info, void *user_data)
{
	size_t *count = user_data;

	ARG_UNUSED(info);
	(*count)++;
}

/* bt_keys_find()/find_addr(): miss on empty pool, miss for a type that is
 * not set, hit, and the public bond enumeration agrees.
 */
static ZTEST(bt_keys_pool, test_find_and_foreach)
{
	const struct bt_keys *keys;
	size_t count = 0;

	zassert_is_null(bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]),
			"Find must miss on an empty pool");

	keys = bt_keys_get_type(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]);
	zassert_not_null(keys);

	zassert_equal_ptr(bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]), keys,
			  "IRK key not findable");
	zassert_is_null(bt_keys_find(BT_KEYS_LTK, BT_ID_DEFAULT, &peers[0]),
			"Find must miss for a type that is not set");
	zassert_equal_ptr(bt_keys_find_addr(BT_ID_DEFAULT, &peers[0]), keys,
			  "find_addr must locate the slot");

	bt_foreach_bond(BT_ID_DEFAULT, bond_count_cb, &count);
	zassert_equal(count, 1U, "bt_foreach_bond counted %zu bonds", count);
}

/* ------------------------------------------------------------------------
 * Settings migration / edge-case coverage (keys_set branches)
 *
 * Each test writes a crafted entry directly to the settings backend and
 * reloads only that entry with settings_load_subtree(), so every keys_set
 * branch is reached through the real persistence path without interference
 * from other stored bonds.
 * ---------------------------------------------------------------------
 */

static const bt_addr_le_t migration_peers[] = {
	{.type = BT_ADDR_LE_PUBLIC, .a.val = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25}},
	{.type = BT_ADDR_LE_PUBLIC, .a.val = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35}},
	{.type = BT_ADDR_LE_PUBLIC, .a.val = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45}},
	{.type = BT_ADDR_LE_PUBLIC, .a.val = {0x50, 0x51, 0x52, 0x53, 0x54, 0x55}},
	{.type = BT_ADDR_LE_PUBLIC, .a.val = {0x60, 0x61, 0x62, 0x63, 0x64, 0x65}},
	{.type = BT_ADDR_LE_PUBLIC, .a.val = {0x70, 0x71, 0x72, 0x73, 0x74, 0x75}},
	{.type = BT_ADDR_LE_PUBLIC, .a.val = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85}},
};

/* Must match the host's STORAGE_CFG_VERSION. If the host bumps it, this
 * test fails loudly - update this constant.
 */
#define STORAGE_CFG_VERSION_CURRENT 17U

static void migration_blob_init(struct bt_keys *keys, const bt_addr_le_t *addr, uint16_t types,
				uint8_t flags)
{
	uint32_t cfg_flags = 0;

	/* cfg_flags must match the host's STORAGE_CFG_FLAGS, which is derived
	 * from the same Kconfig options.
	 */
	if (IS_ENABLED(CONFIG_BT_SIGNING)) {
		cfg_flags |= BT_KEYS_CFG_SIGNING;
	}
	if (IS_ENABLED(CONFIG_BT_SMP_SC_PAIR_ONLY)) {
		cfg_flags |= BT_KEYS_CFG_SC_PAIR_ONLY;
	}
	if (IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)) {
		cfg_flags |= BT_KEYS_CFG_OVERWRITE_OLDEST;
	}

	(void)memset(keys, 0, sizeof(*keys));
	keys->id = BT_ID_DEFAULT;
	bt_addr_le_copy(&keys->addr, addr);
	keys->cfg_version = STORAGE_CFG_VERSION_CURRENT;
	sys_put_le24(cfg_flags, keys->cfg_flags);
	keys->enc_size = 16;
	keys->keys = types;
	keys->flags = flags;
}

/* The suite must also pass when run without the other suites, so it
 * initializes the settings subsystem itself.
 */
static void *settings_suite_setup(void)
{
	int err;

	err = settings_subsys_init();
	zassert_true(err == 0 || err == -EALREADY, "settings_subsys_init() failed (%d)", err);

	return NULL;
}

static void settings_suite_teardown(void *fixture)
{
	char name[BT_SETTINGS_KEY_MAX];
	int err;

	ARG_UNUSED(fixture);

	bt_keys_reset();
	for (size_t i = 0; i < ARRAY_SIZE(migration_peers); i++) {
		zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &migration_peers[i],
						  NULL));
		err = settings_save_one(name, NULL, 0);
		zassert_true(err == 0 || err == -ENOENT, "settings_save_one() failed (%d)", err);
	}
}

ZTEST_SUITE(bt_keys_settings, NULL, settings_suite_setup, reset_key_pool, NULL,
	    settings_suite_teardown);

/* Well-formed stored bond restores through the real load path. */
static ZTEST(bt_keys_settings, test_load_stored_bond)
{
	struct bt_keys stored;
	const struct bt_keys *keys;
	char name[BT_SETTINGS_KEY_MAX];

	migration_blob_init(&stored, &migration_peers[0], BT_KEYS_IRK, 0);
	for (int i = 0; i < 16; i++) {
		stored.irk.val[i] = 0xa0 + i;
	}

	zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &migration_peers[0], NULL));
	zassert_ok(settings_save_one(name, stored.storage_start, BT_KEYS_STORAGE_LEN));
	zassert_ok(settings_load_subtree(name));

	keys = bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &migration_peers[0]);
	zassert_not_null(keys, "Stored bond not restored");
	zassert_mem_equal(keys->irk.val, stored.irk.val, 16, "IRK mismatch after load");
}

/* A zero-length entry is settings' delete representation: it removes an
 * existing bond (covers the keys_set delete branch).
 */
static ZTEST(bt_keys_settings, test_load_delete_marker_removes_bond)
{
	struct bt_keys stored;
	const struct bt_keys *keys;
	char name[BT_SETTINGS_KEY_MAX];

	migration_blob_init(&stored, &migration_peers[1], BT_KEYS_IRK, 0);

	zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &migration_peers[1], NULL));
	zassert_ok(settings_save_one(name, stored.storage_start, BT_KEYS_STORAGE_LEN));
	zassert_ok(settings_load_subtree(name));

	keys = bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &migration_peers[1]);
	zassert_not_null(keys, "Stored bond not restored");

	/* Drop the NVS copy and deliver a zero-length entry through the
	 * runtime backend to reach the keys_set delete branch.
	 */
	zassert_ok(settings_save_one(name, NULL, 0));
	zassert_ok(settings_runtime_set(name, stored.storage_start, 0));
	zassert_ok(settings_load_subtree(name));

	zassert_is_null(bt_keys_find_addr(BT_ID_DEFAULT, &migration_peers[1]),
			"Delete marker did not remove the bond");
}

/* Old storage format: no cfg_version/cfg_flags, blob starts at enc_size. */
static ZTEST(bt_keys_settings, test_load_old_format_bond)
{
	struct bt_keys stored;
	const struct bt_keys *keys;
	char name[BT_SETTINGS_KEY_MAX];
	const size_t old_len = sizeof(struct bt_keys) - offsetof(struct bt_keys, enc_size);

	migration_blob_init(&stored, &migration_peers[2], BT_KEYS_IRK, 0);
	for (int i = 0; i < 16; i++) {
		stored.irk.val[i] = 0xb0 + i;
	}

	zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &migration_peers[2], NULL));
	zassert_ok(settings_save_one(name, &stored.enc_size, old_len));
	zassert_ok(settings_load_subtree(name));

	keys = bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &migration_peers[2]);
	zassert_not_null(keys, "Old-format bond not restored");
	zassert_mem_equal(keys->irk.val, stored.irk.val, 16, "IRK mismatch after old-format load");
}

/* Neither the current nor the compatible length: rejected. */
static ZTEST(bt_keys_settings, test_load_rejects_invalid_length)
{
	struct bt_keys stored;
	char name[BT_SETTINGS_KEY_MAX];

	migration_blob_init(&stored, &migration_peers[3], BT_KEYS_IRK, 0);

	zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &migration_peers[3], NULL));
	zassert_ok(settings_save_one(name, stored.storage_start, BT_KEYS_STORAGE_LEN - 1));
	zassert_ok(settings_load_subtree(name));

	zassert_is_null(bt_keys_find_addr(BT_ID_DEFAULT, &migration_peers[3]),
			"Invalid-length entry must be rejected");
}

/* cfg_flags that do not match the current build: rejected. The toggle
 * mismatches regardless of which flags the build enables.
 */
static ZTEST(bt_keys_settings, test_load_rejects_cfg_flags_mismatch)
{
	struct bt_keys stored;
	char name[BT_SETTINGS_KEY_MAX];

	migration_blob_init(&stored, &migration_peers[4], BT_KEYS_IRK, 0);
	stored.cfg_flags[0] ^= BT_KEYS_CFG_SIGNING;

	zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &migration_peers[4], NULL));
	zassert_ok(settings_save_one(name, stored.storage_start, BT_KEYS_STORAGE_LEN));
	zassert_ok(settings_load_subtree(name));

	zassert_is_null(bt_keys_find_addr(BT_ID_DEFAULT, &migration_peers[4]),
			"cfg flags mismatch must be rejected");
}

/* A cfg_version other than the current one: rejected. */
static ZTEST(bt_keys_settings, test_load_rejects_cfg_version_mismatch)
{
	struct bt_keys stored;
	char name[BT_SETTINGS_KEY_MAX];

	migration_blob_init(&stored, &migration_peers[6], BT_KEYS_IRK, 0);
	stored.cfg_version = STORAGE_CFG_VERSION_CURRENT + 1;

	zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &migration_peers[6], NULL));
	zassert_ok(settings_save_one(name, stored.storage_start, BT_KEYS_STORAGE_LEN));
	zassert_ok(settings_load_subtree(name));

	zassert_is_null(bt_keys_find_addr(BT_ID_DEFAULT, &migration_peers[6]),
			"cfg version mismatch must be rejected");
}

/* Legacy authenticated flag without SC or OOB pairing: downgraded to
 * unauthenticated on load. With SC set, it is retained.
 */
static ZTEST(bt_keys_settings, test_load_downgrades_unprovable_authentication)
{
	struct bt_keys stored;
	const struct bt_keys *keys;
	char name[BT_SETTINGS_KEY_MAX];

	zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &migration_peers[5], NULL));

	migration_blob_init(&stored, &migration_peers[5], BT_KEYS_LTK, BT_KEYS_AUTHENTICATED);
	zassert_ok(settings_save_one(name, stored.storage_start, BT_KEYS_STORAGE_LEN));
	zassert_ok(settings_load_subtree(name));

	keys = bt_keys_find(BT_KEYS_LTK, BT_ID_DEFAULT, &migration_peers[5]);
	zassert_not_null(keys, "LTK bond not restored");
	zassert_equal(keys->flags & BT_KEYS_AUTHENTICATED, 0U,
		      "AUTHENTICATED without SC/OOB must be downgraded");

	migration_blob_init(&stored, &migration_peers[5], BT_KEYS_LTK,
			    BT_KEYS_AUTHENTICATED | BT_KEYS_SC);
	zassert_ok(settings_save_one(name, stored.storage_start, BT_KEYS_STORAGE_LEN));
	zassert_ok(settings_load_subtree(name));

	keys = bt_keys_find(BT_KEYS_LTK, BT_ID_DEFAULT, &migration_peers[5]);
	zassert_not_null(keys);
	zassert_not_equal(keys->flags & BT_KEYS_AUTHENTICATED, 0U,
			  "AUTHENTICATED with SC must be retained");
}

#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
/* Deliver an HCI event to the host. Takes ownership of buf. */
static void drv_recv(struct net_buf *buf)
{
	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

/* ------------------------------------------------------------------------
 * CONFIG_BT_KEYS_OVERWRITE_OLDEST pool eviction coverage.
 *
 * In-use entries are simulated with fake incoming connections: the host
 * advertises, then an LE Connection Complete event with the peripheral
 * role is injected through the test HCI driver.
 * ---------------------------------------------------------------------
 */

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);

static struct bt_conn *connected_peers[2];
static size_t connected_count;

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	zassert_equal(err, 0U, "Connection failed (%u)", err);
	if (connected_count < ARRAY_SIZE(connected_peers)) {
		connected_peers[connected_count++] = bt_conn_ref(conn);
	}
	k_sem_give(&connected_sem);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);
	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(test_conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static void inject_conn_complete(const bt_addr_le_t *peer, uint16_t handle)
{
	struct net_buf *buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	struct bt_hci_evt_le_meta_event *mev;
	struct bt_hci_evt_le_conn_complete *cc;

	evt_create(buf, BT_HCI_EVT_LE_META_EVENT,
		   sizeof(struct bt_hci_evt_le_meta_event) +
			   sizeof(struct bt_hci_evt_le_conn_complete));
	mev = net_buf_add(buf, sizeof(*mev));
	cc = net_buf_add(buf, sizeof(*cc));

	mev->subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	cc->status = BT_HCI_ERR_SUCCESS;
	cc->handle = sys_cpu_to_le16(handle);
	cc->role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&cc->peer_addr, peer);
	cc->interval = sys_cpu_to_le16(24U);
	cc->latency = 0U;
	cc->supv_timeout = sys_cpu_to_le16(400U);
	cc->clock_accuracy = 0U;

	drv_recv(buf);
}

static void inject_disconn_complete(uint16_t handle)
{
	struct net_buf *buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	struct bt_hci_evt_disconn_complete *dc;

	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(struct bt_hci_evt_disconn_complete));
	dc = net_buf_add(buf, sizeof(*dc));

	dc->status = BT_HCI_ERR_SUCCESS;
	dc->handle = sys_cpu_to_le16(handle);
	dc->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;

	drv_recv(buf);
}

static void connect_peer(const bt_addr_le_t *peer, uint16_t handle)
{
	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0));
	inject_conn_complete(peer, handle);
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connected callback");
	zassert_ok(bt_le_adv_stop());
}

static void disconnect_peer(struct bt_conn *conn, uint16_t handle)
{
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN));
	inject_disconn_complete(handle);
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnected callback");
	bt_conn_unref(conn);
}

static void oldest_before(void *fixture)
{
	ARG_UNUSED(fixture);

	connected_count = 0;
	bt_keys_reset();
}

/* The suite must also pass when run without the other suites, so it
 * enables Bluetooth itself. settings_load() is needed for the "bt" tree
 * to be applied before the stack is ready.
 */
static void *oldest_setup(void)
{
	int err;

	err = bt_enable(NULL);
	zassert_true(err == 0 || err == -EALREADY, "bt_enable failed (%d)", err);
	zassert_ok(settings_load());

	return NULL;
}

ZTEST_SUITE(bt_keys_oldest, NULL, oldest_setup, oldest_before, NULL, NULL);

/* A full pool evicts the oldest entry and hands out its slot. */
static ZTEST(bt_keys_oldest, test_evict_oldest_not_in_use)
{
	const struct bt_keys *k1 = bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]);
	const struct bt_keys *k2 = bt_keys_get_addr(BT_ID_DEFAULT, &peers[1]);
	const struct bt_keys *k3;

	zassert_not_null(k1);
	zassert_not_null(k2);

	k3 = bt_keys_get_addr(BT_ID_DEFAULT, &peer_extra);
	zassert_not_null(k3, "Full pool must evict the oldest entry");
	zassert_is_null(bt_keys_find_addr(BT_ID_DEFAULT, &peers[0]),
			"Oldest peer still in the pool");
	zassert_equal_ptr(k3, k1, "Eviction must reuse the oldest slot");
}

/* bt_keys_update_usage() makes an entry the most recently used one, so the
 * other entry becomes the eviction candidate.
 */
static ZTEST(bt_keys_oldest, test_least_recently_used_is_evicted)
{
	const struct bt_keys *k3;

	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]));
	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[1]));

	bt_keys_update_usage(BT_ID_DEFAULT, &peers[0]);

	k3 = bt_keys_get_addr(BT_ID_DEFAULT, &peer_extra);
	zassert_not_null(k3, "Full pool must evict an entry");
	zassert_is_null(bt_keys_find_addr(BT_ID_DEFAULT, &peers[1]),
			"Least recently used peer must be evicted");
	zassert_not_null(bt_keys_find_addr(BT_ID_DEFAULT, &peers[0]),
			 "Most recently used peer must survive");
}

/* An entry in use by a connected peer must not be evicted. */
static ZTEST(bt_keys_oldest, test_in_use_entry_is_not_evicted)
{
	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]));
	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[1]));

	connect_peer(&peers[0], 0x0001);

	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peer_extra));
	zassert_is_null(bt_keys_find_addr(BT_ID_DEFAULT, &peers[1]),
			"The unused peer must be evicted");
	zassert_not_null(bt_keys_find_addr(BT_ID_DEFAULT, &peers[0]),
			 "The in-use peer must survive");

	disconnect_peer(connected_peers[0], 0x0001);
}

/* When every entry is in use no slot can be handed out. */
static ZTEST(bt_keys_oldest, test_all_in_use_returns_null)
{
	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]));
	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[1]));

	connect_peer(&peers[0], 0x0001);
	connect_peer(&peers[1], 0x0002);

	zassert_is_null(bt_keys_get_addr(BT_ID_DEFAULT, &peer_extra),
			"All entries in use, but a slot was still handed out");

	disconnect_peer(connected_peers[0], 0x0001);
	disconnect_peer(connected_peers[1], 0x0002);
}

/* CONFIG_BT_KEYS_SAVE_AGING_COUNTER_ON_PAIRING: bt_keys_update_usage()
 * writes the aging counter back to settings.
 */
static ZTEST(bt_keys_oldest, test_aging_counter_is_stored)
{
	static const bt_addr_le_t peer_aging = {
		.type = BT_ADDR_LE_PUBLIC,
		.a.val = {0x90, 0x91, 0x92, 0x93, 0x94, 0x95},
	};
	struct bt_keys *keys;
	uint32_t counter;
	char name[BT_SETTINGS_KEY_MAX];

	zassert_not_null(bt_keys_get_type(BT_KEYS_IRK, BT_ID_DEFAULT, &peer_aging));
	zassert_ok(bt_settings_encode_key(name, sizeof(name), "keys", &peer_aging, NULL));

	/* A new key is already the most recently used one, so another key
	 * must be touched first for bt_keys_update_usage() to take effect.
	 */
	zassert_not_null(bt_keys_get_addr(BT_ID_DEFAULT, &peers[0]));
	bt_keys_update_usage(BT_ID_DEFAULT, &peer_aging);

	keys = bt_keys_find_addr(BT_ID_DEFAULT, &peer_aging);
	zassert_not_null(keys);
	counter = keys->aging_counter;

	/* A reload must restore the counter written by bt_keys_update_usage(). */
	bt_keys_reset();
	zassert_ok(settings_load_subtree(name));

	keys = bt_keys_find_addr(BT_ID_DEFAULT, &peer_aging);
	zassert_not_null(keys, "Aging counter was not stored");
	zassert_equal(keys->aging_counter, counter, "Aging counter mismatch after reload");

	/* Drop the entry again: a leftover stored bond changes what the other
	 * suites restore from settings (e.g. bt_keys_commit's rl_entries count).
	 */
	zassert_ok(settings_delete(name));
}
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
