/*
 * Copyright (c) 2024 NXP
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT u_blox_m8

#include <zephyr/kernel.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>

#include <zephyr/modem/ubx.h>
#include <zephyr/modem/backend/uart.h>

#include "gnss_u_blox_iface.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ubx_m8, CONFIG_GNSS_LOG_LEVEL);

UBX_FRAME_DEFINE(disable_gga,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_GGA, 0));
UBX_FRAME_DEFINE(disable_rmc,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_RMC, 0));
UBX_FRAME_DEFINE(disable_gsv,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_GSV, 0));
UBX_FRAME_DEFINE(disable_dtm,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_DTM, 0));
UBX_FRAME_DEFINE(disable_gbs,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_GBS, 0));
UBX_FRAME_DEFINE(disable_gll,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_GLL, 0));
UBX_FRAME_DEFINE(disable_gns,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_GNS, 0));
UBX_FRAME_DEFINE(disable_grs,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_GRS, 0));
UBX_FRAME_DEFINE(disable_gsa,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_GSA, 0));
UBX_FRAME_DEFINE(disable_gst,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_GST, 0));
UBX_FRAME_DEFINE(disable_vlw,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_VLW, 0));
UBX_FRAME_DEFINE(disable_vtg,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_VTG, 0));
UBX_FRAME_DEFINE(disable_zda,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NMEA_STD, UBX_MSG_ID_NMEA_STD_ZDA, 0));
UBX_FRAME_DEFINE(enable_nav,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_PVT, 1));
#if CONFIG_GNSS_SATELLITES
UBX_FRAME_DEFINE(enable_sat,
	UBX_FRAME_CFG_MSG_RATE_INITIALIZER(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_SAT, 1));
#endif

UBX_FRAME_ARRAY_DEFINE(u_blox_m8_init_seq,
	&disable_gga, &disable_rmc, &disable_gsv, &disable_dtm, &disable_gbs,
	&disable_gll, &disable_gns, &disable_grs, &disable_gsa, &disable_gst,
	&disable_vlw, &disable_vtg, &disable_zda, &enable_nav,
#if CONFIG_GNSS_SATELLITES
	&enable_sat,
#endif
);

MODEM_UBX_MATCH_ARRAY_DEFINE(u_blox_m8_unsol_messages,
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_PVT,
			       gnss_ubx_common_pvt_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_SAT,
			       gnss_ubx_common_satellite_callback),
#endif
);

static int ubx_m8_init(const struct device *dev)
{
	int err = 0;
	const struct u_blox_iface_config *cfg = dev->config;

	err = u_blox_iface_init(dev, u_blox_m8_unsol_messages,
				ARRAY_SIZE(u_blox_m8_unsol_messages), false);
	if (err < 0) {
		return err;
	}

	const static struct ubx_frame version_get = UBX_FRAME_GET_INITIALIZER(
						UBX_CLASS_ID_MON,
						UBX_MSG_ID_MON_VER);
	struct ubx_mon_ver ver;

	err = u_blox_iface_msg_get(dev, &version_get,
				   UBX_FRAME_SZ(version_get.payload_size),
				   (void *)&ver, sizeof(ver));
	if (err != 0) {
		LOG_ERR("Failed to get Modem Version info: %d", err);
		return err;
	}
	LOG_INF("SW Version: %s, HW Version: %s", ver.sw_ver, ver.hw_ver);

	const static struct ubx_frame stop_gnss = UBX_FRAME_CFG_RST_INITIALIZER(
							UBX_CFG_RST_HOT_START,
							UBX_CFG_RST_MODE_GNSS_STOP);

	err = u_blox_iface_msg_send(dev, &stop_gnss,
				    UBX_FRAME_SZ(stop_gnss.payload_size), false);
	if (err != 0) {
		LOG_ERR("Failed to stop GNSS module: %d", err);
		return err;
	}
	k_sleep(K_MSEC(1000));

	err = gnss_set_fix_rate(dev, cfg->fix_rate_ms);
	if (err != 0) {
		LOG_ERR("Failed to set fix-rate: %d", err);
		return err;
	}

	for (size_t i = 0 ; i < ARRAY_SIZE(u_blox_m8_init_seq) ; i++) {
		err = u_blox_iface_msg_send(dev,
				      u_blox_m8_init_seq[i],
				      UBX_FRAME_SZ(u_blox_m8_init_seq[i]->payload_size),
				      true);
		if (err < 0) {
			LOG_ERR("Failed to send init sequence - idx: %d, result: %d", i, err);
			return err;
		}
	}

	const static struct ubx_frame start_gnss = UBX_FRAME_CFG_RST_INITIALIZER(
							UBX_CFG_RST_HOT_START,
							UBX_CFG_RST_MODE_GNSS_START);

	err = u_blox_iface_msg_send(dev, &start_gnss,
				    UBX_FRAME_SZ(start_gnss.payload_size), false);
	if (err != 0) {
		LOG_ERR("Failed to start GNSS module: %d", err);
		return err;
	}

	return 0;
}

static int ubx_m8_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	if (fix_interval_ms < 50 || fix_interval_ms > 65535) {
		return -EINVAL;
	}

	struct ubx_cfg_rate rate = {
		.meas_rate_ms = (uint16_t)fix_interval_ms,
		.nav_rate = 1,
		.time_ref = UBX_CFG_RATE_TIME_REF_GPS,
	};

	return u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_RATE,
					     (const uint8_t *)&rate, sizeof(rate),
					     true);
}

static int ubx_m8_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	struct ubx_cfg_rate rate;
	int err;

	const static struct ubx_frame get_fix_rate = UBX_FRAME_GET_INITIALIZER(UBX_CLASS_ID_CFG,
									       UBX_MSG_ID_CFG_RATE);

	err = u_blox_iface_msg_get(dev, &get_fix_rate,
				   UBX_FRAME_SZ(get_fix_rate.payload_size),
				   (void *)&rate, sizeof(rate));
	if (err == 0) {
		*fix_interval_ms = rate.meas_rate_ms;
	}

	return err;
}

static int ubx_m8_set_navigation_mode(const struct device *dev, enum gnss_navigation_mode mode)
{
	enum ubx_dyn_model nav_model;

	switch (mode) {
	case GNSS_NAVIGATION_MODE_ZERO_DYNAMICS:
		nav_model = UBX_DYN_MODEL_STATIONARY;
		break;
	case GNSS_NAVIGATION_MODE_LOW_DYNAMICS:
		nav_model = UBX_DYN_MODEL_PEDESTRIAN;
		break;
	case GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS:
		nav_model = UBX_DYN_MODEL_PORTABLE;
		break;
	case GNSS_NAVIGATION_MODE_HIGH_DYNAMICS:
		nav_model = UBX_DYN_MODEL_AIRBORNE_2G;
		break;
	default:
		return -EINVAL;
	}

	/** The zero'd elements won't be applied as long as their apply bit is not set. */
	struct ubx_cfg_nav5 nav_mode = {
		.apply = (UBX_CFG_NAV5_APPLY_DYN | UBX_CFG_NAV5_APPLY_FIX_MODE),
		.dyn_model = nav_model,
		.fix_mode = UBX_FIX_MODE_AUTO,
	};

	return u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_NAV5,
				       (const uint8_t *)&nav_mode, sizeof(nav_mode),
				       true);
}

static int ubx_m8_get_navigation_mode(const struct device *dev, enum gnss_navigation_mode *mode)
{
	struct ubx_cfg_nav5 nav_mode;
	int err;

	const static struct ubx_frame get_nav_mode = UBX_FRAME_GET_INITIALIZER(UBX_CLASS_ID_CFG,
									       UBX_MSG_ID_CFG_NAV5);

	err = u_blox_iface_msg_get(dev, &get_nav_mode,
			     UBX_FRAME_SZ(get_nav_mode.payload_size),
			     &nav_mode, sizeof(nav_mode));

	switch (nav_mode.dyn_model) {
	case UBX_DYN_MODEL_STATIONARY:
		*mode = GNSS_NAVIGATION_MODE_ZERO_DYNAMICS;
		break;
	case UBX_DYN_MODEL_PEDESTRIAN:
		*mode = GNSS_NAVIGATION_MODE_LOW_DYNAMICS;
		break;
	case UBX_DYN_MODEL_PORTABLE:
		*mode = GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
		break;
	case UBX_DYN_MODEL_AIRBORNE_2G:
		*mode = GNSS_NAVIGATION_MODE_HIGH_DYNAMICS;
		break;
	default:
		return -EIO;
	}

	return 0;
}

static int ubx_m8_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	return -ENOTSUP;
}

static int ubx_m8_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	static const struct ubx_frame get_enabled_systems = UBX_FRAME_GET_INITIALIZER(
									UBX_CLASS_ID_MON,
									UBX_MSG_ID_MON_GNSS);
	struct ubx_mon_gnss gnss_selection;
	int err;

	err = u_blox_iface_msg_get(dev, &get_enabled_systems,
			     UBX_FRAME_SZ(get_enabled_systems.payload_size),
			     (void *)&gnss_selection, sizeof(gnss_selection));
	if (err != 0) {
		return err;
	}

	*systems = 0;
	*systems |= (gnss_selection.selection.enabled & UBX_GNSS_SELECTION_GPS) ?
			GNSS_SYSTEM_GPS : 0;
	*systems |= (gnss_selection.selection.enabled & UBX_GNSS_SELECTION_GLONASS) ?
			GNSS_SYSTEM_GLONASS : 0;
	*systems |= (gnss_selection.selection.enabled & UBX_GNSS_SELECTION_BEIDOU) ?
			GNSS_SYSTEM_BEIDOU : 0;
	*systems |= (gnss_selection.selection.enabled & UBX_GNSS_SELECTION_GALILEO) ?
			GNSS_SYSTEM_GALILEO : 0;

	return 0;
}

static int ubx_m8_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	static const struct ubx_frame get_enabled_systems = UBX_FRAME_GET_INITIALIZER(
									UBX_CLASS_ID_MON,
									UBX_MSG_ID_MON_GNSS);
	struct ubx_mon_gnss gnss_selection;
	int err;

	err = u_blox_iface_msg_get(dev, &get_enabled_systems,
			     UBX_FRAME_SZ(get_enabled_systems.payload_size),
			     (void *)&gnss_selection, sizeof(gnss_selection));
	if (err != 0) {
		return err;
	}

	*systems = 0;
	*systems |= (gnss_selection.selection.supported & UBX_GNSS_SELECTION_GPS) ?
			GNSS_SYSTEM_GPS : 0;
	*systems |= (gnss_selection.selection.supported & UBX_GNSS_SELECTION_GLONASS) ?
			GNSS_SYSTEM_GLONASS : 0;
	*systems |= (gnss_selection.selection.supported & UBX_GNSS_SELECTION_BEIDOU) ?
			GNSS_SYSTEM_BEIDOU : 0;
	*systems |= (gnss_selection.selection.supported & UBX_GNSS_SELECTION_GALILEO) ?
			GNSS_SYSTEM_GALILEO : 0;

	return 0;
}

static DEVICE_API(gnss, gnss_api) = {
	.set_fix_rate = ubx_m8_set_fix_rate,
	.get_fix_rate = ubx_m8_get_fix_rate,
	.set_navigation_mode = ubx_m8_set_navigation_mode,
	.get_navigation_mode = ubx_m8_get_navigation_mode,
	.set_enabled_systems = ubx_m8_set_enabled_systems,
	.get_enabled_systems = ubx_m8_get_enabled_systems,
	.get_supported_systems = ubx_m8_get_supported_systems,
};

#define UBX_M8(inst)										   \
												   \
	BUILD_ASSERT(										   \
		(DT_PROP(DT_INST_BUS(inst), current_speed) == 9600) ||				   \
		(DT_PROP(DT_INST_BUS(inst), current_speed) == 19200) ||				   \
		(DT_PROP(DT_INST_BUS(inst), current_speed) == 38400) ||				   \
		(DT_PROP(DT_INST_BUS(inst), current_speed) == 57600) ||				   \
		(DT_PROP(DT_INST_BUS(inst), current_speed) == 115200) ||			   \
		(DT_PROP(DT_INST_BUS(inst), current_speed) == 230400) ||			   \
		(DT_PROP(DT_INST_BUS(inst), current_speed) == 460800),				   \
		"Invalid current-speed. Please set the UART current-speed to a baudrate "	   \
		"compatible with the modem.");							   \
												   \
	BUILD_ASSERT((DT_INST_PROP(inst, fix_rate) >= 50) &&					   \
		     (DT_INST_PROP(inst, fix_rate) < 65536),					   \
		     "Invalid fix-rate. Please set it higher than 50-ms"			   \
		     " and must fit in 16-bits.");						   \
												   \
	static const struct u_blox_iface_config u_blox_m8_cfg_##inst = {			   \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),					   \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),			   \
		.baudrate = {									   \
			.initial = DT_INST_PROP(inst, initial_baudrate),			   \
			.desired = DT_PROP(DT_INST_BUS(inst), current_speed),			   \
		},										   \
		.fix_rate_ms = DT_INST_PROP(inst, fix_rate),					   \
	};											   \
												   \
	static struct u_blox_iface_data u_blox_m8_data_##inst;					   \
												   \
	DEVICE_DT_INST_DEFINE(inst,								   \
			      ubx_m8_init,							   \
			      NULL,								   \
			      &u_blox_m8_data_##inst,						   \
			      &u_blox_m8_cfg_##inst,						   \
			      POST_KERNEL,							   \
			      CONFIG_GNSS_INIT_PRIORITY,					   \
			      &gnss_api);

DT_INST_FOREACH_STATUS_OKAY(UBX_M8)
