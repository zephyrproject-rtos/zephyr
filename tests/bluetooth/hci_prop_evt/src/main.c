/* main.c - HCI prop event test */

/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <errno.h>
#include <string.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/net_buf.h>

#include <fake_hci.h>

/* HCI Proprietary vendor event */
const uint8_t hci_prop_evt_prefix[2] = { 0xAB, 0xBA };

struct hci_evt_prop {
	uint8_t  prefix[2];
} __packed;

struct hci_evt_prop_report {
	uint8_t  data_len;
	uint8_t  data[0];
} __packed;

/* Semaphore to test if the prop callback was called. */
static K_SEM_DEFINE(prop_cb_sem, 0, 1);

/* Used to verify prop event data. */
static uint8_t *prop_cb_data;
static uint8_t prop_cb_data_len;

/* Prop callback. */
static bool prop_cb(struct net_buf_simple *buf)
{
	struct hci_evt_prop *pe;

	pe = net_buf_simple_pull_mem(buf, sizeof(*pe));

	if (memcmp(&pe->prefix[0], &hci_prop_evt_prefix[0],
		   ARRAY_SIZE(hci_prop_evt_prefix)) == 0) {
		struct hci_evt_prop_report *per;

		per = net_buf_simple_pull_mem(buf, sizeof(*per));

		uint8_t data_len = per->data_len;
		uint8_t *data = &per->data[0];

		/* Allocate memory for storing the data */
		prop_cb_data = k_malloc(data_len);
		zassert_not_null(prop_cb_data, "Cannot allocate memory");

		/* Copy data so it can be verified later */
		memcpy(prop_cb_data, data, data_len);
		prop_cb_data_len = data_len;

		/* Give control back to test */
		k_sem_give(&prop_cb_sem);

		return true;
	}

	return false;
}

/* Send a prop event report with the given data. */
static void send_prop_report(uint8_t *data, uint8_t data_len)
{
	struct hci_evt_prop_report *per;
	struct hci_evt_prop *pe;
	struct net_buf *buf;

	/* HCI Vendor Specific event carrying the prop event report */
	buf = fake_hci_evt(BT_HCI_EVT_VENDOR, sizeof(*pe) + sizeof(*per) + data_len);
	pe = net_buf_add(buf, sizeof(*pe));
	(void)memcpy(&pe->prefix[0], &hci_prop_evt_prefix[0], ARRAY_SIZE(hci_prop_evt_prefix));
	per = net_buf_add(buf, sizeof(*per));
	per->data_len = data_len;
	(void)net_buf_add_mem(buf, data, data_len);

	fake_hci_recv(buf);
}

ZTEST_SUITE(test_hci_prop_evt, NULL, NULL, NULL, NULL, NULL);

/* Test. */
ZTEST(test_hci_prop_evt, test_hci_prop_evt_entry)
{
	/* Bogus Controller features */
	(void)memset(fake_hci.features, 0xFF, sizeof(fake_hci.features));
	(void)memset(fake_hci.le_features, 0xFF, sizeof(fake_hci.le_features));

	/* Go! Wait until Bluetooth initialization is done  */
	zassert_true((bt_enable(NULL) == 0),
		     "bt_enable failed");

	/* Register the prop callback */
	bt_hci_register_vnd_evt_cb(prop_cb);

	/* Generate some data */
	uint8_t data_len = 10;
	uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	/* Send the prop event report */
	send_prop_report(&data[0], data_len);

	/* Wait for the prop callback to be called */
	zassert_true(k_sem_take(&prop_cb_sem, K_MSEC(100)) == 0,
		     "prop_cb was not called within timeout");

	/* Verify the data length */
	zassert_true(prop_cb_data_len == data_len,
		     "prop_cb_data_len invalid");

	/* Verify the data itself */
	zassert_true(memcmp(prop_cb_data, data, data_len) == 0,
		     "prop_cb_data invalid");

	/* Free the data memory */
	k_free(prop_cb_data);
}
