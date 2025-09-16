/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>

#include "gnss_ubx_common.h"

void gnss_ubx_common_pvt_callback(struct modem_ubx *ubx, const struct ubx_frame *frame,
				 size_t len, void *user_data)
{
	if (len < UBX_FRAME_SZ(sizeof(struct ubx_nav_pvt))) {
		return;
	}

	struct gnss_ubx_common_data *data = user_data;
	const struct device *dev = data->gnss;
	const struct ubx_nav_pvt *nav_pvt = (const struct ubx_nav_pvt *)frame->payload_and_checksum;

	enum gnss_fix_quality fix_quality = GNSS_FIX_QUALITY_INVALID;
	enum gnss_fix_status fix_status = GNSS_FIX_STATUS_NO_FIX;

	if ((nav_pvt->flags & UBX_NAV_PVT_FLAGS_GNSS_FIX_OK) &&
	    !(nav_pvt->nav.flags3 & UBX_NAV_PVT_FLAGS3_INVALID_LLH)) {

		if (nav_pvt->flags & UBX_NAV_PVT_FLAGS_GNSS_CARR_SOLN_FLOATING) {
			fix_quality = GNSS_FIX_QUALITY_FLOAT_RTK;
			fix_status = GNSS_FIX_STATUS_DGNSS_FIX;
		} else if (nav_pvt->flags & UBX_NAV_PVT_FLAGS_GNSS_CARR_SOLN_FIXED) {
			fix_quality = GNSS_FIX_QUALITY_RTK;
			fix_status = GNSS_FIX_STATUS_DGNSS_FIX;
		} else if (
			(nav_pvt->fix_type == UBX_NAV_FIX_TYPE_GNSS_DR_COMBINED) ||
			(nav_pvt->fix_type == UBX_NAV_FIX_TYPE_DR)) {

			fix_quality = GNSS_FIX_QUALITY_ESTIMATED;
			fix_status = GNSS_FIX_STATUS_ESTIMATED_FIX;
		} else if (
			(nav_pvt->fix_type == UBX_NAV_FIX_TYPE_2D) ||
			(nav_pvt->fix_type == UBX_NAV_FIX_TYPE_3D)) {

			fix_quality = GNSS_FIX_QUALITY_GNSS_SPS;
			fix_status = GNSS_FIX_STATUS_GNSS_FIX;
		} else {
			/** This is handled by the already initialized state of
			 * fix_quality and fix_status, conveying info the fix
			 * is not valid.
			 */
		}
	}

	struct gnss_data gnss_data = {
		.info = {
			.satellites_cnt = nav_pvt->nav.num_sv,
			.hdop = nav_pvt->nav.pdop * 10,
			.geoid_separation = (nav_pvt->nav.height -
						nav_pvt->nav.hmsl),
			.fix_status = fix_status,
			.fix_quality = fix_quality,
		},
		.nav_data = {
			.latitude = (int64_t)nav_pvt->nav.latitude * 100,
			.longitude = (int64_t)nav_pvt->nav.longitude * 100,
			.bearing = (((nav_pvt->nav.head_motion < 0) ?
					(nav_pvt->nav.head_motion + (360 * 100000)) :
					(nav_pvt->nav.head_motion)) / 100),
			.speed = nav_pvt->nav.ground_speed,
			.altitude = nav_pvt->nav.hmsl,
		},
		.utc = {
			.hour = nav_pvt->time.hour,
			.minute = nav_pvt->time.minute,
			.millisecond = (nav_pvt->time.second * 1000) +
					(nav_pvt->time.nano / 1000000),
			.month_day = nav_pvt->time.day,
			.month = nav_pvt->time.month,
			.century_year = (nav_pvt->time.year % 100),
		},
	};

	gnss_publish_data(dev, &gnss_data);
}

#if CONFIG_GNSS_SATELLITES
void gnss_ubx_common_satellite_callback(struct modem_ubx *ubx, const struct ubx_frame *frame,
				       size_t len, void *user_data)
{
	if (len < UBX_FRAME_SZ(sizeof(struct ubx_nav_sat))) {
		return;
	}

	const struct ubx_nav_sat *ubx_sat = (const struct ubx_nav_sat *)frame->payload_and_checksum;
	int num_satellites = (len - UBX_FRAME_SZ_WITHOUT_PAYLOAD - sizeof(struct ubx_nav_sat)) /
			     sizeof(struct ubx_nav_sat_info);
	struct gnss_ubx_common_data *data = user_data;
	const struct device *dev = data->gnss;

	num_satellites = MIN(num_satellites, data->satellites.size);

	for (size_t i = 0 ; i < num_satellites ; i++) {
		enum gnss_system gnss_system = 0;

		switch (ubx_sat->sat[i].gnss_id) {
		case UBX_GNSS_ID_GPS:
			gnss_system = GNSS_SYSTEM_GPS;
			break;
		case UBX_GNSS_ID_SBAS:
			gnss_system = GNSS_SYSTEM_SBAS;
			break;
		case UBX_GNSS_ID_GALILEO:
			gnss_system = GNSS_SYSTEM_GALILEO;
			break;
		case UBX_GNSS_ID_BEIDOU:
			gnss_system = GNSS_SYSTEM_BEIDOU;
			break;
		case UBX_GNSS_ID_QZSS:
			gnss_system = GNSS_SYSTEM_QZSS;
			break;
		case UBX_GNSS_ID_GLONASS:
			gnss_system = GNSS_SYSTEM_GLONASS;
			break;
		default:
			break;
		}

		struct gnss_satellite sat = {
			/** TODO: Determine how to determine PRN from UBX sat info.
			 * For now passing SV_ID.
			 */
			.prn = ubx_sat->sat[i].sv_id,
			.snr = ubx_sat->sat[i].cno,
			.elevation = ubx_sat->sat[i].elevation,
			.azimuth = ubx_sat->sat[i].azimuth,
			.system = gnss_system,
			.is_tracked = !!(ubx_sat->sat[i].flags & UBX_NAV_SAT_FLAGS_SV_USED),
			.is_corrected = !!(ubx_sat->sat[i].flags & UBX_NAV_SAT_FLAGS_RTCM_CORR_USED)
		};

		data->satellites.data[i] = sat;
	}

	gnss_publish_satellites(dev, data->satellites.data, num_satellites);
}
#endif

void gnss_ubx_common_init(struct gnss_ubx_common_data *data,
			 const struct gnss_ubx_common_config *config)
{
	data->gnss = config->gnss;
#if CONFIG_GNSS_SATELLITES
	data->satellites.data = config->satellites.buf;
	data->satellites.size = config->satellites.size;
#endif
}
