/*
 * Copyright (c) 2024 Jerónimo Agulló
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#include "gnss_nmea0183.h"
#include "gnss_nmea0183_match.h"
#include "gnss_parse.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(luatos_air530z, CONFIG_GNSS_LOG_LEVEL);

#define DT_DRV_COMPAT luatos_air530z

#define UART_RECV_BUF_SZ 128
#define UART_TRANS_BUF_SZ 64

#define CHAT_RECV_BUF_SZ 256
#define CHAT_ARGV_SZ 32

MODEM_CHAT_SCRIPT_CMDS_DEFINE(init_script_cmds,
#if CONFIG_GNSS_SATELLITES
	/* receive only GGA, RMC and GSV NMEA messages */
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("$PCAS03,1,0,0,1,1,0,0,0,0,0,0,0,0*1F", 10),
#else
	/* receive only GGA and RMC NMEA messages */
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("$PCAS03,1,0,0,0,1,0,0,0,0,0,0,0,0*1E", 10),
#endif
);

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(init_script, init_script_cmds, NULL, 5);

struct gnss_luatos_air530z_config {
	const struct device *uart;
	const struct gpio_dt_spec on_off_gpio;
	const int uart_baudrate;
};

struct gnss_luatos_air530z_data {
	struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_LUATOS_AIR530Z_SATELLITES_COUNT];
#endif

	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[UART_RECV_BUF_SZ];
	uint8_t uart_backend_transmit_buf[UART_TRANS_BUF_SZ];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[CHAT_RECV_BUF_SZ];
	uint8_t chat_delimiter[2];
	uint8_t *chat_argv[CHAT_ARGV_SZ];

	/* Dynamic chat script */
	uint8_t dynamic_separators_buf[2];
	uint8_t dynamic_request_buf[32];
	struct modem_chat_script_chat dynamic_script_chat;
	struct modem_chat_script dynamic_script;

	struct k_sem lock;
};

MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
	MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
	MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);

static void luatos_air530z_lock(const struct device *dev)
{
	struct gnss_luatos_air530z_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void luatos_air530z_unlock(const struct device *dev)
{
	struct gnss_luatos_air530z_data *data = dev->data;

	k_sem_give(&data->lock);
}

static int gnss_luatos_air530z_init_nmea0183_match(const struct device *dev)
{
	struct gnss_luatos_air530z_data *data = dev->data;

	const struct gnss_nmea0183_match_config match_config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = data->satellites,
		.satellites_size = ARRAY_SIZE(data->satellites),
#endif
	};

	return gnss_nmea0183_match_init(&data->match_data, &match_config);
}

static void gnss_luatos_air530z_init_pipe(const struct device *dev)
{
	const struct gnss_luatos_air530z_config *config = dev->config;
	struct gnss_luatos_air530z_data *data = dev->data;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = config->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = sizeof(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

static int gnss_luatos_air530z_init_chat(const struct device *dev)
{
	struct gnss_luatos_air530z_data *data = dev->data;

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

	return modem_chat_init(&data->chat, &chat_config);
}

static void luatos_air530z_init_dynamic_script(const struct device *dev)
{
	struct gnss_luatos_air530z_data *data = dev->data;

	/* Air530z doesn't respond to commands. Thus, response_matches_size = 0; */
	data->dynamic_script_chat.request = data->dynamic_request_buf;
	data->dynamic_script_chat.response_matches = NULL;
	data->dynamic_script_chat.response_matches_size = 0;
	data->dynamic_script_chat.timeout = 0;

	data->dynamic_script.name = "PCAS";
	data->dynamic_script.script_chats = &data->dynamic_script_chat;
	data->dynamic_script.script_chats_size = 1;
	data->dynamic_script.abort_matches = NULL;
	data->dynamic_script.abort_matches_size = 0;
	data->dynamic_script.callback = NULL;
	data->dynamic_script.timeout = 5;
}

static int gnss_luatos_air530z_init(const struct device *dev)
{
	struct gnss_luatos_air530z_data *data = dev->data;
	const struct gnss_luatos_air530z_config *config = dev->config;
	int ret;

	k_sem_init(&data->lock, 1, 1);

	ret = gnss_luatos_air530z_init_nmea0183_match(dev);
	if (ret < 0) {
		return ret;
	}

	gnss_luatos_air530z_init_pipe(dev);

	ret = gnss_luatos_air530z_init_chat(dev);
	if (ret < 0) {
		return ret;
	}

	luatos_air530z_init_dynamic_script(dev);

	ret = modem_pipe_open(data->uart_pipe, K_SECONDS(10));
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->uart_pipe);
	if (ret < 0) {
		modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		return ret;
	}

	ret = modem_chat_run_script(&data->chat, &init_script);
	if (ret < 0) {
		LOG_ERR("Failed to run init_script");
		modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		return ret;
	}

	/* setup on-off gpio for power management */
	if (!gpio_is_ready_dt(&config->on_off_gpio)) {
		LOG_ERR("on-off GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->on_off_gpio, GPIO_OUTPUT_HIGH);

	return 0;
}

static int luatos_air530z_pm_resume(const struct device *dev)
{
	struct gnss_luatos_air530z_data *data = dev->data;
	int ret;

	ret = modem_pipe_open(data->uart_pipe, K_SECONDS(10));
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->uart_pipe);
	if (ret < 0) {
		modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		return ret;
	}

	ret = modem_chat_run_script(&data->chat, &init_script);
	if (ret < 0) {
		modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		return ret;
	}

	return 0;
}

static int luatos_air530z_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct gnss_luatos_air530z_data *data = dev->data;
	const struct gnss_luatos_air530z_config *config = dev->config;
	int ret = -ENOTSUP;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		gpio_pin_set_dt(&config->on_off_gpio, 0);
		ret = modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		break;

	case PM_DEVICE_ACTION_RESUME:
		gpio_pin_set_dt(&config->on_off_gpio, 1);
		ret = luatos_air530z_pm_resume(dev);
		break;

	default:
		break;
	}

	return ret;
}

static int luatos_air530z_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	struct gnss_luatos_air530z_data *data = dev->data;
	int ret;

	if (fix_interval_ms < 100 || fix_interval_ms > 1000) {
		return -EINVAL;
	}

	luatos_air530z_lock(dev);

	ret = gnss_nmea0183_snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
				    "PCAS02,%u", fix_interval_ms);

	data->dynamic_script_chat.request_size = ret;

	ret = modem_chat_run_script(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		goto unlock_return;
	}

unlock_return:
	luatos_air530z_unlock(dev);
	return ret;
}

static int luatos_air530z_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	struct gnss_luatos_air530z_data *data = dev->data;
	gnss_systems_t supported_systems;
	uint8_t encoded_systems = 0;
	int ret;

	supported_systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS | GNSS_SYSTEM_BEIDOU);

	if ((~supported_systems) & systems) {
		return -EINVAL;
	}

	luatos_air530z_lock(dev);

	WRITE_BIT(encoded_systems, 0, systems & GNSS_SYSTEM_GPS);
	WRITE_BIT(encoded_systems, 1, systems & GNSS_SYSTEM_GLONASS);
	WRITE_BIT(encoded_systems, 2, systems & GNSS_SYSTEM_BEIDOU);

	ret = gnss_nmea0183_snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
					"PCAS04,%u", encoded_systems);
	if (ret < 0) {
		goto unlock_return;
	}

	data->dynamic_script_chat.request_size = ret;

	ret = modem_chat_run_script(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		goto unlock_return;
	}

unlock_return:
	luatos_air530z_unlock(dev);
	return ret;

}

static int luatos_air530z_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	*systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS | GNSS_SYSTEM_BEIDOU);
	return 0;
}

static const struct gnss_driver_api gnss_api = {
	.set_fix_rate = luatos_air530z_set_fix_rate,
	.set_enabled_systems = luatos_air530z_set_enabled_systems,
	.get_supported_systems = luatos_air530z_get_supported_systems,
};

#define LUATOS_AIR530Z(inst)									\
	static const struct gnss_luatos_air530z_config gnss_luatos_air530z_cfg_##inst = {	\
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.on_off_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, on_off_gpios, { 0 }),		\
	};											\
												\
	static struct gnss_luatos_air530z_data gnss_luatos_air530z_data_##inst = {		\
		.chat_delimiter = {'\r', '\n'},							\
		.dynamic_separators_buf = {',', '*'},						\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, luatos_air530z_pm_action);				\
												\
	DEVICE_DT_INST_DEFINE(inst, gnss_luatos_air530z_init,					\
		PM_DEVICE_DT_INST_GET(inst),							\
		&gnss_luatos_air530z_data_##inst,						\
		&gnss_luatos_air530z_cfg_##inst,						\
		POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY, &gnss_api);

DT_INST_FOREACH_STATUS_OKAY(LUATOS_AIR530Z)
