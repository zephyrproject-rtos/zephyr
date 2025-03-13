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
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/socket.h>
#include "lwm2m_rd_client.h"

#define APP_BANNER "Run LWM2M client"

#define WAIT_TIME	K_SECONDS(10)
#define CONNECT_TIME	K_SECONDS(10)

#define NAME	"Zephyr"
#define MODEL	"client-1"
#define SERIAL	"serial-1"
#define VERSION	"1.2.3"

static struct lwm2m_ctx client;
static void rd_client_event(struct lwm2m_ctx *client,
			    enum lwm2m_rd_client_event client_event);
static void observe_cb(enum lwm2m_observe_event event,
		       struct lwm2m_obj_path *path, void *user_data);

static uint8_t bat_idx = LWM2M_DEVICE_PWR_SRC_TYPE_BAT_INT;
static int bat_mv = 3800;
static int bat_ma = 125;
static uint8_t usb_idx = LWM2M_DEVICE_PWR_SRC_TYPE_USB;
static int usb_mv = 5000;
static int usb_ma = 900;

static void reboot_handler(struct k_work *work)
{
	/* I cannot really restart the client, as we don't know
	 * the endpoint name. Testcase sets that on a command line.
	 * So we only stop.
	 */
	lwm2m_rd_client_stop(&client, rd_client_event, true);
}

K_WORK_DEFINE(reboot_work, reboot_handler);

static int device_reboot_cb(uint16_t obj_inst_id,
			    uint8_t *args, uint16_t args_len)
{
	LOG_INF("DEVICE: REBOOT");
	k_work_submit(&reboot_work);
	return 0;
}

int set_socketoptions(struct lwm2m_ctx *ctx)
{
	if (IS_ENABLED(CONFIG_MBEDTLS_SSL_DTLS_CONNECTION_ID) && ctx->use_dtls) {
		int ret;

		/* Enable CID */
		int cid = TLS_DTLS_CID_ENABLED;

		ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_DTLS_CID, &cid,
				       sizeof(cid));
		if (ret) {
			ret = -errno;
			LOG_ERR("Failed to enable TLS_DTLS_CID: %d", ret);
		}

		/* Allow DTLS handshake to timeout much faster.
		 * these tests run on TUN/TAP network, so there should be no network latency.
		 */
		uint32_t min = 100;
		uint32_t max = 500;

		zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_DTLS_HANDSHAKE_TIMEOUT_MIN, &min,
				 sizeof(min));
		zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_DTLS_HANDSHAKE_TIMEOUT_MAX, &max,
				 sizeof(max));
	}
	return lwm2m_set_default_sockopt(ctx);
}

static int create_appdata(uint16_t obj_inst_id)
{
	/* Create BinaryAppData object */
	static uint8_t data[4096];
	static char description[16];

	lwm2m_set_res_buf(&LWM2M_OBJ(19, 0, 0, 0), data, sizeof(data), 0, 0);
	lwm2m_set_res_buf(&LWM2M_OBJ(19, 0, 3), description, sizeof(description), 0, 0);

	return 0;
}

static int lwm2m_setup(void)
{
	/* setup DEVICE object */

	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 0), NAME, sizeof(NAME),
			  sizeof(NAME), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 1), MODEL, sizeof(MODEL),
			  sizeof(MODEL), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 2), SERIAL, sizeof(SERIAL),
			  sizeof(SERIAL), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 3), VERSION, sizeof(VERSION),
			  sizeof(VERSION), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_register_exec_callback(&LWM2M_OBJ(3, 0, 4), device_reboot_cb);
	lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 17), CONFIG_BOARD, sizeof(CONFIG_BOARD),
			  sizeof(CONFIG_BOARD), LWM2M_RES_DATA_FLAG_RO);

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

	lwm2m_register_create_callback(19, create_appdata);

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
		LOG_DBG("Deregistration client");
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

int main(void)
{
	int ret;

	ret = lwm2m_setup();
	if (ret < 0) {
		LOG_ERR("Cannot setup LWM2M fields (%d)", ret);
		return 0;
	}

	client.tls_tag = 1;
	client.set_socketoptions = set_socketoptions;
	client.event_cb = rd_client_event;
	client.observe_cb = observe_cb;
	client.sock_fd = -1;

	lwm2m_rd_client_set_ctx(&client);

	return 0;
}
