/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <argparse.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "bs_macro.h"
#include "bs_sync.h"

#include <testlib/adv.h>
#include <testlib/att_read.h>
#include <testlib/att_write.h>
#include <testlib/conn.h>
#include <testlib/enable_quiet.h>
#include <testlib/log_utils.h>
#include <testlib/scan.h>
#include <testlib/security.h>

/* This test uses system asserts to fail tests. */
BUILD_ASSERT(__ASSERT_ON);

#define CENTRAL_DEVICE_NBR    0
#define PERIPHERAL_DEVICE_NBR 1

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define UUID_1                                                                                     \
	BT_UUID_DECLARE_128(0xdb, 0x1f, 0xe2, 0x52, 0xf3, 0xc6, 0x43, 0x66, 0xb3, 0x92, 0x5d,      \
			    0xc6, 0xe7, 0xc9, 0x59, 0x9d)

#define UUID_2                                                                                     \
	BT_UUID_DECLARE_128(0x3f, 0xa4, 0x7f, 0x44, 0x2e, 0x2a, 0x43, 0x05, 0xab, 0x38, 0x07,      \
			    0x8d, 0x16, 0xbf, 0x99, 0xf1)

#define UUID_3                                                                                     \
	BT_UUID_DECLARE_128(0x06, 0x30, 0xbb, 0xae, 0xff, 0x9a, 0x4e, 0x83, 0xa6, 0x5c, 0xf0,      \
			    0x4e, 0xdf, 0xb8, 0x79, 0x1d)

static ssize_t read_mtu_validation_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t buf_len, uint16_t offset)
{
	return 0;
}

static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(UUID_1),
	BT_GATT_CHARACTERISTIC(UUID_2, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
			       read_mtu_validation_chrc, NULL, NULL),
	BT_GATT_CHARACTERISTIC(UUID_3, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_AUTHEN,
			       read_mtu_validation_chrc, NULL, NULL),
};

static struct bt_gatt_service sample_svc_requiring_encryption = {
	.attrs = attrs,
	.attr_count = ARRAY_SIZE(attrs),
};

static void bs_sync_all_log(char *log_msg)
{
	/* Everyone meets here. */
	bt_testlib_bs_sync_all();

	if (get_device_nbr() == 0) {
		LOG_WRN("Sync point: %s", log_msg);
	}

	/* Everyone waits for d0 to finish logging. */
	bt_testlib_bs_sync_all();
}

static int ltk_reply(struct bt_conn *conn, const uint8_t *ltk)
{
	struct bt_hci_cp_le_ltk_req_reply *cp;
	struct net_buf *buf;
	int err;
	uint16_t handle;

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(ltk);

	/* This implementation lacks proper error handling. It's ok for
	 * testing, since we can just fail the test and crash.
	 */

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate buffer");
		return -ENOBUFS;
	}

	err = bt_testlib_get_conn_handle(conn, &handle);
	if (err) {
		LOG_ERR("Unable to get conn handle");
		net_buf_unref(buf);
		return -ENOTCONN;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	memcpy(cp->ltk, ltk, sizeof(cp->ltk));

	err = bt_hci_cmd_send(BT_HCI_OP_LE_LTK_REQ_REPLY, buf);
	if (err) {
		LOG_ERR("Failed to send LTK reply command (err %d)", err);
		net_buf_unref(buf);
	}

	return err;
}

/* What follows is a simplistic implementation of a LTK request hook. It
 * allows hooking a single pre-selected connection. Be mindful of race
 * conditions when using this hook.
 *
 * The purpose of the hook is to give the application a copy of the HCI
 * LTK request and let the application decide if the stack shall handle
 * the request. If the application does not tell the stack to handle the
 * request, then it becomes the applications responsibility to reply to
 * the LTK request.
 *
 * This implementation simply compares the request against a conn object
 * in a global variable.
 */

BUILD_ASSERT(IS_ENABLED(CONFIG_BT_HOOK_CONN_LTK_REQUEST));

static K_SEM_DEFINE(ltk_request_sem, 0, 1);
static struct bt_conn *special_conn;

/* This is the global symbol that implements the hook. */
bool bt_hook_conn_ltk_request(const uint8_t *evt)
{
	struct bt_hci_evt_le_ltk_request *evt_data = (void *)evt;
	struct bt_conn *conn;
	bool redirect_encrypt;

	LOG_INF("LTK request hook called");

	conn = bt_conn_lookup_handle(evt_data->handle, BT_CONN_TYPE_LE);
	if (!conn) {
		LOG_ERR("Unable to lookup conn");
		return false;
	}

	redirect_encrypt = (conn == special_conn);

	bt_conn_unref(conn);
	conn = NULL;

	if (redirect_encrypt) {
		LOG_INF("Matched conn: redirecting encryption");
		k_sem_give(&ltk_request_sem);
	}

	return redirect_encrypt;
}

/* This is a sample source of an LTK agreed upon by both devices. In
 * practice, it does not have to be a global variable.
 */
static const uint8_t oob_preshared_ltk[16] = {0xac, 0xa3, 0x62, 0x5a, 0x13, 0x60, 0xcc, 0x03,
					      0x1b, 0x28, 0x52, 0xcb, 0x7c, 0xa2, 0xc0, 0xdc};

static int start_encryption(struct bt_conn *conn, const uint8_t *ltk)
{
	int err;
	struct bt_hci_cp_le_start_encryption *cp;
	struct net_buf *buf;
	uint16_t handle;

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(ltk);

	/* This implementation lacks proper error handling. It's ok for
	 * testing, since we can just fail the test and crash.
	 */
	buf = bt_hci_cmd_create(BT_HCI_OP_LE_START_ENCRYPTION, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	err = bt_testlib_get_conn_handle(conn, &handle);
	if (err) {
		return err;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memset(cp, 0, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	memcpy(cp->ltk, ltk, sizeof(cp->ltk));

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_START_ENCRYPTION, buf, NULL);
}

void the_test(void)
{
	bool central = (get_device_nbr() == CENTRAL_DEVICE_NBR);
	bool peripheral = (get_device_nbr() == PERIPHERAL_DEVICE_NBR);
	struct bt_conn *conn = NULL;

	if (peripheral) {
		/* Services can and should be registered before bt_enable. */
		EXPECT_ZERO(bt_gatt_service_register(&sample_svc_requiring_encryption));
	}

	bt_testlib_enable_quiet();

	if (peripheral) {
		EXPECT_ZERO(bt_testlib_adv_conn_name(&conn, "peripheral"));

		special_conn = conn;
	}

	if (central) {
		bt_addr_le_t adva;

		EXPECT_ZERO(bt_testlib_scan_find_name(&adva, "peripheral"));
		EXPECT_ZERO(bt_testlib_connect(&adva, &conn));
	}

	bs_sync_all_log("Setup: Connected");

	if (central) {
		LOG_INF("Central starts encryption with custom LTK.");
		EXPECT_ZERO(start_encryption(conn, oob_preshared_ltk));
	}

	if (peripheral) {
		k_sem_take(&ltk_request_sem, K_FOREVER);

		LOG_INF("Peripheral repsonds with the same LTK.");
		EXPECT_ZERO(ltk_reply(conn, oob_preshared_ltk));

		testlib_wait_for_encryption(conn);
	}

	bs_sync_all_log("Security updated");

	__ASSERT_NO_MSG(bt_conn_get_security(conn) == BT_SECURITY_L2);

	bs_sync_all_log("Testing GATT security");

	if (central) {
		uint16_t chrc_enc_perm_handle;
		uint16_t chrc_aut_perm_handle;

		LOG_INF("Performing GATT discovery");

		EXPECT_ZERO(bt_testlib_gatt_discover_svc_chrc_val(conn, UUID_1, UUID_2,
								  &chrc_enc_perm_handle));
		EXPECT_ZERO(bt_testlib_gatt_discover_svc_chrc_val(conn, UUID_1, UUID_3,
								  &chrc_aut_perm_handle));

		LOG_INF("Trying read operations");

		/* Test BT_GATT_PERM_READ_ENCRYPT. This shall pass
		 * because the link is encrypted.
		 */
		EXPECT_ZERO(bt_testlib_att_read_by_handle_sync(NULL, NULL, NULL, conn, 0,
							       chrc_enc_perm_handle, 0));

		/* Test BT_GATT_PERM_READ_AUTHEN. This shall not pass
		 * because the 'authenticated' property for a connection
		 * is a separate concept defined by GAP.
		 */
		__ASSERT_NO_MSG(bt_testlib_att_read_by_handle_sync(NULL, NULL, NULL, conn, 0,
								   chrc_aut_perm_handle,
								   0) == BT_ATT_ERR_AUTHENTICATION);
	}

	bs_sync_all_log("Test complete");
	PASS("Test complete\n");
}
