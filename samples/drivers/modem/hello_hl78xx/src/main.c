/*
 * Copyright (c) 2025 Netfeasa
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/modem/hl78xx_apis.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/modem/chat.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>

/* Macros used to subscribe to specific Zephyr NET management events. */
#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
#define L4_EVENT_MASK (NET_EVENT_DNS_SERVER_ADD | NET_EVENT_L4_DISCONNECTED)
#else
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#endif
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

LOG_MODULE_REGISTER(main, CONFIG_MODEM_LOG_LEVEL);

static K_SEM_DEFINE(network_connected_sem, 0, 1);
const struct device *modem = DEVICE_DT_GET(DT_ALIAS(modem));

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

static const char *rat_get_in_string(enum hl78xx_cell_rat_mode rat)
{
	switch (rat) {
	case HL78XX_RAT_CAT_M1:
		return "CAT-M1";
	case HL78XX_RAT_NB1:
		return "NB1";
	case HL78XX_RAT_GSM:
		return "GSM";
	case HL78XX_RAT_NBNTN:
		return "NTN";
	default:
		return "Not ready";
	}
}

static const char *reg_status_get_in_string(enum hl78xx_registration_status rat)
{
	switch (rat) {
	case HL78XX_REGISTRATION_NOT_REGISTERED:
		return "Not Registered";
	case HL78XX_REGISTRATION_REGISTERED_HOME:
		return "Home Network";
	case HL78XX_REGISTRATION_SEARCHING:
		return "Network Searching";
	case HL78XX_REGISTRATION_DENIED:
		return "Registiration Denied";
	case HL78XX_REGISTRATION_UNKNOWN:
		return "Out of covarege or Unknown";
	case HL78XX_REGISTRATION_REGISTERED_ROAMING:
		return "Roaming Network";
	default:
		return "Not ready";
	}
}
/* Zephyr NET management event callback structures. */
static void on_net_event_l4_disconnected(void)
{
	LOG_INF("Disconnected from network");
}

static void on_net_event_l4_connected(void)
{
	LOG_INF("Connected to network");
	k_sem_give(&network_connected_sem);
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("Fatal error received from the connectivity layer");
		return;
	}
}

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
#if defined(CONFIG_NET_SAMPLE_LWM2M_WAIT_DNS)
	case NET_EVENT_DNS_SERVER_ADD:
#else
	case NET_EVENT_L4_CONNECTED:
#endif
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

static void evnt_listener(struct hl78xx_evt *event)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d HL78XX modem Event Received: %d", __LINE__, event->type);
#endif
	switch (event->type) {
		/* Do something */
	case HL78XX_RAT_UPDATE:
		LOG_DBG("%d HL78XX modem rat mode changed: %d", __LINE__, event->content.rat_mode);
		break;
	case HL78XX_LTE_REGISTRATION_STAT_UPDATE:
		LOG_DBG("%d HL78XX modem registration status: %d", __LINE__,
			event->content.reg_status);
		break;
	case HL78XX_LTE_SIM_REGISTRATIION:
		break;
	case HL78XX_LTE_PSMEV:
		break;
	default:
		break;
	}
}

static void hl78xx_on_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %s", __LINE__, __func__, argv[0]);
#endif
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", hl78xx_on_ok);

HL78XX_EVT_MONITOR(listner_evt, evnt_listener);

int main(void)
{
	int ret = 0;

	if (device_is_ready(modem) == false) {
		LOG_ERR("%d, %s Device %s is not ready", __LINE__, __func__, modem->name);
	}
#ifdef CONFIG_PM_DEVICE
	LOG_INF("Powering on modem\n");
	pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);
#endif

#ifdef CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE
	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		struct net_if *iface = net_if_get_default();

		if (!iface) {
			LOG_ERR("No network interface found!");
			return -ENODEV;
		}

		/* Setup handler for Zephyr NET Connection Manager events. */
		net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
		net_mgmt_add_event_callback(&l4_cb);

		/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
		net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler,
					     CONN_LAYER_EVENT_MASK);
		net_mgmt_add_event_callback(&conn_cb);

		ret = net_if_up(iface);

		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("net_if_up, error: %d", ret);
			return ret;
		}

		(void)conn_mgr_if_connect(iface);

		LOG_INF("Waiting for network connection...");
		k_sem_take(&network_connected_sem, K_FOREVER);
	}
#endif
	/* Start pleacing your modem based code here */
	char manufacturer[20] = {0};

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_MANUFACTURER, manufacturer,
			      sizeof(manufacturer));
	LOG_DBG("Manufecturer: %s", manufacturer);

	char fw_ver[17] = {0};

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_FW_VERSION, fw_ver, sizeof(fw_ver));
	LOG_DBG("Firmware Version: %s", fw_ver);

	char apn[64] = {0};

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_APN, apn, sizeof(apn));
	LOG_DBG("APN: %s", apn);

	char imei[17] = {0};

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_IMEI, imei, sizeof(imei));
	LOG_DBG("Imei: %s", imei);

	enum hl78xx_cell_rat_mode tech;
	enum hl78xx_registration_status status;

	hl78xx_get_registration_status(modem, &tech, &status);
	LOG_DBG("RAT: %s", rat_get_in_string(tech));
	LOG_DBG("Connection status: %s", reg_status_get_in_string(status));

	int16_t rsrp;

	hl78xx_get_signal(modem, HL78XX_SIGNAL_RSRP, &rsrp);
	LOG_DBG("RSRP : %d", rsrp);

	const char *newapn = "";

	hl78xx_set_apn(modem, newapn, 0);

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_APN, apn, sizeof(apn));
	LOG_DBG("New APN: %s", (strlen(apn) > 0) ? apn : "\"\"");

	const char *sample_cmd = "AT";

	hl78xx_modem_cmd_send(modem, sample_cmd, strlen(sample_cmd), &ok_match, 1);
	return 0;
}
