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
#include <zephyr/net/socket.h>

/* Macros used to subscribe to specific Zephyr NET management events. */
#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
#define L4_EVENT_MASK (NET_EVENT_DNS_SERVER_ADD | NET_EVENT_L4_DISCONNECTED)
#else
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#endif
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

#define TEST_SERVER_PORT     6000
#define TEST_SERVER_ENDPOINT "flake.legato.io"

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
#ifdef CONFIG_MODEM_HL78XX_12
	case HL78XX_RAT_GSM:
		return "GSM";
#ifdef CONFIG_MODEM_HL78XX_12_FW_R6
	case HL78XX_RAT_NBNTN:
		return "NTN";
#endif /* CONFIG_MODEM_HL78XX_12_FW_R6 */
#endif /* CONFIG_MODEM_HL78XX_12 */
	default:
		return "Not ready";
	}
}
/** Convert registration status to string */
static const char *reg_status_get_in_string(enum cellular_registration_status rat)
{
	switch (rat) {
	case CELLULAR_REGISTRATION_NOT_REGISTERED:
		return "Not Registered";
	case CELLULAR_REGISTRATION_REGISTERED_HOME:
		return "Home Network";
	case CELLULAR_REGISTRATION_SEARCHING:
		return "Network Searching";
	case CELLULAR_REGISTRATION_DENIED:
		return "Registration Denied";
	case CELLULAR_REGISTRATION_UNKNOWN:
		return "Out of coverage or Unknown";
	case CELLULAR_REGISTRATION_REGISTERED_ROAMING:
		return "Roaming Network";
	default:
		return "Not ready";
	}
}

/** Convert module status code to string */
/** Convert hl78xx module status enum to string */
const char *hl78xx_module_status_to_string(enum hl78xx_module_status status)
{
	switch (status) {
	case HL78XX_MODULE_READY:
		return "Module is ready to receive commands. No access code required.";
	case HL78XX_MODULE_WAITING_FOR_ACCESS_CODE:
		return "Module is waiting for an access code.";
	case HL78XX_MODULE_SIM_NOT_PRESENT:
		return "SIM card is not present.";
	case HL78XX_MODULE_SIMLOCK:
		return "Module is in SIMlock state.";
	case HL78XX_MODULE_UNRECOVERABLE_ERROR:
		return "Unrecoverable error.";
	case HL78XX_MODULE_UNKNOWN_STATE:
		return "Unknown state.";
	case HL78XX_MODULE_INACTIVE_SIM:
		return "Inactive SIM.";
	default:
		return "Invalid module status.";
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

static void connectivity_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("Fatal error received from the connectivity layer");
		return;
	}
}

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
			     struct net_if *iface)
{
	switch (event) {
#if defined(CONFIG_NET_SAMPLE_COMMON_WAIT_DNS_SERVER_ADDITION)
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

static void evnt_listener(struct hl78xx_evt *event, struct hl78xx_evt_monitor_entry *context)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d HL78XX modem Event Received: %d", __LINE__, event->type);
#endif
	switch (event->type) {
		/* Do something */
	case HL78XX_LTE_RAT_UPDATE:
		LOG_INF("%d HL78XX modem rat mode changed: %d", __LINE__, event->content.rat_mode);
		break;
	case HL78XX_LTE_REGISTRATION_STAT_UPDATE:
		LOG_INF("%d HL78XX modem registration status: %d", __LINE__,
			event->content.reg_status);
		break;
	case HL78XX_LTE_MODEM_STARTUP:
		LOG_INF("%d HL78XX modem startup status: %s", __LINE__,
			hl78xx_module_status_to_string(event->content.value));
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

/**
 * @brief resolve_broker_addr - Resolve the broker address and port.
 * @param broker Pointer to sockaddr_in structure to store the resolved address.
 */
static int resolve_broker_addr(struct sockaddr_in *broker)
{
	int ret;
	struct addrinfo *ai = NULL;

	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
	};
	char port_string[6] = {0};

	snprintf(port_string, sizeof(port_string), "%d", TEST_SERVER_PORT);
	ret = getaddrinfo(TEST_SERVER_ENDPOINT, port_string, &hints, &ai);
	if (ret == 0) {
		char addr_str[INET_ADDRSTRLEN];

		memcpy(broker, ai->ai_addr, MIN(ai->ai_addrlen, sizeof(struct sockaddr_storage)));

		inet_ntop(AF_INET, &broker->sin_addr, addr_str, sizeof(addr_str));
		LOG_INF("Resolved: %s:%u", addr_str, htons(broker->sin_port));
	} else {
		LOG_ERR("failed to resolve hostname err = %d (errno = %d)", ret, errno);
	}

	freeaddrinfo(ai);

	return ret;
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", hl78xx_on_ok);

HL78XX_EVT_MONITOR(listener_evt, evnt_listener);

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
	/* Modem information */
	char manufacturer[MDM_MANUFACTURER_LENGTH] = {0};
	char fw_ver[MDM_REVISION_LENGTH] = {0};
	char apn[MDM_APN_MAX_LENGTH] = {0};
	char operator[MDM_MODEL_LENGTH] = {0};
	char imei[MDM_IMEI_LENGTH] = {0};
	enum hl78xx_cell_rat_mode tech;
	enum cellular_registration_status status;
	int16_t rsrp;
	const char *newapn = "";
	const char *sample_cmd = "AT";

#ifndef CONFIG_MODEM_HL78XX_AUTORAT
#if defined(CONFIG_MODEM_HL78XX_RAT_M1)
	tech = HL78XX_RAT_CAT_M1;
#elif defined(CONFIG_MODEM_HL78XX_RAT_NB1)
	tech = HL78XX_RAT_NB1;
#elif defined(CONFIG_MODEM_HL78XX_RAT_GSM)
	tech = HL78XX_RAT_GSM;
#elif defined(CONFIG_MODEM_HL78XX_RAT_NBNTN)
	tech = HL78XX_RAT_NBNTN;
#else
#error "No rat has been selected."
#endif
#endif /* MODEM_HL78XX_AUTORAT */

	cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_MANUFACTURER, manufacturer,
				sizeof(manufacturer));

	cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_FW_VERSION, fw_ver, sizeof(fw_ver));

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_APN, (char *)apn, sizeof(apn));

	cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_IMEI, imei, sizeof(imei));
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	/* In auto rat mode, get the current rat from the modem status */
	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_CURRENT_RAT,
			      (enum cellular_access_technology *)&tech, sizeof(tech));
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */
	/* Get the current registration status */
	cellular_get_registration_status(modem, hl78xx_rat_to_access_tech(tech), &status);
	/* Get the current signal strength */
	cellular_get_signal(modem, CELLULAR_SIGNAL_RSRP, &rsrp);
	/* Get the current network operator name */
	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_NETWORK_OPERATOR, (char *)operator,
			      sizeof(operator));

	LOG_RAW("\n**********************************************************\n");
	LOG_RAW("********* Hello HL78XX Modem Sample Application **********\n");
	LOG_RAW("**********************************************************\n");
	LOG_INF("Manufacturer: %s", manufacturer);
	LOG_INF("Firmware Version: %s", fw_ver);
	LOG_INF("APN: \"%s\"", apn);
	LOG_INF("Imei: %s", imei);
	LOG_INF("RAT: %s", rat_get_in_string(tech));
	LOG_INF("Connection status: %s(%d)", reg_status_get_in_string(status), status);
	LOG_INF("RSRP : %d", rsrp);
	LOG_INF("Operator: %s", (strlen(operator) > 0) ? operator : "\"\"");
	LOG_RAW("**********************************************************\n\n");

	LOG_INF("Setting new APN: %s", newapn);
	ret = cellular_set_apn(modem, newapn);
	if (ret < 0) {
		LOG_ERR("Failed to set new APN, error: %d", ret);
	}

	k_sem_reset(&network_connected_sem);
	LOG_INF("Waiting for network connection...");
	k_sem_take(&network_connected_sem, K_FOREVER);

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_APN, apn, sizeof(apn));

	hl78xx_modem_cmd_send(modem, sample_cmd, strlen(sample_cmd), &ok_match, 1);
	LOG_INF("New APN: %s", (strlen(apn) > 0) ? apn : "\"\"");

	struct sockaddr_in test_endpoint_addr;

	LOG_INF("Test endpoint: %s:%d", TEST_SERVER_ENDPOINT, TEST_SERVER_PORT);

	resolve_broker_addr(&test_endpoint_addr);

	LOG_INF("Sample application finished.");

	return 0;
}
