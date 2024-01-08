/*
 * Copyright 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <string.h>

#include "gnss_nmea0183.h"

#define TEST_DDMM_MMMM_MAX_ROUNDING_ERROR_NDEG (1)

struct test_ddmm_mmmm_sample {
	const char *ddmm_mmmm;
	int64_t ndeg;
};

/*
 * The conversion from ddmm.mmmm to decimal nano degree is
 * ((1/60) * mm.mmmm * 1E9) + (dd * 1E9)
 */
static const struct test_ddmm_mmmm_sample ddmm_mmmm_samples[] = {
	{.ddmm_mmmm = "00.0", .ndeg = 0},
	{.ddmm_mmmm = "000.0", .ndeg = 0},
	{.ddmm_mmmm = "9000.0000", .ndeg = 90000000000},
	{.ddmm_mmmm = "4530.0000", .ndeg = 45500000000},
	{.ddmm_mmmm = "4530.3000", .ndeg = 45505000000},
	{.ddmm_mmmm = "4530.3001", .ndeg = 45505001667},
	{.ddmm_mmmm = "4530.9999", .ndeg = 45516665000},
	{.ddmm_mmmm = "18000.0000", .ndeg = 180000000000}
};

ZTEST(gnss_nmea0183, test_ddmm_mmmm)
{
	int64_t min_ndeg;
	int64_t max_ndeg;
	int64_t ndeg;

	for (size_t i = 0; i < ARRAY_SIZE(ddmm_mmmm_samples); i++) {
		zassert_ok(gnss_nmea0183_ddmm_mmmm_to_ndeg(ddmm_mmmm_samples[i].ddmm_mmmm, &ndeg),
			   "Parse failed");

		min_ndeg = ddmm_mmmm_samples[i].ndeg - TEST_DDMM_MMMM_MAX_ROUNDING_ERROR_NDEG;
		max_ndeg = ddmm_mmmm_samples[i].ndeg + TEST_DDMM_MMMM_MAX_ROUNDING_ERROR_NDEG;
		zassert_true(ndeg >= min_ndeg, "Parsed value falls below max rounding error");
		zassert_true(ndeg <= max_ndeg, "Parsed value is above max rounding error");
	}

	/* Minutes can only go from 0 to 59.9999 */
	zassert_equal(gnss_nmea0183_ddmm_mmmm_to_ndeg("99.0000", &ndeg), -EINVAL,
		      "Parse should fail");

	zassert_equal(gnss_nmea0183_ddmm_mmmm_to_ndeg("60.0000", &ndeg), -EINVAL,
		      "Parse should fail");

	/* Missing dot */
	zassert_equal(gnss_nmea0183_ddmm_mmmm_to_ndeg("18000", &ndeg), -EINVAL,
		      "Parse should fail");

	/* Invalid chars */
	zassert_equal(gnss_nmea0183_ddmm_mmmm_to_ndeg("900#.0a000", &ndeg), -EINVAL,
		      "Parse should fail");

	/* Negative angle */
	zassert_equal(gnss_nmea0183_ddmm_mmmm_to_ndeg("-18000.0", &ndeg), -EINVAL,
		      "Parse should fail");
}

struct test_knots_to_mms_sample {
	const char *str;
	int64_t value;
};

static const struct test_knots_to_mms_sample knots_to_mms_samples[] = {
	{.str = "1", .value = 514},
	{.str = "2.2", .value = 1131},
	{.str = "003241.12543", .value = 1667364}
};

ZTEST(gnss_nmea0183, test_knots_to_mms)
{
	int64_t mms;

	for (size_t i = 0; i < ARRAY_SIZE(knots_to_mms_samples); i++) {
		zassert_ok(gnss_nmea0183_knots_to_mms(knots_to_mms_samples[i].str, &mms),
			   "Parse failed");

		zassert_equal(knots_to_mms_samples[i].value, mms,
			      "Parsed value falls below max rounding error");
	}
}

struct test_hhmmss_sample {
	const char *str;
	uint8_t hour;
	uint8_t minute;
	uint16_t millisecond;
};

static const struct test_hhmmss_sample hhmmss_samples[] = {
	{.str = "000102", .hour = 0, .minute = 1, .millisecond = 2000},
	{.str = "235959.999", .hour = 23, .minute = 59, .millisecond = 59999},
	{.str = "000000.0", .hour = 0, .minute = 0, .millisecond = 0}
};

ZTEST(gnss_nmea0183, test_hhmmss)
{
	struct gnss_time utc;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(hhmmss_samples); i++) {
		zassert_ok(gnss_nmea0183_parse_hhmmss(hhmmss_samples[i].str, &utc),
			   "Parse failed");

		zassert_equal(hhmmss_samples[i].hour, utc.hour, "Failed to parse hour");
		zassert_equal(hhmmss_samples[i].minute, utc.minute, "Failed to parse minute");
		zassert_equal(hhmmss_samples[i].millisecond, utc.millisecond,
			      "Failed to parse millisecond");
	}

	ret = gnss_nmea0183_parse_hhmmss("-101010", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_hhmmss("01010", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_hhmmss("246060.999", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_hhmmss("99a9c9", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_hhmmss("12121212", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");
}

struct test_ddmmyy_sample {
	const char *str;
	uint8_t month_day;
	uint8_t month;
	uint16_t century_year;
};

static const struct test_ddmmyy_sample ddmmyy_samples[] = {
	{.str = "010203", .month_day = 1, .month = 2, .century_year = 3},
	{.str = "311299", .month_day = 31, .month = 12, .century_year = 99},
	{.str = "010100", .month_day = 1, .month = 1, .century_year = 0}
};

ZTEST(gnss_nmea0183, test_ddmmyy)
{
	struct gnss_time utc;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(ddmmyy_samples); i++) {
		zassert_ok(gnss_nmea0183_parse_ddmmyy(ddmmyy_samples[i].str, &utc),
			   "Parse failed");

		zassert_equal(ddmmyy_samples[i].month_day, utc.month_day,
			      "Failed to parse monthday");

		zassert_equal(ddmmyy_samples[i].month, utc.month, "Failed to parse month");
		zassert_equal(ddmmyy_samples[i].century_year, utc.century_year,
			      "Failed to parse year");
	}

	ret = gnss_nmea0183_parse_ddmmyy("000000", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_ddmmyy("-12123", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_ddmmyy("01010", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_ddmmyy("999999", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_ddmmyy("99a9c9", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");

	ret = gnss_nmea0183_parse_ddmmyy("12121212", &utc);
	zassert_equal(ret, -EINVAL, "Should fail to parse invalid value");
}

/* "$GNRMC,160833.099,V,,,,,,,090923,,,N,V*27" */
const char *rmc_argv_no_fix[15] = {
	"$GNRMC",
	"160833.099",
	"V",
	"",
	"",
	"",
	"",
	"",
	"",
	"090923",
	"",
	"",
	"N",
	"V",
	"27"
};

static struct gnss_data data;

ZTEST(gnss_nmea0183, test_parse_rmc_no_fix)
{
	int ret;

	/* Corrupt data */
	memset(&data, 0xFF, sizeof(data));

	ret = gnss_nmea0183_parse_rmc(rmc_argv_no_fix, ARRAY_SIZE(rmc_argv_no_fix), &data);
	zassert_ok(ret, "NMEA0183 RMC message parse should succeed");
}

/* "$GNGGA,160834.099,,,,,0,0,,,M,,M,,*5E" */
const char *gga_argv_no_fix[16] = {
	"$GNGGA",
	"160834.099",
	"",
	"",
	"",
	"",
	"0",
	"0",
	"",
	"",
	"M",
	"",
	"M",
	"",
	"5E"
};

ZTEST(gnss_nmea0183, test_parse_gga_no_fix)
{
	int ret;

	/* Corrupt data */
	memset(&data, 0xFF, sizeof(data));

	ret = gnss_nmea0183_parse_gga(gga_argv_no_fix, ARRAY_SIZE(gga_argv_no_fix), &data);
	zassert_ok(ret, "NMEA0183 GGA message parse should succeed");
	zassert_equal(data.info.fix_quality, GNSS_FIX_QUALITY_INVALID,
		      "Incorrectly parsed fix quality");

	zassert_equal(data.info.fix_status, GNSS_FIX_STATUS_NO_FIX,
		      "Incorrectly parsed fix status");
}

/* "$GNRMC,160849.000,A,5709.736602,N,00957.660738,E,0.33,0.00,090923,,,A,V*03" */
const char *rmc_argv_fix[15] = {
	"$GNRMC",
	"160849.000",
	"A",
	"5709.736602",
	"N",
	"00957.660738",
	"E",
	"0.33",
	"33.31",
	"090923",
	"",
	"",
	"A",
	"V",
	"03",
};

ZTEST(gnss_nmea0183, test_parse_rmc_fix)
{
	int ret;

	/* Corrupt data */
	memset(&data, 0xFF, sizeof(data));

	ret = gnss_nmea0183_parse_rmc(rmc_argv_fix, ARRAY_SIZE(rmc_argv_fix), &data);
	zassert_ok(ret, "NMEA0183 RMC message parse should succeed");
	zassert_equal(data.nav_data.latitude, 57162276699, "Incorrectly parsed latitude");
	zassert_equal(data.nav_data.longitude, 9961012299, "Incorrectly parsed longitude");
	zassert_equal(data.nav_data.speed, 169, "Incorrectly parsed speed");
	zassert_equal(data.nav_data.bearing, 33310, "Incorrectly parsed speed");
	zassert_equal(data.utc.hour, 16, "Incorrectly parsed hour");
	zassert_equal(data.utc.minute, 8, "Incorrectly parsed minute");
	zassert_equal(data.utc.millisecond, 49000, "Incorrectly parsed millisecond");
	zassert_equal(data.utc.month_day, 9, "Incorrectly parsed month day");
	zassert_equal(data.utc.month, 9, "Incorrectly parsed month");
	zassert_equal(data.utc.century_year, 23, "Incorrectly parsed century year");
}

/* "$GNGGA,160858.000,5709.734778,N,00957.659514,E,1,6,1.41,15.234,M,42.371,M,,*72" */
const char *gga_argv_fix[16] = {
	"$GNGGA",
	"160858.000",
	"5709.734778",
	"N",
	"00957.659514",
	"E",
	"1",
	"6",
	"1.41",
	"15.234",
	"M",
	"42.371",
	"M",
	"",
	"",
	"72",
};

ZTEST(gnss_nmea0183, test_parse_gga_fix)
{
	int ret;

	/* Corrupt data */
	memset(&data, 0xFF, sizeof(data));

	ret = gnss_nmea0183_parse_gga(gga_argv_fix, ARRAY_SIZE(gga_argv_fix), &data);
	zassert_ok(ret, "NMEA0183 GGA message parse should succeed");
	zassert_equal(data.info.fix_quality, GNSS_FIX_QUALITY_GNSS_SPS,
		      "Incorrectly parsed fix quality");

	zassert_equal(data.info.fix_status, GNSS_FIX_STATUS_GNSS_FIX,
		      "Incorrectly parsed fix status");

	zassert_equal(data.info.satellites_cnt, 6,
		      "Incorrectly parsed number of satelites");

	zassert_equal(data.info.hdop, 1410, "Incorrectly parsed HDOP");
	zassert_equal(data.nav_data.altitude, 42371, "Incorrectly parsed altitude");
}

ZTEST(gnss_nmea0183, test_snprintk)
{
	int ret;
	char buf[sizeof("$PAIR002,3*27")];

	ret = gnss_nmea0183_snprintk(buf, sizeof(buf), "PAIR%03u,%u", 2, 3);
	zassert_equal(ret, (sizeof("$PAIR002,3*27") - 1), "Failed to format NMEA0183 message");
	zassert_ok(strcmp(buf, "$PAIR002,3*27"), "Incorrectly formatted NMEA0183 message");

	ret = gnss_nmea0183_snprintk(buf, sizeof(buf) - 1, "PAIR%03u,%u", 2, 3);
	zassert_equal(ret, -ENOMEM, "Should fail with -ENOMEM as buffer is too small");
}

/* $GPGSV,8,1,25,21,44,141,47,15,14,049,44,6,31,255,46,3,25,280,44*75 */
const char *gpgsv_8_1_25[21] = {
	"$GPGSV",
	"8",
	"1",
	"25",
	"21",
	"44",
	"141",
	"47",
	"15",
	"14",
	"049",
	"44",
	"6",
	"31",
	"255",
	"46",
	"3",
	"25",
	"280",
	"44",
	"75",
};

static const struct gnss_nmea0183_gsv_header gpgsv_8_1_25_header = {
	.system = GNSS_SYSTEM_GPS,
	.number_of_messages = 8,
	.message_number = 1,
	.number_of_svs = 25
};

static const struct gnss_satellite gpgsv_8_1_25_sats[] = {
	{.prn = 21, .elevation = 44, .azimuth = 141, .snr = 47,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
	{.prn = 15, .elevation = 14, .azimuth = 49, .snr = 44,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
	{.prn = 6, .elevation = 31, .azimuth = 255, .snr = 46,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
	{.prn = 3, .elevation = 25, .azimuth = 280, .snr = 44,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
};

/* $GPGSV,8,2,25,18,61,057,48,22,68,320,52,27,34,268,47,24,32,076,45*76 */
const char *gpgsv_8_2_25[21] = {
	"$GPGSV",
	"8",
	"2",
	"25",
	"18",
	"61",
	"057",
	"48",
	"22",
	"68",
	"320",
	"52",
	"27",
	"34",
	"268",
	"47",
	"24",
	"32",
	"076",
	"45",
	"76",
};

static const struct gnss_nmea0183_gsv_header gpgsv_8_2_25_header = {
	.system = GNSS_SYSTEM_GPS,
	.number_of_messages = 8,
	.message_number = 2,
	.number_of_svs = 25
};

static const struct gnss_satellite gpgsv_8_2_25_sats[] = {
	{.prn = 18, .elevation = 61, .azimuth = 57, .snr = 48,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
	{.prn = 22, .elevation = 68, .azimuth = 320, .snr = 52,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
	{.prn = 27, .elevation = 34, .azimuth = 268, .snr = 47,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
	{.prn = 24, .elevation = 32, .azimuth = 76, .snr = 45,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
};

/* $GPGSV,8,3,25,14,51,214,49,19,23,308,46*7E */
const char *gpgsv_8_3_25[13] = {
	"$GPGSV",
	"8",
	"3",
	"25",
	"14",
	"51",
	"214",
	"49",
	"19",
	"23",
	"308",
	"46",
	"7E",
};

static const struct gnss_nmea0183_gsv_header gpgsv_8_3_25_header = {
	.system = GNSS_SYSTEM_GPS,
	.number_of_messages = 8,
	.message_number = 3,
	.number_of_svs = 25
};

static const struct gnss_satellite gpgsv_8_3_25_sats[] = {
	{.prn = 14, .elevation = 51, .azimuth = 214, .snr = 49,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
	{.prn = 19, .elevation = 23, .azimuth = 308, .snr = 46,
	 .system = GNSS_SYSTEM_GPS, .is_tracked = true},
};

/* $GPGSV,8,4,25,51,44,183,49,46,41,169,43,48,36,220,45*47 */
const char *gpgsv_8_4_25[17] = {
	"$GPGSV",
	"8",
	"4",
	"25",
	"51",
	"44",
	"183",
	"49",
	"46",
	"41",
	"169",
	"43",
	"48",
	"36",
	"220",
	"45",
	"47",
};

static const struct gnss_nmea0183_gsv_header gpgsv_8_4_25_header = {
	.system = GNSS_SYSTEM_GPS,
	.number_of_messages = 8,
	.message_number = 4,
	.number_of_svs = 25
};

static const struct gnss_satellite gpgsv_8_4_25_sats[] = {
	{.prn = (51 + 87), .elevation = 44, .azimuth = 183, .snr = 49,
	 .system = GNSS_SYSTEM_SBAS, .is_tracked = true},
	{.prn = (46 + 87), .elevation = 41, .azimuth = 169, .snr = 43,
	 .system = GNSS_SYSTEM_SBAS, .is_tracked = true},
	{.prn = (48 + 87), .elevation = 36, .azimuth = 220, .snr = 45,
	 .system = GNSS_SYSTEM_SBAS, .is_tracked = true},
};

/* $GLGSV,8,5,25,82,49,219,52,76,22,051,41,83,37,316,51,67,57,010,51*6C */
const char *glgsv_8_5_25[21] = {
	"$GLGSV",
	"8",
	"5",
	"25",
	"82",
	"49",
	"219",
	"52",
	"76",
	"22",
	"051",
	"41",
	"83",
	"37",
	"316",
	"51",
	"67",
	"57",
	"010",
	"51",
	"6C",
};

static const struct gnss_nmea0183_gsv_header glgsv_8_5_25_header = {
	.system = GNSS_SYSTEM_GLONASS,
	.number_of_messages = 8,
	.message_number = 5,
	.number_of_svs = 25
};

static const struct gnss_satellite glgsv_8_5_25_sats[] = {
	{.prn = (82 - 64), .elevation = 49, .azimuth = 219, .snr = 52,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
	{.prn = (76 - 64), .elevation = 22, .azimuth = 51, .snr = 41,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
	{.prn = (83 - 64), .elevation = 37, .azimuth = 316, .snr = 51,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
	{.prn = (67 - 64), .elevation = 57, .azimuth = 10, .snr = 51,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
};

/* $GLGSV,8,6,25,77,24,108,44,81,10,181,46,78,1,152,34,66,18,060,45*50 */
const char *glgsv_8_6_25[21] = {
	"$GLGSV",
	"8",
	"6",
	"25",
	"77",
	"24",
	"108",
	"44",
	"81",
	"10",
	"181",
	"46",
	"78",
	"1",
	"152",
	"34",
	"66",
	"18",
	"060",
	"45",
	"50",
};

static const struct gnss_nmea0183_gsv_header glgsv_8_6_25_header = {
	.system = GNSS_SYSTEM_GLONASS,
	.number_of_messages = 8,
	.message_number = 6,
	.number_of_svs = 25
};

static const struct gnss_satellite glgsv_8_6_25_sats[] = {
	{.prn = (77 - 64), .elevation = 24, .azimuth = 108, .snr = 44,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
	{.prn = (81 - 64), .elevation = 10, .azimuth = 181, .snr = 46,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
	{.prn = (78 - 64), .elevation = 1, .azimuth = 152, .snr = 34,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
	{.prn = (66 - 64), .elevation = 18, .azimuth = 60, .snr = 45,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
};

/* $GLGSV,8,7,25,68,37,284,50*5C */
const char *glgsv_8_7_25[9] = {
	"$GLGSV",
	"8",
	"7",
	"25",
	"68",
	"37",
	"284",
	"50",
	"5C",
};

static const struct gnss_nmea0183_gsv_header glgsv_8_7_25_header = {
	.system = GNSS_SYSTEM_GLONASS,
	.number_of_messages = 8,
	.message_number = 7,
	.number_of_svs = 25
};

static const struct gnss_satellite glgsv_8_7_25_sats[] = {
	{.prn = (68 - 64), .elevation = 37, .azimuth = 284, .snr = 50,
	 .system = GNSS_SYSTEM_GLONASS, .is_tracked = true},
};

/* $GBGSV,8,8,25,111,35,221,47,112,4,179,39,114,48,290,48*11 */
const char *gbgsv_8_8_25[17] = {
	"$GBGSV",
	"8",
	"8",
	"25",
	"111",
	"35",
	"221",
	"47",
	"112",
	"4",
	"179",
	"39",
	"114",
	"48",
	"290",
	"48",
	"11",
};

static const struct gnss_nmea0183_gsv_header gbgsv_8_8_25_header = {
	.system = GNSS_SYSTEM_BEIDOU,
	.number_of_messages = 8,
	.message_number = 8,
	.number_of_svs = 25
};

static const struct gnss_satellite gbgsv_8_8_25_sats[] = {
	{.prn = (111 - 100), .elevation = 35, .azimuth = 221, .snr = 47,
	 .system = GNSS_SYSTEM_BEIDOU, .is_tracked = true},
	{.prn = (112 - 100), .elevation = 4, .azimuth = 179, .snr = 39,
	 .system = GNSS_SYSTEM_BEIDOU, .is_tracked = true},
	{.prn = (114 - 100), .elevation = 48, .azimuth = 290, .snr = 48,
	 .system = GNSS_SYSTEM_BEIDOU, .is_tracked = true},
};

struct test_gsv_sample {
	const char **argv;
	uint16_t argc;
	const struct gnss_nmea0183_gsv_header *header;
	const struct gnss_satellite *satellites;
	uint16_t number_of_svs;
};

static const struct test_gsv_sample gsv_samples[] = {
	{.argv = gpgsv_8_1_25, .argc = ARRAY_SIZE(gpgsv_8_1_25), .header = &gpgsv_8_1_25_header,
	 .satellites = gpgsv_8_1_25_sats, .number_of_svs = ARRAY_SIZE(gpgsv_8_1_25_sats)},
	{.argv = gpgsv_8_2_25, .argc = ARRAY_SIZE(gpgsv_8_2_25), .header = &gpgsv_8_2_25_header,
	 .satellites = gpgsv_8_2_25_sats, .number_of_svs = ARRAY_SIZE(gpgsv_8_2_25_sats)},
	{.argv = gpgsv_8_3_25, .argc = ARRAY_SIZE(gpgsv_8_3_25), .header = &gpgsv_8_3_25_header,
	 .satellites = gpgsv_8_3_25_sats, .number_of_svs = ARRAY_SIZE(gpgsv_8_3_25_sats)},
	{.argv = gpgsv_8_4_25, .argc = ARRAY_SIZE(gpgsv_8_4_25), .header = &gpgsv_8_4_25_header,
	 .satellites = gpgsv_8_4_25_sats, .number_of_svs = ARRAY_SIZE(gpgsv_8_4_25_sats)},
	{.argv = glgsv_8_5_25, .argc = ARRAY_SIZE(glgsv_8_5_25), .header = &glgsv_8_5_25_header,
	 .satellites = glgsv_8_5_25_sats, .number_of_svs = ARRAY_SIZE(glgsv_8_5_25_sats)},
	{.argv = glgsv_8_6_25, .argc = ARRAY_SIZE(glgsv_8_6_25), .header = &glgsv_8_6_25_header,
	 .satellites = glgsv_8_6_25_sats, .number_of_svs = ARRAY_SIZE(glgsv_8_6_25_sats)},
	{.argv = glgsv_8_7_25, .argc = ARRAY_SIZE(glgsv_8_7_25), .header = &glgsv_8_7_25_header,
	 .satellites = glgsv_8_7_25_sats, .number_of_svs = ARRAY_SIZE(glgsv_8_7_25_sats)},
	{.argv = gbgsv_8_8_25, .argc = ARRAY_SIZE(gbgsv_8_8_25), .header = &gbgsv_8_8_25_header,
	 .satellites = gbgsv_8_8_25_sats, .number_of_svs = ARRAY_SIZE(gbgsv_8_8_25_sats)},
};

ZTEST(gnss_nmea0183, test_gsv_parse_headers)
{
	struct gnss_nmea0183_gsv_header header;
	int ret;

	for (uint16_t i = 0; i < ARRAY_SIZE(gsv_samples); i++) {
		ret = gnss_nmea0183_parse_gsv_header(gsv_samples[i].argv, gsv_samples[i].argc,
						     &header);

		zassert_ok(ret, "Failed to parse GSV header");

		zassert_equal(header.system, gsv_samples[i].header->system,
			      "Failed to parse GNSS system");

		zassert_equal(header.number_of_messages,
			      gsv_samples[i].header->number_of_messages,
			      "Failed to parse number of messages");

		zassert_equal(header.message_number, gsv_samples[i].header->message_number,
			      "Failed to parse message number");

		zassert_equal(header.number_of_svs, gsv_samples[i].header->number_of_svs,
			      "Failed to parse number of space vehicles");
	}
}

ZTEST(gnss_nmea0183, test_gsv_parse_satellites)
{
	struct gnss_satellite satellites[4];
	int ret;

	for (uint16_t i = 0; i < ARRAY_SIZE(gsv_samples); i++) {
		ret = gnss_nmea0183_parse_gsv_svs(gsv_samples[i].argv, gsv_samples[i].argc,
						  satellites, ARRAY_SIZE(satellites));

		zassert_equal(ret, gsv_samples[i].number_of_svs,
			      "Incorrect number of satellites parsed");

		for (uint16_t u = 0; u < gsv_samples[i].number_of_svs; u++) {
			zassert_equal(gsv_samples[i].satellites[u].prn,
				      satellites[u].prn,
				      "Failed to parse satellite prn");
			zassert_equal(gsv_samples[i].satellites[u].snr,
				      satellites[u].snr,
				      "Failed to parse satellite snr");
			zassert_equal(gsv_samples[i].satellites[u].elevation,
				      satellites[u].elevation,
				      "Failed to parse satellite elevation");
			zassert_equal(gsv_samples[i].satellites[u].azimuth,
				      satellites[u].azimuth,
				      "Failed to parse satellite azimuth");
			zassert_equal(gsv_samples[i].satellites[u].system,
				      satellites[u].system,
				      "Failed to parse satellite system");
			zassert_equal(gsv_samples[i].satellites[u].is_tracked,
				      satellites[u].is_tracked,
				      "Failed to parse satellite is_tracked");
		}
	}
}

ZTEST_SUITE(gnss_nmea0183, NULL, NULL, NULL, NULL, NULL);
