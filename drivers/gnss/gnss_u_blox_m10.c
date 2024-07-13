/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/ubx.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdlib.h>

#include "gnss_nmea0183.h"
#include "gnss_nmea0183_match.h"
#include "gnss_parse.h"

#include "gnss_u_blox_protocol/gnss_u_blox_protocol.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ubx_m10, CONFIG_GNSS_LOG_LEVEL);

#define DT_DRV_COMPAT			u_blox_m10

#define UART_RECV_BUF_SZ		128
#define UART_TRNF_BUF_SZ		128

#define CHAT_RECV_BUF_SZ		256
#define CHAT_ARGV_SZ			32

#define UBX_RECV_BUF_SZ			UBX_FRM_SZ_MAX
#define UBX_TRNS_BUF_SZ			UBX_FRM_SZ_MAX
#define UBX_WORK_BUF_SZ			UBX_FRM_SZ_MAX

#define UBX_FRM_BUF_SZ			UBX_FRM_SZ_MAX

#define MODEM_UBX_SCRIPT_TIMEOUT_MS	1000
#define UBX_M10_SCRIPT_RETRY_DEFAULT	10

#define UBX_M10_GNSS_SYS_CNT		8
#define UBX_M10_GNSS_SUPP_SYS_CNT	6
/* The datasheet of the device doesn't specify boot time. But 1 sec helped significantly. */
#define UBX_M10_BOOT_TIME_MS		1000

struct ubx_m10_config {
	const struct device *uart;
	const uint32_t uart_baudrate;
};

struct ubx_m10_data {
	struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_U_BLOX_M10_SATELLITES_COUNT];
#endif

	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[UART_RECV_BUF_SZ];
	uint8_t uart_backend_transmit_buf[UART_TRNF_BUF_SZ];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[CHAT_RECV_BUF_SZ];
	uint8_t *chat_argv[CHAT_ARGV_SZ];

	/* Modem ubx */
	struct modem_ubx ubx;
	uint8_t ubx_receive_buf[UBX_RECV_BUF_SZ];
	uint8_t ubx_work_buf[UBX_WORK_BUF_SZ];

	/* Modem ubx script */
	struct modem_ubx_script script;
	uint8_t request_buf[UBX_FRM_BUF_SZ];
	uint8_t response_buf[UBX_FRM_BUF_SZ];
	uint8_t match_buf[UBX_FRM_BUF_SZ];

	struct k_spinlock lock;
};

MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
	MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
	MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);

static int ubx_m10_resume(const struct device *dev)
{
	struct ubx_m10_data *data = dev->data;
	int ret;

	ret = modem_pipe_open(data->uart_pipe);
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->uart_pipe);
	if (ret < 0) {
		(void)modem_pipe_close(data->uart_pipe);
		return ret;
	}

	return ret;
}

static int ubx_m10_turn_off(const struct device *dev)
{
	struct ubx_m10_data *data = dev->data;

	return modem_pipe_close(data->uart_pipe);
}

static int ubx_m10_init_nmea0183_match(const struct device *dev)
{
	struct ubx_m10_data *data = dev->data;

	const struct gnss_nmea0183_match_config match_config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = data->satellites,
		.satellites_size = ARRAY_SIZE(data->satellites),
#endif
	};

	return gnss_nmea0183_match_init(&data->match_data, &match_config);
}

static void ubx_m10_init_pipe(const struct device *dev)
{
	const struct ubx_m10_config *cfg = dev->config;
	struct ubx_m10_data *data = dev->data;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = cfg->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = sizeof(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

static uint8_t ubx_m10_char_delimiter[] = {'\r', '\n'};

static int ubx_m10_init_chat(const struct device *dev)
{
	struct ubx_m10_data *data = dev->data;

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_receive_buf,
		.receive_buf_size = sizeof(data->chat_receive_buf),
		.delimiter = ubx_m10_char_delimiter,
		.delimiter_size = ARRAY_SIZE(ubx_m10_char_delimiter),
		.filter = NULL,
		.filter_size = 0,
		.argv = data->chat_argv,
		.argv_size = ARRAY_SIZE(data->chat_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	return modem_chat_init(&data->chat, &chat_config);
}

static int ubx_m10_init_ubx(const struct device *dev)
{
	struct ubx_m10_data *data = dev->data;

	const struct modem_ubx_config ubx_config = {
		.user_data = data,
		.receive_buf = data->ubx_receive_buf,
		.receive_buf_size = sizeof(data->ubx_receive_buf),
		.work_buf = data->ubx_work_buf,
		.work_buf_size = sizeof(data->ubx_work_buf),
	};

	return modem_ubx_init(&data->ubx, &ubx_config);
}

/**
 * @brief Changes modem module (chat or ubx) attached to the uart pipe.
 * @param dev Dev instance
 * @param change_from_to 0 for changing from "chat" to "ubx", 1 for changing from "ubx" to "chat"
 * @returns 0 if successful
 * @returns negative errno code if failure
 */
static int ubx_m10_modem_module_change(const struct device *dev, bool change_from_to)
{
	struct ubx_m10_data *data = dev->data;
	int ret;

	if (change_from_to == 0) {
		modem_chat_release(&data->chat);
		ret = modem_ubx_attach(&data->ubx, data->uart_pipe);
	} else { /* change_from_to == 1 */
		modem_ubx_release(&data->ubx);
		ret = modem_chat_attach(&data->chat, data->uart_pipe);
	}

	if (ret < 0) {
		(void)modem_pipe_close(data->uart_pipe);
	}

	return ret;
}

static int ubx_m10_modem_ubx_run_script(const struct device *dev,
					struct modem_ubx_script *modem_ubx_script_tx)
{
	struct ubx_m10_data *data = dev->data;
	int ret;

	ret = ubx_m10_modem_module_change(dev, 0);
	if (ret < 0) {
		goto reset_modem_module;
	}

	ret = modem_ubx_run_script(&data->ubx, modem_ubx_script_tx);
	if (ret < 0) {
		goto reset_modem_module;
	}

reset_modem_module:
	ret |= ubx_m10_modem_module_change(dev, 1);

	return ret;
}

static void ubx_m10_modem_ubx_script_fill(const struct device *dev)
{
	struct ubx_m10_data *data = dev->data;

	data->script.request = (struct ubx_frame *)data->request_buf;
	data->script.response = (struct ubx_frame *)data->response_buf;
	data->script.match = (struct ubx_frame *)data->match_buf;
	data->script.retry_count = UBX_M10_SCRIPT_RETRY_DEFAULT;
	data->script.timeout = K_MSEC(MODEM_UBX_SCRIPT_TIMEOUT_MS);
}

static int ubx_m10_modem_ubx_script_init(const struct device *dev, void *payload, uint16_t payld_sz,
					 enum ubx_msg_class msg_cls, enum ubx_config_message msg_id)
{
	int ret;
	struct ubx_m10_data *data = dev->data;
	struct ubx_cfg_ack_payload match_payload = {
		.message_class = msg_cls,
		.message_id = msg_id,
	};

	ubx_m10_modem_ubx_script_fill(dev);

	ret = ubx_create_and_validate_frame(data->match_buf, sizeof(data->match_buf), UBX_CLASS_ACK,
					    UBX_ACK_ACK, &match_payload, UBX_CFG_ACK_PAYLOAD_SZ);
	if (ret < 0) {
		return ret;
	}

	ret = ubx_create_and_validate_frame(data->request_buf, sizeof(data->request_buf), msg_cls,
					    msg_id, payload, payld_sz);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int ubx_m10_ubx_cfg_rate(const struct device *dev)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	struct ubx_cfg_rate_payload payload;

	key = k_spin_lock(&data->lock);

	ubx_cfg_rate_payload_default(&payload);

	ret = ubx_m10_modem_ubx_script_init(dev, &payload, UBX_CFG_RATE_PAYLOAD_SZ, UBX_CLASS_CFG,
					    UBX_CFG_RATE);
	if (ret < 0) {
		goto unlock;
	}

	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_ubx_cfg_prt_set(const struct device *dev, uint32_t target_baudrate,
				   uint8_t retry)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	struct ubx_cfg_prt_set_payload payload;

	key = k_spin_lock(&data->lock);

	ubx_cfg_prt_set_payload_default(&payload);
	payload.baudrate = target_baudrate;

	ret = ubx_m10_modem_ubx_script_init(dev, &payload, UBX_CFG_PRT_SET_PAYLOAD_SZ,
					    UBX_CLASS_CFG, UBX_CFG_PRT);
	if (ret < 0) {
		goto unlock;
	}

	data->script.retry_count = retry;
	/* Returns failure if "target_baudrate" is different than device's currently set baudrate,
	 * because the device will change its baudrate and respond with UBX-ACK with new baudrate,
	 * which we will miss. Hence, we need to change uart's baudrate after sending the frame
	 * (in order to receive response as well), which we are not doing right now.
	 */
	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_ubx_cfg_rst(const struct device *dev, uint8_t reset_mode)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	struct ubx_cfg_rst_payload payload;

	key = k_spin_lock(&data->lock);

	ubx_cfg_rst_payload_default(&payload);

	payload.nav_bbr_mask = UBX_CFG_RST_NAV_BBR_MASK_HOT_START;
	payload.reset_mode = reset_mode;

	ret = ubx_m10_modem_ubx_script_init(dev, &payload, UBX_CFG_RST_PAYLOAD_SZ, UBX_CLASS_CFG,
					    UBX_CFG_RST);
	if (ret < 0) {
		goto unlock;
	}

	data->script.match = NULL;
	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

	if (reset_mode == UBX_CFG_RST_RESET_MODE_CONTROLLED_GNSS_STOP) {
		k_sleep(K_MSEC(UBX_CFG_RST_WAIT_MS));
	}

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_set_uart_baudrate(const struct device *dev, uint32_t baudrate)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	const struct ubx_m10_config *config = dev->config;
	struct uart_config uart_cfg;

	key = k_spin_lock(&data->lock);

	ret = ubx_m10_turn_off(dev);
	if (ret < 0) {
		goto reset_and_unlock;
	}

	ret = uart_config_get(config->uart, &uart_cfg);
	if (ret < 0) {
		goto reset_and_unlock;
	}
	uart_cfg.baudrate = baudrate;

	ret = uart_configure(config->uart, &uart_cfg);

reset_and_unlock:
	ret |= ubx_m10_resume(dev);

	k_spin_unlock(&data->lock, key);

	return ret;
}

static bool ubx_m10_validate_baudrate(const struct device *dev, uint32_t baudrate)
{
	for (int i = 0; i < UBX_BAUDRATE_COUNT; ++i) {
		if (baudrate == ubx_baudrate[i]) {
			return true;
		}
	}

	return false;
}

/* This function will return failure if "target_baudrate" != device's current baudrate.
 * Refer the function description of ubx_m10_ubx_cfg_prt_set for a detailed explanation.
 */
static int ubx_m10_configure_gnss_device_baudrate_prerequisite(const struct device *dev)
{
	/* Retry = 1 should be enough, but setting 2 just to be safe. */
	int ret, retry = 2;
	const struct ubx_m10_config *config = dev->config;
	uint32_t target_baudrate = config->uart_baudrate;

	ret = ubx_m10_validate_baudrate(dev, target_baudrate);
	if (ret < 0) {
		return ret;
	}

	/* Try communication with device with all possible baudrates, because initially we don't
	 * know the currently set baudrate of the device. We will match the baudrate in one of the
	 * following attempts and the device will thus change its baudrate to "target_baudrate".
	 */
	for (int i = 0; i < UBX_BAUDRATE_COUNT; ++i) {
		/* Set baudrate of UART pipe as ubx_baudrate[i]. */
		ret = ubx_m10_set_uart_baudrate(dev, ubx_baudrate[i]);
		if (ret < 0) {
			return ret;
		}

		/* Try setting baudrate of device as target_baudrate. */
		ret = ubx_m10_ubx_cfg_prt_set(dev, target_baudrate, retry);
		if (ret == 0) {
			break;
		}
	}

	/* Reset baudrate of UART pipe as target_baudrate. */
	ret = ubx_m10_set_uart_baudrate(dev, target_baudrate);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ubx_m10_configure_gnss_device_baudrate(const struct device *dev)
{
	int ret;
	const struct ubx_m10_config *config = dev->config;
	uint32_t target_baudrate = config->uart_baudrate;

	ret = ubx_m10_validate_baudrate(dev, target_baudrate);
	if (ret < 0) {
		return ret;
	}

	ret = ubx_m10_ubx_cfg_prt_set(dev, target_baudrate, UBX_M10_SCRIPT_RETRY_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ubx_m10_configure_messages(const struct device *dev)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	struct ubx_cfg_msg_payload payload;

	key = k_spin_lock(&data->lock);

	ubx_cfg_msg_payload_default(&payload);

	/* Enabling GGA, RMC and GSV messages. */
	payload.rate = 1;
	uint8_t message_enable[] = {UBX_NMEA_GGA, UBX_NMEA_RMC, UBX_NMEA_GSV};

	for (int i = 0; i < sizeof(message_enable); ++i) {
		payload.message_id = message_enable[i];
		ret = ubx_m10_modem_ubx_script_init(dev, &payload,  UBX_CFG_MSG_PAYLOAD_SZ,
						    UBX_CLASS_CFG, UBX_CFG_MSG);
		if (ret < 0) {
			goto unlock;
		}

		ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
		if (ret < 0) {
			goto unlock;
		}
	}

	/* Disabling DTM, GBS, GLL, GNS, GRS, GSA, GST, VLW, VTG and ZDA messages. */
	payload.rate = 0;
	uint8_t message_disable[] = {UBX_NMEA_DTM, UBX_NMEA_GBS, UBX_NMEA_GLL, UBX_NMEA_GNS,
				     UBX_NMEA_GRS, UBX_NMEA_GSA, UBX_NMEA_GST, UBX_NMEA_VLW,
				     UBX_NMEA_VTG, UBX_NMEA_ZDA};

	for (int i = 0; i < sizeof(message_disable); ++i) {
		payload.message_id = message_disable[i];
		ret = ubx_m10_modem_ubx_script_init(dev, &payload, UBX_CFG_MSG_PAYLOAD_SZ,
						    UBX_CLASS_CFG, UBX_CFG_MSG);
		if (ret < 0) {
			goto unlock;
		}

		ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
		if (ret < 0) {
			goto unlock;
		}
	}

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_navigation_mode_to_ubx_dynamic_model(const struct device *dev,
							enum gnss_navigation_mode mode)
{
	switch (mode) {
	case GNSS_NAVIGATION_MODE_ZERO_DYNAMICS:
		return UBX_DYN_MODEL_STATIONARY;
	case GNSS_NAVIGATION_MODE_LOW_DYNAMICS:
		return UBX_DYN_MODEL_PORTABLE;
	case GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS:
		return UBX_DYN_MODEL_AIRBONE1G;
	case GNSS_NAVIGATION_MODE_HIGH_DYNAMICS:
		return UBX_DYN_MODEL_AIRBONE4G;
	default:
		return -EINVAL;
	}
}

static int ubx_m10_ubx_dynamic_model_to_navigation_mode(const struct device *dev,
							enum ubx_dynamic_model dynamic_model)
{
	switch (dynamic_model) {
	case UBX_DYN_MODEL_PORTABLE:
		return GNSS_NAVIGATION_MODE_LOW_DYNAMICS;
	case UBX_DYN_MODEL_STATIONARY:
		return GNSS_NAVIGATION_MODE_ZERO_DYNAMICS;
	case UBX_DYN_MODEL_PEDESTRIAN:
		return GNSS_NAVIGATION_MODE_LOW_DYNAMICS;
	case UBX_DYN_MODEL_AUTOMOTIV:
		return GNSS_NAVIGATION_MODE_LOW_DYNAMICS;
	case UBX_DYN_MODEL_SEA:
		return GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
	case UBX_DYN_MODEL_AIRBONE1G:
		return GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
	case UBX_DYN_MODEL_AIRBONE2G:
		return GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
	case UBX_DYN_MODEL_AIRBONE4G:
		return GNSS_NAVIGATION_MODE_HIGH_DYNAMICS;
	case UBX_DYN_MODEL_WIRST:
		return GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
	case UBX_DYN_MODEL_BIKE:
		return GNSS_NAVIGATION_MODE_HIGH_DYNAMICS;
	default:
		return -EINVAL;
	}
}

static int ubx_m10_set_navigation_mode(const struct device *dev, enum gnss_navigation_mode mode)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	struct ubx_cfg_nav5_payload payload;

	key = k_spin_lock(&data->lock);

	ubx_cfg_nav5_payload_default(&payload);

	ret = ubx_m10_navigation_mode_to_ubx_dynamic_model(dev, mode);
	if (ret < 0) {
		goto unlock;
	}

	payload.dyn_model = ret;

	ret = ubx_m10_modem_ubx_script_init(dev, &payload, UBX_CFG_NAV5_PAYLOAD_SZ, UBX_CLASS_CFG,
					    UBX_CFG_NAV5);
	if (ret < 0) {
		goto unlock;
	}

	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

	k_sleep(K_MSEC(UBX_CFG_NAV5_WAIT_MS));

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_get_navigation_mode(const struct device *dev, enum gnss_navigation_mode *mode)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	enum ubx_dynamic_model dynamic_model;

	key = k_spin_lock(&data->lock);

	ret = ubx_m10_modem_ubx_script_init(dev, NULL, UBX_FRM_GET_PAYLOAD_SZ, UBX_CLASS_CFG,
					    UBX_CFG_NAV5);
	if (ret < 0) {
		goto unlock;
	}

	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

	struct ubx_frame *response = data->script.response;

	dynamic_model = ((struct ubx_cfg_nav5_payload *)response->payload_and_checksum)->dyn_model;
	ret = ubx_m10_ubx_dynamic_model_to_navigation_mode(dev, dynamic_model);
	if (ret < 0) {
		goto unlock;
	}

	*mode = ret;

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	*systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS | GNSS_SYSTEM_GALILEO |
		    GNSS_SYSTEM_BEIDOU | GNSS_SYSTEM_SBAS | GNSS_SYSTEM_QZSS);

	return 0;
}

static int ubx_m10_ubx_gnss_id_to_gnss_system(const struct device *dev, enum ubx_gnss_id gnss_id)
{
	switch (gnss_id) {
	case UBX_GNSS_ID_GPS:
		return GNSS_SYSTEM_GPS;
	case UBX_GNSS_ID_SBAS:
		return GNSS_SYSTEM_SBAS;
	case UBX_GNSS_ID_GALILEO:
		return GNSS_SYSTEM_GALILEO;
	case UBX_GNSS_ID_BEIDOU:
		return GNSS_SYSTEM_BEIDOU;
	case UBX_GNSS_ID_QZSS:
		return GNSS_SYSTEM_QZSS;
	case UBX_GNSS_ID_GLONASS:
		return GNSS_SYSTEM_GLONASS;
	default:
		return -EINVAL;
	};
}

static int ubx_m10_config_block_fill(const struct device *dev, gnss_systems_t gnss_system,
				     struct ubx_cfg_gnss_payload *payload, uint8_t index,
				     uint32_t enable)
{
	uint32_t signal_config;

	switch (gnss_system) {
	case GNSS_SYSTEM_GPS:
		payload->config_blocks[index].gnss_id = UBX_GNSS_ID_GPS;
		signal_config = UBX_CFG_GNSS_FLAG_SGN_CNF_GPS_L1C_A;
		break;
	case GNSS_SYSTEM_GLONASS:
		payload->config_blocks[index].gnss_id = UBX_GNSS_ID_GLONASS;
		signal_config = UBX_CFG_GNSS_FLAG_SGN_CNF_GLONASS_L1;
		break;
	case GNSS_SYSTEM_GALILEO:
		payload->config_blocks[index].gnss_id = UBX_GNSS_ID_GALILEO;
		signal_config = UBX_CFG_GNSS_FLAG_SGN_CNF_GALILEO_E1;
		break;
	case GNSS_SYSTEM_BEIDOU:
		payload->config_blocks[index].gnss_id = UBX_GNSS_ID_BEIDOU;
		signal_config = UBX_CFG_GNSS_FLAG_SGN_CNF_BEIDOU_B1I;
		break;
	case GNSS_SYSTEM_QZSS:
		payload->config_blocks[index].gnss_id = UBX_GNSS_ID_QZSS;
		signal_config = UBX_CFG_GNSS_FLAG_SGN_CNF_QZSS_L1C_A;
		break;
	case GNSS_SYSTEM_SBAS:
		payload->config_blocks[index].gnss_id = UBX_GNSS_ID_SBAS;
		signal_config = UBX_CFG_GNSS_FLAG_SGN_CNF_SBAS_L1C_A;
		break;
	default:
		return -EINVAL;
	};

	payload->config_blocks[index].flags = enable | signal_config;

	return 0;
}

static int ubx_m10_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;

	key = k_spin_lock(&data->lock);

	struct ubx_cfg_gnss_payload *payload;

	/* Get number of tracking channels for each supported gnss system by sending CFG-GNSS. */
	ret = ubx_m10_modem_ubx_script_init(dev, NULL, UBX_FRM_GET_PAYLOAD_SZ, UBX_CLASS_CFG,
					    UBX_CFG_GNSS);
	if (ret < 0) {
		goto unlock;
	}

	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

	struct ubx_frame *response = data->script.response;
	uint16_t res_trk_ch_sum = 0, max_trk_ch_sum = 0;

	/* Calculate sum of reserved and maximum tracking channels for each supported gnss system,
	 * and assert that the sum is not greater than the number of tracking channels in use.
	 */
	payload = (struct ubx_cfg_gnss_payload *) response->payload_and_checksum;
	for (int i = 0; i < payload->num_config_blocks; ++i) {
		ret = ubx_m10_ubx_gnss_id_to_gnss_system(dev, payload->config_blocks[i].gnss_id);
		if (ret < 0) {
			ret = -EINVAL;
			goto unlock;
		}

		if (ret & systems) {
			res_trk_ch_sum += payload->config_blocks[i].num_res_trk_ch;
			max_trk_ch_sum += payload->config_blocks[i].max_num_trk_ch;
		}

		if (res_trk_ch_sum > payload->num_trk_ch_use ||
		    max_trk_ch_sum > payload->num_trk_ch_use) {
			ret = -EINVAL;
			goto unlock;
		}
	}

	/* Prepare payload (payload) for sending CFG-GNSS for enabling the gnss systems. */
	payload = malloc(sizeof(*payload) +
		sizeof(struct ubx_cfg_gnss_payload_config_block) * UBX_M10_GNSS_SUPP_SYS_CNT);
	payload->num_config_blocks = UBX_M10_GNSS_SUPP_SYS_CNT;

	ubx_cfg_gnss_payload_default(payload);

	uint8_t filled_blocks = 0;
	gnss_systems_t supported_systems;

	ret = ubx_m10_get_supported_systems(dev, &supported_systems);
	if (ret < 0) {
		goto free_and_unlock;
	}

	for (int i = 0; i < UBX_M10_GNSS_SYS_CNT; ++i) {
		gnss_systems_t gnss_system = 1 << i;

		if (gnss_system & supported_systems) {
			uint32_t enable = (systems & gnss_system) ?
					  UBX_CFG_GNSS_FLAG_ENABLE : UBX_CFG_GNSS_FLAG_DISABLE;

			ret = ubx_m10_config_block_fill(dev, gnss_system, payload, filled_blocks,
							enable);
			if (ret < 0) {
				goto free_and_unlock;
			}

			++filled_blocks;
		}
	}

	ret = ubx_m10_modem_ubx_script_init(dev, payload,
					    UBX_CFG_GNSS_PAYLOAD_SZ(UBX_M10_GNSS_SUPP_SYS_CNT),
					    UBX_CLASS_CFG, UBX_CFG_GNSS);
	if (ret < 0) {
		goto free_and_unlock;
	}

	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto free_and_unlock;
	}

	k_sleep(K_MSEC(UBX_CFG_GNSS_WAIT_MS));

free_and_unlock:
	free(payload);

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;

	key = k_spin_lock(&data->lock);

	ret = ubx_m10_modem_ubx_script_init(dev, NULL, UBX_FRM_GET_PAYLOAD_SZ, UBX_CLASS_CFG,
					    UBX_CFG_GNSS);
	if (ret < 0) {
		goto unlock;
	}

	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

	struct ubx_frame *response = data->script.response;
	struct ubx_cfg_gnss_payload *payload =
		(struct ubx_cfg_gnss_payload *) response->payload_and_checksum;

	*systems = 0;
	for (int i = 0; i < payload->num_config_blocks; ++i) {
		if (payload->config_blocks[i].flags & UBX_CFG_GNSS_FLAG_ENABLE) {
			enum ubx_gnss_id gnss_id = payload->config_blocks[i].gnss_id;

			ret = ubx_m10_ubx_gnss_id_to_gnss_system(dev, gnss_id);
			if (ret < 0) {
				goto unlock;
			}

			*systems |= ret;
		}
	}

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	struct ubx_cfg_rate_payload payload;

	if (fix_interval_ms < 50) {
		return -1;
	}

	key = k_spin_lock(&data->lock);

	ubx_cfg_rate_payload_default(&payload);
	payload.meas_rate_ms = fix_interval_ms;

	ret = ubx_m10_modem_ubx_script_init(dev, &payload, UBX_CFG_RATE_PAYLOAD_SZ, UBX_CLASS_CFG,
					    UBX_CFG_RATE);
	if (ret < 0) {
		goto unlock;
	}

	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int ubx_m10_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	int ret;
	k_spinlock_key_t key;
	struct ubx_m10_data *data = dev->data;
	struct ubx_cfg_rate_payload *payload;

	key = k_spin_lock(&data->lock);

	ret = ubx_m10_modem_ubx_script_init(dev, NULL, UBX_FRM_GET_PAYLOAD_SZ, UBX_CLASS_CFG,
					    UBX_CFG_RATE);
	if (ret < 0) {
		goto unlock;
	}

	ret = ubx_m10_modem_ubx_run_script(dev, &(data->script));
	if (ret < 0) {
		goto unlock;
	}

	struct ubx_frame *response = data->script.response;

	payload = (struct ubx_cfg_rate_payload *) response->payload_and_checksum;
	*fix_interval_ms = payload->meas_rate_ms;

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static const struct gnss_driver_api gnss_api = {
	.set_fix_rate = ubx_m10_set_fix_rate,
	.get_fix_rate = ubx_m10_get_fix_rate,
	.set_navigation_mode = ubx_m10_set_navigation_mode,
	.get_navigation_mode = ubx_m10_get_navigation_mode,
	.set_enabled_systems = ubx_m10_set_enabled_systems,
	.get_enabled_systems = ubx_m10_get_enabled_systems,
	.get_supported_systems = ubx_m10_get_supported_systems,
};

static int ubx_m10_configure(const struct device *dev)
{
	int ret;

	/* The return value could be ignored. See function description for more details. */
	(void)ubx_m10_configure_gnss_device_baudrate_prerequisite(dev);

	/* Stopping GNSS messages for clearer communication while configuring the device. */
	ret = ubx_m10_ubx_cfg_rst(dev, UBX_CFG_RST_RESET_MODE_CONTROLLED_GNSS_STOP);
	if (ret < 0) {
		goto reset;
	}

	ret = ubx_m10_ubx_cfg_rate(dev);
	if (ret < 0) {
		LOG_ERR("Configuring rate failed. Returned %d.", ret);
		goto reset;
	}

	ret = ubx_m10_configure_gnss_device_baudrate(dev);
	if (ret < 0) {
		LOG_ERR("Configuring baudrate failed. Returned %d.", ret);
		goto reset;
	}

	ret = ubx_m10_configure_messages(dev);
	if (ret < 0) {
		LOG_ERR("Configuring messages failed. Returned %d.", ret);
		goto reset;
	}

reset:
	ret = ubx_m10_ubx_cfg_rst(dev, UBX_CFG_RST_RESET_MODE_CONTROLLED_GNSS_START);
	if (ret < 0) {
		goto reset;
	}

	return ret;
}

static int ubx_m10_init(const struct device *dev)
{
	int ret;

	k_sleep(K_MSEC(UBX_M10_BOOT_TIME_MS));

	ret = ubx_m10_init_nmea0183_match(dev);
	if (ret < 0) {
		return ret;
	}

	ubx_m10_init_pipe(dev);

	ret = ubx_m10_init_chat(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ubx_m10_init_ubx(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ubx_m10_resume(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ubx_m10_configure(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define UBX_M10(inst)										\
	static const struct ubx_m10_config ubx_m10_cfg_##inst = {				\
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.uart_baudrate = DT_PROP(DT_DRV_INST(inst), uart_baudrate),			\
	};											\
												\
	static struct ubx_m10_data ubx_m10_data_##inst = {					\
		.script.request = (struct ubx_frame *)ubx_m10_data_##inst.request_buf,		\
		.script.response = (struct ubx_frame *)ubx_m10_data_##inst.response_buf,	\
		.script.match = (struct ubx_frame *)ubx_m10_data_##inst.match_buf,		\
		.script.retry_count = UBX_M10_SCRIPT_RETRY_DEFAULT,				\
		.script.timeout = K_MSEC(MODEM_UBX_SCRIPT_TIMEOUT_MS),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst,								\
			      ubx_m10_init,							\
			      NULL,								\
			      &ubx_m10_data_##inst,						\
			      &ubx_m10_cfg_##inst,						\
			      POST_KERNEL,							\
			      CONFIG_GNSS_INIT_PRIORITY,					\
			      &gnss_api);

DT_INST_FOREACH_STATUS_OKAY(UBX_M10)
