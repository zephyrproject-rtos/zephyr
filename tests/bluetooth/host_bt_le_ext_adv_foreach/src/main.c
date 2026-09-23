/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Application tests for bt_le_ext_adv_foreach().
 *
 * The advertising set storage differs depending on CONFIG_BT_EXT_ADV, so the
 * test suite is built both with extended advertising support enabled (using the
 * advertising set pool) and disabled (using the single legacy advertiser).
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Command handler structure for cmd_handle(). */
struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

/* Add event header to net_buf. */
static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

/* Create a command complete event. */
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

/* Generic command complete with success status. */
static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *ccst;

	ccst = cmd_complete(evt, len, opcode);
	(void)memset(ccst, 0, len);
	ccst->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00U;
	(void)memset(rp->features, 0xFF, sizeof(rp->features));
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
	rp->status = 0x00U;
}

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00U;
	(void)memset(rp->features, 0xFF, sizeof(rp->features));
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00U;
	(void)memset(&rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void le_set_ext_adv_param(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				 uint16_t opcode)
{
	struct bt_hci_rp_le_set_ext_adv_param *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->tx_power = 0;
}

/* HCI command table covering bt_enable(), bt_le_ext_adv_create(),
 * bt_le_ext_adv_delete(), bt_le_adv_start() and bt_le_adv_stop().
 */
static const struct cmd_handler cmds[] = {
	{
		BT_HCI_OP_READ_LOCAL_VERSION_INFO,
		sizeof(struct bt_hci_rp_read_local_version_info),
		generic_success,
	},
	{
		BT_HCI_OP_READ_SUPPORTED_COMMANDS,
		sizeof(struct bt_hci_rp_read_supported_commands),
		read_supported_commands,
	},
	{
		BT_HCI_OP_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_read_local_features),
		read_local_features,
	},
	{
		BT_HCI_OP_READ_BD_ADDR,
		sizeof(struct bt_hci_rp_read_bd_addr),
		generic_success,
	},
	{
		BT_HCI_OP_SET_EVENT_MASK,
		sizeof(struct bt_hci_evt_cc_status),
		generic_success,
	},
	{
		BT_HCI_OP_LE_SET_EVENT_MASK,
		sizeof(struct bt_hci_evt_cc_status),
		generic_success,
	},
	{
		BT_HCI_OP_LE_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_le_read_local_features),
		le_read_local_features,
	},
	{
		BT_HCI_OP_LE_READ_SUPP_STATES,
		sizeof(struct bt_hci_rp_le_read_supp_states),
		le_read_supp_states,
	},
	{
		BT_HCI_OP_LE_RAND,
		sizeof(struct bt_hci_rp_le_rand),
		generic_success,
	},
	{
		BT_HCI_OP_LE_SET_RANDOM_ADDRESS,
		sizeof(struct bt_hci_cp_le_set_random_address),
		generic_success,
	},
	{
		BT_HCI_OP_RESET,
		0,
		generic_success,
	},

	/* Extended advertising commands */
	{
		BT_HCI_OP_LE_SET_EXT_ADV_PARAM,
		sizeof(struct bt_hci_rp_le_set_ext_adv_param),
		le_set_ext_adv_param,
	},
	{
		BT_HCI_OP_LE_SET_ADV_SET_RANDOM_ADDR,
		sizeof(struct bt_hci_evt_cc_status),
		generic_success,
	},
	{
		BT_HCI_OP_LE_REMOVE_ADV_SET,
		sizeof(struct bt_hci_evt_cc_status),
		generic_success,
	},
	/* Legacy advertising commands */
	{
		BT_HCI_OP_LE_SET_ADV_PARAM,
		sizeof(struct bt_hci_evt_cc_status),
		generic_success,
	},
	{
		BT_HCI_OP_LE_SET_ADV_ENABLE,
		sizeof(struct bt_hci_evt_cc_status),
		generic_success,
	},
	{
		BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN,
		sizeof(struct bt_hci_rp_le_read_max_adv_data_len),
		generic_success,
	},
};

/* Loop over handlers to find and invoke the one matching opcode. */
static int cmd_handle_helper(uint16_t opcode, struct net_buf *cmd, struct net_buf **evt,
			     const struct cmd_handler *handlers, size_t num_handlers)
{
	for (size_t i = 0; i < num_handlers; i++) {
		const struct cmd_handler *handler = &handlers[i];

		if (handler->opcode != opcode) {
			continue;
		}

		if (handler->handler != NULL) {
			handler->handler(cmd, evt, handler->len, opcode);
			return 0;
		}
	}

	zassert_unreachable("opcode 0x%04X not handled", opcode);

	return -EINVAL;
}

static int cmd_handle(const struct device *dev, struct net_buf *cmd,
		      const struct cmd_handler *handlers, size_t num_handlers)
{
	struct net_buf *evt = NULL;
	struct bt_hci_evt_cc_status *ccst;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;
	int err;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	err = cmd_handle_helper(opcode, cmd, &evt, handlers, num_handlers);

	if (err == -EINVAL) {
		ccst = cmd_complete(&evt, sizeof(*ccst), opcode);
		ccst->status = BT_HCI_ERR_UNKNOWN_CMD;
	}

	if (evt != NULL) {
		bt_hci_recv(dev, evt);
	}

	return err;
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected buffer type %u", type);
	zassert_ok(cmd_handle(dev, buf, cmds, ARRAY_SIZE(cmds)), "Unknown HCI command");

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {};                                  \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

/* Sentinel used to verify that the user data is passed on to the callback. */
static int user_data_sentinel = 1234;

#define MAX_ADV_SETS COND_CODE_1(CONFIG_BT_EXT_ADV, (CONFIG_BT_EXT_ADV_MAX_ADV_SET), (1))

/* State captured by the iteration callbacks. */
static struct {
	size_t call_count;
	const struct bt_le_ext_adv *visited[MAX_ADV_SETS];
	bool unexpected_data;
	bool null_adv;
	void *received_data;
} cb_state;

static bool count_cb(const struct bt_le_ext_adv *adv, void *data)
{
	if (adv == NULL) {
		cb_state.null_adv = true;
	}

	if (data != &user_data_sentinel) {
		cb_state.unexpected_data = true;
	}

	cb_state.received_data = data;

	if (cb_state.call_count < ARRAY_SIZE(cb_state.visited)) {
		cb_state.visited[cb_state.call_count] = adv;
	}

	cb_state.call_count++;

	return true;
}

static bool stop_cb(const struct bt_le_ext_adv *adv, void *data)
{
	(void)count_cb(adv, data);

	return false;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)memset(&cb_state, 0, sizeof(cb_state));
}

static void *suite_setup(void)
{
	int err = bt_enable(NULL);

	zassert_true(err == 0 || err == -EALREADY, "bt_enable failed: %d", err);

	return NULL;
}

ZTEST_SUITE(bt_le_ext_adv_foreach, NULL, suite_setup, test_before, NULL, NULL);

/*
 * Passing a NULL callback is invalid and shall be rejected without touching any
 * advertising set.
 */
static ZTEST(bt_le_ext_adv_foreach, test_null_callback_returns_einval)
{
	int err;

	err = bt_le_ext_adv_foreach(NULL, &user_data_sentinel);

	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

/* A NULL callback is rejected regardless of the user data provided. */
static ZTEST(bt_le_ext_adv_foreach, test_null_callback_with_null_data_returns_einval)
{
	int err;

	err = bt_le_ext_adv_foreach(NULL, NULL);

	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

#if defined(CONFIG_BT_EXT_ADV)
/* The advertising sets created by the currently running test. */
static struct bt_le_ext_adv *created_advs[MAX_ADV_SETS];
static size_t created_adv_cnt;

static void expect_visited(struct bt_le_ext_adv *adv)
{
	for (size_t i = 0U; i < MIN(cb_state.call_count, ARRAY_SIZE(cb_state.visited)); i++) {
		if (cb_state.visited[i] == adv) {
			return;
		}
	}

	zassert_unreachable("Advertising set %p was not provided to the callback", adv);
}

static struct bt_le_ext_adv *create_adv_set(void)
{
	struct bt_le_ext_adv *adv = NULL;
	int err;

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);

	zassert_ok(err, "Failed to create advertising set (%d)", err);
	zassert_not_null(adv, "Advertising set is NULL");
	zassert_true(created_adv_cnt < ARRAY_SIZE(created_advs), "Too many advertising sets");

	created_advs[created_adv_cnt++] = adv;

	return adv;
}

static void delete_adv_set(struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set (%d)", err);

	for (size_t i = 0; i < created_adv_cnt; i++) {
		if (created_advs[i] == adv) {
			created_advs[i] = created_advs[created_adv_cnt - 1];
			created_adv_cnt--;
			break;
		}
	}
}

static void delete_all_adv_sets(void *fixture)
{
	ARG_UNUSED(fixture);

	while (created_adv_cnt > 0) {
		delete_adv_set(created_advs[created_adv_cnt - 1]);
	}
}

static void ext_adv_before(void *fixture)
{
	test_before(fixture);
}

ZTEST_SUITE(bt_le_ext_adv_foreach_ext, NULL, suite_setup, ext_adv_before, delete_all_adv_sets,
	    NULL);

/* Without any created advertising set the callback shall not be called. */
static ZTEST(bt_le_ext_adv_foreach_ext, test_no_adv_sets_returns_success)
{
	int err;

	err = bt_le_ext_adv_foreach(count_cb, &user_data_sentinel);

	zassert_ok(err, "Unexpected return value %d", err);
	zassert_equal(cb_state.call_count, 0, "Callback called %zu times", cb_state.call_count);
}

/* Every created advertising set shall be provided to the callback exactly once. */
static ZTEST(bt_le_ext_adv_foreach_ext, test_all_adv_sets_are_provided)
{
	struct bt_le_ext_adv *advs[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
	int err;

	ARRAY_FOR_EACH_PTR(advs, adv) {
		*adv = create_adv_set();
	}

	err = bt_le_ext_adv_foreach(count_cb, &user_data_sentinel);

	zassert_ok(err, "Unexpected return value %d", err);
	zassert_equal(cb_state.call_count, ARRAY_SIZE(advs), "Callback called %zu times",
		      cb_state.call_count);
	zassert_false(cb_state.null_adv, "Callback called with a NULL advertising set");
	zassert_false(cb_state.unexpected_data, "Callback called with unexpected user data");

	ARRAY_FOR_EACH_PTR(advs, adv) {
		expect_visited(*adv);
	}
}

/* Deleted advertising sets shall no longer be provided to the callback. */
static ZTEST(bt_le_ext_adv_foreach_ext, test_deleted_adv_set_is_not_provided)
{
	struct bt_le_ext_adv *first;
	struct bt_le_ext_adv *second;
	int err;

	first = create_adv_set();
	second = create_adv_set();

	delete_adv_set(first);

	err = bt_le_ext_adv_foreach(count_cb, &user_data_sentinel);

	zassert_ok(err, "Unexpected return value %d", err);
	zassert_equal(cb_state.call_count, 1, "Callback called %zu times", cb_state.call_count);
	expect_visited(second);
}

/*
 * A callback returning false shall stop the iteration and cause -ECANCELED to
 * be returned.
 */
static ZTEST(bt_le_ext_adv_foreach_ext, test_callback_stop_returns_ecanceled)
{
	int err;

	for (size_t i = 0; i < CONFIG_BT_EXT_ADV_MAX_ADV_SET; i++) {
		(void)create_adv_set();
	}

	err = bt_le_ext_adv_foreach(stop_cb, &user_data_sentinel);

	zassert_equal(err, -ECANCELED, "Unexpected return value %d", err);
	zassert_equal(cb_state.call_count, 1, "Callback called %zu times", cb_state.call_count);
}

/*
 * The user data pointer is optional and shall be forwarded to the callback
 * unmodified, including when it is NULL.
 */
static ZTEST(bt_le_ext_adv_foreach_ext, test_null_user_data_is_forwarded)
{
	int err;

	(void)create_adv_set();

	/* count_cb() flags any user data that is not the sentinel. */
	err = bt_le_ext_adv_foreach(count_cb, NULL);

	zassert_ok(err, "Unexpected return value %d", err);
	zassert_equal(cb_state.call_count, 1, "Callback called %zu times", cb_state.call_count);
	zassert_true(cb_state.unexpected_data, "Callback did not receive the NULL user data");
	zassert_equal(cb_state.received_data, NULL, "Callback received %p instead of NULL",
		      cb_state.received_data);
}
#else  /* !defined(CONFIG_BT_EXT_ADV) */

ZTEST_SUITE(bt_le_ext_adv_foreach_legacy, NULL, suite_setup, test_before, NULL, NULL);

static ZTEST(bt_le_ext_adv_foreach_legacy, test_adv_is_provided)
{
	int err;

	err = bt_le_ext_adv_foreach(count_cb, &user_data_sentinel);

	zassert_ok(err, "Unexpected return value %d", err);
	zassert_equal(cb_state.call_count, 1, "Callback called %zu times", cb_state.call_count);
	zassert_false(cb_state.null_adv, "Callback called with a NULL advertising set");
	zassert_false(cb_state.unexpected_data, "Callback called with unexpected user data");
}

/*
 * A callback returning false shall stop the iteration and cause -ECANCELED to
 * be returned.
 */
static ZTEST(bt_le_ext_adv_foreach_legacy, test_callback_stop_returns_ecanceled)
{
	int err;

	err = bt_le_ext_adv_foreach(stop_cb, &user_data_sentinel);

	zassert_equal(err, -ECANCELED, "Unexpected return value %d", err);
	zassert_equal(cb_state.call_count, 1, "Callback called %zu times", cb_state.call_count);
}

/*
 * The user data pointer is optional and shall be forwarded to the callback
 * unmodified, including when it is NULL.
 */
static ZTEST(bt_le_ext_adv_foreach_legacy, test_null_user_data_is_forwarded)
{
	int err;

	/* count_cb() flags any user data that is not the sentinel. */
	err = bt_le_ext_adv_foreach(count_cb, NULL);

	zassert_ok(err, "Unexpected return value %d", err);
	zassert_equal(cb_state.call_count, 1, "Callback called %zu times", cb_state.call_count);
	zassert_true(cb_state.unexpected_data, "Callback did not receive the NULL user data");
	zassert_equal(cb_state.received_data, NULL, "Callback received %p instead of NULL",
		      cb_state.received_data);
}
#endif /* defined(CONFIG_BT_EXT_ADV) */
