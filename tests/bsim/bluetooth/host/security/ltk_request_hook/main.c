/* Copyright (c) 2023 Nordic Semiconductor ASA
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

#include "testlib/adv.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "bs_macro.h"
#include "bs_sync.h"
#include <testlib/conn.h>
#include "testlib/log_utils.h"
#include "testlib/scan.h"
#include "testlib/security.h"

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

static struct bt_gatt_service svc = {
	.attrs = attrs,
	.attr_count = ARRAY_SIZE(attrs),
};

static void find_the_chrc(struct bt_conn *conn, const struct bt_uuid *svc,
			  const struct bt_uuid *chrc, uint16_t *chrc_value_handle)
{
	uint16_t svc_handle;
	uint16_t svc_end_handle;
	uint16_t chrc_end_handle;

	EXPECT_ZERO(bt_testlib_gatt_discover_primary(&svc_handle, &svc_end_handle, conn, svc, 1,
						     0xffff));

	LOG_INF("svc_handle: %u, svc_end_handle: %u", svc_handle, svc_end_handle);

	EXPECT_ZERO(bt_testlib_gatt_discover_characteristic(chrc_value_handle, &chrc_end_handle,
							    NULL, conn, chrc, (svc_handle + 1),
							    svc_end_handle));

	LOG_INF("chrc_value_handle: %u, chrc_end_handle: %u", *chrc_value_handle, chrc_end_handle);
}

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

static inline void bt_enable_quiet(void)
{
	bt_testlib_log_level_set("bt_hci_core", LOG_LEVEL_ERR);
	bt_testlib_log_level_set("bt_id", LOG_LEVEL_ERR);

	EXPECT_ZERO(bt_enable(NULL));

	bt_testlib_log_level_set("bt_hci_core", LOG_LEVEL_INF);
	bt_testlib_log_level_set("bt_id", LOG_LEVEL_INF);
}

static int ltk_reply(uint16_t handle, uint8_t *ltk)
{
	struct bt_hci_cp_le_ltk_req_reply *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_LTK_REQ_REPLY, sizeof(*cp));
	if (!buf) {
		LOG_ERR("Unable to allocate buffer");
		return -ENOBUFS;
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

/* This implementation allows hooking a single pre-selected connection.
 */

static K_SEM_DEFINE(ltk_request_sem, 0, 1);
static struct bt_conn *special_conn;

static bool ltk_request_hook(const uint8_t *evt)
{
	struct bt_hci_evt_le_ltk_request *evt_data;
	struct bt_conn *conn;
	bool redirect_encrypt;

	LOG_INF("LTK request hook called");

	evt_data = (void *)evt;
	conn = bt_conn_lookup_handle(evt_data->handle, BT_CONN_TYPE_LE);
	redirect_encrypt = conn == special_conn;

	bt_conn_unref(conn);

	if (redirect_encrypt) {
		LOG_INF("Matched conn: redirecting encryption");
		k_sem_give(&ltk_request_sem);
	}

	return redirect_encrypt;
}

void the_test(void)
{
	bool central = (get_device_nbr() == CENTRAL_DEVICE_NBR);
	bool peripheral = (get_device_nbr() == PERIPHERAL_DEVICE_NBR);
	bt_addr_le_t adva;
	struct bt_conn *conn = NULL;
	uint8_t ltk[16];

	if (peripheral) {
		EXPECT_ZERO(bt_gatt_service_register(&svc));
		EXPECT_ZERO(bt_conn_ltk_request_hook_install(ltk_request_hook));
	}

	bt_enable_quiet();

	bs_sync_all_log("Setup: Bonding using SMP");

	if (peripheral) {
		EXPECT_ZERO(bt_set_name("peripheral"));
		EXPECT_ZERO(bt_testlib_adv_conn(
			&conn, BT_ID_DEFAULT,
			(BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FORCE_NAME_IN_AD)));
	}

	if (central) {
		EXPECT_ZERO(bt_testlib_scan_find_name(&adva, "peripheral"));
		EXPECT_ZERO(bt_testlib_connect(&adva, &conn));
		EXPECT_ZERO(bt_testlib_secure(conn, BT_SECURITY_L2));
	}

	bs_sync_all_log("Setup: Bond established");

	if (peripheral) {
		LOG_INF("Setup: Export the resulting LTK.");
		EXPECT_ZERO(bt_testlib_get_ltk(conn, ltk));
		LOG_INF("Setup: Remove the bond on the peripheral.");
		EXPECT_ZERO(bt_unpair(BT_ID_DEFAULT, NULL));
	}

	EXPECT_ZERO(bt_testlib_wait_disconnected(conn));
	bt_testlib_conn_unref(&conn);

	/* At this point, the central's host still has the key material in
	 * storage and the association with the peripheral's address. The
	 * peripheral has cleared all this information, and if the hook is
	 * not used, will fail to respond to the central's encryption
	 * request.
	 */
	bs_sync_all_log("Setup: Disconnected");

	if (peripheral) {
		EXPECT_ZERO(bt_set_name("peripheral"));
		EXPECT_ZERO(bt_testlib_adv_conn(
			&conn, BT_ID_DEFAULT,
			(BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FORCE_NAME_IN_AD)));
		special_conn = conn;
	}

	if (central) {
		EXPECT_ZERO(bt_testlib_scan_find_name(&adva, "peripheral"));
		EXPECT_ZERO(bt_testlib_connect(&adva, &conn));
	}

	bs_sync_all_log("Setup: Connected");

	if (central) {
		LOG_INF("Test: Central re-establishes encryption using bond information.");
		EXPECT_ZERO(bt_testlib_secure(conn, BT_SECURITY_L2));
	}

	if (peripheral) {
		struct bt_conn_info info;
		EXPECT_ZERO(bt_conn_get_info(conn, &info));

		k_sem_take(&ltk_request_sem, K_FOREVER);

		LOG_INF("Test: Peripheral re-establishes encryption using LTK from app.");

		EXPECT_ZERO(ltk_reply(info.handle, ltk));

		/* Wait for encryption */
		testlib_wait_for_encryption(conn);
	}

	bs_sync_all_log("Security updated");

	__ASSERT_NO_MSG(bt_conn_get_security(conn) == BT_SECURITY_L2);

	if (central) {
		uint16_t chrc_enc_perm_handle = 0;
		uint16_t chrc_aut_perm_handle = 0;

		/* Test BT_GATT_PERM_READ_ENCRYPT */
		find_the_chrc(conn, UUID_1, UUID_2, &chrc_enc_perm_handle);
		EXPECT_ZERO(bt_testlib_att_read_by_handle_sync(NULL, NULL, NULL, conn, 0,
							       chrc_enc_perm_handle, 0));

		/* Test BT_GATT_PERM_READ_AUTHEN */
		find_the_chrc(conn, UUID_1, UUID_3, &chrc_aut_perm_handle);
		__ASSERT_NO_MSG(bt_testlib_att_read_by_handle_sync(NULL, NULL, NULL, conn, 0,
								   chrc_aut_perm_handle,
								   0) == BT_ATT_ERR_AUTHENTICATION);
	}

	bs_sync_all_log("Test complete");
	PASS("Test complete\n");
}
