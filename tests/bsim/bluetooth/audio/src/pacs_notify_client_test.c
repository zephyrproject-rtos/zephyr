/*
 * Copyright (c) 2023 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

#include "common.h"
#include "common/bt_str.h"

struct pacs_instance_t {
	uint16_t start_handle;
	uint16_t end_handle;

	struct bt_gatt_subscribe_params sink_pacs_sub;
	struct bt_gatt_subscribe_params source_pacs_sub;
	struct bt_gatt_subscribe_params sink_loc_sub;
	struct bt_gatt_subscribe_params source_loc_sub;
	struct bt_gatt_subscribe_params available_contexts_sub;
	struct bt_gatt_subscribe_params supported_contexts_sub;

	struct bt_gatt_discover_params discover_params;

	int notify_received_mask;
};

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_pacs_snk_discovered);
CREATE_FLAG(flag_pacs_src_discovered);
CREATE_FLAG(flag_snk_loc_discovered);
CREATE_FLAG(flag_src_loc_discovered);
CREATE_FLAG(flag_available_contexts_discovered);
CREATE_FLAG(flag_supported_contexts_discovered);
CREATE_FLAG(flag_all_notifications_received);

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);

static struct pacs_instance_t pacs_instance;

static uint8_t pacs_notify_handler(struct bt_conn *conn,
				  struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t length)
{
	printk("%p\n", params);

	if (params == &pacs_instance.sink_pacs_sub) {
		printk("Received sink_pacs_sub notification\n");
		pacs_instance.notify_received_mask |= BIT(0);
	} else if (params == &pacs_instance.source_pacs_sub) {
		printk("Received source_pacs_sub notification\n");
		pacs_instance.notify_received_mask |= BIT(1);
	} else if (params == &pacs_instance.sink_loc_sub) {
		printk("Received sink_loc_sub notification\n");
		pacs_instance.notify_received_mask |= BIT(2);
	} else if (params == &pacs_instance.source_loc_sub) {
		printk("Received source_loc_sub notification\n");
		pacs_instance.notify_received_mask |= BIT(3);
	} else if (params == &pacs_instance.available_contexts_sub) {
		printk("Received available_contexts_sub notification\n");
		pacs_instance.notify_received_mask |= BIT(4);
	} else if (params == &pacs_instance.supported_contexts_sub) {
		printk("Received supported_contexts_sub notification\n");
		pacs_instance.notify_received_mask |= BIT(5);
	}

	printk("pacs_instance.notify_received_mask is %d\n", pacs_instance.notify_received_mask);

	if (pacs_instance.notify_received_mask ==
	    (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5))) {
		pacs_instance.notify_received_mask = 0;
		SET_FLAG(flag_all_notifications_received);
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_supported_contexts(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_PACS_SUPPORTED_CONTEXT)) {
		printk("PACS Supported Contexts Characteristic handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.supported_contexts_sub;
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		pacs_instance.discover_params.uuid = &uuid.uuid;
		pacs_instance.discover_params.start_handle = attr->handle + 2;
		pacs_instance.discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &pacs_instance.discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		printk("CCC handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.supported_contexts_sub;
		subscribe_params->notify = pacs_notify_handler;
		subscribe_params->value = BT_GATT_CCC_NOTIFY;
		subscribe_params->ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			SET_FLAG(flag_supported_contexts_discovered);
			printk("[SUBSCRIBED]\n");
		}
	} else {
		printk("Unknown handle at %d\n", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_supported_contexts(void)
{
	int err = 0;

	printk("%s\n", __func__);

	memcpy(&uuid, BT_UUID_PACS_SUPPORTED_CONTEXT, sizeof(uuid));
	pacs_instance.discover_params.uuid = &uuid.uuid;
	pacs_instance.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	pacs_instance.discover_params.func = discover_supported_contexts;

	err = bt_gatt_discover(default_conn, &pacs_instance.discover_params);
	if (err != 0) {
		FAIL("Service Discovery failed (err %d)\n", err);
		return;
	}
}

static uint8_t discover_available_contexts(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_PACS_AVAILABLE_CONTEXT)) {
		printk("PACS Available Contexts Characteristic handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.available_contexts_sub;
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		pacs_instance.discover_params.uuid = &uuid.uuid;
		pacs_instance.discover_params.start_handle = attr->handle + 2;
		pacs_instance.discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &pacs_instance.discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		printk("CCC handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.available_contexts_sub;
		subscribe_params->notify = pacs_notify_handler;
		subscribe_params->value = BT_GATT_CCC_NOTIFY;
		subscribe_params->ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			SET_FLAG(flag_available_contexts_discovered);
			printk("[SUBSCRIBED]\n");
		}
	} else {
		printk("Unknown handle at %d\n", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_available_contexts(void)
{
	int err = 0;

	printk("%s\n", __func__);

	memcpy(&uuid, BT_UUID_PACS_AVAILABLE_CONTEXT, sizeof(uuid));
	pacs_instance.discover_params.uuid = &uuid.uuid;
	pacs_instance.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	pacs_instance.discover_params.func = discover_available_contexts;

	err = bt_gatt_discover(default_conn, &pacs_instance.discover_params);
	if (err != 0) {
		FAIL("Service Discovery failed (err %d)\n", err);
		return;
	}
}

static uint8_t discover_src_loc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_PACS_SRC_LOC)) {
		printk("PACS Source Location Characteristic handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.source_loc_sub;
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		pacs_instance.discover_params.uuid = &uuid.uuid;
		pacs_instance.discover_params.start_handle = attr->handle + 2;
		pacs_instance.discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &pacs_instance.discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		printk("CCC handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.source_loc_sub;
		subscribe_params->notify = pacs_notify_handler;
		subscribe_params->value = BT_GATT_CCC_NOTIFY;
		subscribe_params->ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			SET_FLAG(flag_src_loc_discovered);
			printk("[SUBSCRIBED]\n");
		}
	} else {
		printk("Unknown handle at %d\n", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_src_loc(void)
{
	int err = 0;

	printk("%s\n", __func__);

	memcpy(&uuid, BT_UUID_PACS_SRC_LOC, sizeof(uuid));
	pacs_instance.discover_params.uuid = &uuid.uuid;
	pacs_instance.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	pacs_instance.discover_params.func = discover_src_loc;

	err = bt_gatt_discover(default_conn, &pacs_instance.discover_params);
	if (err != 0) {
		FAIL("Service Discovery failed (err %d)\n", err);
		return;
	}
}

static uint8_t discover_snk_loc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_PACS_SNK_LOC)) {
		printk("PACS Sink Location Characteristic handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.sink_loc_sub;
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		pacs_instance.discover_params.uuid = &uuid.uuid;
		pacs_instance.discover_params.start_handle = attr->handle + 2;
		pacs_instance.discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &pacs_instance.discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		printk("CCC handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.sink_loc_sub;
		subscribe_params->notify = pacs_notify_handler;
		subscribe_params->value = BT_GATT_CCC_NOTIFY;
		subscribe_params->ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			SET_FLAG(flag_snk_loc_discovered);
			printk("[SUBSCRIBED]\n");
		}
	} else {
		printk("Unknown handle at %d\n", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_snk_loc(void)
{
	int err = 0;

	printk("%s\n", __func__);

	memcpy(&uuid, BT_UUID_PACS_SNK_LOC, sizeof(uuid));
	pacs_instance.discover_params.uuid = &uuid.uuid;
	pacs_instance.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	pacs_instance.discover_params.func = discover_snk_loc;

	err = bt_gatt_discover(default_conn, &pacs_instance.discover_params);
	if (err != 0) {
		FAIL("Service Discovery failed (err %d)\n", err);
		return;
	}
}

static uint8_t discover_pacs_src(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_PACS_SRC)) {
		printk("PACS Source Characteristic handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.source_pacs_sub;
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		pacs_instance.discover_params.uuid = &uuid.uuid;
		pacs_instance.discover_params.start_handle = attr->handle + 2;
		pacs_instance.discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &pacs_instance.discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		printk("CCC handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.source_pacs_sub;
		subscribe_params->notify = pacs_notify_handler;
		subscribe_params->value = BT_GATT_CCC_NOTIFY;
		subscribe_params->ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			SET_FLAG(flag_pacs_src_discovered);
			printk("[SUBSCRIBED]\n");
		}
	} else {
		printk("Unknown handle at %d\n", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_src_pacs(void)
{
	int err = 0;

	printk("%s\n", __func__);

	memcpy(&uuid, BT_UUID_PACS_SRC, sizeof(uuid));
	pacs_instance.discover_params.uuid = &uuid.uuid;
	pacs_instance.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	pacs_instance.discover_params.func = discover_pacs_src;

	err = bt_gatt_discover(default_conn, &pacs_instance.discover_params);
	if (err != 0) {
		FAIL("Service Discovery failed (err %d)\n", err);
		return;
	}
}

static uint8_t discover_pacs_snk(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_subscribe_params *subscribe_params;
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(params->uuid, BT_UUID_PACS_SNK)) {
		printk("PACS Sink Characteristic handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.sink_pacs_sub;
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		pacs_instance.discover_params.uuid = &uuid.uuid;
		pacs_instance.discover_params.start_handle = attr->handle + 2;
		pacs_instance.discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params->value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &pacs_instance.discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC)) {
		printk("CCC handle at %d\n", attr->handle);
		subscribe_params = &pacs_instance.sink_pacs_sub;
		subscribe_params->notify = pacs_notify_handler;
		subscribe_params->value = BT_GATT_CCC_NOTIFY;
		subscribe_params->ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			SET_FLAG(flag_pacs_snk_discovered);
			printk("[SUBSCRIBED]\n");
		}
	} else {
		printk("Unknown handle at %d\n", attr->handle);
		return BT_GATT_ITER_CONTINUE;
	}

	return BT_GATT_ITER_STOP;
}

static void discover_and_subscribe_snk_pacs(void)
{
	int err = 0;

	printk("%s\n", __func__);

	memcpy(&uuid, BT_UUID_PACS_SNK, sizeof(uuid));
	pacs_instance.discover_params.uuid = &uuid.uuid;
	pacs_instance.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	pacs_instance.discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	pacs_instance.discover_params.func = discover_pacs_snk;

	err = bt_gatt_discover(default_conn, &pacs_instance.discover_params);
	if (err != 0) {
		FAIL("Service Discovery failed (err %d)\n", err);
		return;
	}
}

static void test_main(void)
{
	int err;

	printk("Enabling Bluetooth\n");
	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	bt_le_scan_cb_register(&common_scan_cb);

	printk("Starting scan\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scanning (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	printk("Raising security\n");
	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to ser security level %d (err %d)\n", BT_SECURITY_L2, err);
		return;
	}

	printk("Starting Discovery\n");

	discover_and_subscribe_snk_pacs();
	WAIT_FOR_FLAG(flag_pacs_snk_discovered);

	discover_and_subscribe_snk_loc();
	WAIT_FOR_FLAG(flag_snk_loc_discovered);

	discover_and_subscribe_src_pacs();
	WAIT_FOR_FLAG(flag_pacs_src_discovered);

	discover_and_subscribe_src_loc();
	WAIT_FOR_FLAG(flag_src_loc_discovered);

	discover_and_subscribe_available_contexts();
	WAIT_FOR_FLAG(flag_available_contexts_discovered);

	discover_and_subscribe_supported_contexts();
	WAIT_FOR_FLAG(flag_supported_contexts_discovered);

	printk("Waiting for all notifications to be received\n");

	WAIT_FOR_FLAG(flag_all_notifications_received);

	/* Disconnect and wait for server to advertise again (after notifications are triggered) */
	UNSET_FLAG(flag_all_notifications_received);
	bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	printk("Starting scan\n");
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Could not start scanning (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	printk("Raising security\n");
	err = bt_conn_set_security(default_conn, BT_SECURITY_L2);
	if (err) {
		FAIL("Failed to ser security level %d (err %d)\n", BT_SECURITY_L2, err);
		return;
	}

	printk("Waiting for all notifications to be received\n");
	WAIT_FOR_FLAG(flag_all_notifications_received);

	bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	PASS("GATT client Passed\n");
}

static const struct bst_test_instance test_pacs_notify_client[] = {
	{
		.test_id = "pacs_notify_client",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_pacs_notify_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_pacs_notify_client);
}
