/*
 * Copyright (c) 2024 NXP
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT u_blox_m8

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>

#include <zephyr/modem/ubx.h>
#include <zephyr/modem/backend/uart.h>

#include "gnss_ubx_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ubx_m8, CONFIG_GNSS_LOG_LEVEL);

struct ubx_m8_config {
	const struct device *bus;
	uint16_t fix_rate_ms;
	struct {
		uint32_t initial;
		uint32_t desired;
	} baudrate;
};

struct ubx_m8_data {
	struct gnss_ubx_common_data common_data;
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
	struct gnss_satellite satellites[CONFIG_GNSS_U_BLOX_M8_SATELLITES_COUNT];
#endif
};

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

static int ubx_m8_msg_get(const struct device *dev, const struct ubx_frame *req,
			  size_t len, void *rsp, size_t min_rsp_size)
{
	struct ubx_m8_data *data = dev->data;
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
	data->script.inst.request.buf = req;
	data->script.inst.request.len = len;

	err = modem_ubx_run_script(&data->ubx.inst, &data->script.inst);
	if (err != 0 || (data->script.inst.response.buf_len < UBX_FRAME_SZ(min_rsp_size))) {
		return -EIO;
	}

	memcpy(rsp, rsp_frame->payload_and_checksum, min_rsp_size);

	(void)k_mutex_unlock(&data->script.lock);

	return 0;
}

static int ubx_m8_msg_send(const struct device *dev, const struct ubx_frame *req,
			  size_t len, bool wait_for_ack)
{
	struct ubx_m8_data *data = dev->data;
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
	data->script.inst.request.buf = req;
	data->script.inst.request.len = len;

	err = modem_ubx_run_script(&data->ubx.inst, &data->script.inst);

	(void)k_mutex_unlock(&data->script.lock);

	return err;
}

static int ubx_m8_msg_payload_send(const struct device *dev, uint8_t class, uint8_t id,
				   const uint8_t *payload, size_t payload_size, bool wait_for_ack)
{
	struct ubx_m8_data *data = dev->data;
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
		err = ubx_m8_msg_send(dev, frame, err, wait_for_ack);
	}

	(void)k_mutex_unlock(&data->script.lock);

	return err;
}

static inline int init_modem(const struct device *dev)
{
	int err;
	struct ubx_m8_data *data = dev->data;
	const struct ubx_m8_config *cfg = dev->config;

	const struct modem_ubx_config ubx_config = {
		.user_data = (void *)&data->common_data,
		.receive_buf = data->ubx.receive_buf,
		.receive_buf_size = sizeof(data->ubx.receive_buf),
		.unsol_matches = {
			.array = u_blox_m8_unsol_messages,
			.size = ARRAY_SIZE(u_blox_m8_unsol_messages),
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

static inline int reattach_modem(const struct device *dev)
{
	int err;
	struct ubx_m8_data *data = dev->data;

	(void)modem_ubx_release(&data->ubx.inst);
	(void)modem_pipe_close(data->backend.pipe, K_SECONDS(1));

	err = modem_pipe_open(data->backend.pipe, K_SECONDS(1));
	if (err != 0) {
		LOG_ERR("Failed to re-open modem pipe: %d", err);
		return err;
	}

	err = modem_ubx_attach(&data->ubx.inst, data->backend.pipe);
	if (err != 0) {
		LOG_ERR("Failed to re-attach modem pipe to UBX inst: %d", err);
	}

	return 0;
}

static inline int configure_baudrate(const struct device *dev)
{
	int err = 0;
	const struct ubx_m8_config *cfg = dev->config;
	struct uart_config uart_cfg;

	err = uart_config_get(cfg->bus, &uart_cfg);
	if (err < 0) {
		LOG_ERR("Failed to get UART config: %d", err);
		return err;
	}

	uint32_t desired_baudrate = cfg->baudrate.desired;
	uint32_t initial_baudrate = cfg->baudrate.initial;

	uart_cfg.baudrate = initial_baudrate;
	err = uart_configure(cfg->bus, &uart_cfg);
	if (err < 0) {
		LOG_ERR("Failed to configure UART: %d", err);
	}

	struct ubx_cfg_prt port_config = {
		.port_id = UBX_CFG_PORT_ID_UART,
		.baudrate = desired_baudrate,
		.mode = UBX_CFG_PRT_MODE_CHAR_LEN(UBX_CFG_PRT_PORT_MODE_CHAR_LEN_8) |
			UBX_CFG_PRT_MODE_PARITY(UBX_CFG_PRT_PORT_MODE_PARITY_NONE) |
			UBX_CFG_PRT_MODE_STOP_BITS(UBX_CFG_PRT_PORT_MODE_STOP_BITS_1),
		.in_proto_mask = UBX_CFG_PRT_PROTO_MASK_UBX,
		.out_proto_mask = UBX_CFG_PRT_PROTO_MASK_UBX,
	};
	(void)ubx_m8_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_PRT,
				      (const uint8_t *)&port_config,
				      sizeof(port_config), true);

	uart_cfg.baudrate = desired_baudrate;

	err = uart_configure(cfg->bus, &uart_cfg);
	if (err < 0) {
		LOG_ERR("Failed to configure UART: %d", err);
	}

	return err;
}

static inline int init_match(const struct device *dev)
{
	struct ubx_m8_data *data = dev->data;
	struct gnss_ubx_common_config match_config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = {
			.buf = data->satellites,
			.size = ARRAY_SIZE(data->satellites),
		},
#endif
	};

	gnss_ubx_common_init(&data->common_data, &match_config);

	return 0;
}

static int ubx_m8_init(const struct device *dev)
{
	int err = 0;
	const struct ubx_m8_config *cfg = dev->config;

	(void)init_match(dev);

	err = init_modem(dev);
	if (err < 0) {
		LOG_ERR("Failed to initialize modem: %d", err);
	}

	err = configure_baudrate(dev);
	if (err < 0) {
		LOG_ERR("Failed to configure baud-rate: %d", err);
		return err;
	}

	err = reattach_modem(dev);
	if (err < 0) {
		LOG_ERR("Failed to re-attach modem: %d", err);
		return err;
	}

	const static struct ubx_frame version_get = UBX_FRAME_GET_INITIALIZER(
						UBX_CLASS_ID_MON,
						UBX_MSG_ID_MON_VER);
	struct ubx_mon_ver ver;

	err = ubx_m8_msg_get(dev, &version_get,
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

	err = ubx_m8_msg_send(dev, &stop_gnss, UBX_FRAME_SZ(stop_gnss.payload_size), false);
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
		err = ubx_m8_msg_send(dev,
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

	err = ubx_m8_msg_send(dev, &start_gnss, UBX_FRAME_SZ(start_gnss.payload_size), false);
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

	return ubx_m8_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_RATE,
				       (const uint8_t *)&rate, sizeof(rate),
				       true);
}

static int ubx_m8_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	struct ubx_cfg_rate rate;
	int err;

	const static struct ubx_frame get_fix_rate = UBX_FRAME_GET_INITIALIZER(UBX_CLASS_ID_CFG,
									       UBX_MSG_ID_CFG_RATE);

	err = ubx_m8_msg_get(dev, &get_fix_rate,
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

	return ubx_m8_msg_payload_send(dev, UBX_CLASS_ID_CFG, UBX_MSG_ID_CFG_NAV5,
				       (const uint8_t *)&nav_mode, sizeof(nav_mode),
				       true);
}

static int ubx_m8_get_navigation_mode(const struct device *dev, enum gnss_navigation_mode *mode)
{
	struct ubx_cfg_nav5 nav_mode;
	int err;

	const static struct ubx_frame get_nav_mode = UBX_FRAME_GET_INITIALIZER(UBX_CLASS_ID_CFG,
									       UBX_MSG_ID_CFG_NAV5);

	err = ubx_m8_msg_get(dev, &get_nav_mode,
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

	err = ubx_m8_msg_get(dev, &get_enabled_systems,
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

	err = ubx_m8_msg_get(dev, &get_enabled_systems,
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
	static const struct ubx_m8_config ubx_m8_cfg_##inst = {					   \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),					   \
		.baudrate = {									   \
			.initial = DT_INST_PROP(inst, initial_baudrate),			   \
			.desired = DT_PROP(DT_INST_BUS(inst), current_speed),			   \
		},										   \
		.fix_rate_ms = DT_INST_PROP(inst, fix_rate),					   \
	};											   \
												   \
	static struct ubx_m8_data ubx_m8_data_##inst;						   \
												   \
	DEVICE_DT_INST_DEFINE(inst,								   \
			      ubx_m8_init,							   \
			      NULL,								   \
			      &ubx_m8_data_##inst,						   \
			      &ubx_m8_cfg_##inst,						   \
			      POST_KERNEL,							   \
			      CONFIG_GNSS_INIT_PRIORITY,					   \
			      &gnss_api);

DT_INST_FOREACH_STATUS_OKAY(UBX_M8)
