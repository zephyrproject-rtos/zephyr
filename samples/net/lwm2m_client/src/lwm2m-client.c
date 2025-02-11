/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_client_app
#define LOG_LEVEL LOG_LEVEL_DBG

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include "modules.h"
#include "lwm2m_resource_ids.h"

#define APP_BANNER "Run LWM2M client"

#define WAIT_TIME	K_SECONDS(10)
#define CONNECT_TIME	K_SECONDS(10)

#define CLIENT_MANUFACTURER	"Zephyr"
#define CLIENT_MODEL_NUMBER	"OMA-LWM2M Sample Client"
#define CLIENT_SERIAL_NUMBER	"345000123"
#define CLIENT_FIRMWARE_VER	"1.0"
#define CLIENT_HW_VER		"1.0.1"
#define TEMP_SENSOR_UNITS	"Celcius"

/* Macros used to subscribe to specific Zephyr NET management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

static uint8_t bat_idx = LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT;
static int bat_mv = 3800;
static int bat_ma = 125;
static uint8_t usb_idx = LWM2M_DEVICE_PWR_SRC_TYPE_USB;
static int usb_mv = 5000;
static int usb_ma = 900;
static uint8_t bat_level = 95;
static uint8_t bat_status = LWM2M_DEVICE_BATTERY_STATUS_CHARGING;
static int mem_free = 15;
static int mem_total = 25;
static double min_range = 0.0;
static double max_range = 100;

static struct lwm2m_ctx client_ctx;

static const char *endpoint =
	(sizeof(CONFIG_LWM2M_APP_ID) > 1 ? CONFIG_LWM2M_APP_ID : CONFIG_BOARD);

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
BUILD_ASSERT(sizeof(endpoint) <= CONFIG_LWM2M_SECURITY_KEY_SIZE,
		"Client ID length is too long");
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

static struct k_sem quit_lock;

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

static K_SEM_DEFINE(network_connected_sem, 0, 1);

static int device_reboot_cb(uint16_t obj_inst_id,
			    uint8_t *args, uint16_t args_len)
{
	LOG_INF("DEVICE: REBOOT");
	/* Add an error for testing */
	lwm2m_device_add_err(LWM2M_DEVICE_ERROR_LOW_POWER);
	/* Change the battery voltage for testing */
	lwm2m_set_s32(&LWM2M_OBJ(3, 0, 7, 0), (bat_mv - 1));

	return 0;
}

static int device_factory_default_cb(uint16_t obj_inst_id,
				     uint8_t *args, uint16_t args_len)
{
	LOG_INF("DEVICE: FACTORY DEFAULT");
	/* Add an error for testing */
	lwm2m_device_add_err(LWM2M_DEVICE_ERROR_GPS_FAILURE);
	/* Change the USB current for testing */
	lwm2m_set_s32(&LWM2M_OBJ(3, 0, 8, 1), (usb_ma - 1));

	return 0;
}

static int lwm2m_setup(void)
{
	struct lwm2m_res_item temp_sensor_items[] = {
		{&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MIN_RANGE_VALUE_RID), &min_range,
		 sizeof(min_range)},
		{&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MAX_RANGE_VALUE_RID), &max_range,
		 sizeof(max_range)},
		{&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_UNITS_RID), TEMP_SENSOR_UNITS,
		 sizeof(TEMP_SENSOR_UNITS)}
	};

	/* setup SECURITY object */

	/* Server URL */
	lwm2m_set_string(&LWM2M_OBJ(0, 0, 0), CONFIG_LWM2M_APP_SERVER);

	/* Security Mode */
	lwm2m_set_u8(&LWM2M_OBJ(0, 0, 2), IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) ? 0 : 3);
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	lwm2m_set_string(&LWM2M_OBJ(0, 0, 3), endpoint);
	if (sizeof(CONFIG_LWM2M_APP_PSK) > 1) {
		char psk[1 + sizeof(CONFIG_LWM2M_APP_PSK) / 2];
		/* Need to skip the nul terminator from string */
		size_t len = hex2bin(CONFIG_LWM2M_APP_PSK, sizeof(CONFIG_LWM2M_APP_PSK) - 1, psk,
				     sizeof(psk));
		if (len <= 0) {
			return -EINVAL;
		}
		lwm2m_set_opaque(&LWM2M_OBJ(0, 0, 5), (void *)psk, len);
	}
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	/* Mark 1st instance of security object as a bootstrap server */
	lwm2m_set_u8(&LWM2M_OBJ(0, 0, 1), 1);

	/* Create 2nd instance of security object needed for bootstrap */
	lwm2m_create_object_inst(&LWM2M_OBJ(0, 1));
#else
	/* Match Security object instance with a Server object instance with
	 * Short Server ID.
	 */
	lwm2m_set_u16(&LWM2M_OBJ(0, 0, 10), CONFIG_LWM2M_SERVER_DEFAULT_SSID);
	lwm2m_set_u16(&LWM2M_OBJ(1, 0, 0), CONFIG_LWM2M_SERVER_DEFAULT_SSID);
#endif

	/* setup SERVER object */

	/* setup DEVICE object */

	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 0), CLIENT_MANUFACTURER, sizeof(CLIENT_MANUFACTURER),
			  sizeof(CLIENT_MANUFACTURER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 1), CLIENT_MODEL_NUMBER, sizeof(CLIENT_MODEL_NUMBER),
			  sizeof(CLIENT_MODEL_NUMBER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 2), CLIENT_SERIAL_NUMBER, sizeof(CLIENT_SERIAL_NUMBER),
			  sizeof(CLIENT_SERIAL_NUMBER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 3), CLIENT_FIRMWARE_VER, sizeof(CLIENT_FIRMWARE_VER),
			  sizeof(CLIENT_FIRMWARE_VER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_register_exec_callback(&LWM2M_OBJ(3, 0, 4), device_reboot_cb);
	lwm2m_register_exec_callback(&LWM2M_OBJ(3, 0, 5), device_factory_default_cb);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 9), &bat_level, sizeof(bat_level), sizeof(bat_level), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 10), &mem_free, sizeof(mem_free), sizeof(mem_free), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 17), CONFIG_BOARD, sizeof(CONFIG_BOARD),
			  sizeof(CONFIG_BOARD), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 18), CLIENT_HW_VER, sizeof(CLIENT_HW_VER),
			  sizeof(CLIENT_HW_VER), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 20), &bat_status, sizeof(bat_status),
			  sizeof(bat_status), 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 21), &mem_total, sizeof(mem_total),
			  sizeof(mem_total), 0);

	/* add power source resource instances */
	lwm2m_create_res_inst(&LWM2M_OBJ(3, 0, 6, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 6, 0), &bat_idx, sizeof(bat_idx), sizeof(bat_idx), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(3, 0, 7, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 7, 0), &bat_mv, sizeof(bat_mv), sizeof(bat_mv), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(3, 0, 8, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 8, 0), &bat_ma, sizeof(bat_ma), sizeof(bat_ma), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(3, 0, 6, 1));
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 6, 1), &usb_idx, sizeof(usb_idx), sizeof(usb_idx), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(3, 0, 7, 1));
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 7, 1), &usb_mv, sizeof(usb_mv), sizeof(usb_mv), 0);
	lwm2m_create_res_inst(&LWM2M_OBJ(3, 0, 8, 1));
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 8, 1), &usb_ma, sizeof(usb_ma), sizeof(usb_ma), 0);

	/* setup FIRMWARE object */
	if (IS_ENABLED(CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT)) {
		init_firmware_update();
	}

	/* setup TEMP SENSOR object */
	init_temp_sensor();

	/* Set multiple TEMP SENSOR resource values in one function call. */
	int err = lwm2m_set_bulk(temp_sensor_items, ARRAY_SIZE(temp_sensor_items));

	if (err) {
		LOG_ERR("Failed to set TEMP SENSOR resources");
		return err;
	}

	/* IPSO: Light Control object */
	init_led_device();

	/* IPSO: Timer object */
	init_timer_object();

	return 0;
}

static void rd_client_event(struct lwm2m_ctx *client,
			    enum lwm2m_rd_client_event client_event)
{
	switch (client_event) {

	case LWM2M_RD_CLIENT_EVENT_NONE:
		/* do nothing */
		break;

	case LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED:
		LOG_DBG("LwM2M server disabled");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE:
		LOG_DBG("Bootstrap registration failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE:
		LOG_DBG("Bootstrap registration complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE:
		LOG_DBG("Bootstrap transfer complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE:
		LOG_DBG("Registration failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE:
		LOG_DBG("Registration complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT:
		LOG_DBG("Registration timeout!");
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE:
		LOG_DBG("Registration update complete");
		break;

	case LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE:
		LOG_DBG("Deregister failure!");
		break;

	case LWM2M_RD_CLIENT_EVENT_DISCONNECT:
		LOG_DBG("Disconnected");
		break;

	case LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF:
		LOG_DBG("Queue mode RX window closed");
		break;

	case LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED:
		LOG_DBG("LwM2M engine suspended");
		break;

	case LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR:
		LOG_ERR("LwM2M engine reported a network error.");
		lwm2m_rd_client_stop(client, rd_client_event, true);
		break;

	case LWM2M_RD_CLIENT_EVENT_REG_UPDATE:
		LOG_DBG("Registration update");
		break;
	case LWM2M_RD_CLIENT_EVENT_DEREGISTER:
		LOG_DBG("Client De-register");
		break;
	}
}

static void socket_state(int fd, enum lwm2m_socket_states state)
{
	(void) fd;
	switch (state) {
	case LWM2M_SOCKET_STATE_ONGOING:
		LOG_DBG("LWM2M_SOCKET_STATE_ONGOING");
		break;
	case LWM2M_SOCKET_STATE_ONE_RESPONSE:
		LOG_DBG("LWM2M_SOCKET_STATE_ONE_RESPONSE");
		break;
	case LWM2M_SOCKET_STATE_LAST:
		LOG_DBG("LWM2M_SOCKET_STATE_LAST");
		break;
	case LWM2M_SOCKET_STATE_NO_DATA:
		LOG_DBG("LWM2M_SOCKET_STATE_NO_DATA");
		break;
	}
}

static void observe_cb(enum lwm2m_observe_event event,
		       struct lwm2m_obj_path *path, void *user_data)
{
	char buf[LWM2M_MAX_PATH_STR_SIZE];

	switch (event) {

	case LWM2M_OBSERVE_EVENT_OBSERVER_ADDED:
		LOG_INF("Observer added for %s", lwm2m_path_log_buf(buf, path));
		break;

	case LWM2M_OBSERVE_EVENT_OBSERVER_REMOVED:
		LOG_INF("Observer removed for %s", lwm2m_path_log_buf(buf, path));
		break;

	case LWM2M_OBSERVE_EVENT_NOTIFY_ACK:
		LOG_INF("Notify acknowledged for %s", lwm2m_path_log_buf(buf, path));
		break;

	case LWM2M_OBSERVE_EVENT_NOTIFY_TIMEOUT:
		LOG_INF("Notify timeout for %s, trying registration update",
			lwm2m_path_log_buf(buf, path));

		lwm2m_rd_client_update();
		break;
	}
}

static void on_net_event_l4_disconnected(void)
{
	LOG_INF("Disconnected from network");
	lwm2m_engine_pause();
}

static void on_net_event_l4_connected(void)
{
	LOG_INF("Connected to network");
	k_sem_give(&network_connected_sem);
	lwm2m_engine_resume();
}

static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("IP Up");
		on_net_event_l4_connected();
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("IP down");
		on_net_event_l4_disconnected();
		break;
	default:
		break;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("Fatal error received from the connectivity layer");
		return;
	}
}

int main(void)
{
	uint32_t flags = IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) ?
				LWM2M_RD_CLIENT_FLAG_BOOTSTRAP : 0;
	int ret;

	LOG_INF(APP_BANNER);

	k_sem_init(&quit_lock, 0, K_SEM_MAX_LIMIT);

	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		/* Setup handler for Zephyr NET Connection Manager events. */
		net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
		net_mgmt_add_event_callback(&l4_cb);

		/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
		net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler,
					     CONN_LAYER_EVENT_MASK);
		net_mgmt_add_event_callback(&conn_cb);

		ret = net_if_up(net_if_get_default());

		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("net_if_up, error: %d", ret);
			return ret;
		}

		ret = conn_mgr_if_connect(net_if_get_default());
		/* Ignore errors from interfaces not requiring connectivity */
		if (ret == 0) {
			LOG_INF("Connecting to network");
			k_sem_take(&network_connected_sem, K_FOREVER);
		}
	}

	ret = lwm2m_setup();
	if (ret < 0) {
		LOG_ERR("Cannot setup LWM2M fields (%d)", ret);
		return 0;
	}

	(void)memset(&client_ctx, 0x0, sizeof(client_ctx));
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	client_ctx.tls_tag = CONFIG_LWM2M_APP_TLS_TAG;
#endif
	client_ctx.set_socket_state = socket_state;

	/* client_ctx.sec_obj_inst is 0 as a starting point */
	lwm2m_rd_client_start(&client_ctx, endpoint, flags, rd_client_event, observe_cb);

	k_sem_take(&quit_lock, K_FOREVER);
	return 0;
}
