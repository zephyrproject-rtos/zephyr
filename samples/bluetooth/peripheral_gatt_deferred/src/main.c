/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#define DEFERRED_SERVICE_UUID BT_UUID_128_ENCODE(0x3a1c0b00, 0x7f2d, 0x4e51, 0x9c6a, 0x1d8b4f0e2a73)
#define DEFERRED_VALUE_UUID   BT_UUID_128_ENCODE(0x3a1c0b01, 0x7f2d, 0x4e51, 0x9c6a, 0x1d8b4f0e2a73)

/* Stands in for however long the value really takes to fetch or to store */
#define FETCH_TIME K_MSEC(100)

static const struct bt_uuid_128 deferred_service_uuid = BT_UUID_INIT_128(DEFERRED_SERVICE_UUID);
static const struct bt_uuid_128 deferred_value_uuid = BT_UUID_INIT_128(DEFERRED_VALUE_UUID);

static uint8_t deferred_value;

/* Only valid during the callback, so it is copied. One slot is enough: this
 * sample has a single bearer and the host serves one request per bearer. The
 * copy holds no reference to the connection, so one is taken while the
 * request is kept and dropped once it has a response.
 */
static struct bt_gatt_req pending_req;
static uint8_t pending_write;
static bool pending_is_read;

static void respond(struct k_work *work)
{
	struct bt_gatt_req req = pending_req;
	uint8_t value = pending_write;
	bool is_read = pending_is_read;
	int err;

	ARG_UNUSED(work);

	pending_req.conn = NULL;

	if (is_read) {
		err = bt_gatt_read_rsp(&req, &deferred_value, sizeof(deferred_value));
	} else {
		deferred_value = value;
		err = bt_gatt_write_rsp(&req);
	}

	/* Done with the request, and with its connection */
	bt_conn_drop(&req.conn);

	printk("Responded from the work queue (err %d)\n", err);
}

static K_WORK_DELAYABLE_DEFINE(respond_work, respond);

static int read_deferred(const struct bt_gatt_req *req, const struct bt_gatt_attr *attr,
		      uint16_t offset, uint16_t max_len)
{
	ARG_UNUSED(attr);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_ATTRIBUTE_NOT_LONG);
	}

	if (pending_req.conn != NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	pending_req = *req;
	pending_is_read = true;
	(void)bt_conn_ref(pending_req.conn);
	k_work_schedule(&respond_work, FETCH_TIME);

	printk("Read deferred, room for %u octets\n", max_len);

	return 0;
}

static int write_deferred(const struct bt_gatt_req *req, const struct bt_gatt_attr *attr,
		       const void *buf, uint16_t len, uint16_t offset,
		       enum bt_gatt_attr_write_flag flags)
{
	const uint8_t *value = buf;

	ARG_UNUSED(attr);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(deferred_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (req->id == 0U) {
		/* A request with id 0 cannot be responded to later: a Write
		 * Command, or an Execute Write whose queue also holds a
		 * synchronous attribute. Complete the write before returning.
		 */
		deferred_value = value[0];
		printk("Write applied right away, flags 0x%02x\n", flags);

		return 0;
	}

	if (pending_req.conn != NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	pending_req = *req;
	pending_write = value[0];
	pending_is_read = false;
	(void)bt_conn_ref(pending_req.conn);
	k_work_schedule(&respond_work, FETCH_TIME);

	printk("Write of 0x%02x deferred\n", value[0]);

	return 0;
}

BT_GATT_SERVICE_DEFINE(deferred_svc, BT_GATT_PRIMARY_SERVICE(&deferred_service_uuid),
		       BT_GATT_CHARACTERISTIC_DEFERRED(&deferred_value_uuid.uuid,
						    BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
						    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
						    read_deferred, write_deferred, NULL));

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, DEFERRED_SERVICE_UUID),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);

		return 0;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err != 0) {
		printk("Advertising failed to start (err %d)\n", err);

		return 0;
	}

	printk("Advertising as %s\n", CONFIG_BT_DEVICE_NAME);

	return 0;
}
