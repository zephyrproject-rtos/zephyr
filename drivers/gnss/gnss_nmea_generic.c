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

#include <zephyr/drivers/counter.h>

/* DT_DRV_COMPAT is only defined further down, so name the compatible here. */
#define HAS_TIMEPULSE DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(gnss_nmea_generic, counter_captures)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gnss_nmea_generic, CONFIG_GNSS_LOG_LEVEL);

#define UART_RX_BUF_SZ   (256 + IS_ENABLED(CONFIG_GNSS_SATELLITES) * 512)
#define UART_TX_BUF_SZ   64
#define CHAT_RECV_BUF_SZ 256
#define CHAT_ARGV_SZ     32

struct gnss_nmea_generic_config {
	const struct device *uart;
	const struct modem_chat_script *const init_chat_script;
	struct gpio_dt_spec enable_gpio;
	struct gpio_dt_spec reset_gpio;
	uint32_t reset_pulse_duration_ms;
	uint32_t startup_time_ms;
#if HAS_TIMEPULSE
	struct counter_capture_dt_spec timepulse;
#endif
};

struct gnss_nmea_generic_data {
#if HAS_TIMEPULSE
	atomic_t timepulse;
#endif
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

MODEM_CHAT_MATCHES_DEFINE(
	unsol_matches, MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
	MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);

#if HAS_TIMEPULSE
/*
 * A capture reports the ticks of whichever counter took it, which is the
 * kernel clock only when the capture came from a kernel clock counter. Anchor
 * it instead: read both clocks now and step back by however long ago the
 * capture was, so the answer is in kernel ticks whatever took it.
 */
static k_ticks_t gnss_nmea_generic_uptime_of(const struct counter_capture_dt_spec *spec,
					     uint32_t captured)
{
	k_ticks_t now = k_uptime_ticks();
	uint32_t counter_now;
	uint32_t elapsed;
	uint32_t freq;
	uint32_t top;
	uint64_t ago_us;

	if (counter_get_value(spec->dev, &counter_now) < 0) {
		return now;
	}

	freq = counter_get_frequency(spec->dev);
	if (freq == 0U) {
		return now;
	}

	/* A counter wraps at its top value, which is not always the whole
	 * range of the tick type, so take the difference around that.
	 */
	top = counter_get_top_value(spec->dev);
	if (counter_now >= captured) {
		elapsed = counter_now - captured;
	} else {
		elapsed = (top - captured) + counter_now + 1U;
	}

	ago_us = ((uint64_t)elapsed * USEC_PER_SEC) / freq;

	return now - (k_ticks_t)k_us_to_ticks_floor64(ago_us);
}

static void gnss_nmea_generic_timepulse_cb(const struct device *counter_dev, uint8_t chan_id,
					   counter_capture_flags_t flags, uint32_t ticks,
					   void *user_data)
{
	const struct device *dev = user_data;
	const struct gnss_nmea_generic_config *cfg = dev->config;
	struct gnss_nmea_generic_data *data = dev->data;

	ARG_UNUSED(counter_dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(flags);

	atomic_set(&data->timepulse,
		   (atomic_val_t)gnss_nmea_generic_uptime_of(&cfg->timepulse, ticks));
}

static int gnss_nmea_generic_get_latest_timepulse(const struct device *dev, k_ticks_t *timestamp)
{
	const struct gnss_nmea_generic_config *cfg = dev->config;
	struct gnss_nmea_generic_data *data = dev->data;

	if (cfg->timepulse.dev == NULL) {
		return -ENOSYS;
	}

	*timestamp = (k_ticks_t)atomic_get(&data->timepulse);

	return 0;
}

static int gnss_nmea_generic_timepulse_init(const struct device *dev)
{
	const struct gnss_nmea_generic_config *cfg = dev->config;
	int ret;

	if (cfg->timepulse.dev == NULL) {
		return 0;
	}

	if (!device_is_ready(cfg->timepulse.dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->timepulse.dev);
		return -ENODEV;
	}

	ret = counter_capture_configure_dt(&cfg->timepulse, gnss_nmea_generic_timepulse_cb,
					   (void *)dev);
	if (ret < 0) {
		LOG_ERR("Could not configure the timepulse capture: %d", ret);
	}

	return ret;
}

/* The pulse says something about a receiver that is running, so follow the
 * device rather than keep timestamping a receiver that was suspended.
 */
static int gnss_nmea_generic_timepulse_set(const struct device *dev, bool enable)
{
	const struct gnss_nmea_generic_config *cfg = dev->config;

	if (cfg->timepulse.dev == NULL) {
		return 0;
	}

	if (enable) {
		return counter_enable_capture(cfg->timepulse.dev, cfg->timepulse.chan_id);
	}

	return counter_disable_capture(cfg->timepulse.dev, cfg->timepulse.chan_id);
}
#endif /* HAS_TIMEPULSE */
static int gnss_nmea_generic_enable(const struct device *dev, bool enable)
{
	const struct gnss_nmea_generic_config *cfg = dev->config;
	int ret;

	if (cfg->enable_gpio.port == NULL) {
		return 0;
	}

	ret = gpio_pin_set_dt(&cfg->enable_gpio, enable);
	if (ret != 0) {
		return ret;
	}

	if (enable && (cfg->startup_time_ms > 0U)) {
		k_sleep(K_MSEC(cfg->startup_time_ms));
	}

	return 0;
}

static int gnss_nmea_generic_init_gpios(const struct device *dev)
{
	const struct gnss_nmea_generic_config *cfg = dev->config;
	int ret;

	if (cfg->enable_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->enable_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->enable_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			return ret;
		}
	}

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			return ret;
		}

		k_sleep(K_MSEC(cfg->reset_pulse_duration_ms));
		return gpio_pin_set_dt(&cfg->reset_gpio, 0);
	}

	return 0;
}

static int gnss_nmea_generic_resume(const struct device *dev)
{
	const struct gnss_nmea_generic_config *cfg = dev->config;
	struct gnss_nmea_generic_data *data = dev->data;
	int ret;
	int cleanup_ret;

	ret = gnss_nmea_generic_enable(dev, true);
	if (ret != 0) {
		return ret;
	}

	ret = modem_pipe_open(data->uart_pipe, K_SECONDS(10));
	if (ret < 0) {
		goto disable;
	}

	ret = modem_chat_attach(&data->chat, data->uart_pipe);

	if (ret == 0) {
		ret = modem_chat_run_script(&data->chat, cfg->init_chat_script);
	}

	if (ret < 0) {
		modem_chat_release(&data->chat);
		cleanup_ret = modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		if (cleanup_ret < 0) {
			LOG_ERR("Failed to close UART pipe: %d", cleanup_ret);
		}
		goto disable;
	}

#if HAS_TIMEPULSE
	ret = gnss_nmea_generic_timepulse_set(dev, true);
	if (ret < 0) {
		modem_chat_release(&data->chat);
		cleanup_ret = modem_pipe_close(data->uart_pipe, K_SECONDS(10));
		if (cleanup_ret < 0) {
			LOG_ERR("Failed to close UART pipe: %d", cleanup_ret);
		}
		goto disable;
	}
#endif

	return 0;

disable:
	cleanup_ret = gnss_nmea_generic_enable(dev, false);
	if (cleanup_ret != 0) {
		LOG_ERR("Failed to disable receiver: %d", cleanup_ret);
	}

	return ret;
}

#if CONFIG_PM_DEVICE
static int gnss_nmea_generic_suspend(const struct device *dev)
{
	struct gnss_nmea_generic_data *data = dev->data;
	int ret;
	int restore_ret;

	ret = modem_pipe_close(data->uart_pipe, K_SECONDS(10));
	if (ret < 0) {
		return ret;
	}

	ret = gnss_nmea_generic_enable(dev, false);
	if (ret != 0) {
		restore_ret = modem_pipe_open(data->uart_pipe, K_SECONDS(10));
		if (restore_ret < 0) {
			LOG_ERR("Failed to reopen UART pipe: %d", restore_ret);
		}
		return ret;
	}

#if HAS_TIMEPULSE
	(void)gnss_nmea_generic_timepulse_set(dev, false);
#endif

	modem_chat_release(&data->chat);
	return 0;
}
#endif

static DEVICE_API(gnss, gnss_api) = {
#if HAS_TIMEPULSE
	.get_latest_timepulse = gnss_nmea_generic_get_latest_timepulse,
#endif
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

	ret = gnss_nmea_generic_init_gpios(dev);
	if (ret != 0) {
		return ret;
	}

	ret = gnss_nmea_generic_init_nmea0183_match(dev);
	if (ret < 0) {
		return ret;
	}

	gnss_nmea_generic_init_pipe(dev);

	ret = gnss_nmea_generic_init_chat(dev);
	if (ret < 0) {
		return ret;
	}

#if HAS_TIMEPULSE
	ret = gnss_nmea_generic_timepulse_init(dev);
	if (ret < 0) {
		return ret;
	}
#endif

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
	case PM_DEVICE_ACTION_SUSPEND:
		return gnss_nmea_generic_suspend(dev);
	default:
		return -ENOTSUP;
	}
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(gnss_nmea_generic)
MODEM_CHAT_SCRIPT_EMPTY_DEFINE(gnss_nmea_generic_init_chat_script);
#endif

#define GNSS_NMEA_GENERIC(inst)								\
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(inst, reset_gpios) ||                                 \
		     (DT_INST_PROP_OR(inst, reset_pulse_duration_ms, 0) > 0),                     \
		     "reset-gpios requires a positive reset-pulse-duration-ms");                  \
	static const struct gnss_nmea_generic_config gnss_nmea_generic_cfg_##inst = {	\
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),				\
		.init_chat_script = &_CONCAT(DT_DRV_COMPAT, _init_chat_script),         \
		.enable_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, enable_gpios, {0}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                   \
		.reset_pulse_duration_ms =                                                        \
			DT_INST_PROP_OR(inst, reset_pulse_duration_ms, 0),                        \
		.startup_time_ms = DT_INST_PROP(inst, startup_time_ms),                           \
		IF_ENABLED(HAS_TIMEPULSE,                                                         \
			   (.timepulse = COUNTER_CAPTURE_DT_SPEC_INST_GET_OR(                     \
				    inst, counter_captures, {0}),))                               \
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
