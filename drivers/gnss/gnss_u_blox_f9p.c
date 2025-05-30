/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT u_blox_f9p

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>

#include <zephyr/modem/ubx.h>
#include <zephyr/modem/ubx/protocol.h>
#include <zephyr/modem/ubx/keys.h>
#include <zephyr/modem/backend/uart.h>

#include <zephyr/rtk/rtk.h>

#include "gnss_ubx_match.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ubx_f9p, CONFIG_GNSS_LOG_LEVEL);

struct ubx_f9p_config {
	const struct device *bus;
	uint16_t fix_rate_ms;
	uint32_t unused;
};

struct ubx_f9p_data {
	struct gnss_ubx_match_data match_data;
	struct {
		struct modem_pipe *pipe;
		struct modem_backend_uart uart_backend;
		uint8_t receive_buf[1024];
		uint8_t transmit_buf[256];
	} backend;
	struct {
		struct modem_ubx inst;
		uint8_t receive_buf[1024];
	} ubx;
	struct {
		struct modem_ubx_script inst;
		uint8_t response_buf[512];
		uint8_t request_buf[256];
		struct k_mutex lock;
	} script;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_U_BLOX_F9P_SATELLITES_COUNT];
#endif
};

UBX_FRAME_DEFINE(disable_gga,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_GGA_UART1, 0));
UBX_FRAME_DEFINE(disable_rmc,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_RMC_UART1, 0));
UBX_FRAME_DEFINE(disable_gsv,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_GSV_UART1, 0));
UBX_FRAME_DEFINE(disable_dtm,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_DTM_UART1, 0));
UBX_FRAME_DEFINE(disable_gbs,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_GBS_UART1, 0));
UBX_FRAME_DEFINE(disable_gll,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_GLL_UART1, 0));
UBX_FRAME_DEFINE(disable_gns,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_GNS_UART1, 0));
UBX_FRAME_DEFINE(disable_grs,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_GRS_UART1, 0));
UBX_FRAME_DEFINE(disable_gsa,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_GSA_UART1, 0));
UBX_FRAME_DEFINE(disable_gst,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_GST_UART1, 0));
UBX_FRAME_DEFINE(disable_vlw,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_VLW_UART1, 0));
UBX_FRAME_DEFINE(disable_vtg,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_VTG_UART1, 0));
UBX_FRAME_DEFINE(disable_zda,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_NMEA_ZDA_UART1, 0));
UBX_FRAME_DEFINE(enable_nav,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_UBX_NAV_PVT_UART1, 1));
UBX_FRAME_DEFINE(nav_fix_mode_auto,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_NAV_CFG_FIX_MODE, UBX_FIX_MODE_AUTO));
UBX_FRAME_DEFINE(enable_prot_in_ubx,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_UART1_PROTO_IN_UBX, 1));
UBX_FRAME_DEFINE(enable_prot_in_rtcm3,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_UART1_PROTO_IN_RTCM3X, 1));
UBX_FRAME_DEFINE(enable_prot_out_ubx,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_UART1_PROTO_OUT_UBX, 1));
UBX_FRAME_DEFINE(disable_prot_out_rtcm3,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_UART1_PROTO_OUT_RTCM3X, 0));
UBX_FRAME_DEFINE(enable_ubx_rtcm_rsp,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_UBX_RXM_RTCM_UART1, 1));
UBX_FRAME_DEFINE(set_rtk_fix_mode,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_NAV_HP_CFG_GNSS_MODE,
					     UBX_NAV_HP_DGNSS_MODE_RTK_FIXED));
#if CONFIG_GNSS_SATELLITES
UBX_FRAME_DEFINE(enable_sat,
	UBX_FRAME_CFG_VAL_SET_U8_INITIALIZER(UBX_KEY_MSG_OUT_UBX_NAV_SAT_UART1, 1));
#endif

UBX_FRAME_ARRAY_DEFINE(u_blox_f9p_init_seq,
	&disable_gga, &disable_rmc, &disable_gsv, &disable_dtm, &disable_gbs,
	&disable_gll, &disable_gns, &disable_grs, &disable_gsa, &disable_gst,
	&disable_vlw, &disable_vtg, &disable_zda, &enable_nav, &nav_fix_mode_auto,
	&enable_prot_in_ubx, &enable_prot_in_rtcm3, &enable_prot_out_ubx, &disable_prot_out_rtcm3,
	&enable_ubx_rtcm_rsp, &set_rtk_fix_mode,
#if CONFIG_GNSS_SATELLITES
	&enable_sat,
#endif
);

MODEM_UBX_MATCH_ARRAY_DEFINE(u_blox_f9p_unsol_messages,
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_PVT,
			       gnss_ubx_match_pvt_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_NAV, UBX_MSG_ID_NAV_SAT,
			       gnss_ubx_match_satellite_callback),
#endif
);

static int ubx_f9p_msg_get(const struct device *dev, const struct ubx_frame *req,
			  size_t len, void *rsp, size_t min_rsp_size)
{
	struct ubx_f9p_data *data = dev->data;
	struct ubx_frame *rsp_frame = (struct ubx_frame *)data->script.inst.response.buf;
	int err;

	err = k_mutex_lock(&data->script.lock, K_SECONDS(3));
	if (err != 0) {
		LOG_ERR("Failed to take script lock: %d", err);
		return err;
	}

	data->script.inst.timeout = K_SECONDS(3);
	data->script.inst.retry_count = 2;
	data->script.inst.match.filter.class = req->class;
	data->script.inst.match.filter.id = req->id;
	data->script.inst.request.buf = (const uint8_t *)req;
	data->script.inst.request.len = len;

	err = modem_ubx_run_script(&data->ubx.inst, &data->script.inst);
	if (err != 0 || (data->script.inst.response.buf_len < UBX_FRM_SZ(min_rsp_size))) {
		return -EIO;
	}

	memcpy(rsp, rsp_frame->payload_and_checksum, min_rsp_size);

	(void)k_mutex_unlock(&data->script.lock);

	return 0;
}

static int ubx_f9p_msg_send(const struct device *dev, const struct ubx_frame *req,
			  size_t len, bool wait_for_ack)
{
	struct ubx_f9p_data *data = dev->data;
	int err;

	err = k_mutex_lock(&data->script.lock, K_SECONDS(3));
	if (err != 0) {
		LOG_ERR("Failed to take script lock: %d", err);
		return err;
	}

	data->script.inst.timeout = K_SECONDS(3);
	data->script.inst.retry_count = wait_for_ack ? 2 : 0;
	data->script.inst.match.filter.class = wait_for_ack ? UBX_CLASS_ID_ACK : 0;
	data->script.inst.match.filter.id = UBX_MSG_ID_ACK;
	data->script.inst.request.buf = (const uint8_t *)req;
	data->script.inst.request.len = len;

	err = modem_ubx_run_script(&data->ubx.inst, &data->script.inst);

	(void)k_mutex_unlock(&data->script.lock);

	return err;
}

static int ubx_f9p_msg_payload_send(const struct device *dev, uint8_t class, uint8_t id,
				   const uint8_t *payload, size_t payload_size, bool wait_for_ack)
{
	struct ubx_f9p_data *data = dev->data;
	struct ubx_frame *frame = (struct ubx_frame *)data->script.request_buf;
	int err;

	err = k_mutex_lock(&data->script.lock, K_SECONDS(3));
	if (err != 0) {
		LOG_ERR("Failed to take script lock: %d", err);
		return err;
	}

	err = ubx_frame_encode(class, id, payload, payload_size,
			       (uint8_t *)frame, sizeof(data->script.request_buf));
	if (err > 0) {
		err = ubx_f9p_msg_send(dev, frame, err, wait_for_ack);
	}

	(void)k_mutex_unlock(&data->script.lock);

	return err;
}

#if CONFIG_GNSS_U_BLOX_F9P_RTK

static void f9p_rtk_data_cb(const struct device *dev, const struct rtk_data *rtk_data)
{
	/** In this case, we forward the frame directly to the modem. It can either use
	 * it or not depending on the RTCM3 message type and its alignment with what the
	 * GNSS modem has observed.
	 */
	(void)ubx_f9p_msg_send(dev, (const void *)rtk_data->data, rtk_data->len, false);
}

#endif /* CONFIG_GNSS_U_BLOX_F9P_RTK */

static inline int init_modem(const struct device *dev)
{
	int err;
	struct ubx_f9p_data *data = dev->data;
	const struct ubx_f9p_config *cfg = dev->config;

	const struct modem_ubx_config ubx_config = {
		.user_data = (void *)&data->match_data,
		.receive_buf = data->ubx.receive_buf,
		.receive_buf_size = sizeof(data->ubx.receive_buf),
		.unsol_matches = {
			.array = u_blox_f9p_unsol_messages,
			.size = ARRAY_SIZE(u_blox_f9p_unsol_messages),
		},
	};

	(void)modem_ubx_init(&data->ubx.inst, &ubx_config);

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = cfg->bus,
		.receive_buf = data->backend.receive_buf,
		.receive_buf_size = sizeof(data->backend.receive_buf),
		.transmit_buf = data->backend.transmit_buf,
		.transmit_buf_size = sizeof(data->backend.transmit_buf),
	};

	data->backend.pipe = modem_backend_uart_init(&data->backend.uart_backend,
						     &uart_backend_config);
	err = modem_pipe_open(data->backend.pipe, K_SECONDS(1));
	if (err != 0) {
		LOG_ERR("Failed to open Modem pipe: %d", err);
		return err;
	}

	err = modem_ubx_attach(&data->ubx.inst, data->backend.pipe);
	if (err != 0) {
		LOG_ERR("Failed to attach UBX inst to modem pipe: %d", err);
		return err;
	}

	(void)k_mutex_init(&data->script.lock);

	data->script.inst.response.buf = data->script.response_buf;
	data->script.inst.response.buf_len = sizeof(data->script.response_buf);

	return err;
}

static inline int init_match(const struct device *dev)
{
	struct ubx_f9p_data *data = dev->data;
	struct gnss_ubx_match_config match_config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = {
			.buf = data->satellites,
			.size = ARRAY_SIZE(data->satellites),
		},
#endif
	};

	gnss_ubx_match_init(&data->match_data, &match_config);

	return 0;
}

static int ublox_f9p_init(const struct device *dev)
{
	int err = 0;
	const struct ubx_f9p_config *cfg = dev->config;

	(void)init_match(dev);

	err = init_modem(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize modem: %d", err);
	}

	const static struct ubx_frame version_get = UBX_FRAME_GET_INITIALIZER(
						UBX_CLASS_ID_MON,
						UBX_MSG_ID_MON_VER);
	struct ubx_mon_ver ver;

	err = ubx_f9p_msg_get(dev, &version_get,
			      UBX_FRM_SZ(version_get.payload_size),
			      (void *)&ver, sizeof(ver));
	if (err != 0) {
		LOG_ERR("Failed to get Modem Version info: %d", err);
		return err;
	}
	LOG_INF("SW Version: %s, HW Version: %s", ver.sw_ver, ver.hw_ver);

	err = gnss_set_fix_rate(dev, cfg->fix_rate_ms);
	if (err != 0) {
		LOG_ERR("Failed to set fix-rate: %d", err);
		return err;
	}

	for (size_t i = 0 ; i < ARRAY_SIZE(u_blox_f9p_init_seq) ; i++) {
		err = ubx_f9p_msg_send(dev,
				       u_blox_f9p_init_seq[i],
				       UBX_FRM_SZ(u_blox_f9p_init_seq[i]->payload_size),
				       true);
		if (err < 0) {
			LOG_ERR("Failed to send init sequence - idx: %d, result: %d", i, err);
			return err;
		}
	}

	return 0;
}

static int ubx_f9p_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	if (fix_interval_ms < 50 || fix_interval_ms > 65535) {
		return -EINVAL;
	}

	struct ubx_cfg_val_u16 rate = {
		.hdr = {
			.ver = UBX_CFG_VAL_VER_SIMPLE,
			.layer.ram = 1,
		},
		.key = UBX_KEY_RATE_MEAS,
		.value = fix_interval_ms,
	};

	return ubx_f9p_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_VAL_SET,
					(const uint8_t *)&rate, sizeof(rate),
					true);
}

static int ubx_f9p_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	int err;
	struct ubx_cfg_val_u16 rate;

	const static struct ubx_frame get_fix_rate =
		UBX_FRAME_CFG_VAL_GET_INITIALIZER(UBX_KEY_RATE_MEAS);

	err = ubx_f9p_msg_get(dev, &get_fix_rate,
			     UBX_FRM_SZ(get_fix_rate.payload_size),
			     (void *)&rate, sizeof(rate));
	if (err == 0) {
		*fix_interval_ms = rate.value;
	}

	return err;
}

static int ubx_f9p_set_navigation_mode(const struct device *dev, enum gnss_navigation_mode mode)
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

	struct ubx_cfg_val_u8 rate = {
		.hdr = {
			.ver = UBX_CFG_VAL_VER_SIMPLE,
			.layer.ram = 1,
		},
		.key = UBX_KEY_NAV_CFG_DYN_MODEL,
		.value = nav_model,
	};

	return ubx_f9p_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_VAL_SET,
					(const uint8_t *)&rate, sizeof(rate),
					true);
}

static int ubx_f9p_get_navigation_mode(const struct device *dev, enum gnss_navigation_mode *mode)
{
	int err;
	struct ubx_cfg_val_u8 nav_mode;

	const static struct ubx_frame get_nav_mode =
		UBX_FRAME_CFG_VAL_GET_INITIALIZER(UBX_KEY_NAV_CFG_DYN_MODEL);

	err = ubx_f9p_msg_get(dev, &get_nav_mode,
			     UBX_FRM_SZ(get_nav_mode.payload_size),
			     (void *)&nav_mode, sizeof(nav_mode));
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
	case UBX_DYN_MODEL_AIRBORNE_2G:
		*mode = GNSS_NAVIGATION_MODE_HIGH_DYNAMICS;
		break;
	default:
		return -EIO;
	}

	return err;
}

static int ubx_f9p_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	return -ENOTSUP;
}

static int ubx_f9p_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	static const struct ubx_frame get_enabled_systems = UBX_FRAME_GET_INITIALIZER(
									UBX_CLASS_ID_MON,
									UBX_MSG_ID_MON_GNSS);
	struct ubx_mon_gnss gnss_selection;
	int err;

	err = ubx_f9p_msg_get(dev, &get_enabled_systems,
			     UBX_FRM_SZ(get_enabled_systems.payload_size),
			     (void *)&gnss_selection, sizeof(gnss_selection));
	if (err != 0) {
		return err;
	}

	*systems = 0;
	*systems |= gnss_selection.enabled.gps ? GNSS_SYSTEM_GPS : 0;
	*systems |= gnss_selection.enabled.glonass ? GNSS_SYSTEM_GLONASS : 0;
	*systems |= gnss_selection.enabled.beidou ? GNSS_SYSTEM_BEIDOU : 0;
	*systems |= gnss_selection.enabled.galileo ? GNSS_SYSTEM_GALILEO : 0;

	return 0;
}

static int ubx_f9p_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	static const struct ubx_frame get_enabled_systems = UBX_FRAME_GET_INITIALIZER(
									UBX_CLASS_ID_MON,
									UBX_MSG_ID_MON_GNSS);
	struct ubx_mon_gnss gnss_selection;
	int err;

	err = ubx_f9p_msg_get(dev, &get_enabled_systems,
			     UBX_FRM_SZ(get_enabled_systems.payload_size),
			     (void *)&gnss_selection, sizeof(gnss_selection));
	if (err != 0) {
		return err;
	}

	*systems = 0;
	*systems |= gnss_selection.supported.gps ? GNSS_SYSTEM_GPS : 0;
	*systems |= gnss_selection.supported.glonass ? GNSS_SYSTEM_GLONASS : 0;
	*systems |= gnss_selection.supported.beidou ? GNSS_SYSTEM_BEIDOU : 0;
	*systems |= gnss_selection.supported.galileo ? GNSS_SYSTEM_GALILEO : 0;

	return 0;
}


static DEVICE_API(gnss, ublox_f9p_driver_api) = {
	.set_fix_rate = ubx_f9p_set_fix_rate,
	.get_fix_rate = ubx_f9p_get_fix_rate,
	.set_navigation_mode = ubx_f9p_set_navigation_mode,
	.get_navigation_mode = ubx_f9p_get_navigation_mode,
	.set_enabled_systems = ubx_f9p_set_enabled_systems,
	.get_enabled_systems = ubx_f9p_get_enabled_systems,
	.get_supported_systems = ubx_f9p_get_supported_systems,
};


#define UBX_F9P(inst)										   \
												   \
	BUILD_ASSERT((DT_INST_PROP(inst, fix_rate) >= 50) &&					   \
		     (DT_INST_PROP(inst, fix_rate) < 65536),					   \
		     "Invalid fix-rate. Please set it higher than 50-ms"			   \
		     " and must fit in 16-bits.");						   \
												   \
	static const struct ubx_f9p_config ubx_f9p_cfg_##inst = {				   \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),					   \
		.fix_rate_ms = DT_INST_PROP(inst, fix_rate),					   \
	};											   \
												   \
	static struct ubx_f9p_data ubx_f9p_data_##inst;						   \
												   \
	COND_CODE_1(CONFIG_GNSS_U_BLOX_F9P_RTK,							   \
		    (RTK_DATA_CALLBACK_DEFINE(DEVICE_DT_INST_GET(inst), f9p_rtk_data_cb)),	   \
		    ());									   \
												   \
	DEVICE_DT_INST_DEFINE(inst,								   \
			      ublox_f9p_init,							   \
			      NULL,								   \
			      &ubx_f9p_data_##inst,						   \
			      &ubx_f9p_cfg_##inst,						   \
			      POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY,				   \
			      &ublox_f9p_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UBX_F9P)
