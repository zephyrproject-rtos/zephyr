/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* Transient Application Key encryption (Core Vol 3 Part C 10.6.2). Also built into
 * gatt/ccc_props, the image without the Security Manager.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "babblekit/flags.h"
#include "babblekit/testcase.h"
#include "bstests.h"

#define TAK_UUID(n)                                                                                \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x7a6b0000 + (n), 0x1c2d, 0x4e3f, 0x8a9b,           \
					       0x0c1d2e3f4a5b))

#define SVC_UUID        TAK_UUID(0)
#define ENC_UUID        TAK_UUID(1)
#define ENC_TAK_UUID    TAK_UUID(2)
#define AUTHEN_UUID     TAK_UUID(3)
#define AUTHEN_TAK_UUID TAK_UUID(4)

/* PSMs from the dynamic range (0x0080 to 0x00ff) for the two L2CAP servers; both require
 * encryption, one permits a TAK.
 */
#define PSM_TAK    0x0080U
#define PSM_NO_TAK 0x0081U

/* LE GATT Security Levels value (Core Vol 3 Part C 12.7): one {mode, level} pair, for LE
 * security mode 1. The databases of these images have attributes that a TAK does not open, so
 * mode 4 is not named.
 */
#define SECURITY_LEVELS_LEN 2U
#define SECURITY_MODE_1     0x01U

/* Room for more than the test expects of a read, so that a longer value shows */
#define READ_DATA_LEN 8U

/* Encryption attempts against a Peripheral without a key: the second shows a retry is possible */
#define NO_KEY_ATTEMPTS 2U

/* Requests that bring back anything the peer sent before them, and then its answer to that */
#define ROUND_TRIPS 2U

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_disconnected);
DEFINE_FLAG_STATIC(flag_recycled);
DEFINE_FLAG_STATIC(flag_security_changed);
DEFINE_FLAG_STATIC(flag_read_done);
/* The next failed security procedure is answered with a TAK from its callback */
DEFINE_FLAG_STATIC(flag_tak_on_failure);

static uint8_t read_data[READ_DATA_LEN];
static enum bt_security_err sec_err;
static struct bt_conn *tak_conn;
static bt_security_t sec_level;
static uint8_t disc_reason;
static uint16_t read_len;
static uint8_t read_err;

static int supply_key(bool is_matching)
{
	uint8_t tak[BT_CONN_LE_TAK_SIZE] = { 0x54, 0x41, 0x4b, 0x00, 0x01, 0x02, 0x03, 0x04,
					     0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c };

	if (!is_matching) {
		/* Any difference from the key of the peer will do */
		tak[0] ^= 0xffU;
	}

	return bt_conn_le_tak_set(tak_conn, tak);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == BT_HCI_ERR_SUCCESS, "Connection failed (err 0x%02x)", err);

	/* The Central holds the reference that bt_conn_le_create() gave it */
	if (tak_conn == NULL) {
		tak_conn = bt_conn_ref(conn);
	}

	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected: reason 0x%02x\n", reason);

	disc_reason = reason;
	SET_FLAG(flag_disconnected);
}

static void recycled(void)
{
	SET_FLAG(flag_recycled);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	printk("Security changed: level %d err %d\n", level, err);

	if (err != BT_SECURITY_ERR_SUCCESS && IS_FLAG_SET(flag_tak_on_failure)) {
		int ret;

		/* The failed pairing is still being wound up by its caller */
		UNSET_FLAG(flag_tak_on_failure);

		ret = bt_conn_le_non_paired_set(conn);
		TEST_ASSERT(ret == 0, "bt_conn_le_non_paired_set failed (err %d)", ret);

		ret = supply_key(true);
		TEST_ASSERT(ret == 0, "bt_conn_le_tak_set failed (err %d)", ret);

		return;
	}

	sec_level = level;
	sec_err = err;
	SET_FLAG(flag_security_changed);
}

#if defined(CONFIG_BT_SMP)
DEFINE_FLAG_STATIC(flag_pairing_event);
static enum bt_security_err pairing_err;
static atomic_t pairing_failed_cnt;

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	pairing_err = BT_SECURITY_ERR_SUCCESS;
	SET_FLAG(flag_pairing_event);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err err)
{
	printk("Pairing failed: err %d\n", err);

	pairing_err = err;
	(void)atomic_inc(&pairing_failed_cnt);
	SET_FLAG(flag_pairing_event);
}

static void count_bond(const struct bt_bond_info *info, void *user_data)
{
	size_t *count = user_data;

	(*count)++;
}

static void register_pairing_callbacks(void)
{
	static struct bt_conn_auth_info_cb auth_info_cb = {
		.pairing_complete = pairing_complete,
		.pairing_failed = pairing_failed,
	};
	int err;

	err = bt_conn_auth_info_cb_register(&auth_info_cb);
	TEST_ASSERT(err == 0, "bt_conn_auth_info_cb_register failed (err %d)", err);
}

static void expect_not_paired(void)
{
	size_t bonds = 0;

	TEST_ASSERT(!IS_FLAG_SET(flag_pairing_event), "Pairing ran (err %d)", pairing_err);

	bt_foreach_bond(BT_ID_DEFAULT, count_bond, &bonds);
	TEST_ASSERT(bonds == 0U, "%zu bonds stored", bonds);
}
#else
static void register_pairing_callbacks(void)
{
}

static void expect_not_paired(void)
{
}
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL)
DEFINE_FLAG_STATIC(flag_chan_connected);
DEFINE_FLAG_STATIC(flag_chan_disconnected);

static void chan_connected(struct bt_l2cap_chan *chan)
{
	SET_FLAG(flag_chan_connected);
}

static void chan_disconnected(struct bt_l2cap_chan *chan)
{
	SET_FLAG(flag_chan_disconnected);
}

static int chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	return 0;
}

static const struct bt_l2cap_chan_ops chan_ops = {
	.connected = chan_connected,
	.disconnected = chan_disconnected,
	.recv = chan_recv,
};

static int server_accept(struct bt_conn *conn, struct bt_l2cap_server *server,
			 struct bt_l2cap_chan **chan)
{
	static struct bt_l2cap_le_chan server_chan = {
		.chan.ops = &chan_ops,
	};

	*chan = &server_chan.chan;

	return 0;
}

static void register_servers(void)
{
	static struct bt_l2cap_server tak_server = {
		.psm = PSM_TAK,
		.sec_level = BT_SECURITY_L2,
		.tak = true,
		.accept = server_accept,
	};
	static struct bt_l2cap_server no_tak_server = {
		.psm = PSM_NO_TAK,
		.sec_level = BT_SECURITY_L2,
		.accept = server_accept,
	};
	int err;

	err = bt_l2cap_server_register(&tak_server);
	TEST_ASSERT(err == 0, "bt_l2cap_server_register failed (err %d)", err);

	err = bt_l2cap_server_register(&no_tak_server);
	TEST_ASSERT(err == 0, "bt_l2cap_server_register failed (err %d)", err);
}

static void expect_channel(uint16_t psm, bool is_accepted)
{
	static struct bt_l2cap_le_chan client_chan;
	int err;

	(void)memset(&client_chan, 0, sizeof(client_chan));
	client_chan.chan.ops = &chan_ops;

	err = bt_l2cap_chan_connect(tak_conn, &client_chan.chan, psm);
	TEST_ASSERT(err == 0, "bt_l2cap_chan_connect failed (err %d)", err);

	if (is_accepted) {
		WAIT_FOR_FLAG(flag_chan_connected);

		err = bt_l2cap_chan_disconnect(&client_chan.chan);
		TEST_ASSERT(err == 0, "bt_l2cap_chan_disconnect failed (err %d)", err);
	}

	WAIT_FOR_FLAG(flag_chan_disconnected);
	TEST_ASSERT(IS_FLAG_SET(flag_chan_connected) == is_accepted, "PSM 0x%04x %s", psm,
		    is_accepted ? "refused" : "accepted");

	UNSET_FLAG(flag_chan_connected);
	UNSET_FLAG(flag_chan_disconnected);
}
#else
static void register_servers(void)
{
}

static void expect_channel(uint16_t psm, bool is_accepted)
{
	ARGS_UNUSED(psm, is_accepted);
}
#endif /* CONFIG_BT_L2CAP_DYNAMIC_CHANNEL */

static void setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
		.disconnected = disconnected,
		.recycled = recycled,
		.security_changed = security_changed,
	};
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (err %d)", err);

	err = bt_conn_cb_register(&conn_cb);
	TEST_ASSERT(err == 0, "bt_conn_cb_register failed (err %d)", err);

	register_pairing_callbacks();
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	int err;

	if (tak_conn != NULL) {
		return;
	}

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "bt_le_scan_stop failed (err %d)", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &tak_conn);
	TEST_ASSERT(err == 0, "bt_conn_le_create failed (err %d)", err);
}

static void connect_to_peripheral(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	TEST_ASSERT(err == 0, "bt_le_scan_start failed (err %d)", err);

	WAIT_FOR_FLAG(flag_connected);
}

static void wait_for_central(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "bt_le_adv_start failed (err %d)", err);

	WAIT_FOR_FLAG(flag_connected);
}

static void wait_for_connection_end(void)
{
	WAIT_FOR_FLAG(flag_disconnected);
	bt_conn_drop(&tak_conn);

	/* The next connection needs the connection object back */
	WAIT_FOR_FLAG(flag_recycled);

	UNSET_FLAG(flag_connected);
	UNSET_FLAG(flag_disconnected);
	UNSET_FLAG(flag_recycled);
}

static void end_connection(void)
{
	int err;

	err = bt_conn_disconnect(tak_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	TEST_ASSERT(err == 0, "bt_conn_disconnect failed (err %d)", err);

	wait_for_connection_end();
}

static void use_non_paired(void)
{
	static const uint8_t zero_tak[BT_CONN_LE_TAK_SIZE];
	int err;

	err = bt_conn_le_non_paired_set(tak_conn);
	TEST_ASSERT(err == 0, "bt_conn_le_non_paired_set failed (err %d)", err);

	/* A key buffer that no key derivation wrote to */
	err = bt_conn_le_tak_set(tak_conn, zero_tak);
	TEST_ASSERT(err == -EINVAL, "All-zero key returned %d", err);

	/* Pairing is not available as a fallback, before or after the key */
	err = bt_conn_set_security(tak_conn, BT_SECURITY_L2);
	TEST_ASSERT(err == -EPERM, "bt_conn_set_security returned %d", err);
}

static void expect_tak_link(void)
{
	struct bt_conn_info info;
	int err;

	WAIT_FOR_FLAG(flag_security_changed);
	TEST_ASSERT(sec_err == BT_SECURITY_ERR_SUCCESS, "Encryption failed (err %d)", sec_err);
	TEST_ASSERT(sec_level == BT_SECURITY_L2, "Unexpected level %d", sec_level);

	err = bt_conn_get_info(tak_conn, &info);
	TEST_ASSERT(err == 0, "bt_conn_get_info failed (err %d)", err);
	TEST_ASSERT(info.security.level == BT_SECURITY_L2, "Unexpected level %d",
		    info.security.level);
	TEST_ASSERT((info.security.flags & BT_SECURITY_FLAG_TAK) != 0U, "TAK flag not set");
	TEST_ASSERT(info.security.enc_key_size == BT_CONN_LE_TAK_SIZE, "Unexpected key size %u",
		    info.security.enc_key_size);

	/* Encrypted already at L2; anything above would take pairing */
	err = bt_conn_set_security(tak_conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "L2 on a TAK link failed (err %d)", err);
	err = bt_conn_set_security(tak_conn, BT_SECURITY_L3);
	TEST_ASSERT(err == -EPERM, "L3 on a TAK link returned %d", err);

	err = supply_key(true);
	TEST_ASSERT(err == -EALREADY, "Second key returned %d", err);
}

static uint8_t read_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_read_params *params,
		       const void *data, uint16_t length)
{
	read_err = err;
	read_len = 0U;

	if (data != NULL) {
		read_len = MIN(length, sizeof(read_data));
		(void)memcpy(read_data, data, read_len);
	}

	SET_FLAG(flag_read_done);

	return BT_GATT_ITER_STOP;
}

static void expect_read(const struct bt_uuid *uuid, uint8_t expected)
{
	/* Has to outlive the request */
	static struct bt_gatt_read_params params;
	int err;

	params = (struct bt_gatt_read_params){
		.func = read_cb,
		.by_uuid.uuid = uuid,
		.by_uuid.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
		.by_uuid.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
	};

	UNSET_FLAG(flag_read_done);

	err = bt_gatt_read(tak_conn, &params);
	TEST_ASSERT(err == 0, "bt_gatt_read failed (err %d)", err);

	WAIT_FOR_FLAG(flag_read_done);
	TEST_ASSERT(read_err == expected, "Read returned 0x%02x, expected 0x%02x", read_err,
		    expected);
}

/* The GAP service names the highest mode 1 level the database asks for, which is at least the
 * level 3 of the service below.
 */
static void expect_security_levels(void)
{
	expect_read(BT_UUID_GATT_SL, BT_ATT_ERR_SUCCESS);
	TEST_ASSERT(read_len == SECURITY_LEVELS_LEN, "Unexpected length %u", read_len);
	TEST_ASSERT(read_data[0] == SECURITY_MODE_1 && read_data[1] >= BT_SECURITY_L3,
		    "Unexpected mode 1 entry %u %u", read_data[0], read_data[1]);
}

static ssize_t read_value(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	static const uint8_t value = 0x5aU;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value, sizeof(value));
}

/* Static because the images have no dynamic database; their other tests find services by UUID. */
BT_GATT_SERVICE_DEFINE(tak_svc,
	BT_GATT_PRIMARY_SERVICE(SVC_UUID),
	BT_GATT_CHARACTERISTIC(ENC_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT, read_value,
			       NULL, NULL),
	BT_GATT_CHARACTERISTIC(ENC_TAK_UUID, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_TAK, read_value, NULL,
			       NULL),
	BT_GATT_CHARACTERISTIC(AUTHEN_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_AUTHEN,
			       read_value, NULL, NULL),
	BT_GATT_CHARACTERISTIC(AUTHEN_TAK_UUID, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_TAK, read_value, NULL,
			       NULL),
);

/* Both devices supply the same key: the link is encrypted, nothing is paired, and the server
 * opens only what permits a TAK.
 */
static void central_main(void)
{
	int err;

	setup();
	connect_to_peripheral();
	use_non_paired();

	expect_read(ENC_TAK_UUID, BT_ATT_ERR_INSUFFICIENT_ENCRYPTION);
	expect_channel(PSM_TAK, false);

	err = supply_key(true);
	TEST_ASSERT(err == 0, "bt_conn_le_tak_set failed (err %d)", err);
	expect_tak_link();

	expect_read(ENC_TAK_UUID, BT_ATT_ERR_SUCCESS);
	expect_read(AUTHEN_TAK_UUID, BT_ATT_ERR_SUCCESS);
	expect_read(ENC_UUID, BT_ATT_ERR_INSUFFICIENT_ENCRYPTION);
	expect_read(AUTHEN_UUID, BT_ATT_ERR_AUTHENTICATION);
	expect_security_levels();

	expect_channel(PSM_TAK, true);
	expect_channel(PSM_NO_TAK, false);

	expect_not_paired();
	end_connection();

	TEST_PASS("Central done");
}

static void peripheral_main(void)
{
	int err;

	setup();
	register_servers();
	wait_for_central();
	use_non_paired();

	err = supply_key(true);
	TEST_ASSERT(err == 0, "bt_conn_le_tak_set failed (err %d)", err);
	expect_tak_link();

	wait_for_connection_end();
	expect_not_paired();

	TEST_PASS("Peripheral done");
}

/* The devices supply different keys: encryption does not come up and the link ends. This also
 * shows that the supplied key is the one the Controller uses.
 */
static void key_mismatch(bool is_matching)
{
	int err;

	use_non_paired();

	err = supply_key(is_matching);
	TEST_ASSERT(err == 0, "bt_conn_le_tak_set failed (err %d)", err);

	WAIT_FOR_FLAG(flag_disconnected);
	TEST_ASSERT(!IS_FLAG_SET(flag_security_changed) || sec_err != BT_SECURITY_ERR_SUCCESS,
		    "Encrypted with different keys");
	expect_not_paired();
}

static void central_key_mismatch_main(void)
{
	setup();
	connect_to_peripheral();
	key_mismatch(false);

	TEST_PASS("Central done");
}

static void peripheral_key_mismatch_main(void)
{
	setup();
	wait_for_central();
	key_mismatch(true);
	TEST_ASSERT(disc_reason == BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL, "Unexpected reason 0x%02x",
		    disc_reason);

	TEST_PASS("Peripheral done");
}

/* The Peripheral has no key: it refuses the encryption, the link stays up unencrypted, and the
 * Central can try again.
 */
static void central_no_key_main(void)
{
	setup();
	connect_to_peripheral();
	use_non_paired();

	for (uint8_t attempt = 0U; attempt < NO_KEY_ATTEMPTS; attempt++) {
		int err;

		err = supply_key(true);
		TEST_ASSERT(err == 0, "bt_conn_le_tak_set failed (err %d)", err);

		WAIT_FOR_FLAG(flag_security_changed);
		TEST_ASSERT(sec_err == BT_SECURITY_ERR_PIN_OR_KEY_MISSING,
			    "Unexpected security err %d", sec_err);
		TEST_ASSERT(bt_conn_get_security(tak_conn) == BT_SECURITY_L1,
			    "Unexpected level %d", bt_conn_get_security(tak_conn));
		UNSET_FLAG(flag_security_changed);
	}

	expect_not_paired();
	end_connection();

	TEST_PASS("Central done");
}

static void peripheral_no_key_main(void)
{
	setup();
	wait_for_central();
	use_non_paired();

	wait_for_connection_end();
	TEST_ASSERT(!IS_FLAG_SET(flag_security_changed), "Security changed without a key");
	expect_not_paired();

	TEST_PASS("Peripheral done");
}

#if defined(CONFIG_BT_SMP)
/* A device that uses the non-paired security model answers pairing with Pairing Not Supported
 * and sends no pairing or security request of its own. First connection: the Peripheral uses
 * the model and the Central pairs. Second connection: the other way around.
 */
static void expect_pairing_refused(void)
{
	int err;

	/* A pairing or security request that the peer sent on its own arrives ahead of the first
	 * response, and the Pairing Failed that ends it ahead of the second.
	 */
	for (uint8_t round_trip = 0U; round_trip < ROUND_TRIPS; round_trip++) {
		expect_read(BT_UUID_GAP_DEVICE_NAME, BT_ATT_ERR_SUCCESS);
	}

	TEST_ASSERT(!IS_FLAG_SET(flag_pairing_event) && !IS_FLAG_SET(flag_security_changed),
		    "Peer started pairing");

	/* Pairing Request as Central, Security Request as Peripheral */
	err = bt_conn_set_security(tak_conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "bt_conn_set_security failed (err %d)", err);

	WAIT_FOR_FLAG(flag_security_changed);
	TEST_ASSERT(sec_err == BT_SECURITY_ERR_PAIR_NOT_SUPPORTED, "Unexpected security err %d",
		    sec_err);
	TEST_ASSERT(bt_conn_get_security(tak_conn) == BT_SECURITY_L1, "Unexpected level %d",
		    bt_conn_get_security(tak_conn));

	UNSET_FLAG(flag_security_changed);
	UNSET_FLAG(flag_pairing_event);
}

static void central_reject_main(void)
{
	setup();

	connect_to_peripheral();
	expect_pairing_refused();
	end_connection();

	connect_to_peripheral();
	use_non_paired();
	wait_for_connection_end();
	expect_not_paired();

	TEST_PASS("Central done");
}

/* A Central whose pairing was refused switches to a TAK from the callback that reports it: the
 * pairing is over, and is not started again for the level the TAK encryption asks for.
 */
static void central_after_pairing_main(void)
{
	int err;

	setup();
	connect_to_peripheral();

	/* Answered this way once the Peripheral uses the non-paired security model: the pairing
	 * below is then refused, not accepted.
	 */
	expect_read(ENC_TAK_UUID, BT_ATT_ERR_INSUFFICIENT_ENCRYPTION);

	SET_FLAG(flag_tak_on_failure);

	err = bt_conn_set_security(tak_conn, BT_SECURITY_L2);
	TEST_ASSERT(err == 0, "bt_conn_set_security failed (err %d)", err);

	expect_tak_link();
	expect_read(ENC_TAK_UUID, BT_ATT_ERR_SUCCESS);

	end_connection();

	/* A pairing that was started again gets no answer from a peer that uses the non-paired
	 * security model, and fails when the connection ends.
	 */
	TEST_ASSERT(atomic_get(&pairing_failed_cnt) == 1, "Pairing failed %ld times",
		    atomic_get(&pairing_failed_cnt));

	TEST_PASS("Central done");
}

static void peripheral_reject_main(void)
{
	setup();

	wait_for_central();
	use_non_paired();
	wait_for_connection_end();
	expect_not_paired();

	wait_for_central();
	expect_pairing_refused();
	end_connection();

	TEST_PASS("Peripheral done");
}
#endif /* CONFIG_BT_SMP */

struct bst_test_list *test_tak_install(struct bst_test_list *tests)
{
	static const struct bst_test_instance tak_tests[] = {
		{
			.test_id = "tak_central",
			.test_main_f = central_main,
		},
		{
			.test_id = "tak_peripheral",
			.test_main_f = peripheral_main,
		},
		{
			.test_id = "tak_central_bad_key",
			.test_main_f = central_key_mismatch_main,
		},
		{
			.test_id = "tak_peripheral_bad_key",
			.test_main_f = peripheral_key_mismatch_main,
		},
		{
			.test_id = "tak_central_no_key",
			.test_main_f = central_no_key_main,
		},
		{
			.test_id = "tak_peripheral_no_key",
			.test_main_f = peripheral_no_key_main,
		},
#if defined(CONFIG_BT_SMP)
		{
			.test_id = "tak_central_after_pairing",
			.test_main_f = central_after_pairing_main,
		},
		{
			.test_id = "tak_central_reject",
			.test_main_f = central_reject_main,
		},
		{
			.test_id = "tak_peripheral_reject",
			.test_main_f = peripheral_reject_main,
		},
#endif /* CONFIG_BT_SMP */
		BSTEST_END_MARKER,
	};

	return bst_add_tests(tests, tak_tests);
}
