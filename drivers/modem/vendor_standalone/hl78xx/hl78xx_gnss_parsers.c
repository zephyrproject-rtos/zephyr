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
#include "hl78xx_gnssloc.h"
#include "../../../gnss/gnss_parse.h"

LOG_MODULE_DECLARE(hl78xx_gnss);

enum hl78xx_gnss_aux_sentence {
	HL78XX_GNSS_AUX_SENTENCE_GSA,
	HL78XX_GNSS_AUX_SENTENCE_GST,
	HL78XX_GNSS_AUX_SENTENCE_EPU,
};

static K_SEM_DEFINE(semlock, 1, 1);

/* -------------------------------------------------------------------------
 * NMEA Sentence Parser Callbacks
 * -------------------------------------------------------------------------
 */

#ifdef CONFIG_HL78XX_GNSS_AUX_DATA_PARSER

static bool hl78xx_gnss_aux_should_publish(enum hl78xx_gnss_aux_sentence sentence)
{
	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_PEPU)) {
		return sentence == HL78XX_GNSS_AUX_SENTENCE_EPU;
	}

	if (IS_ENABLED(CONFIG_HL78XX_GNSS_NMEA_GST)) {
		return sentence == HL78XX_GNSS_AUX_SENTENCE_GST;
	}

	return sentence == HL78XX_GNSS_AUX_SENTENCE_GSA;
}

static int gnss_nmea0183_match_parse_utc(char **argv, uint16_t argc, uint32_t *utc)
{
	int64_t i64;

	if (argv == NULL || utc == NULL || argc < 2 || argv[1] == NULL) {
		return -EINVAL;
	}

	if ((gnss_parse_dec_to_milli(argv[1], &i64) < 0) || (i64 < 0) || (i64 > UINT32_MAX)) {
		return -EINVAL;
	}

	*utc = (uint32_t)i64;
	return 0;
}

static void hl78xx_gnss_parse_optional_milli(const char *str, int64_t *value)
{
	if (str == NULL || value == NULL || str[0] == '\0') {
		if (value != NULL) {
			*value = 0;
		}

		return;
	}

	if (gnss_parse_dec_to_milli(str, value) < 0) {
		*value = 0;
	}
}

int hl78xx_gnss_parse_gsa(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data)
{
	/* GSA message should have at least 17 fields:
	 * 0: $xxGSA, 1: mode, 2: fix_type, 3-14: sat_ids, 15: pdop, 16: hdop, 17: vdop
	 */
	if (argv == NULL || aux_data == NULL || argc < 18) {
		return -EINVAL;
	}

	/* Parse fix type (1=no fix, 2=2D, 3=3D) */
	if (gnss_parse_atoi(argv[2], 10, &aux_data->gsa.fix_type) < 0) {
		return -EINVAL;
	}

	/* Parse PDOP (Position Dilution of Precision) */
	if (gnss_parse_dec_to_milli(argv[15], &aux_data->gsa.pdop) < 0) {
		aux_data->gsa.pdop = 0;
	}

	/* Parse HDOP (Horizontal Dilution of Precision) */
	if (gnss_parse_dec_to_milli(argv[16], &aux_data->gsa.hdop) < 0) {
		aux_data->gsa.hdop = 0;
	}

	/* Parse VDOP (Vertical Dilution of Precision) */
	if (gnss_parse_dec_to_milli(argv[17], &aux_data->gsa.vdop) < 0) {
		aux_data->gsa.vdop = 0;
	}

	return 0;
}

void gnss_nmea0183_match_gsa_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;
	int ret;

	ARG_UNUSED(chat);

	if (data == NULL) {
		return;
	}
	/* Parse fix type (1=no fix, 2=2D, 3=3D) */
	ret = hl78xx_gnss_parse_gsa(argv, argc, &data->aux_data);
	if (ret < 0) {
		LOG_WRN("GSA: failed to parse GSA data");
		return;
	}

	/* Note: DOP values could be stored in user_data if extended
	 * gnss_data structure is needed
	 */
	if (hl78xx_gnss_aux_should_publish(HL78XX_GNSS_AUX_SENTENCE_GSA)) {
		gnss_publish_aux_data((const struct device *)data->dev, &data->aux_data,
				      sizeof(data->aux_data));
	}
}

int hl78xx_gnss_parse_gst(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data)
{
	/* GST message should have at least 9 fields:
	 * 0: $xxGST, 1: time, 2: rms, 3: smajor, 4: sminor,
	 * 5: orient, 6: lat_err, 7: lon_err, 8: alt_err
	 */
	if (argv == NULL || aux_data == NULL || argc < 9) {
		return -EINVAL;
	}

	if (gnss_nmea0183_match_parse_utc(argv, argc, &aux_data->gst.gst_utc) < 0) {
		return -EINVAL;
	}
	/* Parse RMS error (meters) */
	hl78xx_gnss_parse_optional_milli(argv[2], &aux_data->gst.rms);
	/* Parse latitude error (meters) */
	hl78xx_gnss_parse_optional_milli(argv[6], &aux_data->gst.lat_err);
	/* Parse longitude error (meters) */
	hl78xx_gnss_parse_optional_milli(argv[7], &aux_data->gst.lon_err);
	/* Parse altitude error (meters) */
	hl78xx_gnss_parse_optional_milli(argv[8], &aux_data->gst.alt_err);

	return 0;
}

void gnss_nmea0183_match_gst_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;
	int ret;

	ARG_UNUSED(chat);

	if (data == NULL) {
		return;
	}

	ret = hl78xx_gnss_parse_gst(argv, argc, &data->aux_data);
	if (ret < 0) {
		return;
	}

	if (hl78xx_gnss_aux_should_publish(HL78XX_GNSS_AUX_SENTENCE_GST)) {
		gnss_publish_aux_data((const struct device *)data->dev, &data->aux_data,
				      sizeof(data->aux_data));
	}
}

int hl78xx_gnss_parse_epu(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data)
{
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

	if (argv == NULL || aux_data == NULL || argc < 12) {
		return -EINVAL;
	}

	/*
	 * Empty EPU fields are accepted and converted to zero. A PSEPU
	 * sentence with empty fields is still useful as an end-of-frame marker.
	 */

	hl78xx_gnss_parse_optional_milli(argv[1], &aux_data->epu.pos_3d);
	hl78xx_gnss_parse_optional_milli(argv[2], &aux_data->epu.pos_2d);
	hl78xx_gnss_parse_optional_milli(argv[3], &aux_data->epu.pos_lat);
	hl78xx_gnss_parse_optional_milli(argv[4], &aux_data->epu.pos_lon);
	hl78xx_gnss_parse_optional_milli(argv[5], &aux_data->epu.pos_alt);
	hl78xx_gnss_parse_optional_milli(argv[6], &aux_data->epu.vel_3d);
	hl78xx_gnss_parse_optional_milli(argv[7], &aux_data->epu.vel_2d);
	hl78xx_gnss_parse_optional_milli(argv[8], &aux_data->epu.vel_hdg);
	hl78xx_gnss_parse_optional_milli(argv[9], &aux_data->epu.vel_east);
	hl78xx_gnss_parse_optional_milli(argv[10], &aux_data->epu.vel_north);
	hl78xx_gnss_parse_optional_milli(argv[11], &aux_data->epu.vel_up);

	return 0;
}

void gnss_nmea0183_match_epu_callback(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct hl78xx_gnss_data *data = (struct hl78xx_gnss_data *)user_data;
	int ret;

	ARG_UNUSED(chat);

	if (data == NULL) {
		return;
	}

	ret = hl78xx_gnss_parse_epu(argv, argc, &data->aux_data);
	if (ret < 0) {
		return;
	}

	if (hl78xx_gnss_aux_should_publish(HL78XX_GNSS_AUX_SENTENCE_EPU)) {
		gnss_publish_aux_data((const struct device *)data->dev, &data->aux_data,
				      sizeof(data->aux_data));
	}
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
