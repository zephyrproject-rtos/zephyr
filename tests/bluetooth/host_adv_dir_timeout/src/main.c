/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_adv_dir_timeout_test, CONFIG_BT_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Length of the command complete parameters used for commands without a dedicated handler.
 * The parameters are zeroed, and the size is large enough to hold the return parameters of any
 * command that the Host may send.
 */
#define UNHANDLED_CMD_RP_SIZE 64U

/* The test Controller reports the timeout of high duty cycle directed advertising with a
 * connection complete event. Without this quirk an LE Advertising Set Terminated event follows.
 */
#define CONN_COMPLETE_ONLY                                                                         \
	((BT_DT_HCI_QUIRKS_INST_GET(0) & BT_HCI_QUIRK_NO_ADV_TERM_ON_DIR_TIMEOUT) != 0)

/* Advertising handle of the most recent LE Set Extended Advertising Parameters command. */
static uint8_t last_adv_handle;

/* Error of the most recent connected callback. */
static uint8_t conn_err;

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(sent_sem, 0, 1);

/* Command handler structure for cmd_handle(). */
struct cmd_handler {
	uint16_t opcode; /* HCI command opcode */
	uint8_t len;     /* HCI command response length */
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

/* Add event to net_buf. */
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

	ARG_UNUSED(buf);

	ccst = cmd_complete(evt, len, opcode);

	/* Fill any event parameters with zero */
	(void)memset(ccst, 0, len);

	ccst->status = BT_HCI_ERR_SUCCESS;
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
	/* LE supported, and BR/EDR not supported */
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
	/* Only report support for LE Extended Advertising */
	rp->features[BT_LE_FEAT_BIT_EXT_ADV / 8] = BIT(BT_LE_FEAT_BIT_EXT_ADV % 8);
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

static void le_read_max_adv_data_len(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				     uint16_t opcode)
{
	struct bt_hci_rp_le_read_max_adv_data_len *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->max_adv_data_len = sys_cpu_to_le16(BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN);
}

/* Handler for BT_HCI_OP_LE_SET_EXT_ADV_PARAM. It stores the advertising handle, which the
 * LE Advertising Set Terminated event of the test Controller has to carry.
 */
static void le_set_ext_adv_param(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				 uint16_t opcode)
{
	const struct bt_hci_cp_le_set_ext_adv_param *cp = (void *)buf->data;
	struct bt_hci_rp_le_set_ext_adv_param *rp;

	ARG_UNUSED(len);

	zassert_true(buf->len >= sizeof(*cp), "Unexpected command length %u", buf->len);

	last_adv_handle = cp->handle;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->tx_power = 0;
}

/* Handlers for the commands where the Host requires specific return parameters. Any other
 * command is accepted by cmd_handle() with zeroed return parameters.
 */
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
		BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN,
		sizeof(struct bt_hci_rp_le_read_max_adv_data_len),
		le_read_max_adv_data_len,
	},
	{
		BT_HCI_OP_LE_SET_EXT_ADV_PARAM,
		sizeof(struct bt_hci_rp_le_set_ext_adv_param),
		le_set_ext_adv_param,
	},
};

/* Lookup the command opcode and invoke the handler. */
static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	ARRAY_FOR_EACH_PTR(cmds, handler) {
		if (handler->opcode == opcode) {
			handler->handler(cmd, &evt, handler->len, opcode);
			bt_hci_recv(dev, evt);

			return;
		}
	}

	LOG_DBG("Unhandled opcode 0x%04X", opcode);
	generic_success(cmd, &evt, UNHANDLED_CMD_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
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

	cmd_handle(dev, buf);

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void *le_meta_event(struct net_buf **buf, uint8_t subevent, uint8_t len)
{
	struct bt_hci_evt_le_meta_event *meta;

	*buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + len);
	meta = net_buf_add(*buf, sizeof(*meta));
	meta->subevent = subevent;

	return net_buf_add(*buf, len);
}

/* Let the test Controller report that no connection was created before the duration expired. */
static void conn_complete_timeout(void)
{
	struct bt_hci_evt_le_enh_conn_complete *cc;
	struct net_buf *buf;

	cc = le_meta_event(&buf, BT_HCI_EVT_LE_ENH_CONN_COMPLETE, sizeof(*cc));
	(void)memset(cc, 0, sizeof(*cc));
	cc->status = BT_HCI_ERR_ADV_TIMEOUT;
	cc->role = BT_HCI_ROLE_PERIPHERAL;
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

/* Let the test Controller report the timeout with the events that it sends, and check that the
 * Host ends the advertising when the event that it has to act on arrives.
 */
static void adv_timeout(void)
{
	int err;

	conn_complete_timeout();

	if (!CONN_COMPLETE_ONLY) {
		struct bt_hci_evt_le_adv_set_terminated *term;
		struct net_buf *buf;

		err = k_sem_take(&connected_sem, K_MSEC(100));
		zassert_equal(err, -EAGAIN, "Advertising ended by the connection complete event");

		term = le_meta_event(&buf, BT_HCI_EVT_LE_ADV_SET_TERMINATED, sizeof(*term));
		(void)memset(term, 0, sizeof(*term));
		term->status = BT_HCI_ERR_ADV_TIMEOUT;
		term->adv_handle = last_adv_handle;
		bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
	}

	err = k_sem_take(&connected_sem, K_SECONDS(1));
	zassert_ok(err, "No connected callback");
	zassert_equal(conn_err, BT_HCI_ERR_ADV_TIMEOUT, "Unexpected error 0x%02x", conn_err);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	conn_err = err;
	k_sem_give(&connected_sem);
}

static void adv_sent(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info)
{
	ARG_UNUSED(adv);
	ARG_UNUSED(info);

	k_sem_give(&sent_sem);
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.sent = adv_sent,
};

static void *adv_dir_timeout_setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
	};
	int err;

	err = bt_enable(NULL);
	zassert_ok(err, "Bluetooth init failed: %d", err);

	err = bt_conn_cb_register(&conn_cb);
	zassert_ok(err, "Failed to register connection callbacks: %d", err);

	return NULL;
}

ZTEST_SUITE(bt_adv_dir_timeout, NULL, adv_dir_timeout_setup, NULL, NULL, NULL);

static const bt_addr_le_t peer = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x01, 0x02, 0x03, 0x04, 0x05, 0xc6 } },
};

static const bt_addr_le_t other_peer = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x06, 0x05, 0x04, 0x03, 0x02, 0xc1 } },
};

static ZTEST(bt_adv_dir_timeout, test_ext_adv_start)
{
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&peer), &adv_cb, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);

	/* The second round fails to start if the first timeout left the set enabled */
	for (size_t i = 0U; i < 2U; i++) {
		struct bt_le_ext_adv_info info;

		err = bt_le_ext_adv_start(
			adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
		zassert_ok(err, "Failed to start advertising: %d", err);

		adv_timeout();

		err = k_sem_take(&sent_sem, K_SECONDS(1));
		zassert_ok(err, "No sent callback");

		err = bt_le_ext_adv_get_info(adv, &info);
		zassert_ok(err, "Failed to get advertising set info: %d", err);
		zassert_equal(info.ext_adv_state, BT_LE_EXT_ADV_STATE_DISABLED,
			      "Advertising set still enabled");
	}

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}

static ZTEST(bt_adv_dir_timeout, test_adv_start)
{
	/* The second round fails to start if the first timeout left the advertiser enabled */
	for (size_t i = 0U; i < 2U; i++) {
		int err;

		err = bt_le_adv_start(BT_LE_ADV_CONN_DIR(&peer), NULL, 0, NULL, 0);
		zassert_ok(err, "Failed to start advertising: %d", err);

		adv_timeout();
	}
}

/* A connection complete event for the timeout that arrives after the application stopped the
 * set has no effect, on that set or on its next start.
 */
static ZTEST(bt_adv_dir_timeout, test_ext_adv_stopped)
{
	struct bt_le_ext_adv_info info;
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&peer), &adv_cb, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);

	err = bt_le_ext_adv_start(
		adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_ok(err, "Failed to start advertising: %d", err);

	err = bt_le_ext_adv_stop(adv);
	zassert_ok(err, "Failed to stop advertising: %d", err);

	conn_complete_timeout();

	err = k_sem_take(&connected_sem, K_MSEC(100));
	zassert_equal(err, -EAGAIN, "Connected callback for a stopped set");
	err = k_sem_take(&sent_sem, K_NO_WAIT);
	zassert_equal(err, -EBUSY, "Sent callback for a stopped set");

	err = bt_le_ext_adv_start(
		adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_ok(err, "Failed to restart advertising: %d", err);

	err = bt_le_ext_adv_get_info(adv, &info);
	zassert_ok(err, "Failed to get advertising set info: %d", err);
	zassert_equal(info.ext_adv_state, BT_LE_EXT_ADV_STATE_ENABLED, "Advertising set disabled");

	err = bt_le_ext_adv_stop(adv);
	zassert_ok(err, "Failed to stop advertising: %d", err);

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}

/* The timeout of the high duty cycle set must leave a low duty cycle directed set enabled. */
static ZTEST(bt_adv_dir_timeout, test_ext_adv_low_duty_unaffected)
{
	struct bt_le_ext_adv *low_duty;
	struct bt_le_ext_adv_info info;
	struct bt_le_ext_adv *adv;
	int err;

	if (!CONN_COMPLETE_ONLY || CONFIG_BT_EXT_ADV_MAX_ADV_SET < 2) {
		ztest_test_skip();
	}

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR_LOW_DUTY(&other_peer), NULL, &low_duty);
	zassert_ok(err, "Failed to create low duty cycle set: %d", err);

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&peer), &adv_cb, &adv);
	zassert_ok(err, "Failed to create high duty cycle set: %d", err);

	err = bt_le_ext_adv_start(low_duty, BT_LE_EXT_ADV_START_DEFAULT);
	zassert_ok(err, "Failed to start low duty cycle advertising: %d", err);

	err = bt_le_ext_adv_start(
		adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_ok(err, "Failed to start high duty cycle advertising: %d", err);

	adv_timeout();

	err = k_sem_take(&sent_sem, K_SECONDS(1));
	zassert_ok(err, "No sent callback");

	err = bt_le_ext_adv_get_info(adv, &info);
	zassert_ok(err, "Failed to get advertising set info: %d", err);
	zassert_equal(info.ext_adv_state, BT_LE_EXT_ADV_STATE_DISABLED,
		      "High duty cycle set still enabled");

	err = bt_le_ext_adv_get_info(low_duty, &info);
	zassert_ok(err, "Failed to get advertising set info: %d", err);
	zassert_equal(info.ext_adv_state, BT_LE_EXT_ADV_STATE_ENABLED,
		      "Low duty cycle set disabled");

	err = bt_le_ext_adv_stop(low_duty);
	zassert_ok(err, "Failed to stop advertising: %d", err);

	err = bt_le_ext_adv_delete(low_duty);
	zassert_ok(err, "Failed to delete advertising set: %d", err);

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}

/* The connection complete event names no advertising set, so with the quirk only one high duty
 * cycle directed set can be enabled.
 */
static ZTEST(bt_adv_dir_timeout, test_ext_adv_second_high_duty)
{
	struct bt_le_ext_adv *second;
	struct bt_le_ext_adv *adv;
	int err;

	if (!CONN_COMPLETE_ONLY || CONFIG_BT_EXT_ADV_MAX_ADV_SET < 2) {
		ztest_test_skip();
	}

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&peer), &adv_cb, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);

	err = bt_le_ext_adv_start(
		adv, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_ok(err, "Failed to start advertising: %d", err);

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_DIR(&other_peer), NULL, &second);
	zassert_ok(err, "Failed to create second set: %d", err);

	err = bt_le_ext_adv_start(
		second, BT_LE_EXT_ADV_START_PARAM(BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT, 0));
	zassert_equal(err, -EBUSY, "Second high duty cycle set started: %d", err);

	err = bt_le_ext_adv_delete(second);
	zassert_ok(err, "Failed to delete second set: %d", err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_DIR(&other_peer), NULL, 0, NULL, 0);
	zassert_equal(err, -EBUSY, "Legacy high duty cycle advertising started: %d", err);

	adv_timeout();

	err = k_sem_take(&sent_sem, K_SECONDS(1));
	zassert_ok(err, "No sent callback");

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}
