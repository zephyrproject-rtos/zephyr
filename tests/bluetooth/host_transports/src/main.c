/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* A CONFIG_BT_CLASSIC Host on a BR/EDR-only, an LE-only and a dual-mode Controller */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <fake_hci.h>

/* Read Local Supported Features, octet 4 */
#define FEAT_BREDR_NOT_SUPPORTED BIT(5)
#define FEAT_LE_SUPPORTED        BIT(6)

/* Read Local Supported Features, octet 7 */
#define FEAT_EXT_FEATURES BIT(7)

/* Read Local Extended Features page 2, octet 1: Secure Connections (Controller Support) */
#define FEAT_P2_SC BIT(0)

/* LE Read Local Supported Features, octet 0: LE Encryption */
#define LE_FEAT_ENC BIT(0)

#define BREDR_ACL_LEN 1021U
#define BREDR_ACL_NUM 4U

#define MAX_CMDS 128U

static const bt_addr_t public_addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };

/* The commands the Controller has received since the last record_clear() */
static uint16_t cmds[MAX_CMDS];
static size_t cmd_count;
static uint64_t evt_mask;

/* The highest extended features page the Controller reports */
static uint8_t ext_max_page;

static void read_buffer_size_rsp(uint16_t opcode)
{
	struct bt_hci_rp_read_buffer_size *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(BREDR_ACL_LEN);
	rp->sco_max_len = 0U;
	rp->acl_max_num = sys_cpu_to_le16(BREDR_ACL_NUM);
	rp->sco_max_num = 0U;

	fake_hci_recv(buf);
}

static void read_ext_features_rsp(uint16_t opcode, uint8_t page)
{
	struct bt_hci_rp_read_local_ext_features *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->page = page;
	rp->max_page = ext_max_page;

	if (page == 2U) {
		rp->ext_features[1] = FEAT_P2_SC;
	}

	fake_hci_recv(buf);
}

static bool cmd_cb(uint16_t opcode, struct net_buf *buf)
{
	if (cmd_count < MAX_CMDS) {
		cmds[cmd_count] = opcode;
	}

	cmd_count++;

	if (opcode == BT_HCI_OP_SET_EVENT_MASK) {
		evt_mask = sys_get_le64(buf->data);
	}

	/* The default response would report no BR/EDR buffers */
	if (opcode == BT_HCI_OP_READ_BUFFER_SIZE) {
		read_buffer_size_rsp(opcode);
		return true;
	}

	if (opcode == BT_HCI_OP_READ_LOCAL_EXT_FEATURES) {
		read_ext_features_rsp(opcode, buf->data[0]);
		return true;
	}

	return false;
}

static const struct fake_hci_cb cb = {
	.cmd = cmd_cb,
};

static void record_clear(void)
{
	cmd_count = 0U;
	evt_mask = 0U;
}

static void ctlr_set(bool bredr, bool le)
{
	(void)memset(&fake_hci, 0, sizeof(fake_hci));
	ext_max_page = 0U;
	bt_addr_copy(&fake_hci.bd_addr, &public_addr);
	fake_hci.features[4] =
		(bredr ? 0U : FEAT_BREDR_NOT_SUPPORTED) | (le ? FEAT_LE_SUPPORTED : 0U);

	if (le) {
		fake_hci.le_features[0] = LE_FEAT_ENC;
		fake_hci.le_acl_len = BT_GAP_DATA_LEN_DEFAULT;
		fake_hci.le_acl_num = 1U;
	}
}

static bool is_le_cmd(uint16_t opcode)
{
	return BT_OGF(opcode) == BT_OGF_LE;
}

/* The commands that only a Controller with BR/EDR supports */
static bool is_bredr_cmd(uint16_t opcode)
{
	switch (opcode) {
	case BT_HCI_OP_DISCONNECT:
	case BT_HCI_OP_READ_REMOTE_VERSION_INFO:
		return false;
	case BT_HCI_OP_WRITE_LOCAL_NAME:
	case BT_HCI_OP_WRITE_PAGE_TIMEOUT:
	case BT_HCI_OP_WRITE_SCAN_ENABLE:
	case BT_HCI_OP_WRITE_CLASS_OF_DEVICE:
	case BT_HCI_OP_WRITE_INQUIRY_MODE:
	case BT_HCI_OP_WRITE_SSP_MODE:
	case BT_HCI_OP_LE_WRITE_LE_HOST_SUPP:
	case BT_HCI_OP_WRITE_SC_HOST_SUPP:
		return true;
	default:
		return (BT_OGF(opcode) == BT_OGF_LINK_CTRL) ||
		       (BT_OGF(opcode) == BT_OGF_LINK_POLICY);
	}
}

static void record_check(void)
{
	zassert_true(cmd_count <= MAX_CMDS, "%zu commands, recorded only %u", cmd_count, MAX_CMDS);
}

static bool any_sent(bool (*match)(uint16_t opcode))
{
	record_check();

	for (size_t i = 0U; i < cmd_count; i++) {
		if (match(cmds[i])) {
			return true;
		}
	}

	return false;
}

static bool sent(uint16_t opcode)
{
	record_check();

	for (size_t i = 0U; i < cmd_count; i++) {
		if (cmds[i] == opcode) {
			return true;
		}
	}

	return false;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	record_clear();
	fake_hci_cb_set(&cb);
}

static void after(void *fixture)
{
	int err;

	ARG_UNUSED(fixture);

	/* -EALREADY when the test failed before enabling */
	err = bt_disable();
	zassert_true((err == 0) || (err == -EALREADY), "bt_disable() failed (err %d)", err);
	fake_hci_cb_set(NULL);
}

ZTEST_SUITE(host_transports, NULL, NULL, before, after, NULL);

/* LE APIs fail with -ENOTSUP and nothing LE reaches the Controller, while BR/EDR works */
static ZTEST(host_transports, test_bredr_only)
{
	ctlr_set(true, false);

	zassert_equal(bt_enable(NULL), 0, "bt_enable() failed");
	zassert_false(any_sent(is_le_cmd), "LE command sent during initialization");
	zassert_equal(evt_mask & BT_EVT_MASK_LE_META_EVENT, 0U, "LE Meta event enabled");
	zassert_not_equal(evt_mask & BT_EVT_MASK_ENCRYPT_CHANGE, 0U,
			  "Encryption Change event not enabled");
	zassert_not_equal(evt_mask & BT_EVT_MASK_INQUIRY_COMPLETE, 0U,
			  "Inquiry Complete event not enabled");

	zassert_equal(bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0U, NULL, 0U), -ENOTSUP);
	zassert_equal(bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL), -ENOTSUP);
	zassert_equal(bt_hci_cmd_send_sync(BT_HCI_OP_LE_RAND, NULL, NULL), -ENOTSUP);
	zassert_false(any_sent(is_le_cmd), "LE command sent by an API");

	zassert_equal(bt_br_set_connectable(true, NULL), 0, "BR/EDR connectable failed");
	zassert_true(sent(BT_HCI_OP_WRITE_SCAN_ENABLE), "Write Scan Enable not sent");
}

/* BR/EDR APIs fail with -ENOTSUP and nothing BR/EDR reaches the Controller, while LE works */
static ZTEST(host_transports, test_le_only)
{
	ctlr_set(false, true);

	zassert_equal(bt_enable(NULL), 0, "bt_enable() failed");
	zassert_false(any_sent(is_bredr_cmd), "BR/EDR command sent during initialization");
	zassert_not_equal(evt_mask & BT_EVT_MASK_LE_META_EVENT, 0U, "LE Meta event not enabled");
	zassert_equal(evt_mask & BT_EVT_MASK_INQUIRY_COMPLETE, 0U, "Inquiry Complete enabled");

	zassert_equal(bt_br_set_connectable(true, NULL), -ENOTSUP);
	zassert_equal(bt_br_set_class_of_device(0x200404U), -ENOTSUP);
	zassert_false(any_sent(is_bredr_cmd), "BR/EDR command sent by an API");

	zassert_equal(bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL), 0, "LE scanning failed");
	zassert_equal(bt_le_scan_stop(), 0, "Stopping LE scanning failed");
}

static ZTEST(host_transports, test_dual_mode)
{
	ctlr_set(true, true);

	zassert_equal(bt_enable(NULL), 0, "bt_enable() failed");
	zassert_true(sent(BT_HCI_OP_WRITE_SSP_MODE), "BR/EDR not initialized");
	zassert_true(sent(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP), "LE Host support not enabled");
	zassert_not_equal(evt_mask & BT_EVT_MASK_LE_META_EVENT, 0U, "LE Meta event not enabled");
	zassert_not_equal(evt_mask & BT_EVT_MASK_INQUIRY_COMPLETE, 0U,
			  "Inquiry Complete event not enabled");

	zassert_equal(bt_br_set_connectable(true, NULL), 0, "BR/EDR connectable failed");
	zassert_equal(bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL), 0, "LE scanning failed");
	zassert_equal(bt_le_scan_stop(), 0, "Stopping LE scanning failed");
}

/* What an LE Controller reported does not carry over to a BR/EDR-only one */
static ZTEST(host_transports, test_le_then_bredr_only)
{
	static const uint8_t no_features[BT_LE_LOCAL_SUPPORTED_FEATURES_SIZE];
	struct bt_le_local_features local;

	ctlr_set(false, true);
	zassert_equal(bt_enable(NULL), 0, "bt_enable() failed on the LE Controller");
	zassert_equal(bt_le_get_local_features(&local), 0);
	zassert_not_equal(memcmp(local.features, no_features, sizeof(no_features)), 0,
			  "No LE features from the LE Controller");
	zassert_not_equal(local.acl_mtu, 0U, "No LE ACL MTU from the LE Controller");
	zassert_not_equal(local.acl_pkts, 0U, "No LE ACL buffers from the LE Controller");
	zassert_equal(bt_disable(), 0, "bt_disable() failed");

	ctlr_set(true, false);
	record_clear();
	zassert_equal(bt_enable(NULL), 0, "bt_enable() failed on the BR/EDR Controller");
	zassert_false(any_sent(is_le_cmd), "LE command sent during initialization");

	zassert_equal(bt_le_get_local_features(&local), 0);
	zassert_equal(memcmp(local.features, no_features, sizeof(no_features)), 0,
		      "LE features kept from the previous Controller");
	zassert_equal(local.states, 0U, "LE states kept from the previous Controller");
	zassert_equal(local.acl_mtu, 0U, "LE ACL MTU kept from the previous Controller");
	zassert_equal(local.acl_pkts, 0U, "LE ACL buffers kept from the previous Controller");
	zassert_equal(local.max_adv_data_len, 0U,
		      "Advertising data length kept from the previous Controller");
}

/* The extended features of a previous Controller do not carry over */
static ZTEST(host_transports, test_sc_then_no_page_2)
{
	ctlr_set(true, false);
	fake_hci.features[7] = FEAT_EXT_FEATURES;
	ext_max_page = 2U;
	zassert_equal(bt_enable(NULL), 0, "bt_enable() failed on the first Controller");
	zassert_true(sent(BT_HCI_OP_WRITE_SC_HOST_SUPP), "Secure Connections not enabled");
	zassert_equal(bt_disable(), 0, "bt_disable() failed");

	ctlr_set(true, false);
	fake_hci.features[7] = FEAT_EXT_FEATURES;
	ext_max_page = 1U;
	record_clear();
	zassert_equal(bt_enable(NULL), 0, "bt_enable() failed on the second Controller");
	zassert_false(sent(BT_HCI_OP_WRITE_SC_HOST_SUPP),
		      "Secure Connections enabled on a Controller without page 2");
}
