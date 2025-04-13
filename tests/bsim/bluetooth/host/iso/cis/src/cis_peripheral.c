/*
 * Copyright (c) 2023-2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <testlib/conn.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"
#include "bstests.h"
#include "common.h"

extern enum bst_result_t bst_result;

DEFINE_FLAG_STATIC(flag_data_received);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};
static struct bt_iso_chan iso_chan;

/** Print data as d_0 d_1 d_2 ... d_(n-2) d_(n-1) d_(n) to show the 3 first and 3 last octets
 *
 * Examples:
 * 01
 * 0102
 * 010203
 * 01020304
 * 0102030405
 * 010203040506
 * 010203...050607
 * 010203...060708
 * etc.
 */
static void iso_print_data(uint8_t *data, size_t data_len)
{
	/* Maximum number of octets from each end of the data */
	const uint8_t max_octets = 3;
	char data_str[35];
	size_t str_len;

	str_len = bin2hex(data, MIN(max_octets, data_len), data_str, sizeof(data_str));
	if (data_len > max_octets) {
		if (data_len > (max_octets * 2)) {
			static const char dots[] = "...";

			strcat(&data_str[str_len], dots);
			str_len += strlen(dots);
		}

		str_len += bin2hex(data + (data_len - MIN(max_octets, data_len - max_octets)),
				   MIN(max_octets, data_len - max_octets), data_str + str_len,
				   sizeof(data_str) - str_len);
	}

	printk("\t %s\n", data_str);
}

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		     struct net_buf *buf)
{
	if (info->flags & BT_ISO_FLAGS_VALID) {
		printk("Incoming data channel %p len %u\n", chan, buf->len);
		iso_print_data(buf->data, buf->len);
		SET_FLAG(flag_data_received);
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	int err;

	printk("ISO Channel %p connected\n", chan);

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST, &hci_path);
	TEST_ASSERT(err == 0, "Failed to set ISO data path: %d", err);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected (reason 0x%02x)\n", chan, reason);
}

static int iso_accept(const struct bt_iso_accept_info *info, struct bt_iso_chan **chan)
{
	printk("Incoming request from %p\n", (void *)info->acl);

	if (iso_chan.iso) {
		TEST_FAIL("No channels available");

		return -ENOMEM;
	}

	*chan = &iso_chan;

	return 0;
}

static void init(void)
{
	static struct bt_iso_chan_io_qos iso_rx = {
		.sdu = CONFIG_BT_ISO_TX_MTU,
	};
	static struct bt_iso_server iso_server = {
#if defined(CONFIG_BT_SMP)
		.sec_level = BT_SECURITY_L2,
#endif /* CONFIG_BT_SMP */
		.accept = iso_accept,
	};
	static struct bt_iso_chan_ops iso_ops = {
		.recv = iso_recv,
		.connected = iso_connected,
		.disconnected = iso_disconnected,
	};
	static struct bt_iso_chan_qos iso_qos = {
		.rx = &iso_rx,
		.tx = NULL,
	};
	int err;

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth enable failed (err %d)", err);

		return;
	}

	iso_chan.ops = &iso_ops;
	iso_chan.qos = &iso_qos;
#if defined(CONFIG_BT_SMP)
	iso_chan.required_sec_level = BT_SECURITY_L2,
#endif /* CONFIG_BT_SMP */

	err = bt_iso_server_register(&iso_server);
	if (err) {
		TEST_FAIL("Unable to register ISO server (err %d)", err);

		return;
	}
}

static void adv_connect(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		TEST_FAIL("Advertising failed to start (err %d)", err);

		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_connected);
}

static void test_main(void)
{
	init();

	while (true) {
		adv_connect();
		bt_testlib_conn_wait_free();

		if (IS_FLAG_SET(flag_data_received)) {
			TEST_PASS("Test passed");
		}
	}
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "peripheral",
		.test_descr = "Peripheral",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_cis_peripheral_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
