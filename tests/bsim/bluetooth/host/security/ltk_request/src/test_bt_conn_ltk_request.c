/* Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <babblekit/testcase.h>
#include <bs_tracing.h>
#include <bsim_args_runner.h>
#include <bstests.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <testlib/adv.h>
#include <testlib/att_read.h>
#include <testlib/conn.h>
#include <testlib/enable_quiet.h>
#include <testlib/scan.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

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
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(buf);
	ARG_UNUSED(buf_len);
	ARG_UNUSED(offset);

	return 0;
}

/* Sample GATT service with two characteristics:
 * - One that requires encryption (no authentication)
 * - One that requires authentication
 *
 * Both characteristics share the same read handler, which successfully returns
 * an empty payload.
 */
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

/* Minimal implementation of an LTK request hook.
 *
 * The hook allows the application to inspect the HCI LTK Request event and
 * choose whether the stack should handle it via SMP, or whether the
 * application will provide the LTK directly at the HCI level.
 *
 * This example redirects handling only for a single pre-selected connection,
 * tracked in a global variable.
 */

BUILD_ASSERT(IS_ENABLED(CONFIG_BT_CONN_LTK_REQUEST_REPLY_API));

static K_SEM_DEFINE(ltk_request_sem, 0, 1);

/* This variable is a demonstration of how to use the hook to
 * act on a specific connection. In this test there is only one
 * connection, so this variable is not needed. But it is done to
 * demonstrate the technique.
 */
static struct bt_conn *custom_ltk_conn;

static bool app_ltk_request_cb(struct bt_conn *conn, uint64_t rand, uint16_t ediv)
{
	ARG_UNUSED(rand);
	ARG_UNUSED(ediv);

	bool redirect_encrypt;

	LOG_INF("LTK request hook called");

	redirect_encrypt = (conn == custom_ltk_conn);

	if (redirect_encrypt) {
		LOG_INF("Matched conn: redirecting encryption");
		k_sem_give(&ltk_request_sem);
	}

	return redirect_encrypt;
}

/* Pre-shared LTK used by both devices in this test.
 *
 * In production, the LTK must be provisioned securely (for
 * example, via ECDH key exchange with authentication or
 * obtained from a trusted backend). It is hardcoded here for
 * simplicity.
 */
static const uint8_t oob_preshared_ltk[16] = {0xac, 0xa3, 0x62, 0x5a, 0x13, 0x60, 0xcc, 0x03,
					      0x1b, 0x28, 0x52, 0xcb, 0x7c, 0xa2, 0xc0, 0xdc};

/** @brief Central-side function that starts encryption by directly calling the HCI command.
 */
static void start_encryption(struct bt_conn *conn, const uint8_t *ltk)
{
	struct bt_hci_cp_le_start_encryption *cp;
	struct net_buf *buf;
	uint16_t handle;
	int err;

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(ltk);

	err = bt_hci_get_conn_handle(conn, &handle);
	if (err) {
		LOG_ERR("Failed to get connection handle (err %d)", err);
		return;
	}

	buf = bt_hci_cmd_alloc(K_FOREVER);

	/* K_FOREVER guarantees us a buffer. */
	__ASSERT_NO_MSG(buf != NULL);

	cp = net_buf_add(buf, sizeof(*cp));
	memset(cp, 0, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	memcpy(cp->ltk, ltk, sizeof(cp->ltk));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_START_ENCRYPTION, buf, NULL);
	if (err) {
		LOG_ERR("Failed to send LE start encryption command (err %d)", err);
		k_panic();
	}
}

void test_bt_conn_ltk_request(void)
{
	/* This test requires two devices. We name the two devices "central"
	 * and "peripheral" for convenience.
	 */
	bool central = (get_device_nbr() == 0);
	bool peripheral = (get_device_nbr() == 1);

	struct bt_conn *conn = NULL;

	/* This is the test body and it is run independently on both devices.
	 * It is written so it looks like the two devices are controlled by
	 * one thread of running code, but this is achieved by careful
	 * synchronization so that the devices are in lockstep where it
	 * matters.
	 */

	/* === The usual: GATT service registration and bt_enable === */
	if (peripheral) {
		int err;

		err = bt_gatt_service_register(&sample_svc_requiring_encryption);
		__ASSERT_NO_MSG(!err);

		err = bt_conn_ltk_request_cb_register(app_ltk_request_cb);
		if (err) {
			TEST_FAIL("Failed to register LTK request hook (err %d)", err);
		}
	}

	bt_testlib_silent_bt_enable();

	/* === The usual: Connecting === */
	if (peripheral) {
		int err;

		err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, "peripheral");
		__ASSERT_NO_MSG(!err);
		custom_ltk_conn = conn;
	}

	if (central) {
		bt_addr_le_t adva;
		int err;

		err = bt_testlib_scan_find_name(&adva, "peripheral");
		__ASSERT_NO_MSG(!err);
		err = bt_testlib_connect(&adva, &conn);
		__ASSERT_NO_MSG(!err);
	}

	/* === The meat of the test: Encryption === */
	if (central) {
		LOG_INF("Central starts encryption with custom LTK.");
		start_encryption(conn, oob_preshared_ltk);
	}

	if (peripheral) {
		int err;

		/* When central starts encryption, the LTK request callback is
		 * triggered on the peripheral. See `app_ltk_request_cb` now to
		 * follow the flow.
		 */
		k_sem_take(&ltk_request_sem, K_FOREVER);

		LOG_INF("Peripheral responds with the same LTK.");
		err = bt_conn_ltk_request_reply(conn, oob_preshared_ltk);
		if (err) {
			TEST_FAIL("Failed to reply with LTK (err %d)", err);
		}
	}

	/* Once we have replied with the custom LTK the link should
	 * become encrypted.
	 */
	testlib_wait_for_encryption(conn);
	{
		bt_security_t sec_level;

		sec_level = bt_conn_get_security(conn);
		if (sec_level != BT_SECURITY_L2) {
			TEST_FAIL("Link should be at level 2 security (encrypted and not "
				  "authenticated), but it is %d",
				  sec_level);
		}
	}

	/* === Testing effects on other parts of the system: GATT security === */

	if (central) {
		uint16_t chrc_enc_perm_handle;
		uint16_t chrc_aut_perm_handle;
		int err;

		LOG_INF("Performing GATT discovery");

		/* Setup: Discover GATT handles. */
		err = bt_testlib_gatt_discover_svc_chrc_val(conn, UUID_1, UUID_2,
							    &chrc_enc_perm_handle);
		__ASSERT_NO_MSG(!err);
		err = bt_testlib_gatt_discover_svc_chrc_val(conn, UUID_1, UUID_3,
							    &chrc_aut_perm_handle);
		__ASSERT_NO_MSG(!err);

		LOG_INF("Trying read operations");

		/* Test BT_GATT_PERM_READ_ENCRYPT. This shall pass
		 * because the link is encrypted.
		 */
		err = bt_testlib_att_read_by_handle_sync(NULL, NULL, NULL, conn, 0,
							 chrc_enc_perm_handle, 0);
		__ASSERT_NO_MSG(err >= 0);
		if (err != BT_ATT_ERR_SUCCESS) {
			TEST_FAIL("Reading the characteristic that requires encryption should "
				  "give the ATT response BT_ATT_ERR_SUCCESS, but was %d",
				  err);
		}

		/* Test BT_GATT_PERM_READ_AUTHEN. This shall not pass
		 * because the 'authenticated' property for a connection
		 * is a separate concept defined by GAP.
		 */
		err = bt_testlib_att_read_by_handle_sync(NULL, NULL, NULL, conn, 0,
							 chrc_aut_perm_handle, 0);
		__ASSERT_NO_MSG(err >= 0);
		if (err != BT_ATT_ERR_AUTHENTICATION) {
			TEST_FAIL("Reading the characteristic that requires authentication "
				  "should give the ATT response BT_ATT_ERR_AUTHENTICATION, but was "
				  "%d",
				  err);
		}
	}

	TEST_PASS("Test complete");

	if (central) {
		bt_testlib_disconnect(&conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	if (peripheral) {
		bt_testlib_wait_disconnected(conn);
		bt_testlib_conn_unref(&conn);

		/* Terminate the simulation to save CPU cycles. */
		bs_trace_silent_exit(0);
	}
}

static struct bst_test_list *test_installer(struct bst_test_list *installed_tests)
{
	static const struct bst_test_instance tests[] = {
		{
			.test_id = "test_bt_conn_ltk_request",
			.test_main_f = test_bt_conn_ltk_request,
		},
		BSTEST_END_MARKER,
	};

	return bst_add_tests(installed_tests, tests);
};

bst_test_install_t test_installers[] = {test_installer, NULL};

int main(void)
{
	bst_main();
	return 0;
}
