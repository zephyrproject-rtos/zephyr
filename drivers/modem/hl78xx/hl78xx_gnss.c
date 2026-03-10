/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <string.h>

#include "hl78xx.h"
#include "hl78xx_chat.h"
#include "hl78xx_gnss.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hl78xx_gnss, CONFIG_GNSS_LOG_LEVEL);

#define HL78XX_GNSS_PM_TIMEOUT       K_MSEC(500U)
#define HL78XX_GNSS_SCRIPT_TIMEOUT_S 10U

/* AT+GNSSCONF configuration type */
#define GNSS_CONF_STATIC_FILTER  4  /* Enable/disable static filter */
#define GNSS_CONF_CONSTELLATIONS 10 /* Configure satellite constellations */

/* AT+GNSSNMEA sentence bit mask */
#define GNSS_NMEA_GGA  BIT(0)  /* GGA sentence */
#define GNSS_NMEA_GSA  BIT(1)  /* GSA sentence */
#define GNSS_NMEA_GSV  BIT(2)  /* GSV sentence */
#define GNSS_NMEA_RMC  BIT(3)  /* RMC sentence */
#define GNSS_NMEA_VTG  BIT(4)  /* VTG sentence */
#define GNSS_NMEA_GNS  BIT(5)  /* GNS sentence */
#define GNSS_NMEA_GST  BIT(6)  /* GST sentence */
#define GNSS_NMEA_GLL  BIT(7)  /* GLL sentence */
#define GNSS_NMEA_ZDA  BIT(8)  /* ZDA sentence */
#define GNSS_NMEA_PEPU BIT(12) /* PEPU sentence (position/velocity uncertainty) */

/* Minimum NMEA mask for GNSS URCs and location data (GGA, GST, RMC required) */
#define GNSS_NMEA_MASK_MINIMUM (GNSS_NMEA_GGA | GNSS_NMEA_GST | GNSS_NMEA_RMC)

/* GNSS start modes from Kconfig */
#if defined(CONFIG_HL78XX_GNSS_START_MODE_AUTO)
#define HL78XX_GNSS_START_MODE 0 /* AUTO mode */
#elif defined(CONFIG_HL78XX_GNSS_START_MODE_WARM)
#define HL78XX_GNSS_START_MODE 1 /* WARM mode */
#elif defined(CONFIG_HL78XX_GNSS_START_MODE_COLD)
#define HL78XX_GNSS_START_MODE 2 /* COLD mode */
#elif defined(CONFIG_HL78XX_GNSS_START_MODE_FACTORY)
#define HL78XX_GNSS_START_MODE 3 /* FACTORY mode */
#else
#define HL78XX_GNSS_START_MODE 0 /* Default to AUTO */
#endif

/* GNSS constellations from Kconfig */
#if defined(CONFIG_HL78XX_GNSS_CONSTELLATIONS_GPS_GLONASS)
#define HL78XX_GNSS_CONSTELLATION_CONFIG 1 /* GPS + GLONASS */
#else
#define HL78XX_GNSS_CONSTELLATION_CONFIG 0 /* GPS only */
#endif

/* -------------------------------------------------------------------------
 * Locking and PM helpers
 * -------------------------------------------------------------------------
 */
static void hl78xx_gnss_lock(const struct device *dev)
{
	struct hl78xx_gnss_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void hl78xx_gnss_unlock(const struct device *dev)
{
	struct hl78xx_gnss_data *data = dev->data;

	k_sem_give(&data->lock);
}

static void hl78xx_gnss_pm_changed(const struct device *dev)
{
	struct hl78xx_gnss_data *data = dev->data;

	data->pm_deadline = sys_timepoint_calc(HL78XX_GNSS_PM_TIMEOUT);
}

static void hl78xx_gnss_bus_pipe_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
					 void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_BUS_OPENED);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_BUS_CLOSED);
		break;

	default:
		break;
	}
}

/* -------------------------------------------------------------------------
 * GNSS Search State Machine
 * -------------------------------------------------------------------------
 */

static const char *gnss_search_state_str(enum hl78xx_gnss_search_state state)
{
	switch (state) {
	case HL78XX_GNSS_SEARCH_STATE_IDLE:
		return "IDLE";
	case HL78XX_GNSS_SEARCH_STATE_WAITING_FOR_AIRPLANE:
		return "WAITING_FOR_AIRPLANE";
	case HL78XX_GNSS_SEARCH_STATE_STARTING:
		return "STARTING";
	case HL78XX_GNSS_SEARCH_STATE_SEARCHING:
		return "SEARCHING";
	case HL78XX_GNSS_SEARCH_STATE_STOPPING:
		return "STOPPING";
	case HL78XX_GNSS_SEARCH_STATE_BLOCKED_BY_LTE:
		return "BLOCKED_BY_LTE";
	default:
		return "UNKNOWN";
	}
}

void gnss_set_search_state(struct hl78xx_gnss_data *gnss, enum hl78xx_gnss_search_state new_state)
{
	if (gnss->search_state != new_state) {
		LOG_DBG("GNSS search state: %s -> %s", gnss_search_state_str(gnss->search_state),
			gnss_search_state_str(new_state));
		gnss->search_state = new_state;
	}
}

struct hl78xx_gnss_data *hl78xx_get_gnss_data(struct hl78xx_data *data)
{
	if (data == NULL || data->gnss_dev == NULL) {
		return NULL;
	}
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;

	if (data_nmea == NULL || data_nmea->gnss == NULL) {
		return NULL;
	}
	return data_nmea->gnss->data;
}

bool hl78xx_gnss_check_and_clear_pending(struct hl78xx_data *data)
{
	struct hl78xx_gnss_data *gnss = hl78xx_get_gnss_data(data);

	if (gnss != NULL && gnss->gnss_mode_enter_pending) {
		gnss->gnss_mode_enter_pending = false;
		return true;
	}
	return false;
}

bool hl78xx_gnss_is_pending(struct hl78xx_data *data)
{
	struct hl78xx_gnss_data *gnss = hl78xx_get_gnss_data(data);

	if (gnss != NULL) {
		return gnss->gnss_mode_enter_pending;
	}
	return false;
}

bool hl78xx_gnss_is_active(struct hl78xx_gnss_data *gnss)
{
	if (gnss == NULL) {
		return false;
	}
	return gnss->search_state != HL78XX_GNSS_SEARCH_STATE_IDLE;
}

bool hl78xx_is_in_gnss_mode(struct hl78xx_data *data)
{
	if (data == NULL) {
		return false;
	}
	return data->status.state == MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT ||
	       data->status.state == MODEM_HL78XX_STATE_GNSS_SEARCH_STARTED;
}

enum hl78xx_gnss_search_state hl78xx_gnss_get_search_state(struct hl78xx_gnss_data *gnss)
{
	return gnss->search_state;
}

bool hl78xx_gnss_is_searching(struct hl78xx_gnss_data *gnss)
{
	return gnss->search_state == HL78XX_GNSS_SEARCH_STATE_SEARCHING;
}

bool hl78xx_gnss_search_is_queued(struct hl78xx_gnss_data *gnss)
{
	return gnss->search_state == HL78XX_GNSS_SEARCH_STATE_WAITING_FOR_AIRPLANE;
}

bool hl78xx_gnss_queue_search(struct hl78xx_gnss_data *gnss)
{
	bool was_queued = hl78xx_gnss_search_is_queued(gnss);

	if (gnss->search_state == HL78XX_GNSS_SEARCH_STATE_SEARCHING) {
		LOG_DBG("GNSS already searching, ignoring queue request");
		return true;
	}

	if (gnss->search_state == HL78XX_GNSS_SEARCH_STATE_STARTING) {
		LOG_DBG("GNSS start in progress, ignoring queue request");
		return true;
	}

	gnss_set_search_state(gnss, HL78XX_GNSS_SEARCH_STATE_WAITING_FOR_AIRPLANE);

	hl78xx_delegate_event(gnss->parent_data, MODEM_HL78XX_EVENT_GNSS_START_REQUESTED);
	return was_queued;
}

bool hl78xx_gnss_clear_search_queue(struct hl78xx_gnss_data *gnss)
{
	bool was_queued = hl78xx_gnss_search_is_queued(gnss);

	if (gnss->search_state == HL78XX_GNSS_SEARCH_STATE_WAITING_FOR_AIRPLANE ||
	    gnss->search_state == HL78XX_GNSS_SEARCH_STATE_BLOCKED_BY_LTE) {
		gnss_set_search_state(gnss, HL78XX_GNSS_SEARCH_STATE_IDLE);
	}

	return was_queued;
}

int hl78xx_gnss_start_search(struct hl78xx_gnss_data *gnss,
			     const struct hl78xx_gnss_search_config *config)
{
	if (gnss == NULL) {
		return -EINVAL;
	}

	/* Check current state */
	switch (gnss->search_state) {
	case HL78XX_GNSS_SEARCH_STATE_SEARCHING:
		LOG_WRN("GNSS search already in progress");
		return -EALREADY;

	case HL78XX_GNSS_SEARCH_STATE_STARTING:
		LOG_WRN("GNSS start already in progress");
		return -EBUSY;

	case HL78XX_GNSS_SEARCH_STATE_STOPPING:
		LOG_WRN("GNSS stop in progress, please wait");
		return -EBUSY;

	default:
		break;
	}

	/* Apply configuration if provided */
	if (config != NULL) {
		gnss->output_port = config->output_port;
		gnss->search_timeout_ms = config->timeout_ms;
		/* Store config for later use */
		gnss->search_config = *config;
	}

	/* Queue the search - it will start when modem is in airplane mode */
	hl78xx_gnss_queue_search(gnss);

	return 0;
}

int hl78xx_gnss_stop_search(struct hl78xx_gnss_data *gnss)
{
	if (gnss == NULL) {
		return -EINVAL;
	}

	switch (gnss->search_state) {
	case HL78XX_GNSS_SEARCH_STATE_IDLE:
		LOG_DBG("GNSS already idle");
		return 0;

	case HL78XX_GNSS_SEARCH_STATE_WAITING_FOR_AIRPLANE:
	case HL78XX_GNSS_SEARCH_STATE_BLOCKED_BY_LTE:
		/* Just clear the queue */
		hl78xx_gnss_clear_search_queue(gnss);
		return 0;

	case HL78XX_GNSS_SEARCH_STATE_STOPPING:
		LOG_DBG("GNSS stop already in progress");
		return -EALREADY;

	case HL78XX_GNSS_SEARCH_STATE_SEARCHING:
	case HL78XX_GNSS_SEARCH_STATE_STARTING:
		/* Request stop via event */
		hl78xx_delegate_event(gnss->parent_data, MODEM_HL78XX_EVENT_GNSS_STOP_REQUESTED);
		return 0;

	default:
		return -EINVAL;
	}
}
/* -------------------------------------------------------------------------
 * GNSS URC Handlers
 * -------------------------------------------------------------------------
 */
#ifdef CONFIG_HL78XX_GNSS_SOURCE_NMEA
void hl78xx_gnss_nmea0183_match_gga(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	gnss_nmea0183_match_gga_callback(chat, argv, argc, data->gnss_dev->data);
}

void hl78xx_gnss_nmea0183_match_rmc(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	gnss_nmea0183_match_rmc_callback(chat, argv, argc, data->gnss_dev->data);
}

void hl78xx_gnss_nmea0183_match_gsv(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	gnss_nmea0183_match_gsv_callback(chat, argv, argc, data->gnss_dev->data);
}
#endif /* CONFIG_HL78XX_GNSS_SOURCE_NMEA */
#ifdef CONFIG_HL78XX_GNSS_AUX_DATA_PARSER
void hl78xx_gnss_nmea0183_match_gsa(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	gnss_nmea0183_match_gsa_callback(chat, argv, argc, data->gnss_dev->data);
}

void hl78xx_gnss_nmea0183_match_gst(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	gnss_nmea0183_match_gst_callback(chat, argv, argc, data->gnss_dev->data);
}

void hl78xx_gnss_nmea_match_epu(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	gnss_nmea0183_match_epu_callback(chat, argv, argc, data->gnss_dev->data);
}
#endif /* CONFIG_HL78XX_GNSS_AUX_DATA_PARSER */
/* -------------------------------------------------------------------------
 * GNSSLOC URC Handlers
 * -------------------------------------------------------------------------
 * These handlers parse AT+GNSSLOC? response fields and convert them to
 * Zephyr's navigation_data format. The parser helper functions are in
 * hl78xx_gnss_parsers.c for reuse.
 */

void hl78xx_gnss_on_gnssloc(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	ARG_UNUSED(chat);
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;

	/* Reset navigation data for new GNSSLOC response */
	memset(&data_nmea->data.nav_data, 0, sizeof(data_nmea->data.nav_data));
	data_nmea->data.info.fix_status = GNSS_FIX_STATUS_NO_FIX;
	data_nmea->data.info.fix_quality = GNSS_FIX_QUALITY_INVALID;

	LOG_DBG("GNSSLOC header received");
}

void hl78xx_gnss_on_gnssloc_latitude(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	int64_t latitude_ndeg;
	int ret;

	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_WRN("GNSSLOC Latitude: no data");
		return;
	}

	ret = gnssloc_dms_to_ndeg(argv[1], &latitude_ndeg);
	if (ret < 0) {
		LOG_WRN("GNSSLOC Latitude: parse error %d for '%s'", ret, argv[1]);
		return;
	}

	data_nmea->data.nav_data.latitude = latitude_ndeg;
	LOG_DBG("GNSSLOC Latitude: %s -> %lld ndeg", argv[1], latitude_ndeg);
}

void hl78xx_gnss_on_gnssloc_longitude(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	int64_t longitude_ndeg;
	int ret;

	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_WRN("GNSSLOC Longitude: no data");
		return;
	}

	ret = gnssloc_dms_to_ndeg(argv[1], &longitude_ndeg);
	if (ret < 0) {
		LOG_WRN("GNSSLOC Longitude: parse error %d for '%s'", ret, argv[1]);
		return;
	}

	data_nmea->data.nav_data.longitude = longitude_ndeg;
	LOG_DBG("GNSSLOC Longitude: %s -> %lld ndeg", argv[1], longitude_ndeg);
}

void hl78xx_gnss_on_gnssloc_gpstime(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	int ret;

	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_WRN("GNSSLOC GpsTime: no data");
		return;
	}

	ret = gnssloc_parse_gpstime(argv[1], &data_nmea->data.utc);
	if (ret < 0) {
		LOG_WRN("GNSSLOC GpsTime: parse error %d for '%s'", ret, argv[1]);
		return;
	}

	LOG_DBG("GNSSLOC GpsTime: %s -> %02d:%02d:%02d %02d/%02d/%02d", argv[1],
		data_nmea->data.utc.hour, data_nmea->data.utc.minute,
		data_nmea->data.utc.millisecond / 1000, data_nmea->data.utc.month_day,
		data_nmea->data.utc.month, data_nmea->data.utc.century_year);
}

void hl78xx_gnss_on_gnssloc_fixtype(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;

	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_WRN("GNSSLOC FixType: no data");
		data_nmea->data.info.fix_status = GNSS_FIX_STATUS_NO_FIX;
		return;
	}

	/* Parse fix type: "2D", "3D", or no fix
	 * 3D fix = full position (lat, lon, alt) - standard SPS quality
	 * 2D fix = horizontal only (lat, lon) - estimated quality (less reliable)
	 */
	if (strcmp(argv[1], "3D") == 0) {
		data_nmea->data.info.fix_status = GNSS_FIX_STATUS_GNSS_FIX;
		data_nmea->data.info.fix_quality = GNSS_FIX_QUALITY_GNSS_SPS;
	} else if (strcmp(argv[1], "2D") == 0) {
		/* 2D fix is valid but less precise - use ESTIMATED quality */
		data_nmea->data.info.fix_status = GNSS_FIX_STATUS_ESTIMATED_FIX;
		data_nmea->data.info.fix_quality = GNSS_FIX_QUALITY_ESTIMATED;
	} else {
		data_nmea->data.info.fix_status = GNSS_FIX_STATUS_NO_FIX;
		data_nmea->data.info.fix_quality = GNSS_FIX_QUALITY_INVALID;
	}

	LOG_DBG("GNSSLOC FixType: %s -> fix_status=%d", argv[1], data_nmea->data.info.fix_status);
}

void hl78xx_gnss_on_gnssloc_hepe(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	int64_t hepe_milli;
	int ret;

	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_WRN("GNSSLOC HEPE: no data");
		return;
	}

	/* Parse HEPE in meters, store as hdop approximation */
	ret = gnssloc_parse_value_with_unit(argv[1], &hepe_milli);
	if (ret < 0) {
		LOG_WRN("GNSSLOC HEPE: parse error %d for '%s'", ret, argv[1]);
		return;
	}

	/* Store HEPE as hdop (approximation - HEPE ~ HDOP * UERE) */
	data_nmea->data.info.hdop = (uint32_t)(hepe_milli > 0 ? hepe_milli : 0);
	LOG_DBG("GNSSLOC HEPE: %s -> %d milli-m", argv[1], (int)hepe_milli);
}

void hl78xx_gnss_on_gnssloc_altitude(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	int64_t altitude_milli;
	int ret;

	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_WRN("GNSSLOC Altitude: no data");
		return;
	}

	/* Parse altitude in meters, convert to millimeters */
	ret = gnssloc_parse_value_with_unit(argv[1], &altitude_milli);
	if (ret < 0) {
		LOG_WRN("GNSSLOC Altitude: parse error %d for '%s'", ret, argv[1]);
		return;
	}

	/* Store altitude in millimeters */
	if (altitude_milli > INT32_MAX || altitude_milli < INT32_MIN) {
		LOG_WRN("GNSSLOC Altitude: value out of range");
		return;
	}

	data_nmea->data.nav_data.altitude = (int32_t)altitude_milli;
	LOG_DBG("GNSSLOC Altitude: %s -> %d mm", argv[1], (int)altitude_milli);
}

void hl78xx_gnss_on_gnssloc_altunc(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	ARG_UNUSED(chat);
	ARG_UNUSED(user_data);

	/* Altitude uncertainty is logged but not stored in navigation_data */
	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_DBG("GNSSLOC AltUnc: no data");
		return;
	}
	LOG_DBG("GNSSLOC AltUnc: %s", argv[1]);
}

void hl78xx_gnss_on_gnssloc_direction(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	int64_t bearing_milli;
	int ret;

	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_DBG("GNSSLOC Direction: no data");
		return;
	}

	/* Parse direction in degrees, convert to millidegrees */
	ret = gnss_parse_dec_to_milli(argv[1], &bearing_milli);
	if (ret < 0) {
		LOG_WRN("GNSSLOC Direction: parse error %d for '%s'", ret, argv[1]);
		return;
	}

	/* Validate and store bearing (0-359999 millidegrees) */
	if (bearing_milli < 0 || bearing_milli > 359999) {
		LOG_WRN("GNSSLOC Direction: value %lld out of range", bearing_milli);
		return;
	}

	data_nmea->data.nav_data.bearing = (uint32_t)bearing_milli;
	LOG_DBG("GNSSLOC Direction: %s -> %u mdeg", argv[1], data_nmea->data.nav_data.bearing);
}

void hl78xx_gnss_on_gnssloc_horspeed(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	uint32_t speed_mms;
	int ret;

	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_DBG("GNSSLOC HorSpeed: no data");
		return;
	}

	/* Parse speed in m/s, convert to mm/s */
	ret = gnssloc_parse_speed_to_mms(argv[1], &speed_mms);
	if (ret < 0) {
		LOG_WRN("GNSSLOC HorSpeed: parse error %d for '%s'", ret, argv[1]);
		return;
	}

	data_nmea->data.nav_data.speed = speed_mms;
	LOG_DBG("GNSSLOC HorSpeed: %s -> %u mm/s", argv[1], speed_mms);
}

void hl78xx_gnss_on_gnssloc_verspeed(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	ARG_UNUSED(chat);
	ARG_UNUSED(user_data);

	/* Vertical speed is logged but not stored in navigation_data */
	if (argc < 2 || argv[1] == NULL || strlen(argv[1]) == 0) {
		LOG_DBG("GNSSLOC VerSpeed: no data");
		return;
	}
	LOG_DBG("GNSSLOC VerSpeed: %s", argv[1]);
}

void hl78xx_gnss_on_gnssloc_OK(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	ARG_UNUSED(chat);
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;

	LOG_DBG("GNSSLOC completed successfully");

	/* Publish GNSS data if we have a valid fix */
	if (data_nmea->data.info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
		LOG_DBG("Publishing GNSS data: lat=%lld, lon=%lld, alt=%d, spd=%u, brg=%u",
			data_nmea->data.nav_data.latitude, data_nmea->data.nav_data.longitude,
			data_nmea->data.nav_data.altitude, data_nmea->data.nav_data.speed,
			data_nmea->data.nav_data.bearing);
		gnss_publish_data(data_nmea->gnss, &data_nmea->data);
	} else {
		LOG_DBG("No fix available, not publishing");
	}
}

void hl78xx_on_gnssnmea(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	struct hl78xx_gnss_data *data_gnss = data_nmea->gnss->data;

	if (argc < 5) {
		return;
	}

	data_gnss->output_port = ATOI(argv[1], 0, "gnssnmea_output");
	data_gnss->fix_interval_ms = ATOI(argv[2], 0, "gnssnmea_rate");

	LOG_DBG("NMEA type: %d, rate: %d", data_gnss->output_port, data_gnss->fix_interval_ms);
}

void hl78xx_on_gnssconf_enabledsys(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	struct hl78xx_gnss_data *data_gnss = data_nmea->gnss->data;
	int enabled_systems;

	if (argc < 2) {
		return;
	}

	enabled_systems = ATOI(argv[1], 0, "gnssconf_enabledsys");
	data_gnss->enabled_systems = (gnss_systems_t)enabled_systems;
	LOG_DBG("Enabled GNSS systems: 0x%02X", enabled_systems);
}

void hl78xx_on_gnssconf_enabledfilter(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	struct hl78xx_gnss_data *data_gnss = data_nmea->gnss->data;
	int static_filter;

	if (argc < 2) {
		return;
	}

	static_filter = ATOI(argv[1], 0, "gnssconf_enabledfilter");
	data_gnss->static_filter_enabled = (static_filter == 1) ? true : false;

	LOG_DBG("Static filter: %d", static_filter);
}

#ifdef CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE
/**
 * @brief Handler for +GNSSAD response (A-GNSS assistance data status)
 *
 * Format: +GNSSAD: <mode>[,<days>,<hours>,<minutes>]
 *
 * <mode>:
 *   0 = Data is not valid / empty
 *   1 = Data is valid
 *
 * When mode=1, additional fields indicate time until expiry:
 *   <days>    = Days remaining (1-28)
 *   <hours>   = Hours remaining (0-23)
 *   <minutes> = Minutes remaining (0-59)
 */
void hl78xx_on_gnssad(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea;
	struct hl78xx_gnss_data *data_gnss;
	int mode;

	if (data == NULL || data->gnss_dev == NULL) {
		LOG_ERR("GNSS device not available");
		return;
	}

	data_nmea = data->gnss_dev->data;
	if (data_nmea == NULL || data_nmea->gnss == NULL) {
		LOG_ERR("GNSS NMEA data not available");
		return;
	}

	data_gnss = data_nmea->gnss->data;
	if (data_gnss == NULL) {
		LOG_ERR("GNSS data structure not available");
		return;
	}

	if (argc < 2) {
		LOG_WRN("GNSSAD: insufficient arguments (%d)", argc);
		return;
	}

	/* Parse mode (validity indicator) */
	mode = ATOI(argv[1], 0, "gnssad_mode");
	data_gnss->agnss_status.mode = (enum hl78xx_agnss_mode)mode;

	if (mode == HL78XX_AGNSS_MODE_VALID && argc >= 5) {
		/* Parse expiry time fields */
		data_gnss->agnss_status.days = (uint8_t)ATOI(argv[2], 0, "gnssad_days");
		data_gnss->agnss_status.hours = (uint8_t)ATOI(argv[3], 0, "gnssad_hours");
		data_gnss->agnss_status.minutes = (uint8_t)ATOI(argv[4], 0, "gnssad_minutes");

		LOG_INF("A-GNSS data valid, expires in: %d days, %d hours, %d minutes",
			data_gnss->agnss_status.days, data_gnss->agnss_status.hours,
			data_gnss->agnss_status.minutes);
	} else if (mode == HL78XX_AGNSS_MODE_INVALID) {
		/* Data not valid - clear expiry fields */
		data_gnss->agnss_status.days = 0;
		data_gnss->agnss_status.hours = 0;
		data_gnss->agnss_status.minutes = 0;

		LOG_INF("A-GNSS data not valid or empty");
	} else {
		LOG_WRN("GNSSAD: unexpected mode=%d with argc=%d", mode, argc);
	}
}
#endif /* CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE */

/**
 * @brief Handler for +GNSSEV URC (GNSS event notifications)
 *
 * Format: +GNSSEV: <event_type>,<event_value>
 *
 * Event types:
 *   1 = GNSS start event
 *       - event_value: 0 = Failed (LTE active blocks GNSS)
 *                      1 = Success
 *   2 = GNSS stop event
 *   3 = GNSS fix status change
 *
 * Most critical: +GNSSEV: 1,0 indicates GNSS failed to start because
 * LTE modem is active (shared RF path conflict).
 */
void hl78xx_gnss_on_gnssev(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	struct hl78xx_gnss_data *data_gnss = data_nmea->gnss->data;
	struct hl78xx_evt gnss_evt;
	int event_type = 0;
	int event_value = 0;

	if (argc < 3) {
		LOG_WRN("GNSSEV URC: insufficient arguments (%u)", argc);
		return;
	}

	event_type = ATOI(argv[1], 0, "gnss_ev_type");
	event_value = ATOI(argv[2], 0, "gnss_ev_status");

	LOG_DBG("GNSSEV: type=%d, value=%d", event_type, event_value);

	switch (event_type) {
	case HL78XX_GNSSEV_INITIALISATION: /* GNSS initialization event */
		gnss_evt.type = HL78XX_GNSS_EVENT_INIT;
		gnss_evt.content.event_status = (enum hl78xx_event_status)event_value;
		event_dispatcher_dispatch(&gnss_evt);
		break;
	case HL78XX_GNSSEV_START: /* GNSS start event */
		if (event_value == 0) {
			LOG_ERR("GNSS start failed: LTE modem is active (shared RF path)");
			LOG_ERR("GNSS requires airplane mode (CFUN=4) or PSM/idle-eDRX");
			data_gnss->gnss_start_status = false;
			gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_BLOCKED_BY_LTE);
			/* Notify user that GNSS was blocked - they should enter airplane mode */
			gnss_evt.type = HL78XX_GNSS_EVENT_START_BLOCKED;
			gnss_evt.content.status = false;
			event_dispatcher_dispatch(&gnss_evt);
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_GNSS_SEARCH_STARTED_FAILED);
		} else {
			LOG_INF("GNSS started successfully");
			data_gnss->gnss_start_status = true;
			gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_SEARCHING);

			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_GNSS_SEARCH_STARTED);

			/* Also dispatch START event for user */
			gnss_evt.type = HL78XX_GNSS_EVENT_START;
			gnss_evt.content.status = true;
			event_dispatcher_dispatch(&gnss_evt);
		}
		break;

	case HL78XX_GNSSEV_STOP: /* GNSS stop event */
		LOG_INF("GNSS stopped (event_value=%d)", event_value);
		if (event_value == 1) {
			data_gnss->gnss_start_status = false;
			gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_IDLE);

			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_GNSS_STOPPED);

			gnss_evt.type = HL78XX_GNSS_EVENT_STOP;
			gnss_evt.content.event_status = (enum hl78xx_event_status)event_value;
		}
		event_dispatcher_dispatch(&gnss_evt);
		break;

	case HL78XX_GNSSEV_POSITION: /* GNSS fix status change */
		LOG_DBG("GNSS fix status changed: (value=%d)", event_value);
		data_gnss->position_status = (enum gnss_position_events)event_value;
		gnss_evt.type = HL78XX_GNSS_EVENT_POSITION;
		gnss_evt.content.position_event = (enum gnss_position_events)event_value;
		event_dispatcher_dispatch(&gnss_evt);
		break;

	default:
		LOG_WRN("Unknown GNSSEV type: %d", event_type);
		break;
	}
}

/* -------------------------------------------------------------------------
 * AT command helpers for KGPS* commands
 * -------------------------------------------------------------------------
 */
#ifdef CONFIG_PM_DEVICE
/* Forward declarations for PM-only functions */
static int hl78xx_gnss_start(const struct device *dev, int gnss_start_mode);
static int hl78xx_gnss_stop(const struct device *dev);
#endif

static uint16_t hl78xx_generate_nmea_mask(void)
{
	uint16_t mask = 0;

	/** Bit 0 — GGA */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GGA)) {
		mask |= (1 << 0);
	}

	/** Bit 1 — GSA */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GSA)) {
		mask |= (1 << 1);
	}

	/** Bit 2 — GSV */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GSV)) {
		mask |= (1 << 2);
	}

	/** Bit 3 — RMC */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_RMC)) {
		mask |= (1 << 3);
	}

	/** Bit 4 — VTG */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_VTG)) {
		mask |= (1 << 4);
	}

	/** Bit 5 — GNS */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GNS)) {
		mask |= (1 << 5);
	}

	/** Bit 6 — GST */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GST)) {
		mask |= (1 << 6);
	}

	/** Bit 7 — GLL */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GLL)) {
		mask |= (1 << 7);
	}

	/** Bit 8 — ZDA */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_ZDA)) {
		mask |= (1 << 8);
	}

	/** Bit 9 — PIDX */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_PIDX)) {
		mask |= (1 << 9);
	}

	/** Bit 10 — GST (duplicate) */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GST_DUP)) {
		mask |= (1 << 10);
	}

	/** Bit 11 — DTM */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_DTM)) {
		mask |= (1 << 11);
	}

	/** Bit 12 — PEPU */
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_PEPU)) {
		mask |= (1 << 12);
	}

	return mask;
}

static int hl78xx_gnss_configure_nmea_output(struct hl78xx_gnss_data *data)
{
	char cmd[64];
	int ret;

	hl78xx_gnss_lock(data->dev);

	ret = snprintk(cmd, sizeof(cmd), "AT+GNSSNMEA=%u", data->output_port);
	if (ret < 0 || ret >= sizeof(cmd)) {
		return -ENOMEM;
	}
	LOG_DBG("portcmd: %s", cmd);
	ret = modem_dynamic_cmd_send(
		data->parent_data, NULL, cmd, strlen(cmd), hl78xx_get_connect_matches(),
		hl78xx_get_connect_matches_size(), HL78XX_GNSS_SCRIPT_TIMEOUT_S, false);
	if (ret < 0) {
		goto unlock_return;
	}

	LOG_DBG("NMEA output configured: port=%u", data->output_port);
unlock_return:
	hl78xx_gnss_unlock(data->dev);
	return ret;
}

static int hl78xx_gnss_start(const struct device *dev, int gnss_start_mode)
{
	struct hl78xx_gnss_data *data = dev->data;
	char cmd[32];
	int ret;

	if (data->gnss_start_status) {
		LOG_WRN("GNSS already running");
		return 0;
	}
	hl78xx_gnss_lock(dev);

	/* Start GNSS: AT+GNSSSTART=<start_mode>
	 * Start mode is configured via Kconfig (default AUTO for normal operations)
	 */
	ret = snprintk(cmd, sizeof(cmd), "AT+GNSSSTART=%u", gnss_start_mode);
	if (ret < 0 || ret >= sizeof(cmd)) {
		return -ENOMEM;
	}

	ret = modem_dynamic_cmd_send(data->parent_data, NULL, cmd, strlen(cmd),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     HL78XX_GNSS_SCRIPT_TIMEOUT_S, false);
	if (ret < 0) {
		goto unlock_return;
	}

	LOG_INF("GNSS started (mode=%u)", gnss_start_mode);

unlock_return:
	hl78xx_gnss_unlock(dev);
	return ret;
}

static int hl78xx_gnss_stop(const struct device *dev)
{
	struct hl78xx_gnss_data *data = dev->data;
	const char *cmd = "AT+GNSSSTOP";
	int ret;

	if (!data->gnss_start_status) {
		LOG_DBG("GNSS already stopped");
		return 0;
	}
	hl78xx_gnss_lock(dev);

	ret = modem_dynamic_cmd_send(data->parent_data, NULL, cmd, strlen(cmd),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     HL78XX_GNSS_SCRIPT_TIMEOUT_S, false);
	if (ret < 0) {
		goto unlock_return;
	}

	LOG_INF("GNSS stopped");
unlock_return:
	hl78xx_gnss_unlock(dev);
	return ret;
}

static int hl78xx_gnss_configure(const struct device *dev)
{
	struct hl78xx_gnss_data *data = dev->data;
	char cmd[64];
	int ret;
	uint16_t nmea_mask;

	nmea_mask = hl78xx_generate_nmea_mask();

	/* AT+GNSSNMEA=<output>,<rate>,<profile_mask>,<nmea_mask>
	 * output: from Kconfig (default 4 = same port / URC mode)
	 * rate: current fix rate
	 * profile_mask: 0 = all profiles
	 * nmea_mask: enabled sentences
	 */
	ret = snprintk(cmd, sizeof(cmd), "AT+GNSSNMEA=0,1000,0,%x", nmea_mask);
	if (ret < 0 || ret >= sizeof(cmd)) {
		return -ENOMEM;
	}

	return modem_dynamic_cmd_send(data->parent_data, NULL, cmd, strlen(cmd),
				      hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				      HL78XX_GNSS_SCRIPT_TIMEOUT_S, false);
}

/* -------------------------------------------------------------------------
 * GNSS driver API implementation
 * -------------------------------------------------------------------------
 */
static int hl78xx_gnss_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	int ret;
	uint16_t nmea_mask;
	char cmd[64];
	struct hl78xx_gnss_data *data = dev->data;

	/* Don't allow configuration changes while GNSS search is active */
	if (data->search_state != HL78XX_GNSS_SEARCH_STATE_IDLE) {
		LOG_WRN("Cannot set fix rate while GNSS search is active (state=%d)",
			data->search_state);
		return -EBUSY;
	}

	if (fix_interval_ms < 100 || fix_interval_ms > 10000) {
		LOG_ERR("Fix rate %u ms out of range (100-10000)", fix_interval_ms);
		return -EINVAL;
	}

	hl78xx_gnss_lock(dev);

	nmea_mask = hl78xx_generate_nmea_mask();
	ret = snprintk(cmd, sizeof(cmd), "AT+GNSSNMEA=0,%u,0,%X", fix_interval_ms, nmea_mask);
	if (ret < 0) {
		goto unlock_return;
	}

	data->fix_interval_ms = fix_interval_ms;

	ret = modem_dynamic_cmd_send(data->parent_data, NULL, cmd, ret, hl78xx_get_ok_match(),
				     hl78xx_get_ok_match_size(), HL78XX_GNSS_SCRIPT_TIMEOUT_S,
				     false);
	if (ret < 0) {
		goto unlock_return;
	}

unlock_return:
	hl78xx_gnss_unlock(dev);
	return ret;
}

static int hl78xx_gnss_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	int ret;
	const char *cmd_buf = "AT+GNSSNMEA?";
	struct hl78xx_gnss_data *data = dev->data;

	if (!fix_interval_ms) {
		return -EINVAL;
	}

	hl78xx_gnss_lock(dev);

	ret = modem_dynamic_cmd_send(data->parent_data, NULL, cmd_buf, strlen(cmd_buf),
				     hl78xx_get_gnssnmea_match(), 1, HL78XX_GNSS_SCRIPT_TIMEOUT_S,
				     false);
	if (ret < 0) {
		goto unlock_return;
	}
	LOG_DBG("Current fix interval: %u ms", data->fix_interval_ms);
	*fix_interval_ms = data->fix_interval_ms;

unlock_return:
	hl78xx_gnss_unlock(dev);
	return ret;
}

static int hl78xx_gnss_set_navigation_mode(const struct device *dev, enum gnss_navigation_mode mode)
{
	struct hl78xx_gnss_data *data = dev->data;

	/* Don't allow configuration changes while GNSS search is active */
	if (data->search_state != HL78XX_GNSS_SEARCH_STATE_IDLE) {
		LOG_WRN("Cannot set navigation mode while GNSS search is active (state=%d)",
			data->search_state);
		return -EBUSY;
	}

	/* HL78xx does not support navigation mode configuration via AT commands
	 * Return success to maintain API compatibility
	 */
	LOG_DBG("Navigation mode setting not supported, ignoring mode=%d", mode);
	return 0;
}

static int hl78xx_gnss_get_navigation_mode(const struct device *dev,
					   enum gnss_navigation_mode *mode)
{
	if (!mode) {
		return -EINVAL;
	}

	/* Default to balanced dynamics */
	*mode = GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
	return 0;
}

static int hl78xx_gnss_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	struct hl78xx_gnss_data *data = dev->data;
	gnss_systems_t supported_systems;
	uint8_t encoded_systems = 0;
	char cmd_buf[64];
	int ret;

	/* Don't allow configuration changes while GNSS search is active */
	if (data->search_state != HL78XX_GNSS_SEARCH_STATE_IDLE) {
		LOG_WRN("Cannot set enabled systems while GNSS search is active (state=%d)",
			data->search_state);
		return -EBUSY;
	}

	/* HL78xx only supports GPS and GLONASS via AT commands */
	supported_systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS);

	if (!(systems & GNSS_SYSTEM_GPS)) {
		LOG_ERR("GPS must be enabled");
		return -EINVAL;
	}

	if ((~supported_systems) & systems) {
		LOG_WRN("Unsupported GNSS systems requested: 0x%08x, using GPS+GLONASS only",
			systems);
	}

	hl78xx_gnss_lock(dev);

	if ((systems & GNSS_SYSTEM_GPS) && (systems & GNSS_SYSTEM_GLONASS)) {
		encoded_systems = 1;
	} else if (systems & GNSS_SYSTEM_GPS) {
		encoded_systems = 0;
	} else {
		return -EINVAL;
	}

	ret = snprintk(cmd_buf, sizeof(cmd_buf), "AT+GNSSCONF=10,%u", encoded_systems);
	if (ret < 0) {
		goto unlock_return;
	}
	ret = modem_dynamic_cmd_send(data->parent_data, NULL, cmd_buf, ret, hl78xx_get_ok_match(),
				     hl78xx_get_ok_match_size(), HL78XX_GNSS_SCRIPT_TIMEOUT_S,
				     false);
	if (ret < 0) {
		goto unlock_return;
	}
	data->enabled_systems = systems;

unlock_return:
	hl78xx_gnss_unlock(dev);
	return ret;
}

static int hl78xx_gnss_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	int ret;
	struct hl78xx_gnss_data *data = dev->data;
	const char *cmd_buf = "AT+GNSSCONF?";

	if (!systems) {
		return -EINVAL;
	}

	hl78xx_gnss_lock(dev);

	ret = modem_dynamic_cmd_send(data->parent_data, NULL, cmd_buf, strlen(cmd_buf),
				     hl78xx_get_gnssconf_enabledsys_match(), 1,
				     HL78XX_GNSS_SCRIPT_TIMEOUT_S, false);
	if (ret < 0) {
		goto unlock_return;
	}

	WRITE_BIT(*systems, 0, 1); /* GPS is enabled default anyway */
	WRITE_BIT(*systems, 1, (data->enabled_systems == true));

unlock_return:
	hl78xx_gnss_unlock(dev);
	return ret;
}

static int hl78xx_gnss_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	if (!systems) {
		return -EINVAL;
	}

	/* HL78xx only supports GPS and GLONASS via AT commands */
	*systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS);

	return 0;
}

static DEVICE_API(gnss, hl78xx_gnss_api) = {
	.set_fix_rate = hl78xx_gnss_set_fix_rate,
	.get_fix_rate = hl78xx_gnss_get_fix_rate,
	.set_navigation_mode = hl78xx_gnss_set_navigation_mode,
	.get_navigation_mode = hl78xx_gnss_get_navigation_mode,
	.set_enabled_systems = hl78xx_gnss_set_enabled_systems,
	.get_enabled_systems = hl78xx_gnss_get_enabled_systems,
	.get_supported_systems = hl78xx_gnss_get_supported_systems,
};

/* -------------------------------------------------------------------------
 * Power management
 * -------------------------------------------------------------------------
 */

static int hl78xx_gnss_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = -ENOTSUP;

	hl78xx_gnss_lock(dev);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		LOG_DBG("GNSS suspending");
		break;

	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("GNSS resuming");
		ret = 0;
		break;

	case PM_DEVICE_ACTION_TURN_ON:
		LOG_DBG("GNSS powered on");
		ret = 0;
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		LOG_DBG("GNSS powered off");
		ret = hl78xx_gnss_stop(dev);
		break;

	default:
		break;
	}

	hl78xx_gnss_pm_changed(dev);
	hl78xx_gnss_unlock(dev);

	return ret;
}

/* -------------------------------------------------------------------------
 * Initialization
 * -------------------------------------------------------------------------
 */
static int hl78xx_gnss_init_nmea0183_match(const struct device *dev)
{
	struct hl78xx_gnss_data *data = dev->data;

	const struct gnss_nmea0183_match_config nmea_config = {
		.gnss = dev,
#if defined(CONFIG_GNSS_SATELLITES) && defined(CONFIG_HL78XX_GNSS_SOURCE_NMEA)
		.satellites = data->satellites,
		.satellites_size = ARRAY_SIZE(data->satellites),
#endif
	};

	return gnss_nmea0183_match_init(&data->match_data, &nmea_config);
}

static int hl78xx_gnss_init(const struct device *dev)
{
	const struct hl78xx_gnss_config *config = dev->config;
	struct hl78xx_gnss_data *data = dev->data;
	int ret;

	LOG_INF("Initializing HL78xx GNSS driver");

	/* Initialize semaphore */
	k_sem_init(&data->lock, 1, 1);

	/* Get parent modem data */
	if (!config->parent_modem) {
		LOG_ERR("Parent modem device not configured");
		return -EINVAL;
	}
	data->dev = dev;
	data->parent_data = (struct hl78xx_data *)config->parent_modem->data;
	if (!data->parent_data) {
		LOG_ERR("Parent modem data not available");
		return -EINVAL;
	}

	/* Store reference to GNSS device in parent modem */
	data->parent_data->gnss_dev = dev;

	/* Initialize NMEA0183 match subsystem */
	ret = hl78xx_gnss_init_nmea0183_match(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize NMEA0183 match: %d", ret);
		return ret;
	}

	/* Initialize state machine */
	data->search_state = HL78XX_GNSS_SEARCH_STATE_IDLE;
	data->fix_rate_ms = config->fix_rate_default;
	data->enabled_systems = 0;
	data->output_port = NMEA_OUTPUT_NONE;
	data->search_timeout_ms = 0;

	hl78xx_gnss_pm_changed(dev);

	LOG_INF("HL78xx GNSS driver initialized successfully");
	return pm_device_driver_init(dev, hl78xx_gnss_pm_action);
}

int hl78xx_on_run_gnss_init_script_state_enter(struct hl78xx_data *data)
{
	if (data->status.phone_functionality.functionality == HL78XX_AIRPLANE) {
		/* Already in airplane mode */
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_SUCCESS);
		return 0;
	}

	HL78XX_LOG_DBG("Setting airplane mode (CFUN=4)...");
	hl78xx_api_func_set_phone_functionality(data->dev, HL78XX_AIRPLANE, false);

	return hl78xx_run_gnss_init_chat_script_async(data);
}

void hl78xx_run_gnss_init_script_event_handler(struct hl78xx_data *data, enum hl78xx_event event)
{
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	struct hl78xx_gnss_data *data_gnss = data_nmea->gnss->data;
	struct hl78xx_evt gnss_evt;

	switch (event) {
	case MODEM_HL78XX_EVENT_RESUME:
		LOG_DBG("GNSS init: RESUME event");
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		LOG_DBG("GNSS init: SUSPEND event");
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		LOG_DBG("GNSS init: SCRIPT_SUCCESS - configuring GNSS");
		hl78xx_gnss_configure(data->gnss_dev);
		gnss_evt.content.status = true;
		gnss_evt.type = HL78XX_GNSS_ENGINE_READY;
		event_dispatcher_dispatch(&gnss_evt);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		LOG_WRN("GNSS init: SCRIPT_FAILURE event");
		gnss_evt.content.status = false;
		gnss_evt.type = HL78XX_GNSS_ENGINE_READY;
		event_dispatcher_dispatch(&gnss_evt);
		break;

	case MODEM_HL78XX_EVENT_PHONE_FUNCTIONALITY_CHANGED:
		LOG_DBG("GNSS init: PHONE_FUNCTIONALITY_CHANGED (cfun=%d)",
			data->status.phone_functionality.functionality);
		if (data->status.phone_functionality.functionality == HL78XX_FULLY_FUNCTIONAL) {
			/* Exiting GNSS mode - return to LTE registration flow */
			LOG_INF("Full functionality restored, returning to LTE mode");
			gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_IDLE);
			data_gnss->exit_to_lte_pending = false;
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT);
		}
		break;
	case MODEM_HL78XX_EVENT_GNSS_SEARCH_STARTED_FAILED:
		LOG_DBG("GNSS init: GNSS_SEARCH_STARTED_FAILED");
		gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_BLOCKED_BY_LTE);
		break;
	case MODEM_HL78XX_EVENT_GNSS_START_REQUESTED:
		LOG_DBG("GNSS init: START_REQUESTED (cfun=%d, state=%s)",
			data->status.phone_functionality.functionality,
			gnss_search_state_str(data_gnss->search_state));
		/*
		 * If already in airplane mode and search was just queued, start immediately.
		 * Otherwise, the search will start when PHONE_FUNCTIONALITY_CHANGED fires.
		 */
		hl78xx_start_timer(data_gnss->parent_data, K_MSEC(3000));

		break;
	case MODEM_HL78XX_EVENT_GNSS_SEARCH_STARTED:
		LOG_DBG("GNSS init: GNSS_SEARCH_STARTED");
		if (data_gnss->output_port != NMEA_OUTPUT_NONE) {
			hl78xx_gnss_configure_nmea_output(data_gnss);
		}
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_GNSS_SEARCH_STARTED);
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:

		if (data->status.phone_functionality.functionality == HL78XX_AIRPLANE) {
			if (data_gnss->search_state ==
			    HL78XX_GNSS_SEARCH_STATE_WAITING_FOR_AIRPLANE) {
				LOG_INF("Already in airplane mode, starting GNSS immediately");
				gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_STARTING);
				hl78xx_gnss_start(data->gnss_dev, HL78XX_GNSS_START_MODE);
				if (data_gnss->search_timeout_ms != 0) {
					hl78xx_start_timer(data_gnss->parent_data,
							   K_MSEC(data_gnss->search_timeout_ms));
				}
			} else {
				LOG_DBG("GNSS search already started or not queued");
			}
		} else {
			LOG_INF("GNSS search queued, waiting for airplane mode (CFUN=4)");
		}
		break;

	default:
		LOG_DBG("GNSS init: unhandled event %d", event);
		break;
	}
}

int hl78xx_on_run_gnss_init_script_state_leave(struct hl78xx_data *data)
{
	return 0;
}

int hl78xx_on_gnss_search_started_state_enter(struct hl78xx_data *data)
{

	/* If phone is in AIRPLANE mode and GNSS search is queued but not started,
	 * start GNSS now
	 */

	return 0;
}

int hl78xx_on_gnss_search_started_state_leave(struct hl78xx_data *data)
{
	return 0;
}

void hl78xx_gnss_search_started_event_handler(struct hl78xx_data *data, enum hl78xx_event event)
{
	struct gnss_nmea0183_match_data *data_nmea = data->gnss_dev->data;
	struct hl78xx_gnss_data *data_gnss = data_nmea->gnss->data;
	struct hl78xx_evt gnss_evt;
	int ret;

	switch (event) {
	case MODEM_HL78XX_EVENT_GNSS_FIX_ACQUIRED:
		LOG_INF("GNSS fix acquired");
		break;

	case MODEM_HL78XX_EVENT_GNSS_FIX_LOST:
		LOG_INF("GNSS fix lost");
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		LOG_DBG("GNSS search: SCRIPT_SUCCESS (state=%s)",
			gnss_search_state_str(data_gnss->search_state));
		/* Script completed - this could be GNSSLOC query or other command */
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		LOG_WRN("GNSS search: timeout expired - stopping search");

		/* Notify user about timeout */
		gnss_evt.type = HL78XX_GNSS_EVENT_SEARCH_TIMEOUT;
		gnss_evt.content.status = false;
		event_dispatcher_dispatch(&gnss_evt);
		break;

	case MODEM_HL78XX_EVENT_GNSS_STOP_REQUESTED:
		LOG_INF("GNSS search: stop requested");
		gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_STOPPING);
		hl78xx_gnss_clear_search_queue(data_gnss);

		/* Abort any running script before starting stop script */
		modem_chat_script_abort(&data->chat);
		/* Handle NMEA output termination if active */
		if (data_gnss->output_port != NMEA_OUTPUT_NONE) {
			modem_chat_release(&data->chat);
			modem_pipe_attach(data->uart_pipe, hl78xx_gnss_bus_pipe_handler, data);
			LOG_DBG("Sending termination pattern to end NMEA output");
			ret = modem_pipe_transmit(data->uart_pipe,
						  data->buffers.termination_pattern,
						  data->buffers.termination_pattern_size);
			if (ret < 0) {
				LOG_ERR("Failed to send termination pattern: %d", ret);
			}
			modem_chat_attach(&data->chat, data->uart_pipe);
			/* Script will handle the rest */
			hl78xx_run_gnss_terminate_nmea_chat_script(data);
		}
		ret = hl78xx_run_gnss_stop_search_chat_script(data);
		if (ret < 0) {
			LOG_ERR("Failed to run GNSS stop script: %d", ret);
			/* Force state to idle on error */
			gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_IDLE);
		}
		break;

	case MODEM_HL78XX_EVENT_GNSS_STOPPED:
		LOG_INF("GNSS search: stopped");
		gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_IDLE);
		/* Check if GNSS mode exit was requested */
		if (data_gnss->exit_to_lte_pending) {
			data_gnss->exit_to_lte_pending = false;
			LOG_INF("GNSS stopped, mode exit complete. User can now set phone "
				"functionality.");
			/*
			 * Notify user that GNSS mode exit is complete.
			 * Modem is now in airplane mode - user decides what to do next:
			 * - Call hl78xx_api_func_set_phone_functionality(dev,
			 * HL78XX_FULLY_FUNCTIONAL, false) to return to LTE
			 * - Stay in airplane mode for low power operation
			 */

			gnss_evt.type = HL78XX_GNSS_EVENT_MODE_EXITED;
			gnss_evt.content.status = true;
			event_dispatcher_dispatch(&gnss_evt);
		}
		break;

	case MODEM_HL78XX_EVENT_GNSS_MODE_EXIT_REQUESTED:
		LOG_INF("GNSS mode exit requested");
		if (data_gnss->search_state == HL78XX_GNSS_SEARCH_STATE_SEARCHING ||
		    data_gnss->search_state == HL78XX_GNSS_SEARCH_STATE_STARTING) {
			/* Stop search first, then notify user */
			data_gnss->exit_to_lte_pending = true;
			data_gnss->gnss_mode_enter_pending = false;
			LOG_INF("Stopping GNSS search before exiting mode...");
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_GNSS_STOP_REQUESTED);
		} else {
			/* No search in progress, exit immediately */
			LOG_INF("Exiting GNSS mode (no active search). User can now set phone "
				"functionality.");
			/*
			 * Notify user that GNSS mode exit is complete.
			 * Modem is in airplane mode - user decides what to do next.
			 */
			gnss_evt.type = HL78XX_GNSS_EVENT_MODE_EXITED;
			gnss_evt.content.status = true;
			event_dispatcher_dispatch(&gnss_evt);
		}
		break;

	case MODEM_HL78XX_EVENT_PHONE_FUNCTIONALITY_CHANGED:
		LOG_DBG("GNSS search: PHONE_FUNCTIONALITY_CHANGED (cfun=%d)",
			data->status.phone_functionality.functionality);
		/*
		 * If modem exits airplane mode while GNSS is searching,
		 * GNSS will be automatically stopped by the modem (+GNSSEV: 2,1).
		 * Transition back to LTE mode.
		 */
		if (data->status.phone_functionality.functionality == HL78XX_FULLY_FUNCTIONAL) {
			LOG_INF("Full functionality restored, returning to LTE mode");
			gnss_set_search_state(data_gnss, HL78XX_GNSS_SEARCH_STATE_IDLE);
			data_gnss->exit_to_lte_pending = false;
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT);
		}
		break;

	case MODEM_HL78XX_EVENT_GNSS_START_REQUESTED:
		/* Ignore if already searching - prevents duplicate starts */
		LOG_DBG("GNSS search: ignoring START_REQUESTED (already searching)");
		break;
	case MODEM_HL78XX_EVENT_MDM_RESTART:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
	default:
		LOG_DBG("GNSS search: unhandled event %d", event);
		break;
	}
}
/* -------------------------------------------------------------------------
 * Device instantiation
 * -------------------------------------------------------------------------
 */
#define HL78XX_GNSS_DEVICE_DEFINE(inst)                                                            \
	static const struct hl78xx_gnss_config hl78xx_gnss_config_##inst = {                       \
		.parent_modem = DEVICE_DT_GET(DT_INST_PARENT(inst)),                               \
		.fix_rate_default = DT_INST_PROP_OR(inst, fix_rate, 1000),                         \
	};                                                                                         \
                                                                                                   \
	static struct hl78xx_gnss_data hl78xx_gnss_data_##inst;                                    \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, hl78xx_gnss_pm_action);                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, hl78xx_gnss_init, PM_DEVICE_DT_INST_GET(inst),                 \
			      &hl78xx_gnss_data_##inst, &hl78xx_gnss_config_##inst, POST_KERNEL,   \
			      CONFIG_GNSS_INIT_PRIORITY, &hl78xx_gnss_api);

#define DT_DRV_COMPAT swir_hl7812_gnss
DT_INST_FOREACH_STATUS_OKAY(HL78XX_GNSS_DEVICE_DEFINE)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800_gnss
DT_INST_FOREACH_STATUS_OKAY(HL78XX_GNSS_DEVICE_DEFINE)
#undef DT_DRV_COMPAT
