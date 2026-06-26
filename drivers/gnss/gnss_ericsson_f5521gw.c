/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT ericsson_f5521gw

#include <zephyr/drivers/gnss.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "gnss_nmea0183.h"
#include "gnss_nmea0183_match.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gnss_f5521gw, CONFIG_GNSS_LOG_LEVEL);

#define UART_RX_BUF_SZ    (256 + IS_ENABLED(CONFIG_GNSS_SATELLITES) * 512)
#define UART_TX_BUF_SZ    64
#define CHAT_RECV_BUF_SZ  256
#define CHAT_ARGV_SZ      32
#define CHAT_DELIMETER_SZ 2

/* clang-format off */
MODEM_CHAT_MATCH_DEFINE(f5521gw_ok_match, "OK", "", NULL);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(f5521gw_resume_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT*E2GPSCTL=1,1,0", f5521gw_ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT*E2GPSNPD", f5521gw_ok_match),
);

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(f5521gw_resume_script, f5521gw_resume_cmds, NULL, 10000);

#if CONFIG_PM_DEVICE
MODEM_CHAT_SCRIPT_CMDS_DEFINE(f5521gw_suspend_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT*E2GPSCTL=0", f5521gw_ok_match)
);

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(f5521gw_suspend_script, f5521gw_suspend_cmds, NULL, 10000);
#endif /* CONFIG_PM_DEVICE */

MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
	MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
	MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);
/* clang-format on */

struct f5521gw_config {
	const struct device *uart;
};

struct f5521gw_data {
	struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_ERICSSON_F5521GW_SATELLITES_COUNT];
#endif

	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[UART_RX_BUF_SZ];
	uint8_t uart_backend_transmit_buf[UART_TX_BUF_SZ];

	struct modem_chat chat;
	uint8_t chat_receive_buf[CHAT_RECV_BUF_SZ];
	uint8_t *chat_argv[CHAT_ARGV_SZ];
	uint8_t chat_delimiter[CHAT_DELIMETER_SZ];
};

static int f5521gw_resume(const struct device *dev)
{
	struct f5521gw_data *data = dev->data;
	int ret;

	ret = modem_pipe_open(data->uart_pipe, K_SECONDS(10));
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->uart_pipe);

	if (ret == 0) {
		ret = modem_chat_run_script(&data->chat, &f5521gw_resume_script);
	}

	if (ret < 0) {
		modem_pipe_close(data->uart_pipe, K_SECONDS(10));
	}

	return ret;
}

#if CONFIG_PM_DEVICE
static int f5521gw_suspend(const struct device *dev)
{
	struct f5521gw_data *data = dev->data;
	int ret;

	LOG_DBG("Suspending");

	ret = modem_chat_run_script(&data->chat, &f5521gw_suspend_script);
	if (ret < 0) {
		LOG_ERR("Failed to suspend GNSS");
	}

	modem_pipe_close(data->uart_pipe, K_SECONDS(10));
	return ret;
}

static int f5521gw_turn_on(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("Powered on");
	return 0;
}

static int f5521gw_turn_off(const struct device *dev)
{
	struct f5521gw_data *data = dev->data;

	LOG_DBG("Powered off");
	return modem_pipe_close(data->uart_pipe, K_SECONDS(10));
}

static int f5521gw_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return f5521gw_resume(dev);

	case PM_DEVICE_ACTION_SUSPEND:
		return f5521gw_suspend(dev);

	case PM_DEVICE_ACTION_TURN_ON:
		return f5521gw_turn_on(dev);

	case PM_DEVICE_ACTION_TURN_OFF:
		return f5521gw_turn_off(dev);

	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static int f5521gw_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	ARG_UNUSED(dev);

	*fix_interval_ms = 1000;
	return 0;
}

static int f5521gw_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	ARG_UNUSED(dev);

	/* F5521gw supports only GPS */
	*systems = GNSS_SYSTEM_GPS;
	return 0;
}

static int f5521gw_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	ARG_UNUSED(dev);

	/* F5521gw supports only GPS */
	*systems = GNSS_SYSTEM_GPS;
	return 0;
}

static DEVICE_API(gnss, gnss_api) = {
	.get_fix_rate = f5521gw_get_fix_rate,
	.get_enabled_systems = f5521gw_get_enabled_systems,
	.get_supported_systems = f5521gw_get_supported_systems,
};

static int f5521gw_init(const struct device *dev)
{
	const struct f5521gw_config *cfg = dev->config;
	struct f5521gw_data *data = dev->data;
	int ret;

	const struct gnss_nmea0183_match_config match_config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = data->satellites,
		.satellites_size = ARRAY_SIZE(data->satellites),
#endif
	};

	ret = gnss_nmea0183_match_init(&data->match_data, &match_config);
	if (ret < 0) {
		return ret;
	}

	const struct modem_backend_uart_config uart_config = {
		.uart = cfg->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = sizeof(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = sizeof(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_config);

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_receive_buf,
		.receive_buf_size = sizeof(data->chat_receive_buf),
		.delimiter = data->chat_delimiter,
		.delimiter_size = ARRAY_SIZE(data->chat_delimiter),
		.filter = NULL,
		.filter_size = 0,
		.argv = data->chat_argv,
		.argv_size = ARRAY_SIZE(data->chat_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	ret = modem_chat_init(&data->chat, &chat_config);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_PM_DEVICE
	pm_device_init_suspended(dev);
#else
	ret = f5521gw_resume(dev);
	if (ret < 0) {
		LOG_ERR("Failed to start GNSS: %d", ret);
		return ret;
	}
#endif

	return 0;
}

#define F5521GW_DEFINE(inst)                                                                       \
	static const struct f5521gw_config f5521gw_cfg_##inst = {                                  \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
	};                                                                                         \
                                                                                                   \
	static struct f5521gw_data f5521gw_data_##inst = {                                         \
		.chat_delimiter = {'\r', '\n'},                                                    \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, f5521gw_pm_action);                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, f5521gw_init, PM_DEVICE_DT_INST_GET(inst),                     \
			      &f5521gw_data_##inst, &f5521gw_cfg_##inst, POST_KERNEL,              \
			      CONFIG_GNSS_INIT_PRIORITY, &gnss_api);

DT_INST_FOREACH_STATUS_OKAY(F5521GW_DEFINE)
