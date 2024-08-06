/*
 * Copyright (c) 2023 Trackunit Corporation
 * Copyright (c) 2023 Bjarki Arge Andreasen
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
#include <zephyr/pm/device_runtime.h>
#include <string.h>

#include "gnss_nmea0183.h"
#include "gnss_nmea0183_match.h"
#include "gnss_parse.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(quectel_lcx6g, CONFIG_GNSS_LOG_LEVEL);

#define QUECTEL_LCX6G_PM_TIMEOUT_MS    500U
#define QUECTEL_LCX6G_SCRIPT_TIMEOUT_S 10U

#define QUECTEL_LCX6G_PAIR_NAV_MODE_STATIONARY 4
#define QUECTEL_LCX6G_PAIR_NAV_MODE_FITNESS    1
#define QUECTEL_LCX6G_PAIR_NAV_MODE_NORMAL     0
#define QUECTEL_LCX6G_PAIR_NAV_MODE_DRONE      5

#define QUECTEL_LCX6G_PAIR_PPS_MODE_DISABLED             0
#define QUECTEL_LCX6G_PAIR_PPS_MODE_ENABLED              4
#define QUECTEL_LCX6G_PAIR_PPS_MODE_ENABLED_AFTER_LOCK   1
#define QUECTEL_LCX6G_PAIR_PPS_MODE_ENABLED_WHILE_LOCKED 2

struct quectel_lcx6g_config {
	const struct device *uart;
	const enum gnss_pps_mode pps_mode;
	const uint16_t pps_pulse_width;
};

struct quectel_lcx6g_data {
	struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_QUECTEL_LCX6G_SAT_ARRAY_SIZE];
#endif

	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_GNSS_QUECTEL_LCX6G_UART_RX_BUF_SIZE];
	uint8_t uart_backend_transmit_buf[CONFIG_GNSS_QUECTEL_LCX6G_UART_TX_BUF_SIZE];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[256];
	uint8_t chat_delimiter[2];
	uint8_t *chat_argv[32];

	/* Pair chat script */
	uint8_t pair_request_buf[32];
	uint8_t pair_match_buf[32];
	struct modem_chat_match pair_match;
	struct modem_chat_script_chat pair_script_chat;
	struct modem_chat_script pair_script;

	/* Allocation for responses from GNSS modem */
	union {
		uint16_t fix_rate_response;
		gnss_systems_t enabled_systems_response;
		enum gnss_navigation_mode navigation_mode_response;
	};

	struct k_sem lock;
	k_timeout_t pm_timeout;
};

#ifdef CONFIG_PM_DEVICE
MODEM_CHAT_MATCH_DEFINE(pair003_success_match, "$PAIR001,003,0*38", "", NULL);
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	suspend_script_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR003*39", pair003_success_match)
);

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(suspend_script, suspend_script_cmds,
				  NULL, QUECTEL_LCX6G_SCRIPT_TIMEOUT_S);
#endif /* CONFIG_PM_DEVICE */

MODEM_CHAT_MATCH_DEFINE(pair062_ack_match, "$PAIR001,062,0*3F", "", NULL);
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	resume_script_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR002*38", modem_chat_any_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR062,0,1*3F", pair062_ack_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR062,1,0*3F", pair062_ack_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR062,2,0*3C", pair062_ack_match),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR062,3,5*38", pair062_ack_match),
#else
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR062,3,0*3D", pair062_ack_match),
#endif
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR062,4,1*3B", pair062_ack_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("$PAIR062,5,0*3B", pair062_ack_match),
);

MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(resume_script, resume_script_cmds,
				  NULL, QUECTEL_LCX6G_SCRIPT_TIMEOUT_S);

MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
	MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
	MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);

static int quectel_lcx6g_configure_pps(const struct device *dev)
{
	const struct quectel_lcx6g_config *config = dev->config;
	struct quectel_lcx6g_data *data = dev->data;
	uint8_t pps_mode = 0;
	int ret;

	switch (config->pps_mode) {
	case GNSS_PPS_MODE_DISABLED:
		pps_mode = QUECTEL_LCX6G_PAIR_PPS_MODE_DISABLED;
		break;

	case GNSS_PPS_MODE_ENABLED:
		pps_mode = QUECTEL_LCX6G_PAIR_PPS_MODE_ENABLED;
		break;

	case GNSS_PPS_MODE_ENABLED_AFTER_LOCK:
		pps_mode = QUECTEL_LCX6G_PAIR_PPS_MODE_ENABLED_AFTER_LOCK;
		break;

	case GNSS_PPS_MODE_ENABLED_WHILE_LOCKED:
		pps_mode = QUECTEL_LCX6G_PAIR_PPS_MODE_ENABLED_WHILE_LOCKED;
		break;
	}

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR752,%u,%u", pps_mode, config->pps_pulse_width);
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		return ret;
	}

	ret = gnss_nmea0183_snprintk(data->pair_match_buf, sizeof(data->pair_match_buf),
				     "PAIR001,752,0");
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		return ret;
	}

	return modem_chat_run_script(&data->chat, &data->pair_script);
}

static void quectel_lcx6g_lock(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void quectel_lcx6g_unlock(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;

	k_sem_give(&data->lock);
}

static void quectel_lcx6g_pm_changed(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;
	uint32_t pm_ready_at_ms;

	pm_ready_at_ms = k_uptime_get() + QUECTEL_LCX6G_PM_TIMEOUT_MS;
	data->pm_timeout = K_TIMEOUT_ABS_MS(pm_ready_at_ms);
}

static void quectel_lcx6g_await_pm_ready(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;

	LOG_INF("Waiting until PM ready");
	k_sleep(data->pm_timeout);
}

static int quectel_lcx6g_resume(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;
	int ret;

	LOG_INF("Resuming");

	quectel_lcx6g_await_pm_ready(dev);

	ret = modem_pipe_open(data->uart_pipe, K_SECONDS(10));
	if (ret < 0) {
		LOG_ERR("Failed to open pipe");
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->uart_pipe);
	if (ret < 0) {
		LOG_ERR("Failed to attach chat");
		modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		return ret;
	}

	ret = modem_chat_run_script(&data->chat, &resume_script);
	if (ret < 0) {
		LOG_ERR("Failed to initialize GNSS");
		modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		return ret;
	}

	ret = quectel_lcx6g_configure_pps(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure PPS");
		modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		return ret;
	}

	LOG_INF("Resumed");
	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int quectel_lcx6g_suspend(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;
	int ret;

	LOG_INF("Suspending");

	quectel_lcx6g_await_pm_ready(dev);

	ret = modem_chat_run_script(&data->chat, &suspend_script);
	if (ret < 0) {
		LOG_ERR("Failed to suspend GNSS");
	} else {
		LOG_INF("Suspended");
	}

	modem_pipe_close(data->uart_pipe, K_SECONDS(10));
	return ret;
}

static void quectel_lcx6g_turn_on(const struct device *dev)
{
	LOG_INF("Powered on");
}

static int quectel_lcx6g_turn_off(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;

	LOG_INF("Powered off");

	return modem_pipe_close(data->uart_pipe, K_SECONDS(10));
}

static int quectel_lcx6g_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = -ENOTSUP;

	quectel_lcx6g_lock(dev);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = quectel_lcx6g_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = quectel_lcx6g_resume(dev);
		break;

	case PM_DEVICE_ACTION_TURN_ON:
		quectel_lcx6g_turn_on(dev);
		ret = 0;
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		ret = quectel_lcx6g_turn_off(dev);
		break;

	default:
		break;
	}

	quectel_lcx6g_pm_changed(dev);

	quectel_lcx6g_unlock(dev);
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int quectel_lcx6g_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	struct quectel_lcx6g_data *data = dev->data;
	int ret;

	if (fix_interval_ms < 100 || fix_interval_ms > 1000) {
		return -EINVAL;
	}

	quectel_lcx6g_lock(dev);

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR050,%u", fix_interval_ms);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = gnss_nmea0183_snprintk(data->pair_match_buf, sizeof(data->pair_match_buf),
				     "PAIR001,050,0");
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_run_script(&data->chat, &data->pair_script);
	if (ret < 0) {
		goto unlock_return;
	}

unlock_return:
	quectel_lcx6g_unlock(dev);
	return ret;
}

static void quectel_lcx6g_get_fix_rate_callback(struct modem_chat *chat, char **argv,
						uint16_t argc, void *user_data)
{
	struct quectel_lcx6g_data *data = user_data;
	int32_t tmp;

	if (argc != 3) {
		return;
	}

	if ((gnss_parse_atoi(argv[1], 10, &tmp) < 0) || (tmp < 0) || (tmp > 1000)) {
		return;
	}

	data->fix_rate_response = (uint16_t)tmp;
}

static int quectel_lcx6g_get_fix_rate(const struct device *dev, uint32_t *fix_interval_ms)
{
	struct quectel_lcx6g_data *data = dev->data;
	int ret;

	quectel_lcx6g_lock(dev);

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR051");
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	strncpy(data->pair_match_buf, "$PAIR051,", sizeof(data->pair_match_buf));
	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	modem_chat_match_set_callback(&data->pair_match, quectel_lcx6g_get_fix_rate_callback);
	ret = modem_chat_run_script(&data->chat, &data->pair_script);
	modem_chat_match_set_callback(&data->pair_match, NULL);
	if (ret < 0) {
		goto unlock_return;
	}

	*fix_interval_ms = data->fix_rate_response;

unlock_return:
	quectel_lcx6g_unlock(dev);
	return 0;
}

static int quectel_lcx6g_set_navigation_mode(const struct device *dev,
					     enum gnss_navigation_mode mode)
{
	struct quectel_lcx6g_data *data = dev->data;
	uint8_t navigation_mode = 0;
	int ret;

	switch (mode) {
	case GNSS_NAVIGATION_MODE_ZERO_DYNAMICS:
		navigation_mode = QUECTEL_LCX6G_PAIR_NAV_MODE_STATIONARY;
		break;

	case GNSS_NAVIGATION_MODE_LOW_DYNAMICS:
		navigation_mode = QUECTEL_LCX6G_PAIR_NAV_MODE_FITNESS;
		break;

	case GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS:
		navigation_mode = QUECTEL_LCX6G_PAIR_NAV_MODE_NORMAL;
		break;

	case GNSS_NAVIGATION_MODE_HIGH_DYNAMICS:
		navigation_mode = QUECTEL_LCX6G_PAIR_NAV_MODE_DRONE;
		break;
	}

	quectel_lcx6g_lock(dev);

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR080,%u", navigation_mode);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = gnss_nmea0183_snprintk(data->pair_match_buf, sizeof(data->pair_match_buf),
				     "PAIR001,080,0");
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_run_script(&data->chat, &data->pair_script);
	if (ret < 0) {
		goto unlock_return;
	}

unlock_return:
	quectel_lcx6g_unlock(dev);
	return ret;
}

static void quectel_lcx6g_get_nav_mode_callback(struct modem_chat *chat, char **argv,
						uint16_t argc, void *user_data)
{
	struct quectel_lcx6g_data *data = user_data;
	int32_t tmp;

	if (argc != 3) {
		return;
	}

	if ((gnss_parse_atoi(argv[1], 10, &tmp) < 0) || (tmp < 0) || (tmp > 7)) {
		return;
	}

	switch (tmp) {
	case QUECTEL_LCX6G_PAIR_NAV_MODE_FITNESS:
		data->navigation_mode_response = GNSS_NAVIGATION_MODE_LOW_DYNAMICS;
		break;

	case QUECTEL_LCX6G_PAIR_NAV_MODE_STATIONARY:
		data->navigation_mode_response = GNSS_NAVIGATION_MODE_ZERO_DYNAMICS;
		break;

	case QUECTEL_LCX6G_PAIR_NAV_MODE_DRONE:
		data->navigation_mode_response = GNSS_NAVIGATION_MODE_HIGH_DYNAMICS;
		break;

	default:
		data->navigation_mode_response = GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS;
		break;
	}
}

static int quectel_lcx6g_get_navigation_mode(const struct device *dev,
					     enum gnss_navigation_mode *mode)
{
	struct quectel_lcx6g_data *data = dev->data;
	int ret;

	quectel_lcx6g_lock(dev);

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR081");
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	strncpy(data->pair_match_buf, "$PAIR081,", sizeof(data->pair_match_buf));
	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	modem_chat_match_set_callback(&data->pair_match, quectel_lcx6g_get_nav_mode_callback);
	ret = modem_chat_run_script(&data->chat, &data->pair_script);
	modem_chat_match_set_callback(&data->pair_match, NULL);
	if (ret < 0) {
		goto unlock_return;
	}

	*mode = data->navigation_mode_response;

unlock_return:
	quectel_lcx6g_unlock(dev);
	return ret;
}

static int quectel_lcx6g_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	struct quectel_lcx6g_data *data = dev->data;
	gnss_systems_t supported_systems;
	int ret;

	supported_systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS | GNSS_SYSTEM_GALILEO |
			     GNSS_SYSTEM_BEIDOU | GNSS_SYSTEM_QZSS | GNSS_SYSTEM_SBAS);

	if ((~supported_systems) & systems) {
		return -EINVAL;
	}

	quectel_lcx6g_lock(dev);

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR066,%u,%u,%u,%u,%u,0",
				     (0 < (systems & GNSS_SYSTEM_GPS)),
				     (0 < (systems & GNSS_SYSTEM_GLONASS)),
				     (0 < (systems & GNSS_SYSTEM_GALILEO)),
				     (0 < (systems & GNSS_SYSTEM_BEIDOU)),
				     (0 < (systems & GNSS_SYSTEM_QZSS)));
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = gnss_nmea0183_snprintk(data->pair_match_buf, sizeof(data->pair_match_buf),
				     "PAIR001,066,0");
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_run_script(&data->chat, &data->pair_script);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR410,%u", (0 < (systems & GNSS_SYSTEM_SBAS)));
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = gnss_nmea0183_snprintk(data->pair_match_buf, sizeof(data->pair_match_buf),
				     "PAIR001,410,0");
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_run_script(&data->chat, &data->pair_script);
	if (ret < 0) {
		goto unlock_return;
	}

unlock_return:
	quectel_lcx6g_unlock(dev);
	return ret;
}

static inline bool search_mode_enabled(const char *arg)
{
	return arg[0] == '1';
}

static void quectel_lcx6g_get_search_mode_callback(struct modem_chat *chat, char **argv,
						   uint16_t argc, void *user_data)
{
	struct quectel_lcx6g_data *data = user_data;

	if (argc != 8) {
		return;
	}

	data->enabled_systems_response = search_mode_enabled(argv[1]) ? GNSS_SYSTEM_GPS : 0;
	data->enabled_systems_response |= search_mode_enabled(argv[2]) ? GNSS_SYSTEM_GLONASS : 0;
	data->enabled_systems_response |= search_mode_enabled(argv[3]) ? GNSS_SYSTEM_GALILEO : 0;
	data->enabled_systems_response |= search_mode_enabled(argv[4]) ? GNSS_SYSTEM_BEIDOU : 0;
	data->enabled_systems_response |= search_mode_enabled(argv[5]) ? GNSS_SYSTEM_QZSS : 0;
}

static void quectel_lcx6g_get_sbas_status_callback(struct modem_chat *chat, char **argv,
						   uint16_t argc, void *user_data)
{
	struct quectel_lcx6g_data *data = user_data;

	if (argc != 3) {
		return;
	}

	data->enabled_systems_response |= ('1' == argv[1][0]) ? GNSS_SYSTEM_SBAS : 0;
}


static int quectel_lcx6g_get_enabled_systems(const struct device *dev, gnss_systems_t *systems)
{
	struct quectel_lcx6g_data *data = dev->data;
	int ret;

	quectel_lcx6g_lock(dev);

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR067");
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	strncpy(data->pair_match_buf, "$PAIR067,", sizeof(data->pair_match_buf));
	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	modem_chat_match_set_callback(&data->pair_match, quectel_lcx6g_get_search_mode_callback);
	ret = modem_chat_run_script(&data->chat, &data->pair_script);
	modem_chat_match_set_callback(&data->pair_match, NULL);
	if (ret < 0) {
		goto unlock_return;
	}

	ret = gnss_nmea0183_snprintk(data->pair_request_buf, sizeof(data->pair_request_buf),
				     "PAIR411");
	if (ret < 0) {
		goto unlock_return;
	}

	ret = modem_chat_script_chat_set_request(&data->pair_script_chat, data->pair_request_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	strncpy(data->pair_match_buf, "$PAIR411,", sizeof(data->pair_match_buf));
	ret = modem_chat_match_set_match(&data->pair_match, data->pair_match_buf);
	if (ret < 0) {
		goto unlock_return;
	}

	modem_chat_match_set_callback(&data->pair_match, quectel_lcx6g_get_sbas_status_callback);
	ret = modem_chat_run_script(&data->chat, &data->pair_script);
	modem_chat_match_set_callback(&data->pair_match, NULL);
	if (ret < 0) {
		goto unlock_return;
	}

	*systems = data->enabled_systems_response;

unlock_return:
	quectel_lcx6g_unlock(dev);
	return ret;
}

static int quectel_lcx6g_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	*systems = (GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS | GNSS_SYSTEM_GALILEO |
		    GNSS_SYSTEM_BEIDOU | GNSS_SYSTEM_QZSS | GNSS_SYSTEM_SBAS);
	return 0;
}

static const struct gnss_driver_api gnss_api = {
	.set_fix_rate = quectel_lcx6g_set_fix_rate,
	.get_fix_rate = quectel_lcx6g_get_fix_rate,
	.set_navigation_mode = quectel_lcx6g_set_navigation_mode,
	.get_navigation_mode = quectel_lcx6g_get_navigation_mode,
	.set_enabled_systems = quectel_lcx6g_set_enabled_systems,
	.get_enabled_systems = quectel_lcx6g_get_enabled_systems,
	.get_supported_systems = quectel_lcx6g_get_supported_systems,
};

static int quectel_lcx6g_init_nmea0183_match(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;

	const struct gnss_nmea0183_match_config config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = data->satellites,
		.satellites_size = ARRAY_SIZE(data->satellites),
#endif
	};

	return gnss_nmea0183_match_init(&data->match_data, &config);
}

static void quectel_lcx6g_init_pipe(const struct device *dev)
{
	const struct quectel_lcx6g_config *config = dev->config;
	struct quectel_lcx6g_data *data = dev->data;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = config->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = ARRAY_SIZE(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

static int quectel_lcx6g_init_chat(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;

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

static void quectel_lcx6g_init_pair_script(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;

	modem_chat_match_init(&data->pair_match);
	modem_chat_match_set_separators(&data->pair_match, ",*");

	modem_chat_script_chat_init(&data->pair_script_chat);
	modem_chat_script_chat_set_response_matches(&data->pair_script_chat,
						    &data->pair_match, 1);

	modem_chat_script_init(&data->pair_script);
	modem_chat_script_set_name(&data->pair_script, "pair");
	modem_chat_script_set_script_chats(&data->pair_script, &data->pair_script_chat, 1);
	modem_chat_script_set_abort_matches(&data->pair_script, NULL, 0);
	modem_chat_script_set_timeout(&data->pair_script, 10);
}

static int quectel_lcx6g_init(const struct device *dev)
{
	struct quectel_lcx6g_data *data = dev->data;
	int ret;

	k_sem_init(&data->lock, 1, 1);

	ret = quectel_lcx6g_init_nmea0183_match(dev);
	if (ret < 0) {
		return ret;
	}

	quectel_lcx6g_init_pipe(dev);

	ret = quectel_lcx6g_init_chat(dev);
	if (ret < 0) {
		return ret;
	}

	quectel_lcx6g_init_pair_script(dev);

	quectel_lcx6g_pm_changed(dev);

	if (pm_device_is_powered(dev)) {
		ret = quectel_lcx6g_resume(dev);
		if (ret < 0) {
			return ret;
		}
		quectel_lcx6g_pm_changed(dev);
	} else {
		pm_device_init_off(dev);
	}

	return pm_device_runtime_enable(dev);
}

#define LCX6G_INST_NAME(inst, name) \
	_CONCAT(_CONCAT(_CONCAT(name, _), DT_DRV_COMPAT), inst)

#define LCX6G_DEVICE(inst)								\
	static const struct quectel_lcx6g_config LCX6G_INST_NAME(inst, config) = {	\
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),				\
		.pps_mode = DT_INST_STRING_UPPER_TOKEN(inst, pps_mode),			\
		.pps_pulse_width = DT_INST_PROP(inst, pps_pulse_width),			\
	};										\
											\
	static struct quectel_lcx6g_data LCX6G_INST_NAME(inst, data) = {		\
		.chat_delimiter = {'\r', '\n'},						\
	};										\
											\
	PM_DEVICE_DT_INST_DEFINE(inst, quectel_lcx6g_pm_action);			\
											\
	DEVICE_DT_INST_DEFINE(inst, quectel_lcx6g_init, PM_DEVICE_DT_INST_GET(inst),	\
			 &LCX6G_INST_NAME(inst, data), &LCX6G_INST_NAME(inst, config),	\
			 POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY, &gnss_api);

#define DT_DRV_COMPAT quectel_lc26g
DT_INST_FOREACH_STATUS_OKAY(LCX6G_DEVICE)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT quectel_lc76g
DT_INST_FOREACH_STATUS_OKAY(LCX6G_DEVICE)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT quectel_lc86g
DT_INST_FOREACH_STATUS_OKAY(LCX6G_DEVICE)
#undef DT_DRV_COMPAT
