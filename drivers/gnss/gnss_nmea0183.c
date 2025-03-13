/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <string.h>
#include <stdarg.h>
#include <stdarg.h>

#include "gnss_nmea0183.h"
#include "gnss_parse.h"

#define GNSS_NMEA0183_PICO_DEGREES_IN_DEGREE      (1000000000000ULL)
#define GNSS_NMEA0183_PICO_DEGREES_IN_MINUTE      (GNSS_NMEA0183_PICO_DEGREES_IN_DEGREE / 60ULL)
#define GNSS_NMEA0183_PICO_DEGREES_IN_NANO_DEGREE (1000ULL)
#define GNSS_NMEA0183_NANO_KNOTS_IN_MMS           (1943861LL)

#define GNSS_NMEA0183_MESSAGE_SIZE_MIN      (6)
#define GNSS_NMEA0183_MESSAGE_CHECKSUM_SIZE (3)

#define GNSS_NMEA0183_GSV_HDR_ARG_CNT (4)
#define GNSS_NMEA0183_GSV_SV_ARG_CNT  (4)

#define GNSS_NMEA0183_GSV_PRN_GPS_RANGE      (32)
#define GNSS_NMEA0183_GSV_PRN_SBAS_OFFSET    (87)
#define GNSS_NMEA0183_GSV_PRN_GLONASS_OFFSET (64)
#define GNSS_NMEA0183_GSV_PRN_BEIDOU_OFFSET  (100)

struct gsv_header_args {
	const char *message_id;
	const char *number_of_messages;
	const char *message_number;
	const char *numver_of_svs;
};

struct gsv_sv_args {
	const char *prn;
	const char *elevation;
	const char *azimuth;
	const char *snr;
};

static int gnss_system_from_gsv_header_args(const struct gsv_header_args *args,
					    enum gnss_system *sv_system)
{
	switch (args->message_id[2]) {
	case 'A':
		*sv_system = GNSS_SYSTEM_GALILEO;
		break;
	case 'B':
		*sv_system = GNSS_SYSTEM_BEIDOU;
		break;
	case 'P':
		*sv_system = GNSS_SYSTEM_GPS;
		break;
	case 'L':
		*sv_system = GNSS_SYSTEM_GLONASS;
		break;
	case 'Q':
		*sv_system = GNSS_SYSTEM_QZSS;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void align_satellite_with_gnss_system(enum gnss_system sv_system,
					     struct gnss_satellite *satellite)
{
	switch (sv_system) {
	case GNSS_SYSTEM_GPS:
		if (satellite->prn > GNSS_NMEA0183_GSV_PRN_GPS_RANGE) {
			satellite->system = GNSS_SYSTEM_SBAS;
			satellite->prn += GNSS_NMEA0183_GSV_PRN_SBAS_OFFSET;
			break;
		}

		satellite->system = GNSS_SYSTEM_GPS;
		break;

	case GNSS_SYSTEM_GLONASS:
		satellite->system = GNSS_SYSTEM_GLONASS;
		satellite->prn -= GNSS_NMEA0183_GSV_PRN_GLONASS_OFFSET;
		break;

	case GNSS_SYSTEM_GALILEO:
		satellite->system = GNSS_SYSTEM_GALILEO;
		break;

	case GNSS_SYSTEM_BEIDOU:
		satellite->system = GNSS_SYSTEM_BEIDOU;
		satellite->prn -= GNSS_NMEA0183_GSV_PRN_BEIDOU_OFFSET;
		break;

	case GNSS_SYSTEM_QZSS:
		satellite->system = GNSS_SYSTEM_QZSS;
		break;

	case GNSS_SYSTEM_IRNSS:
	case GNSS_SYSTEM_IMES:
	case GNSS_SYSTEM_SBAS:
		break;
	}
}

uint8_t gnss_nmea0183_checksum(const char *str)
{
	uint8_t checksum = 0;
	size_t end;

	__ASSERT(str != NULL, "str argument must be provided");

	end = strlen(str);
	for (size_t i = 0; i < end; i++) {
		checksum = checksum ^ str[i];
	}

	return checksum;
}

int gnss_nmea0183_snprintk(char *str, size_t size, const char *fmt, ...)
{
	va_list ap;
	uint8_t checksum;
	int pos;
	int len;

	__ASSERT(str != NULL, "str argument must be provided");
	__ASSERT(fmt != NULL, "fmt argument must be provided");

	if (size < GNSS_NMEA0183_MESSAGE_SIZE_MIN) {
		return -ENOMEM;
	}

	str[0] = '$';

	va_start(ap, fmt);
	pos = vsnprintk(&str[1], size - 1, fmt, ap) + 1;
	va_end(ap);

	if (pos < 0) {
		return -EINVAL;
	}

	len = pos + GNSS_NMEA0183_MESSAGE_CHECKSUM_SIZE;

	if ((size - 1) < len) {
		return -ENOMEM;
	}

	checksum = gnss_nmea0183_checksum(&str[1]);
	pos = snprintk(&str[pos], size - pos, "*%02X", checksum);
	if (pos != 3) {
		return -EINVAL;
	}

	str[len] = '\0';
	return len;
}

int gnss_nmea0183_ddmm_mmmm_to_ndeg(const char *ddmm_mmmm, int64_t *ndeg)
{
	uint64_t pico_degrees = 0;
	int8_t decimal = -1;
	int8_t pos = 0;
	uint64_t increment;

	__ASSERT(ddmm_mmmm != NULL, "ddmm_mmmm argument must be provided");
	__ASSERT(ndeg != NULL, "ndeg argument must be provided");

	/* Find decimal */
	while (ddmm_mmmm[pos] != '\0') {
		/* Verify if char is decimal */
		if (ddmm_mmmm[pos] == '.') {
			decimal = pos;
			break;
		}

		/* Advance position */
		pos++;
	}

	/* Verify decimal was found and placed correctly */
	if (decimal < 1) {
		return -EINVAL;
	}

	/* Validate potential degree fraction is within bounds */
	if (decimal > 1 && ddmm_mmmm[decimal - 2] > '5') {
		return -EINVAL;
	}

	/* Convert minute fraction to pico degrees and add it to pico_degrees */
	pos = decimal + 1;
	increment = (GNSS_NMEA0183_PICO_DEGREES_IN_MINUTE / 10);
	while (ddmm_mmmm[pos] != '\0') {
		/* Verify char is decimal */
		if (ddmm_mmmm[pos] < '0' || ddmm_mmmm[pos] > '9') {
			return -EINVAL;
		}

		/* Add increment to pico_degrees */
		pico_degrees += (ddmm_mmmm[pos] - '0') * increment;

		/* Update unit */
		increment /= 10;

		/* Increment position */
		pos++;
	}

	/* Convert minutes and degrees to pico_degrees */
	pos = decimal - 1;
	increment = GNSS_NMEA0183_PICO_DEGREES_IN_MINUTE;
	while (pos >= 0) {
		/* Check if digit switched from minutes to degrees */
		if ((decimal - pos) == 3) {
			/* Reset increment to degrees */
			increment = GNSS_NMEA0183_PICO_DEGREES_IN_DEGREE;
		}

		/* Verify char is decimal */
		if (ddmm_mmmm[pos] < '0' || ddmm_mmmm[pos] > '9') {
			return -EINVAL;
		}

		/* Add increment to pico_degrees */
		pico_degrees += (ddmm_mmmm[pos] - '0') * increment;

		/* Update unit */
		increment *= 10;

		/* Decrement position */
		pos--;
	}

	/* Convert to nano degrees */
	*ndeg = (int64_t)(pico_degrees / GNSS_NMEA0183_PICO_DEGREES_IN_NANO_DEGREE);
	return 0;
}

bool gnss_nmea0183_validate_message(char **argv, uint16_t argc)
{
	int32_t tmp = 0;
	uint8_t checksum = 0;
	size_t len;

	__ASSERT(argv != NULL, "argv argument must be provided");

	/* Message must contain message id and checksum */
	if (argc < 2) {
		return false;
	}

	/* First argument should start with '$' which is not covered by checksum */
	if ((argc < 1) || (argv[0][0] != '$')) {
		return false;
	}

	len = strlen(argv[0]);
	for (uint16_t u = 1; u < len; u++) {
		checksum ^= argv[0][u];
	}
	checksum ^= ',';

	/* Cover all except last argument which contains the checksum*/
	for (uint16_t i = 1; i < (argc - 1); i++) {
		len = strlen(argv[i]);
		for (uint16_t u = 0; u < len; u++) {
			checksum ^= argv[i][u];
		}
		checksum ^= ',';
	}

	if ((gnss_parse_atoi(argv[argc - 1], 16, &tmp) < 0) ||
	    (tmp > UINT8_MAX) ||
	    (tmp < 0)) {
		return false;
	}

	return checksum == (uint8_t)tmp;
}

int gnss_nmea0183_knots_to_mms(const char *str, int64_t *mms)
{
	int ret;

	__ASSERT(str != NULL, "str argument must be provided");
	__ASSERT(mms != NULL, "mms argument must be provided");

	ret = gnss_parse_dec_to_nano(str, mms);
	if (ret < 0) {
		return ret;
	}

	*mms = (*mms) / GNSS_NMEA0183_NANO_KNOTS_IN_MMS;
	return 0;
}

int gnss_nmea0183_parse_hhmmss(const char *hhmmss, struct gnss_time *utc)
{
	int64_t i64;
	int32_t i32;
	char part[3] = {0};

	__ASSERT(hhmmss != NULL, "hhmmss argument must be provided");
	__ASSERT(utc != NULL, "utc argument must be provided");

	if (strlen(hhmmss) < 6) {
		return -EINVAL;
	}

	memcpy(part, hhmmss, 2);
	if ((gnss_parse_atoi(part, 10, &i32) < 0) ||
	    (i32 < 0) ||
	    (i32 > 23)) {
		return -EINVAL;
	}

	utc->hour = (uint8_t)i32;

	memcpy(part, &hhmmss[2], 2);
	if ((gnss_parse_atoi(part, 10, &i32) < 0) ||
	    (i32 < 0) ||
	    (i32 > 59)) {
		return -EINVAL;
	}

	utc->minute = (uint8_t)i32;

	if ((gnss_parse_dec_to_milli(&hhmmss[4], &i64) < 0) ||
	    (i64 < 0) ||
	    (i64 > 59999)) {
		return -EINVAL;
	}

	utc->millisecond = (uint16_t)i64;
	return 0;
}

int gnss_nmea0183_parse_ddmmyy(const char *ddmmyy, struct gnss_time *utc)
{
	int32_t i32;
	char part[3] = {0};

	__ASSERT(ddmmyy != NULL, "ddmmyy argument must be provided");
	__ASSERT(utc != NULL, "utc argument must be provided");

	if (strlen(ddmmyy) != 6) {
		return -EINVAL;
	}

	memcpy(part, ddmmyy, 2);
	if ((gnss_parse_atoi(part, 10, &i32) < 0) ||
	    (i32 < 1) ||
	    (i32 > 31)) {
		return -EINVAL;
	}

	utc->month_day = (uint8_t)i32;

	memcpy(part, &ddmmyy[2], 2);
	if ((gnss_parse_atoi(part, 10, &i32) < 0) ||
	    (i32 < 1) ||
	    (i32 > 12)) {
		return -EINVAL;
	}

	utc->month = (uint8_t)i32;

	memcpy(part, &ddmmyy[4], 2);
	if ((gnss_parse_atoi(part, 10, &i32) < 0) ||
	    (i32 < 0) ||
	    (i32 > 99)) {
		return -EINVAL;
	}

	utc->century_year = (uint8_t)i32;
	return 0;
}

int gnss_nmea0183_parse_rmc(const char **argv, uint16_t argc, struct gnss_data *data)
{
	int64_t tmp;

	__ASSERT(argv != NULL, "argv argument must be provided");
	__ASSERT(data != NULL, "data argument must be provided");

	if (argc < 10)  {
		return -EINVAL;
	}

	/* Validate GNSS has fix */
	if (argv[2][0] == 'V') {
		return 0;
	}

	if (argv[2][0] != 'A') {
		return -EINVAL;
	}

	/* Parse UTC time */
	if ((gnss_nmea0183_parse_hhmmss(argv[1], &data->utc) < 0)) {
		return -EINVAL;
	}

	/* Validate cardinal directions */
	if (((argv[4][0] != 'N') && (argv[4][0] != 'S')) ||
	    ((argv[6][0] != 'E') && (argv[6][0] != 'W'))) {
		return -EINVAL;
	}

	/* Parse coordinates */
	if ((gnss_nmea0183_ddmm_mmmm_to_ndeg(argv[3], &data->nav_data.latitude) < 0) ||
	    (gnss_nmea0183_ddmm_mmmm_to_ndeg(argv[5], &data->nav_data.longitude) < 0)) {
		return -EINVAL;
	}

	/* Align sign of coordinates with cardinal directions */
	data->nav_data.latitude = argv[4][0] == 'N'
				    ? data->nav_data.latitude
				    : -data->nav_data.latitude;

	data->nav_data.longitude = argv[6][0] == 'E'
				     ? data->nav_data.longitude
				     : -data->nav_data.longitude;

	/* Parse speed */
	if ((gnss_nmea0183_knots_to_mms(argv[7], &tmp) < 0) ||
	    (tmp > UINT32_MAX)) {
		return -EINVAL;
	}

	data->nav_data.speed = (uint32_t)tmp;

	/* Parse bearing */
	if ((gnss_parse_dec_to_milli(argv[8], &tmp) < 0) ||
	    (tmp > 359999) ||
	    (tmp < 0)) {
		return -EINVAL;
	}

	data->nav_data.bearing = (uint32_t)tmp;

	/* Parse UTC date */
	if ((gnss_nmea0183_parse_ddmmyy(argv[9], &data->utc) < 0)) {
		return -EINVAL;
	}

	return 0;
}

static int parse_gga_fix_quality(const char *str, enum gnss_fix_quality *fix_quality)
{
	__ASSERT(str != NULL, "str argument must be provided");
	__ASSERT(fix_quality != NULL, "fix_quality argument must be provided");

	if ((str[1] != ((char)'\0')) || (str[0] < ((char)'0')) || (((char)'6') < str[0])) {
		return -EINVAL;
	}

	(*fix_quality) = (enum gnss_fix_quality)(str[0] - ((char)'0'));
	return 0;
}

static enum gnss_fix_status fix_status_from_fix_quality(enum gnss_fix_quality fix_quality)
{
	enum gnss_fix_status fix_status = GNSS_FIX_STATUS_NO_FIX;

	switch (fix_quality) {
	case GNSS_FIX_QUALITY_GNSS_SPS:
	case GNSS_FIX_QUALITY_GNSS_PPS:
		fix_status = GNSS_FIX_STATUS_GNSS_FIX;
		break;

	case GNSS_FIX_QUALITY_DGNSS:
	case GNSS_FIX_QUALITY_RTK:
	case GNSS_FIX_QUALITY_FLOAT_RTK:
		fix_status = GNSS_FIX_STATUS_DGNSS_FIX;
		break;

	case GNSS_FIX_QUALITY_ESTIMATED:
		fix_status = GNSS_FIX_STATUS_ESTIMATED_FIX;
		break;

	default:
		break;
	}

	return fix_status;
}

int gnss_nmea0183_parse_gga(const char **argv, uint16_t argc, struct gnss_data *data)
{
	int32_t tmp32;
	int64_t tmp64;

	__ASSERT(argv != NULL, "argv argument must be provided");
	__ASSERT(data != NULL, "data argument must be provided");

	if (argc < 12)  {
		return -EINVAL;
	}

	/* Parse fix quality and status */
	if (parse_gga_fix_quality(argv[6], &data->info.fix_quality) < 0) {
		return -EINVAL;
	}

	data->info.fix_status = fix_status_from_fix_quality(data->info.fix_quality);

	/* Validate GNSS has fix */
	if (data->info.fix_status == GNSS_FIX_STATUS_NO_FIX) {
		return 0;
	}

	/* Parse number of satellites */
	if ((gnss_parse_atoi(argv[7], 10, &tmp32) < 0) ||
	    (tmp32 > UINT16_MAX) ||
	    (tmp32 < 0)) {
		return -EINVAL;
	}

	data->info.satellites_cnt = (uint16_t)tmp32;

	/* Parse HDOP */
	if ((gnss_parse_dec_to_milli(argv[8], &tmp64) < 0) ||
	    (tmp64 > UINT32_MAX) ||
	    (tmp64 < 0)) {
		return -EINVAL;
	}

	data->info.hdop = (uint16_t)tmp64;

	/* Parse altitude */
	if ((gnss_parse_dec_to_milli(argv[9], &tmp64) < 0) ||
	    (tmp64 > INT32_MAX) ||
	    (tmp64 < INT32_MIN)) {
		return -EINVAL;
	}

	data->nav_data.altitude = (int32_t)tmp64;

	/* Parse geoid separation */
	if ((gnss_parse_dec_to_milli(argv[11], &tmp64) < 0) ||
	    (tmp64 > INT32_MAX) ||
	    (tmp64 < INT32_MIN)) {
		return -EINVAL;
	}

	data->info.geoid_separation = (int32_t)tmp64;

	return 0;
}

static int parse_gsv_svs(struct gnss_satellite *satellites, const struct gsv_sv_args *svs,
			 uint16_t svs_size)
{
	int32_t i32;

	for (uint16_t i = 0; i < svs_size; i++) {
		/* Parse PRN */
		if ((gnss_parse_atoi(svs[i].prn, 10, &i32) < 0) ||
		    (i32 < 0) || (i32 > UINT16_MAX)) {
			return -EINVAL;
		}

		satellites[i].prn = (uint16_t)i32;

		/* Parse elevation */
		if ((gnss_parse_atoi(svs[i].elevation, 10, &i32) < 0) ||
		    (i32 < 0) || (i32 > 90)) {
			return -EINVAL;
		}

		satellites[i].elevation = (uint8_t)i32;

		/* Parse azimuth */
		if ((gnss_parse_atoi(svs[i].azimuth, 10, &i32) < 0) ||
		    (i32 < 0) || (i32 > 359)) {
			return -EINVAL;
		}

		satellites[i].azimuth = (uint16_t)i32;

		/* Parse SNR */
		if (strlen(svs[i].snr) == 0) {
			satellites[i].snr = 0;
			satellites[i].is_tracked = false;
			continue;
		}

		if ((gnss_parse_atoi(svs[i].snr, 10, &i32) < 0) ||
			(i32 < 0) || (i32 > 99)) {
			return -EINVAL;
		}

		satellites[i].snr = (uint16_t)i32;
		satellites[i].is_tracked = true;
	}

	return 0;
}

int gnss_nmea0183_parse_gsv_header(const char **argv, uint16_t argc,
				   struct gnss_nmea0183_gsv_header *header)
{
	const struct gsv_header_args *args = (const struct gsv_header_args *)argv;
	int i32;

	__ASSERT(argv != NULL, "argv argument must be provided");
	__ASSERT(header != NULL, "header argument must be provided");

	if (argc < 4) {
		return -EINVAL;
	}

	/* Parse GNSS sv_system */
	if (gnss_system_from_gsv_header_args(args, &header->system) < 0) {
		return -EINVAL;
	}

	/* Parse number of messages */
	if ((gnss_parse_atoi(args->number_of_messages, 10, &i32) < 0) ||
		(i32 < 0) || (i32 > UINT16_MAX)) {
		return -EINVAL;
	}

	header->number_of_messages = (uint16_t)i32;

	/* Parse message number */
	if ((gnss_parse_atoi(args->message_number, 10, &i32) < 0) ||
		(i32 < 0) || (i32 > UINT16_MAX)) {
		return -EINVAL;
	}

	header->message_number = (uint16_t)i32;

	/* Parse message number */
	if ((gnss_parse_atoi(args->numver_of_svs, 10, &i32) < 0) ||
		(i32 < 0) || (i32 > UINT16_MAX)) {
		return -EINVAL;
	}

	header->number_of_svs = (uint16_t)i32;
	return 0;
}

int gnss_nmea0183_parse_gsv_svs(const char **argv, uint16_t argc,
				struct gnss_satellite *satellites, uint16_t size)
{
	const struct gsv_header_args *header_args = (const struct gsv_header_args *)argv;
	const struct gsv_sv_args *sv_args = (const struct gsv_sv_args *)(argv + 4);
	uint16_t sv_args_size;
	enum gnss_system sv_system;

	__ASSERT(argv != NULL, "argv argument must be provided");
	__ASSERT(satellites != NULL, "satellites argument must be provided");

	if (argc < 9) {
		return 0;
	}

	sv_args_size = (argc - GNSS_NMEA0183_GSV_HDR_ARG_CNT) / GNSS_NMEA0183_GSV_SV_ARG_CNT;

	if (size < sv_args_size) {
		return -ENOMEM;
	}

	if (parse_gsv_svs(satellites, sv_args, sv_args_size) < 0) {
		return -EINVAL;
	}

	if (gnss_system_from_gsv_header_args(header_args, &sv_system) < 0) {
		return -EINVAL;
	}

	for (uint16_t i = 0; i < sv_args_size; i++) {
		align_satellite_with_gnss_system(sv_system, &satellites[i]);
	}

	return (int)sv_args_size;
}
