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
	ssize_t read_len;

	LOG_INF("Server side buf_len %u", buf_len);

	/* Note: We assume `buf_len` is equal to the usable payload
	 * capacity of the response PDU. I.e. `(ATT_MTU - 1)` for
	 * BT_ATT_OP_READ_RSP and BT_ATT_OP_READ_BLOB_RSP.
	 */

	/* Send back a full PDU on the first read (on offset 0). Then an
	 * not full one for the second read to conlude the long read..
	 */
	read_len = buf_len;
	if (offset > 0) {
		__ASSERT_NO_MSG(read_len > 0);
		/* The second PDU is one-less-than-full to test for off
		 * by one errors.
		 */
		read_len -= 1;
	}

	/* If the ATT_MTU is too large, sending a one-less-than-full
	 * response would exeed the max attribute length limit.
	 */
	__ASSERT(buf_len < (BT_ATT_MAX_ATTRIBUTE_LEN / 2),
		 "The EATT buffer is too large for this test.");

	/* Ensure the padding bytes (that are not overwritten later in
	 * this function) are initialized.
	 */
	memset(buf, 0, read_len);

	/* Echo back the requested read size in the first two bytes of
	 * each read.
	 */
	__ASSERT_NO_MSG(read_len >= 2);
	sys_put_le16(read_len, buf);

	return read_len;
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

	LOG_INF("svc_handle: %u, svc_end_handle: %u", svc_handle, svc_end_handle);

	EXPECT_ZERO(bt_testlib_gatt_discover_characteristic(chrc_value_handle, &chrc_end_handle,
							    NULL, conn, UUID_2, (svc_handle + 1),
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

static void test_long_read(enum bt_att_chan_opt bearer, uint16_t chrc_value_handle,
			   struct bt_conn *conn)
{
	bool central = (get_device_nbr() == CENTRAL_DEVICE_NBR);

	if (central) {
		size_t read_count;

		NET_BUF_SIMPLE_DEFINE(attr_value_buf, BT_ATT_MAX_ATTRIBUTE_LEN);

		/* Perform the whole long read operation. */
		EXPECT_ZERO(bt_testlib_gatt_long_read(&attr_value_buf, NULL, NULL, conn, bearer,
						      chrc_value_handle, 0));

		/* Parse the read attribute value to verify the
		 * integrity of the transfer.
		 *
		 * Each response starts with the length of the whole
		 * response and the rest is zero-padded.
		 */
		for (read_count = 0; attr_value_buf.len; read_count++) {
			uint16_t encoded_len;
			uint16_t padding_size;

			LOG_INF("Verifying read %u", read_count);

			__ASSERT(attr_value_buf.len >= sizeof(encoded_len),
				 "Incomplete encoded length");
			encoded_len = net_buf_simple_pull_le16(&attr_value_buf);

			padding_size = (encoded_len - sizeof(uint16_t));
			LOG_INF("Padding size %u", padding_size);

			/* Check and discard padding. */
			for (uint16_t i = 0; i < padding_size; i++) {
				__ASSERT(attr_value_buf.len, "Unexpected end of buffer");
				__ASSERT(net_buf_simple_pull_u8(&attr_value_buf) == 0,
					 "Expected a padding byte at %u", i);
			}
		}
		LOG_INF("Verified %u reads", read_count);
		__ASSERT(read_count > 1, "Expected at least two reads");
	}
}

void the_test(void)
{
	bool central = (get_device_nbr() == CENTRAL_DEVICE_NBR);
	bool peripheral = (get_device_nbr() == PERIPHERAL_DEVICE_NBR);
	bt_addr_le_t adva;
	struct bt_conn *conn = NULL;
	uint16_t chrc_value_handle = 0;

	if (peripheral) {
		EXPECT_ZERO(bt_gatt_service_register(&svc));
	}

	bt_enable_quiet();

	if (peripheral) {
		EXPECT_ZERO(bt_set_name("peripheral"));
		EXPECT_ZERO(bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, bt_get_name()));
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
	}

	bs_sync_all_log("Testing UATT");
	test_long_read(BT_ATT_CHAN_OPT_UNENHANCED_ONLY, chrc_value_handle, conn);

	bs_sync_all_log("Testing EATT");
	test_long_read(BT_ATT_CHAN_OPT_ENHANCED_ONLY, chrc_value_handle, conn);

	bs_sync_all_log("Test Complete");

	PASS("Test complete\n");
}
