/*
 * Copyright (c) 2025 Netfeasa
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdlib.h>
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
#ifdef CONFIG_HL78XX_GNSS
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#endif

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
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
static K_SEM_DEFINE(fota_complete_rerun, 0, 1);
#endif
static const struct device *modem = DEVICE_DT_GET(DT_ALIAS(modem));

/* GNSS support */
#ifdef CONFIG_HL78XX_GNSS
#define GNSS_DEVICE DEVICE_DT_GET(DT_ALIAS(gnss))
static K_SEM_DEFINE(gnss_sem, 0, 1);
static K_SEM_DEFINE(gnss_exit_sem, 0, 1);
#if defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE)
static void k_work_wake_fn(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(gnss_wake_work, k_work_wake_fn);
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
#endif

/* Zephyr NET management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
static int fota_update_status = -1;
#endif
#ifdef CONFIG_MODEM_HL78XX_12
#if defined(CONFIG_MODEM_HL78XX_PSM)
static enum hl78xx_psmev_event mdm_psmev_current = HL78XX_PSM_EVENT_NONE;
#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_EDRX
static enum hl78xx_edrx_event mdm_edrxev_current = HL78XX_EDRX_EVENT_IDLE_NONE;
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#endif /* CONFIG_MODEM_HL78XX_12 */
static enum cellular_registration_status registration_status;

/** Convert RAT mode enum to string */
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
static const char *reg_status_get_in_string(enum cellular_registration_status status)
{
	switch (status) {
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

/** Convert hl78xx module status enum to string */
static const char *hl78xx_module_status_to_string(enum hl78xx_module_status status)
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

/* Network event handlers */
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

/* -------------------------------------------------------------------------
 * GNSS Callbacks
 * -------------------------------------------------------------------------
 */
#ifdef CONFIG_HL78XX_GNSS

/* GNSS data callback - called when a fix is received */
static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	if (!data) {
		LOG_ERR("Received NULL GNSS data");
		return;
	}

	LOG_INF("========== GNSS FIX RECEIVED ==========");

	/* UTC time - millisecond field contains seconds*1000 + milliseconds */
	uint16_t seconds = data->utc.millisecond / 1000;
	uint16_t millis = data->utc.millisecond % 1000;

	LOG_INF("  UTC Time: %02u:%02u:%02u.%03u", data->utc.hour, data->utc.minute, seconds,
		millis);

	/* Fix status and position */
	if (data->info.fix_status == GNSS_FIX_STATUS_GNSS_FIX ||
	    data->info.fix_status == GNSS_FIX_STATUS_ESTIMATED_FIX) {
		const char *fix_type = (data->info.fix_status == GNSS_FIX_STATUS_GNSS_FIX)
					       ? "VALID (3D Fix)"
					       : "VALID (2D Fix)";
		LOG_INF("  Fix Status: %s", fix_type);
		/* Latitude/Longitude are in nanodegrees - divide by 1e9 for degrees */
		LOG_INF("  Latitude:  %lld.%06lld deg", data->nav_data.latitude / 1000000000LL,
			llabs((data->nav_data.latitude % 1000000000LL) / 1000));
		LOG_INF("  Longitude: %lld.%06lld deg", data->nav_data.longitude / 1000000000LL,
			llabs((data->nav_data.longitude % 1000000000LL) / 1000));
		LOG_INF("  Altitude:  %d.%03d m", (int)(data->nav_data.altitude / 1000),
			(int)abs(data->nav_data.altitude % 1000));
		LOG_INF("  Speed:     %u.%03u m/s", data->nav_data.speed / 1000,
			data->nav_data.speed % 1000);
		LOG_INF("  Bearing:   %u.%03u deg", data->nav_data.bearing / 1000,
			data->nav_data.bearing % 1000);
		k_sem_give(&gnss_sem);
	} else if (data->info.fix_status == GNSS_FIX_STATUS_NO_FIX) {
		LOG_WRN("  Fix Status: NO FIX");
	} else {
		LOG_WRN("  Fix Status: %d", data->info.fix_status);
	}

	/* Fix quality and precision */
	LOG_INF("  Fix Quality: %d", data->info.fix_quality);
	LOG_INF("  Satellites Used: %u", data->info.satellites_cnt);
	LOG_INF("  HDOP: %u.%03u", data->info.hdop / 1000, data->info.hdop % 1000);

	LOG_INF("=======================================");
}

#ifdef CONFIG_GNSS_SATELLITES
/* GNSS satellites callback */
static void gnss_satellites_cb(const struct device *dev, const struct gnss_satellite *satellites,
			       uint16_t size)
{
	LOG_INF("========== SATELLITES UPDATE ==========");
	LOG_INF("  Total satellites in view: %u", size);

	for (uint16_t i = 0; i < size && i < 15; i++) {
		LOG_INF("    [%2u] PRN:%3u  Elev:%2d deg  Azim:%3u deg  SNR:%2u dB  Track:%c", i,
			satellites[i].prn, satellites[i].elevation, satellites[i].azimuth,
			satellites[i].snr, satellites[i].is_tracked ? 'Y' : 'N');
	}

	if (size > 15) {
		LOG_INF("    ... and %u more satellites", size - 15);
	}

	LOG_INF("=======================================");
}
#endif /* CONFIG_GNSS_SATELLITES */

#ifdef CONFIG_HL78XX_GNSS_AUX_DATA_PARSER
/* GNSS auxiliary data callback */
static void gnss_aux_data_cb(const struct device *dev,
			     const struct hl78xx_gnss_nmea_aux_data *aux_data, uint16_t size)
{
	LOG_INF("========== GNSS AUXILIARY DATA ==========");
	LOG_INF("  GSA: fix_type=%d, PDOP=%" PRId64 ".%03" PRId64 ", HDOP=%" PRId64 ".%03" PRId64
		", VDOP=%" PRId64 ".%03" PRId64,
		aux_data->gsa.fix_type, aux_data->gsa.pdop / 1000, aux_data->gsa.pdop % 1000,
		aux_data->gsa.hdop / 1000, aux_data->gsa.hdop % 1000, aux_data->gsa.vdop / 1000,
		aux_data->gsa.vdop % 1000);
	LOG_INF("  GST: RMS=%" PRId64 ".%03" PRId64 "m, Lat_err=%" PRId64 ".%03" PRId64
		"m, Lon_err=%" PRId64 ".%03" PRId64 "m",
		aux_data->gst.rms / 1000, aux_data->gst.rms % 1000, aux_data->gst.lat_err / 1000,
		aux_data->gst.lat_err % 1000, aux_data->gst.lon_err / 1000,
		aux_data->gst.lon_err % 1000);
	LOG_INF("=========================================");
}
#endif /* CONFIG_HL78XX_GNSS_AUX_DATA_PARSER */

/* Register GNSS callbacks */
GNSS_DATA_CALLBACK_DEFINE(GNSS_DEVICE, gnss_data_cb);

#ifdef CONFIG_GNSS_SATELLITES
GNSS_SATELLITES_CALLBACK_DEFINE(GNSS_DEVICE, gnss_satellites_cb);
#endif

#ifdef CONFIG_HL78XX_GNSS_AUX_DATA_PARSER
GNSS_AUX_DATA_CALLBACK_DEFINE(GNSS_DEVICE, gnss_aux_data_cb);
#endif

#endif /* CONFIG_HL78XX_GNSS */

static void evnt_listener(struct hl78xx_evt *event, struct hl78xx_evt_monitor_entry *context)
{
	switch (event->type) {
	case HL78XX_LTE_RAT_UPDATE:
		LOG_INF("%d HL78XX modem rat mode changed: %d", __LINE__, event->content.rat_mode);
		break;
	case HL78XX_LTE_REGISTRATION_STAT_UPDATE:
		LOG_INF("%d HL78XX modem registration status: %d", __LINE__,
			event->content.reg_status);
		registration_status = event->content.reg_status;
#if defined(CONFIG_MODEM_HL78XX_12) && defined(CONFIG_HL78XX_GNSS) &&                              \
	defined(CONFIG_MODEM_HL78XX_PSM)
		if ((event->content.reg_status == CELLULAR_REGISTRATION_REGISTERED_HOME ||
		     event->content.reg_status == CELLULAR_REGISTRATION_REGISTERED_ROAMING) &&
		    mdm_psmev_current == HL78XX_PSM_EVENT_EXIT) {
			mdm_psmev_current = HL78XX_PSM_EVENT_NONE;
			k_sem_give(&network_connected_sem);
		}
#endif /* CONFIG_MODEM_HL78XX_12 && CONFIG_HL78XX_GNSS && CONFIG_MODEM_HL78XX_PSM */
		break;
	case HL78XX_LTE_MODEM_STARTUP:
		LOG_INF("%d HL78XX modem startup status: %s", __LINE__,
			hl78xx_module_status_to_string(event->content.value));
		break;
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
	case HL78XX_LTE_FOTA_UPDATE_STATUS:
		LOG_INF("%d HL78XX modem FOTA update status: %d", __LINE__,
			event->content.wdsi_indication);
		if (event->content.wdsi_indication == WDSI_FIRMWARE_UPDATE_SUCCESS) {
			LOG_INF("FOTA update complete, restarting modem...");
			k_sem_reset(&network_connected_sem);
			fota_update_status = (int)WDSI_FIRMWARE_UPDATE_SUCCESS;
			k_sem_give(&fota_complete_rerun);
		} else if (event->content.wdsi_indication == WDSI_FIRMWARE_UPDATE_FAILED) {
			LOG_INF("FOTA update failed.");
			fota_update_status = (int)WDSI_FIRMWARE_UPDATE_FAILED;
			k_sem_give(&fota_complete_rerun);
		} else if (event->content.wdsi_indication == WDSI_FIRMWARE_DOWNLOAD_REQUEST &&
			   fota_update_status != (int)WDSI_FIRMWARE_DOWNLOAD_REQUEST) {
			LOG_INF("FOTA download requested, starting download...");
			if (fota_update_status != (int)WDSI_SESSION_STARTED) {
				return;
			}
			fota_update_status = (int)WDSI_FIRMWARE_DOWNLOAD_REQUEST;
			k_sem_give(&fota_complete_rerun);
		} else if (event->content.wdsi_indication == WDSI_SESSION_STARTED) {
			LOG_INF("FOTA session started...");
			fota_update_status = (int)WDSI_SESSION_STARTED;
		} else {
			/* Other WDSI indications can be handled here if needed */
		}

		break;
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */

		/* GNSS events */
#ifdef CONFIG_HL78XX_GNSS
	case HL78XX_GNSS_EVENT_INIT:
		LOG_DBG("GNSS engine initialized");
		break;

	case HL78XX_GNSS_ENGINE_READY:
		LOG_INF("HL78XX GNSS engine ready");
		if (event->content.status) {
			k_sem_give(&gnss_sem);
		} else {
			LOG_WRN("GNSS engine initialization failed");
		}
		break;

	case HL78XX_GNSS_EVENT_START:
		LOG_INF("GNSS search started successfully");
		break;

	case HL78XX_GNSS_EVENT_STOP:
		LOG_INF("GNSS search stopped");
		break;

	case HL78XX_GNSS_EVENT_START_BLOCKED:
		/*
		 * GNSS failed to start because LTE modem is active.
		 * User should either:
		 * 1. Put modem in airplane mode (AT+CFUN=4)
		 * 2. Wait for PSM/idle-eDRX
		 * The GNSS search is still queued and will auto-start when
		 * airplane mode is entered.
		 */
		LOG_WRN("GNSS blocked by LTE! Enter airplane mode to start GNSS");
		LOG_WRN("Hint: Use hl78xx_set_phone_functionality(dev, HL78XX_AIRPLANE)");
		k_sem_give(&gnss_sem);
		break;

	case HL78XX_GNSS_EVENT_SEARCH_TIMEOUT:
		LOG_WRN("GNSS search timeout expired");
		k_sem_give(&gnss_sem);
		break;

	case HL78XX_GNSS_EVENT_POSITION:
		LOG_INF("GNSS position event: %d", event->content.position_event);
		switch (event->content.position_event) {
		case GNSS_FIX_LOST_OR_UNAVAILABLE:
			LOG_INF("  GNSS Fix Lost or Unavailable");
			break;
		case GNSS_ESTIMATED_POSITION_AVAILABLE:
			LOG_INF("  Estimated GNSS Position Available");
			break;
		case GNSS_2D_POSITION_AVAILABLE:
			LOG_INF("  2D GNSS Position Available");
			break;
		case GNSS_3D_POSITION_AVAILABLE:
			LOG_INF("  3D GNSS Position Available");
			k_sem_give(&gnss_sem);
			break;
		case GNSS_FIX_CHANGED_TO_INVALID:
			LOG_INF("  GNSS Fix Changed to Invalid Position");
			break;
		default:
			LOG_INF("  Unknown GNSS Position Event: %d", event->content.position_event);
			break;
		}
		break;

	case HL78XX_GNSS_EVENT_MODE_EXITED:
		/*
		 * GNSS mode exit complete.
		 * In airplane mode: modem stays in CFUN=4, user must restore LTE.
		 * In PSM mode:      driver auto-transitions to AWAIT_REGISTERED.
		 */
		if (event->content.status) {
			LOG_INF("GNSS mode exited (user-requested)");
		} else {
			LOG_INF("GNSS mode exited (auto-stopped by LTE)");
		}
		k_sem_give(&gnss_exit_sem);
		break;
#endif /* CONFIG_HL78XX_GNSS */

#if defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE)
#ifdef CONFIG_MODEM_HL78XX_EDRX
	case HL78XX_EDRX_IDLE_UPDATE:
#ifdef CONFIG_MODEM_HL78XX_12
		mdm_edrxev_current = event->content.edrx_event;
#endif /* CONFIG_MODEM_HL78XX_12 */
		if (event->content.edrx_event == HL78XX_EDRX_EVENT_IDLE_ENTER) {
			LOG_INF("Modem entered eDRX idle mode");
#if defined(CONFIG_HL78XX_GNSS)
			k_sem_give(&gnss_sem);
#endif
		} else if (event->content.edrx_event == HL78XX_EDRX_EVENT_IDLE_EXIT) {
			LOG_INF("Modem exited eDRX idle mode");
		} else {
			LOG_INF("Modem eDRX event: NONE");
		}
		break;
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#ifdef CONFIG_MODEM_HL78XX_PSM
	case HL78XX_LTE_PSMEV_UPDATE:
#ifdef CONFIG_MODEM_HL78XX_12
		mdm_psmev_current = event->content.psm_event;
#endif /* CONFIG_MODEM_HL78XX_12 */
		if (event->content.psm_event == HL78XX_PSM_EVENT_ENTER) {
			LOG_INF("Modem entered PSM");
#ifdef CONFIG_HL78XX_GNSS
			k_sem_give(&gnss_sem);
#endif
		} else if (event->content.psm_event == HL78XX_PSM_EVENT_EXIT) {
			LOG_INF("Modem exited PSM");
		} else {
			LOG_INF("Modem PSM event: NONE");
		}
		break;
#endif /* CONFIG_MODEM_HL78XX_PSM */
	case HL78XX_CELLMEAS_UPDATE:
		LOG_INF("Cellular measurement update: RSRP=%d dBm", event->content.cellmeas.rsrp);
#if defined(CONFIG_MODEM_HL78XX_12) && defined(CONFIG_HL78XX_GNSS) &&                              \
	defined(CONFIG_MODEM_HL78XX_EDRX)
		if ((registration_status == CELLULAR_REGISTRATION_REGISTERED_HOME ||
		     registration_status == CELLULAR_REGISTRATION_REGISTERED_ROAMING) &&
		    mdm_edrxev_current == HL78XX_EDRX_EVENT_IDLE_EXIT) {
			mdm_edrxev_current = HL78XX_EDRX_EVENT_IDLE_NONE;
			k_sem_give(&network_connected_sem);
		}
#endif /* CONFIG_MODEM_HL78XX_12 && CONFIG_HL78XX_GNSS */
		break;
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	default:
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
		LOG_DBG("%d HL78XX modem Event Received: %d", __LINE__, event->type);
#endif
		break;
	}
}

static void hl78xx_on_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, argv[0]);
#endif
}

/**
 * @brief Resolve test server hostname to an IPv4 address.
 * @param broker Pointer to sockaddr_in structure to store the resolved address.
 * @return 0 on success, negative errno on failure.
 */
static int resolve_broker_addr(struct net_sockaddr_in *broker)
{
	int ret;
	struct zsock_addrinfo *ai = NULL;

	const struct zsock_addrinfo hints = {
		.ai_family = NET_PF_INET,
		.ai_socktype = NET_SOCK_STREAM,
		.ai_protocol = 0,
	};
	char port_string[6] = {0};

	snprintf(port_string, sizeof(port_string), "%d", TEST_SERVER_PORT);
	ret = zsock_getaddrinfo(TEST_SERVER_ENDPOINT, port_string, &hints, &ai);
	if (ret == 0) {
		char addr_str[NET_INET_ADDRSTRLEN];

		memcpy(broker, ai->ai_addr,
		       MIN(ai->ai_addrlen, sizeof(struct net_sockaddr_storage)));

		zsock_inet_ntop(NET_PF_INET, &broker->sin_addr, addr_str, sizeof(addr_str));
		LOG_INF("Resolved: %s:%u", addr_str, net_htons(broker->sin_port));
	} else {
		LOG_ERR("failed to resolve hostname err = %d (errno = %d)", ret, errno);
	}

	zsock_freeaddrinfo(ai);

	return ret;
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", hl78xx_on_ok);

HL78XX_EVT_MONITOR(listener_evt, evnt_listener);
#if defined(CONFIG_HL78XX_GNSS) && defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE)
static void k_work_wake_fn(struct k_work *work)
{
	/* Wake modem from LPM sleep so the GNSS engine stays active */
	LOG_DBG("LPM wake work triggered, waking modem for GNSS");
	hl78xx_wakeup_modem(modem);
}
#endif /* CONFIG_HL78XX_GNSS && CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

int main(void)
{
	int ret = 0;

	if (device_is_ready(modem) == false) {
		LOG_ERR("%d, %s Device %s is not ready", __LINE__, __func__, modem->name);
	}
#ifdef CONFIG_PM_DEVICE
	LOG_INF("Powering on modem");
	pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME);
#endif
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
	}
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE)) {
		LOG_INF("Modem configured to boot in fully functional mode.");
	} else {
		LOG_INF("Modem configured to boot in minimum functionality mode.");
#if defined(CONFIG_HL78XX_GNSS) && defined(CONFIG_MODEM_HL78XX_BOOT_IN_AIRPLANE_MODE)
		/* In airplane-mode GNSS path, search GNSS before LTE */
		LOG_INF("Airplane-mode boot: searching GNSS before LTE");
		goto gnss_demo_start;
#endif
	}
#if defined(CONFIG_MODEM_HL78XX_AIRVANTAGE) || defined(CONFIG_HL78XX_GNSS)
app_rerun:
#endif
	LOG_INF("Waiting for network connection...");
	k_sem_take(&network_connected_sem, K_FOREVER);

	/* Modem information */
	char manufacturer[MDM_MANUFACTURER_LENGTH] = {0};
	char fw_ver[MDM_REVISION_LENGTH] = {0};
	char apn[MDM_APN_MAX_LENGTH] = {0};
	char operator[MDM_MODEL_LENGTH] = {0};
	char imei[MDM_IMEI_LENGTH] = {0};
	char serial_number[MDM_SERIAL_NUMBER_LENGTH] = {0};
	enum hl78xx_cell_rat_mode tech;
	enum cellular_registration_status status;
	int16_t signal_strength = 0;
	uint32_t current_baudrate = 0;
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
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

	cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_MANUFACTURER, manufacturer,
				sizeof(manufacturer));

	cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_FW_VERSION, fw_ver, sizeof(fw_ver));

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_APN, apn, sizeof(apn));

	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_SERIAL_NUMBER, serial_number,
			      sizeof(serial_number));

	cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_IMEI, imei, sizeof(imei));
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	/* In auto rat mode, get the current rat from the modem status */
	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_CURRENT_RAT,
			      (enum cellular_access_technology *)&tech, sizeof(tech));
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */
	/* Get the current registration status */
	cellular_get_registration_status(modem, hl78xx_rat_to_access_tech(tech), &status);
	/* Get the current signal strength */
#ifdef CONFIG_MODEM_HL78XX_RAT_GSM
	cellular_get_signal(modem, CELLULAR_SIGNAL_RSSI, &signal_strength);
#else
	cellular_get_signal(modem, CELLULAR_SIGNAL_RSRP, &signal_strength);
#endif
	/* Get the current network operator name */
	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_NETWORK_OPERATOR, operator,
			      sizeof(operator));
	/* Get the current baudrate */
	hl78xx_get_modem_info(modem, HL78XX_MODEM_INFO_CURRENT_BAUD_RATE, &current_baudrate,
			      sizeof(current_baudrate));

	LOG_RAW("\n**********************************************************\n");
	LOG_RAW("********* Hello HL78XX Modem Sample Application **********\n");
	LOG_RAW("**********************************************************\n");
	LOG_INF("Manufacturer: %s", manufacturer);
	LOG_INF("Firmware Version: %s", fw_ver);
	LOG_INF("Module Serial Number: %s", serial_number);
	LOG_INF("APN: \"%s\"", apn);
	LOG_INF("Imei: %s", imei);
	LOG_INF("RAT: %s", rat_get_in_string(tech));
	LOG_INF("Connection status: %s(%d)", reg_status_get_in_string(status), status);
#ifdef CONFIG_MODEM_HL78XX_RAT_GSM
	LOG_INF("RSSI : %d", signal_strength);
#else
	LOG_INF("RSRP : %d", signal_strength);
#endif
	LOG_INF("Operator: %s", (strlen(operator) > 0) ? operator : "\"\"");
	LOG_INF("Current Baudrate: %dbps", current_baudrate);
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
#if defined(CONFIG_HL78XX_GNSS) && defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE)
app_post_lpm_run:
#endif /* CONFIG_HL78XX_GNSS && CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	struct net_sockaddr_in test_endpoint_addr;

	LOG_INF("Test endpoint: %s:%d", TEST_SERVER_ENDPOINT, TEST_SERVER_PORT);
	resolve_broker_addr(&test_endpoint_addr);
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE_UA_CONNECT_AIRVANTAGE
	LOG_INF("Starting AirVantage DM session...");
	hl78xx_start_airvantage_dm_session(modem);
	k_sem_reset(&fota_complete_rerun);
	LOG_INF("Waiting for AirVantage FOTA Creation...");
	/* Wait for FOTA download request, max 120 seconds */
	ret = k_sem_take(&fota_complete_rerun, K_SECONDS(120));
	if (ret < 0) {
		LOG_WRN("AirVantage DM session timed out waiting for FOTA download request.%d",
			ret);
	} else {
		k_sem_reset(&fota_complete_rerun);
		LOG_INF("Waiting for AirVantage FOTA Completion...");
		/* Wait for FOTA completion */
		ret = k_sem_take(&fota_complete_rerun, K_FOREVER);
		if (fota_update_status == (int)WDSI_FIRMWARE_UPDATE_SUCCESS) {
			LOG_INF("FOTA update successful, restarting application to apply update.");
#ifdef CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE
			goto app_rerun;
#endif
		} else if (fota_update_status == (int)WDSI_FIRMWARE_UPDATE_FAILED) {
			LOG_WRN("FOTA update failed.");
		} else {
			LOG_WRN("FOTA update status unknown.");
		}
	}

#else
	LOG_WRN("AirVantage User Agreement not accepted, cannot start DM session.");
#endif
#endif  /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
	/* -------------------------------------------------------------------------
	 * GNSS Demo
	 * -------------------------------------------------------------------------
	 */
#ifdef CONFIG_HL78XX_GNSS
#ifdef CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE
	/* Download A-GNSS data while LTE connection is available */
	struct hl78xx_agnss_status agnss_status;

	ret = hl78xx_gnss_assist_data_get_status(modem, &agnss_status);
	if (ret == 0 && agnss_status.mode == HL78XX_AGNSS_MODE_VALID) {
		LOG_INF("A-GNSS valid, expires in %d days %d:%d", agnss_status.days,
			agnss_status.hours, agnss_status.minutes);
	}
	LOG_DBG("A-GNSS mode: %d", agnss_status.mode);
	/* If A-GNSS data is invalid or expiring soon, download new data */
	if (agnss_status.mode == HL78XX_AGNSS_MODE_INVALID) {
		/*
		 * Note 1: Downloading A-GNSS data may take several minutes
		 * depending on network conditions.
		 *
		 * Note 2: The AT+GNSSAD command may occasionally return
		 * CME ERROR 60. Retry if this occurs.
		 */
		ret = hl78xx_gnss_assist_data_download(modem, HL78XX_AGNSS_DAYS_2);
		if (ret < 0) {
			LOG_ERR("Failed to download A-GNSS data: %d", ret);
		} else {
			LOG_INF("A-GNSS data download started");
		}
	}
#endif /* CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE */
#ifdef CONFIG_MODEM_HL78XX_BOOT_IN_AIRPLANE_MODE
gnss_demo_start:
#endif /* CONFIG_MODEM_HL78XX_BOOT_IN_AIRPLANE_MODE */
	LOG_RAW("\n**********************************************************\n");
	LOG_RAW("************* HL78XX GNSS Demonstration ******************\n");
	LOG_RAW("**********************************************************\n");

	const struct device *gnss_dev = GNSS_DEVICE;

	/* Check if GNSS device is ready */
	if (!device_is_ready(gnss_dev)) {
		LOG_ERR("GNSS device %s is not ready!", gnss_dev->name);
		goto gnss_demo_end;
	}
	k_sem_reset(&gnss_sem);
	LOG_INF("GNSS device ready: %s", gnss_dev->name);

#if defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE) &&                                                 \
	!defined(CONFIG_MODEM_HL78XX_BOOT_IN_AIRPLANE_MODE) &&                                     \
	!defined(CONFIG_MODEM_HL78XX_POWER_DOWN)
	/* ---------------------------------------------------------------
	 * PSM/eDRX GNSS path (requires modem to be in LTE mode, not airplane)
	 * ---------------------------------------------------------------
	 * Per HL78xx GNSS App Note 5.3: "GNSS can be used in PSM mode"
	 * because the LTE modem is hibernating and the RF Rx path is free.
	 *
	 * Flow:
	 * 1. Request GNSS mode (sets pending flag)
	 * 2. Wait for modem to enter PSM sleep
	 * 3. Driver auto-wakes modem and initialises GNSS engine
	 * 4. Configure GNSS and queue search
	 * 5. Wait for fix
	 * 6. Exit GNSS mode → driver auto-returns to AWAIT_REGISTERED
	 * 7. LTE re-registers, ready for data or next PSM cycle
	 */
	LOG_INF("LPM GNSS mode: requesting GNSS (pending until PSM/eDRX entry)...");
	k_sem_reset(&gnss_sem);
	ret = hl78xx_enter_gnss_mode(gnss_dev);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to request GNSS mode: %d", ret);
		goto gnss_demo_end;
	}
#ifdef CONFIG_MODEM_HL78XX_EDRX
	/* Wait for modem to enter PSM/eDRX.
	 * In a real application this could take minutes/hours depending on
	 * the configured T3412 (periodic TAU) value.
	 */
	LOG_INF("Waiting for modem to enter eDRX...%d ms left",
		hl78xx_edrx_get_time_to_sleep(modem));
#endif /* CONFIG_MODEM_HL78XX_EDRX */
	ret = k_sem_take(&gnss_sem, K_FOREVER);
	if (ret < 0) {
		LOG_WRN("PSM/eDRX entry wait failed: %d", ret);
		goto gnss_demo_end;
	}
	/* Wait for GNSS engine ready (auto-triggered by the driver after
	 * it wakes the modem from PSM/eDRX sleep)
	 */
	k_sem_reset(&gnss_sem);
	LOG_INF("PSM/eDRX entered - driver will wake modem within 60s for GNSS");
	k_work_schedule(&gnss_wake_work, K_SECONDS(60));
	ret = k_sem_take(&gnss_sem, K_SECONDS(160));
	if (ret < 0) {
		LOG_WRN("GNSS engine init timeout after PSM/eDRX wake");
		goto gnss_demo_end;
	}
	LOG_INF("GNSS engine initialized (LPM path)");

#else  /* Airplane-mode GNSS path (boot or driver-initiated CFUN=4) */
	/* ---------------------------------------------------------------
	 * Airplane-mode GNSS path
	 * ---------------------------------------------------------------
	 * Enter GNSS mode - this will:
	 * 1. Put modem in airplane mode (AT+CFUN=4)
	 * 2. Initialize GNSS engine
	 * 3. Fire HL78XX_GNSS_ENGINE_READY event when complete
	 * Note: GNSS shares RF path with LTE, so LTE must be disabled
	 */
	LOG_INF("Entering GNSS mode...");
	ret = hl78xx_enter_gnss_mode(gnss_dev);
	if (ret < 0) {
		LOG_ERR("Failed to enter GNSS mode: %d", ret);
		goto gnss_demo_end;
	}

	/* Wait for GNSS engine initialization to complete
	 * HL78XX_GNSS_ENGINE_READY fires after GNSS init script succeeds
	 */
	ret = k_sem_take(&gnss_sem, K_SECONDS(30));
	if (ret < 0) {
		LOG_WRN("GNSS engine init timeout, failing demo.");
		goto gnss_demo_end;
	}
	LOG_INF("GNSS engine initialized");
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE && !BOOT_IN_AIRPLANE_MODE */

#ifdef CONFIG_PM_DEVICE
	/* Power on GNSS */
	LOG_INF("Powering on GNSS...");
	ret = pm_device_action_run(gnss_dev, PM_DEVICE_ACTION_RESUME);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to power on GNSS: %d", ret);
	} else {
		LOG_INF("GNSS powered on");
	}
#endif

	/* Get current fix rate */
	uint32_t fix_rate_ms;

	ret = gnss_get_fix_rate(gnss_dev, &fix_rate_ms);
	if (ret == 0) {
		LOG_INF("Current fix rate: %u ms", fix_rate_ms);
	}

	/* Get supported systems */
	gnss_systems_t systems;

	ret = gnss_get_supported_systems(gnss_dev, &systems);
	if (ret == 0) {
		LOG_INF("Supported GNSS systems: 0x%08x", systems);
		if (systems & GNSS_SYSTEM_GPS) {
			LOG_INF("  - GPS");
		}
		if (systems & GNSS_SYSTEM_GLONASS) {
			LOG_INF("  - GLONASS");
		}
	}
	LOG_INF("Setting GNSS fix rate to 1000 ms...");
	/* Set fix rate to 1000 ms */
	ret = gnss_set_fix_rate(gnss_dev, 1000);
	if (ret == 0) {
		LOG_INF("GNSS fix rate set to 1000 ms");
	}
	LOG_INF("Enabling GPS and GLONASS systems...");
	/* Enable GPS and GLONASS */
	ret = gnss_set_enabled_systems(gnss_dev, GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS);
	/* Configure GNSS */
	LOG_INF("Configuring GNSS for NMEA output...");
#ifdef CONFIG_HL78XX_GNSS_SOURCE_NMEA
	hl78xx_gnss_set_nmea_output(gnss_dev, NMEA_OUTPUT_SAME_PORT);
#else
	hl78xx_gnss_set_nmea_output(gnss_dev, NMEA_OUTPUT_NONE);
#endif
	/* Set search timeout (0 = no timeout) */
	hl78xx_gnss_set_search_timeout(gnss_dev, 20000);
	k_sem_reset(&gnss_sem);
	k_sem_reset(&gnss_exit_sem);

	/* Queue GNSS search - already in GNSS mode, search will start */
	LOG_INF("Queueing GNSS search...");
	ret = hl78xx_queue_gnss_search(gnss_dev);
	if (ret) {
		LOG_INF("GNSS search already queued or in progress");
	} else {
		LOG_INF("GNSS search started - waiting for fix...");
	}

	/* Wait for GNSS fix or search timeout */
	LOG_INF("Collecting GNSS data...");
	ret = k_sem_take(&gnss_sem, K_FOREVER);
	if (ret < 0) {
		LOG_WRN("GNSS data collection failed");
		goto gnss_demo_end;
	}
#ifdef CONFIG_HL78XX_GNSS_SOURCE_LOC
	LOG_INF("GNSS data collection complete.");
	ret = hl78xx_gnss_get_latest_known_fix(GNSS_DEVICE);
	if (ret == -EBUSY) {
		LOG_DBG("Skipping GNSSLOC query - modem busy");
	}
#endif /* CONFIG_HL78XX_GNSS_SOURCE_LOC */
	/* Exit GNSS mode
	 * This will:
	 * 1. Stop GNSS search
	 * 2. Fire HL78XX_GNSS_EVENT_MODE_EXITED when complete
	 * In airplane mode: modem stays in CFUN=4, user must restore LTE.
	 * In PSM mode: driver auto-transitions back to AWAIT_REGISTERED.
	 */
	LOG_INF("Exiting GNSS mode...");
	ret = hl78xx_exit_gnss_mode(gnss_dev);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to exit GNSS mode: %d", ret);
	} else {
		/* Wait for GNSS mode exit to complete */
		LOG_INF("Waiting for GNSS mode exit to complete...");
		ret = k_sem_take(&gnss_exit_sem, K_FOREVER);
		if (ret < 0) {
			LOG_WRN("GNSS mode exit timeout");
		} else {
			LOG_INF("GNSS mode exit complete");
		}
	}

#if defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE) &&                                                 \
	!defined(CONFIG_MODEM_HL78XX_BOOT_IN_AIRPLANE_MODE) &&                                     \
	!defined(CONFIG_MODEM_HL78XX_POWER_DOWN)
	/*
	 * LPM GNSS return path:
	 * - HL7812 PSM/eDRX: modem stayed in CFUN=1 during GNSS (no airplane
	 *   mode needed). Driver auto-transitions to AWAIT_REGISTERED.
	 *   Send a ping to trigger LTE activity and exit PSM sleep.
	 * - HL7800 PSM/eDRX: modem was put in CFUN=4 for GNSS (HL7800 cannot
	 *   do GNSS while LTE is active). Must restore CFUN=1 to return to LTE.
	 */
	LOG_INF("LPM GNSS done - waiting for network re-registration...");
	k_sem_reset(&network_connected_sem);
	k_sem_take(&network_connected_sem, K_FOREVER);
	goto app_post_lpm_run;
#else
	static bool is_reset_required;

	/* Airplane mode path: must explicitly restore full functionality */
	/* If modem did not boot in fully functional mode, reset is required to configure lte */
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE) == false &&
	    is_reset_required == false) {
		is_reset_required = true;
	} else {
		is_reset_required = false;
	}
	/* Now explicitly return to LTE mode */
	LOG_INF("Returning to LTE mode (AT+CFUN=1)...");
	ret = hl78xx_api_func_set_phone_functionality(modem, HL78XX_FULLY_FUNCTIONAL,
						      is_reset_required);
	if (ret < 0) {
		LOG_ERR("Failed to set full functionality: %d", ret);
	} else {
		LOG_INF("Phone functionality restored, modem returning to LTE");
	}
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE && !BOOT_IN_AIRPLANE_MODE */

gnss_demo_end:
	LOG_RAW("**********************************************************\n\n");
#ifdef CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE
	/* Delete assistance data */
	ret = hl78xx_gnss_assist_data_delete(gnss_dev);
#endif /* CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE */
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE) == false) {
		LOG_INF("Demo complete, entering LTE mode...");
		goto app_rerun;
	}
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	LOG_INF("Demo complete, re-entering LPM cycle...");
	k_sem_reset(&network_connected_sem);
	k_sem_take(&network_connected_sem, K_FOREVER);
	goto app_post_lpm_run;
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
#endif /* CONFIG_HL78XX_GNSS */

	LOG_INF("Sample application finished.");
	LOG_INF("Entering monitoring loop...");

	/* Main loop - keep running and receiving callbacks */
	while (1) {
		k_sleep(K_SECONDS(60));
		LOG_INF("Uptime: %lld seconds", k_uptime_get() / 1000);
	}

	return 0;
}
