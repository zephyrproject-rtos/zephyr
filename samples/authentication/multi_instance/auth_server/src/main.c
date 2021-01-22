/*
 * Copyright (c) 2021 Golden Bits Software, Inc.
 *
 *  Sample authentication BLE peripheral
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/l2cap.h>
#include <device.h>
#include <drivers/uart.h>

#include <logging/log.h>
#include <logging/log_ctrl.h>

#include <auth/auth_lib.h>


LOG_MODULE_REGISTER(periph_auth, CONFIG_AUTH_LOG_LEVEL);


#include "../../../certs/auth_certs.h"

/**
 * Declare UUIDs used for the Authentication service and characteristics
 */

static struct bt_uuid_128 auth_service_uuid = AUTH_SERVICE_UUID;

static struct bt_uuid_128 auth_client_char = AUTH_SVC_CLIENT_CHAR_UUID;

static struct bt_uuid_128 auth_server_char = AUTH_SVC_SERVER_CHAR;


static void client_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

/* AUTH Service Declaration */
BT_GATT_SERVICE_DEFINE(auth_svc,
       BT_GATT_PRIMARY_SERVICE(&auth_service_uuid),

	/**
	 *    Central (client role) bt_gatt_write()  ---> server characteristic --> bt_gatt_read() Peripheral (server role)
	 *
	 *                Central    <---  Notification (client characteristic)  <--- Peripheral
	 *
	 */

	/**
	 * Client characteristic, used by the peripheral (server role) to write messages authentication messages
	 * to the central (client role).  The peripheral needs to alert the central a message is
	 * ready to be read.
	 */
	BT_GATT_CHARACTERISTIC((const struct bt_uuid *)&auth_client_char, BT_GATT_CHRC_INDICATE,
			      (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), NULL, NULL, NULL),
	BT_GATT_CCC(client_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	/**
	* Server characteristic, used by the central (client role) to write authentication messages to.
	* to the server (peripheral)
	*/
	BT_GATT_CHARACTERISTIC((const struct bt_uuid *)&auth_server_char, BT_GATT_CHRC_WRITE,
			      (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), NULL, auth_xp_bt_central_write, NULL),
	);



struct bt_conn *default_conn;

static bool is_connected;

/* Authentication connection info for serial and Bluetooth */
static struct authenticate_conn auth_conn_bt;
static struct authenticate_conn auth_conn_serial;


/* The Root and Intermediate Certs in a single CA chain.
 * plus the server cert. All in PEM format.
 */
static const uint8_t auth_cert_ca_chain[] = AUTH_ROOTCA_CERT_PEM AUTH_INTERMEDIATE_CERT_PEM;
static const uint8_t auth_dev_server_cert[] = AUTH_SERVER_CERT_PEM;
static const uint8_t auth_server_privatekey[] = AUTH_SERVER_PRIVATE_KEY_PEM;


#define AUTH_DTLS_INST          AUTH_INST_1_ID
#define AUTH_CHALLENGE_INST     AUTH_INST_2_ID


static struct auth_optional_param dtls_certs_param = {
	.param_id = AUTH_DTLS_PARAM,
	.param_body = {
		.dtls_certs = {
			.server_ca_chain_pem = {
				.cert = auth_cert_ca_chain,
				.cert_size = sizeof(auth_cert_ca_chain),
			},

			.device_cert_pem = {
				.cert = auth_dev_server_cert,
				.cert_size = sizeof(auth_dev_server_cert),
				.priv_key = auth_server_privatekey,
				.priv_key_size = sizeof(auth_server_privatekey)
			}
		}
	}
};


#define NEW_SHARED_KEY_LEN          (32u)

/* Use a different key than default */
static uint8_t chal_resp_sharedkey[NEW_SHARED_KEY_LEN] = {
	0x21, 0x8e, 0x37, 0x42, 0x1e, 0xe1, 0x2a, 0x22, 0x7c, 0x4b, 0x3f, 0x3f, 0x07, 0x5e, 0x8a, 0xd8,
	0x24, 0xdf, 0xca, 0xf4, 0x04, 0xd0, 0x3e, 0x22, 0x61, 0x9f, 0x24, 0xa3, 0xc7, 0xf6, 0x5d, 0x66
};


static struct auth_optional_param chal_resp_param = {
	.param_id = AUTH_CHALRESP_PARAM,
	.param_body = {
		.chal_resp = {
			.shared_key = chal_resp_sharedkey,
		},
	}
};

/**
 * Set up the advertising data
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, AUTH_SERVICE_UUID_BYTES),  /* auth service UUID */
};


static const struct device *uart_dev;

static struct uart_config uart_cfg = {
	.baudrate = 115200,
	.parity = UART_CFG_PARITY_NONE,
	.stop_bits = UART_CFG_STOP_BITS_1,
	.data_bits = UART_CFG_DATA_BITS_8,
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
};

/**
 *  Connection callback
 *
 * @param conn  The Bluetooth connection.
 * @param err   Error value, 0 == success
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
	int ret;
	struct auth_xport_evt conn_evt;

	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		default_conn = bt_conn_ref(conn);
		printk("Connected\n");

		struct auth_xp_bt_params xport_param = {
			.conn = conn, .is_central = false,
			.client_attr = &auth_svc.attrs[1] };

		ret = auth_xport_init(&auth_conn_bt.xport_hdl, auth_conn_bt.instance,
				      AUTH_XP_TYPE_BLUETOOTH, &xport_param);

		if (ret) {
			printk("Failed to initialize BT transport, err: %d", ret);
			return;
		}

		is_connected = true;

		/* send connection event to BT transport */
		conn_evt.event = XP_EVT_CONNECT;
		auth_xport_event(auth_conn_bt.xport_hdl, &conn_evt);

		/* Start Bluetooth authentication */
		ret = auth_lib_start(&auth_conn_bt);

		if (ret) {
			printk("Failed to start authentication, err: %d\n", ret);
		}
	}
}

/**
 * The disconnect callback.
 *
 * @param conn     Bluetooth connection struct
 * @param reason   Disconnect reason code.
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct auth_xport_evt conn_evt;

	printk("Disconnected (reason 0x%02x)\n", reason);

	is_connected = false;

	/* Send disconnect event to BT transport. */
	conn_evt.event = XP_EVT_DISCONNECT;
	auth_xport_event(auth_conn_bt.xport_hdl, &conn_evt);

	/* Deinit lower transport */
	auth_xport_deinit(auth_conn_bt.xport_hdl);
	auth_conn_bt.xport_hdl = NULL;

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

/**
 * Connect callbacks
 */
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

/**
 * If the pairing was canceled.
 *
 * @param conn The Bluetooth connection.
 */
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

/**
 * Called after the BT module has initialized or not (error occurred).
 *
 * @param err  Error code, 0 == success
 */
static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising after BT module has initialized OK */
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

/**
 * Authentication status callback
 *
 * @param auth_conn   The authentication connection.
 * @param instance    Instance ID.
 * @param status      Status
 * @param context     Optional context.
 */
static void auth_status(struct authenticate_conn *auth_conn,
			enum auth_instance_id instance,
			enum auth_status status, void *context)
{
	/* display status */
	if (instance == AUTH_DTLS_INST) {
		printk("Auth instance DTLS (%d) status: %s\n", instance,
		       auth_lib_getstatus_str(status));

		if (status == AUTH_STATUS_IN_PROCESS) {
			printk("     DTLS may take 30-60 seconds.\n");
		}
	}

	if (instance == AUTH_CHALLENGE_INST) {
		printk("Auth instance Challenge-Response (%d) status: %s\n", instance,
		       auth_lib_getstatus_str(status));
	}
}


/**
 * Called when client notification is (dis)enabled by the Central
 *
 * @param attr    GATT attribute.
 * @param value   BT_GATT_CCC_NOTIFY if changes are notified.
 */
static void client_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY) ? true : false;

	LOG_INF("Client notifications %s", notif_enabled ? "enabled" : "disabled");
}

/**
 *  Configure the UART params.
 *
 * @return 0 on success, else negative error value.
 */
static int config_uart(void)
{
	struct auth_xp_serial_params xp_params;

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));

	int err = uart_configure(uart_dev, &uart_cfg);

	if (err) {
		LOG_ERR("Failed to configure UART, err: %d", err);
		return err;
	}

	/* If successful,then init lower transport layer. */
	xp_params.uart_dev = uart_dev;

	err = auth_xport_init(&auth_conn_serial.xport_hdl, auth_conn_serial.instance,
			      AUTH_XP_TYPE_SERIAL, &xp_params);

	if (err) {
		LOG_ERR("Failed to initialize authentication transport, error: %d", err);
	}

	return err;
}


/**
 * Process log messages
 */
static void process_log_msgs(void)
{
	while (log_process(false)) {
		; /* intentionally empty statement */
	}
}


static void idle_function(void)
{
	while (true) {
		process_log_msgs();
		k_yield();
	}
}

void main(void)
{
	int err = 0;

	log_init();

	printk("Starting multi auth server.\n");

	/* Instance uses DTLS auth method over Bluetooth transport. */
	err = auth_lib_init(&auth_conn_bt, AUTH_DTLS_INST, auth_status, NULL,
			    &dtls_certs_param,
			    AUTH_CONN_SERVER | AUTH_CONN_DTLS_AUTH_METHOD);

	if (err) {
		printk("Failed to initialize DTLS auth method.\n");
		idle_function();
	}

	/* Instance 2 uses Challenge-Response method over serial transport */
	err = auth_lib_init(&auth_conn_serial, AUTH_CHALLENGE_INST, auth_status,
			    NULL, &chal_resp_param,
			    AUTH_CONN_SERVER | AUTH_CONN_CHALLENGE_AUTH_METHOD);

	if (err) {
		printk("Failed to initialize Challenge-Response auth method.\n");
		idle_function(); /* never returns */
	}

	/* Configure the UART for the serial transport. */
	err = config_uart();
	if (err) {
		printk("Failed to initialize UART for serial link.\n");
		idle_function(); /* never returns */
	}


	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		idle_function(); /* never returns */
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	/* Start Challenge-Response auth over serial transport. */
	err = auth_lib_start(&auth_conn_serial);

	if (err) {
		printk("Failed to start Challenge-Response authentication, err: %d\n", err);
	}

	/* never returns */
	idle_function();

	/* should never reach here */
}
