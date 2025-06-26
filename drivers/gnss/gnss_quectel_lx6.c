/*
 * Copyright (c) 2025 Nick Ward <nix.ward@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/drivers/gnss/quectel_lx6.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/modem/backend/quectel_i2c.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/chat.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "gnss_nmea0183.h"
#include "gnss_nmea0183_match.h"
#include "gnss_parse.h"

#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(quectel_lx6, CONFIG_GNSS_LOG_LEVEL);

#define SUPPORTED_SYSTEMS (GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS | \
	GNSS_SYSTEM_GALILEO | GNSS_SYSTEM_BEIDOU | GNSS_SYSTEM_QZSS)

#define QUECTEL_LX6_SCRIPT_TIMEOUT_S 10U

#define QUECTEL_LX6_PMTK_NAV_MODE_NORMAL	0
#define QUECTEL_LX6_PMTK_NAV_MODE_FITNESS	1
#define QUECTEL_LX6_PMTK_NAV_MODE_AVIATION	2
#define QUECTEL_LX6_PMTK_NAV_MODE_BALLOON	3 /* Unused high-altitude purposes */
#define QUECTEL_LX6_PMTK_NAV_MODE_STATIONARY	4

#define QUECTEL_LX6_PMTK_PPS_CONFIG_DISABLED			0
#define QUECTEL_LX6_PMTK_PPS_CONFIG_ENABLED_AFTER_FIRST_FIX	1
#define QUECTEL_LX6_PMTK_PPS_CONFIG_ENABLED_3D_FIX_ONLY		2
#define QUECTEL_LX6_PMTK_PPS_CONFIG_ENABLED_2D_3D_FIX_ONLY	3
#define QUECTEL_LX6_PMTK_PPS_CONFIG_ALWAYS			4

struct quectel_lx6_config {
	bool i2c_bus;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec vcc;
	const enum gnss_pps_mode pps_mode;
	const uint16_t pps_pulse_width;
	union {
		struct modem_backend_quectel_i2c_config i2c_config;
		struct modem_backend_uart_config uart_config;
	} backend;
};

struct quectel_lx6_data {
	struct gnss_nmea0183_match_data match_data;
#if CONFIG_GNSS_SATELLITES
	struct gnss_satellite satellites[CONFIG_GNSS_QUECTEL_LX6_SAT_ARRAY_SIZE];
#endif

	union {
		struct modem_backend_quectel_i2c i2c;
		struct modem_backend_uart uart;
	} backend;

	struct modem_pipe *pipe;
	uint8_t backend_receive_buf[CONFIG_GNSS_QUECTEL_LX6_RX_BUF_SIZE];
	uint8_t backend_transmit_buf[CONFIG_GNSS_QUECTEL_LX6_TX_BUF_SIZE];

	struct modem_chat chat;
	uint8_t chat_receive_buf[256];
	uint8_t chat_delimiter[2];
	uint8_t *chat_argv[32];

	uint8_t pmtk_request_buf[64];
	uint8_t pmtk_match_buf[32];
	struct modem_chat_match pmtk_match;
	struct modem_chat_script_chat pmtk_script_chat;
	struct modem_chat_script pmtk_script;

	struct k_sem sem;

#if CONFIG_GNSS_QUECTEL_LX6_RESET_ON_INIT
	bool oneshot_reset;
#endif
};

/* System message - Startup */
MODEM_CHAT_MATCH_DEFINE(pmtk104_success_match, "$PMTK010,001*2E", "", NULL);
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	full_cold_start_script_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP("$PMTK104*37", pmtk104_success_match)
);
MODEM_CHAT_SCRIPT_NO_ABORT_DEFINE(full_cold_start_script, full_cold_start_script_cmds,
				  NULL, QUECTEL_LX6_SCRIPT_TIMEOUT_S);

MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
	MODEM_CHAT_MATCH_WILDCARD("$??GGA,", ",*", gnss_nmea0183_match_gga_callback),
	MODEM_CHAT_MATCH_WILDCARD("$??RMC,", ",*", gnss_nmea0183_match_rmc_callback),
#if CONFIG_GNSS_SATELLITES
	MODEM_CHAT_MATCH_WILDCARD("$??GSV,", ",*", gnss_nmea0183_match_gsv_callback),
#endif
);

static int quectel_lx6_configure_pps(const struct device *dev)
{
	uint8_t pps_mode = QUECTEL_LX6_PMTK_PPS_CONFIG_DISABLED;
	const struct quectel_lx6_config *config = dev->config;
	struct quectel_lx6_data *data = dev->data;
	int ret;

	switch (config->pps_mode) {
	case GNSS_PPS_MODE_DISABLED:
		pps_mode = QUECTEL_LX6_PMTK_PPS_CONFIG_DISABLED;
		break;

	case GNSS_PPS_MODE_ENABLED:
		pps_mode = QUECTEL_LX6_PMTK_PPS_CONFIG_ALWAYS;
		break;

	case GNSS_PPS_MODE_ENABLED_AFTER_LOCK:
		pps_mode = QUECTEL_LX6_PMTK_PPS_CONFIG_ENABLED_AFTER_FIRST_FIX;
		break;

	case GNSS_PPS_MODE_ENABLED_WHILE_LOCKED:
		return -ENOTSUP;
	}

	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK285,%u,%u", pps_mode, config->pps_pulse_width);
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		return ret;
	}

	ret = gnss_nmea0183_snprintk(data->pmtk_match_buf, sizeof(data->pmtk_match_buf),
				     "PMTK001,285,3");
	if (ret < 0) {
		return ret;
	}

	ret = modem_chat_match_set_match(&data->pmtk_match, data->pmtk_match_buf);
	if (ret < 0) {
		return ret;
	}

	return modem_chat_run_script(&data->chat, &data->pmtk_script);
}

int quectel_lx6_cold_start(const struct device *dev)
{
	const struct quectel_lx6_config *config = dev->config;
	struct quectel_lx6_data *data = dev->data;
	int ret;

	ret = modem_chat_run_script(&data->chat, &full_cold_start_script);
	if (ret < 0) {
		LOG_ERR("Failed to full cold restart GNSS: %d", ret);
		modem_pipe_close(data->pipe, K_SECONDS(10));
		return ret;
	}

	if (config->i2c_bus) {
		/* Closing pipe while LX6 is not responsive to I2C commands */
		modem_pipe_close(data->pipe, K_SECONDS(10));

		k_sleep(K_SECONDS(1));

		ret = modem_pipe_open(data->pipe, K_SECONDS(10));
		if (ret < 0) {
			LOG_ERR("Failed to open modem pipe: %d", ret);
			return ret;
		}
	} else {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}

static int quectel_lx6_resume(const struct device *dev)
{
	const struct quectel_lx6_config *config = dev->config;
	struct quectel_lx6_data *data = dev->data;
	int ret;

	LOG_DBG("Resume");

	if (config->vcc.port != NULL) {
		ret = gpio_pin_configure_dt(&config->vcc, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to set VCC high: %d", ret);
			return ret;
		}
		k_msleep(250);
	} else {
		/* TODO add check that either VCC or FORCE_ON are defined */
		return -ENOTSUP;
	}

	ret = modem_pipe_open(data->pipe, K_SECONDS(10));
	if (ret < 0) {
		LOG_ERR("Failed to open modem pipe: %d", ret);
		return ret;
	}

	ret = modem_chat_attach(&data->chat, data->pipe);
	if (ret < 0) {
		LOG_ERR("Failed to attach chat: %d", ret);
		modem_pipe_close(data->pipe, K_SECONDS(10));
		return ret;
	}

	ret = quectel_lx6_configure_pps(dev);
	if (ret < 0) {
		LOG_ERR("Failed to configure PPS: %d", ret);
		modem_pipe_close(data->pipe, K_SECONDS(10));
		return ret;
	}

	return ret;
}

static int quectel_lx6_suspend(const struct device *dev)
{
	const struct quectel_lx6_config *config = dev->config;
	struct quectel_lx6_data *data = dev->data;
	int ret;

	LOG_DBG("Suspend");

	if (config->vcc.port != NULL) {
		ret = gpio_pin_configure_dt(&config->vcc, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("[%s] VCC pin active", dev->name);
		}
	} else {
		/* TODO add check that either VCC or FORCE_ON are defined */
		return -ENOTSUP;
	}

	modem_pipe_close(data->pipe, K_SECONDS(10));

	return ret;
}

#if CONFIG_GNSS_QUECTEL_LX6_RESET_ON_INIT
static int quectel_lx6_gpio_reset(const struct device *dev)
{
	const struct quectel_lx6_config *config = dev->config;
	int ret;

	if ((config->reset.port != NULL) && (config->vcc.port != NULL)) {
		ret = gpio_pin_set_dt(&config->vcc, 1);
		if (ret < 0) {
			LOG_ERR("[%s] couldn't config VCC", dev->name);
			return ret;
		}
		ret = gpio_pin_set_dt(&config->reset, 0); /* Inactive is high */
		if (ret < 0) {
			LOG_ERR("Failed to inactivate reset pin: %d", ret);
		}
		k_msleep(4);                              /* > 2ms */
		ret = gpio_pin_set_dt(&config->reset, 1); /* Active is low */
		if (ret < 0) {
			LOG_ERR("Failed to activate reset pin: %d", ret);
		}
		k_msleep(12);                             /* Pulldown > 10ms */
		ret = gpio_pin_set_dt(&config->reset, 0); /* Inactive is high */
		if (ret < 0) {
			LOG_ERR("Failed to inactivate reset pin: %d", ret);
		}
	} else {
		LOG_WRN("[%s] couldn't reset", dev->name);
	}

	return ret;
}
#endif

static int quectel_lx6_turn_on(const struct device *dev)
{
	const struct quectel_lx6_config *config = dev->config;
	struct quectel_lx6_data *data = dev->data;
	int ret;

	LOG_INF("Turn on");

	if (config->vcc.port != NULL) {
		ret = gpio_pin_configure_dt(&config->vcc, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("[%s] couldn't activate VCC", dev->name);
			return ret;
		}
	}

#if CONFIG_GNSS_QUECTEL_LX6_RESET_ON_INIT
	if (!data->oneshot_reset) {
		data->oneshot_reset = true;
		quectel_lx6_gpio_reset(dev);
	}
#endif

	k_msleep(250);

	ret = modem_pipe_open(data->pipe, K_SECONDS(10));
	if (ret < 0) {
		LOG_ERR("Failed to open modem pipe: %d", ret);
	}

	return ret;
}

static int quectel_lx6_turn_off(const struct device *dev)
{
	const struct quectel_lx6_config *config = dev->config;
	struct quectel_lx6_data *data = dev->data;
	int ret;

	LOG_INF("Turn off");

	if (config->vcc.port != NULL) {
		ret = gpio_pin_configure_dt(&config->vcc, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("[%s] couldn't avoid back powering VCC", dev->name);
			return ret;
		}
	}

	return modem_pipe_close(data->pipe, K_SECONDS(10));
}

static int quectel_lx6_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct quectel_lx6_data *data = dev->data;
	int ret = -ENOTSUP;

	(void)k_sem_take(&data->sem, K_FOREVER);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = quectel_lx6_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = quectel_lx6_resume(dev);
		break;

	case PM_DEVICE_ACTION_TURN_ON:
		ret = quectel_lx6_turn_on(dev);
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		ret = quectel_lx6_turn_off(dev);
		break;

	default:
		break;
	}

	(void)k_sem_give(&data->sem);

	return ret;
}

static int quectel_lx6_set_fix_rate(const struct device *dev, uint32_t fix_interval_ms)
{
	struct quectel_lx6_data *data = dev->data;
	int ret;

	if (IN_RANGE(fix_interval_ms, 100, 999)) {
		/* TODO limit fix interval depending on UART baudrate */
		return -ENOSYS;
	}

	/* Full range of lx6 */
	if (!IN_RANGE(fix_interval_ms, 100, 10000)) {
		return -EINVAL;
	}

	if (fix_interval_ms > 1000) {
		if (fix_interval_ms % 1000 != 0) {
			return -EINVAL;
		}
	}

	(void)k_sem_take(&data->sem, K_FOREVER);

	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK220,%u", fix_interval_ms);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = gnss_nmea0183_snprintk(data->pmtk_match_buf, sizeof(data->pmtk_match_buf),
				     "PMTK001,220,3,%u", fix_interval_ms);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_match_set_match(&data->pmtk_match, data->pmtk_match_buf);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_run_script(&data->chat, &data->pmtk_script);
	if (ret < 0) {
		goto out_unlock;
	}

out_unlock:
	(void)k_sem_give(&data->sem);

	return ret;
}

static int quectel_lx6_set_navigation_mode(const struct device *dev,
					     enum gnss_navigation_mode mode)
{
	struct quectel_lx6_data *data = dev->data;
	uint8_t navigation_mode = 0;
	int ret;

	switch (mode) {
	case GNSS_NAVIGATION_MODE_ZERO_DYNAMICS:
		navigation_mode = QUECTEL_LX6_PMTK_NAV_MODE_STATIONARY;
		break;

	case GNSS_NAVIGATION_MODE_LOW_DYNAMICS:
		navigation_mode = QUECTEL_LX6_PMTK_NAV_MODE_FITNESS;
		break;

	case GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS:
		navigation_mode = QUECTEL_LX6_PMTK_NAV_MODE_NORMAL;
		break;

	case GNSS_NAVIGATION_MODE_HIGH_DYNAMICS:
		navigation_mode = QUECTEL_LX6_PMTK_NAV_MODE_AVIATION;
		break;
	}

	(void)k_sem_take(&data->sem, K_FOREVER);

	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK886,%u", navigation_mode);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = gnss_nmea0183_snprintk(data->pmtk_match_buf, sizeof(data->pmtk_match_buf),
				     "PMTK001,886,3");
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_match_set_match(&data->pmtk_match, data->pmtk_match_buf);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_run_script(&data->chat, &data->pmtk_script);
	if (ret < 0) {
		goto out_unlock;
	}

out_unlock:
	(void)k_sem_give(&data->sem);

	return ret;
}

static int quectel_lx6_set_enabled_systems(const struct device *dev, gnss_systems_t systems)
{
	struct quectel_lx6_data *data = dev->data;
	gnss_systems_t supported_systems;
	char msg[12];
	int ret;

	supported_systems = SUPPORTED_SYSTEMS;

	if (((~supported_systems) & systems) != 0) {
		LOG_ERR("Unsupported system");
		return -EINVAL;
	}

#define UNSUPPORTED_COMBO0 (GNSS_SYSTEM_GLONASS | GNSS_SYSTEM_BEIDOU)
#define UNSUPPORTED_COMBO1 (GNSS_SYSTEM_GALILEO | GNSS_SYSTEM_BEIDOU)

	if ((UNSUPPORTED_COMBO0 & systems) == UNSUPPORTED_COMBO0) {
		LOG_ERR("GLONASS and BDS cannot be enabled at the same time");
		return -EINVAL;
	}

	if ((UNSUPPORTED_COMBO1 & systems) == UNSUPPORTED_COMBO1) {
		LOG_ERR("GALILEO and BDS cannot be enabled at the same time");
		return -EINVAL;
	}

	(void)k_sem_take(&data->sem, K_FOREVER);

	ret = snprintf(msg, sizeof(msg), "%u,%u,%u,0,%u",
		       (0 < (systems & GNSS_SYSTEM_GPS)),
		       (0 < (systems & GNSS_SYSTEM_GLONASS)),
		       (0 < (systems & GNSS_SYSTEM_GALILEO)),
		       (0 < (systems & GNSS_SYSTEM_BEIDOU)));
	if (ret < 0) {
		goto out_unlock;
	}

	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK353,%s", msg);
				     /* Note GNSS_SYSTEM_QZSS needs to be handled elsewhere */
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = gnss_nmea0183_snprintk(data->pmtk_match_buf, sizeof(data->pmtk_match_buf),
				     "PMTK001,353,3,%s", msg);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_match_set_match(&data->pmtk_match, data->pmtk_match_buf);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_run_script(&data->chat, &data->pmtk_script);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = gnss_nmea0183_snprintk(data->pmtk_request_buf, sizeof(data->pmtk_request_buf),
				     "PMTK351,%u", (0 < (systems & GNSS_SYSTEM_QZSS)));
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_script_chat_set_request(&data->pmtk_script_chat, data->pmtk_request_buf);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = gnss_nmea0183_snprintk(data->pmtk_match_buf, sizeof(data->pmtk_match_buf),
				     "PMTK001,351,3");
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_match_set_match(&data->pmtk_match, data->pmtk_match_buf);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = modem_chat_run_script(&data->chat, &data->pmtk_script);
	if (ret < 0) {
		goto out_unlock;
	}

out_unlock:
	(void)k_sem_give(&data->sem);

	return ret;
}

static inline bool search_mode_enabled(const char *arg)
{
	return arg[0] == '1';
}

static int quectel_lx6_get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	*systems = SUPPORTED_SYSTEMS;

	return 0;
}

static DEVICE_API(gnss, gnss_api) = {
	.set_fix_rate = quectel_lx6_set_fix_rate,
	.get_fix_rate = NULL, /* not supported */
	.set_navigation_mode = quectel_lx6_set_navigation_mode,
	.get_navigation_mode = NULL, /* not supported */
	.set_enabled_systems = quectel_lx6_set_enabled_systems,
	.get_enabled_systems = NULL, /* not supported */
	.get_supported_systems = quectel_lx6_get_supported_systems,
};

static int quectel_lx6_init_nmea0183_match(const struct device *dev)
{
	struct quectel_lx6_data *data = dev->data;

	const struct gnss_nmea0183_match_config config = {
		.gnss = dev,
#if CONFIG_GNSS_SATELLITES
		.satellites = data->satellites,
		.satellites_size = ARRAY_SIZE(data->satellites),
#endif
	};

	return gnss_nmea0183_match_init(&data->match_data, &config);
}

static void quectel_lx6_init_pipe(const struct device *dev)
{
	const struct quectel_lx6_config *config = dev->config;
	struct quectel_lx6_data *data = dev->data;

	if (config->i2c_bus) {
#ifdef CONFIG_MODEM_BACKEND_QUECTEL_I2C
		data->pipe = modem_backend_quectel_i2c_init(&data->backend.i2c,
							    &config->backend.i2c_config);
#endif
	} else {
#ifdef CONFIG_MODEM_BACKEND_UART
		data->pipe = modem_backend_uart_init(&data->backend.uart,
						     &config->backend.uart_config);
#endif
	}
}

static void quectel_lx6_init_pmtk_script(const struct device *dev)
{
	struct quectel_lx6_data *data = dev->data;

	modem_chat_match_init(&data->pmtk_match);
	modem_chat_match_set_separators(&data->pmtk_match, ",*");

	modem_chat_script_chat_init(&data->pmtk_script_chat);
	modem_chat_script_chat_set_response_matches(&data->pmtk_script_chat,
						    &data->pmtk_match, 1);

	modem_chat_script_init(&data->pmtk_script);
	modem_chat_script_set_name(&data->pmtk_script, "pmtk");
	modem_chat_script_set_script_chats(&data->pmtk_script, &data->pmtk_script_chat, 1);
	modem_chat_script_set_abort_matches(&data->pmtk_script, NULL, 0);
	modem_chat_script_set_timeout(&data->pmtk_script, 10);
}

static int quectel_lx6_init(const struct device *dev)
{
	struct quectel_lx6_data *data = dev->data;
	int ret;

	k_sem_init(&data->sem, 1, 1);

#if CONFIG_GNSS_QUECTEL_LX6_RESET_ON_INIT
	data->oneshot_reset = false;
#endif

	ret = quectel_lx6_init_nmea0183_match(dev);
	if (ret < 0) {
		return ret;
	}

	quectel_lx6_init_pipe(dev);

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

	ret = modem_chat_init(&data->chat, &chat_config);
	if (ret < 0) {
		return ret;
	}

	quectel_lx6_init_pmtk_script(dev);

	return pm_device_driver_init(dev, quectel_lx6_pm_action);
}

#define LX6_INST_NAME(inst, name)							\
	_CONCAT(_CONCAT(_CONCAT(_CONCAT(name, _), DT_DRV_COMPAT), _), inst)

#define LX6_BACKEND_I2C_CONFIG(inst)							\
	.i2c = I2C_DT_SPEC_INST_GET(inst),						\
	.i2c_poll_interval_ms = CONFIG_GNSS_QUECTEL_LX6_I2C_POLL_MS,			\
	.receive_buf = LX6_INST_NAME(inst, data).backend_receive_buf,			\
	.receive_buf_size = ARRAY_SIZE(LX6_INST_NAME(inst, data).backend_receive_buf),  \
	.transmit_buf = LX6_INST_NAME(inst, data).backend_transmit_buf,			\
	.transmit_buf_size = ARRAY_SIZE(LX6_INST_NAME(inst, data).backend_transmit_buf)

#define LX6_BACKEND_UART_CONFIG(inst)							\
	.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
	.receive_buf = LX6_INST_NAME(inst, data).backend_receive_buf,			\
	.receive_buf_size = ARRAY_SIZE(LX6_INST_NAME(inst, data).backend_receive_buf),	\
	.transmit_buf = LX6_INST_NAME(inst, data).backend_transmit_buf,			\
	.transmit_buf_size = ARRAY_SIZE(LX6_INST_NAME(inst, data).backend_transmit_buf)

#define LX6_CONFIG_COMMON(inst)								\
	.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),			\
	.vcc = GPIO_DT_SPEC_INST_GET_OR(inst, vcc_gpios, {0}),				\
	.pps_mode = DT_INST_STRING_UPPER_TOKEN(inst, pps_mode),				\
	.pps_pulse_width = DT_INST_PROP(inst, pps_pulse_width)

#define LX6_CONFIG_I2C(inst)								\
	{										\
		.i2c_bus = true,							\
		.backend.i2c_config = { LX6_BACKEND_I2C_CONFIG(inst) },			\
		LX6_CONFIG_COMMON(inst)							\
	}

#define LX6_CONFIG_UART(inst)								\
	{										\
		.i2c_bus = false,							\
		.backend.uart_config = { LX6_BACKEND_UART_CONFIG(inst) },		\
		LX6_CONFIG_COMMON(inst)							\
	}

#define LX6_DEVICE(inst)								\
	static struct quectel_lx6_data LX6_INST_NAME(inst, data) = {			\
		.chat_delimiter = {'\r', '\n'},						\
	};										\
											\
	static const struct quectel_lx6_config LX6_INST_NAME(inst, config) =		\
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c),					\
			(LX6_CONFIG_I2C(inst)),						\
			(LX6_CONFIG_UART(inst)));					\
											\
	PM_DEVICE_DT_INST_DEFINE(inst, quectel_lx6_pm_action);				\
											\
	DEVICE_DT_INST_DEFINE(inst, quectel_lx6_init, PM_DEVICE_DT_INST_GET(inst),	\
			 &LX6_INST_NAME(inst, data), &LX6_INST_NAME(inst, config),	\
			 POST_KERNEL, CONFIG_GNSS_INIT_PRIORITY, &gnss_api);

#define DT_DRV_COMPAT quectel_l96
DT_INST_FOREACH_STATUS_OKAY(LX6_DEVICE)
#undef DT_DRV_COMPAT
