/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hl78xx_gnss_parsers.c
 *
 * @brief Supplementary NMEA0183 parsers for HL78xx GNSS driver
 *
 * This file contains parsers for supplementary NMEA0183 sentences that are not
 * part of the standard Zephyr GNSS subsystem. These include:
 * - GSA: GNSS DOP and Active Satellites
 * - GST: GNSS Pseudorange Error Statistics
 * - EPU: Sierra proprietary EPU (Estimated Position Error)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hl78xx_gnss_parsers.h"
#include "hl78xx_gnss.h"
#include "../../gnss/gnss_parse.h"

LOG_MODULE_DECLARE(hl78xx_gnss);

static K_SEM_DEFINE(semlock, 1, 1);

/* -------------------------------------------------------------------------
 * GNSSLOC Parser Helper Functions
 * -------------------------------------------------------------------------
 */

int gnssloc_dms_to_ndeg(const char *str, int64_t *ndeg)
{
	int64_t degrees = 0;
	int64_t minutes = 0;
	int64_t seconds_nano = 0;
	int64_t result;
	char direction = '\0';
	char *ptr = (char *)str;
	char *end;
	char sec_buf[16];
	size_t sec_len;

	if (str == NULL || ndeg == NULL) {
		return -EINVAL;
	}

	/* Skip leading spaces */
	while (*ptr == ' ') {
		ptr++;
	}

	/* Parse degrees */
	degrees = strtol(ptr, &end, 10);
	if (ptr == end || degrees < 0 || degrees > 180) {
		return -EINVAL;
	}
	ptr = end;

	/* Skip to "Deg" and past it */
	while (*ptr && strncmp(ptr, "Deg", 3) != 0) {
		ptr++;
	}
	if (*ptr) {
		ptr += 3; /* Skip "Deg" */
	} else {
		return -EINVAL;
	}

	/* Skip spaces */
	while (*ptr == ' ') {
		ptr++;
	}

	/* Parse minutes */
	minutes = strtol(ptr, &end, 10);
	if (ptr == end || minutes < 0 || minutes > 59) {
		return -EINVAL;
	}
	ptr = end;

	/* Skip to "Min" and past it */
	while (*ptr && strncmp(ptr, "Min", 3) != 0) {
		ptr++;
	}
	if (*ptr) {
		ptr += 3; /* Skip "Min" */
	} else {
		return -EINVAL;
	}

	/* Skip spaces */
	while (*ptr == ' ') {
		ptr++;
	}

	/* Extract seconds value into a clean buffer for gnss_parse_dec_to_nano()
	 * The string looks like "13.61 Sec N" - we need just "13.61"
	 */
	sec_len = 0;
	while (ptr[sec_len] && (ptr[sec_len] == '.' ||
	       (ptr[sec_len] >= '0' && ptr[sec_len] <= '9') ||
	       ptr[sec_len] == '-')) {
		if (sec_len >= sizeof(sec_buf) - 1) {
			return -EINVAL;
		}
		sec_buf[sec_len] = ptr[sec_len];
		sec_len++;
	}
	sec_buf[sec_len] = '\0';

	if (sec_len == 0) {
		return -EINVAL;
	}

	/* Parse seconds (can be decimal like "14.43") as nano-seconds */
	if (gnss_parse_dec_to_nano(sec_buf, &seconds_nano) < 0) {
		return -EINVAL;
	}
	if (seconds_nano < 0 || seconds_nano > 60000000000LL) {
		return -EINVAL;
	}

	/* Advance past the number */
	ptr += sec_len;

	/* Skip to "Sec" and past it */
	while (*ptr && strncmp(ptr, "Sec", 3) != 0) {
		ptr++;
	}
	if (*ptr) {
		ptr += 3; /* Skip "Sec" */
	} else {
		return -EINVAL;
	}

	/* Skip spaces and find direction (N/S/E/W) */
	while (*ptr) {
		if (*ptr == 'N' || *ptr == 'S' || *ptr == 'E' || *ptr == 'W') {
			direction = *ptr;
			break;
		}
		ptr++;
	}

	if (direction == '\0') {
		return -EINVAL;
	}

	/* Convert to nanodegrees:
	 * 1 degree = 1,000,000,000 nanodegrees
	 * 1 minute = 1/60 degree
	 * 1 second = 1/3600 degree
	 */
	result = degrees * 1000000000LL;                  /* Degrees to nanodegrees */
	result += (minutes * 1000000000LL) / 60LL;        /* Minutes to nanodegrees */
	result += seconds_nano / 3600LL;                  /* Seconds (in nano) to nanodegrees */

	/* Apply sign based on direction */
	if (direction == 'S' || direction == 'W') {
		result = -result;
	}

	*ndeg = result;
	return 0;
}

int gnssloc_parse_value_with_unit(const char *str, int64_t *milli)
{
	int ret;
	char value_buf[32];
	char *ptr;
	size_t len;

	if (str == NULL || milli == NULL) {
		return -EINVAL;
	}

	/* Copy the string and find where the unit starts */
	len = strlen(str);
	if (len >= sizeof(value_buf)) {
		len = sizeof(value_buf) - 1;
	}
	memcpy(value_buf, str, len);
	value_buf[len] = '\0';

	/* Find and truncate at unit (space before 'm', 'k', etc.) */
	ptr = value_buf;
	while (*ptr) {
		if (*ptr == ' ' && (*(ptr + 1) == 'm' || *(ptr + 1) == 'k')) {
			*ptr = '\0';
			break;
		}
		ptr++;
	}

	/* Parse the numeric part as milli using Zephyr's gnss_parse_dec_to_milli */
	ret = gnss_parse_dec_to_milli(value_buf, milli);
	return ret;
}

int gnssloc_parse_speed_to_mms(const char *str, uint32_t *mms)
{
	int64_t milli;
	int ret;

	if (str == NULL || mms == NULL) {
		return -EINVAL;
	}

	ret = gnssloc_parse_value_with_unit(str, &milli);
	if (ret < 0) {
		return ret;
	}

	if (milli < 0 || milli > UINT32_MAX) {
		return -ERANGE;
	}

	*mms = (uint32_t)milli;
	return 0;
}

int gnssloc_parse_gpstime(const char *str, struct gnss_time *utc)
{
	int year, month, day, hour, minute, second;

	if (str == NULL || utc == NULL) {
		return -EINVAL;
	}

	/* Parse format: "2026 1 25 23:15:56" */
	if (sscanf(str, "%d %d %d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
		return -EINVAL;
	}

	/* Validate ranges */
	if (year < 2000 || year > 2099 ||
	    month < 1 || month > 12 ||
	    day < 1 || day > 31 ||
	    hour < 0 || hour > 23 ||
	    minute < 0 || minute > 59 ||
	    second < 0 || second > 59) {
		return -ERANGE;
	}

	utc->century_year = (uint8_t)(year % 100);
	utc->month = (uint8_t)month;
	utc->month_day = (uint8_t)day;
	utc->hour = (uint8_t)hour;
	utc->minute = (uint8_t)minute;
	utc->millisecond = (uint16_t)(second * 1000);

	return 0;
}

/* -------------------------------------------------------------------------
 * NMEA Sentence Parser Callbacks
 * -------------------------------------------------------------------------
 */

#ifdef CONFIG_HL78XX_GNSS_AUX_DATA_PARSER
static int gnss_nmea0183_match_parse_utc(char **argv, uint16_t argc, uint32_t *utc)
{
	int64_t i64;

	if ((gnss_parse_dec_to_milli(argv[1], &i64) < 0) || (i64 < 0) || (i64 > UINT32_MAX)) {
		return -EINVAL;
	}

	*utc = (uint32_t)i64;
	return 0;
}

void gnss_nmea0183_match_gsa_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;

	/* GSA message should have at least 17 fields:
	 * 0: $xxGSA, 1: mode, 2: fix_type, 3-14: sat_ids, 15: pdop, 16: hdop, 17: vdop
	 */
	if (argc < 18) {
		LOG_DBG("GSA: insufficient fields (argc=%u)", argc);
		return;
	}

	/* Parse fix type (1=no fix, 2=2D, 3=3D) */
	if (gnss_parse_atoi(argv[2], 10, &data->aux_data.gsa.fix_type) < 0) {
		LOG_WRN("GSA: failed to parse fix_type");
		return;
	}

	/* Parse PDOP (Position Dilution of Precision) */
	if (gnss_parse_dec_to_milli(argv[15], &data->aux_data.gsa.pdop) < 0) {
		LOG_DBG("GSA: failed to parse PDOP");
		data->aux_data.gsa.pdop = 0;
	}

	/* Parse HDOP (Horizontal Dilution of Precision) */
	if (gnss_parse_dec_to_milli(argv[16], &data->aux_data.gsa.hdop) < 0) {
		LOG_DBG("GSA: failed to parse HDOP");
		data->aux_data.gsa.hdop = 0;
	}

	/* Parse VDOP (Vertical Dilution of Precision) */
	if (gnss_parse_dec_to_milli(argv[17], &data->aux_data.gsa.vdop) < 0) {
		LOG_DBG("GSA: failed to parse VDOP");
		data->aux_data.gsa.vdop = 0;
	}

	/* Note: DOP values could be stored in user_data if extended
	 * gnss_data structure is needed
	 */
}

void gnss_nmea0183_match_gst_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;

	/* GST message should have at least 9 fields:
	 * 0: $xxGST, 1: time, 2: rms, 3: smajor, 4: sminor,
	 * 5: orient, 6: lat_err, 7: lon_err, 8: alt_err
	 */
	if (argc < 9) {
		LOG_DBG("GST: insufficient fields (argc=%u)", argc);
		return;
	}
	if (gnss_nmea0183_match_parse_utc(argv, argc, &data->aux_data.gst.gst_utc) < 0) {
		return;
	}
	/* Parse RMS error (meters) */
	if (gnss_parse_dec_to_milli(argv[2], &data->aux_data.gst.rms) < 0) {
		LOG_DBG("GST: failed to parse RMS");
		data->aux_data.gst.rms = 0;
	}

	/* Parse latitude error (meters) */
	if (gnss_parse_dec_to_milli(argv[6], &data->aux_data.gst.lat_err) < 0) {
		LOG_DBG("GST: failed to parse lat_err");
		data->aux_data.gst.lat_err = 0;
	}

	/* Parse longitude error (meters) */
	if (gnss_parse_dec_to_milli(argv[7], &data->aux_data.gst.lon_err) < 0) {
		LOG_DBG("GST: failed to parse lon_err");
		data->aux_data.gst.lon_err = 0;
	}

	/* Parse altitude error (meters) */
	if (gnss_parse_dec_to_milli(argv[8], &data->aux_data.gst.alt_err) < 0) {
		LOG_DBG("GST: failed to parse alt_err");
		data->aux_data.gst.alt_err = 0;
	}
}
void gnss_nmea0183_match_epu_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;

	/* PSEPU message format: $PSEPU,x.x,x.x,x.x,x.x,x.x,x.xx,x.xx,x.x,x.xx,x.xx,x.xx*hh
	 * 0: $PSEPU
	 * 1: position 3D uncertainty (0.0 to 999.9)
	 * 2: position 2D uncertainty (0.0 to 999.9)
	 * 3: position Latitude uncertainty (0.0 to 999.9)
	 * 4: position Longitude uncertainty (0.0 to 999.9)
	 * 5: position Altitude uncertainty (0.0 to 999.9)
	 * 6: velocity 3D uncertainty (0.00 to 500.00)
	 * 7: velocity 2D uncertainty (0.00 to 500.00)
	 * 8: velocity Heading uncertainty (0.0 to 180.0)
	 * 9: velocity East uncertainty (0.00 to 500.00)
	 * 10: velocity North uncertainty (0.00 to 500.00)
	 * 11: velocity Up uncertainty (0.00 to 500.00)
	 * All units in meters (except heading in degrees)
	 */
	if (argc < 12) {
		LOG_DBG("PSEPU: insufficient fields (argc=%u)", argc);
		return;
	}

	/* Parse position uncertainties */
	if (gnss_parse_dec_to_milli(argv[1], &data->aux_data.epu.pos_3d) < 0) {
		LOG_DBG("PSEPU: failed to parse pos_3d");
		data->aux_data.epu.pos_3d = 0;
	}
	if (gnss_parse_dec_to_milli(argv[2], &data->aux_data.epu.pos_2d) < 0) {
		LOG_DBG("PSEPU: failed to parse pos_2d");
		data->aux_data.epu.pos_2d = 0;
	}
	if (gnss_parse_dec_to_milli(argv[3], &data->aux_data.epu.pos_lat) < 0) {
		LOG_DBG("PSEPU: failed to parse pos_lat");
		data->aux_data.epu.pos_lat = 0;
	}
	if (gnss_parse_dec_to_milli(argv[4], &data->aux_data.epu.pos_lon) < 0) {
		LOG_DBG("PSEPU: failed to parse pos_lon");
		data->aux_data.epu.pos_lon = 0;
	}
	if (gnss_parse_dec_to_milli(argv[5], &data->aux_data.epu.pos_alt) < 0) {
		LOG_DBG("PSEPU: failed to parse pos_alt");
		data->aux_data.epu.pos_alt = 0;
	}

	/* Parse velocity uncertainties */
	if (gnss_parse_dec_to_milli(argv[6], &data->aux_data.epu.vel_3d) < 0) {
		LOG_DBG("PSEPU: failed to parse vel_3d");
		data->aux_data.epu.vel_3d = 0;
	}
	if (gnss_parse_dec_to_milli(argv[7], &data->aux_data.epu.vel_2d) < 0) {
		LOG_DBG("PSEPU: failed to parse vel_2d");
		data->aux_data.epu.vel_2d = 0;
	}
	if (gnss_parse_dec_to_milli(argv[8], &data->aux_data.epu.vel_hdg) < 0) {
		LOG_DBG("PSEPU: failed to parse vel_hdg");
		data->aux_data.epu.vel_hdg = 0;
	}
	if (gnss_parse_dec_to_milli(argv[9], &data->aux_data.epu.vel_east) < 0) {
		LOG_DBG("PSEPU: failed to parse vel_east");
		data->aux_data.epu.vel_east = 0;
	}
	if (gnss_parse_dec_to_milli(argv[10], &data->aux_data.epu.vel_north) < 0) {
		LOG_DBG("PSEPU: failed to parse vel_north");
		data->aux_data.epu.vel_north = 0;
	}
	if (gnss_parse_dec_to_milli(argv[11], &data->aux_data.epu.vel_up) < 0) {
		LOG_DBG("PSEPU: failed to parse vel_up");
		data->aux_data.epu.vel_up = 0;
	}

	gnss_publish_aux_data((const struct device *)data->dev, &data->aux_data,
			      sizeof(data->aux_data));
}

void gnss_publish_aux_data(const struct device *dev,
			   const struct hl78xx_gnss_nmea_aux_data *aux_data, uint16_t size)
{
	(void)k_sem_take(&semlock, K_FOREVER);

	STRUCT_SECTION_FOREACH(hl78xx_gnss_aux_data_callback, callback) {
		if (callback->dev == NULL || callback->dev == dev) {
			callback->callback(dev, aux_data, size);
		}
	}

	k_sem_give(&semlock);
}
#endif /* CONFIG_HL78XX_GNSS_AUX_DATA_PARSER */
