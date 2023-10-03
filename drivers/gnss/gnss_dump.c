/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gnss_dump.h"
#include <zephyr/kernel.h>
#include <string.h>

static const char *gnss_fix_status_to_str(enum gnss_fix_status fix_status)
{
	switch (fix_status) {
	case GNSS_FIX_STATUS_NO_FIX:
		return "NO_FIX";
	case GNSS_FIX_STATUS_GNSS_FIX:
		return "GNSS_FIX";
	case GNSS_FIX_STATUS_DGNSS_FIX:
		return "DGNSS_FIX";
	case GNSS_FIX_STATUS_ESTIMATED_FIX:
		return "ESTIMATED_FIX";
	}

	return "unknown";
}

static const char *gnss_fix_quality_to_str(enum gnss_fix_quality fix_quality)
{
	switch (fix_quality) {
	case GNSS_FIX_QUALITY_INVALID:
		return "INVALID";
	case GNSS_FIX_QUALITY_GNSS_SPS:
		return "GNSS_SPS";
	case GNSS_FIX_QUALITY_DGNSS:
		return "DGNSS";
	case GNSS_FIX_QUALITY_GNSS_PPS:
		return "GNSS_PPS";
	case GNSS_FIX_QUALITY_RTK:
		return "RTK";
	case GNSS_FIX_QUALITY_FLOAT_RTK:
		return "FLOAT_RTK";
	case GNSS_FIX_QUALITY_ESTIMATED:
		return "ESTIMATED";
	}

	return "unknown";
}

#if CONFIG_GNSS_SATELLITES
static const char *gnss_system_to_str(enum gnss_system system)
{
	switch (system) {
	case GNSS_SYSTEM_GPS:
		return "GPS";
	case GNSS_SYSTEM_GLONASS:
		return "GLONASS";
	case GNSS_SYSTEM_GALILEO:
		return "GALILEO";
	case GNSS_SYSTEM_BEIDOU:
		return "BEIDOU";
	case GNSS_SYSTEM_QZSS:
		return "QZSS";
	case GNSS_SYSTEM_IRNSS:
		return "IRNSS";
	case GNSS_SYSTEM_SBAS:
		return "SBAS";
	case GNSS_SYSTEM_IMES:
		return "IMES";
	}

	return "unknown";
}
#endif

int gnss_dump_info(char *str, uint16_t strsize, const struct gnss_info *info)
{
	int ret;
	const char *fmt = "gnss_info: {satellites_cnt: %u, hdop: %u.%u, fix_status: %s, "
			  "fix_quality: %s}";

	ret = snprintk(str, strsize, fmt, info->satellites_cnt, info->hdop / 1000,
		       info->hdop % 1000, gnss_fix_status_to_str(info->fix_status),
		       gnss_fix_quality_to_str(info->fix_quality));

	return (strsize < ret) ? -ENOMEM : 0;
}

int gnss_dump_nav_data(char *str, uint16_t strsize, const struct navigation_data *nav_data)
{
	int ret;
	int32_t altitude_int;
	int32_t altitude_fraction;
	const char *fmt = "navigation_data: {latitude: %lli.%lli, longitude : %lli.%lli, "
			  "bearing %u.%u, speed %u.%u, altitude: %i.%i}";

	altitude_int = nav_data->altitude / 1000;
	altitude_fraction = nav_data->altitude % 1000;
	altitude_fraction = (altitude_fraction < 0) ? -altitude_fraction : altitude_fraction;

	ret = snprintk(str, strsize, fmt, nav_data->latitude / 1000000000,
		       nav_data->latitude % 1000000000, nav_data->longitude / 1000000000,
		       nav_data->longitude % 1000000000, nav_data->bearing / 1000,
		       nav_data->bearing % 1000, nav_data->speed / 1000, nav_data->speed % 1000,
		       altitude_int, altitude_fraction);

	return (strsize < ret) ? -ENOMEM : 0;
}

int gnss_dump_time(char *str, uint16_t strsize, const struct gnss_time *utc)
{
	int ret;
	const char *fmt = "gnss_time: {hour: %u, minute: %u, millisecond %u, month_day %u, "
			  "month: %u, century_year: %u}";

	ret = snprintk(str, strsize, fmt, utc->hour, utc->minute, utc->millisecond,
		       utc->month_day, utc->month, utc->century_year);

	return (strsize < ret) ? -ENOMEM : 0;
}

#if CONFIG_GNSS_SATELLITES
int gnss_dump_satellite(char *str, uint16_t strsize, const struct gnss_satellite *satellite)
{
	int ret;
	const char *fmt = "gnss_satellite: {prn: %u, snr: %u, elevation %u, azimuth %u, "
			  "system: %s, is_tracked: %u}";

	ret = snprintk(str, strsize, fmt, satellite->prn, satellite->snr, satellite->elevation,
		       satellite->azimuth, gnss_system_to_str(satellite->system),
		       satellite->is_tracked);

	return (strsize < ret) ? -ENOMEM : 0;
}
#endif
