/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* The Host sets CONFIG_BT_BR_MIN_ENC_KEY_SIZE in a Controller that supports HCI Set Min Encryption
 * Key Size, and checks the negotiated size in any case.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
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

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of the commands that get zeroes */
#define GENERIC_RP_SIZE 64U

#define CONN_HANDLE 0x0001U
#define ACL_PKTS    4U

#define KEY_SIZE_SHORT (CONFIG_BT_BR_MIN_ENC_KEY_SIZE - 1U)

/* The minimum in the Controller until the Host sets it */
#define CTLR_DEFAULT_MIN_KEY_SIZE BT_HCI_ENCRYPTION_KEY_SIZE_MIN

#define HCI_NO_RESET ((BT_DT_HCI_QUIRKS_INST_GET(0) & BT_HCI_QUIRK_NO_RESET) != 0U)

#define EVENT_TIMEOUT K_SECONDS(1)
/* Long enough for the Host to act on an event when it should not do anything */
#define SETTLE        K_MSEC(50)

static const bt_addr_t peer = {
	.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U },
};

/* What the Controller claims and answers */
static bool cmd_supported;
static uint8_t cmd_status_rsp;

/* The key size negotiation of the Link Managers */
static uint8_t ctlr_min_key_size;
static uint8_t peer_max_key_size;

/* What the Host sent. HCI Reset restores the Controller's default minimum, so only what
 * came after the last one counts.
 */
static uint32_t reset_count;
static uint32_t cmd_count;
static uint8_t cmd_value;
static uint8_t disconnect_reason;

static K_SEM_DEFINE(disconnect_sem, 0, 1);
static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(security_changed_sem, 0, 1);

/* What the application learned from the last security_changed() */
static bt_security_t security_level;
static enum bt_security_err security_err;

/* The connection of the test, also for what a failed test left */
static struct bt_conn *test_conn;

static void evt_send(struct net_buf *buf)
{
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

static struct net_buf *evt_create(uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);
	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;

	return buf;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = evt_create(BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void cmd_status(uint16_t opcode)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = BT_HCI_ERR_SUCCESS;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
	evt_send(buf);
}

static void generic_complete(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cc_status *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, GENERIC_RP_SIZE, opcode);
	(void)memset(rp, 0, GENERIC_RP_SIZE);
	rp->status = status;
	evt_send(buf);
}

static void read_local_features(uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE and BR/EDR supported */
	rp->features[4] = BIT(6);
	evt_send(buf);
}

static void read_supported_commands(uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
	if (!cmd_supported) {
		/* Core 6.3 Vol 4 Part E 6.27 */
		rp->commands[45] &= (uint8_t)~BIT(7);
	}
	evt_send(buf);
}

static void read_bd_addr(uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
	evt_send(buf);
}

static void read_buffer_size(uint16_t opcode)
{
	struct bt_hci_rp_read_buffer_size *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(255U);
	rp->acl_max_num = sys_cpu_to_le16(ACL_PKTS);
	evt_send(buf);
}

static void read_key_size(uint16_t opcode)
{
	struct bt_hci_rp_read_encryption_key_size *rp;
	struct net_buf *buf;

	rp = cmd_complete(&buf, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->handle = sys_cpu_to_le16(CONN_HANDLE);
	rp->key_size = peer_max_key_size;
	evt_send(buf);
}

static void encrypt_change(uint8_t status, uint8_t encrypt)
{
	struct bt_hci_evt_encrypt_change *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_ENCRYPT_CHANGE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt->encrypt = encrypt;
	evt_send(buf);
}

static void set_min_key_size(const struct bt_hci_cp_set_min_encryption_key_size *cp,
			     uint16_t opcode)
{
	cmd_value = cp->min_encryption_key_size;
	cmd_count++;
	if (cmd_status_rsp == BT_HCI_ERR_SUCCESS) {
		ctlr_min_key_size = cp->min_encryption_key_size;
	}

	generic_complete(opcode, cmd_status_rsp);
}

/* Core 6.3 Vol 2 Part C 4.2.5.2 gives the LMP error. Vol 4 Part E 7.7.8 only asks for a non-zero
 * status, so the status on HCI is an assumption.
 */
static void set_conn_encrypt(uint16_t opcode)
{
	cmd_status(opcode);

	if (peer_max_key_size < ctlr_min_key_size) {
		encrypt_change(BT_HCI_ERR_UNSUPP_LL_PARAM_VAL, BT_HCI_ENCRYPTION_OFF);
	} else {
		encrypt_change(BT_HCI_ERR_SUCCESS, BT_HCI_ENCRYPTION_ON_BR_E0);
	}
}

static void auth_complete(void)
{
	struct bt_hci_evt_auth_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_AUTH_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	evt_send(buf);
}

static void conn_complete(void)
{
	struct bt_hci_evt_conn_complete *evt;
	struct net_buf *buf;

	buf = evt_create(BT_HCI_EVT_CONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CONN_HANDLE);
	bt_addr_copy(&evt->bdaddr, &peer);
	evt->link_type = BT_HCI_ACL;
	evt_send(buf);
}

static void cmd_handle(struct net_buf *buf)
{
	const struct bt_hci_cmd_hdr *hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	uint16_t opcode = sys_le16_to_cpu(hdr->opcode);

	switch (opcode) {
	case BT_HCI_OP_RESET:
		reset_count++;
		cmd_count = 0U;
		ctlr_min_key_size = CTLR_DEFAULT_MIN_KEY_SIZE;
		generic_complete(opcode, BT_HCI_ERR_SUCCESS);
		break;
	case BT_HCI_OP_READ_LOCAL_FEATURES:
		read_local_features(opcode);
		break;
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS:
		read_supported_commands(opcode);
		break;
	case BT_HCI_OP_READ_BD_ADDR:
		read_bd_addr(opcode);
		break;
	case BT_HCI_OP_READ_BUFFER_SIZE:
		read_buffer_size(opcode);
		break;
	case BT_HCI_OP_SET_MIN_ENCRYPTION_KEY_SIZE:
		set_min_key_size((const void *)buf->data, opcode);
		break;
	case BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE:
		read_key_size(opcode);
		break;
	case BT_HCI_OP_CONNECT:
	case BT_HCI_OP_READ_REMOTE_FEATURES:
		cmd_status(opcode);
		break;
	case BT_HCI_OP_AUTH_REQUESTED:
		cmd_status(opcode);
		auth_complete();
		break;
	case BT_HCI_OP_SET_CONN_ENCRYPT:
		set_conn_encrypt(opcode);
		break;
	case BT_HCI_OP_DISCONNECT:
		disconnect_reason = ((struct bt_hci_cp_disconnect *)buf->data)->reason;
		cmd_status(opcode);
		k_sem_give(&disconnect_sem);
		break;
	case BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS:
		/* No event for this one */
		break;
	default:
		generic_complete(opcode, BT_HCI_ERR_SUCCESS);
		break;
	}
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
	ARG_UNUSED(dev);

	if (net_buf_pull_u8(buf) == BT_HCI_H4_CMD) {
		cmd_handle(buf);
	}

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

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(err);

	k_sem_give(&connected_sem);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	ARG_UNUSED(conn);

	security_level = level;
	security_err = err;
	k_sem_give(&security_changed_sem);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.security_changed = security_changed,
};

/* The link up, and the Host starting security on it. The rest is up to the Controller. */
static void link_secure(void)
{
	test_conn = bt_conn_create_br(&peer, BT_BR_CONN_PARAM_DEFAULT);
	zassert_not_null(test_conn, "Creating the connection failed");

	conn_complete();
	zassert_ok(k_sem_take(&connected_sem, EVENT_TIMEOUT), "No connected callback");

	zassert_ok(bt_conn_set_security(test_conn, BT_SECURITY_L2), "Starting security failed");
}

static void expect_host_refused(void)
{
	zassert_ok(k_sem_take(&disconnect_sem, EVENT_TIMEOUT), "The link was not disconnected");
	zassert_equal(disconnect_reason, BT_HCI_ERR_AUTH_FAIL, "Reason 0x%02x", disconnect_reason);
}

static void expect_security_changed(bt_security_t level, enum bt_security_err err)
{
	zassert_ok(k_sem_take(&security_changed_sem, EVENT_TIMEOUT),
		   "No security_changed callback");
	zassert_equal(security_err, err, "Error %d", security_err);
	zassert_equal(security_level, level, "Level %d", security_level);
	zassert_equal(k_sem_take(&disconnect_sem, SETTLE), -EAGAIN, "The link was disconnected");
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	cmd_supported = true;
	cmd_status_rsp = BT_HCI_ERR_SUCCESS;
	ctlr_min_key_size = CTLR_DEFAULT_MIN_KEY_SIZE;
	peer_max_key_size = KEY_SIZE_SHORT;
	reset_count = 0U;
	cmd_count = 0U;
	cmd_value = 0U;
	disconnect_reason = BT_HCI_ERR_SUCCESS;
	security_level = BT_SECURITY_L0;
	security_err = BT_SECURITY_ERR_SUCCESS;
	k_sem_reset(&disconnect_sem);
	k_sem_reset(&connected_sem);
	k_sem_reset(&security_changed_sem);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (bt_is_ready()) {
		zassert_ok(bt_disable(), "Bluetooth disable failed");
	}

	bt_conn_drop(&test_conn);
}

ZTEST_SUITE(host_br_min_enc_key_size, NULL, NULL, before, after, NULL);

static ZTEST(host_br_min_enc_key_size, test_sent_with_configured_value)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	zassert_equal(cmd_count, 1U, "Sent %u times", cmd_count);
	zassert_equal(cmd_value, CONFIG_BT_BR_MIN_ENC_KEY_SIZE, "Sent %u", cmd_value);
}

static ZTEST(host_br_min_enc_key_size, test_not_sent_when_unsupported)
{
	cmd_supported = false;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	zassert_equal(cmd_count, 0U, "Sent %u times", cmd_count);
}

static ZTEST(host_br_min_enc_key_size, test_refused_value_is_not_fatal)
{
	cmd_status_rsp = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	zassert_equal(cmd_count, 1U, "Sent %u times", cmd_count);
}

static ZTEST(host_br_min_enc_key_size, test_sent_again_after_disable)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_equal(cmd_count, 1U, "Sent %u times", cmd_count);
	zassert_ok(bt_disable(), "Bluetooth disable failed");

	cmd_count = 0U;
	cmd_value = 0U;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed again");

	zassert_equal(cmd_count, 1U, "Sent %u times", cmd_count);
	zassert_equal(cmd_value, CONFIG_BT_BR_MIN_ENC_KEY_SIZE, "Sent %u", cmd_value);
	zassert_equal(reset_count != 0U, !HCI_NO_RESET, "%u times HCI Reset", reset_count);
}

static ZTEST(host_br_min_enc_key_size, test_short_key_not_negotiated)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	link_secure();

	expect_security_changed(BT_SECURITY_L1, BT_SECURITY_ERR_UNSPECIFIED);
}

static ZTEST(host_br_min_enc_key_size, test_key_at_minimum_accepted)
{
	peer_max_key_size = CONFIG_BT_BR_MIN_ENC_KEY_SIZE;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	link_secure();

	expect_security_changed(BT_SECURITY_L2, BT_SECURITY_ERR_SUCCESS);
}

static ZTEST(host_br_min_enc_key_size, test_host_refuses_when_unsupported)
{
	cmd_supported = false;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_equal(cmd_count, 0U, "Sent %u times", cmd_count);

	link_secure();

	expect_host_refused();
}

static ZTEST(host_br_min_enc_key_size, test_host_refuses_after_refused_value)
{
	cmd_status_rsp = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_equal(cmd_count, 1U, "Sent %u times", cmd_count);

	link_secure();

	expect_host_refused();
}
