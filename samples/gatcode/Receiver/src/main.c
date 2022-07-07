/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 SixOctets Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* UUIDs - https://www.guidgenerator.com */
#define BT_UUID_PACKET_SERVICE BT_UUID_128_ENCODE(0x595d0001, 0xc3b2, 0x40a2, 0xab93, 0x28e30fa26298)
#define BT_UUID_CHARACTERISTIC_A BT_UUID_128_ENCODE(0x2fe883b0, 0x4305, 0x42f8, 0x8efe, 0x51bafee55253)
#define BT_UUID_CHARACTERISTIC_B BT_UUID_128_ENCODE(0x7a482d03, 0x80cd, 0x4db4, 0xbfa5, 0x103910d89b61)



static int scan_start(void);

static struct bt_conn *default_conn;
static struct bt_uuid_128 uuid = BT_UUID_INIT_128(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static double pow(double x, double y)
{
	double result = 1;

	if (y < 0) {
		y = -y;
		while (y--) {
			result /= x;
		}
	} else {
		while (y--) {
			result *= x;
		}
	}

	return result;
}

static uint8_t received_A_notification(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	printk("received_A_notification\n");
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t received_B_notification(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	printk("received_B_notification\n");
	return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (bt_uuid_cmp(discover_params.uuid, BT_UUID_DECLARE_128(BT_UUID_PACKET_SERVICE)) == 0)
	{
		memcpy(&uuid, BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_A), sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}
	else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_A)) == 0)
	{
		memcpy(&uuid, BT_UUID_DECLARE_128(BT_UUID_GATT_CCC_VAL), sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else {
		subscribe_params.notify = received_A_notification;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}

		return BT_GATT_ITER_STOP;
	}




	






	/* BT_UUID_CHARACTERISTIC_B */
	

	// if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_A))) {
	// 	memcpy(&uuid, BT_UUID_DECLARE_128(BT_UUID_PACKET_SERVICE), sizeof(uuid));
	// 	discover_params.uuid = &uuid.uuid;
	// 	discover_params.start_handle = attr->handle + 1;
	// 	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	// 	err = bt_gatt_discover(conn, &discover_params);
	// 	if (err) {
	// 		printk("Discover failed (err %d)\n", err);
	// 	}
	// } 
	// else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_B))) {
	// 	memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
	// 	discover_params.uuid = &uuid.uuid;
	// 	discover_params.start_handle = attr->handle + 2;
	// 	discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
	// 	subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

	// 	err = bt_gatt_discover(conn, &discover_params);
	// 	if (err) {
	// 		printk("Discover failed (err %d)\n", err);
	// 	}
	// } else {
	// 	subscribe_params.notify = notify_func;
	// 	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	// 	subscribe_params.ccc_handle = attr->handle;

	// 	err = bt_gatt_subscribe(conn, &subscribe_params);
	// 	if (err && err != -EALREADY) {
	// 		printk("Subscribe failed (err %d)\n", err);
	// 	} else {
	// 		printk("[SUBSCRIBED]\n");
	// 	}

	// 	return BT_GATT_ITER_STOP;
	// }

	return BT_GATT_ITER_CONTINUE;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		scan_start();
		return;
	}

	printk("Connected: %s\n", addr);
	gpio_pin_set_dt(&led, 1);

	if (conn == default_conn) {
		memcpy(&uuid, BT_UUID_DECLARE_128(BT_UUID_PACKET_SERVICE), sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		err = bt_gatt_discover(default_conn, &discover_params);
		if (err) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}
}

#define NAME_LEN            30
#define TX_DEVICE_NAME "GatCode's Packet Sender"

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	char name[NAME_LEN];
	uint8_t len;
	int err;

	switch (data->type) {
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';

		if(strcmp(name, TX_DEVICE_NAME) != 0) {
			return true;
		}

		err = bt_le_scan_stop();
		if (err) {
			printk("Stop LE scan failed (err %d)\n", err);
		}

		err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
						BT_LE_CONN_PARAM_DEFAULT,
						&default_conn);
		if (err) {
			printk("Create connection failed (err %d)\n",
					err);
			scan_start();
		}
		
		return false;
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	
	/* We're only interested in connectable events */
	if (type == BT_HCI_ADV_IND || type == BT_HCI_ADV_DIRECT_IND) {
		printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	       dev, type, ad->len, rssi);
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static int scan_start(void)
{
	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime.
	 */
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	return bt_le_scan_start(&scan_param, device_found);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);
	gpio_pin_set_dt(&led, 0);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	err = scan_start();
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err)
{
	/* Check if BLE is running */
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start Scanning */
	err = scan_start();
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

void main(void)
{
	int err;

	/* Initialize the LED */
	if (!device_is_ready(led.port)) {
 		printk("Error setting LED\n");
 	}

 	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
 	if (err < 0) {
 		printk("Error setting LED (err %d)\n", err);
 	}

	/* Initialize the Bluetooth Subsystem and start scanning*/
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}


// #include <zephyr.h>
// #include <sys/printk.h>

// #include <zephyr/device.h>
// #include <zephyr/devicetree.h>
// #include <zephyr/drivers/gpio.h>

// #include <zephyr/types.h>
// #include <stddef.h>
// #include <sys/util.h>
// #include <sys/byteorder.h>
// #include <drivers/sensor.h>

// #include <bluetooth/bluetooth.h>
// #include <bluetooth/uuid.h>
// #include <bluetooth/gatt.h>
// #include <bluetooth/hci.h>
// // #include <bluetooth/scan.h>
// // #include <bluetooth/gatt_dm.h>

// #include <logging/log.h>

// LOG_MODULE_REGISTER(logger);

// #define LED0_NODE DT_ALIAS(led0)
// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

// /* UUIDs - https://www.guidgenerator.com */
// #define BT_UUID_PACKET_SERVICE BT_UUID_128_ENCODE(0x595d0001, 0xc3b2, 0x40a2, 0xab93, 0x28e30fa26298)
// #define BT_UUID_CHARACTERISTIC_A BT_UUID_128_ENCODE(0x2fe883b0, 0x4305, 0x42f8, 0x8efe, 0x51bafee55253)
// #define BT_UUID_CHARACTERISTIC_B BT_UUID_128_ENCODE(0x7a482d03, 0x80cd, 0x4db4, 0xbfa5, 0x103910d89b61)

// static struct bt_conn *default_conn;
// static struct bt_gatt_subscribe_params subscribe_params;
// static struct bt_gatt_subscribe_params subscribe_params_temp;

// static void auth_cancel(struct bt_conn *conn) {
// 	char addr[BT_ADDR_LE_STR_LEN];
// 	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
// 	LOG_INF("Pairing cancelled: %s", log_strdup(addr));
// }

// static void pairing_confirm(struct bt_conn *conn) {
// 	char addr[BT_ADDR_LE_STR_LEN];
// 	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
// 	bt_conn_auth_pairing_confirm(conn);
// 	LOG_INF("Pairing confirmed: %s", log_strdup(addr));
// }

// static void pairing_complete(struct bt_conn *conn, bool bonded) {
// 	char addr[BT_ADDR_LE_STR_LEN];
// 	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
// 	LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr),
// 		bonded);
// }

// static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
// 	char addr[BT_ADDR_LE_STR_LEN];
// 	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
// 	LOG_WRN("Pairing failed conn: %s, reason %d", log_strdup(addr),
// 		reason);
// }

// static uint8_t received_A_notification (struct bt_conn *conn,
// 			struct bt_gatt_subscribe_params *params,
// 			const void *data, uint16_t length)
// {
// 	if (!data) {
// 		LOG_INF("[UNSUBSCRIBED]");
// 		return BT_GATT_ITER_STOP;
// 	}

//     uint16_t state_reading = ((uint8_t*)data)[0];
// 	LOG_INF("[NOTIFICATION] start/stop state: %d", state_reading == 0 ? "OFF" : "ON");

// 	return BT_GATT_ITER_CONTINUE;
// }

// static uint8_t received_B_notification (struct bt_conn *conn,
// 			struct bt_gatt_subscribe_params *params,
// 			const void *data, uint16_t length)
// {
// 	if (!data) {
// 		LOG_INF("[UNSUBSCRIBED]");
// 		return BT_GATT_ITER_STOP;
// 	}

// 	LOG_INF("[NOTIFICATION] audio:");

// 	uint8_t packetSize = 10;
// 	for(uint8_t i = 0; i < packetSize; i++) {
// 		printk("%i ", ((uint8_t*)data)[i]);
// 	}
// 	printk("\n");

// 	return BT_GATT_ITER_CONTINUE;
// }

// static void discovery_complete(struct bt_gatt_dm *dm,
// 							   void *context)
// {
// 	LOG_INF("Service discovery completed");

// 	bt_gatt_dm_data_print(dm);

// 	// link gatt service characteristics with my central
// 	const struct bt_gatt_dm_attr *gatt_service_attr = bt_gatt_dm_service_get(dm);
// 	const struct bt_gatt_service_val *gatt_service = bt_gatt_dm_attr_service_val(gatt_service_attr);
// 	const struct bt_gatt_dm_attr *gatt_chrc;
// 	const struct bt_gatt_dm_attr *gatt_desc;

// 	/* Start Stop Characteristic */
// 	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_A));
// 	if (!gatt_chrc) {
// 		LOG_ERR("Missing start stop characteristic.");
// 		return -EINVAL;
// 	}

//     /* Start Stop */
// 	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_A));
// 	if (!gatt_desc) {
// 		LOG_ERR("Missing start stop value descriptor in characteristic.");
// 		return -EINVAL;
// 	}
// 	LOG_INF("Found handle for start stop characteristic.");
// 	uint16_t start_stop_handle = gatt_desc->handle;

//     /* Start Stop CCC */
// 	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
// 	if (!gatt_desc) {
// 		LOG_ERR("Missing start stop CCC in characteristic.");
// 		return -EINVAL;
// 	}
//     LOG_INF("Found handle for CCC of start stop characteristic.");
// 	uint16_t start_stop_ccc_handle = gatt_desc->handle;

//     int err;
//     subscribe_params.notify = received_A_notification;
//     subscribe_params.value = BT_GATT_CCC_NOTIFY;
// 	subscribe_params.value_handle = start_stop_handle;
// 	subscribe_params.ccc_handle = start_stop_ccc_handle;
// 	atomic_set_bit(subscribe_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

// 	err = bt_gatt_subscribe(default_conn, &subscribe_params);
// 	if (err) {
// 		LOG_ERR("Subscribe failed (err %d)", err);
// 	} else {
// 		LOG_INF("[SUBSCRIBED]");
// 	}

//     /* Audio Characteristic */
// 	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_B));
// 	if (!gatt_chrc) {
// 		LOG_ERR("Missing audio characteristic.");
// 		return -EINVAL;
// 	}

//     /* Audio */
// 	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_DECLARE_128(BT_UUID_CHARACTERISTIC_B));
// 	if (!gatt_desc) {
// 		LOG_ERR("Missing audio value descriptor in characteristic.");
// 		return -EINVAL;
// 	}
// 	LOG_INF("Found handle for audio characteristic.");
// 	uint16_t audio_handle = gatt_desc->handle;

//     /* Audio CCC */
// 	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
// 	if (!gatt_desc) {
// 		LOG_ERR("Missing audio CCC in characteristic.");
// 		return -EINVAL;
// 	}
//     LOG_INF("Found handle for CCC of audio characteristic.");
// 	uint16_t audio_ccc_handle = gatt_desc->handle;

//     subscribe_params_temp.notify = received_B_notification;
//     subscribe_params_temp.value = BT_GATT_CCC_NOTIFY;
// 	subscribe_params_temp.value_handle = audio_handle;
// 	subscribe_params_temp.ccc_handle = audio_ccc_handle;
// 	atomic_set_bit(subscribe_params_temp.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

// 	err = bt_gatt_subscribe(default_conn, &subscribe_params_temp);
// 	if (err) {
// 		LOG_ERR("Subscribe failed (err %d)", err);
// 	} else {
// 		LOG_INF("[SUBSCRIBED]");
// 	}

// 	bt_gatt_dm_data_release(dm);

// 	LOG_INF("Service discovery done");
// }

// static void discovery_service_not_found(struct bt_conn *conn,
// 										void *context)
// {
// 	LOG_INF("Service not found");
// }

// static void discovery_error(struct bt_conn *conn,
// 							int err,
// 							void *context)
// {
// 	LOG_WRN("Error while discovering GATT database: (%d)", err);
// }

// struct bt_gatt_dm_cb discovery_cb = {
// 	.completed = discovery_complete,
// 	.service_not_found = discovery_service_not_found,
// 	.error_found = discovery_error,
// };

// static void gatt_discover(struct bt_conn *conn)
// {
// 	int err;

// 	if (conn != default_conn)
// 	{
// 		return;
// 	}

// 	// // LOG_INF("Started to discover gatt server table");
// 	// err = bt_gatt_dm_start(conn,
// 	// 					   BT_UUID_DECLARE_128(BT_UUID_PACKET_SERVICE),
// 	// 					   &discovery_cb,
// 	// 					   NULL);
	
// 	if (err)
// 	{
// 		LOG_ERR("could not start the discovery procedure, error "
// 				"code: %d",
// 				err);
// 	}
// }

// static struct bt_conn_auth_cb conn_auth_callbacks = {
// 	.cancel = auth_cancel,
// 	.pairing_confirm = pairing_confirm,
// 	.pairing_complete = pairing_complete,
// 	.pairing_failed = pairing_failed
// };

// static void connected(struct bt_conn *conn, uint8_t err) {
// 	if (err) {
// 		LOG_ERR("Connection failed (err 0x%02x)\n", err);
// 		return;
// 	}

// 	default_conn = bt_conn_ref(conn);
// 	LOG_INF("Connected.");
// 	gpio_pin_set_dt(&led, 1);
	
// 	gatt_discover(conn);

// 	err = bt_scan_stop();
// 	if ((!err) && (err != -EALREADY))
// 	{
// 		LOG_ERR("Stop LE scan failed (err %d)", err);
// 	}
// 	LOG_INF("Stopped scanning");
// }

// static void disconnected(struct bt_conn *conn, uint8_t reason) {
// 	LOG_WRN("Disconnected (reason 0x%02x)\n", reason);

// 	gpio_pin_set_dt(&led, 0);

// 	if (default_conn) {
// 		bt_conn_unref(default_conn);
// 		default_conn = NULL;
// 	}
// }

// static struct bt_conn_cb conn_callbacks = {
// 	.connected = connected,
// 	.disconnected = disconnected
// };

// // static void scan_filter_match(struct bt_scan_device_info *device_info,
// // 			      struct bt_scan_filter_match *filter_match,
// // 			      bool connectable) {
// // 	char addr[BT_ADDR_LE_STR_LEN];

// // 	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

// // 	LOG_INF("Filters matched. Address: %s connectable: %d",
// // 		log_strdup(addr), connectable);
// // }

// // static void scan_connecting_error(struct bt_scan_device_info *device_info) {
// // 	LOG_WRN("Connecting failed");
// // }

// // static void scan_connecting(struct bt_scan_device_info *device_info,
// // 			    struct bt_conn *conn) {
// // 	default_conn = bt_conn_ref(conn);
// }

// static struct bt_le_scan_cb scan_callbacks = {
// 	.recv = scan_recv,
// };

// BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
// 		scan_connecting_error, scan_connecting);

// static int scan_init(void)
// {
// 	int err;
// 	struct bt_scan_init_param scan_init = {
// 		.connect_if_match = 1,
// 	};

// 	bt_scan_init(&scan_init);
// 	bt_le_scan_cb_register(&scan_cb);

// 	(void)bt_le_filter_accept_list_clear();

// 	err = bt_le_filter_accept_list_add(BT_UUID_DECLARE_128(BT_UUID_PACKET_SERVICE));
// 	if (err < 0) {
// 		LOG_ERR("Scanning filters cannot be set (err %d)", err);
// 		return err;
// 	}

// 	// err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_DECLARE_128(BT_UUID_PACKET_SERVICE));
// 	// if (err) {
// 	// 	LOG_ERR("Scanning filters cannot be set (err %d)", err);
// 	// 	return err;
// 	// }

// 	// err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
// 	// if (err) {
// 	// 	LOG_ERR("Filters cannot be turned on (err %d)", err);
// 	// 	return err;
// 	// }

// 	LOG_INF("Scan module initialized");
// 	return err;
// }

// void main(void)
// {
// 	int err;
	
// 	/* Initialize the LED */
// 	if (!device_is_ready(led.port)) {
//  		printk("Error setting LED\n");
//  	}

//  	err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
//  	if (err < 0) {
//  		printk("Error setting LED (err %d)\n", err);
//  	}

// 	/* Register Bluetooth Authorization */
// 	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
// 	if (err) {
// 		LOG_ERR("Failed to register authorization callbacks.");
// 	}

// 	/* Initialize the Bluetooth Subsystem */
// 	err = bt_enable(NULL);
// 	if (err) {
// 		LOG_ERR("Bluetooth init failed (err %d)\n", err);
// 	}

// 	LOG_INF("Bluetooth initialized");

// 	/* Add Bluetooth Callbacks */
// 	bt_conn_cb_register(&conn_callbacks);
	
// 	/* Start Bluetooth Scanning */
// 	err = scan_init();
// 	if (err) {
// 		LOG_ERR("Failed to initialize BLE scan");
// 	}

// 	err = bt_scan_start(BT_HCI_LE_SCAN_ACTIVE);
// 	if (err) {
// 		LOG_ERR("Scanning failed to start (err %d)", err);
// 	}

// 	LOG_INF("Scanning successfully started");
// }
