/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"
#include "argparse.h"
#include "bs_pc_backchannel.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/__assert.h>

/* Custom Service Variables */
static const struct bt_uuid_128 test_svc_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 test_svc_uuid_2 = BT_UUID_INIT_128(
	0xf1, 0xdd, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 test_chrc_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static uint8_t test_value[] = { 'T', 'e', 's', 't', '\0' };

DEFINE_FLAG(flag_client_read);

static ssize_t read_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	printk("Client has read from test char\n");
	SET_FLAG(flag_client_read);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

void wait_for_client_read(void)
{
	WAIT_FOR_FLAG(flag_client_read);
}

static ssize_t write_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;

	printk("Client has written to test char\n");

	if (offset + len > sizeof(test_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr test_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test_svc_uuid),

	BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ |
			       BT_GATT_PERM_WRITE,
			       read_test, write_test, test_value),
};

static struct bt_gatt_attr test_attrs_2[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test_svc_uuid_2),

	BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       read_test, write_test, test_value),
};

static struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);
static struct bt_gatt_service test_svc_2 = BT_GATT_SERVICE(test_attrs_2);

void gatt_register_service_1(void)
{
	int err = bt_gatt_service_register(&test_svc);

	__ASSERT(!err, "Failed to register GATT service (err %d)\n", err);
}

void gatt_register_service_2(void)
{
	/* This service is only used to trigger a GATT DB change.
	 * No reads or writes will be attempted.
	 */
	int err = bt_gatt_service_register(&test_svc_2);

	__ASSERT(!err, "Failed to register GATT service (err %d)\n", err);
}

/* We need to discover:
 * - Dynamic service
 * - Client Features (to set robust caching)
 * - Service Changed (to sub to indications)
 */
enum GATT_HANDLES {
CLIENT_FEATURES,
SERVICE_CHANGED,
TEST_CHAR,
NUM_HANDLES,
};

uint16_t gatt_handles[NUM_HANDLES] = {0};

DEFINE_FLAG(flag_discovered);

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	if (attr == NULL) {
		for (int i = 0; i < ARRAY_SIZE(gatt_handles); i++) {
			printk("handle[%d] = 0x%x\n", i, gatt_handles[i]);

			if (gatt_handles[i] == 0) {
				FAIL("Did not discover all characteristics\n");
			}
		}

		(void)memset(params, 0, sizeof(*params));

		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		const struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_GATT_CLIENT_FEATURES) == 0) {
			printk("Found client supported features\n");
			gatt_handles[CLIENT_FEATURES] = chrc->value_handle;

		} else if (bt_uuid_cmp(chrc->uuid, BT_UUID_GATT_SC) == 0) {
			printk("Found service changed\n");
			gatt_handles[SERVICE_CHANGED] = chrc->value_handle;

		} else if (bt_uuid_cmp(chrc->uuid, &test_chrc_uuid.uuid) == 0) {
			printk("Found test characteristic\n");
			gatt_handles[TEST_CHAR] = chrc->value_handle;
		}
	}

	return BT_GATT_ITER_CONTINUE;
}

DEFINE_FLAG(flag_sc_indicated);
static uint8_t sc_indicated(struct bt_conn *conn,
			    struct bt_gatt_subscribe_params *params,
			    const void *data, uint16_t length)
{
	if (!data) {
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	printk("SC received\n");
	SET_FLAG(flag_sc_indicated);

	return BT_GATT_ITER_CONTINUE;
}

void wait_for_sc_indication(void)
{
	WAIT_FOR_FLAG(flag_sc_indicated);
}

DEFINE_FLAG(flag_sc_subscribed);
static void sc_subscribed(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_subscribe_params *params)
{
	if (params->value) {
		printk("SC subscribed\n");
		SET_FLAG(flag_sc_subscribed);
	} else {
		printk("SC unsubscribed\n");
		UNSET_FLAG(flag_sc_subscribed);
	}
}

static struct bt_gatt_discover_params disc_params;
static struct bt_gatt_subscribe_params subscribe_params;
void gatt_subscribe_to_service_changed(bool subscribe)
{
	int err;

	subscribe_params.value_handle = gatt_handles[SERVICE_CHANGED];
	subscribe_params.notify = sc_indicated;
	subscribe_params.subscribe = sc_subscribed;

	if (subscribe) {
		subscribe_params.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
		subscribe_params.disc_params = &disc_params,
		subscribe_params.value = BT_GATT_CCC_INDICATE;
		subscribe_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

		err = bt_gatt_subscribe(get_conn(), &subscribe_params);
		WAIT_FOR_FLAG(flag_sc_subscribed);
	} else {
		/* Params are already set to the correct values by the previous
		 * call of this fn.
		 */
		err = bt_gatt_unsubscribe(get_conn(), &subscribe_params);
		WAIT_FOR_FLAG_UNSET(flag_sc_subscribed);
	}

	if (err != 0) {
		FAIL("Subscription failed(err %d)\n", err);
	} else {
		printk("%subscribed %s SC indications\n",
		       subscribe ? "S" : "Uns",
		       subscribe ? "to" : "from");
	}
}

void gatt_discover(void)
{
	static struct bt_gatt_discover_params discover_params;
	int err;

	printk("Discovering services and characteristics\n");
	UNSET_FLAG(flag_discovered);

	discover_params.uuid = NULL;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(get_conn(), &discover_params);
	if (err != 0) {
		FAIL("Discover failed(err %d)\n", err);
	}

	WAIT_FOR_FLAG(flag_discovered);
	printk("Discover complete\n");
}

DEFINE_FLAG(flag_written);

static void write_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	if (err != BT_ATT_ERR_SUCCESS) {
		FAIL("Write failed: 0x%02X\n", err);
	}

	SET_FLAG(flag_written);
}

#define CF_BIT_ROBUST_CACHING 0
void activate_robust_caching(void)
{
	int err;

	static const uint8_t csf = BIT(CF_BIT_ROBUST_CACHING);
	static struct bt_gatt_write_params write_params = {
		.func = write_cb,
		.offset = 0,
		.data = &csf,
		.length = sizeof(csf),
	};

	write_params.handle = gatt_handles[CLIENT_FEATURES];

	UNSET_FLAG(flag_written);
	err = bt_gatt_write(get_conn(), &write_params);

	__ASSERT(!err, "Failed to enable robust caching\n");

	WAIT_FOR_FLAG(flag_written);
	printk("Robust caching enabled\n");
}

DEFINE_FLAG(flag_read);

static uint8_t _expect_success(struct bt_conn *conn, uint8_t err,
			       struct bt_gatt_read_params *params,
			       const void *data, uint16_t length)
{
	/* printk("GATT read cb: err 0x%02X\n", err); */
	__ASSERT(err == 0, "Failed to read: err 0x%x\n", err);

	SET_FLAG(flag_read);

	return 0;
}

static uint8_t _expect_out_of_sync_cb(struct bt_conn *conn, uint8_t err,
				     struct bt_gatt_read_params *params,
				     const void *data, uint16_t length)
{
	/* printk("GATT read cb: err 0x%02X\n", err); */
	__ASSERT(err == BT_ATT_ERR_DB_OUT_OF_SYNC,
		 "Didn't get expected error code: err 0x%x\n", err);

	SET_FLAG(flag_read);

	return 0;
}

static void read_char(uint16_t handle, bool expect_success)
{
	int err;

	struct bt_gatt_read_params read_params = {
		.handle_count = 1,
		.single = {
			.handle = handle,
			.offset = 0,
		},
	};

	if (expect_success) {
		read_params.func = _expect_success;
	} else {
		read_params.func = _expect_out_of_sync_cb;
	}

	UNSET_FLAG(flag_read);

	err = bt_gatt_read(get_conn(), &read_params);
	__ASSERT(!err, "Failed to read char\n");

	WAIT_FOR_FLAG(flag_read);
}

void read_test_char(bool expect_success)
{
	read_char(gatt_handles[TEST_CHAR], expect_success);
}

void gatt_clear_flags(void)
{
	UNSET_FLAG(flag_client_read);
	UNSET_FLAG(flag_discovered);
	UNSET_FLAG(flag_sc_indicated);
	UNSET_FLAG(flag_sc_subscribed);
	UNSET_FLAG(flag_written);
	UNSET_FLAG(flag_read);
}
