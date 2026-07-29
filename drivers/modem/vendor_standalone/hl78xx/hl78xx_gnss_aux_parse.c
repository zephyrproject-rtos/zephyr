/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include "hl78xx_gnss_aux_parse.h"
#include "../../../gnss/gnss_parse.h"

/*
 * @brief Parse a mandatory NMEA UTC field to milliseconds.
 *
 * @param argv NMEA argument vector.
 * @param argc Number of arguments in argv.
 * @param utc Destination UTC value in milliseconds.
 *
 * @retval 0 on success.
 * @retval -EINVAL on invalid input.
 */
static int hl78xx_gnss_parse_utc(char **argv, uint16_t argc, uint32_t *utc)
{
	int64_t i64;

	if (argv == NULL || utc == NULL || argc < 2 || argv[1] == NULL) {
		return -EINVAL;
	}

	if ((gnss_parse_dec_to_milli(argv[1], &i64) < 0) || i64 < 0 || i64 > UINT32_MAX) {
		return -EINVAL;
	}

	*utc = (uint32_t)i64;
	return 0;
}

/*
 * @brief Parse an optional decimal field as milli-units.
 *
 * Missing, empty, or invalid optional fields are converted to zero.
 *
 * @param str Decimal string.
 * @param value Destination parsed value.
 */
static void hl78xx_gnss_parse_optional_milli(const char *str, int64_t *value)
{
	if (value == NULL) {
		return;
	}

	if (str == NULL || str[0] == '\0') {
		*value = 0;
		return;
	}

	if (gnss_parse_dec_to_milli(str, value) < 0) {
		*value = 0;
	}
}

int hl78xx_gnss_parse_gsa(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data)
{
	if (argv == NULL || aux_data == NULL || argc < 18) {
		return -EINVAL;
	}

	if (argv[2] == NULL) {
		return -EINVAL;
	}

	if (gnss_parse_atoi(argv[2], 10, &aux_data->gsa.fix_type) < 0) {
		return -EINVAL;
	}

	hl78xx_gnss_parse_optional_milli(argv[15], &aux_data->gsa.pdop);
	hl78xx_gnss_parse_optional_milli(argv[16], &aux_data->gsa.hdop);
	hl78xx_gnss_parse_optional_milli(argv[17], &aux_data->gsa.vdop);

	return 0;
}

int hl78xx_gnss_parse_gst(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data)
{
	if (argv == NULL || aux_data == NULL || argc < 9) {
		return -EINVAL;
	}

	if (hl78xx_gnss_parse_utc(argv, argc, &aux_data->gst.gst_utc) < 0) {
		return -EINVAL;
	}

	hl78xx_gnss_parse_optional_milli(argv[2], &aux_data->gst.rms);
	hl78xx_gnss_parse_optional_milli(argv[6], &aux_data->gst.lat_err);
	hl78xx_gnss_parse_optional_milli(argv[7], &aux_data->gst.lon_err);
	hl78xx_gnss_parse_optional_milli(argv[8], &aux_data->gst.alt_err);

	return 0;
}

int hl78xx_gnss_parse_epu(char **argv, uint16_t argc, struct hl78xx_gnss_nmea_aux_data *aux_data)
{
	if (argv == NULL || aux_data == NULL || argc < 12) {
		return -EINVAL;
	}

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
