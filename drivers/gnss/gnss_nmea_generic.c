/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <string.h>

#include "gnss_nmea0183.h"
#include "gnss_nmea0183_match.h"
#include "gnss_parse.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gnss_nmea_generic, CONFIG_GNSS_LOG_LEVEL);

#define UART_RX_BUF_SZ (256 + IS_ENABLED(CONFIG_GNSS_SATELLITES) * 512)
#define UART_TX_BUF_SZ 64
#define CHAT_RECV_BUF_SZ 256
#define CHAT_ARGV_SZ 32

struct gnss_nmea_generic_config {
	const struct device *uart;
	const struct modem_chat_script *const init_chat_script;
};

struct gnss_nmea_generic_data {
	struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_NMEA_GENERIC_SATELLITES_COUNT];
#endif

	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[UART_RX_BUF_SZ];
	uint8_t uart_backend_transmit_buf[UART_TX_BUF_SZ];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[CHAT_RECV_BUF_SZ];
	uint8_t *chat_argv[CHAT_ARGV_SZ];
};

MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
	MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
	MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);

static int gnss_nmea_generic_resume(const struct device *dev)
{
	const struct gnss_nmea_generic_config *cfg = dev->config;
	struct gnss_nmea_generic_data *data = dev->data;
	int ret;

	ret = modem_pipe_open(data->uart_pipe);
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->uart_pipe);

	if (ret == 0) {
		ret = modem_chat_run_script(&data->chat, cfg->init_chat_script);
	}

	if (ret < 0) {
		modem_pipe_close(data->uart_pipe);
	}
	return ret;
}

static struct gnss_driver_api gnss_api = {
};

static int gnss_nmea_generic_init_nmea0183_match(const struct device *dev)
{
	struct gnss_nmea_generic_data *data = dev->data;

	const struct gnss_nmea0183_match_config match_config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = data->satellites,
		.satellites_size = ARRAY_SIZE(data->satellites),
#endif
	};

	return gnss_nmea0183_match_init(&data->match_data, &match_config);
}

static void gnss_nmea_generic_init_pipe(const struct device *dev)
{
	const struct gnss_nmea_generic_config *cfg = dev->config;
	struct gnss_nmea_generic_data *data = dev->data;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = cfg->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = sizeof(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = sizeof(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

static uint8_t gnss_nmea_generic_char_delimiter[] = {'\r', '\n'};

static int gnss_nmea_generic_init_chat(const struct device *dev)
{
	struct gnss_nmea_generic_data *data = dev->data;

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_receive_buf,
		.receive_buf_size = sizeof(data->chat_receive_buf),
		.delimiter = gnss_nmea_generic_char_delimiter,
		.delimiter_size = ARRAY_SIZE(gnss_nmea_generic_char_delimiter),
		.filter = NULL,
		.filter_size = 0,
		.argv = data->chat_argv,
		.argv_size = ARRAY_SIZE(data->chat_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	return modem_chat_init(&data->chat, &chat_config);
}

static int gnss_nmea_generic_init(const struct device *dev)
{
	int ret;

	ret = gnss_nmea_generic_init_nmea0183_match(dev);
	if (ret < 0) {
		return ret;
	}

	gnss_nmea_generic_init_pipe(dev);

	ret = gnss_nmea_generic_init_chat(dev);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_PM_DEVICE
	pm_device_init_suspended(dev);
#else
	ret = gnss_nmea_generic_resume(dev);
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}

#if CONFIG_PM_DEVICE
static int gnss_nmea_generic_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return gnss_nmea_generic_resume(dev);
	default:
		return -ENOTSUP;
	}
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(gnss_nmea_generic)
MODEM_CHAT_SCRIPT_EMPTY_DEFINE(gnss_nmea_generic_init_chat_script);
#endif

#define GNSS_NMEA_GENERIC(inst)								\
	static struct gnss_nmea_generic_config gnss_nmea_generic_cfg_##inst = {		\
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),				\
		.init_chat_script = &_CONCAT(DT_DRV_COMPAT, _init_chat_script),         \
	};										\
											\
	static struct gnss_nmea_generic_data gnss_nmea_generic_data_##inst;		\
											\
	PM_DEVICE_DT_INST_DEFINE(inst, gnss_nmea_generic_pm_action);                    \
                                                                                        \
	DEVICE_DT_INST_DEFINE(inst, gnss_nmea_generic_init, PM_DEVICE_DT_INST_GET(inst),\
			      &gnss_nmea_generic_data_##inst,				\
			      &gnss_nmea_generic_cfg_##inst,				\
			      POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY, &gnss_api);

#define DT_DRV_COMPAT gnss_nmea_generic
DT_INST_FOREACH_STATUS_OKAY(GNSS_NMEA_GENERIC)
#undef DT_DRV_COMPAT
