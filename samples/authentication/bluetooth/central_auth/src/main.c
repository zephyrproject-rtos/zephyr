
/* main.c - Application main entry point
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
#include <device.h>
#include <drivers/gpio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <auth/auth_lib.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>

#include <auth/auth_lib.h>

#if defined(CONFIG_AUTH_DTLS)
#include "../../../certs/auth_certs.h"
#endif


LOG_MODULE_REGISTER(central_auth, CONFIG_AUTH_LOG_LEVEL);


/**
 * Declare UUIDs used for the Authentication service and characteristics
 * see auth_xport.h
 */

static struct bt_uuid_128 auth_service_uuid = AUTH_SERVICE_UUID;

static struct bt_uuid_128 auth_client_char = AUTH_SVC_CLIENT_CHAR_UUID;

static struct bt_uuid_128 auth_server_char = AUTH_SVC_SERVER_CHAR;



#if defined(CONFIG_AUTH_DTLS)
/* The Root and Intermediate Certs in a single CA chain.
 * plus the server cert. All in PEM format.*/
static const uint8_t auth_cert_ca_chain[] = AUTH_ROOTCA_CERT_PEM AUTH_INTERMEDIATE_CERT_PEM;
static const uint8_t auth_dev_client_cert[] = AUTH_CLIENT_CERT_PEM;
static const uint8_t auth_client_privatekey[] = AUTH_CLIENT_PRIVATE_KEY_PEM;

static struct auth_optional_param dtls_certs_param = {
	.param_id = AUTH_DTLS_PARAM,
	.param_body = {
		.dtls_certs = {
			.server_ca_chain_pem = {
				.cert = auth_cert_ca_chain,
				.cert_size = sizeof(auth_cert_ca_chain),
			},

			.device_cert_pem = {
				.cert = auth_dev_client_cert,
				.cert_size = sizeof(auth_dev_client_cert),
				.priv_key = auth_client_privatekey,
				.priv_key_size = sizeof(auth_client_privatekey)
			}
		}
	}
};
#endif

#if defined(CONFIG_AUTH_CHALLENGE_RESPONSE)
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
#endif

static struct bt_conn *default_conn;

/* UUID to discover */
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;



/**
 * Authentication connect struct
 */
static struct authenticate_conn central_auth_conn;



/* Auth service, client descriptor, server descriptor */
#define AUTH_SVC_GATT_COUNT             (4u)

#define AUTH_SVC_INDEX                  (0u)
#define AUTH_SVC_CLIENT_CHAR_INDEX      (1u)

/* for enable/disable of notification */
#define AUTH_SVC_CLIENT_CCC_INDEX       (2u)
#define AUTH_SVC_SERVER_CHAR_INDEX      (3u)

/**
 * Used to store Authentication GATT service and characteristics.
 */
typedef struct {
	const struct bt_uuid *uuid;
	const struct bt_gatt_attr *attr;
	uint16_t handle;
	uint16_t value_handle;
	uint8_t permissions; /* Bitfields from: BT_GATT_PERM_NONE, in gatt.h */
	const uint32_t gatt_disc_type;
} auth_svc_gatt_t;


static uint32_t auth_desc_index;


/* Table content should match indexes above */
static auth_svc_gatt_t auth_svc_gatt_tbl[AUTH_SVC_GATT_COUNT] = {
	{ (const struct bt_uuid *)&auth_service_uuid,  NULL, 0, 0, BT_GATT_PERM_NONE, BT_GATT_DISCOVER_PRIMARY },               /* AUTH_SVC_INDEX */
	{ (const struct bt_uuid *)&auth_client_char,   NULL, 0, 0, BT_GATT_PERM_NONE, BT_GATT_DISCOVER_CHARACTERISTIC },        /* AUTH_SVC_CLIENT_CHAR_INDEX */
	{ BT_UUID_GATT_CCC,                            NULL, 0, 0, BT_GATT_PERM_NONE, BT_GATT_DISCOVER_DESCRIPTOR },            /*AUTH_SVC_CLIENT_CCC_INDEX CCC for Client char */
	{ (const struct bt_uuid *)&auth_server_char,   NULL, 0, 0, BT_GATT_PERM_NONE, BT_GATT_DISCOVER_CHARACTERISTIC }  /* AUTH_SVC_SERVER_CHAR_INDEX */
};


/**
 * Params used to change the connection MTU length.
 */
struct bt_gatt_exchange_params mtu_parms;

void mtu_change_cb(struct bt_conn *conn, uint8_t err,
		   struct bt_gatt_exchange_params *params)
{
	if (err) {
		LOG_ERR("Failed to set MTU, err: %d", err);
	} else {
		LOG_DBG("Successfully set MTU to: %d", bt_gatt_get_mtu(conn));
	}
}


/**
 * Characteristic discovery function
 *
 *
 * @param conn    Bluetooth conection.
 * @param attr    Discovered attribute.  NOTE: This pointer will go out fo scope
 *                do not save pointer for future use.
 * @param params  Discover params.
 *
 * @return BT_GATT_ITER_STOP
 */
static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		LOG_INF("Discover complete, NULL attribute.");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}


	/* debug output */
	LOG_DBG("====auth_desc_index is: %d=====", auth_desc_index);
	LOG_DBG("[ATTRIBUTE] handle 0x%x", attr->handle);
	LOG_DBG("[ATTRIBUTE] value handle 0x%x", bt_gatt_attr_value_handle(attr));

	/* let's get string */
	char uuid_str[50];
	bt_uuid_to_str(attr->uuid, uuid_str, sizeof(uuid_str));
	LOG_DBG("Attribute UUID: %s", log_strdup(uuid_str));

	/* print attribute UUID */
	bt_uuid_to_str(discover_params.uuid, uuid_str, sizeof(uuid_str));
	LOG_DBG("Discovery UUID: %s", log_strdup(uuid_str));


	/**
	 * Verify the correct UUID was found
	 */
	if (bt_uuid_cmp(discover_params.uuid, auth_svc_gatt_tbl[auth_desc_index].uuid)) {

		/* Failed, not the UUID we're expecting */
		LOG_ERR("Error Unknown UUID.");
		return BT_GATT_ITER_STOP;
	}

	/* save off GATT info */
	auth_svc_gatt_tbl[auth_desc_index].attr = NULL; /* NOTE: attr var not used for the Central */
	auth_svc_gatt_tbl[auth_desc_index].handle = attr->handle;
	auth_svc_gatt_tbl[auth_desc_index].value_handle = bt_gatt_attr_value_handle(attr);
	auth_svc_gatt_tbl[auth_desc_index].permissions = attr->perm;

	auth_desc_index++;

	/* Are all of the characteristics discovered? */
	if (auth_desc_index >= AUTH_SVC_GATT_COUNT) {

		/* we're done */
		LOG_INF("Discover complete");

		/* save off the server attribute handle */

		/* setup the subscribe params
		 * Value handle for the Client characteristic for indication of
		 * peripheral data.
		 */
		subscribe_params.notify = auth_xp_bt_central_notify;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.value_handle =
			auth_svc_gatt_tbl[AUTH_SVC_CLIENT_CHAR_INDEX].value_handle;

		/* Handle for the CCC descriptor itself */
		subscribe_params.ccc_handle =
			auth_svc_gatt_tbl[AUTH_SVC_CLIENT_CCC_INDEX].handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			LOG_ERR("Subscribe failed (err %d)", err);
		}

		/* Get the server BT characteristic, the central sends data to this characteristic */
		uint16_t server_char_handle =
			auth_svc_gatt_tbl[AUTH_SVC_SERVER_CHAR_INDEX].value_handle;

		/* setup the BT transport params */
		struct auth_xp_bt_params xport_params =
		{ .conn = conn, .is_central = true,
		  .server_char_hdl = server_char_handle };

		err = auth_xport_init(&central_auth_conn.xport_hdl,
				      central_auth_conn.instance,
				      AUTH_XP_TYPE_BLUETOOTH, &xport_params);

		if (err) {
			LOG_ERR("Failed to initialize Bluetooth transport, err: %d", err);
			return BT_GATT_ITER_STOP;
		}

		/* Start auth process */
		err = auth_lib_start(&central_auth_conn);
		if (err) {
			LOG_ERR("Failed to start auth service, err: %d", err);
		} else {
			LOG_INF("Started auth service.");
		}

		return BT_GATT_ITER_STOP;
	}

	/* set up the next discovery params */
	discover_params.uuid = auth_svc_gatt_tbl[auth_desc_index].uuid;
	discover_params.start_handle = attr->handle + 1;
	discover_params.type = auth_svc_gatt_tbl[auth_desc_index].gatt_disc_type;


	/* Start discovery */
	err = bt_gatt_discover(conn, &discover_params);
	if (err) {
		LOG_ERR("Discover failed (err %d)", err);
	}


	return BT_GATT_ITER_STOP;
}

/**
 * Connected to the peripheral device
 *
 * @param conn       The Bluetooth connection.
 * @param conn_err   Connection error, 0 == no error
 */
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_ERR("Failed to connect to %s (%u)", log_strdup(addr), conn_err);
		return;
	}

	LOG_INF("Connected: %s", log_strdup(addr));

	if (conn == default_conn) {

		/* set the max MTU, only for GATT interface */
		mtu_parms.func = mtu_change_cb;
		bt_gatt_exchange_mtu(conn, &mtu_parms);

		/* reset gatt discovery index */
		auth_desc_index = 0;

		discover_params.uuid = auth_svc_gatt_tbl[auth_desc_index].uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = 0x0001;
		discover_params.end_handle = 0xffff;
		discover_params.type = auth_svc_gatt_tbl[auth_desc_index].gatt_disc_type;

		/**
		 * Discover characteristics for the service
		 */
		err = bt_gatt_discover(default_conn, &discover_params);
		if (err) {
			LOG_ERR("Discover failed(err %d)", err);
			return;
		}
	}
}

/**
 * Parse through the BLE adv data, looking for our service
 *
 * @param data       Bluetooth data
 * @param user_data  User data.
 *
 * @return true on success, else false
 */
static bool bt_adv_data_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int err;
	struct bt_uuid_128 auth_uuid;

	LOG_DBG("[AD]: %u data_len %u", data->type, data->data_len);

	if (data->type == BT_DATA_UUID128_ALL) {

		if (data->data_len != BT_UUID_SIZE_128) {
			LOG_WRN("AD malformed");
			return false;
		}

		/* build full UUID to check */
		auth_uuid.uuid.type = BT_UUID_TYPE_128;
		memcpy(auth_uuid.val, data->data, BT_UUID_SIZE_128);


		/**
		 * Is this the service we're looking for? If not continue
		 * else stop the scan and connect to the device.
		 */
		if (bt_uuid_cmp((const struct bt_uuid *)&auth_uuid,
				(const struct bt_uuid *)&auth_service_uuid)) {
			return true;
		}

		/* stop scanning, we've found the service */
		err = bt_le_scan_stop();
		if (err) {
			LOG_ERR("Stop LE scan failed (err %d)", err);
			return false;
		}

		/**
		 * @brief  Connect to the device
		 */
		struct bt_conn_le_create_param param = BT_CONN_LE_CREATE_PARAM_INIT(
			BT_CONN_LE_OPT_NONE,
			BT_GAP_SCAN_FAST_INTERVAL,
			BT_GAP_SCAN_FAST_INTERVAL);

		if (bt_conn_le_create(addr, &param, BT_LE_CONN_PARAM_DEFAULT,
				      &default_conn)) {
			LOG_ERR("Failed to create BLE connection to peripheral.");
		}

		return false;
	}

	return true;
}

/**
 * Found a device when scanning.
 *
 * @param addr  BT address
 * @param rssi  Signal strength
 * @param type  Device type
 * @param ad    Simple buffer.
 */
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	LOG_DBG("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i",
		log_strdup(dev), type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_HCI_ADV_IND || type == BT_HCI_ADV_DIRECT_IND) {
		bt_data_parse(ad, bt_adv_data_found, (void *)addr);
	}
}

/**
 * BT disconnect callback.
 *
 * @param conn    The Bluetooth connection.
 * @param reason  Disconnect reason.
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", log_strdup(addr), reason);

	if (default_conn != conn) {
		return;
	}

	/* de init transport */
	/* Send disconnect event to BT transport. */
	struct auth_xport_evt conn_evt = { .event = XP_EVT_DISCONNECT };
	auth_xport_event(central_auth_conn.xport_hdl, &conn_evt);

	/* Deinit lower transport */
	auth_xport_deinit(central_auth_conn.xport_hdl);
	central_auth_conn.xport_hdl = NULL;

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

/**
 * (dis)Connection callbacks
 */
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};


/**
 * Authentication status callback.
 *
 * @param auth_conn  Authentication connection.
 * @param instance   Instance ID.
 * @param status     Auth status.
 * @param context    Optional context.
 */
static void auth_status(struct authenticate_conn *auth_conn,  enum auth_instance_id instance,
			enum auth_status status, void *context)
{
	/* display status */
	printk("Authentication instance (%d) status: %s\n", instance,
	       auth_lib_getstatus_str(status));

#if defined(CONFIG_AUTH_DTLS)
	if (status == AUTH_STATUS_IN_PROCESS) {
		printk("     DTLS may take 30-60 seconds.\n");
	}
#endif
}

/**
 * Process log messages.
 */
static void process_log_msgs(void)
{
	while (log_process(false)) {
		; /* intentionally empty statement */
	}
}

/**
 * Idle process for app.
 */
static void idle_function(void)
{
	/* Just spin while the BT modules handle the connection */
	while (true) {

		process_log_msgs();

		/* Let the handshake thread run */
		k_yield();
	}
}


/**
 *  Central main entry point.
 */
void main(void)
{
	int err = 0;
	struct auth_optional_param *opt_parms = NULL;

	log_init();

	LOG_INF("Client Auth started.");


	uint32_t flags = AUTH_CONN_CLIENT;

#if defined(CONFIG_AUTH_DTLS) && defined(CONFIG_AUTH_CHALLENGE_RESPONSE)
#error Invalid authentication config, either DTLS or Challenge-Response, not both.
#endif

#if defined(CONFIG_AUTH_DTLS)
	flags |= AUTH_CONN_DTLS_AUTH_METHOD;

	/* set TLS certs */
	opt_parms = &dtls_certs_param;
	printk("Using DTLS authentication method.\n");
#endif


#if defined(CONFIG_AUTH_CHALLENGE_RESPONSE)
	flags |= AUTH_CONN_CHALLENGE_AUTH_METHOD;

	/* Use different shared key */
	opt_parms = &chal_resp_param;
	printk("Using Challenge-Response authentication method.\n");
#endif


	err = auth_lib_init(&central_auth_conn, AUTH_INST_1_ID, auth_status,
			    NULL, opt_parms, flags);

	if (err) {
		LOG_ERR("Failed to init authentication service, err: %d.", err);
		idle_function(); /* does not return */
	}

	/**
	 * @brief Enable the Bluetooth module.  Passing NULL to bt_enable
	 * will block while the BLE stack is initialized.
	 * Enable bluetooth module
	 */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Failed to enable the bluetooth module, err: %d", err);
		idle_function(); /* does not return */
	}

	/* Register connect/disconnect callbacks */
	bt_conn_cb_register(&conn_callbacks);

	printk("Starting BLE scanning\n");

	/* start scanning for peripheral */
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);

	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}

	/* does not return */
	idle_function();

	/* should not reach here */

}
