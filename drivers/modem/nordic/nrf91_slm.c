/*
 * Copyright 2025 Nova Dynamics LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf91_slm

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf91_slm, CONFIG_MODEM_LOG_LEVEL);

#include <string.h>
#include <stdlib.h>

#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>

#include "nrf91_slm.h"

#define PERIODIC_SCRIPT_TIMEOUT K_MSEC(CONFIG_MODEM_NRF91_SLM_PERIODIC_SCRIPT_MS)

/* Magic constants */
#define CSQ_RSSI_UNKNOWN  (99)
#define CESQ_RSRP_UNKNOWN (255)
#define CESQ_RSRQ_UNKNOWN (255)

/* Magic numbers to units conversions */
#define CSQ_RSSI_TO_DB(v)  (-113 + (2 * (rssi)))
#define CESQ_RSRP_TO_DB(v) (-140 + (v))
#define CESQ_RSRQ_TO_DB(v) (-20 + ((v) / 2))

enum nrf91_slm_event {
	NRF91_SLM_EVENT_RESUME = 0,
	NRF91_SLM_EVENT_SUSPEND,
	NRF91_SLM_EVENT_SCRIPT_SUCCESS,
	NRF91_SLM_EVENT_SCRIPT_FAILED,
	NRF91_SLM_EVENT_TIMEOUT,
	NRF91_SLM_EVENT_REGISTERED,
	NRF91_SLM_EVENT_DEREGISTERED,
	NRF91_SLM_EVENT_PPP_CONNECTED,
	NRF91_SLM_EVENT_PPP_DISCONNECTED,
};

struct nrf91_slm_config {
	const struct device *uart;
	struct gpio_dt_spec power_gpio;
	struct gpio_dt_spec reset_gpio;
	uint16_t power_pulse_duration_ms;
	uint16_t reset_pulse_duration_ms;
	uint16_t startup_time_ms;
	uint16_t shutdown_time_ms;
	bool autostarts;
};

static struct nrf91_slm_data mdata;

static const struct nrf91_slm_config mconfig = {
	.uart = DEVICE_DT_GET(DT_INST_BUS(0)),
	.power_gpio = GPIO_DT_SPEC_INST_GET_OR(0, mdm_power_gpios, {}),
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(0, mdm_reset_gpios, {}),
	.power_pulse_duration_ms = 100,
	.reset_pulse_duration_ms = 100,
	.startup_time_ms = 2000,
	.shutdown_time_ms = 10000,
	.autostarts = false,
};

/******************************************************************************
 * Logging functions
 *****************************************************************************/

#if defined(CONFIG_LOG) && (CONFIG_MODEM_LOG_LEVEL == LOG_LEVEL_DBG)

static const char *nrf91_slm_state_str(enum nrf91_slm_state state)
{
	switch (state) {
	case NRF91_SLM_STATE_IDLE:
		return "idle";
	case NRF91_SLM_STATE_RESET_PULSE:
		return "reset pulse";
	case NRF91_SLM_STATE_POWER_ON_PULSE:
		return "power pulse";
	case NRF91_SLM_STATE_AWAIT_POWER_ON:
		return "await power on";
	case NRF91_SLM_STATE_RUN_INIT_SCRIPT:
		return "run init script";
	case NRF91_SLM_STATE_AWAIT_REGISTERED:
		return "await registered";
	case NRF91_SLM_STATE_DISCONNECT_PPP:
		return "disconnect ppp";
	case NRF91_SLM_STATE_RUN_DIAL_SCRIPT:
		return "run dial script";
	case NRF91_SLM_STATE_CARRIER_ON:
		return "carrier on";
	case NRF91_SLM_STATE_INIT_POWER_OFF:
		return "init power off";
	case NRF91_SLM_STATE_POWER_OFF_PULSE:
		return "power off pulse";
	case NRF91_SLM_STATE_AWAIT_POWER_OFF:
		return "await power off";
	}

	return "";
}

static const char *nrf91_slm_event_str(enum nrf91_slm_event event)
{
	switch (event) {
	case NRF91_SLM_EVENT_RESUME:
		return "resume";
	case NRF91_SLM_EVENT_SUSPEND:
		return "suspend";
	case NRF91_SLM_EVENT_SCRIPT_SUCCESS:
		return "script success";
	case NRF91_SLM_EVENT_SCRIPT_FAILED:
		return "script failed";
	case NRF91_SLM_EVENT_TIMEOUT:
		return "timeout";
	case NRF91_SLM_EVENT_REGISTERED:
		return "registered";
	case NRF91_SLM_EVENT_DEREGISTERED:
		return "deregistered";
	case NRF91_SLM_EVENT_PPP_CONNECTED:
		return "ppp connected";
	case NRF91_SLM_EVENT_PPP_DISCONNECTED:
		return "ppp disconnected";
	}

	return "";
}

static void nrf91_slm_log_state_changed(enum nrf91_slm_state last_state,
					enum nrf91_slm_state new_state)
{
	LOG_DBG("switch from %s to %s", nrf91_slm_state_str(last_state),
		nrf91_slm_state_str(new_state));
}

static void nrf91_slm_log_event(enum nrf91_slm_event evt)
{
	LOG_DBG("event %s", nrf91_slm_event_str(evt));
}

#endif

/******************************************************************************
 * Helper functions
 *****************************************************************************/

static bool nrf91_slm_gpio_is_enabled(const struct gpio_dt_spec *gpio)
{
	return gpio->port != NULL;
}

static void nrf91_slm_start_timer(struct nrf91_slm_data *data, k_timeout_t timeout)
{
	k_work_schedule(&data->timeout_work, timeout);
}

static void nrf91_slm_stop_timer(struct nrf91_slm_data *data)
{
	k_work_cancel_delayable(&data->timeout_work);
}

static void nrf91_slm_try_run_script(struct nrf91_slm_data *data,
				     const struct modem_chat_script *script)
{
	int ret = k_mutex_lock(&data->chat_lock, K_NO_WAIT);

	if (ret == 0) {
		ret = modem_chat_run_script_async(&data->chat, script);
		k_mutex_unlock(&data->chat_lock);
	}

	if (ret < 0) {
		nrf91_slm_start_timer(data, PERIODIC_SCRIPT_TIMEOUT);
	}
}

static void nrf91_slm_delegate_event(struct nrf91_slm_data *data, enum nrf91_slm_event evt)
{
	k_mutex_lock(&data->event_rb_lock, K_FOREVER);
	ring_buf_put(&data->event_rb, (uint8_t *)&evt, 1);
	k_mutex_unlock(&data->event_rb_lock);
	k_work_submit(&data->event_dispatch_work);
}

static bool nrf91_slm_is_registered(struct nrf91_slm_data *data)
{
	return (data->registration_status_gsm == CELLULAR_REGISTRATION_REGISTERED_HOME) ||
	       (data->registration_status_gsm == CELLULAR_REGISTRATION_REGISTERED_ROAMING) ||
	       (data->registration_status_gprs == CELLULAR_REGISTRATION_REGISTERED_HOME) ||
	       (data->registration_status_gprs == CELLULAR_REGISTRATION_REGISTERED_ROAMING) ||
	       (data->registration_status_lte == CELLULAR_REGISTRATION_REGISTERED_HOME) ||
	       (data->registration_status_lte == CELLULAR_REGISTRATION_REGISTERED_ROAMING);
}

/******************************************************************************
 * Modem chat callbacks
 *****************************************************************************/

static void nrf91_slm_chat_callback_handler(struct modem_chat *chat,
					    enum modem_chat_script_result result, void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_SCRIPT_SUCCESS);
	} else {
		nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_SCRIPT_FAILED);
	}
}

static void nrf91_slm_chat_on_imei(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->imei, argv[1], sizeof(data->imei) - 1);
}

static void nrf91_slm_chat_on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->model_id, argv[1], sizeof(data->model_id) - 1);
}

static void nrf91_slm_chat_on_cgmi(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->manufacturer, argv[1], sizeof(data->manufacturer) - 1);
}

static void nrf91_slm_chat_on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->fw_version, argv[1], sizeof(data->fw_version) - 1);
}

static void nrf91_slm_chat_on_csq(struct modem_chat *chat, char **argv, uint16_t argc,
				  void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (argc != 3) {
		return;
	}

	data->rssi = (uint8_t)atoi(argv[1]);
}

static void nrf91_slm_chat_on_cesq(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;

	if (argc != 7) {
		return;
	}

	data->rsrq = (uint8_t)atoi(argv[5]);
	data->rsrp = (uint8_t)atoi(argv[6]);
}

static void nrf91_slm_chat_on_cxreg(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;
	enum cellular_registration_status registration_status = 0;

	/* This receives both +C*REG? read command answers and unsolicited notifications.
	 * Their syntax differs in that the former has one more parameter, <n>, which is first.
	 */
	if (argc >= 3 && argv[2][0] != '"') {
		/* +CEREG: <n>,<stat>[,<tac>[...]] */
		registration_status = atoi(argv[2]);
	} else if (argc >= 2) {
		/* +CEREG: <stat>[,<tac>[...]] */
		registration_status = atoi(argv[1]);
	} else {
		return;
	}

	if (strcmp(argv[0], "+CREG: ") == 0) {
		data->registration_status_gsm = registration_status;
	} else if (strcmp(argv[0], "+CGREG: ") == 0) {
		data->registration_status_gprs = registration_status;
	} else { /* CEREG */
		data->registration_status_lte = registration_status;
	}

	if (nrf91_slm_is_registered(data)) {
		nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_REGISTERED);
	} else {
		nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_DEREGISTERED);
	}
}

static void nrf91_slm_chat_on_xppp(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)user_data;
	int status = atoi(argv[1]);

	if (status) {
		nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_PPP_CONNECTED);
	} else {
		nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_PPP_DISCONNECTED);
	}
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(allow_match,
				MODEM_CHAT_MATCH("OK", "", NULL),
				MODEM_CHAT_MATCH("ERROR", "", NULL));

MODEM_CHAT_MATCH_DEFINE(imei_match, "", "", nrf91_slm_chat_on_imei);
MODEM_CHAT_MATCH_DEFINE(cgmm_match, "", "", nrf91_slm_chat_on_cgmm);
MODEM_CHAT_MATCH_DEFINE(cgmi_match, "", "", nrf91_slm_chat_on_cgmi);
MODEM_CHAT_MATCH_DEFINE(cgmr_match, "", "", nrf91_slm_chat_on_cgmr);
MODEM_CHAT_MATCH_DEFINE(csq_match, "+CSQ: ", ",", nrf91_slm_chat_on_csq);
MODEM_CHAT_MATCH_DEFINE(cesq_match, "+CESQ: ", ",", nrf91_slm_chat_on_cesq);

MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
				MODEM_CHAT_MATCH("+CREG: ", ",", nrf91_slm_chat_on_cxreg),
				MODEM_CHAT_MATCH("+CEREG: ", ",", nrf91_slm_chat_on_cxreg),
				MODEM_CHAT_MATCH("+CGREG: ", ",", nrf91_slm_chat_on_cxreg),
				MODEM_CHAT_MATCH("#XPPP: ", ",", nrf91_slm_chat_on_xppp));

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("ERROR", "", NULL));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_init_chat_script_cmds,
				MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT", allow_match),
				MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CMEE=0", allow_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_init_chat_script, nordic_nrf91_slm_init_chat_script_cmds,
			 abort_matches, nrf91_slm_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_dial_chat_script_cmds,
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_dial_chat_script, nordic_nrf91_slm_dial_chat_script_cmds,
			 abort_matches, nrf91_slm_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_periodic_chat_script_cmds,
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_periodic_chat_script,
			nordic_nrf91_slm_periodic_chat_script_cmds, abort_matches,
			nrf91_slm_chat_callback_handler, 4);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_ppp_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#XPPP=0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#XPPP?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_ppp_chat_script, nordic_nrf91_slm_ppp_chat_script_cmds,
			 abort_matches, nrf91_slm_chat_callback_handler, 10);

/******************************************************************************
 * Modem state machine
 *****************************************************************************/

static void nrf91_slm_enter_state(struct nrf91_slm_data *data, enum nrf91_slm_state state);

static void nrf91_slm_timeout_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct nrf91_slm_data *data = CONTAINER_OF(dwork, struct nrf91_slm_data, timeout_work);

	nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_TIMEOUT);
}

static void nrf91_slm_begin_power_off_pulse(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	if (nrf91_slm_gpio_is_enabled(&config->power_gpio)) {
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_POWER_OFF_PULSE);
	} else {
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_IDLE);
	}
}

static int nrf91_slm_on_idle_state_enter(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;
	int ret;

	if (nrf91_slm_gpio_is_enabled(&config->reset_gpio)) {
		gpio_pin_set_dt(&config->reset_gpio, 1);
	}

	modem_chat_release(&data->chat);
	ret = modem_pipe_close(data->uart_pipe, K_SECONDS(2));
	if (ret < 0) {
		LOG_ERR("failed to close pipe");
	}

	data->registration_status_gsm = CELLULAR_REGISTRATION_NOT_REGISTERED;
	data->registration_status_gprs = CELLULAR_REGISTRATION_NOT_REGISTERED;
	data->registration_status_lte = CELLULAR_REGISTRATION_NOT_REGISTERED;

	k_sem_give(&data->suspended_sem);
	return ret;
}

static void nrf91_slm_idle_event_handler(struct nrf91_slm_data *data, enum nrf91_slm_event evt)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;
	int ret;

	switch (evt) {
	case NRF91_SLM_EVENT_RESUME:
		ret = modem_pipe_open(data->uart_pipe, K_SECONDS(2));
		if (ret < 0) {
			LOG_ERR("failed to open pipe");
			return;
		}

		modem_chat_attach(&data->chat, data->uart_pipe);

		if (config->autostarts) {
			nrf91_slm_enter_state(data, NRF91_SLM_STATE_AWAIT_POWER_ON);
			break;
		}

		if (nrf91_slm_gpio_is_enabled(&config->power_gpio)) {
			nrf91_slm_enter_state(data, NRF91_SLM_STATE_POWER_ON_PULSE);
			break;
		}

		if (nrf91_slm_gpio_is_enabled(&config->reset_gpio)) {
			nrf91_slm_enter_state(data, NRF91_SLM_STATE_AWAIT_POWER_ON);
			break;
		}

		nrf91_slm_enter_state(data, NRF91_SLM_STATE_RUN_INIT_SCRIPT);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		k_sem_give(&data->suspended_sem);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_idle_state_leave(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	k_sem_take(&data->suspended_sem, K_NO_WAIT);

	if (nrf91_slm_gpio_is_enabled(&config->reset_gpio)) {
		gpio_pin_set_dt(&config->reset_gpio, 0);
	}

	return 0;
}

static int nrf91_slm_on_reset_pulse_state_enter(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	gpio_pin_set_dt(&config->reset_gpio, 1);
	nrf91_slm_start_timer(data, K_MSEC(config->reset_pulse_duration_ms));
	return 0;
}

static void nrf91_slm_reset_pulse_event_handler(struct nrf91_slm_data *data,
						enum nrf91_slm_event evt)
{
	switch (evt) {
	case NRF91_SLM_EVENT_TIMEOUT:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_AWAIT_POWER_ON);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_reset_pulse_state_leave(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	gpio_pin_set_dt(&config->reset_gpio, 0);
	nrf91_slm_stop_timer(data);
	return 0;
}

static int nrf91_slm_on_power_on_pulse_state_enter(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 1);
	nrf91_slm_start_timer(data, K_MSEC(config->power_pulse_duration_ms));
	return 0;
}

static void nrf91_slm_power_on_pulse_event_handler(struct nrf91_slm_data *data,
						   enum nrf91_slm_event evt)
{
	switch (evt) {
	case NRF91_SLM_EVENT_TIMEOUT:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_AWAIT_POWER_ON);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_power_on_pulse_state_leave(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 0);
	nrf91_slm_stop_timer(data);
	return 0;
}

static int nrf91_slm_on_await_power_on_state_enter(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	nrf91_slm_start_timer(data, K_MSEC(config->startup_time_ms));
	return 0;
}

static void nrf91_slm_await_power_on_event_handler(struct nrf91_slm_data *data,
						   enum nrf91_slm_event evt)
{
	switch (evt) {
	case NRF91_SLM_EVENT_TIMEOUT:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_RUN_INIT_SCRIPT);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_run_init_script_state_enter(struct nrf91_slm_data *data)
{
	nrf91_slm_try_run_script(data, &nordic_nrf91_slm_init_chat_script);
	return 0;
}

static void nrf91_slm_run_init_script_event_handler(struct nrf91_slm_data *data,
						    enum nrf91_slm_event evt)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	switch (evt) {
	case NRF91_SLM_EVENT_SCRIPT_SUCCESS:
		net_if_set_link_addr(data->netif, data->imei, ARRAY_SIZE(data->imei),
				     NET_LINK_UNKNOWN);

		nrf91_slm_enter_state(data, NRF91_SLM_STATE_RUN_DIAL_SCRIPT);
		break;

	case NRF91_SLM_EVENT_TIMEOUT:
		nrf91_slm_try_run_script(data, &nordic_nrf91_slm_init_chat_script);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_IDLE);
		break;

	case NRF91_SLM_EVENT_SCRIPT_FAILED:
		if (nrf91_slm_gpio_is_enabled(&config->power_gpio)) {
			nrf91_slm_enter_state(data, NRF91_SLM_STATE_POWER_ON_PULSE);
			break;
		}

		if (nrf91_slm_gpio_is_enabled(&config->reset_gpio)) {
			nrf91_slm_enter_state(data, NRF91_SLM_STATE_RESET_PULSE);
			break;
		}

		nrf91_slm_enter_state(data, NRF91_SLM_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_run_dial_script_state_enter(struct nrf91_slm_data *data)
{
	/* Allow modem time to enter command mode before running dial script */
	nrf91_slm_start_timer(data, K_MSEC(100));
	return 0;
}

static void nrf91_slm_run_dial_script_event_handler(struct nrf91_slm_data *data,
						    enum nrf91_slm_event evt)
{
	switch (evt) {
	case NRF91_SLM_EVENT_SCRIPT_SUCCESS:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_AWAIT_REGISTERED);
		break;

	case NRF91_SLM_EVENT_TIMEOUT:
		nrf91_slm_try_run_script(data, &nordic_nrf91_slm_dial_chat_script);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_await_registered_state_enter(struct nrf91_slm_data *data)
{
	nrf91_slm_start_timer(data, PERIODIC_SCRIPT_TIMEOUT);
	return 0;
}

static void nrf91_slm_await_registered_event_handler(struct nrf91_slm_data *data,
						     enum nrf91_slm_event evt)
{
	switch (evt) {
	case NRF91_SLM_EVENT_SCRIPT_SUCCESS:
	case NRF91_SLM_EVENT_SCRIPT_FAILED:
		nrf91_slm_start_timer(data, PERIODIC_SCRIPT_TIMEOUT);
		break;

	case NRF91_SLM_EVENT_TIMEOUT:
		nrf91_slm_try_run_script(data, &nordic_nrf91_slm_periodic_chat_script);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_INIT_POWER_OFF);
		break;

	case NRF91_SLM_EVENT_REGISTERED:
	case NRF91_SLM_EVENT_PPP_CONNECTED:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_DISCONNECT_PPP);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_disconnect_ppp_state_enter(struct nrf91_slm_data *data)
{
	nrf91_slm_try_run_script(data, &nordic_nrf91_slm_ppp_chat_script);
	return 0;
}

static void nrf91_slm_disconnect_ppp_event_handler(struct nrf91_slm_data *data,
						   enum nrf91_slm_event evt)
{
	switch (evt) {
	case NRF91_SLM_EVENT_SCRIPT_SUCCESS:
		nrf91_slm_start_timer(data, PERIODIC_SCRIPT_TIMEOUT);
		break;

	case NRF91_SLM_EVENT_TIMEOUT:
		nrf91_slm_try_run_script(data, &nordic_nrf91_slm_ppp_chat_script);
		break;

	case NRF91_SLM_EVENT_REGISTERED:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_CARRIER_ON);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_INIT_POWER_OFF);
		break;

	case NRF91_SLM_EVENT_SCRIPT_FAILED:
	case NRF91_SLM_EVENT_PPP_DISCONNECTED:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_CARRIER_ON);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_await_registered_state_leave(struct nrf91_slm_data *data)
{
	nrf91_slm_stop_timer(data);
	return 0;
}

static int nrf91_slm_on_carrier_on_state_enter(struct nrf91_slm_data *data)
{
	net_if_carrier_on(data->netif);
	nrf91_slm_start_timer(data, PERIODIC_SCRIPT_TIMEOUT);
	return 0;
}

static void nrf91_slm_carrier_on_event_handler(struct nrf91_slm_data *data,
					       enum nrf91_slm_event evt)
{
	switch (evt) {
	case NRF91_SLM_EVENT_SCRIPT_SUCCESS:
	case NRF91_SLM_EVENT_SCRIPT_FAILED:
		nrf91_slm_start_timer(data, PERIODIC_SCRIPT_TIMEOUT);
		break;

	case NRF91_SLM_EVENT_TIMEOUT:
		nrf91_slm_try_run_script(data, &nordic_nrf91_slm_periodic_chat_script);
		break;

	case NRF91_SLM_EVENT_DEREGISTERED:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_RUN_DIAL_SCRIPT);
		break;

	case NRF91_SLM_EVENT_SUSPEND:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_INIT_POWER_OFF);
		break;

	case NRF91_SLM_EVENT_PPP_CONNECTED:
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_DISCONNECT_PPP);
		break;

	default:
		break;
	}
}

static int nrf91_slm_on_carrier_on_state_leave(struct nrf91_slm_data *data)
{
	nrf91_slm_stop_timer(data);
	net_if_carrier_off(data->netif);
	modem_chat_release(&data->chat);
	return 0;
}

static int nrf91_slm_on_init_power_off_state_enter(struct nrf91_slm_data *data)
{
	nrf91_slm_start_timer(data, K_MSEC(2000));
	return 0;
}

static void nrf91_slm_init_power_off_event_handler(struct nrf91_slm_data *data,
						   enum nrf91_slm_event evt)
{
	if (evt == NRF91_SLM_EVENT_TIMEOUT) {
		nrf91_slm_begin_power_off_pulse(data);
	}
}

static int nrf91_slm_on_power_off_pulse_state_enter(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 1);
	nrf91_slm_start_timer(data, K_MSEC(config->power_pulse_duration_ms));
	return 0;
}

static void nrf91_slm_power_off_pulse_event_handler(struct nrf91_slm_data *data,
						    enum nrf91_slm_event evt)
{
	if (evt == NRF91_SLM_EVENT_TIMEOUT) {
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_AWAIT_POWER_OFF);
	}
}

static int nrf91_slm_on_power_off_pulse_state_leave(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 0);
	nrf91_slm_stop_timer(data);
	return 0;
}

static int nrf91_slm_on_await_power_off_state_enter(struct nrf91_slm_data *data)
{
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)data->dev->config;

	nrf91_slm_start_timer(data, K_MSEC(config->shutdown_time_ms));
	return 0;
}

static void nrf91_slm_await_power_off_event_handler(struct nrf91_slm_data *data,
						    enum nrf91_slm_event evt)
{
	if (evt == NRF91_SLM_EVENT_TIMEOUT) {
		nrf91_slm_enter_state(data, NRF91_SLM_STATE_IDLE);
	}
}

static int nrf91_slm_on_state_enter(struct nrf91_slm_data *data)
{
	int ret;

	switch (data->state) {
	case NRF91_SLM_STATE_IDLE:
		ret = nrf91_slm_on_idle_state_enter(data);
		break;

	case NRF91_SLM_STATE_RESET_PULSE:
		ret = nrf91_slm_on_reset_pulse_state_enter(data);
		break;

	case NRF91_SLM_STATE_POWER_ON_PULSE:
		ret = nrf91_slm_on_power_on_pulse_state_enter(data);
		break;

	case NRF91_SLM_STATE_AWAIT_POWER_ON:
		ret = nrf91_slm_on_await_power_on_state_enter(data);
		break;

	case NRF91_SLM_STATE_RUN_INIT_SCRIPT:
		ret = nrf91_slm_on_run_init_script_state_enter(data);
		break;

	case NRF91_SLM_STATE_RUN_DIAL_SCRIPT:
		ret = nrf91_slm_on_run_dial_script_state_enter(data);
		break;

	case NRF91_SLM_STATE_AWAIT_REGISTERED:
		ret = nrf91_slm_on_await_registered_state_enter(data);
		break;

	case NRF91_SLM_STATE_DISCONNECT_PPP:
		ret = nrf91_slm_on_disconnect_ppp_state_enter(data);
		break;

	case NRF91_SLM_STATE_CARRIER_ON:
		ret = nrf91_slm_on_carrier_on_state_enter(data);
		break;

	case NRF91_SLM_STATE_INIT_POWER_OFF:
		ret = nrf91_slm_on_init_power_off_state_enter(data);
		break;

	case NRF91_SLM_STATE_POWER_OFF_PULSE:
		ret = nrf91_slm_on_power_off_pulse_state_enter(data);
		break;

	case NRF91_SLM_STATE_AWAIT_POWER_OFF:
		ret = nrf91_slm_on_await_power_off_state_enter(data);
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

static int nrf91_slm_on_state_leave(struct nrf91_slm_data *data)
{
	int ret;

	switch (data->state) {
	case NRF91_SLM_STATE_IDLE:
		ret = nrf91_slm_on_idle_state_leave(data);
		break;

	case NRF91_SLM_STATE_RESET_PULSE:
		ret = nrf91_slm_on_reset_pulse_state_leave(data);
		break;

	case NRF91_SLM_STATE_POWER_ON_PULSE:
		ret = nrf91_slm_on_power_on_pulse_state_leave(data);
		break;

	case NRF91_SLM_STATE_AWAIT_REGISTERED:
		ret = nrf91_slm_on_await_registered_state_leave(data);
		break;

	case NRF91_SLM_STATE_CARRIER_ON:
		ret = nrf91_slm_on_carrier_on_state_leave(data);
		break;

	case NRF91_SLM_STATE_POWER_OFF_PULSE:
		ret = nrf91_slm_on_power_off_pulse_state_leave(data);
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

static void nrf91_slm_enter_state(struct nrf91_slm_data *data, enum nrf91_slm_state state)
{
	int ret;

	ret = nrf91_slm_on_state_leave(data);

	if (ret < 0) {
		LOG_WRN("failed to leave state, error: %i", ret);

		return;
	}

	data->state = state;
	ret = nrf91_slm_on_state_enter(data);

	if (ret < 0) {
		LOG_WRN("failed to enter state error: %i", ret);
	}
}

static void nrf91_slm_event_handler(struct nrf91_slm_data *data, enum nrf91_slm_event evt)
{
	enum nrf91_slm_state state;

	state = data->state;

#if defined(CONFIG_LOG) && (CONFIG_MODEM_LOG_LEVEL == LOG_LEVEL_DBG)
	nrf91_slm_log_event(evt);
#endif

	switch (data->state) {
	case NRF91_SLM_STATE_IDLE:
		nrf91_slm_idle_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_RESET_PULSE:
		nrf91_slm_reset_pulse_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_POWER_ON_PULSE:
		nrf91_slm_power_on_pulse_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_AWAIT_POWER_ON:
		nrf91_slm_await_power_on_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_RUN_INIT_SCRIPT:
		nrf91_slm_run_init_script_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_RUN_DIAL_SCRIPT:
		nrf91_slm_run_dial_script_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_AWAIT_REGISTERED:
		nrf91_slm_await_registered_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_DISCONNECT_PPP:
		nrf91_slm_disconnect_ppp_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_CARRIER_ON:
		nrf91_slm_carrier_on_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_INIT_POWER_OFF:
		nrf91_slm_init_power_off_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_POWER_OFF_PULSE:
		nrf91_slm_power_off_pulse_event_handler(data, evt);
		break;

	case NRF91_SLM_STATE_AWAIT_POWER_OFF:
		nrf91_slm_await_power_off_event_handler(data, evt);
		break;
	default:
		LOG_WRN("unhandled state: %d", data->state);
		break;
	}

#if defined(CONFIG_LOG) && (CONFIG_MODEM_LOG_LEVEL == LOG_LEVEL_DBG)
	if (state != data->state) {
		nrf91_slm_log_state_changed(state, data->state);
	}
#endif
}

static void nrf91_slm_event_dispatch_handler(struct k_work *item)
{
	struct nrf91_slm_data *data =
		CONTAINER_OF(item, struct nrf91_slm_data, event_dispatch_work);

	uint8_t events[sizeof(data->event_buf)];
	uint8_t events_cnt;

	k_mutex_lock(&data->event_rb_lock, K_FOREVER);

	events_cnt = (uint8_t)ring_buf_get(&data->event_rb, events, sizeof(data->event_buf));

	k_mutex_unlock(&data->event_rb_lock);

	for (uint8_t i = 0; i < events_cnt; i++) {
		nrf91_slm_event_handler(data, (enum nrf91_slm_event)events[i]);
	}
}

/******************************************************************************
 * Cellular API
 *****************************************************************************/

MODEM_CHAT_SCRIPT_CMDS_DEFINE(get_signal_csq_chat_script_cmds,
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CSQ", csq_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(get_signal_csq_chat_script, get_signal_csq_chat_script_cmds, abort_matches,
			 nrf91_slm_chat_callback_handler, 2);

static inline int nrf91_slm_csq_parse_rssi(uint8_t rssi, int16_t *value)
{
	/* AT+CSQ returns a response +CSQ: <rssi>,<ber> where:
	 * - rssi is a integer from 0 to 31 whose values describes a signal strength
	 *   between -113 dBm for 0 and -51dbM for 31 or unknown for 99
	 * - ber is an integer from 0 to 7 that describes the error rate, it can also
	 *   be 99 for an unknown error rate
	 */
	if (rssi == CSQ_RSSI_UNKNOWN) {
		return -EINVAL;
	}

	*value = (int16_t)CSQ_RSSI_TO_DB(rssi);
	return 0;
}

MODEM_CHAT_SCRIPT_CMDS_DEFINE(get_signal_cesq_chat_script_cmds,
				MODEM_CHAT_SCRIPT_CMD_RESP("AT+CESQ", cesq_match),
				MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(get_signal_cesq_chat_script, get_signal_cesq_chat_script_cmds,
			 abort_matches, nrf91_slm_chat_callback_handler, 2);

static inline int nrf91_slm_cesq_parse_rsrp(uint8_t rsrp, int16_t *value)
{
	if (rsrp == CESQ_RSRP_UNKNOWN) {
		return -EINVAL;
	}

	*value = (int16_t)CESQ_RSRP_TO_DB(rsrp);
	return 0;
}

static inline int nrf91_slm_cesq_parse_rsrq(uint8_t rsrq, int16_t *value)
{
	if (rsrq == CESQ_RSRQ_UNKNOWN) {
		return -EINVAL;
	}

	*value = (int16_t)CESQ_RSRQ_TO_DB(rsrq);
	return 0;
}

static int nrf91_slm_get_signal(const struct device *dev, const enum cellular_signal_type type,
				int16_t *value)
{
	int ret = -ENOTSUP;
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)dev->data;

	if ((data->state != NRF91_SLM_STATE_AWAIT_REGISTERED) &&
	    (data->state != NRF91_SLM_STATE_CARRIER_ON)) {
		return -ENODATA;
	}

	/* Run chat script */
	switch (type) {
	case CELLULAR_SIGNAL_RSSI:
		ret = modem_chat_run_script(&data->chat, &get_signal_csq_chat_script);
		break;

	case CELLULAR_SIGNAL_RSRP:
	case CELLULAR_SIGNAL_RSRQ:
		ret = modem_chat_run_script(&data->chat, &get_signal_cesq_chat_script);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	/* Verify chat script ran successfully */
	if (ret < 0) {
		return ret;
	}

	/* Parse received value */
	switch (type) {
	case CELLULAR_SIGNAL_RSSI:
		ret = nrf91_slm_csq_parse_rssi(data->rssi, value);
		break;

	case CELLULAR_SIGNAL_RSRP:
		ret = nrf91_slm_cesq_parse_rsrp(data->rsrp, value);
		break;

	case CELLULAR_SIGNAL_RSRQ:
		ret = nrf91_slm_cesq_parse_rsrq(data->rsrq, value);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int nrf91_slm_get_modem_info(const struct device *dev, enum cellular_modem_info_type type,
				    char *info, size_t size)
{
	int ret = 0;
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)dev->data;

	switch (type) {
	case CELLULAR_MODEM_INFO_IMEI:
		strncpy(info, &data->imei[0], MIN(size, sizeof(data->imei)));
		break;
	case CELLULAR_MODEM_INFO_SIM_IMSI:
		strncpy(info, &data->imsi[0], MIN(size, sizeof(data->imsi)));
		break;
	case CELLULAR_MODEM_INFO_MANUFACTURER:
		strncpy(info, &data->manufacturer[0], MIN(size, sizeof(data->manufacturer)));
		break;
	case CELLULAR_MODEM_INFO_FW_VERSION:
		strncpy(info, &data->fw_version[0], MIN(size, sizeof(data->fw_version)));
		break;
	case CELLULAR_MODEM_INFO_MODEL_ID:
		strncpy(info, &data->model_id[0], MIN(size, sizeof(data->model_id)));
		break;
	case CELLULAR_MODEM_INFO_SIM_ICCID:
		strncpy(info, &data->iccid[0], MIN(size, sizeof(data->iccid)));
		break;
	default:
		ret = -ENODATA;
		break;
	}

	return ret;
}

static int nrf91_slm_get_registration_status(const struct device *dev,
					     enum cellular_access_technology tech,
					     enum cellular_registration_status *status)
{
	int ret = 0;
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)dev->data;

	if (data->state != NRF91_SLM_STATE_CARRIER_ON) {
		return -EAGAIN;
	}

	switch (tech) {
	case CELLULAR_ACCESS_TECHNOLOGY_GSM:
		*status = data->registration_status_gsm;
		break;
	case CELLULAR_ACCESS_TECHNOLOGY_GPRS:
	case CELLULAR_ACCESS_TECHNOLOGY_UMTS:
	case CELLULAR_ACCESS_TECHNOLOGY_EDGE:
		*status = data->registration_status_gprs;
		break;
	case CELLULAR_ACCESS_TECHNOLOGY_LTE:
	case CELLULAR_ACCESS_TECHNOLOGY_LTE_CAT_M1:
	case CELLULAR_ACCESS_TECHNOLOGY_LTE_CAT_M2:
	case CELLULAR_ACCESS_TECHNOLOGY_NB_IOT:
		*status = data->registration_status_lte;
		break;
	default:
		ret = -ENODATA;
		break;
	}

	return ret;
}

static DEVICE_API(cellular, nrf91_slm_api) = {
	.get_signal = nrf91_slm_get_signal,
	.get_modem_info = nrf91_slm_get_modem_info,
	.get_registration_status = nrf91_slm_get_registration_status,
};

/******************************************************************************
 * Power Management
 *****************************************************************************/

#ifdef CONFIG_PM_DEVICE
static int nrf91_slm_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_RESUME);
		ret = 0;
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_SUSPEND);
		ret = k_sem_take(&data->suspended_sem, K_SECONDS(30));
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

PM_DEVICE_DT_INST_DEFINE(0, nrf91_slm_pm_action);

#endif /* CONFIG_PM_DEVICE */

/******************************************************************************
 * Device Initialization
 *****************************************************************************/

static void nrf91_slm_init_pipe(const struct device *dev)
{
	const struct nrf91_slm_config *config = dev->config;
	struct nrf91_slm_data *data = dev->data;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = config->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = ARRAY_SIZE(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

static int nrf91_slm_init_chat(const struct device *dev)
{
	struct nrf91_slm_data *data = dev->data;

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_receive_buf,
		.receive_buf_size = ARRAY_SIZE(data->chat_receive_buf),
		.delimiter = data->chat_delimiter,
		.delimiter_size = strlen(data->chat_delimiter),
		.filter = data->chat_filter,
		.filter_size = data->chat_filter ? strlen(data->chat_filter) : 0,
		.argv = data->chat_argv,
		.argv_size = ARRAY_SIZE(data->chat_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	return modem_chat_init(&data->chat, &chat_config);
}

static int nrf91_slm_init(const struct device *dev)
{
	struct nrf91_slm_data *data = (struct nrf91_slm_data *)dev->data;
	const struct nrf91_slm_config *config = (const struct nrf91_slm_config *)dev->config;
	int ret;

	data->dev = dev;
	data->chat_delimiter = "\r\n";

	k_work_init_delayable(&data->timeout_work, nrf91_slm_timeout_handler);

	k_work_init(&data->event_dispatch_work, nrf91_slm_event_dispatch_handler);
	ring_buf_init(&data->event_rb, sizeof(data->event_buf), data->event_buf);
	ring_buf_init(&data->sock_recv_rb, sizeof(data->sock_recv_rb_buf), data->sock_recv_rb_buf);

	k_sem_init(&data->suspended_sem, 0, 1);
	k_sem_init(&data->sock_recv_sem, 0, 1);
	k_sem_init(&data->sock_send_sem, 0, 1);

	if (nrf91_slm_gpio_is_enabled(&config->power_gpio)) {
		ret = gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("failed to configure power GPIO (%d)", ret);
			return ret;
		}
	}

	if (nrf91_slm_gpio_is_enabled(&config->reset_gpio)) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("failed to configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	nrf91_slm_init_pipe(dev);

	ret = nrf91_slm_init_chat(dev);
	if (ret < 0) {
		LOG_ERR("failed to initialize chat (%d)", ret);
		return ret;
	}

#ifndef CONFIG_PM_DEVICE
	nrf91_slm_delegate_event(data, NRF91_SLM_EVENT_RESUME);
#else
	pm_device_init_suspended(dev);
#endif /* CONFIG_PM_DEVICE */

	return 0;
}

DEVICE_DT_INST_DEFINE(0, nrf91_slm_init, PM_DEVICE_DT_INST_GET(0), &mdata, &mconfig, POST_KERNEL,
		      99, &nrf91_slm_api);

/******************************************************************************
 * Offload API
 ******************************************************************************/

static bool offload_is_supported(int family, int type, int proto)
{
	if (family != AF_INET && family != AF_INET6) {
		return false;
	}

	if (type != SOCK_DGRAM && type != SOCK_STREAM) {
		return false;
	}

	if (proto != IPPROTO_TCP && proto != IPPROTO_UDP && proto != IPPROTO_TLS_1_2) {
		return false;
	}

	return true;
}

static int offload_socket(int family, int type, int proto)
{
	return nrf91_slm_socket(&mdata, family, type, proto);
}

static ssize_t offload_read(void *obj, void *buf, size_t count)
{
	return nrf91_slm_recvfrom(&mdata, obj, buf, count, 0, NULL, NULL);
}

static ssize_t offload_write(void *obj, const void *buf, size_t count)
{
	return nrf91_slm_sendto(&mdata, obj, buf, count, 0, NULL, 0);
}

static int offload_close(void *obj)
{
	return nrf91_slm_close(&mdata, obj);
}

static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD: {
		struct zsock_pollfd *fds;
		int nfds, timeout;

		fds = va_arg(args, struct zsock_pollfd *);
		nfds = va_arg(args, int);
		timeout = va_arg(args, int);

		return nrf91_slm_poll(&mdata, fds, nfds, timeout);
	}
	default:
		errno = EINVAL;
		return -1;
	}
}

static int offload_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	return nrf91_slm_connect(&mdata, obj, addr, addrlen);
}

static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *dest_addr, socklen_t addrlen)
{
	return nrf91_slm_sendto(&mdata, obj, buf, len, flags, dest_addr, addrlen);
}

static ssize_t offload_recvfrom(void *obj, void *buf, size_t max_len, int flags,
				struct sockaddr *src_addr, socklen_t *addrlen)
{
	return nrf91_slm_recvfrom(&mdata, obj, buf, max_len, flags, src_addr, addrlen);
}

static ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	ssize_t sent = 0;
	const char *buf;
	size_t len;
	int ret;

	for (int i = 0; i < msg->msg_iovlen; i++) {
		buf = msg->msg_iov[i].iov_base;
		len = msg->msg_iov[i].iov_len;

		while (len > 0) {
			ret = nrf91_slm_sendto(&mdata, obj, buf, len, flags, msg->msg_name,
					       msg->msg_namelen);

			if (ret < 0) {
				if (ret == -EAGAIN) {
					k_sleep(K_SECONDS(1));
				} else {
					return ret;
				}
			} else {
				sent += ret;
				buf += ret;
				len -= ret;
			}
		}
	}

	return sent;
}

static int offload_getaddrinfo(const char *node, const char *service,
			       const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{
	return nrf91_slm_getaddrinfo(&mdata, node, service, hints, res);
}

static void offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	nrf91_slm_freeaddrinfo(&mdata, res);
}

static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable = {
		.read = offload_read,
		.write = offload_write,
		.close = offload_close,
		.ioctl = offload_ioctl,
	},
	.bind = NULL,
	.connect = offload_connect,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.listen = NULL,
	.accept = NULL,
	.sendmsg = offload_sendmsg,
	.getsockopt = NULL,
	.setsockopt = NULL,
};

static const struct socket_dns_offload offload_dns_ops = {
	.getaddrinfo = offload_getaddrinfo,
	.freeaddrinfo = offload_freeaddrinfo,
};

static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nrf91_slm_data *data = dev->data;

	net_if_set_link_addr(iface, data->imei, ARRAY_SIZE(data->imei), NET_LINK_UNKNOWN);

	data->netif = iface;

	modem_socket_init(&data->socket_config, &data->sockets[0], ARRAY_SIZE(data->sockets), 0,
			  false, &offload_socket_fd_op_vtable);

	socket_offload_dns_register(&offload_dns_ops);

	net_if_socket_offload_set(iface, offload_socket);
}

static struct offloaded_if_api api_funcs = {
	.iface_api.init = modem_net_iface_init,
};

NET_DEVICE_OFFLOAD_INIT(nrf91_slm_net_dev, "nrf91_slm_net_dev", NULL, PM_DEVICE_DT_INST_GET(0),
			&mdata, &mconfig, 98, &api_funcs, 1500);

NET_SOCKET_OFFLOAD_REGISTER(nrf91_slm_sock, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,
			    offload_is_supported, offload_socket);
