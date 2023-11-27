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

static ssize_t read_mtu_validation_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t buf_len, uint16_t offset)
{

	LOG_DBG("Server side buf_len %u", buf_len);

	k_msleep(100);

	LOG_DBG("============================> trigger disconnect");
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_POWER_OFF);

	/* We ain't read nothin' */
	return 0;
}

static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(UUID_1),
	BT_GATT_CHARACTERISTIC(UUID_2, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_mtu_validation_chrc, NULL, NULL),
};

static struct bt_gatt_service svc = {
	.attrs = attrs,
	.attr_count = ARRAY_SIZE(attrs),
};

static void find_the_chrc(struct bt_conn *conn, uint16_t *chrc_value_handle)
{
	uint16_t svc_handle;
	uint16_t svc_end_handle;
	uint16_t chrc_end_handle;

	EXPECT_ZERO(bt_testlib_gatt_discover_primary(&svc_handle, &svc_end_handle, conn, UUID_1, 1,
						     0xffff));

	LOG_DBG("svc_handle: %u, svc_end_handle: %u", svc_handle, svc_end_handle);

	EXPECT_ZERO(bt_testlib_gatt_discover_characteristic(chrc_value_handle, &chrc_end_handle,
							    NULL, conn, UUID_2, (svc_handle + 1),
							    svc_end_handle));

	LOG_DBG("chrc_value_handle: %u, chrc_end_handle: %u", *chrc_value_handle, chrc_end_handle);
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

#define ITERATIONS 20
#define READ_PARAMS_COUNT 20
static struct bt_gatt_read_params closet[READ_PARAMS_COUNT];

static volatile int outstanding;

static uint8_t gatt_read_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_read_params *params, const void *data,
			    uint16_t length)
{
	LOG_DBG("<------------------------- read done: err %d", err);

	outstanding--;

	return 0;
}

void a_test_iteration(int i)
{
	bool central = (get_device_nbr() == CENTRAL_DEVICE_NBR);
	bool peripheral = (get_device_nbr() == PERIPHERAL_DEVICE_NBR);
	bt_addr_le_t adva;
	uint16_t chrc_value_handle = 0;
	struct bt_conn *conn = NULL;
	int err;

	LOG_DBG("############################## start iteration %d", i);

	bs_sync_all_log("Start iteration");

	if (peripheral) {
		EXPECT_ZERO(bt_set_name("peripheral"));
		EXPECT_ZERO(bt_testlib_adv_conn(
			&conn, BT_ID_DEFAULT,
			(BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FORCE_NAME_IN_AD)));
	}

	if (central) {
		EXPECT_ZERO(bt_testlib_scan_find_name(&adva, "peripheral"));
		EXPECT_ZERO(bt_testlib_connect(&adva, &conn));

		/* Establish EATT bearers. */
		EXPECT_ZERO(bt_testlib_secure(conn, BT_SECURITY_L2));

		while (bt_eatt_count(conn) == 0) {
			k_msleep(100);
		};
	}

	bs_sync_all_log("Connected");

	/* Perform discovery. */
	if (central) {
		find_the_chrc(conn, &chrc_value_handle);
	} else {
		/* Peripheral will use handle 0.
		 *
		 * This will return a permission denied from the central's
		 * server. It doesn't matter as the only thing we want as the
		 * peripheral is to also be trying to fill the TX queue with ATT
		 * PDUs.
		 */
	}

	/* Test purpose: verify no allocated resource leaks when disconnecting
	 * abruptly with non-empty queues.
	 *
	 * Test procedure (in a nutshell):
	 * - open channels
	 * - queue up lots of ATT bufs from both sides
	 * - disconnect ACL
	 * - see if anything stalls or leaks
	 *
	 * Run this procedure more times than there are said resources.
	 */
	for (int p = 0; p < ARRAY_SIZE(closet); p++) {
		memset(&closet[p], 0, sizeof(struct bt_gatt_read_params));

		closet[p].handle_count = 1;
		closet[p].single.handle = chrc_value_handle;
		closet[p].func = gatt_read_cb;

		/* A disconnected channel (or ACL conn) can end up with
		 * gatt_read returning -ENOMEM instead of -ENOTCONN. sometimes.
		 */
		LOG_DBG("-------------------------> gatt_read %d", p);
		err = bt_gatt_read(conn, &closet[p]);
		switch (err) {
		case -ENOMEM:
		case -ENOTCONN:
			LOG_DBG("not connected");
			break;
		case 0:
			outstanding++;
			break;
		default:
			FAIL("unexpected error: %d\n", err);
			break;
		}
	}

	bt_testlib_wait_disconnected(conn);
	bt_testlib_conn_unref(&conn);

	k_msleep(1000);		/* beauty rest */
	EXPECT_ZERO(outstanding);

	LOG_DBG("ended iteration %d", i);
}

void the_test(void)
{
	bool peripheral = (get_device_nbr() == PERIPHERAL_DEVICE_NBR);

	if (peripheral) {
		EXPECT_ZERO(bt_gatt_service_register(&svc));
	}

	bt_enable_quiet();

	for (int i = 0; i < ITERATIONS; i++) {
		a_test_iteration(i);
	}

	bs_sync_all_log("Test Complete");

	PASS("Test complete\n");
}
