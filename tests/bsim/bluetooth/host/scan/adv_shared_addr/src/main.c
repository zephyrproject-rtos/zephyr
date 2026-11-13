/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/controller.h>
#include <zephyr/bluetooth/hci.h>
#include <babblekit/testcase.h>

LOG_MODULE_REGISTER(bt_bsim_scan_adv_shared_addr, LOG_LEVEL_INF);

/* Start scanning while a legacy advertiser is enabled. With legacy
 * advertising commands the scanner shares the advertiser's address, so it
 * must use the address type the advertiser uses.
 */

#define WAIT_TIMEOUT K_SECONDS(2)

static const uint8_t public_addr[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0xc0};

/* Target of the directed advertiser, not present in the simulation. */
static const bt_addr_le_t dir_target = {
	.type = BT_ADDR_LE_RANDOM,
	.a = {{0x11, 0x22, 0x33, 0x44, 0x55, 0xc6}},
};

static const uint8_t peer_magic[] = {0xff, 0xff, 's', 'h', 'a', 'r', 'e', 'd'};

static const struct bt_data peer_ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, peer_magic, sizeof(peer_magic)),
};

static const struct bt_data dut_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
};

static const struct bt_le_scan_param scan_param = {
	.type = BT_LE_SCAN_TYPE_PASSIVE,
	.interval = BT_GAP_SCAN_FAST_INTERVAL,
	.window = BT_GAP_SCAN_FAST_WINDOW,
};

/* Only the test thread sends HCI commands in this test. */
static bool in_scan_start;
static int scan_own_addr_type = -1;
static int scan_random_addr_cmds;
static uint16_t rejected_opcode;
static int rejected_err;
static bt_addr_t random_addr;
static bool random_addr_set;

int __real_bt_hci_cmd_send_sync(uint16_t opcode, struct net_buf *buf, struct net_buf **rsp);

int __wrap_bt_hci_cmd_send_sync(uint16_t opcode, struct net_buf *buf, struct net_buf **rsp)
{
	bool set_random = opcode == BT_HCI_OP_LE_SET_RANDOM_ADDRESS && buf != NULL &&
			  buf->len >= sizeof(bt_addr_t);
	bt_addr_t addr;
	int err;

	if (set_random) {
		memcpy(&addr, buf->data, sizeof(addr));
	}

	if (in_scan_start) {
		if (opcode == BT_HCI_OP_LE_SET_RANDOM_ADDRESS) {
			scan_random_addr_cmds++;
		}

		if (opcode == BT_HCI_OP_LE_SET_SCAN_PARAM && buf != NULL &&
		    buf->len >= sizeof(struct bt_hci_cp_le_set_scan_param)) {
			const struct bt_hci_cp_le_set_scan_param *cp = (const void *)buf->data;

			scan_own_addr_type = cp->addr_type;
		}
	}

	err = __real_bt_hci_cmd_send_sync(opcode, buf, rsp);

	if (set_random && err == 0) {
		bt_addr_copy(&random_addr, &addr);
		random_addr_set = true;
	}

	if (err != 0 && in_scan_start && rejected_opcode == 0U) {
		rejected_opcode = opcode;
		rejected_err = err;
	}

	return err;
}

static K_SEM_DEFINE(peer_found, 0, 1);

static bool find_magic(struct bt_data *data, void *user_data)
{
	bool *found = user_data;

	if (data->type == BT_DATA_MANUFACTURER_DATA && data->data_len == sizeof(peer_magic) &&
	    memcmp(data->data, peer_magic, sizeof(peer_magic)) == 0) {
		*found = true;
		return false;
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	bool found = false;

	bt_data_parse(ad, find_magic, &found);
	if (found) {
		k_sem_give(&peer_found);
	}
}

/* Bring up the host with the given identity type, start the advertiser and
 * then the scanner.
 */
static void start_adv_then_scan(bool public_identity, uint32_t adv_options,
				const bt_addr_le_t *peer)
{
	const struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
		adv_options, BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, peer);
	char addr_str[BT_ADDR_STR_LEN] = "none";
	bt_addr_le_t id_addr;
	size_t count = 1;
	int err;

	if (public_identity) {
		bt_ctlr_set_public_addr(public_addr);
	}

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	bt_id_get(&id_addr, &count);
	TEST_ASSERT(count == 1, "No identity address");
	TEST_ASSERT(id_addr.type == (public_identity ? BT_ADDR_LE_PUBLIC : BT_ADDR_LE_RANDOM),
		    "Unexpected identity address type %u", id_addr.type);

	err = bt_le_adv_start(&adv_param, dut_ad, ARRAY_SIZE(dut_ad), NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed to start (err %d)", err);

	in_scan_start = true;
	err = bt_le_scan_start(&scan_param, device_found);
	in_scan_start = false;

	if (random_addr_set) {
		bt_addr_to_str(&random_addr, addr_str, sizeof(addr_str));
	}

	LOG_INF("scan_err %d rejected 0x%04x/%d own_type %d random_addr_cmds %d random %s", err,
		rejected_opcode, rejected_err, scan_own_addr_type, scan_random_addr_cmds, addr_str);

	TEST_ASSERT(err == 0, "bt_le_scan_start() failed (err %d), opcode 0x%04x rejected with %d",
		    err, rejected_opcode, rejected_err);
	TEST_ASSERT(scan_random_addr_cmds == 0,
		    "The scanner sent %d LE Set Random Address commands", scan_random_addr_cmds);

	if (public_identity) {
		TEST_ASSERT(scan_own_addr_type == BT_HCI_OWN_ADDR_PUBLIC,
			    "Scanner own address type %d, expected public", scan_own_addr_type);
	} else {
		TEST_ASSERT(scan_own_addr_type == BT_HCI_OWN_ADDR_RANDOM,
			    "Scanner own address type %d, expected random", scan_own_addr_type);
		TEST_ASSERT(random_addr_set && bt_addr_eq(&random_addr, &id_addr.a),
			    "Random address %s is not the identity address", addr_str);
	}

	TEST_ASSERT(k_sem_take(&peer_found, WAIT_TIMEOUT) == 0,
		    "No advertising report from the peer");
}

static void test_public_conn(void)
{
	start_adv_then_scan(true, BT_LE_ADV_OPT_CONN, NULL);

	TEST_PASS("Scanned with the public address of a connectable advertiser");
}

static void test_public_use_identity(void)
{
	start_adv_then_scan(true, BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY, NULL);

	TEST_PASS("Scanned with the public identity advertiser address");
}

static void test_public_dir(void)
{
	start_adv_then_scan(true, BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY,
			    &dir_target);

	TEST_PASS("Scanned with the public address of a directed advertiser");
}

static void test_public_dir_rpa(void)
{
	start_adv_then_scan(true,
			    BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY |
				    BT_LE_ADV_OPT_DIR_ADDR_RPA,
			    &dir_target);

	TEST_PASS("Scanned with the public address of a directed RPA advertiser");
}

static void test_random_conn(void)
{
	start_adv_then_scan(false, BT_LE_ADV_OPT_CONN, NULL);

	TEST_PASS("Scanned with the static random address of a connectable advertiser");
}

static void test_peer(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	/* A short fixed interval that differs from the scanner's own advertiser,
	 * so that its advertising events cannot hide every peer advertisement.
	 */
	const struct bt_le_adv_param peer_param = BT_LE_ADV_PARAM_INIT(
		0, BT_GAP_MS_TO_ADV_INTERVAL(30), BT_GAP_MS_TO_ADV_INTERVAL(30), NULL);

	err = bt_le_adv_start(&peer_param, peer_ad, ARRAY_SIZE(peer_ad), NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed to start (err %d)", err);

	TEST_PASS("Peer advertising");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "public_conn",
		.test_descr = "Public identity, connectable advertiser, then scanner",
		.test_main_f = test_public_conn,
	},
	{
		.test_id = "public_use_identity",
		.test_descr = "Public identity, identity advertiser, then scanner",
		.test_main_f = test_public_use_identity,
	},
	{
		.test_id = "public_dir",
		.test_descr = "Public identity, directed advertiser, then scanner",
		.test_main_f = test_public_dir,
	},
	{
		.test_id = "public_dir_rpa",
		.test_descr = "Public identity, directed advertiser to an RPA, then scanner",
		.test_main_f = test_public_dir_rpa,
	},
	{
		.test_id = "random_conn",
		.test_descr = "Static random identity, connectable advertiser, then scanner",
		.test_main_f = test_random_conn,
	},
	{
		.test_id = "peer",
		.test_descr = "Advertise for the scanner",
		.test_main_f = test_peer,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *test_scan_adv_shared_addr_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {test_scan_adv_shared_addr_install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
