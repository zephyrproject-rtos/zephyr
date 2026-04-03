/*
 * Copyright (c) 2026 Hong Nguyen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "gnss_nmea0183.h"
#include "gnss_nmea0183_match.h"
#include "gnss_parse.h"

LOG_MODULE_REGISTER(globaltop_pa6h, CONFIG_GNSS_LOG_LEVEL);

#define DT_DRV_COMPAT globaltop_pa6h

#define UART_RX_BUF_SIZE       128
#define UART_TX_BUF_SIZE       64
#define MODEM_CHAT_RX_BUF_SIZE 256
#define MODEM_CHAT_ARGV_SIZE   32
#define MODEM_TIMEOUT_S        10

struct globaltop_pa6h_config {
	const struct device *uart;
	uint16_t fix_interval_ms;
};

struct globaltop_pa6h_data {
	struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_GLOBALTOP_PA6H_SATELLITES_COUNT];
#endif

	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[UART_RX_BUF_SIZE];
	uint8_t uart_backend_transmit_buf[UART_TX_BUF_SIZE];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[MODEM_CHAT_RX_BUF_SIZE];
	uint8_t chat_delimiter[2];
	uint8_t *chat_argv[MODEM_CHAT_ARGV_SIZE];

	/* Chat script */
	uint8_t pmtk_request_buf[64];
	uint8_t pmtk_match_buf[32];
	struct modem_chat_match pmtk_match;
	struct modem_chat_script_chat pmtk_script_chat;
	struct modem_chat_script pmtk_script;

	/* Internal states */
	gnss_systems_t enabled_systems;
	uint16_t last_fix_interval_ms;
	int8_t pmtk_last_ack_flag;
	bool is_init;

	struct k_sem lock;
};

MODEM_CHAT_MATCHES_DEFINE(
	unsol_matches, MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
	MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);

static void globaltop_pa6h_lock(const struct device *dev)
{
	struct globaltop_pa6h_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void globaltop_pa6h_unlock(const struct device *dev)
{
	struct globaltop_pa6h_data *data = dev->data;

	k_sem_give(&data->lock);
}

static int globaltop_pa6h_init_nmea0183_match(const struct device *dev)
{
	struct globaltop_pa6h_data *data = dev->data;

	const struct gnss_nmea0183_match_config match_config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = data->satellites,
		.satellites_size = ARRAY_SIZE(data->satellites),
#endif
	};

	return gnss_nmea0183_match_init(&data->match_data, &match_config);
}

static void globaltop_pa6h_init_pipe(const struct device *dev)
{
	const struct globaltop_pa6h_config *config = dev->config;
	struct globaltop_pa6h_data *data = dev->data;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = config->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = ARRAY_SIZE(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

static int globaltop_pa6h_init_chat(const struct device *dev)
{
	struct globaltop_pa6h_data *data = dev->data;

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_receive_buf,
		.receive_buf_size = ARRAY_SIZE(data->chat_receive_buf),
		.delimiter = data->chat_delimiter,
		.delimiter_size = ARRAY_SIZE(data->chat_delimiter),
		.filter = NULL,
		.filter_size = 0,
		.argv = data->chat_argv,
		.argv_size = ARRAY_SIZE(data->chat_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	return modem_chat_init(&data->chat, &chat_config);
}

static void globaltop_pa6h_init_pmtk_script(const struct device *dev)
{
	struct globaltop_pa6h_data *data = dev->data;

	modem_chat_match_init(&data->pmtk_match);
	modem_chat_match_set_separators(&data->pmtk_match, ",*");

	modem_chat_script_chat_init(&data->pmtk_script_chat);
	modem_chat_script_chat_set_response_matches(&data->pmtk_script_chat, &data->pmtk_match, 1);

	modem_chat_script_init(&data->pmtk_script);
	modem_chat_script_set_name(&data->pmtk_script, "pmtk");
	modem_chat_script_set_script_chats(&data->pmtk_script, &data->pmtk_script_chat, 1);
	modem_chat_script_set_abort_matches(&data->pmtk_script, NULL, 0);
	modem_chat_script_set_timeout(&data->pmtk_script, MODEM_TIMEOUT_S);
}

static void globaltop_pa6h_pmtk_ack_cb(struct modem_chat *chat, char **argv, uint16_t argc,
				       void *user_data)
{
	struct globaltop_pa6h_data *data = user_data;
	int32_t flag;

	ARG_UNUSED(chat);

	if (argc < 2) {
		return;
	}

	if (gnss_parse_atoi(argv[1], 10, &flag) < 0) {
		return;
	}

	data->pmtk_last_ack_flag = (int8_t)flag;
}

static int globaltop_pa6h_pmtk_send_and_ack(const struct device *dev, uint16_t cmd_id)
{
	struct globaltop_pa6h_data *data = dev->data;
	int ret = 0;

	/* Build the ACK match */
	snprintk(data->pmtk_match_buf, sizeof(data->pmtk_match_buf), "$PMTK001,%u,", cmd_id);
	ret = modem_chat_match_set_match(&data->pmtk_match, data->pmtk_match_buf);
	if (ret < 0) {
		return ret;
	}

	/* Reset ACK flag, wait for response, and remove cb after done */
	data->pmtk_last_ack_flag = -1;
	modem_chat_match_set_callback(&data->pmtk_match, globaltop_pa6h_pmtk_ack_cb);

	ret = modem_chat_run_script(&data->chat, &data->pmtk_script);

	modem_chat_match_set_callback(&data->pmtk_match, NULL);

	if (ret < 0) {
		return ret;
	}

	return data->pmtk_last_ack_flag == 3 ? 0 : -EIO;
}

static int globaltop_pa6h_pm_resume(const struct device *dev)
{
	struct globaltop_pa6h_data *data = dev->data;
	const struct globaltop_pa6h_config *config = dev->config;
	int ret;

	ret = modem_pipe_open(data->uart_pipe, K_SECONDS(MODEM_TIMEOUT_S));
	if (ret < 0) {
		LOG_ERR("Failed to open UART pipe: %d", ret);
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->uart_pipe);
	if (ret < 0) {
		LOG_ERR("Failed to attach chat: %d", ret);
		modem_pipe_close(data->uart_pipe, K_SECONDS(MODEM_TIMEOUT_S));
		return ret;
	}

	/* If 1st resume (at init) -> configure fix rate */
	if (!data->is_init) {
		ret = gnss_set_fix_rate(dev, config->fix_interval_ms);
		if (ret < 0) {
			LOG_ERR("Failed to set initial fix rate: %d", ret);
			modem_pipe_close(data->uart_pipe, K_SECONDS(MODEM_TIMEOUT_S));
			return ret;
		}
		data->is_init = true;
	} else {
		/* Wake the module by sending any byte */
		(void)modem_pipe_transmit(data->uart_pipe, (const uint8_t *)"\r\n", 2);
	}

	LOG_INF("Device resumed");

	return 0;
}

static int globaltop_pa6h_pm_suspend(const struct device *dev)
{
	struct globaltop_pa6h_data *data = dev->data;
	int ret;

	/* 161 = standby cmd type */
	globaltop_pa6h_lock(dev);

	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK161,%u", 0);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = globaltop_pa6h_pmtk_send_and_ack(dev, 161);
	if (ret < 0) {
		goto unlock_return;
	}

	LOG_INF("Device suspended");

	ret = modem_pipe_close(data->uart_pipe, K_SECONDS(MODEM_TIMEOUT_S));

unlock_return:
	globaltop_pa6h_unlock(dev);
	return ret;
}

static int globaltop_pa6h_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = -ENOTSUP;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = globaltop_pa6h_pm_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = globaltop_pa6h_pm_resume(dev);
		break;

	default:
		break;
	}

	return ret;
}

static int globaltop_pa6h_init(const struct device *dev)
{
	struct globaltop_pa6h_data *data = dev->data;
	int ret;

	k_sem_init(&data->lock, 1, 1);

	ret = globaltop_pa6h_init_nmea0183_match(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize NMEA0183 match: %d", ret);
		return ret;
	}

	globaltop_pa6h_init_pipe(dev);

	ret = globaltop_pa6h_init_chat(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize chat: %d", ret);
		return ret;
	}

	globaltop_pa6h_init_pmtk_script(dev);

	/* GPS is always enabled */
	data->enabled_systems = GNSS_SYSTEM_GPS;

	return pm_device_driver_init(dev, globaltop_pa6h_pm_action);
}

static int globaltop_pa6h_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	struct globaltop_pa6h_data *data = dev->data;
	int ret;

	if (fix_interval_ms < 100 || fix_interval_ms > 10000) {
		return -EINVAL;
	}

	globaltop_pa6h_lock(dev);

	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK220,%u", fix_interval_ms);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = globaltop_pa6h_pmtk_send_and_ack(dev, 220);
	if (ret == 0) {
		data->last_fix_interval_ms = (uint16_t)fix_interval_ms;
	}

unlock_return:
	globaltop_pa6h_unlock(dev);
	return ret;
}

static int globaltop_pa6h_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	struct globaltop_pa6h_data *data = dev->data;

	globaltop_pa6h_lock(dev);
	*fix_interval_ms = data->last_fix_interval_ms;
	globaltop_pa6h_unlock(dev);

	return 0;
}

static int globaltop_pa6h_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	struct globaltop_pa6h_data *data = dev->data;
	gnss_systems_t supported_systems;
	int ret;

	supported_systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_QZSS | GNSS_SYSTEM_SBAS);

	/* GPS is always required */
	if (!(systems & GNSS_SYSTEM_GPS) || (systems & ~supported_systems)) {
		return -EINVAL;
	}

	globaltop_pa6h_lock(dev);

	/* PMTK313: enable/disable SBAS (0=disable, 1=enable) */
	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK313,%u", (systems & GNSS_SYSTEM_SBAS) ? 1 : 0);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = globaltop_pa6h_pmtk_send_and_ack(dev, 313);
	if (ret < 0) {
		goto unlock_return;
	}

	data->enabled_systems =
		(data->enabled_systems & ~GNSS_SYSTEM_SBAS) | (systems & GNSS_SYSTEM_SBAS);

	/* PMTK352: enable/disable QZSS (0=enable, 1=disable) */
	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK352,%u", (systems & GNSS_SYSTEM_QZSS) ? 0 : 1);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = globaltop_pa6h_pmtk_send_and_ack(dev, 352);
	if (ret < 0) {
		goto unlock_return;
	}

	data->enabled_systems =
		(data->enabled_systems & ~GNSS_SYSTEM_QZSS) | (systems & GNSS_SYSTEM_QZSS);

unlock_return:
	globaltop_pa6h_unlock(dev);
	return ret;
}

static int globaltop_pa6h_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	struct globaltop_pa6h_data *data = dev->data;

	globaltop_pa6h_lock(dev);
	*systems = data->enabled_systems;
	globaltop_pa6h_unlock(dev);

	return 0;
}

static int globaltop_pa6h_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	ARG_UNUSED(dev);
	*systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_QZSS | GNSS_SYSTEM_SBAS);
	return 0;
}

static DEVICE_API(gnss, globaltop_pa6h_api) = {
	.set_fix_rate = globaltop_pa6h_set_fix_rate,
	.get_fix_rate = globaltop_pa6h_get_fix_rate,
	.set_enabled_systems = globaltop_pa6h_set_enabled_systems,
	.get_enabled_systems = globaltop_pa6h_get_enabled_systems,
	.get_supported_systems = globaltop_pa6h_get_supported_systems,
};

#define GLOBALTOP_PA6H(inst)                                                                       \
                                                                                                   \
	BUILD_ASSERT((DT_PROP(DT_INST_BUS(inst), current_speed) == 4800) ||                        \
			     (DT_PROP(DT_INST_BUS(inst), current_speed) == 9600) ||                \
			     (DT_PROP(DT_INST_BUS(inst), current_speed) == 14400) ||               \
			     (DT_PROP(DT_INST_BUS(inst), current_speed) == 19200) ||               \
			     (DT_PROP(DT_INST_BUS(inst), current_speed) == 38400) ||               \
			     (DT_PROP(DT_INST_BUS(inst), current_speed) == 57600) ||               \
			     (DT_PROP(DT_INST_BUS(inst), current_speed) == 115200),                \
		     "Invalid current-speed. Please set the UART current-speed to a baudrate "     \
		     "compatible with the modem.");                                                \
                                                                                                   \
	BUILD_ASSERT((DT_INST_PROP(inst, fix_interval_ms) >= 100) &&                               \
			     (DT_INST_PROP(inst, fix_interval_ms) <= 10000),                       \
		     "Invalid interval. Please set it between 100 and 10000 ms");                  \
                                                                                                   \
	static const struct globaltop_pa6h_config globaltop_pa6h_cfg_##inst = {                    \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.fix_interval_ms = DT_INST_PROP(inst, fix_interval_ms),                            \
	};                                                                                         \
                                                                                                   \
	static struct globaltop_pa6h_data globaltop_pa6h_data_##inst = {                           \
		.chat_delimiter = {'\r', '\n'},                                                    \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, globaltop_pa6h_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, globaltop_pa6h_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &globaltop_pa6h_data_##inst, &globaltop_pa6h_cfg_##inst,             \
			      POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY, &globaltop_pa6h_api);

DT_INST_FOREACH_STATUS_OKAY(GLOBALTOP_PA6H)
