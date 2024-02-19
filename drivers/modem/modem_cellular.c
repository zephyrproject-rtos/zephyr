/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/ppp.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/net/ppp.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_cellular, CONFIG_MODEM_LOG_LEVEL);

#include <string.h>
#include <stdlib.h>

#define MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT \
	K_MSEC(CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS)

#define MODEM_CELLULAR_DATA_IMEI_LEN         (16)
#define MODEM_CELLULAR_DATA_MODEL_ID_LEN     (65)
#define MODEM_CELLULAR_DATA_IMSI_LEN         (23)
#define MODEM_CELLULAR_DATA_ICCID_LEN        (22)
#define MODEM_CELLULAR_DATA_MANUFACTURER_LEN (65)
#define MODEM_CELLULAR_DATA_FW_VERSION_LEN   (65)

/* Magic constants */
#define CSQ_RSSI_UNKNOWN		     (99)
#define CESQ_RSRP_UNKNOWN		     (255)
#define CESQ_RSRQ_UNKNOWN		     (255)

/* Magic numbers to units conversions */
#define CSQ_RSSI_TO_DB(v) (-113 + (2 * (rssi)))
#define CESQ_RSRP_TO_DB(v) (-140 + (v))
#define CESQ_RSRQ_TO_DB(v) (-20 + ((v) / 2))

enum modem_cellular_state {
	MODEM_CELLULAR_STATE_IDLE = 0,
	MODEM_CELLULAR_STATE_RESET_PULSE,
	MODEM_CELLULAR_STATE_POWER_ON_PULSE,
	MODEM_CELLULAR_STATE_AWAIT_POWER_ON,
	MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT,
	MODEM_CELLULAR_STATE_CONNECT_CMUX,
	MODEM_CELLULAR_STATE_OPEN_DLCI1,
	MODEM_CELLULAR_STATE_OPEN_DLCI2,
	MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT,
	MODEM_CELLULAR_STATE_AWAIT_REGISTERED,
	MODEM_CELLULAR_STATE_CARRIER_ON,
	MODEM_CELLULAR_STATE_INIT_POWER_OFF,
	MODEM_CELLULAR_STATE_POWER_OFF_PULSE,
	MODEM_CELLULAR_STATE_AWAIT_POWER_OFF,
};

enum modem_cellular_event {
	MODEM_CELLULAR_EVENT_RESUME = 0,
	MODEM_CELLULAR_EVENT_SUSPEND,
	MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS,
	MODEM_CELLULAR_EVENT_SCRIPT_FAILED,
	MODEM_CELLULAR_EVENT_CMUX_CONNECTED,
	MODEM_CELLULAR_EVENT_DLCI1_OPENED,
	MODEM_CELLULAR_EVENT_DLCI2_OPENED,
	MODEM_CELLULAR_EVENT_TIMEOUT,
	MODEM_CELLULAR_EVENT_REGISTERED,
	MODEM_CELLULAR_EVENT_DEREGISTERED,
	MODEM_CELLULAR_EVENT_BUS_OPENED,
	MODEM_CELLULAR_EVENT_BUS_CLOSED,
};

struct modem_cellular_data {
	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_MODEM_CELLULAR_UART_BUFFER_SIZES];
	uint8_t uart_backend_transmit_buf[CONFIG_MODEM_CELLULAR_UART_BUFFER_SIZES];

	/* CMUX */
	struct modem_cmux cmux;
	uint8_t cmux_receive_buf[CONFIG_MODEM_CELLULAR_CMUX_MAX_FRAME_SIZE];
	uint8_t cmux_transmit_buf[2 * CONFIG_MODEM_CELLULAR_CMUX_MAX_FRAME_SIZE];
	struct modem_cmux_dlci dlci1;
	struct modem_cmux_dlci dlci2;
	struct modem_pipe *dlci1_pipe;
	struct modem_pipe *dlci2_pipe;
	uint8_t dlci1_receive_buf[CONFIG_MODEM_CELLULAR_CMUX_MAX_FRAME_SIZE];
	/* DLCI 2 is only used for chat scripts. */
	uint8_t dlci2_receive_buf[CONFIG_MODEM_CELLULAR_CHAT_BUFFER_SIZES];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[CONFIG_MODEM_CELLULAR_CHAT_BUFFER_SIZES];
	uint8_t *chat_delimiter;
	uint8_t *chat_filter;
	uint8_t *chat_argv[32];

	/* Status */
	enum cellular_registration_status registration_status_gsm;
	enum cellular_registration_status registration_status_gprs;
	enum cellular_registration_status registration_status_lte;
	uint8_t rssi;
	uint8_t rsrp;
	uint8_t rsrq;
	uint8_t imei[MODEM_CELLULAR_DATA_IMEI_LEN];
	uint8_t model_id[MODEM_CELLULAR_DATA_MODEL_ID_LEN];
	uint8_t imsi[MODEM_CELLULAR_DATA_IMSI_LEN];
	uint8_t iccid[MODEM_CELLULAR_DATA_ICCID_LEN];
	uint8_t manufacturer[MODEM_CELLULAR_DATA_MANUFACTURER_LEN];
	uint8_t fw_version[MODEM_CELLULAR_DATA_FW_VERSION_LEN];

	/* PPP */
	struct modem_ppp *ppp;

	enum modem_cellular_state state;
	const struct device *dev;
	struct k_work_delayable timeout_work;

	/* Power management */
	struct k_sem suspended_sem;

	/* Event dispatcher */
	struct k_work event_dispatch_work;
	uint8_t event_buf[8];
	struct ring_buf event_rb;
	struct k_mutex event_rb_lock;
};

struct modem_cellular_config {
	const struct device *uart;
	const struct gpio_dt_spec power_gpio;
	const struct gpio_dt_spec reset_gpio;
	const uint16_t power_pulse_duration_ms;
	const uint16_t reset_pulse_duration_ms;
	const uint16_t startup_time_ms;
	const uint16_t shutdown_time_ms;
	const bool autostarts;
	const struct modem_chat_script *init_chat_script;
	const struct modem_chat_script *dial_chat_script;
	const struct modem_chat_script *periodic_chat_script;
};

static const char *modem_cellular_state_str(enum modem_cellular_state state)
{
	switch (state) {
	case MODEM_CELLULAR_STATE_IDLE:
		return "idle";
	case MODEM_CELLULAR_STATE_RESET_PULSE:
		return "reset pulse";
	case MODEM_CELLULAR_STATE_POWER_ON_PULSE:
		return "power pulse";
	case MODEM_CELLULAR_STATE_AWAIT_POWER_ON:
		return "await power on";
	case MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT:
		return "run init script";
	case MODEM_CELLULAR_STATE_CONNECT_CMUX:
		return "connect cmux";
	case MODEM_CELLULAR_STATE_OPEN_DLCI1:
		return "open dlci1";
	case MODEM_CELLULAR_STATE_OPEN_DLCI2:
		return "open dlci2";
	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
		return "await registered";
	case MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT:
		return "run dial script";
	case MODEM_CELLULAR_STATE_CARRIER_ON:
		return "carrier on";
	case MODEM_CELLULAR_STATE_INIT_POWER_OFF:
		return "init power off";
	case MODEM_CELLULAR_STATE_POWER_OFF_PULSE:
		return "power off pulse";
	case MODEM_CELLULAR_STATE_AWAIT_POWER_OFF:
		return "await power off";
	}

	return "";
}

static const char *modem_cellular_event_str(enum modem_cellular_event event)
{
	switch (event) {
	case MODEM_CELLULAR_EVENT_RESUME:
		return "resume";
	case MODEM_CELLULAR_EVENT_SUSPEND:
		return "suspend";
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		return "script success";
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		return "script failed";
	case MODEM_CELLULAR_EVENT_CMUX_CONNECTED:
		return "cmux connected";
	case MODEM_CELLULAR_EVENT_DLCI1_OPENED:
		return "dlci1 opened";
	case MODEM_CELLULAR_EVENT_DLCI2_OPENED:
		return "dlci2 opened";
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		return "timeout";
	case MODEM_CELLULAR_EVENT_REGISTERED:
		return "registered";
	case MODEM_CELLULAR_EVENT_DEREGISTERED:
		return "deregistered";
	case MODEM_CELLULAR_EVENT_BUS_OPENED:
		return "bus opened";
	case MODEM_CELLULAR_EVENT_BUS_CLOSED:
		return "bus closed";
	}

	return "";
}

static bool modem_cellular_gpio_is_enabled(const struct gpio_dt_spec *gpio)
{
	return gpio->port != NULL;
}

static void modem_cellular_enter_state(struct modem_cellular_data *data,
				       enum modem_cellular_state state);

static void modem_cellular_delegate_event(struct modem_cellular_data *data,
					  enum modem_cellular_event evt);

static void modem_cellular_event_handler(struct modem_cellular_data *data,
					 enum modem_cellular_event evt);

static void modem_cellular_bus_pipe_handler(struct modem_pipe *pipe,
					    enum modem_pipe_event event,
					    void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_BUS_OPENED);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_BUS_CLOSED);
		break;

	default:
		break;
	}
}

static void modem_cellular_dlci1_pipe_handler(struct modem_pipe *pipe,
					      enum modem_pipe_event event,
					      void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_DLCI1_OPENED);
		break;

	default:
		break;
	}
}

static void modem_cellular_dlci2_pipe_handler(struct modem_pipe *pipe,
					      enum modem_pipe_event event,
					      void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_DLCI2_OPENED);
		break;

	default:
		break;
	}
}

static void modem_cellular_chat_callback_handler(struct modem_chat *chat,
						 enum modem_chat_script_result result,
						 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS);
	} else {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SCRIPT_FAILED);
	}
}

static void modem_cellular_chat_on_imei(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->imei, argv[1], sizeof(data->imei) - 1);
}

static void modem_cellular_chat_on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->model_id, argv[1], sizeof(data->model_id) - 1);
}

static void modem_cellular_chat_on_cgmi(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->manufacturer, argv[1], sizeof(data->manufacturer) - 1);
}

static void modem_cellular_chat_on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->fw_version, argv[1], sizeof(data->fw_version) - 1);
}

static void modem_cellular_chat_on_csq(struct modem_chat *chat, char **argv, uint16_t argc,
				       void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 3) {
		return;
	}

	data->rssi = (uint8_t)atoi(argv[1]);
}

static void modem_cellular_chat_on_cesq(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 7) {
		return;
	}

	data->rsrq = (uint8_t)atoi(argv[5]);
	data->rsrp = (uint8_t)atoi(argv[6]);
}

static void modem_cellular_chat_on_iccid(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->iccid, argv[1], sizeof(data->iccid) - 1);
}

static void modem_cellular_chat_on_imsi(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->imsi, argv[1], sizeof(data->imsi) - 1);
}

static bool modem_cellular_is_registered(struct modem_cellular_data *data)
{
	return (data->registration_status_gsm == CELLULAR_REGISTRATION_REGISTERED_HOME)
		|| (data->registration_status_gsm == CELLULAR_REGISTRATION_REGISTERED_ROAMING)
		|| (data->registration_status_gprs == CELLULAR_REGISTRATION_REGISTERED_HOME)
		|| (data->registration_status_gprs == CELLULAR_REGISTRATION_REGISTERED_ROAMING)
		|| (data->registration_status_lte == CELLULAR_REGISTRATION_REGISTERED_HOME)
		|| (data->registration_status_lte == CELLULAR_REGISTRATION_REGISTERED_ROAMING);
}

static void modem_cellular_chat_on_cxreg(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	enum cellular_registration_status registration_status = 0;

	if (argc == 2) {
		registration_status = atoi(argv[1]);
	} else if (argc == 3) {
		registration_status = atoi(argv[2]);
	} else {
		return;
	}

	if (strcmp(argv[0], "+CREG: ") == 0) {
		data->registration_status_gsm = registration_status;
	} else if (strcmp(argv[0], "+CGREG: ") == 0) {
		data->registration_status_gprs = registration_status;
	} else {
		data->registration_status_lte = registration_status;
	}

	if (modem_cellular_is_registered(data)) {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_REGISTERED);
	} else {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_DEREGISTERED);
	}
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(allow_match,
			  MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH("ERROR", "", NULL));

MODEM_CHAT_MATCH_DEFINE(imei_match, "", "", modem_cellular_chat_on_imei);
MODEM_CHAT_MATCH_DEFINE(cgmm_match, "", "", modem_cellular_chat_on_cgmm);
MODEM_CHAT_MATCH_DEFINE(csq_match, "+CSQ: ", ",", modem_cellular_chat_on_csq);
MODEM_CHAT_MATCH_DEFINE(cesq_match, "+CESQ: ", ",", modem_cellular_chat_on_cesq);
MODEM_CHAT_MATCH_DEFINE(iccid_match __maybe_unused, "+ICCID: ", "", modem_cellular_chat_on_iccid);
MODEM_CHAT_MATCH_DEFINE(cimi_match __maybe_unused, "", "", modem_cellular_chat_on_imsi);
MODEM_CHAT_MATCH_DEFINE(cgmi_match __maybe_unused, "", "", modem_cellular_chat_on_cgmi);
MODEM_CHAT_MATCH_DEFINE(cgmr_match __maybe_unused, "", "", modem_cellular_chat_on_cgmr);

MODEM_CHAT_MATCHES_DEFINE(unsol_matches,
			  MODEM_CHAT_MATCH("+CREG: ", ",", modem_cellular_chat_on_cxreg),
			  MODEM_CHAT_MATCH("+CEREG: ", ",", modem_cellular_chat_on_cxreg),
			  MODEM_CHAT_MATCH("+CGREG: ", ",", modem_cellular_chat_on_cxreg));

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("ERROR", "", NULL));

MODEM_CHAT_MATCHES_DEFINE(dial_abort_matches,
			  MODEM_CHAT_MATCH("ERROR", "", NULL),
			  MODEM_CHAT_MATCH("BUSY", "", NULL),
			  MODEM_CHAT_MATCH("NO ANSWER", "", NULL),
			  MODEM_CHAT_MATCH("NO CARRIER", "", NULL),
			  MODEM_CHAT_MATCH("NO DIALTONE", "", NULL));

#if DT_HAS_COMPAT_STATUS_OKAY(swir_hl7800)
MODEM_CHAT_MATCH_DEFINE(connect_match, "CONNECT", "", NULL);
#endif

static void modem_cellular_log_state_changed(enum modem_cellular_state last_state,
					     enum modem_cellular_state new_state)
{
	LOG_DBG("switch from %s to %s", modem_cellular_state_str(last_state),
		modem_cellular_state_str(new_state));
}

static void modem_cellular_log_event(enum modem_cellular_event evt)
{
	LOG_DBG("event %s", modem_cellular_event_str(evt));
}

static void modem_cellular_start_timer(struct modem_cellular_data *data, k_timeout_t timeout)
{
	k_work_schedule(&data->timeout_work, timeout);
}

static void modem_cellular_stop_timer(struct modem_cellular_data *data)
{
	k_work_cancel_delayable(&data->timeout_work);
}

static void modem_cellular_timeout_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_cellular_data *data =
		CONTAINER_OF(dwork, struct modem_cellular_data, timeout_work);

	modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_TIMEOUT);
}

static void modem_cellular_event_dispatch_handler(struct k_work *item)
{
	struct modem_cellular_data *data =
		CONTAINER_OF(item, struct modem_cellular_data, event_dispatch_work);

	uint8_t events[sizeof(data->event_buf)];
	uint8_t events_cnt;

	k_mutex_lock(&data->event_rb_lock, K_FOREVER);

	events_cnt = (uint8_t)ring_buf_get(&data->event_rb, events, sizeof(data->event_buf));

	k_mutex_unlock(&data->event_rb_lock);

	for (uint8_t i = 0; i < events_cnt; i++) {
		modem_cellular_event_handler(data, (enum modem_cellular_event)events[i]);
	}
}

static void modem_cellular_delegate_event(struct modem_cellular_data *data,
					  enum modem_cellular_event evt)
{
	k_mutex_lock(&data->event_rb_lock, K_FOREVER);
	ring_buf_put(&data->event_rb, (uint8_t *)&evt, 1);
	k_mutex_unlock(&data->event_rb_lock);
	k_work_submit(&data->event_dispatch_work);
}

static int modem_cellular_on_idle_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	if (modem_cellular_gpio_is_enabled(&config->reset_gpio)) {
		gpio_pin_set_dt(&config->reset_gpio, 1);
	}

	modem_chat_release(&data->chat);
	modem_ppp_release(data->ppp);
	modem_cmux_release(&data->cmux);
	modem_pipe_close_async(data->uart_pipe);
	k_sem_give(&data->suspended_sem);
	return 0;
}

static void modem_cellular_idle_event_handler(struct modem_cellular_data *data,
					      enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_RESUME:
		if (config->autostarts) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_POWER_ON);
			break;
		}

		if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_POWER_ON_PULSE);
			break;
		}

		if (modem_cellular_gpio_is_enabled(&config->reset_gpio)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_POWER_ON);
			break;
		}

		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		k_sem_give(&data->suspended_sem);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_idle_state_leave(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	k_sem_take(&data->suspended_sem, K_NO_WAIT);

	if (modem_cellular_gpio_is_enabled(&config->reset_gpio)) {
		gpio_pin_set_dt(&config->reset_gpio, 0);
	}

	return 0;
}

static int modem_cellular_on_reset_pulse_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	gpio_pin_set_dt(&config->reset_gpio, 1);
	modem_cellular_start_timer(data, K_MSEC(config->reset_pulse_duration_ms));
	return 0;
}

static void modem_cellular_reset_pulse_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_POWER_ON);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_reset_pulse_state_leave(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	gpio_pin_set_dt(&config->reset_gpio, 0);
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_power_on_pulse_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 1);
	modem_cellular_start_timer(data, K_MSEC(config->power_pulse_duration_ms));
	return 0;
}

static void modem_cellular_power_on_pulse_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_POWER_ON);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_power_on_pulse_state_leave(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 0);
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_await_power_on_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	modem_cellular_start_timer(data, K_MSEC(config->startup_time_ms));
	return 0;
}

static void modem_cellular_await_power_on_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_run_init_script_state_enter(struct modem_cellular_data *data)
{
	modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
	return modem_pipe_open_async(data->uart_pipe);
}

static void modem_cellular_run_init_script_event_handler(struct modem_cellular_data *data,
							 enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		modem_chat_run_script_async(&data->chat, config->init_chat_script);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		net_if_set_link_addr(modem_ppp_get_iface(data->ppp), data->imei,
				     ARRAY_SIZE(data->imei), NET_LINK_UNKNOWN);

		modem_chat_release(&data->chat);
		modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
		modem_pipe_close_async(data->uart_pipe);
		break;

	case MODEM_CELLULAR_EVENT_BUS_CLOSED:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_CONNECT_CMUX);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_POWER_ON_PULSE);
			break;
		}

		if (modem_cellular_gpio_is_enabled(&config->reset_gpio)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RESET_PULSE);
			break;
		}

		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_connect_cmux_state_enter(struct modem_cellular_data *data)
{
	/*
	 * Allow modem to switch bus into CMUX mode. Some modems disable UART RX while
	 * switching, resulting in UART RX errors as bus is no longer pulled up by modem.
	 */
	modem_cellular_start_timer(data, K_MSEC(100));
	return 0;
}

static void modem_cellular_connect_cmux_event_handler(struct modem_cellular_data *data,
						      enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
		modem_pipe_open_async(data->uart_pipe);
		break;

	case MODEM_CELLULAR_EVENT_BUS_OPENED:
		modem_cmux_attach(&data->cmux, data->uart_pipe);
		modem_cmux_connect_async(&data->cmux);
		break;

	case MODEM_CELLULAR_EVENT_CMUX_CONNECTED:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_OPEN_DLCI1);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_open_dlci1_state_enter(struct modem_cellular_data *data)
{
	modem_pipe_attach(data->dlci1_pipe, modem_cellular_dlci1_pipe_handler, data);
	return modem_pipe_open_async(data->dlci1_pipe);
}

static void modem_cellular_open_dlci1_event_handler(struct modem_cellular_data *data,
						    enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_DLCI1_OPENED:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_OPEN_DLCI2);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_open_dlci1_state_leave(struct modem_cellular_data *data)
{
	modem_pipe_release(data->dlci1_pipe);
	return 0;
}

static int modem_cellular_on_open_dlci2_state_enter(struct modem_cellular_data *data)
{
	modem_pipe_attach(data->dlci2_pipe, modem_cellular_dlci2_pipe_handler, data);
	return modem_pipe_open_async(data->dlci2_pipe);
}

static void modem_cellular_open_dlci2_event_handler(struct modem_cellular_data *data,
						    enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_DLCI2_OPENED:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_open_dlci2_state_leave(struct modem_cellular_data *data)
{
	modem_pipe_release(data->dlci2_pipe);
	return 0;
}

static int modem_cellular_on_run_dial_script_state_enter(struct modem_cellular_data *data)
{
	/* Allow modem time to enter command mode before running dial script */
	modem_cellular_start_timer(data, K_MSEC(100));
	return 0;
}

static void modem_cellular_run_dial_script_event_handler(struct modem_cellular_data *data,
							 enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_chat_attach(&data->chat, data->dlci1_pipe);
		modem_chat_run_script_async(&data->chat, config->dial_chat_script);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_REGISTERED);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_run_dial_script_state_leave(struct modem_cellular_data *data)
{
	modem_chat_release(&data->chat);
	return 0;
}

static int modem_cellular_on_await_registered_state_enter(struct modem_cellular_data *data)
{
	if (modem_ppp_attach(data->ppp, data->dlci1_pipe) < 0) {
		return -EAGAIN;
	}

	modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
	return modem_chat_attach(&data->chat, data->dlci2_pipe);
}

static void modem_cellular_await_registered_event_handler(struct modem_cellular_data *data,
						  enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		break;

	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_chat_run_script_async(&data->chat, config->periodic_chat_script);
		break;

	case MODEM_CELLULAR_EVENT_REGISTERED:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_CARRIER_ON);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_await_registered_state_leave(struct modem_cellular_data *data)
{
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_carrier_on_state_enter(struct modem_cellular_data *data)
{
	net_if_carrier_on(modem_ppp_get_iface(data->ppp));
	modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
	return 0;
}

static void modem_cellular_carrier_on_event_handler(struct modem_cellular_data *data,
						    enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		break;

	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_chat_run_script_async(&data->chat, config->periodic_chat_script);
		break;

	case MODEM_CELLULAR_EVENT_DEREGISTERED:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_carrier_on_state_leave(struct modem_cellular_data *data)
{
	modem_cellular_stop_timer(data);
	net_if_carrier_off(modem_ppp_get_iface(data->ppp));
	modem_chat_release(&data->chat);
	modem_ppp_release(data->ppp);
	return 0;
}

static int modem_cellular_on_init_power_off_state_enter(struct modem_cellular_data *data)
{
	modem_pipe_close_async(data->uart_pipe);
	modem_cellular_start_timer(data, K_MSEC(2000));
	return 0;
}

static void modem_cellular_init_power_off_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_POWER_OFF_PULSE);
			break;
		}

		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_init_power_off_state_leave(struct modem_cellular_data *data)
{
	modem_chat_release(&data->chat);
	modem_ppp_release(data->ppp);
	return 0;
}

static int modem_cellular_on_power_off_pulse_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 1);
	modem_cellular_start_timer(data, K_MSEC(config->power_pulse_duration_ms));
	return 0;
}

static void modem_cellular_power_off_pulse_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_power_off_pulse_state_leave(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 0);
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_await_power_off_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	modem_cellular_start_timer(data, K_MSEC(config->shutdown_time_ms));
	return 0;
}

static void modem_cellular_await_power_off_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_state_enter(struct modem_cellular_data *data)
{
	int ret;

	switch (data->state) {
	case MODEM_CELLULAR_STATE_IDLE:
		ret = modem_cellular_on_idle_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RESET_PULSE:
		ret = modem_cellular_on_reset_pulse_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_POWER_ON_PULSE:
		ret = modem_cellular_on_power_on_pulse_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_POWER_ON:
		ret = modem_cellular_on_await_power_on_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT:
		ret = modem_cellular_on_run_init_script_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_CONNECT_CMUX:
		ret = modem_cellular_on_connect_cmux_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_OPEN_DLCI1:
		ret = modem_cellular_on_open_dlci1_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_OPEN_DLCI2:
		ret = modem_cellular_on_open_dlci2_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT:
		ret = modem_cellular_on_run_dial_script_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
		ret = modem_cellular_on_await_registered_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_CARRIER_ON:
		ret = modem_cellular_on_carrier_on_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_INIT_POWER_OFF:
		ret = modem_cellular_on_init_power_off_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_POWER_OFF_PULSE:
		ret = modem_cellular_on_power_off_pulse_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_POWER_OFF:
		ret = modem_cellular_on_await_power_off_state_enter(data);
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

static int modem_cellular_on_state_leave(struct modem_cellular_data *data)
{
	int ret;

	switch (data->state) {
	case MODEM_CELLULAR_STATE_IDLE:
		ret = modem_cellular_on_idle_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_RESET_PULSE:
		ret = modem_cellular_on_reset_pulse_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_POWER_ON_PULSE:
		ret = modem_cellular_on_power_on_pulse_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_OPEN_DLCI1:
		ret = modem_cellular_on_open_dlci1_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_OPEN_DLCI2:
		ret = modem_cellular_on_open_dlci2_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT:
		ret = modem_cellular_on_run_dial_script_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
		ret = modem_cellular_on_await_registered_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_CARRIER_ON:
		ret = modem_cellular_on_carrier_on_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_INIT_POWER_OFF:
		ret = modem_cellular_on_init_power_off_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_POWER_OFF_PULSE:
		ret = modem_cellular_on_power_off_pulse_state_leave(data);
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

static void modem_cellular_enter_state(struct modem_cellular_data *data,
				       enum modem_cellular_state state)
{
	int ret;

	ret = modem_cellular_on_state_leave(data);

	if (ret < 0) {
		LOG_WRN("failed to leave state, error: %i", ret);

		return;
	}

	data->state = state;
	ret = modem_cellular_on_state_enter(data);

	if (ret < 0) {
		LOG_WRN("failed to enter state error: %i", ret);
	}
}

static void modem_cellular_event_handler(struct modem_cellular_data *data,
					 enum modem_cellular_event evt)
{
	enum modem_cellular_state state;

	state = data->state;

	modem_cellular_log_event(evt);

	switch (data->state) {
	case MODEM_CELLULAR_STATE_IDLE:
		modem_cellular_idle_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RESET_PULSE:
		modem_cellular_reset_pulse_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_POWER_ON_PULSE:
		modem_cellular_power_on_pulse_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_POWER_ON:
		modem_cellular_await_power_on_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT:
		modem_cellular_run_init_script_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_CONNECT_CMUX:
		modem_cellular_connect_cmux_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_OPEN_DLCI1:
		modem_cellular_open_dlci1_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_OPEN_DLCI2:
		modem_cellular_open_dlci2_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT:
		modem_cellular_run_dial_script_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
		modem_cellular_await_registered_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_CARRIER_ON:
		modem_cellular_carrier_on_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_INIT_POWER_OFF:
		modem_cellular_init_power_off_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_POWER_OFF_PULSE:
		modem_cellular_power_off_pulse_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_POWER_OFF:
		modem_cellular_await_power_off_event_handler(data, evt);
		break;
	}

	if (state != data->state) {
		modem_cellular_log_state_changed(state, data->state);
	}
}

static void modem_cellular_cmux_handler(struct modem_cmux *cmux, enum modem_cmux_event event,
					void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	switch (event) {
	case MODEM_CMUX_EVENT_CONNECTED:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_CMUX_CONNECTED);
		break;

	default:
		break;
	}
}

MODEM_CHAT_SCRIPT_CMDS_DEFINE(get_signal_csq_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CSQ", csq_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(get_signal_csq_chat_script, get_signal_csq_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 2);

static inline int modem_cellular_csq_parse_rssi(uint8_t rssi, int16_t *value)
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
			 abort_matches, modem_cellular_chat_callback_handler, 2);

/* AT+CESQ returns a response +CESQ: <rxlev>,<ber>,<rscp>,<ecn0>,<rsrq>,<rsrp> where:
 * - rsrq is a integer from 0 to 34 whose values describes the Reference Signal Receive
 *   Quality between -20 dB for 0 and -3 dB for 34 (0.5 dB steps), or unknown for 255
 * - rsrp is an integer from 0 to 97 that describes the Reference Signal Receive Power
 *   between -140 dBm for 0 and -44 dBm for 97 (1 dBm steps), or unknown for 255
 */
static inline int modem_cellular_cesq_parse_rsrp(uint8_t rsrp, int16_t *value)
{
	if (rsrp == CESQ_RSRP_UNKNOWN) {
		return -EINVAL;
	}

	*value = (int16_t)CESQ_RSRP_TO_DB(rsrp);
	return 0;
}

static inline int modem_cellular_cesq_parse_rsrq(uint8_t rsrq, int16_t *value)
{
	if (rsrq == CESQ_RSRQ_UNKNOWN) {
		return -EINVAL;
	}

	*value = (int16_t)CESQ_RSRQ_TO_DB(rsrq);
	return 0;
}

static int modem_cellular_get_signal(const struct device *dev,
				     const enum cellular_signal_type type,
				     int16_t *value)
{
	int ret = -ENOTSUP;
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;

	if ((data->state != MODEM_CELLULAR_STATE_AWAIT_REGISTERED) &&
	    (data->state != MODEM_CELLULAR_STATE_CARRIER_ON)) {
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
		ret = modem_cellular_csq_parse_rssi(data->rssi, value);
		break;

	case CELLULAR_SIGNAL_RSRP:
		ret = modem_cellular_cesq_parse_rsrp(data->rsrp, value);
		break;

	case CELLULAR_SIGNAL_RSRQ:
		ret = modem_cellular_cesq_parse_rsrq(data->rsrq, value);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int modem_cellular_get_modem_info(const struct device *dev,
					 enum cellular_modem_info_type type,
					 char *info, size_t size)
{
	int ret = 0;
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;

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
static int modem_cellular_get_registration_status(const struct device *dev,
						  enum cellular_access_technology tech,
						  enum cellular_registration_status *status)
{
	int ret = 0;
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;

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

const static struct cellular_driver_api modem_cellular_api = {
	.get_signal = modem_cellular_get_signal,
	.get_modem_info = modem_cellular_get_modem_info,
	.get_registration_status = modem_cellular_get_registration_status,
};

#ifdef CONFIG_PM_DEVICE
static int modem_cellular_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_RESUME);
		ret = 0;
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SUSPEND);
		ret = k_sem_take(&data->suspended_sem, K_SECONDS(30));
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int modem_cellular_init(const struct device *dev)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;
	struct modem_cellular_config *config = (struct modem_cellular_config *)dev->config;

	data->dev = dev;

	k_work_init_delayable(&data->timeout_work, modem_cellular_timeout_handler);

	k_work_init(&data->event_dispatch_work, modem_cellular_event_dispatch_handler);
	ring_buf_init(&data->event_rb, sizeof(data->event_buf), data->event_buf);

	k_sem_init(&data->suspended_sem, 0, 1);

	if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
		gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_INACTIVE);
	}

	if (modem_cellular_gpio_is_enabled(&config->reset_gpio)) {
		gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	}

	{
		const struct modem_backend_uart_config uart_backend_config = {
			.uart = config->uart,
			.receive_buf = data->uart_backend_receive_buf,
			.receive_buf_size = ARRAY_SIZE(data->uart_backend_receive_buf),
			.transmit_buf = data->uart_backend_transmit_buf,
			.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
		};

		data->uart_pipe = modem_backend_uart_init(&data->uart_backend,
							  &uart_backend_config);
	}

	{
		const struct modem_cmux_config cmux_config = {
			.callback = modem_cellular_cmux_handler,
			.user_data = data,
			.receive_buf = data->cmux_receive_buf,
			.receive_buf_size = ARRAY_SIZE(data->cmux_receive_buf),
			.transmit_buf = data->cmux_transmit_buf,
			.transmit_buf_size = ARRAY_SIZE(data->cmux_transmit_buf),
		};

		modem_cmux_init(&data->cmux, &cmux_config);
	}

	{
		const struct modem_cmux_dlci_config dlci1_config = {
			.dlci_address = 1,
			.receive_buf = data->dlci1_receive_buf,
			.receive_buf_size = ARRAY_SIZE(data->dlci1_receive_buf),
		};

		data->dlci1_pipe = modem_cmux_dlci_init(&data->cmux, &data->dlci1,
							&dlci1_config);
	}

	{
		const struct modem_cmux_dlci_config dlci2_config = {
			.dlci_address = 2,
			.receive_buf = data->dlci2_receive_buf,
			.receive_buf_size = ARRAY_SIZE(data->dlci2_receive_buf),
		};

		data->dlci2_pipe = modem_cmux_dlci_init(&data->cmux, &data->dlci2,
							&dlci2_config);
	}

	{
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

		modem_chat_init(&data->chat, &chat_config);
	}

#ifndef CONFIG_PM_DEVICE
	modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_RESUME);
#else
	pm_device_init_suspended(dev);
#endif /* CONFIG_PM_DEVICE */

	return 0;
}

/*
 * Every modem uses two custom scripts to initialize the modem and dial out.
 *
 * The first script is named <dt driver compatible>_init_chat_script, with its
 * script commands named <dt driver compatible>_init_chat_script_cmds. This
 * script is sent to the modem after it has started up, and must configure the
 * modem to use CMUX.
 *
 * The second script is named <dt driver compatible>_dial_chat_script, with its
 * script commands named <dt driver compatible>_dial_chat_script_cmds. This
 * script is sent on a DLCI channel in command mode, and must request the modem
 * dial out and put the DLCI channel into data mode.
 */

#if DT_HAS_COMPAT_STATUS_OKAY(quectel_bg95)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg95_init_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127", 300));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg95_init_chat_script, quectel_bg95_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg95_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT=1,\"IP\","
							 "\""CONFIG_MODEM_CELLULAR_APN"\"",
							 ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0),);

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg95_dial_chat_script, quectel_bg95_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_bg95_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_bg95_periodic_chat_script,
			 quectel_bg95_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(quectel_eg25_g)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	quectel_eg25_g_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127,10,3,30,10,2", 100));

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_init_chat_script, quectel_eg25_g_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_eg25_g_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT=1,\"IP\","
							 "\""CONFIG_MODEM_CELLULAR_APN"\"",
							 ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0),);

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_dial_chat_script, quectel_eg25_g_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(quectel_eg25_g_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CSQ", csq_match));

MODEM_CHAT_SCRIPT_DEFINE(quectel_eg25_g_periodic_chat_script,
			 quectel_eg25_g_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_gsm_ppp)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(zephyr_gsm_ppp_init_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      /* The 300ms delay after sending the AT+CMUX command is required
			       * for some modems to ensure they get enough time to enter CMUX
			       * mode before sending the first CMUX command. If this delay is
			       * too short, modems have been observed to simply deadlock,
			       * refusing to respond to any CMUX command.
			       */
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127", 300));

MODEM_CHAT_SCRIPT_DEFINE(zephyr_gsm_ppp_init_chat_script, zephyr_gsm_ppp_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(zephyr_gsm_ppp_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT=1,\"IP\","
							 "\""CONFIG_MODEM_CELLULAR_APN"\"",
							 ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0),);

MODEM_CHAT_SCRIPT_DEFINE(zephyr_gsm_ppp_dial_chat_script, zephyr_gsm_ppp_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(zephyr_gsm_ppp_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(zephyr_gsm_ppp_periodic_chat_script,
			 zephyr_gsm_ppp_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(simcom_sim7080)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(simcom_sim7080_init_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127", 300));

MODEM_CHAT_SCRIPT_DEFINE(simcom_sim7080_init_chat_script, simcom_sim7080_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(simcom_sim7080_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT=1,\"IP\","
							 "\""CONFIG_MODEM_CELLULAR_APN"\"",
							 ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0),);

MODEM_CHAT_SCRIPT_DEFINE(simcom_sim7080_dial_chat_script, simcom_sim7080_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(simcom_sim7080_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(simcom_sim7080_periodic_chat_script,
			 simcom_sim7080_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(u_blox_sara_r4)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(u_blox_sara_r4_init_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMUX=0,0,5,127", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(u_blox_sara_r4_init_chat_script, u_blox_sara_r4_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(u_blox_sara_r4_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT=1,\"IP\","
							 "\""CONFIG_MODEM_CELLULAR_APN"\"",
							 ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0),);

MODEM_CHAT_SCRIPT_DEFINE(u_blox_sara_r4_dial_chat_script, u_blox_sara_r4_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(u_blox_sara_r4_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(u_blox_sara_r4_periodic_chat_script,
			 u_blox_sara_r4_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(u_blox_sara_r5)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(u_blox_sara_r5_init_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMUX=0,0,5,127", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(u_blox_sara_r5_init_chat_script, u_blox_sara_r5_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(u_blox_sara_r5_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0,1", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT=1,\"IP\","
							 "\""CONFIG_MODEM_CELLULAR_APN"\"",
							 ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0),);

MODEM_CHAT_SCRIPT_DEFINE(u_blox_sara_r5_dial_chat_script, u_blox_sara_r5_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(u_blox_sara_r5_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(u_blox_sara_r5_periodic_chat_script,
			 u_blox_sara_r5_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(swir_hl7800)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(swir_hl7800_init_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127", 0));

MODEM_CHAT_SCRIPT_DEFINE(swir_hl7800_init_chat_script, swir_hl7800_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(swir_hl7800_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT=1,\"IP\","
							 "\""CONFIG_MODEM_CELLULAR_APN"\"",
							 ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KCNXCFG=1,\"GPRS\",\""
							 CONFIG_MODEM_CELLULAR_APN
							 "\",,,\"IPV4\"",
							 ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+WPPP=0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("ATD*99***1#", connect_match));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(swir_hl7800_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(swir_hl7800_periodic_chat_script,
			 swir_hl7800_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);

MODEM_CHAT_SCRIPT_DEFINE(swir_hl7800_dial_chat_script, swir_hl7800_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(telit_me910g1)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(telit_me910g1_init_chat_script_cmds,
				  MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
				  MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
				  MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
				  MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
				  MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
				  /* The Telit me910g1 often has an error trying
				   * to set the PDP context. The radio must be on to set
				   * the context, and this step must be successful.
				   * It is moved to the init script to allow retries.
				   */
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGDCONT=1,\"IP\","
							 "\"" CONFIG_MODEM_CELLULAR_APN "\"",
							 ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=1", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG=1", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
				  MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT+CMUX=0,0,5,127,10,3,30,10,2",
								  300));

MODEM_CHAT_SCRIPT_DEFINE(telit_me910g1_init_chat_script, telit_me910g1_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(telit_me910g1_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP_NONE("ATD*99***1#", 0));

MODEM_CHAT_SCRIPT_DEFINE(telit_me910g1_dial_chat_script, telit_me910g1_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(telit_me910g1_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(telit_me910g1_periodic_chat_script,
			 telit_me910g1_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf91_slm)
MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_init_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT", allow_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#XCMUX=1", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_init_chat_script, nordic_nrf91_slm_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_dial_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT#XCMUX=2", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_dial_chat_script, nordic_nrf91_slm_dial_chat_script_cmds,
			 dial_abort_matches, modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(nordic_nrf91_slm_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(nordic_nrf91_slm_periodic_chat_script,
			 nordic_nrf91_slm_periodic_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 4);
#endif

#define MODEM_CELLULAR_INST_NAME(name, inst) \
	_CONCAT(_CONCAT(_CONCAT(name, _), DT_DRV_COMPAT), inst)

#define MODEM_CELLULAR_DEVICE_QUECTEL_BG95(inst)                                                   \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);                 \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.power_pulse_duration_ms = 1500,                                                   \
		.reset_pulse_duration_ms = 100,                                                    \
		.startup_time_ms = 10000,                                                          \
		.shutdown_time_ms = 5000,                                                          \
		.init_chat_script = &quectel_bg95_init_chat_script,                                \
		.dial_chat_script = &quectel_bg95_dial_chat_script,                                \
		.periodic_chat_script = &_CONCAT(DT_DRV_COMPAT, _periodic_chat_script),            \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define MODEM_CELLULAR_DEVICE_QUECTEL_EG25_G(inst)                                                 \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);                 \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.power_pulse_duration_ms = 1500,                                                   \
		.reset_pulse_duration_ms = 500,                                                    \
		.startup_time_ms = 15000,                                                          \
		.shutdown_time_ms = 5000,                                                          \
		.init_chat_script = &quectel_eg25_g_init_chat_script,                              \
		.dial_chat_script = &quectel_eg25_g_dial_chat_script,                              \
		.periodic_chat_script = &_CONCAT(DT_DRV_COMPAT, _periodic_chat_script),            \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define MODEM_CELLULAR_DEVICE_GSM_PPP(inst)                                                        \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);                 \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.power_pulse_duration_ms = 1500,                                                   \
		.reset_pulse_duration_ms = 100,                                                    \
		.startup_time_ms = 10000,                                                          \
		.shutdown_time_ms = 5000,                                                          \
		.init_chat_script = &zephyr_gsm_ppp_init_chat_script,                              \
		.dial_chat_script = &zephyr_gsm_ppp_dial_chat_script,                              \
		.periodic_chat_script = &zephyr_gsm_ppp_periodic_chat_script,                      \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define MODEM_CELLULAR_DEVICE_SIMCOM_SIM7080(inst)                                                 \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);                 \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.power_pulse_duration_ms = 1500,                                                   \
		.reset_pulse_duration_ms = 100,                                                    \
		.startup_time_ms = 10000,                                                          \
		.shutdown_time_ms = 5000,                                                          \
		.init_chat_script = &simcom_sim7080_init_chat_script,                              \
		.dial_chat_script = &simcom_sim7080_dial_chat_script,                              \
		.periodic_chat_script = &simcom_sim7080_periodic_chat_script,                      \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define MODEM_CELLULAR_DEVICE_U_BLOX_SARA_R4(inst)                                                 \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);                 \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.power_pulse_duration_ms = 1500,                                                   \
		.reset_pulse_duration_ms = 100,                                                    \
		.startup_time_ms = 10000,                                                          \
		.shutdown_time_ms = 5000,                                                          \
		.init_chat_script = &u_blox_sara_r4_init_chat_script,                              \
		.dial_chat_script = &u_blox_sara_r4_dial_chat_script,                              \
		.periodic_chat_script = &u_blox_sara_r4_periodic_chat_script,                      \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define MODEM_CELLULAR_DEVICE_U_BLOX_SARA_R5(inst)                                                 \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);                 \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.autostarts = true,                                                                \
		.power_pulse_duration_ms = 1500,                                                   \
		.reset_pulse_duration_ms = 100,                                                    \
		.startup_time_ms = 1500,                                                           \
		.shutdown_time_ms = 13000,                                                         \
		.init_chat_script = &u_blox_sara_r5_init_chat_script,                              \
		.dial_chat_script = &u_blox_sara_r5_dial_chat_script,                              \
		.periodic_chat_script = &u_blox_sara_r5_periodic_chat_script,                      \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define MODEM_CELLULAR_DEVICE_SWIR_HL7800(inst)                                                    \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);                 \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.power_pulse_duration_ms = 1500,                                                   \
		.reset_pulse_duration_ms = 100,                                                    \
		.startup_time_ms = 10000,                                                          \
		.shutdown_time_ms = 5000,                                                          \
		.init_chat_script = &swir_hl7800_init_chat_script,                                 \
		.dial_chat_script = &swir_hl7800_dial_chat_script,                                 \
		.periodic_chat_script = &swir_hl7800_periodic_chat_script,                         \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define MODEM_CELLULAR_DEVICE_TELIT_ME910G1(inst)                                                  \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 64);                 \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r",                                                            \
		.chat_filter = "\n",                                                               \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.power_pulse_duration_ms = 5050,                                                   \
		.reset_pulse_duration_ms = 250,                                                    \
		.startup_time_ms = 15000,                                                          \
		.shutdown_time_ms = 5000,                                                          \
		.init_chat_script = &telit_me910g1_init_chat_script,                               \
		.dial_chat_script = &telit_me910g1_dial_chat_script,                               \
		.periodic_chat_script = &telit_me910g1_periodic_chat_script,                       \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define MODEM_CELLULAR_DEVICE_NORDIC_NRF91_SLM(inst)						   \
	MODEM_PPP_DEFINE(MODEM_CELLULAR_INST_NAME(ppp, inst), NULL, 98, 1500, 1500);               \
                                                                                                   \
	static struct modem_cellular_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
		.chat_delimiter = "\r\n",                                                          \
		.ppp = &MODEM_CELLULAR_INST_NAME(ppp, inst),                                       \
	};                                                                                         \
                                                                                                   \
	static struct modem_cellular_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.power_pulse_duration_ms = 100,                                                    \
		.reset_pulse_duration_ms = 100,                                                    \
		.startup_time_ms = 2000,                                                           \
		.shutdown_time_ms = 10000,                                                         \
		.init_chat_script = &nordic_nrf91_slm_init_chat_script,                            \
		.dial_chat_script = &nordic_nrf91_slm_dial_chat_script,                            \
		.periodic_chat_script = &nordic_nrf91_slm_periodic_chat_script,                    \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, modem_cellular_pm_action);                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
			      &MODEM_CELLULAR_INST_NAME(data, inst),                               \
			      &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99,            \
			      &modem_cellular_api);

#define DT_DRV_COMPAT quectel_bg95
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_QUECTEL_BG95)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT quectel_eg25_g
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_QUECTEL_EG25_G)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT zephyr_gsm_ppp
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_GSM_PPP)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT simcom_sim7080
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_SIMCOM_SIM7080)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT u_blox_sara_r4
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_U_BLOX_SARA_R4)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT u_blox_sara_r5
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_U_BLOX_SARA_R5)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_SWIR_HL7800)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT telit_me910g1
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_TELIT_ME910G1)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT nordic_nrf91_slm
DT_INST_FOREACH_STATUS_OKAY(MODEM_CELLULAR_DEVICE_NORDIC_NRF91_SLM)
#undef DT_DRV_COMPAT
