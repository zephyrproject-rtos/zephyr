/*
 * Copyright (c) 2026 CampOS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT u_blox_m10

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>

#include <zephyr/modem/ubx.h>
#include <zephyr/modem/ubx/protocol.h>
#include <zephyr/modem/ubx/keys.h>
#include <zephyr/modem/backend/uart.h>

#include "gnss_u_blox_iface.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ubx_m10, CONFIG_GNSS_LOG_LEVEL);

UBX_FRAME_DEFINE(disable_nmea,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_UART1_PROTO_OUT_NMEA, 0));
UBX_FRAME_DEFINE(enable_nav,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_UBX_NAV_PVT_UART1, 1));
UBX_FRAME_DEFINE(nav_fix_mode_auto,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_NAV_CFG_FIX_MODE, UBX_FIX_MODE_AUTO));
UBX_FRAME_DEFINE(enable_prot_in_ubx,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_UART1_PROTO_IN_UBX, 1));
UBX_FRAME_DEFINE(enable_prot_out_ubx,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_UART1_PROTO_OUT_UBX, 1));
#if CONFIG_GNSS_SATELLITES
UBX_FRAME_DEFINE(enable_sat,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_UBX_NAV_SAT_UART1, 1));
#endif

UBX_FRAME_ARRAY_DEFINE(u_blox_m10_init_seq,
	&disable_nmea, &enable_nav, &nav_fix_mode_auto,
	&enable_prot_in_ubx, &enable_prot_out_ubx,
#if CONFIG_GNSS_SATELLITES
	&enable_sat,
#endif
);

MODEM_UBX_MATCH_ARRAY_DEFINE(u_blox_m10_unsol_messages,
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_PVT,
			       gnss_ubx_common_pvt_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_SAT,
			       gnss_ubx_common_satellite_callback),
#endif
);

static int u_blox_m10_init(const struct device *dev)
{
	int err;
	const struct u_blox_iface_config *cfg = dev->config;

	const static struct ubx_frame version_get = UBX_FRAME_GET_INITIALIZER(
						UBX_CLASS_ID_MON, UBX_MSG_ID_MON_VER);
	struct ubx_mon_ver ver;

	err = u_blox_iface_init(dev, u_blox_m10_unsol_messages,
			 ARRAY_SIZE(u_blox_m10_unsol_messages), true);
	if (err < 0) {
		return err;
	}

	err = u_blox_iface_msg_get(dev, &version_get,
				   UBX_FRAME_SZ(version_get.payload_size),
				   (void *)&ver, sizeof(ver));
	if (err != 0) {
		LOG_ERR("Failed to get Modem version info: %d", err);
		return err;
	}
	LOG_INF("SW Version %s, HW Version: %s", ver.sw_ver, ver.hw_ver);

	err = gnss_set_fix_rate(dev, cfg->fix_rate_ms);
	if (err != 0) {
		LOG_ERR("Failed to set GNSS fix-rate: %d", err);
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(u_blox_m10_init_seq); i++) {
		err = u_blox_iface_msg_send(dev,
					u_blox_m10_init_seq[i],
					UBX_FRAME_SZ(u_blox_m10_init_seq[i]->payload_size),
					true);
		if (err < 0) {
			LOG_ERR("Failed to send init sequence. idx: %d, result %d", i, err);
			return err;
		}
	}

	return 0;
}

static int ubx_m10_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	if (fix_interval_ms < 50 || fix_interval_ms > 65535) {
		return -EINVAL;
	}

	struct ubx_cfg_val_u16 rate = {
		.hdr = {
			.ver = UBX_CFG_VAL_VER_SIMPLE,
			.layer = 1,
		},
		.key = UBX_KEY_RATE_MEAS,
		.value = fix_interval_ms,
	};

	return u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG,
					     UBX_MSG_ID_CFG_VAL_SET,
					     (const uint8_t *)&rate, sizeof(rate),
					     true);
}

static int ubx_m10_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	struct ubx_cfg_val_u16 rate;
	int err;

	const static struct ubx_frame get_fix_rate =
		UBX_FRAME_CFG_VAL_GET_INITIALIZER(UBX_KEY_RATE_MEAS);

	err = u_blox_iface_msg_get(dev, &get_fix_rate,
				    UBX_FRAME_SZ(get_fix_rate.payload_size),
				    (void *)&rate, sizeof(rate));

	if (err != 0) {
		LOG_ERR("Failed to retrieve fix rate: %d", err);
		return err;
	}

	*fix_interval_ms = rate.value;

	return 0;
}

/** As this GNSS modem may be used for many applications, the definition of
 * High Dynamics Navigation mode is configurable through Kconfig, in order to
 * maintain a balance between API re-usability and flexibility.
 */
#define GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN							\
	COND_CASE_1(CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_AIRBORNE_1G,			\
		    (UBX_DYN_MODEL_AIRBORNE_1G),						\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_AIRBORNE_2G,			\
		    (UBX_DYN_MODEL_AIRBORNE_2G),						\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_AIRBORNE_4G,			\
		    (UBX_DYN_MODEL_AIRBORNE_4G),						\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_AUTOMOTIVE,			\
		    (UBX_DYN_MODEL_AUTOMOTIVE),							\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_SEA,				\
		    (UBX_DYN_MODEL_SEA),							\
		    CONFIG_GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN_BIKE,				\
		    (UBX_DYN_MODEL_BIKE),							\
		    (UBX_DYN_MODEL_AIRBORNE_2G))

static int ubx_m10_set_navigation_mode(const struct device *dev, enum gnss_navigation_mode mode)
{
	enum ubx_dyn_model nav_model;

	struct ubx_cfg_val_u8 rate = {
		.hdr = {
			.ver = UBX_CFG_VAL_VER_SIMPLE,
			.layer = 1,
		},
		.key = UBX_KEY_NAV_CFG_DYN_MODEL,
	};

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
		nav_model = GNSS_U_BLOX_M10_NAV_MODE_HIGH_DYN;
		break;
	default:
		return -EINVAL;
	}

	rate.value = nav_model;

	return u_blox_iface_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_VAL_SET,
					     (const uint8_t *)&rate, sizeof(rate),
					     true);
}

static int ubx_m10_get_navigation_mode(const struct device *dev, enum gnss_navigation_mode *mode)
{
	int err;
	struct ubx_cfg_val_u8 nav_mode;

	const static struct ubx_frame get_nav_mode =
		UBX_FRAME_CFG_VAL_GET_INITIALIZER(UBX_KEY_NAV_CFG_DYN_MODEL);

	err = u_blox_iface_msg_get(dev, &get_nav_mode,
				   UBX_FRAME_SZ(get_nav_mode.payload_size),
				   (void *)&nav_mode, sizeof(nav_mode));
	if (err != 0) {
		LOG_ERR("Failed to get navigation mode: %d", err);
		return err;
	}

	switch (nav_mode.value) {
	case UBX_DYN_MODEL_STATIONARY:
		*mode = GNSS_NAVIGATION_MODE_ZERO_DYNAMICS;
		break;
	case UBX_DYN_MODEL_PEDESTRIAN:
		*mode = GNSS_NAVIGATION_MODE_LOW_DYNAMICS;
		break;
	case UBX_DYN_MODEL_PORTABLE:
		*mode = GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
		break;
	case UBX_DYN_MODEL_AIRBORNE_1G:
	case UBX_DYN_MODEL_AIRBORNE_2G:
	case UBX_DYN_MODEL_AIRBORNE_4G:
	case UBX_DYN_MODEL_AUTOMOTIVE:
	case UBX_DYN_MODEL_SEA:
	case UBX_DYN_MODEL_BIKE:
		LOG_WRN("Returning generic HIGH_DYNAMICS nav mode. "
			"UBX DYN Model: %d was set", nav_mode.value);
		*mode = GNSS_NAVIGATION_MODE_HIGH_DYNAMICS;
		break;
	default:
		return -EIO;
	}

	return err;
}

static int ubx_m10_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	return -ENOTSUP;
}

static void transform_ubx_selection_to_gnss_systems(uint8_t selection, gnss_systems_t *systems)
{
	*systems = 0;
	*systems |= (selection & UBX_GNSS_SELECTION_GPS) ?
			GNSS_SYSTEM_GPS : 0;
	*systems |= (selection & UBX_GNSS_SELECTION_GLONASS) ?
			GNSS_SYSTEM_GLONASS : 0;
	*systems |= (selection & UBX_GNSS_SELECTION_BEIDOU) ?
			GNSS_SYSTEM_BEIDOU : 0;
	*systems |= (selection & UBX_GNSS_SELECTION_GALILEO) ?
			GNSS_SYSTEM_GALILEO : 0;
}

static int ubx_m10_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
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

	transform_ubx_selection_to_gnss_systems(gnss_selection.selection.enabled, systems);
	return 0;
}

static int ubx_m10_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	static const struct ubx_frame get_support_systems = UBX_FRAME_GET_INITIALIZER(
									UBX_CLASS_ID_MON,
									UBX_MSG_ID_MON_GNSS);
	struct ubx_mon_gnss gnss_selection;
	int err;

	err = u_blox_iface_msg_get(dev, &get_support_systems,
				   UBX_FRAME_SZ(get_support_systems.payload_size),
				   (void *)&gnss_selection, sizeof(gnss_selection));
	if (err != 0) {
		return err;
	}

	transform_ubx_selection_to_gnss_systems(gnss_selection.selection.supported, systems);
	return 0;
}

static DEVICE_API(gnss, ublox_m10_driver_api) = {
	.set_fix_rate = ubx_m10_set_fix_rate,
	.get_fix_rate = ubx_m10_get_fix_rate,
	.set_navigation_mode = ubx_m10_set_navigation_mode,
	.get_navigation_mode = ubx_m10_get_navigation_mode,
	.set_enabled_systems = ubx_m10_set_enabled_systems,
	.get_enabled_systems = ubx_m10_get_enabled_systems,
	.get_supported_systems = ubx_m10_get_supported_systems,
};

#define UBX_M10(inst)										\
												\
	static const struct u_blox_iface_config u_blox_m10_cfg_##inst = {			\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),			\
		.fix_rate_ms = DT_INST_PROP(inst, fix_rate),					\
		.baudrate = {									\
			.initial = DT_INST_PROP(inst, initial_baudrate),			\
			.desired = DT_PROP(DT_INST_BUS(inst), current_speed),			\
		},										\
	};											\
												\
	static struct u_blox_iface_data u_blox_m10_data_##inst;					\
												\
	DEVICE_DT_INST_DEFINE(inst,								\
			      u_blox_m10_init,							\
			      NULL,								\
			      &u_blox_m10_data_##inst,						\
			      &u_blox_m10_cfg_##inst,						\
			      POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY,				\
			      &ublox_m10_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UBX_M10)
