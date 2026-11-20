/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/modem/chat.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/pipelink.h>
#include <zephyr/modem/ppp.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/net/ppp.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_cellular, CONFIG_MODEM_LOG_LEVEL);

#include <string.h>
#include <stdlib.h>

#define MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT \
	K_MSEC(CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS)

/* Runs of the sim_activate script of a SIM slot switch, and the delay between them */
#define MODEM_CELLULAR_SIM_ACTIVATE_ATTEMPTS    10U
#define MODEM_CELLULAR_SIM_ACTIVATE_RETRY_DELAY K_SECONDS(1)

/* Magic constants */
#define CSQ_RSSI_UNKNOWN		     (99)
#define CESQ_RSRP_UNKNOWN		     (255)
#define CESQ_RSRQ_UNKNOWN		     (255)

/* Magic numbers to units conversions */
#define CSQ_RSSI_TO_DB(v) (-113 + (2 * (v)))
#define CESQ_RSRP_TO_DB(v) (-140 + (v))
#define CESQ_RSRQ_TO_DB(v) (-20 + ((v) / 2))

BUILD_ASSERT(sizeof(enum modem_cellular_event) == 1, "Event enum expanded");

#ifdef CONFIG_MODEM_CELLULAR_APN
BUILD_ASSERT(sizeof(CONFIG_MODEM_CELLULAR_APN) - 1 < MODEM_CELLULAR_DATA_APN_LEN,
			"CONFIG_MODEM_CELLULAR_APN too long for data->apn");
#endif

static void modem_cellular_enter_state(struct modem_cellular_data *data,
				       enum modem_cellular_state state);

static void modem_cellular_delegate_event(struct modem_cellular_data *data,
					  enum modem_cellular_event evt);

static void modem_cellular_delegate_event_ptr(struct modem_cellular_data *data,
					      enum modem_cellular_event evt, const void *ptr);

static void modem_cellular_event_handler(struct modem_cellular_data *data,
					 enum modem_cellular_event evt);

static void modem_cellular_connect_cmux_power_off(struct modem_cellular_data *data);

static void modem_cellular_begin_sim_switch(struct modem_cellular_data *data);

static void modem_cellular_sim_switch_revert(struct modem_cellular_data *data);

/* Steps of a SIM slot switch with the modem running */
enum modem_cellular_sim_switch_step {
	MODEM_CELLULAR_SIM_SWITCH_DETACH,
	MODEM_CELLULAR_SIM_SWITCH_DEACTIVATE,
	MODEM_CELLULAR_SIM_SWITCH_SELECT,
	MODEM_CELLULAR_SIM_SWITCH_ACTIVATE,
};

static const char *modem_cellular_state_str(enum modem_cellular_state state)
{
	switch (state) {
	case MODEM_CELLULAR_STATE_IDLE:
		return "idle";
	case MODEM_CELLULAR_STATE_RECOVERY:
		return "recovery";
	case MODEM_CELLULAR_STATE_RESET_PULSE:
		return "reset pulse";
	case MODEM_CELLULAR_STATE_AWAIT_RESET:
		return "await reset";
	case MODEM_CELLULAR_STATE_POWER_ON_PULSE:
		return "power pulse";
	case MODEM_CELLULAR_STATE_AWAIT_POWER_ON:
		return "await power on";
	case MODEM_CELLULAR_STATE_SET_BAUDRATE:
		return "set baudrate";
	case MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT:
		return "run init script";
	case MODEM_CELLULAR_STATE_CONNECT_CMUX:
		return "connect cmux";
	case MODEM_CELLULAR_STATE_OPEN_DLCI1:
		return "open dlci1";
	case MODEM_CELLULAR_STATE_OPEN_DLCI2:
		return "open dlci2";
	case MODEM_CELLULAR_STATE_RUN_SIM_SELECT_SCRIPT:
		return "run sim select script";
	case MODEM_CELLULAR_STATE_RUN_SIM_SWITCH_SCRIPT:
		return "run sim switch script";
	case MODEM_CELLULAR_STATE_RUN_BOARD_INIT_SCRIPT:
		return "run board init script";
	case MODEM_CELLULAR_STATE_WAIT_FOR_APN:
		return "wait for apn";
	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
		return "await registered";
	case MODEM_CELLULAR_STATE_RUN_APN_SCRIPT:
		return "run apn script";
	case MODEM_CELLULAR_STATE_RUN_NETWORK_SCRIPT:
		return "run network script";
	case MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT:
		return "run dial script";
	case MODEM_CELLULAR_STATE_REGISTERED:
		return "registered";
	case MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD:
		return "await PPP dead";
	case MODEM_CELLULAR_STATE_INIT_POWER_OFF:
		return "init power off";
	case MODEM_CELLULAR_STATE_RUN_SHUTDOWN_SCRIPT:
		return "run shutdown script";
	case MODEM_CELLULAR_STATE_POWER_OFF_PULSE:
		return "power off pulse";
	case MODEM_CELLULAR_STATE_AWAIT_POWER_OFF:
		return "await power off";
	case MODEM_CELLULAR_STATE_AWAIT_DIAL:
		return "await dial";
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
	case MODEM_CELLULAR_EVENT_CMUX_DISCONNECTED:
		return "cmux disconnected";
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
	case MODEM_CELLULAR_EVENT_PPP_DEAD:
		return "ppp dead";
	case MODEM_CELLULAR_EVENT_MODEM_READY:
		return "modem ready";
	case MODEM_CELLULAR_EVENT_APN_SET:
		return "apn set";
	case MODEM_CELLULAR_EVENT_RING:
		return "RING";
	case MODEM_CELLULAR_EVENT_PERIODIC_KICK:
		return "periodic kick";
	case MODEM_CELLULAR_EVENT_DIAL:
		return "dial";
	case MODEM_CELLULAR_EVENT_HANGUP:
		return "hangup";
	case MODEM_CELLULAR_EVENT_SIM_SWITCH:
		return "sim switch";
	}

	return "";
}

static bool modem_cellular_apn_change_allowed(enum modem_cellular_state st)
{
	switch (st) {
	case MODEM_CELLULAR_STATE_IDLE:
	case MODEM_CELLULAR_STATE_RESET_PULSE:
	case MODEM_CELLULAR_STATE_AWAIT_RESET:
	case MODEM_CELLULAR_STATE_POWER_ON_PULSE:
	case MODEM_CELLULAR_STATE_AWAIT_POWER_ON:
	case MODEM_CELLULAR_STATE_SET_BAUDRATE:
	case MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT:
	case MODEM_CELLULAR_STATE_CONNECT_CMUX:
	case MODEM_CELLULAR_STATE_OPEN_DLCI1:
	case MODEM_CELLULAR_STATE_OPEN_DLCI2:
	case MODEM_CELLULAR_STATE_RUN_SIM_SELECT_SCRIPT:
	case MODEM_CELLULAR_STATE_RUN_BOARD_INIT_SCRIPT:
	case MODEM_CELLULAR_STATE_WAIT_FOR_APN:
		return true;
	default:
		return false;
	}
}

static bool modem_cellular_has_network_script(const struct modem_cellular_config *config)
{
	return config->vendor->scripts.network != NULL;
}

/* SIM slots without a MUX are SIM interfaces of the modem, which selects them */
static bool modem_cellular_modem_selects_sim(const struct modem_cellular_config *config)
{
	return (config->sim_slots_size > 0U) && (config->sim_slots[0].mux == NULL);
}

void modem_cellular_emit_event(struct modem_cellular_data *data,
			       enum cellular_event evt, const void *payload)
{
	if ((data->cb.fn != NULL) && (data->cb.mask & evt)) {
		data->cb.fn(data->dev, evt, payload, data->cb.user_data);
	}
}

/*
 * Compares the fields that constitute a network-status change. Signal quality
 * (rsrp/rsrq) is deliberately excluded: it fluctuates on nearly every poll and
 * would otherwise re-fire the event continuously.
 */
static bool modem_cellular_network_status_equal(const struct cellular_evt_network_status *a,
						const struct cellular_evt_network_status *b)
{
	if (a->status != b->status || a->access_tech != b->access_tech) {
		return false;
	}

	/* Cell identity */
	if (a->cell.lte.mcc != b->cell.lte.mcc || a->cell.lte.mnc != b->cell.lte.mnc ||
	    a->cell.lte.tac != b->cell.lte.tac || a->cell.lte.gci != b->cell.lte.gci) {
		return false;
	}

	/* Radio channel */
	if (a->cell.lte.earfcn != b->cell.lte.earfcn || a->cell.lte.band != b->cell.lte.band ||
	    a->cell.lte.phys_cell_id != b->cell.lte.phys_cell_id) {
		return false;
	}

	return true;
}

void modem_cellular_emit_network_status(struct modem_cellular_data *data,
					const struct cellular_evt_network_status *status)
{
	bool changed;

	/* api_lock serialises the cache against get_network_status(); emitters run
	 * on the modem work queue and must not already hold it. The cache always
	 * takes the latest value (so the getter reports current signal), but the
	 * change test ignores signal quality.
	 */
	k_mutex_lock(&data->api_lock, K_FOREVER);
	changed = !data->network_status_valid ||
		  !modem_cellular_network_status_equal(&data->network_status, status);
	data->network_status = *status;
	data->network_status_valid = true;
	k_mutex_unlock(&data->api_lock);

	/* Emit outside the lock, and only on a real change so periodic pollers do
	 * not re-fire the event every cycle.
	 */
	if (changed) {
		modem_cellular_emit_event(data, CELLULAR_EVENT_NETWORK_STATUS_CHANGED, status);
	}
}

static void modem_cellular_invalidate_network_status(struct modem_cellular_data *data)
{
	k_mutex_lock(&data->api_lock, K_FOREVER);
	data->network_status_valid = false;
	k_mutex_unlock(&data->api_lock);
}

static void modem_cellular_emit_modem_info(struct modem_cellular_data *data,
					   enum cellular_modem_info_type field)
{
	struct cellular_evt_modem_info evt = {
		.field = field,
	};

	modem_cellular_emit_event(data, CELLULAR_EVENT_MODEM_INFO_CHANGED, &evt);
}

static void modem_cellular_emit_reg_state(struct modem_cellular_data *data,
					   enum cellular_registration_status status)
{
	struct cellular_evt_registration_status evt = {
		.status = status,
	};

	modem_cellular_emit_event(data, CELLULAR_EVENT_REGISTRATION_STATUS_CHANGED, &evt);
}

static bool modem_cellular_gpio_is_enabled(const struct gpio_dt_spec *gpio)
{
	return gpio->port != NULL;
}

static bool modem_cellular_modem_is_powered(const struct modem_cellular_config *config)
{
	int ret;

	/* A status GPIO reads asserted while the modem is already powered on, e.g.
	 * after an MCU-only reset that left the modem running. Without a status GPIO,
	 * report the modem as off so the normal power-on path runs unchanged.
	 */
	if (!modem_cellular_gpio_is_enabled(&config->status_gpio)) {
		return false;
	}

	ret = gpio_pin_get_dt(&config->status_gpio);
	if (ret < 0) {
		LOG_WRN("Failed to read modem status GPIO (%d)", ret);
		return false;
	}

	return ret == 1;
}

static bool modem_cellular_needs_apn(const struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	if (config->use_default_apn) {
		return false;
	}
	return data->apn[0] == '\0';
}

static void modem_cellular_enter_state_network_or_dial(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	if (modem_cellular_has_network_script(config)) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_NETWORK_SCRIPT);
	} else if (IS_ENABLED(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT)) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_DIAL);
	} else {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT);
	}
}

static void modem_cellular_notify_user_pipes_connected(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;
	struct modem_cellular_user_pipe *user_pipe;
	struct modem_pipelink *pipelink;

	for (uint8_t i = 0; i < config->user_pipes_size; i++) {
		user_pipe = &config->user_pipes[i];
		pipelink = user_pipe->pipelink;
		modem_pipelink_notify_connected(pipelink);
	}
}

static void modem_cellular_notify_user_pipes_disconnected(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;
	struct modem_cellular_user_pipe *user_pipe;
	struct modem_pipelink *pipelink;

	for (uint8_t i = 0; i < config->user_pipes_size; i++) {
		user_pipe = &config->user_pipes[i];
		pipelink = user_pipe->pipelink;
		modem_pipelink_notify_disconnected(pipelink);
	}
}

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

void modem_cellular_chat_callback_handler(struct modem_chat *chat,
					  enum modem_chat_script_result result,
					  const struct modem_chat_script_completion_info *info,
					  void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	const struct modem_chat_script *script = info->script;

	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		modem_cellular_delegate_event_ptr(data, MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS,
						  script);
	} else {
		modem_cellular_delegate_event_ptr(data, MODEM_CELLULAR_EVENT_SCRIPT_FAILED, script);
	}
}

void modem_cellular_chat_on_modem_ready(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_cellular_data *data = user_data;

	modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_MODEM_READY);
}


void modem_cellular_chat_on_imei(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	/* Any modem using this callback is making the assumption that IMEI == SN,
	 * as the documented response to 'AT+CGSN' is the SN, not the IMEI.
	 */
	strncpy(data->imei, argv[1], sizeof(data->imei) - 1);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_IMEI);
	strncpy(data->sn, argv[1], sizeof(data->sn) - 1);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_SERIAL_NUMBER);
}

void modem_cellular_chat_on_cgsn_sn(struct modem_chat *chat, char **argv, uint16_t argc,
				    void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->sn, argv[1], sizeof(data->sn) - 1);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_SERIAL_NUMBER);
}

void modem_cellular_chat_on_cgsn_imei(struct modem_chat *chat, char **argv, uint16_t argc,
				      void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	const char *rsp;
	size_t rsp_len;

	if (argc != 2) {
		return;
	}

	/* 3GPP specifies 15 digit string type in decimal format */
	rsp = argv[1];
	rsp_len = strlen(rsp);
	if ((rsp_len != 17) || (rsp[0] != '"') || (rsp[16] != '"')) {
		LOG_WRN("Invalid CGSN response: %s", rsp);
		return;
	}

	/* IMEI from AT+CGSN is string quoted.
	 * 3GPP specifies the length as exactly 15 digits.
	 * Start from offset 1 to skip first quote character.
	 */
	memcpy(data->imei, rsp + 1, 15);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_IMEI);
}

void modem_cellular_chat_on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->model_id, argv[1], sizeof(data->model_id) - 1);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_MODEL_ID);
}

void modem_cellular_chat_on_cgmi(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->manufacturer, argv[1], sizeof(data->manufacturer) - 1);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_MANUFACTURER);
}

void modem_cellular_chat_on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->fw_version, argv[1], sizeof(data->fw_version) - 1);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_FW_VERSION);
}

void modem_cellular_chat_on_csq(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 3) {
		return;
	}

	data->rssi = (uint8_t)atoi(argv[1]);
}

void modem_cellular_chat_on_cesq(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 7) {
		return;
	}

	data->rsrq = (uint8_t)atoi(argv[5]);
	data->rsrp = (uint8_t)atoi(argv[6]);
}

void modem_cellular_chat_on_iccid(struct modem_chat *chat, char **argv, uint16_t argc,
				  void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->iccid, argv[1], sizeof(data->iccid) - 1);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_SIM_ICCID);
}

void modem_cellular_chat_on_imsi(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	if (argc != 2) {
		return;
	}

	strncpy(data->imsi, argv[1], sizeof(data->imsi) - 1);
	modem_cellular_emit_modem_info(data, CELLULAR_MODEM_INFO_SIM_IMSI);
}

/**
 * @brief Store the SIM interface the modem reports as selected.
 *
 * Match callback for the vendor SIM query script. A value that is not an index from 0 to
 * 255 is ignored, and the selected interface then stays unknown.
 *
 * @param chat Chat instance, unused.
 * @param argv Match arguments: the match string and the index of the SIM interface.
 * @param argc Number of match arguments, 2.
 * @param user_data Pointer to the associated @ref modem_cellular_data object.
 */
void modem_cellular_chat_on_sim_slot(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	long slot;
	char *end;

	if (argc != 2) {
		return;
	}

	slot = strtol(argv[1], &end, 10);
	if ((end == argv[1]) || (slot < 0) || (slot > UINT8_MAX)) {
		return;
	}

	data->sim_slot_modem = (int16_t)slot;
}

static bool modem_cellular_status_is_registered(enum cellular_registration_status status)
{
	return (status == CELLULAR_REGISTRATION_REGISTERED_HOME) ||
	       (status == CELLULAR_REGISTRATION_REGISTERED_ROAMING);
}

static bool modem_cellular_is_registered(struct modem_cellular_data *data)
{
	return modem_cellular_status_is_registered(data->registration_status_gsm) ||
	       modem_cellular_status_is_registered(data->registration_status_gprs) ||
	       modem_cellular_status_is_registered(data->registration_status_lte) ||
	       modem_cellular_status_is_registered(data->registration_status_5g);
}

/* Whether a data call can be made: registered, or with the data context active per +CGEV */
static bool modem_cellular_is_attached(struct modem_cellular_data *data)
{
	return modem_cellular_is_registered(data) || data->pdn_active;
}

static void modem_cellular_clear_registration_status(struct modem_cellular_data *data)
{
	data->registration_status_gsm = CELLULAR_REGISTRATION_NOT_REGISTERED;
	data->registration_status_gprs = CELLULAR_REGISTRATION_NOT_REGISTERED;
	data->registration_status_lte = CELLULAR_REGISTRATION_NOT_REGISTERED;
	data->registration_status_5g = CELLULAR_REGISTRATION_NOT_REGISTERED;
	data->pdn_active = false;
}

#if defined(CONFIG_MODEM_CELLULAR_STATS)
static void modem_cellular_stats_on_reg_transition(struct modem_cellular_data *data,
						   bool was_registered, bool is_registered)
{
	if (was_registered && !is_registered) {
		data->stats.deregistrations += 1;
		data->stats_outage_start_ms = k_uptime_get_32();
	} else if (!was_registered && is_registered && (data->stats_outage_start_ms != 0)) {
		data->stats.outage_ms += k_uptime_get_32() - data->stats_outage_start_ms;
		data->stats_outage_start_ms = 0;
	} else {
		/* No change in registration state; nothing to record. */
	}
}

static void modem_cellular_chat_on_cme_error(struct modem_chat *chat, char **argv, uint16_t argc,
					     void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;

	data->stats.command_errors_cme += 1;
}
#endif

void modem_cellular_chat_on_cxreg(struct modem_chat *chat, char **argv, uint16_t argc,
				  void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	enum cellular_registration_status registration_status = CELLULAR_REGISTRATION_UNKNOWN;
	enum cellular_registration_status registration_prev;
	bool packet_domain = true;
	uint8_t num_args;
	uint8_t base;

	/* This receives both +C*REG? read command answers and unsolicited notifications.
	 * Their syntax differs in that the former has one more parameter, <n>, which is first.
	 */
	if (argc >= 3 && argv[2][0] != '"') {
		/* +CEREG: <n>,<stat>[,<tac>[...]] */
		base = 2;
	} else if (argc >= 2) {
		/* +CEREG: <stat>[,<tac>[...]] */
		base = 1;
	} else {
		return;
	}
	/* Long form of the various CXREG options:
	 *   +CREG: <stat>[,<lac>,<ci>[,<AcT>]]
	 *   +CGREG:<stat>[,<lac>,<ci>[,<AcT>,<rac>]]
	 *   +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>]]
	 *   +C5GREG: <stat>[,[<tac>],[<ci>],[<AcT>],[<Allowed_NSSAI_length>],[<Allowed_NSSAI>]]
	 */
	num_args = argc - base;
	registration_status = atoi(argv[base]);
	if (num_args >= 4) {
		data->access_tech = strtol(argv[base + 3], NULL, 10);
	}
	LOG_DBG("REG %d AcT %d", registration_status, data->access_tech);

	IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS,
		   (const bool was_registered = modem_cellular_is_registered(data)));

	if (strcmp(argv[0], "+CREG: ") == 0) {
		/* Circuit-switched, so its status says nothing about the data context */
		packet_domain = false;
		registration_prev = data->registration_status_gsm;
		data->registration_status_gsm = registration_status;
	} else if (strcmp(argv[0], "+CGREG: ") == 0) {
		registration_prev = data->registration_status_gprs;
		data->registration_status_gprs = registration_status;
	} else if (strcmp(argv[0], "+C5GREG: ") == 0) {
		registration_prev = data->registration_status_5g;
		data->registration_status_5g = registration_status;
	} else { /* CEREG */
		registration_prev = data->registration_status_lte;
		data->registration_status_lte = registration_status;
	}

	IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS,
		   (modem_cellular_stats_on_reg_transition(data, was_registered,
							   modem_cellular_is_registered(data))));

	/* Losing packet-domain registration loses the context, unless it was never reported.
	 * This holds even while another RAT, such as circuit-switched +CREG, stays registered.
	 */
	if (packet_domain && modem_cellular_status_is_registered(registration_prev) &&
	    !modem_cellular_status_is_registered(registration_status)) {
		data->pdn_active = false;
	}

	if (modem_cellular_is_registered(data)) {
		/* Drop any cached serving cell on a registration change; it is stale
		 * until the next periodic poll, so get_network_status() reports no
		 * data rather than the previous (deregistered) snapshot.
		 */
		if (registration_prev != registration_status) {
			modem_cellular_invalidate_network_status(data);
		}

		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_REGISTERED);
	} else {
		/* A context that alone counted as registration keeps the data call up */
		if (!modem_cellular_is_attached(data)) {
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_DEREGISTERED);
		}
		/* On deregistration report the status, as periodic network status AT
		 * commands are not guaranteed to respond normally.
		 */
		if (registration_prev != registration_status) {
			struct cellular_evt_network_status evt = {
				.status = registration_status,
				.access_tech = data->access_tech,
			};

			modem_cellular_emit_network_status(data, &evt);
		}
	}
	modem_cellular_emit_reg_state(data, registration_status);
}

/* Context ID in a +CGEV parameter, or -1 when the parameter is empty or not numeric */
static int modem_cellular_cgev_parse_cid(const char *param)
{
	char *end;
	long cid = strtol(param, &end, 10);

	return ((end == param) || (cid < 0)) ? -1 : (int)cid;
}

/* Context ID of a +CGEV ACT or DEACT event, or -1 when the event does not carry one */
static int modem_cellular_cgev_cid(char **argv, uint16_t argc)
{
	/* The last word of the event is <cid>, <p_cid> or <PDP_type> */
	const char *param = strrchr(argv[1], ' ');

	if (param == NULL) {
		return -1;
	}
	param++;

	if ((*param < '0') || (*param > '9')) {
		/* DEACT <PDP_type>, <PDP_addr>, [<cid>], the <cid> only given if known to the MT */
		return (argc > 3) ? modem_cellular_cgev_parse_cid(argv[3]) : -1;
	}
	if (strstr(argv[1], "PDN") != NULL) {
		/* PDN ACT <cid> or PDN DEACT <cid> */
		return modem_cellular_cgev_parse_cid(param);
	}
	/* ACT <p_cid>, <cid>, <event_type> or DEACT <p_cid>, <cid>, <event_type> */
	return (argc > 2) ? modem_cellular_cgev_parse_cid(argv[2]) : -1;
}

void modem_cellular_chat_on_cgev(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)user_data;
	const struct modem_cellular_config *config = data->dev->config;
	/* The modem dials the default context itself, otherwise the driver dials its own */
	const int data_cid =
		config->use_default_pdp_context ? 0 : CONFIG_MODEM_CELLULAR_PDP_CONTEXT_ID;
	bool connected;
	int cid;

	if ((argc < 2) || (strcmp(argv[0], "+CGEV: ") != 0)) {
		return;
	}
	/* See 3GPP TS 27.007 for +CGEV syntax
	 *
	 * Any of the following means PDN connected:
	 * +CGEV: NW PDN ACT <cid>
	 * +CGEV: ME PDN ACT <cid>[,<reason>[,<cid_other>]]
	 * +CGEV: NW ACT <p_cid>, <cid>, <event_type>
	 * +CGEV: ME ACT <p_cid>, <cid>, <event_type>
	 *
	 * Any of the following means PDN disconnected:
	 * +CGEV: NW DETACH
	 * +CGEV: ME DETACH
	 * +CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]
	 * +CGEV: ME DEACT <PDP_type>, <PDP_addr>, [<cid>]
	 * +CGEV: NW PDN DEACT <cid>
	 * +CGEV: ME PDN DEACT <cid>
	 * +CGEV: NW DEACT <p_cid>, <cid>, <event_type>
	 * +CGEV: ME DEACT <p_cid>, <cid>, <event_type>
	 */

	/* argv[0] is the match, the event up to the first ',' follows in argv[1] */
	if (strstr(argv[1], "DETACH") != NULL) {
		IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS,
			   (const bool was_registered = modem_cellular_is_registered(data)));

		/* Detaching from the packet domain ends every context and its registration,
		 * leaving only circuit-switched +CREG.
		 */
		data->registration_status_gprs = CELLULAR_REGISTRATION_NOT_REGISTERED;
		data->registration_status_lte = CELLULAR_REGISTRATION_NOT_REGISTERED;
		data->registration_status_5g = CELLULAR_REGISTRATION_NOT_REGISTERED;
		IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS,
			   (modem_cellular_stats_on_reg_transition(
				   data, was_registered, modem_cellular_is_registered(data))));
		connected = false;
	} else if ((strstr(argv[1], " ACT") != NULL) || (strstr(argv[1], "DEACT") != NULL)) {
		cid = modem_cellular_cgev_cid(argv, argc);
		/* Events for other contexts do not affect the data call. An event without a
		 * <cid> may be for the data call, so it is assumed to be.
		 */
		if ((cid >= 0) && (cid != data_cid)) {
			return;
		}
		connected = (strstr(argv[1], " ACT") != NULL);
	} else {
		return;
	}

	/* Retained, as the states that ignore the events below still need it on leaving */
	data->pdn_active = connected;

	/* We might stay registered while losing the PDN connection, so a disconnect is
	 * always reported. It cannot be gated on the registration status, which is never
	 * set when a PDN activation alone counted as registration. States without a data
	 * call ignore it.
	 */
	if (!connected) {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_DEREGISTERED);
	} else if (!modem_cellular_is_registered(data)) {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_REGISTERED);
	}
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(__maybe_unused allow_match,
			  MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH("ERROR", "", NULL));

MODEM_CHAT_MATCH_DEFINE(imei_match __maybe_unused, "", "", modem_cellular_chat_on_imei);
MODEM_CHAT_MATCH_DEFINE(cgmm_match __maybe_unused, "", "", modem_cellular_chat_on_cgmm);
MODEM_CELLULAR_OK_CHAT_MATCH_DEFINE(__maybe_unused csq_match, "+CSQ: ", ",",
				    modem_cellular_chat_on_csq);
MODEM_CELLULAR_OK_CHAT_MATCH_DEFINE(__maybe_unused cesq_match, "+CESQ: ", ",",
				    modem_cellular_chat_on_cesq);
MODEM_CHAT_MATCH_DEFINE(qccid_match __maybe_unused, "+QCCID: ", "", modem_cellular_chat_on_iccid);
MODEM_CHAT_MATCH_DEFINE(iccid_match __maybe_unused, "+ICCID: ", "", modem_cellular_chat_on_iccid);
MODEM_CHAT_MATCH_DEFINE(ccid_match __maybe_unused, "+CCID: ", "", modem_cellular_chat_on_iccid);
MODEM_CHAT_MATCH_DEFINE(cimi_match __maybe_unused, "", "", modem_cellular_chat_on_imsi);
MODEM_CHAT_MATCH_DEFINE(cgmi_match __maybe_unused, "", "", modem_cellular_chat_on_cgmi);
MODEM_CHAT_MATCH_DEFINE(cgmr_match __maybe_unused, "", "", modem_cellular_chat_on_cgmr);

MODEM_CHAT_MATCHES_DEFINE(abort_matches,
			  MODEM_CHAT_MATCH("ERROR", "", NULL),
			  MODEM_CHAT_MATCH("+CME ERROR", "",
					   COND_CODE_1(CONFIG_MODEM_CELLULAR_STATS,
						       (modem_cellular_chat_on_cme_error),
						       (NULL))));

/* Detach from the packet domain (3GPP TS 27.007), which ends the data call */
MODEM_CHAT_SCRIPT_CMDS_DEFINE(detach_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGATT=0", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(detach_chat_script, detach_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler,
			 CONFIG_MODEM_CELLULAR_DETACH_TIMEOUT);

MODEM_CHAT_MATCHES_DEFINE(__maybe_unused dial_abort_matches,
			  MODEM_CHAT_MATCH("ERROR", "", NULL),
			  MODEM_CHAT_MATCH("BUSY", "", NULL),
			  MODEM_CHAT_MATCH("NO ANSWER", "", NULL),
			  MODEM_CHAT_MATCH("NO CARRIER", "", NULL),
			  MODEM_CHAT_MATCH("NO DIALTONE", "", NULL));

static int append_apn_cmd(struct modem_cellular_data *data, uint8_t *steps, const char *fmt,
			  const char *apn_value)
{
	int n;

	if (*steps >= MODEM_CELLULAR_MAX_APN_CMDS) {
		return -ENOMEM;
	}

	n = snprintk(data->apn_buf[*steps], MODEM_CELLULAR_APN_BUF_SIZE, fmt, apn_value);

	if (n < 0 || n >= MODEM_CELLULAR_APN_BUF_SIZE) {
		return -ENOMEM;
	}

	/* Fill the matching chat entry */
	modem_chat_script_chat_init(&data->apn_chats[*steps]);
	modem_chat_script_chat_set_request(&data->apn_chats[*steps], data->apn_buf[*steps]);
	modem_chat_script_chat_set_response_matches(&data->apn_chats[*steps], &ok_match, 1);

	(*steps)++;
	return 0;
}

static void modem_cellular_build_apn_script(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;
	uint8_t steps = 0;
	const char *apn_value = data->apn;

	if (config->use_default_apn) {
		/* Omit the APN name AT+CGDCONT=<cid>,"IP","" */
		apn_value = "";
	}
	append_apn_cmd(data, &steps,
		       "AT+CGDCONT=" STRINGIFY(CONFIG_MODEM_CELLULAR_PDP_CONTEXT_ID)
		       ",\"IP\",\"%s\"",
		       apn_value);

	/* Vendor‑specific extras */
#if DT_HAS_COMPAT_STATUS_OKAY(swir_hl7800)
	append_apn_cmd(data, &steps,
		       "AT+KCNXCFG=" STRINGIFY(CONFIG_MODEM_CELLULAR_PDP_CONTEXT_ID)
		       ",\"GPRS\",\"%s\",,,\"IPV4\"",
		       apn_value);
#endif

	/* Glue the array into the script object */
	modem_chat_script_set_script_chats(&data->apn_script, data->apn_chats, steps);
}

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

static void modem_cellular_update_timer(struct modem_cellular_data *data, k_timeout_t timeout)
{
	k_work_reschedule(&data->timeout_work, timeout);
}

static void modem_cellular_stop_timer(struct modem_cellular_data *data)
{
	k_work_cancel_delayable(&data->timeout_work);
}

/* Record the script each start runs, which the dispatcher checks every script
 * result against.
 */
static int modem_cellular_run_script(struct modem_cellular_data *data,
				     const struct modem_chat_script *script)
{
	int ret;

	ret = modem_chat_run_script_async(&data->chat, script);
	if (ret == 0) {
		data->script_running = script;
	}

	return ret;
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
	struct modem_cellular_event_pkg pkg;

	while (k_msgq_get(&data->event_queue, &pkg, K_NO_WAIT) == 0) {
		data->event_ptr = pkg.ptr;
		modem_cellular_event_handler(data, pkg.event);
	}
}

/**
 * @brief Queue an event for the state machine.
 *
 * @param data Modem instance data.
 * @param evt Event.
 * @param ptr Pointer passed along with the event, or NULL.
 *
 * @retval 0 Success.
 * @retval -ENOMSG Event queue is full, the event is dropped.
 */
static int modem_cellular_queue_event(struct modem_cellular_data *data,
				      enum modem_cellular_event evt, const void *ptr)
{
	struct modem_cellular_event_pkg pkg = {.event = evt, .ptr = ptr};
	int ret;

	ret = k_msgq_put(&data->event_queue, &pkg, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	k_work_submit(&data->event_dispatch_work);
	return 0;
}

static void modem_cellular_delegate_event_ptr(struct modem_cellular_data *data,
					      enum modem_cellular_event evt, const void *ptr)
{
	if (modem_cellular_queue_event(data, evt, ptr) < 0) {
		LOG_WRN("Event %d dropped", evt);
	}
}

static void modem_cellular_delegate_event(struct modem_cellular_data *data,
					  enum modem_cellular_event evt)
{
	return modem_cellular_delegate_event_ptr(data, evt, NULL);
}

static void modem_cellular_begin_power_off_pulse(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	modem_pipe_close_async(data->uart_pipe);

	if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_POWER_OFF_PULSE);
	} else {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
	}
}

static void modem_cellular_begin_shutdown(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	/* Shutdown script can only be used if cmd_pipe is available, i.e. we are not in
	 * some intermediary state without a pipe for commands available
	 */
	if (config->vendor->scripts.shutdown != NULL && data->cmd_pipe != NULL) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_SHUTDOWN_SCRIPT);
		return;
	}

	modem_cellular_begin_power_off_pulse(data);
}

static uint32_t modem_cellular_baudrate_update(struct modem_cellular_data *data,
					       uint32_t desired_baudrate)
{
	const struct modem_cellular_config *config = data->dev->config;
	struct uart_config cfg = {0};
	uint32_t original_baudrate;
	int ret;

	ret = uart_config_get(config->uart, &cfg);
	if (ret < 0) {
		LOG_ERR("Failed to get UART configuration (%d)", ret);
		return 0;
	}
	original_baudrate = cfg.baudrate;
	cfg.baudrate = desired_baudrate;
	ret = uart_configure(config->uart, &cfg);
	if (ret < 0) {
		LOG_ERR("Failed to set new baudrate (%d)", ret);
		return 0;
	}
	return original_baudrate;
}

static int modem_cellular_on_idle_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	if (modem_cellular_gpio_is_enabled(&config->wake_gpio)) {
		gpio_pin_set_dt(&config->wake_gpio, 0);
	}

	if (modem_cellular_gpio_is_enabled(&config->reset_gpio) && config->hold_reset_on_suspend) {
		gpio_pin_set_dt(&config->reset_gpio, 1);
		data->powered_off = true;
	}

	data->recovery_count = 0;
	/* Modem is powered off, so no CMUX connect attempt is outstanding */
	data->suspend_pending = false;
	data->cmux_connect_failed = false;
	data->cmux_bus_opened = false;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	/* A switch interrupted before its MUX state was applied leaves the modem routed to the
	 * previous slot. If the modem stays powered, as on a restart, switch it again once the
	 * modem runs, unless another switch was requested since.
	 */
	if (data->sim_switching && !data->powered_off && (data->sim_slot_active->mux != NULL) &&
	    (data->sim_switch_step < MODEM_CELLULAR_SIM_SWITCH_ACTIVATE)) {
		if (data->sim_slot_requested == NULL) {
			data->sim_slot_requested = data->sim_slot_active;
		}

		data->sim_slot_active = data->sim_slot_previous;
	}

	/* A switch requested with the modem running is applied at the next power-up, unless a
	 * MUX would switch the SIM lines of a modem the driver left powered: it is then
	 * switched with the modem running.
	 */
	if ((data->sim_slot_requested != NULL) &&
	    ((data->sim_slot_requested->mux == NULL) || data->powered_off)) {
		data->sim_slot_active = data->sim_slot_requested;
		data->sim_slot_requested = NULL;
		memset(data->iccid, 0, sizeof(data->iccid));
		memset(data->imsi, 0, sizeof(data->imsi));
	}

	data->sim_switching = false;
	k_mutex_unlock(&data->api_lock);

	modem_cellular_clear_registration_status(data);
	modem_cellular_notify_user_pipes_disconnected(data);
	modem_chat_release(&data->chat);
	modem_ppp_release(config->ppp);
	modem_cmux_release(&data->cmux);
	modem_pipe_close_async(data->uart_pipe);
	k_sem_give(&data->suspended_sem);
	modem_cellular_emit_event(data, CELLULAR_EVENT_MODEM_SUSPENDED, NULL);
	return 0;
}

/**
 * @brief Route the modem to the active SIM slot.
 *
 * Applies the MUX state of the active SIM slot. Does nothing when the modem has no
 * sim-slots node.
 *
 * @param data Modem instance data.
 *
 * @retval 0 Success.
 * @retval -errno Error from mux_state_apply(); the MUX may route the modem to another slot.
 */
static int modem_cellular_apply_sim_slot(struct modem_cellular_data *data)
{
	const struct modem_cellular_sim_slot *slot = data->sim_slot_active;
	int ret;

	if ((slot == NULL) || (slot->mux == NULL)) {
		return 0;
	}

	ret = mux_state_apply(slot->mux, slot->state);
	if (ret < 0) {
		LOG_ERR("Failed to select SIM slot %u (%d)", slot->index, ret);
	}

	return ret;
}

/**
 * @brief Leave the idle state to power up the modem.
 *
 * Routes the modem to the active SIM slot, then enters the first power-up state that
 * applies to the modem's configuration. The modem stays idle if it cannot be routed to
 * the active SIM slot, rather than run on a SIM other than the one reported.
 *
 * @param data Modem instance data.
 */
static void modem_cellular_idle_resume(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	data->power_on_skipped = false;

	/* The MUX driver sets its own state when it starts, and the slot may have been
	 * changed while suspended, so route the modem to the active slot before it powers up.
	 */
	if (modem_cellular_apply_sim_slot(data) < 0) {
		LOG_ERR("Modem kept idle");
		return;
	}

	if (config->autostarts || config->vendor->force_autostart) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_POWER_ON);
		return;
	}

	if (modem_cellular_gpio_is_enabled(&config->reset_gpio) && config->reset_on_resume) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RESET_PULSE);
		return;
	}

	if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
		if (!modem_cellular_modem_is_powered(config)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_POWER_ON_PULSE);
			return;
		}

		/* Modem is already powered (e.g. MCU-only reset). Pulsing the power
		 * key now would toggle a running modem off, so skip it and bring up
		 * the bus directly; the init script confirms the modem responds.
		 */
		data->power_on_skipped = true;
	}

	if (config->vendor->scripts.set_baudrate != NULL) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_SET_BAUDRATE);
	} else {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT);
	}
}

static void modem_cellular_idle_event_handler(struct modem_cellular_data *data,
					      enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_RESUME:
		/* Hold api_lock until the state has changed, so that select_sim(), which only
		 * switches the SIM slot in the idle state, cannot switch it while the modem
		 * powers up.
		 */
		k_mutex_lock(&data->api_lock, K_FOREVER);
		modem_cellular_idle_resume(data);
		k_mutex_unlock(&data->api_lock);
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
	const struct modem_cellular_config *config = data->dev->config;
	/* Without a status GPIO the modem stays powered unless a power GPIO can cycle it */
	const bool powered = modem_cellular_gpio_is_enabled(&config->status_gpio)
				     ? modem_cellular_modem_is_powered(config)
				     : !modem_cellular_gpio_is_enabled(&config->power_gpio);
	const bool baudrate_kept = data->baudrate_kept && powered &&
				   !(modem_cellular_gpio_is_enabled(&config->reset_gpio) &&
				     config->hold_reset_on_suspend);

	k_sem_take(&data->suspended_sem, K_NO_WAIT);
	data->baudrate_kept = false;
	data->powered_off = false;

	/* Revert to original baudrate if we have changed it. Boots that skip the power-on
	 * and reset pulses would otherwise reach the baudrate script at the new speed,
	 * unless the modem was left powered at it and still is.
	 */
	if ((data->original_baudrate != 0U) && !baudrate_kept) {
		modem_cellular_baudrate_update(data, data->original_baudrate);
	}

	if (modem_cellular_gpio_is_enabled(&config->reset_gpio) && config->hold_reset_on_suspend) {
		gpio_pin_set_dt(&config->reset_gpio, 0);
	}

	if (modem_cellular_gpio_is_enabled(&config->wake_gpio)) {
		gpio_pin_set_dt(&config->wake_gpio, 1);
	}

	return 0;
}

static int modem_cellular_on_reset_pulse_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	if (modem_cellular_gpio_is_enabled(&config->wake_gpio)) {
		gpio_pin_set_dt(&config->wake_gpio, 0);
	}

	/* Revert to original baudrate if we have changed it */
	if (data->original_baudrate) {
		modem_cellular_baudrate_update(data, data->original_baudrate);
	}

	gpio_pin_set_dt(&config->reset_gpio, 1);
	modem_cellular_start_timer(data, K_MSEC(config->vendor->reset_pulse_duration_ms));
	return 0;
}

static void modem_cellular_reset_pulse_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_RESET);
		} else {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_POWER_ON);
		}
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
	const struct modem_cellular_config *config = data->dev->config;

	gpio_pin_set_dt(&config->reset_gpio, 0);

	if (modem_cellular_gpio_is_enabled(&config->wake_gpio)) {
		gpio_pin_set_dt(&config->wake_gpio, 1);
	}

	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_await_reset_state_enter(struct modem_cellular_data *data)
{
	modem_cellular_start_timer(data, K_MSEC(CONFIG_MODEM_CELLULAR_RESET_POWER_ON_DELAY_MS));
	return 0;
}

static void modem_cellular_await_reset_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{	const struct modem_cellular_config *config = data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_POWER_ON_PULSE);
		} else {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_POWER_ON);
		}
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_await_reset_state_leave(struct modem_cellular_data *data)
{
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_power_on_pulse_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	/* Revert to original baudrate if we have changed it */
	if (data->original_baudrate) {
		modem_cellular_baudrate_update(data, data->original_baudrate);
	}

	gpio_pin_set_dt(&config->power_gpio, 1);
	modem_cellular_start_timer(data, K_MSEC(config->vendor->power_pulse_duration_ms));
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
	const struct modem_cellular_config *config = data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 0);
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_await_power_on_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	modem_cellular_start_timer(data, K_MSEC(config->vendor->startup_time_ms));
	modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
	return modem_pipe_open_async(data->uart_pipe);
}

static void modem_cellular_await_power_on_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		break;
	case MODEM_CELLULAR_EVENT_MODEM_READY:
		/* disable the timer and fall through, as we are ready to proceed */
		modem_cellular_stop_timer(data);
		__fallthrough;
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (config->vendor->scripts.set_baudrate != NULL) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_SET_BAUDRATE);
		} else {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_INIT_SCRIPT);
		}
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_set_baudrate_state_enter(struct modem_cellular_data *data)
{
	modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
	return modem_pipe_open_async(data->uart_pipe);
}

static void modem_cellular_set_baudrate_event_handler(struct modem_cellular_data *data,
						      enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		modem_cellular_run_script(data, config->vendor->scripts.set_baudrate);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		/* Some modems save the new speed on first change, meaning the
		 * modem is already at the new baudrate, meaning no reply. So
		 * ignore any failures and continue as if baudrate is already set
		 */
		LOG_DBG("no reply from modem, assuming baudrate is already set");
		__fallthrough;
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		/* Let modem reconfigure */
		modem_cellular_start_timer(data, K_MSEC(CONFIG_MODEM_CELLULAR_NEW_BAUDRATE_DELAY));
		modem_chat_release(&data->chat);
		modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
		modem_pipe_close_async(data->uart_pipe);
		break;

	case MODEM_CELLULAR_EVENT_BUS_CLOSED: {
		/* Update UART port baudrate. The UART may already be at the new baudrate when
		 * it was kept, so preserve the original value from the first change.
		 */
		const uint32_t baudrate =
			modem_cellular_baudrate_update(data, CONFIG_MODEM_CELLULAR_NEW_BAUDRATE);

		if (data->original_baudrate == 0U) {
			data->original_baudrate = baudrate;
		}
		break;
	}

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
	data->init_script_failed = false;
	modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
	return modem_pipe_open_async(data->uart_pipe);
}

static int modem_cellular_on_recovery_state_enter(struct modem_cellular_data *data)
{
	/* Back off before resetting. Growing the delay with the attempt count keeps a
	 * persistent fault from resetting the modem back-to-back, and gives a modem
	 * that is merely slow to respond some time to settle. Clamp it so a large
	 * attempt budget cannot produce an unbounded delay.
	 */
	uint32_t backoff_ms = MIN(CONFIG_MODEM_CELLULAR_RECOVERY_BACKOFF_MS * data->recovery_count,
				  CONFIG_MODEM_CELLULAR_RECOVERY_BACKOFF_MAX_MS);

	LOG_DBG("recovery attempt %u, backoff %u ms", data->recovery_count, backoff_ms);
	modem_cellular_start_timer(data, K_MSEC(backoff_ms));
	return 0;
}

static void modem_cellular_recovery_event_handler(struct modem_cellular_data *data,
						  enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		/* Release CMUX if attached so the UART pipe reattaches cleanly on the
		 * next connect attempt. No-op when CMUX is not attached.
		 */
		modem_cmux_release(&data->cmux);
		data->cmux_connect_failed = false;
		/* The power cycle below loses the registration and any active context */
		modem_cellular_clear_registration_status(data);

		if (modem_cellular_gpio_is_enabled(&config->reset_gpio) &&
		    config->reset_on_recovery) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RESET_PULSE);
		} else if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_POWER_ON_PULSE);
		} else {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		}
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		if (data->cmux_connect_failed) {
			/* Still powered and likely in CMUX mode, as when suspended mid connect */
			modem_cellular_connect_cmux_power_off(data);
			break;
		}
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_recovery_state_leave(struct modem_cellular_data *data)
{
	modem_cellular_stop_timer(data);
	return 0;
}

static void modem_cellular_enter_recovery_state(struct modem_cellular_data *data)
{
	IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS, (data->stats.recoveries += 1));

	/* Bound the recovery loop: a persistent fault parks in IDLE once the
	 * attempt budget is spent, instead of resetting forever. A zero budget
	 * disables the cap.
	 */
	if (CONFIG_MODEM_CELLULAR_MAX_RECOVERIES != 0 &&
	    data->recovery_count >= CONFIG_MODEM_CELLULAR_MAX_RECOVERIES) {
		LOG_WRN("Recovery attempts exhausted (%u), suspending", data->recovery_count);
		data->recovery_count = 0;
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SUSPEND);
		return;
	}

	/* Saturate rather than wrap so the backoff stays monotonic in unlimited mode. */
	if (data->recovery_count < UINT8_MAX) {
		data->recovery_count++;
	}

	modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RECOVERY);
}

static void modem_cellular_run_init_script_event_handler(struct modem_cellular_data *data,
							 enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;
	uint8_t imei_len;
	uint8_t link_addr_len;
	uint8_t *link_addr_ptr;
	int err;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		modem_cellular_run_script(data, config->vendor->scripts.init);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		/* Modem responded, so a skipped power-on pulse was the right call. */
		data->power_on_skipped = false;

		/* Get link_addr_len least significant bytes from IMEI as a link address */
		imei_len = MODEM_CELLULAR_DATA_IMEI_LEN - 1; /* Exclude str end */
		link_addr_len = MIN(NET_LINK_ADDR_MAX_LENGTH, imei_len);
		link_addr_ptr = data->imei + (imei_len - link_addr_len);

		err = net_if_set_link_addr(modem_ppp_get_iface(config->ppp), link_addr_ptr,
					   link_addr_len, NET_LINK_UNKNOWN);
		if (err) {
			LOG_WRN("Failed to set link address on PPP interface (%d)", err);
		}

		modem_chat_release(&data->chat);
		modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
		modem_pipe_close_async(data->uart_pipe);
		break;

	case MODEM_CELLULAR_EVENT_BUS_CLOSED:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_CONNECT_CMUX);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		/* The modem is left powered, at the baudrate the init script is sent at, unless
		 * the script failed at it and this is the suspend of exhausted recoveries.
		 */
		data->baudrate_kept = !data->init_script_failed;
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		data->init_script_failed = true;
		if (data->power_on_skipped) {
			/* Skipped the power-on pulse assuming the modem was already up, but
			 * it did not respond. Power it on now rather than entering recovery.
			 */
			data->power_on_skipped = false;
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_POWER_ON_PULSE);
			break;
		}

		modem_cellular_enter_recovery_state(data);
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
	data->suspend_pending = false;
	data->cmux_connect_failed = false;
	data->cmux_bus_opened = false;
	/* Drop any DLCI2 pipe from a previous session, it is reassigned once DLCI2 opens */
	data->cmd_pipe = NULL;
	modem_cellular_start_timer(data, K_MSEC(100));
	return 0;
}

static void modem_cellular_connect_cmux_power_off(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	modem_cmux_release(&data->cmux);
	if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
		/* The modem is likely still in CMUX mode, where the shutdown
		 * script would only time out, so go straight to the power key.
		 */
		modem_cellular_begin_power_off_pulse(data);
	} else if (data->cmux_bus_opened) {
		/* Without a power key the shutdown script over the UART is
		 * the only way left to power the modem down.
		 */
		data->cmd_pipe = data->uart_pipe;
		modem_cellular_begin_shutdown(data);
	} else {
		/* The UART failed to open, so there is no way to reach the modem */
		modem_cellular_begin_power_off_pulse(data);
	}
}

static void modem_cellular_connect_cmux_failed(struct modem_cellular_data *data)
{
	if (data->suspend_pending) {
		modem_cellular_connect_cmux_power_off(data);
		return;
	}
	/* No further connect event will arrive, so a suspend from here on (including
	 * the one queued when recovery attempts are exhausted) must not be deferred.
	 */
	data->cmux_connect_failed = true;
	modem_cellular_enter_recovery_state(data);
}

static void modem_cellular_connect_cmux_event_handler(struct modem_cellular_data *data,
						      enum modem_cellular_event evt)
{
	int ret;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
		ret = modem_pipe_open_async(data->uart_pipe);
		if (ret < 0) {
			LOG_ERR("Failed to open bus (%d)", ret);
			modem_cellular_connect_cmux_failed(data);
		}
		break;

	case MODEM_CELLULAR_EVENT_BUS_OPENED:
		data->cmux_bus_opened = true;
		modem_cmux_attach(&data->cmux, data->uart_pipe);
		modem_cmux_connect_async(&data->cmux);
		break;

	case MODEM_CELLULAR_EVENT_CMUX_CONNECTED:
		/* CMUX connected: the init/CMUX-connect fault that drives recovery has
		 * cleared, so reset the attempt count. A sustained connect/drop flap is
		 * not bounded by this alone; that would need a stability timer.
		 */
		data->recovery_count = 0;
		if (data->suspend_pending) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
			break;
		}
		modem_cellular_notify_user_pipes_connected(data);
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_OPEN_DLCI1);
		break;

	case MODEM_CELLULAR_EVENT_CMUX_DISCONNECTED:
		modem_cellular_connect_cmux_failed(data);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		if (data->cmux_connect_failed) {
			modem_cellular_connect_cmux_power_off(data);
			break;
		}
		/* The UART is closed until the CMUX connect starts, leaving no channel for
		 * the shutdown script. Power off once the connect attempt settles instead.
		 */
		data->suspend_pending = true;
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
	const struct modem_cellular_config *config = data->dev->config;
	const struct modem_chat_script *dlci_script = config->vendor->scripts.dlci_setup;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_DLCI1_OPENED:
		if (dlci_script) {
			modem_chat_attach(&data->chat, data->dlci1_pipe);
			modem_cellular_run_script(data, dlci_script);
		} else {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_OPEN_DLCI2);
		}
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_chat_release(&data->chat);
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		/* Failure is unlikely to be critical, continue on */
		modem_chat_release(&data->chat);
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_OPEN_DLCI2);
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

static const struct modem_chat_script *modem_cellular_board_init_script(const struct device *dev)
{
	STRUCT_SECTION_FOREACH(modem_cellular_board_init, board_init) {
		if (board_init->dev == dev) {
			return board_init->script;
		}
	}

	return NULL;
}

static void modem_cellular_enter_apn_state(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	if (config->use_default_pdp_context) {
		modem_cellular_enter_state_network_or_dial(data);
	} else if (!modem_cellular_needs_apn(data)) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_APN_SCRIPT);
	} else {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_WAIT_FOR_APN);
	}
}

static int modem_cellular_on_open_dlci2_state_enter(struct modem_cellular_data *data)
{
	modem_pipe_attach(data->dlci2_pipe, modem_cellular_dlci2_pipe_handler, data);
	return modem_pipe_open_async(data->dlci2_pipe);
}

static void modem_cellular_enter_board_init_or_apn_state(struct modem_cellular_data *data)
{
	if (modem_cellular_board_init_script(data->dev) != NULL) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_BOARD_INIT_SCRIPT);
	} else {
		modem_cellular_enter_apn_state(data);
	}
}

static void modem_cellular_open_dlci2_next(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	data->cmd_pipe = data->dlci2_pipe;

	if (modem_cellular_modem_selects_sim(config)) {
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_SIM_SELECT_SCRIPT);
	} else {
		modem_cellular_enter_board_init_or_apn_state(data);
	}
}

static void modem_cellular_open_dlci2_event_handler(struct modem_cellular_data *data,
						    enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;
	const struct modem_chat_script *dlci_script = config->vendor->scripts.dlci_setup;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_DLCI2_OPENED:
		if (dlci_script) {
			modem_chat_attach(&data->chat, data->dlci2_pipe);
			modem_cellular_run_script(data, dlci_script);
		} else {
			modem_cellular_open_dlci2_next(data);
		}
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_chat_release(&data->chat);
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		/* Failure is unlikely to be critical, continue on */
		modem_chat_release(&data->chat);
		modem_cellular_open_dlci2_next(data);
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

static void modem_cellular_wait_for_apn_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_APN_SET:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_APN_SCRIPT);
		break;

	case MODEM_CELLULAR_EVENT_SIM_SWITCH:
		modem_cellular_begin_sim_switch(data);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static void modem_cellular_script_failed(struct modem_cellular_data *data)
{
	data->script_failure_counter++;
	IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS, (data->stats.command_failures += 1));
}

static void modem_cellular_script_success(struct modem_cellular_data *data)
{
	data->script_failure_counter = 0;
}

static bool modem_cellular_is_script_retry_exceeded(struct modem_cellular_data *data)
{
	if (data->script_failure_counter >= CONFIG_MODEM_CELLULAR_MAX_SCRIPT_FAILURES) {
		data->script_failure_counter = 0;
		return true;
	}
	return false;
}

/**
 * @brief Handle a failure to select the active SIM slot on the modem.
 *
 * The modem then stays on the slot it had selected. When it reported that slot, and it is
 * one of the SIM slots, it becomes the active slot, so that the API reports the slot in use.
 * Otherwise, in a switch with the modem running, the slot the modem had before the switch
 * becomes the active slot again.
 *
 * @param data Modem instance data.
 */
static void modem_cellular_sim_select_failed(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	LOG_ERR("Failed to select SIM slot %u", data->sim_slot_active->index);

	for (uint8_t i = 0U; i < config->sim_slots_size; i++) {
		const struct modem_cellular_sim_slot *slot = &config->sim_slots[i];

		if (slot->index == data->sim_slot_modem) {
			k_mutex_lock(&data->api_lock, K_FOREVER);
			data->sim_slot_active = slot;
			k_mutex_unlock(&data->api_lock);
			LOG_WRN("Modem stays on SIM slot %u", slot->index);
			return;
		}
	}

	if (data->sim_switching) {
		modem_cellular_sim_switch_revert(data);
	}
}

/**
 * @brief Go on once the modem has selected the SIM slot, or failed to.
 *
 * In a switch with the modem running, goes on with the switch. Otherwise goes on with the
 * power-up.
 *
 * @param data Modem instance data.
 */
static void modem_cellular_sim_select_done(struct modem_cellular_data *data)
{
	if (data->sim_switching) {
		data->sim_switch_step = MODEM_CELLULAR_SIM_SWITCH_ACTIVATE;
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_SIM_SWITCH_SCRIPT);
	} else {
		modem_cellular_enter_board_init_or_apn_state(data);
	}
}

/**
 * @brief Send the command that selects the active SIM slot on the modem.
 *
 * Builds the command from the vendor format string and the index of the active slot, and
 * runs it as a script whose result is handled by the sim select script state. If the script
 * cannot run, the power-up goes on with the slot the modem has.
 *
 * @param data Modem instance data.
 */
static void modem_cellular_send_sim_select(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;
	int ret;

	data->sim_select_sent = true;

	ret = snprintk(data->sim_select_buf, sizeof(data->sim_select_buf),
		       config->vendor->sim_select.select_cmd, data->sim_slot_active->index);
	if ((ret < 0) || (ret >= (int)sizeof(data->sim_select_buf))) {
		LOG_ERR("SIM select command too long");
		modem_cellular_sim_select_failed(data);
		modem_cellular_sim_select_done(data);
		return;
	}

	modem_chat_script_chat_init(&data->sim_select_chat);
	modem_chat_script_chat_set_request(&data->sim_select_chat, data->sim_select_buf);
	modem_chat_script_chat_set_response_matches(&data->sim_select_chat, &ok_match, 1);
	modem_chat_script_set_script_chats(&data->sim_select_script, &data->sim_select_chat, 1);

	ret = modem_cellular_run_script(data, &data->sim_select_script);
	if (ret < 0) {
		LOG_ERR("Failed to run the SIM select script (%d)", ret);
		modem_cellular_sim_select_failed(data);
		modem_cellular_sim_select_done(data);
	}
}

static int modem_cellular_on_run_sim_select_script_state_enter(struct modem_cellular_data *data)
{
	modem_chat_attach(&data->chat, data->dlci1_pipe);

	data->sim_slot_modem = -1;
	data->sim_select_sent = false;

	/* Allow modem time to enter command mode before running the script */
	modem_cellular_start_timer(data, K_MSEC(200));
	return 0;
}

/**
 * @brief Handle the events of the state that selects the SIM slot on the modem.
 *
 * Reads the slot the modem has selected with the vendor query script, if any, and sends the
 * select command only when it differs from the active slot or is unknown. A failure to
 * select the slot does not stop the power-up: the modem then runs on the slot it had.
 *
 * @param data Modem instance data.
 * @param evt Event to handle.
 */
static void modem_cellular_run_sim_select_script_event_handler(struct modem_cellular_data *data,
							       enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if ((config->vendor->sim_select.query != NULL) &&
		    (modem_cellular_run_script(data, config->vendor->sim_select.query) == 0)) {
			break;
		}

		/* Without a query, the selected slot is unknown */
		modem_cellular_send_sim_select(data);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		if (data->sim_select_sent) {
			LOG_INF("Modem selected SIM slot %u", data->sim_slot_active->index);
			modem_cellular_sim_select_done(data);
		} else if (data->sim_slot_modem == data->sim_slot_active->index) {
			modem_cellular_sim_select_done(data);
		} else {
			modem_cellular_send_sim_select(data);
		}
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		if (!data->sim_select_sent) {
			/* The selected slot is unknown, select the active one anyway */
			modem_cellular_send_sim_select(data);
			break;
		}

		modem_cellular_sim_select_failed(data);
		modem_cellular_sim_select_done(data);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_run_sim_select_script_state_leave(struct modem_cellular_data *data)
{
	modem_cellular_stop_timer(data);
	return 0;
}

/* States in which the SIM slot can be switched with the modem running */
static bool modem_cellular_sim_switch_allowed(enum modem_cellular_state state)
{
	switch (state) {
	case MODEM_CELLULAR_STATE_WAIT_FOR_APN:
	case MODEM_CELLULAR_STATE_AWAIT_DIAL:
	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
	case MODEM_CELLULAR_STATE_REGISTERED:
		return true;
	default:
		return false;
	}
}

/**
 * @brief Start the switch to the requested SIM slot, with the modem running.
 *
 * The requested slot becomes the active one. The data call, if any, is up in the registered
 * state, and in the await registered state of a modem without a network script, which dials
 * before it registers.
 *
 * @param data Modem instance data.
 */
static void modem_cellular_begin_sim_switch(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;
	const struct modem_cellular_sim_slot *slot;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	slot = data->sim_slot_requested;
	data->sim_slot_requested = NULL;
	if ((slot != NULL) && (slot != data->sim_slot_active)) {
		data->sim_slot_previous = data->sim_slot_active;
		data->sim_slot_active = slot;
		memset(data->iccid, 0, sizeof(data->iccid));
		memset(data->imsi, 0, sizeof(data->imsi));
		data->sim_switching = true;
	}
	k_mutex_unlock(&data->api_lock);

	if (!data->sim_switching) {
		return;
	}

	data->sim_switch_ppp = (data->state == MODEM_CELLULAR_STATE_REGISTERED) ||
			       ((data->state == MODEM_CELLULAR_STATE_AWAIT_REGISTERED) &&
				!modem_cellular_has_network_script(config));
	data->sim_switch_step = MODEM_CELLULAR_SIM_SWITCH_DETACH;
	data->sim_activate_attempts = 0U;

	/* The new SIM has to register; the registration and serving cell are the old SIM's */
	modem_cellular_clear_registration_status(data);
	modem_cellular_invalidate_network_status(data);

	LOG_INF("Switching to SIM slot %u", slot->index);
	modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_SIM_SWITCH_SCRIPT);
}

/**
 * @brief Restore the active SIM slot of before the switch in progress.
 *
 * @param data Modem instance data.
 */
static void modem_cellular_sim_switch_revert(struct modem_cellular_data *data)
{
	k_mutex_lock(&data->api_lock, K_FOREVER);
	data->sim_slot_active = data->sim_slot_previous;
	k_mutex_unlock(&data->api_lock);

	LOG_WRN("Modem stays on SIM slot %u", data->sim_slot_active->index);
}

/**
 * @brief End the switch in progress, and connect again.
 *
 * @param data Modem instance data.
 */
static void modem_cellular_sim_switch_finish(struct modem_cellular_data *data)
{
	data->sim_switching = false;
	LOG_INF("SIM slot %u active", data->sim_slot_active->index);
	modem_cellular_enter_apn_state(data);
}

static void modem_cellular_sim_switch_result(struct modem_cellular_data *data, bool success);

/**
 * @brief Run the current step of the switch in progress.
 *
 * The detach, deactivate and activate steps run a chat script, whose result goes to
 * modem_cellular_sim_switch_result(). The select step applies the MUX state of the slot at
 * once, or has the modem select its SIM interface in the sim select script state.
 *
 * @param data Modem instance data.
 */
static void modem_cellular_sim_switch_run_step(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;
	const struct modem_cellular_sim_slot *slot = data->sim_slot_active;
	const struct modem_chat_script *script;
	int ret;

	switch (data->sim_switch_step) {
	case MODEM_CELLULAR_SIM_SWITCH_DETACH:
		script = &detach_chat_script;
		break;

	case MODEM_CELLULAR_SIM_SWITCH_DEACTIVATE:
		script = config->vendor->scripts.sim_deactivate;
		break;

	case MODEM_CELLULAR_SIM_SWITCH_SELECT:
		if (slot->mux == NULL) {
			modem_cellular_enter_state(data,
						   MODEM_CELLULAR_STATE_RUN_SIM_SELECT_SCRIPT);
			return;
		}

		ret = mux_state_apply(slot->mux, slot->state);
		if (ret < 0) {
			LOG_ERR("Failed to select SIM slot %u (%d)", slot->index, ret);
			modem_cellular_sim_switch_revert(data);

			if (modem_cellular_apply_sim_slot(data) < 0) {
				/* The SIM lines are routed nowhere known: power the modem off.
				 * Its resume routes them to the active slot again.
				 */
				data->sim_switching = false;
				modem_chat_release(&data->chat);
				modem_ppp_release(config->ppp);
				modem_cellular_enter_state(data,
							   MODEM_CELLULAR_STATE_INIT_POWER_OFF);
				return;
			}
		}

		data->sim_switch_step = MODEM_CELLULAR_SIM_SWITCH_ACTIVATE;
		modem_cellular_sim_switch_run_step(data);
		return;

	default:
		script = config->vendor->scripts.sim_activate;
		if (script == NULL) {
			modem_cellular_sim_switch_finish(data);
			return;
		}
		break;
	}

	ret = modem_cellular_run_script(data, script);
	if (ret == -EBUSY) {
		/* The chat still runs a script of the previous state */
		modem_cellular_start_timer(data, K_MSEC(200));
	} else if (ret < 0) {
		LOG_ERR("Failed to run the SIM switch script (%d)", ret);
		modem_cellular_sim_switch_result(data, false);
	}
}

/**
 * @brief Handle the result of a step of the switch in progress, and go on.
 *
 * A failed detach does not stop the switch. A failed deactivation does, as the SIM
 * interface may still be active: the previous slot is kept and activated again. A failed
 * activation is left to the network and dial scripts.
 *
 * @param data Modem instance data.
 * @param success Whether the step succeeded.
 */
static void modem_cellular_sim_switch_result(struct modem_cellular_data *data, bool success)
{
	switch (data->sim_switch_step) {
	case MODEM_CELLULAR_SIM_SWITCH_DETACH:
		if (!success) {
			LOG_WRN("Failed to detach from the packet domain");
		}

		data->sim_switch_step = MODEM_CELLULAR_SIM_SWITCH_DEACTIVATE;
		if (data->sim_switch_ppp) {
			/* The detach ends the data call: release PPP before going on */
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD);
			return;
		}
		break;

	case MODEM_CELLULAR_SIM_SWITCH_DEACTIVATE:
		if (success) {
			data->sim_switch_step = MODEM_CELLULAR_SIM_SWITCH_SELECT;
		} else {
			LOG_ERR("Failed to deactivate the SIM interface");
			modem_cellular_sim_switch_revert(data);
			data->sim_switch_step = MODEM_CELLULAR_SIM_SWITCH_ACTIVATE;
		}
		break;

	default:
		if (!success) {
			data->sim_activate_attempts++;
			if (data->sim_activate_attempts < MODEM_CELLULAR_SIM_ACTIVATE_ATTEMPTS) {
				/* The SIM interface may still initialize: run the script again */
				modem_cellular_start_timer(data,
							   MODEM_CELLULAR_SIM_ACTIVATE_RETRY_DELAY);
				return;
			}

			LOG_WRN("Failed to activate the SIM interface");
		}

		modem_cellular_sim_switch_finish(data);
		return;
	}

	modem_cellular_sim_switch_run_step(data);
}

static int modem_cellular_on_run_sim_switch_script_state_enter(struct modem_cellular_data *data)
{
	modem_chat_attach(&data->chat, data->dlci1_pipe);

	/* Allow modem time to enter command mode before running the script */
	modem_cellular_start_timer(data, K_MSEC(200));
	return 0;
}

static void modem_cellular_run_sim_switch_script_event_handler(struct modem_cellular_data *data,
							       enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_cellular_sim_switch_run_step(data);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_sim_switch_result(data, true);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_sim_switch_result(data, false);
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_chat_release(&data->chat);
		modem_ppp_release(config->ppp);
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_run_sim_switch_script_state_leave(struct modem_cellular_data *data)
{
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_run_board_init_script_state_enter(struct modem_cellular_data *data)
{
	modem_chat_attach(&data->chat, data->dlci1_pipe);

	/* The registered script is const; copy it to attach the driver's
	 * completion callback.
	 */
	data->board_init_script = *modem_cellular_board_init_script(data->dev);
	modem_chat_script_set_callback(&data->board_init_script,
				       modem_cellular_chat_callback_handler);

	/* Allow modem time to enter command mode before running the board script */
	modem_cellular_start_timer(data, K_MSEC(200));
	return 0;
}

static void modem_cellular_run_board_init_script_event_handler(struct modem_cellular_data *data,
							       enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_cellular_run_script(data, &data->board_init_script);
		break;
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_script_success(data);
		modem_cellular_enter_apn_state(data);
		break;
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_script_failed(data);
		if (modem_cellular_is_script_retry_exceeded(data)) {
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SUSPEND);
		} else {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;
	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_run_apn_script_state_enter(struct modem_cellular_data *data)
{
	/* Allow modem time to enter command mode before running apn script */
	modem_cellular_start_timer(data, K_MSEC(200));
	modem_cellular_build_apn_script(data);
	return 0;
}

static void modem_cellular_run_apn_script_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_chat_attach(&data->chat, data->dlci1_pipe);
		modem_cellular_run_script(data, &data->apn_script);
		break;
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_script_success(data);
		modem_cellular_enter_state_network_or_dial(data);
		break;
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_script_failed(data);
		if (modem_cellular_is_script_retry_exceeded(data)) {
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SUSPEND);
		} else {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;
	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;
	case MODEM_CELLULAR_EVENT_RING:
		modem_pipe_open_async(data->uart_pipe);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_run_network_script_state_enter(struct modem_cellular_data *data)
{
	modem_chat_attach(&data->chat, data->dlci1_pipe);
	modem_cellular_start_timer(data, K_NO_WAIT);
	return 0;
}

static void modem_cellular_run_network_script_event_handler(struct modem_cellular_data *data,
							    enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config =
		(const struct modem_cellular_config *)data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		modem_cellular_run_script(data, config->vendor->scripts.network);
		break;
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_script_failed(data);
		if (modem_cellular_is_script_retry_exceeded(data)) {
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SUSPEND);
		} else {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_script_success(data);
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_REGISTERED);
		break;
	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;
	case MODEM_CELLULAR_EVENT_RING:
		modem_pipe_open_async(data->uart_pipe);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_await_dial_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	modem_chat_attach(&data->chat, data->dlci1_pipe);

	if (net_if_is_admin_up(modem_ppp_get_iface(config->ppp))) {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_DIAL);
		return 0;
	}

	/* Parked waiting for a consumer to admit the PPP interface. Keep the
	 * periodic script running (as in the registered states) so signal and the
	 * serving/neighbour caches stay fresh for an app deciding whether to dial.
	 */
	modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
	return 0;
}

static void modem_cellular_await_dial_event_handler(struct modem_cellular_data *data,
						    enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;
	int ret;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_DIAL:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_script_success(data);
		modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_script_failed(data);
		if (modem_cellular_is_script_retry_exceeded(data)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_RESUME);
		} else {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;

	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (config->vendor->scripts.periodic == NULL) {
			break;
		}
		if (atomic_get(&data->periodic_paused)) {
			data->periodic_timeout_skipped = true;
			break;
		}
		ret = modem_cellular_run_script(data, config->vendor->scripts.periodic);
		if (ret < 0) {
			LOG_WRN("periodic %s %s, rearming timer", "timer",
				ret == -EBUSY ? "busy" : "failed");
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;

	case MODEM_CELLULAR_EVENT_PERIODIC_KICK:
		if (atomic_get(&data->periodic_paused)) {
			break;
		}
		if (!data->periodic_timeout_skipped) {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
			break;
		}
		data->periodic_timeout_skipped = false;
		if (modem_cellular_run_script(data, config->vendor->scripts.periodic) < 0) {
			LOG_WRN("periodic kick busy, rearming timer");
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;

	case MODEM_CELLULAR_EVENT_SIM_SWITCH:
		modem_cellular_begin_sim_switch(data);
		break;
	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;
	case MODEM_CELLULAR_EVENT_RING:
		modem_pipe_open_async(data->uart_pipe);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_await_dial_state_leave(struct modem_cellular_data *data)
{
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_run_dial_script_state_enter(struct modem_cellular_data *data)
{
	modem_chat_attach(&data->chat, data->dlci2_pipe);
	modem_cellular_start_timer(data, K_NO_WAIT);
	return 0;
}

static void modem_cellular_run_dial_script_event_handler(struct modem_cellular_data *data,
							 enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;
	int ret;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		ret = modem_cellular_run_script(data, config->vendor->scripts.dial);
		if (ret < 0) {
			LOG_WRN("dial script %s, rearming timer",
				ret == -EBUSY ? "busy" : "failed");
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_script_failed(data);
		if (modem_cellular_is_script_retry_exceeded(data)) {
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SUSPEND);
		} else {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_script_success(data);
		modem_chat_release(&data->chat);
		modem_chat_attach(&data->chat, data->dlci1_pipe);

		/* PHY is now up and searching for network */
		net_if_carrier_on(modem_ppp_get_iface(config->ppp));

		if (modem_ppp_attach(config->ppp, data->dlci2_pipe) < 0) {
			LOG_ERR("Failed to attach PPP to DLCI2");
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SUSPEND);
			break;
		}
		/* Check if we are already registered during the network setup */
		if (modem_cellular_has_network_script(config)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_REGISTERED);
		} else {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_REGISTERED);
		}
		break;

	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;
	case MODEM_CELLULAR_EVENT_RING:
		LOG_DBG("RING received!");
		modem_pipe_open_async(data->uart_pipe);
		break;
	case MODEM_CELLULAR_EVENT_REGISTERED:
		/* Restart immediately, if we are waiting to retry the dial-script */
		if (!modem_chat_is_running(&data->chat)) {
			modem_cellular_stop_timer(data);
			modem_cellular_run_script(data, config->vendor->scripts.dial);
		}
		break;
	case MODEM_CELLULAR_EVENT_HANGUP:
		modem_chat_release(&data->chat);
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_DIAL);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_run_dial_script_state_leave(struct modem_cellular_data *data)
{
	return modem_chat_attach(&data->chat, data->dlci1_pipe);
}

static int modem_cellular_on_await_registered_state_enter(struct modem_cellular_data *data)
{
	modem_chat_attach(&data->chat, data->dlci1_pipe);
	modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
	if (modem_cellular_is_attached(data)) {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_REGISTERED);
	}
	return 0;
}

static void modem_cellular_await_registered_event_handler(struct modem_cellular_data *data,
							  enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;
	int ret;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_script_success(data);
		modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_script_failed(data);
		if (modem_cellular_is_script_retry_exceeded(data)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_RESUME);
		} else {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;

	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (config->vendor->scripts.periodic == NULL) {
			break;
		}
		if (atomic_get(&data->periodic_paused)) {
			data->periodic_timeout_skipped = true;
			break;
		}
		ret = modem_cellular_run_script(data, config->vendor->scripts.periodic);
		if (ret < 0) {
			LOG_WRN("periodic %s %s, rearming timer", "timer",
				ret == -EBUSY ? "busy" : "failed");
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;

	case MODEM_CELLULAR_EVENT_PERIODIC_KICK:
		if (atomic_get(&data->periodic_paused)) {
			break;
		}
		if (!data->periodic_timeout_skipped) {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
			break;
		}
		data->periodic_timeout_skipped = false;
		if (modem_cellular_run_script(data, config->vendor->scripts.periodic) < 0) {
			LOG_WRN("periodic kick busy, rearming timer");
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;

	case MODEM_CELLULAR_EVENT_REGISTERED:
		if (modem_cellular_has_network_script(config)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT);
		} else {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_REGISTERED);
		}
		break;

	case MODEM_CELLULAR_EVENT_SIM_SWITCH:
		modem_cellular_begin_sim_switch(data);
		break;
	case MODEM_CELLULAR_EVENT_PPP_DEAD:
		if (net_if_is_admin_up(modem_ppp_get_iface(config->ppp))) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD);
		}
		break;
	case MODEM_CELLULAR_EVENT_HANGUP:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD);
		break;
	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;
	case MODEM_CELLULAR_EVENT_RING:
		LOG_DBG("RING received!");
		modem_pipe_open_async(data->uart_pipe);
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

static int modem_cellular_on_registered_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	net_if_dormant_off(modem_ppp_get_iface(config->ppp));
	modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
	return 0;
}

static void modem_cellular_registered_event_handler(struct modem_cellular_data *data,
						    enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;
	struct cellular_evt_modem_comms_check_result result;
	int ret;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_cellular_script_success(data);
		modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		result.success = true;
		modem_cellular_emit_event(data, CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT, &result);
		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		modem_cellular_script_failed(data);
		if (modem_cellular_is_script_retry_exceeded(data)) {
			net_if_carrier_off(modem_ppp_get_iface(config->ppp));
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
			modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_RESUME);
			LOG_WRN("Maximum script failures reached, restarting modem");
		} else {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		result.success = false;
		modem_cellular_emit_event(data, CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT, &result);
		break;

	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (config->vendor->scripts.periodic == NULL) {
			break;
		}
		if (atomic_get(&data->periodic_paused)) {
			data->periodic_timeout_skipped = true;
			break;
		}
		ret = modem_cellular_run_script(data, config->vendor->scripts.periodic);
		if (ret < 0) {
			LOG_WRN("periodic %s %s, rearming timer", "timer",
				ret == -EBUSY ? "busy" : "failed");
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;

	case MODEM_CELLULAR_EVENT_PERIODIC_KICK:
		if (atomic_get(&data->periodic_paused)) {
			break;
		}
		if (!data->periodic_timeout_skipped) {
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
			break;
		}
		data->periodic_timeout_skipped = false;
		ret = modem_cellular_run_script(data, config->vendor->scripts.periodic);
		if (ret < 0) {
			LOG_WRN("periodic %s %s, rearming timer", "kick",
				ret == -EBUSY ? "busy" : "failed");
			modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
		}
		break;

	case MODEM_CELLULAR_EVENT_DEREGISTERED:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD);
		break;
	case MODEM_CELLULAR_EVENT_PPP_DEAD:
		if (net_if_is_admin_up(modem_ppp_get_iface(config->ppp))) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD);
		}
		break;
	case MODEM_CELLULAR_EVENT_HANGUP:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD);
		break;
	case MODEM_CELLULAR_EVENT_SIM_SWITCH:
		modem_cellular_begin_sim_switch(data);
		break;
	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_chat_release(&data->chat);
		modem_ppp_release(config->ppp);
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;
	case MODEM_CELLULAR_EVENT_RING:
		LOG_DBG("RING received!");
		modem_pipe_open_async(data->uart_pipe);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_registered_state_leave(struct modem_cellular_data *data)
{
	modem_cellular_stop_timer(data);

	return 0;
}

static int modem_cellular_on_await_ppp_dead_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	net_if_dormant_on(modem_ppp_get_iface(config->ppp));
	IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS, (data->stats.link_drops += 1));

	/* PPP is not guaranteed to report the link dead, so do not wait on it forever */
	modem_cellular_start_timer(data, MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT);
	return 0;
}

static void modem_cellular_await_ppp_dead_event_handler(struct modem_cellular_data *data,
						 enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_RING:
		LOG_DBG("RING received!");
		modem_pipe_open_async(data->uart_pipe);
		break;
	case MODEM_CELLULAR_EVENT_PPP_DEAD:
		/* Wait for the channel to return to AT mode after PPP termination */
		modem_cellular_update_timer(data, K_MSEC(config->vendor->reset_pulse_duration_ms));
		break;
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (data->sim_switching) {
			modem_cellular_enter_state(data,
						   MODEM_CELLULAR_STATE_RUN_SIM_SWITCH_SCRIPT);
		} else if (modem_cellular_has_network_script(config) &&
			   !modem_cellular_is_attached(data)) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_REGISTERED);
		} else if (IS_ENABLED(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT) &&
			   !net_if_is_admin_up(modem_ppp_get_iface(config->ppp))) {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_AWAIT_DIAL);
		} else {
			modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT);
		}
		break;
	case MODEM_CELLULAR_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_INIT_POWER_OFF);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_await_ppp_dead_state_leave(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	modem_cellular_stop_timer(data);
	modem_chat_release(&data->chat);
	modem_ppp_release(config->ppp);

	return 0;
}

static int modem_cellular_on_init_power_off_state_enter(struct modem_cellular_data *data)
{
	modem_cmux_disconnect_async(&data->cmux);
	modem_cellular_start_timer(data, K_MSEC(2000));
	return 0;
}

static void modem_cellular_cmux_cleanup(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	modem_cellular_notify_user_pipes_disconnected(data);
	modem_chat_release(&data->chat);
	modem_ppp_release(config->ppp);
}

static void modem_cellular_init_power_off_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	const struct modem_cellular_config *config = data->dev->config;
	uint16_t disconnect_timeout_ms;

	switch (evt) {
	case MODEM_CELLULAR_EVENT_CMUX_DISCONNECTED:
		modem_cellular_stop_timer(data);
		data->cmd_pipe = data->uart_pipe;
		/* CMUX disconnected, notify users */
		modem_cellular_cmux_cleanup(data);
		/* Switch the chat instance back to the UART pipe to handle unsolicited events */
		modem_chat_attach(&data->chat, data->cmd_pipe);
		/* Fallback to `reset_pulse_duration_ms` if `cmux_disconnect_timeout_ms` not
		 * specified as this was the behaviour before Zephyr v4.5.
		 */
		disconnect_timeout_ms = config->vendor->cmux_disconnect_timeout_ms > 0U
						? config->vendor->cmux_disconnect_timeout_ms
						: config->vendor->reset_pulse_duration_ms;
		/* Unless signalled, wait for the modem to return from CMUX to AT mode */
		modem_cellular_start_timer(data, K_MSEC(disconnect_timeout_ms));
		break;
	case MODEM_CELLULAR_EVENT_MODEM_READY:
		/* Modem driver indicated it is ready to handle commands after disconnecting CMUX.
		 * Cancel the timer and proceed as if it has expired.
		 */
		modem_cellular_stop_timer(data);
		__fallthrough;
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		if (data->cmd_pipe != data->uart_pipe) {
			/* No CMUX_DISCONNECTED event occurred, notify users here */
			modem_cellular_cmux_cleanup(data);
		} else {
			/* Release the chat instance attached in CMUX_DISCONNECTED */
			modem_chat_release(&data->chat);
		}
		modem_cellular_begin_shutdown(data);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_run_shutdown_script_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	modem_chat_attach(&data->chat, data->cmd_pipe);
	return modem_cellular_run_script(data, config->vendor->scripts.shutdown);
}

static void modem_cellular_run_shutdown_script_event_handler(struct modem_cellular_data *data,
							     enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_SCRIPT_FAILED:
		data->cmd_pipe = NULL;

		/* If shutdown by software failed, try by power pulse if possible */
		modem_cellular_begin_power_off_pulse(data);

		break;

	case MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS:
		modem_pipe_close_async(data->uart_pipe);
		data->cmd_pipe = NULL;
		data->powered_off = true;
		modem_cellular_enter_state(data, MODEM_CELLULAR_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_run_shutdown_script_state_leave(struct modem_cellular_data *data)
{
	modem_chat_release(&data->chat);
	return 0;
}

static int modem_cellular_on_power_off_pulse_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	data->cmd_pipe = NULL;
	gpio_pin_set_dt(&config->power_gpio, 1);
	modem_cellular_start_timer(data, K_MSEC(config->vendor->power_pulse_duration_ms));
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
	const struct modem_cellular_config *config = data->dev->config;

	gpio_pin_set_dt(&config->power_gpio, 0);
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_await_power_off_state_enter(struct modem_cellular_data *data)
{
	const struct modem_cellular_config *config = data->dev->config;

	modem_cellular_start_timer(data, K_MSEC(config->vendor->shutdown_time_ms));
	return 0;
}

static void modem_cellular_await_power_off_event_handler(struct modem_cellular_data *data,
							enum modem_cellular_event evt)
{
	switch (evt) {
	case MODEM_CELLULAR_EVENT_TIMEOUT:
		data->powered_off = true;
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

	case MODEM_CELLULAR_STATE_AWAIT_RESET:
		ret = modem_cellular_on_await_reset_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RECOVERY:
		ret = modem_cellular_on_recovery_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_POWER_ON_PULSE:
		ret = modem_cellular_on_power_on_pulse_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_POWER_ON:
		ret = modem_cellular_on_await_power_on_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_SET_BAUDRATE:
		ret = modem_cellular_on_set_baudrate_state_enter(data);
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

	case MODEM_CELLULAR_STATE_RUN_SIM_SELECT_SCRIPT:
		ret = modem_cellular_on_run_sim_select_script_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_SIM_SWITCH_SCRIPT:
		ret = modem_cellular_on_run_sim_switch_script_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_BOARD_INIT_SCRIPT:
		ret = modem_cellular_on_run_board_init_script_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_APN_SCRIPT:
		ret = modem_cellular_on_run_apn_script_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_NETWORK_SCRIPT:
		ret = modem_cellular_on_run_network_script_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_DIAL:
		ret = modem_cellular_on_await_dial_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT:
		ret = modem_cellular_on_run_dial_script_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
		ret = modem_cellular_on_await_registered_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_REGISTERED:
		ret = modem_cellular_on_registered_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD:
		ret = modem_cellular_on_await_ppp_dead_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_INIT_POWER_OFF:
		ret = modem_cellular_on_init_power_off_state_enter(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_SHUTDOWN_SCRIPT:
		ret = modem_cellular_on_run_shutdown_script_state_enter(data);
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

	case MODEM_CELLULAR_STATE_AWAIT_RESET:
		ret = modem_cellular_on_await_reset_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_RECOVERY:
		ret = modem_cellular_on_recovery_state_leave(data);
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

	case MODEM_CELLULAR_STATE_RUN_SIM_SELECT_SCRIPT:
		ret = modem_cellular_on_run_sim_select_script_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_SIM_SWITCH_SCRIPT:
		ret = modem_cellular_on_run_sim_switch_script_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_DIAL:
		ret = modem_cellular_on_await_dial_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT:
		ret = modem_cellular_on_run_dial_script_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
		ret = modem_cellular_on_await_registered_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_REGISTERED:
		ret = modem_cellular_on_registered_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD:
		ret = modem_cellular_on_await_ppp_dead_state_leave(data);
		break;

	case MODEM_CELLULAR_STATE_RUN_SHUTDOWN_SCRIPT:
		ret = modem_cellular_on_run_shutdown_script_state_leave(data);
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
	data->script_running = NULL;
	ret = modem_cellular_on_state_enter(data);

	if (ret < 0) {
		LOG_WRN("failed to enter state error: %i", ret);
	}

	/* Start a switch requested while the modem could not start it */
	if ((data->sim_slot_requested != NULL) && modem_cellular_sim_switch_allowed(state)) {
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SIM_SWITCH);
	}
}

static void modem_cellular_event_handler(struct modem_cellular_data *data,
					 enum modem_cellular_event evt)
{
	enum modem_cellular_state state;

	state = data->state;

	modem_cellular_log_event(evt);

	if ((evt == MODEM_CELLULAR_EVENT_SCRIPT_SUCCESS ||
	     evt == MODEM_CELLULAR_EVENT_SCRIPT_FAILED) &&
	    data->event_ptr != data->script_running) {
		LOG_DBG("script result dropped in %s", modem_cellular_state_str(data->state));
		return;
	}

	switch (data->state) {
	case MODEM_CELLULAR_STATE_IDLE:
		modem_cellular_idle_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RESET_PULSE:
		modem_cellular_reset_pulse_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_RESET:
		modem_cellular_await_reset_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RECOVERY:
		modem_cellular_recovery_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_POWER_ON_PULSE:
		modem_cellular_power_on_pulse_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_POWER_ON:
		modem_cellular_await_power_on_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_SET_BAUDRATE:
		modem_cellular_set_baudrate_event_handler(data, evt);
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

	case MODEM_CELLULAR_STATE_RUN_SIM_SELECT_SCRIPT:
		modem_cellular_run_sim_select_script_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RUN_SIM_SWITCH_SCRIPT:
		modem_cellular_run_sim_switch_script_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RUN_BOARD_INIT_SCRIPT:
		modem_cellular_run_board_init_script_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_WAIT_FOR_APN:
		modem_cellular_wait_for_apn_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RUN_APN_SCRIPT:
		modem_cellular_run_apn_script_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RUN_NETWORK_SCRIPT:
		modem_cellular_run_network_script_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_DIAL:
		modem_cellular_await_dial_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT:
		modem_cellular_run_dial_script_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_REGISTERED:
		modem_cellular_await_registered_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_REGISTERED:
		modem_cellular_registered_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_AWAIT_PPP_DEAD:
		modem_cellular_await_ppp_dead_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_INIT_POWER_OFF:
		modem_cellular_init_power_off_event_handler(data, evt);
		break;

	case MODEM_CELLULAR_STATE_RUN_SHUTDOWN_SCRIPT:
		modem_cellular_run_shutdown_script_event_handler(data, evt);
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
	case MODEM_CMUX_EVENT_DISCONNECTED:
		IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS, (data->stats.cmux_disconnects += 1));
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_CMUX_DISCONNECTED);
		break;
	default:
		break;
	}
}

MODEM_CHAT_SCRIPT_CMDS_DEFINE(get_signal_csq_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CSQ", csq_match));

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
			      MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CESQ", cesq_match));

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
	    (data->state != MODEM_CELLULAR_STATE_REGISTERED)) {
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
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;
	const char *info_str;

	if (size <= 1) {
		return -EINVAL;
	}

	switch (type) {
	case CELLULAR_MODEM_INFO_IMEI:
		info_str = &data->imei[0];
		break;
	case CELLULAR_MODEM_INFO_SIM_IMSI:
		info_str = &data->imsi[0];
		break;
	case CELLULAR_MODEM_INFO_MANUFACTURER:
		info_str = &data->manufacturer[0];
		break;
	case CELLULAR_MODEM_INFO_FW_VERSION:
		info_str = &data->fw_version[0];
		break;
	case CELLULAR_MODEM_INFO_MODEL_ID:
		info_str = &data->model_id[0];
		break;
	case CELLULAR_MODEM_INFO_SIM_ICCID:
		info_str = &data->iccid[0];
		break;
	case CELLULAR_MODEM_INFO_SERIAL_NUMBER:
		info_str = &data->sn[0];
		break;
	default:
		return -ENODATA;
	}

	/* All internal copies of modem info are NUL terminated.
	 * Copy at most `size - 1` bytes of the info to the output.
	 * Manually NUL terminate the output.
	 */
	strncpy(info, info_str, size - 1);
	info[size - 1] = '\0';
	return 0;
}
static int modem_cellular_get_registration_status(const struct device *dev,
						  enum cellular_access_technology tech,
						  enum cellular_registration_status *status)
{
	int ret = 0;
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;

	switch (tech) {
	case CELLULAR_ACCESS_TECHNOLOGY_GSM:
	case CELLULAR_ACCESS_TECHNOLOGY_GSM_COMPACT:
		*status = data->registration_status_gsm;
		break;
	case CELLULAR_ACCESS_TECHNOLOGY_GSM_EGPRS:
	case CELLULAR_ACCESS_TECHNOLOGY_EC_GSM_IOT:
	case CELLULAR_ACCESS_TECHNOLOGY_UTRAN:
	case CELLULAR_ACCESS_TECHNOLOGY_UTRAN_HSDPA:
	case CELLULAR_ACCESS_TECHNOLOGY_UTRAN_HSUPA:
	case CELLULAR_ACCESS_TECHNOLOGY_UTRAN_HSDPA_HSUPA:
		*status = data->registration_status_gprs;
		break;
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN:
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1:
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRA_NR_DUAL:
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1_SAT:
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_WB_S1_SAT:
	case CELLULAR_ACCESS_TECHNOLOGY_NG_RAN_SAT:
		*status = data->registration_status_lte;
		break;
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRA_5G_CN:
	case CELLULAR_ACCESS_TECHNOLOGY_NR_5G_CN:
	case CELLULAR_ACCESS_TECHNOLOGY_NG_RAN:
		*status = data->registration_status_5g;
		break;
	default:
		ret = -ENODATA;
		break;
	}

	return ret;
}

static int modem_cellular_get_network_status(const struct device *dev,
					     struct cellular_evt_network_status *status)
{
	struct modem_cellular_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (data->network_status_valid) {
		*status = data->network_status;
	} else {
		ret = -ENODATA;
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int modem_cellular_set_apn(const struct device *dev, const char *apn)
{
	struct modem_cellular_data *data = dev->data;
	int ret = 0;

	if ((apn == NULL) || (*apn == '\0')) {
		return -EINVAL;
	}

	if (strlen(apn) >= MODEM_CELLULAR_DATA_APN_LEN) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (strcmp(apn, data->apn) == 0) {
		ret = -EALREADY;
		goto out;
	}

	/* The new SIM may need another APN, which is configured once the slot is switched */
	if (!modem_cellular_apn_change_allowed(data->state) && (data->sim_slot_requested == NULL) &&
	    !data->sim_switching) {
		ret = -EBUSY;
		goto out;
	}

	strncpy(data->apn, apn, MODEM_CELLULAR_DATA_APN_LEN - 1);
	data->apn[MODEM_CELLULAR_DATA_APN_LEN - 1] = '\0';

	/* Wake the state-machine if it is parked in WAIT_FOR_APN */
	modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_APN_SET);

out:
	k_mutex_unlock(&data->api_lock);
	return ret;
}

static int modem_cellular_set_callback(const struct device *dev, cellular_event_mask_t mask,
				       cellular_event_cb_t cb, void *user_data)
{
	struct modem_cellular_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (cb == NULL) {
		data->cb.fn = NULL;
		data->cb.mask = 0;
		data->cb.user_data = NULL;
	} else {
		data->cb.fn = cb;
		data->cb.mask = mask;
		data->cb.user_data = user_data;
	}

	k_mutex_unlock(&data->api_lock);
	return ret;
}

/**
 * @brief Get the number of SIM slots and the active slot.
 *
 * A modem without a sim-slots node reports one slot, with index 0.
 *
 * @param dev Modem device.
 * @param count Number of SIM slots.
 * @param active Index of the active SIM slot.
 *
 * @retval 0 Success.
 */
static int modem_cellular_get_sim_slots(const struct device *dev, uint8_t *count, uint8_t *active)
{
	const struct modem_cellular_config *config = dev->config;
	struct modem_cellular_data *data = dev->data;

	if (config->sim_slots_size == 0U) {
		*count = 1U;
		*active = 0U;
		return 0;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);
	*count = config->sim_slots_size;
	*active = data->sim_slot_active->index;
	k_mutex_unlock(&data->api_lock);

	return 0;
}

/**
 * @brief Select the active SIM slot.
 *
 * While the modem is in the idle state, a slot with a MUX is routed at once by applying its
 * MUX state, which is only accepted while the driver holds the modem powered off or in
 * reset; if the MUX fails to apply the state, the MUX state of the active slot is applied
 * again. A slot that the modem selects is selected by the modem when it powers up. Once the
 * modem runs, the switch is only accepted if the modem driver provides a script that
 * deactivates the SIM interface. It is then requested, and the state machine detaches from
 * the packet domain, ends the data call, switches the slot and connects again. Clears the
 * ICCID and IMSI of the previous slot.
 *
 * @param dev Modem device.
 * @param slot Index of the SIM slot, its reg property.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP Modem has no sim-slots node.
 * @retval -EINVAL No enabled SIM slot has this index.
 * @retval -EALREADY Slot is already the active one, or the one requested.
 * @retval -EBUSY Modem runs, and its driver cannot switch the SIM slot while it runs, or it
 *         is idle, but the driver did not power it off.
 * @retval -EAGAIN The event queue of the driver is full, the switch is not requested.
 * @retval -errno Error from mux_state_apply().
 */
static int modem_cellular_select_sim(const struct device *dev, uint8_t slot)
{
	const struct modem_cellular_config *config = dev->config;
	struct modem_cellular_data *data = dev->data;
	const struct modem_cellular_sim_slot *sim_slot = NULL;
	int ret;

	if (config->sim_slots_size == 0U) {
		return -ENOTSUP;
	}

	for (uint8_t i = 0U; i < config->sim_slots_size; i++) {
		if (config->sim_slots[i].index == slot) {
			sim_slot = &config->sim_slots[i];
			break;
		}
	}

	if (sim_slot == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (sim_slot == ((data->sim_slot_requested != NULL) ? data->sim_slot_requested
							    : data->sim_slot_active)) {
		ret = -EALREADY;
		goto out;
	}

	/* The SIM lines must only be switched while the SIM interface is inactive. Once the
	 * modem runs, only a modem driver that can deactivate the SIM interface can switch
	 * them, which the state machine does as soon as the modem is in a state that allows
	 * it.
	 */
	if (data->state != MODEM_CELLULAR_STATE_IDLE) {
		const struct modem_cellular_sim_slot *requested = data->sim_slot_requested;

		if (config->vendor->scripts.sim_deactivate == NULL) {
			ret = -EBUSY;
			goto out;
		}

		data->sim_slot_requested = sim_slot;
		if (modem_cellular_queue_event(data, MODEM_CELLULAR_EVENT_SIM_SWITCH, NULL) < 0) {
			/* Without its event, the switch would not start until a state change */
			data->sim_slot_requested = requested;
			ret = -EAGAIN;
			goto out;
		}

		LOG_INF("Switch to SIM slot %u requested", slot);
		ret = 0;
		goto out;
	}

	/* In the idle state, a MUX must only switch the SIM lines while the driver holds the
	 * modem powered off. The idle state alone does not guarantee it: a suspend during the
	 * power-up, or a failed shutdown without a power GPIO, leaves the modem powered.
	 */
	if ((sim_slot->mux != NULL) && !data->powered_off) {
		ret = -EBUSY;
		goto out;
	}

	/* A slot without a MUX is selected by the modem when it powers up */
	ret = 0;
	if (sim_slot->mux != NULL) {
		ret = mux_state_apply(sim_slot->mux, sim_slot->state);
		if (ret < 0) {
			LOG_ERR("Failed to select SIM slot %u (%d)", slot, ret);

			/* The MUX may have changed part of its route: restore the active
			 * slot. If that fails too, the resume keeps the modem idle until the
			 * route is restored.
			 */
			(void)modem_cellular_apply_sim_slot(data);
			goto out;
		}
	}

	data->sim_slot_active = sim_slot;
	data->sim_slot_requested = NULL;

	/* The SIM identity belongs to the previous slot; it is read again on power-up */
	memset(data->iccid, 0, sizeof(data->iccid));
	memset(data->imsi, 0, sizeof(data->imsi));

	LOG_INF("SIM slot %u selected", slot);

out:
	k_mutex_unlock(&data->api_lock);
	return ret;
}

int cellular_modem_pause_periodic_script(const struct device *dev)
{
	const struct modem_cellular_config *config = dev->config;
	struct modem_cellular_data *data = dev->data;

	if (config->vendor->scripts.periodic == NULL) {
		return -ENOTSUP;
	}

	if (!atomic_cas(&data->periodic_paused, 0, 1)) {
		return -EINVAL;
	}

	return 0;
}

int cellular_modem_resume_periodic_script(const struct device *dev)
{
	const struct modem_cellular_config *config = dev->config;
	struct modem_cellular_data *data = dev->data;

	if (config->vendor->scripts.periodic == NULL) {
		return -ENOTSUP;
	}

	if (!atomic_cas(&data->periodic_paused, 1, 0)) {
		return -EINVAL;
	}

	modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_PERIODIC_KICK);
	return 0;
}

#if defined(CONFIG_MODEM_CELLULAR_STATS)
static const struct cellular_stats *modem_cellular_get_stats(const struct device *dev)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;

	return &data->stats;
}
#endif

DEVICE_API(cellular, modem_cellular_api) = {
	.get_signal = modem_cellular_get_signal,
	.get_modem_info = modem_cellular_get_modem_info,
	.get_registration_status = modem_cellular_get_registration_status,
	.get_network_status = modem_cellular_get_network_status,
	.set_apn = modem_cellular_set_apn,
	.set_callback = modem_cellular_set_callback,
	IF_ENABLED(CONFIG_MODEM_CELLULAR_STATS, (.get_stats = modem_cellular_get_stats,))
	.get_sim_slots = modem_cellular_get_sim_slots,
	.select_sim = modem_cellular_select_sim,
};

int modem_cellular_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;
	k_tid_t current_thread = k_current_get();
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_RESUME);
		ret = 0;
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		if (current_thread == k_work_queue_thread_get(&k_sys_work_q)) {
			/* Suspending from the system workqueue cannot work for several reasons:
			 *   `modem_cellular_delegate_event` handling runs on the workqueue
			 *   `modem_chat` relies on workqueue for executing any shutdown commands
			 * Output the error here to make it clear to users what is happening, and
			 * how to avoid it (don't call `net_if_down` or `conn_mgr_all_if_down` from
			 * the system workqueue).
			 */
			LOG_ERR("Cannot suspend from system workqueue");
			return -EDEADLK;
		}
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_SUSPEND);
		ret = k_sem_take(&data->suspended_sem, K_SECONDS(30));
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				   struct net_if *iface)
{
	struct modem_cellular_data *data =
		CONTAINER_OF(cb, struct modem_cellular_data, net_mgmt_event_callback);

	switch (mgmt_event) {
	case NET_EVENT_PPP_PHASE_DEAD:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_PPP_DEAD);
		break;

	default:
		break;
	}
}

#if defined(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT)
/* On-demand connect requires a dedicated modem workqueue (the Kconfig option
 * selects MODEM_DEDICATED_WORKQUEUE). net_if_down() holds the interface lock
 * while the PPP L2 waits for the peer to acknowledge its LCP Terminate-Request.
 * The driver's own state machine work reacts to that same teardown by taking
 * the interface lock, so with all modem work on the system workqueue the reply
 * that would release net_if_down() queues up behind the blocked state machine
 * and never arrives: net_if_down() fails with -EAGAIN, the interface stays
 * admin-up, no NET_EVENT_IF_ADMIN_DOWN is emitted and the call is never hung
 * up. A dedicated modem workqueue keeps the reply path clear.
 */
static void modem_cellular_if_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
					    struct net_if *iface)
{
	struct modem_cellular_data *data =
		CONTAINER_OF(cb, struct modem_cellular_data, if_event_callback);
	const struct modem_cellular_config *config = data->dev->config;

	/* net_mgmt does not filter by interface, so ignore events for any iface
	 * other than our PPP one.
	 */
	if (iface != modem_ppp_get_iface(config->ppp)) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IF_ADMIN_UP:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_DIAL);
		break;
	case NET_EVENT_IF_ADMIN_DOWN:
		modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_HANGUP);
		break;
	default:
		break;
	}
}
#endif /* CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT */

static void modem_cellular_ring_gpio_callback(const struct device *dev, struct gpio_callback *cb,
					      uint32_t pins)
{
	struct modem_cellular_data *data =
		CONTAINER_OF(cb, struct modem_cellular_data, ring_gpio_cb);

	modem_cellular_delegate_event(data, MODEM_CELLULAR_EVENT_RING);
}

/* Releasing a pipe leaves a real UART backend receiving into the instance */
#if defined(CONFIG_ZTEST) && !defined(CONFIG_MODEM_BACKEND_UART)
void modem_cellular_ztest_halt(const struct device *dev)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;
	const struct modem_cellular_config *config = dev->config;
	struct k_work_sync sync;
	struct modem_cellular_event_pkg event;

	k_work_cancel_delayable_sync(&data->timeout_work, &sync);
	k_work_cancel_sync(&data->event_dispatch_work, &sync);

	net_mgmt_del_event_callback(&data->net_mgmt_event_callback);
#if defined(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT)
	net_mgmt_del_event_callback(&data->if_event_callback);
#endif

	if (modem_cellular_gpio_is_enabled(&config->ring_gpio)) {
		gpio_pin_interrupt_configure_dt(&config->ring_gpio, GPIO_INT_DISABLE);
		gpio_remove_callback(config->ring_gpio.port, &data->ring_gpio_cb);
	}

	modem_cellular_notify_user_pipes_disconnected(data);
	modem_chat_release(&data->chat);
	modem_ppp_release(config->ppp);
	modem_cmux_release(&data->cmux);

	if (data->uart_pipe != NULL) {
		modem_pipe_release(data->uart_pipe);
	}

	if (data->dlci1_pipe != NULL) {
		modem_pipe_release(data->dlci1_pipe);
	}

	if (data->dlci2_pipe != NULL) {
		modem_pipe_release(data->dlci2_pipe);
	}

	for (uint8_t i = 0; i < config->user_pipes_size; i++) {
		struct modem_cellular_user_pipe *user_pipe = &config->user_pipes[i];

		if (user_pipe->pipe != NULL) {
			modem_pipe_release(user_pipe->pipe);
		}
	}

	while (k_msgq_get(&data->event_queue, &event, K_NO_WAIT) == 0) {
	}
	k_work_cancel_sync(&data->event_dispatch_work, &sync);
	k_work_cancel_delayable_sync(&data->timeout_work, &sync);

	/* Halting bypasses the idle state, which normally forgets the registration */
	modem_cellular_clear_registration_status(data);
	net_if_dormant_on(modem_ppp_get_iface(config->ppp));

	/* Return to the state modem_cellular_init() expects on a fresh boot */
	data->state = MODEM_CELLULAR_STATE_IDLE;
	data->original_baudrate = 0;
	data->apn[0] = '\0';
	data->recovery_count = 0;
	data->script_failure_counter = 0;
	atomic_clear(&data->periodic_paused);
	data->periodic_timeout_skipped = false;
	data->power_on_skipped = false;
	data->baudrate_kept = false;
	data->init_script_failed = false;
	data->suspend_pending = false;
	data->cmux_connect_failed = false;
	data->cmux_bus_opened = false;
	data->cmd_pipe = NULL;
}
#endif /* CONFIG_ZTEST && !CONFIG_MODEM_BACKEND_UART */

static void modem_cellular_init_apn(struct modem_cellular_data *data)
{
#ifdef CONFIG_MODEM_CELLULAR_APN
	if (strlen(CONFIG_MODEM_CELLULAR_APN) > 0) {
		strncpy(data->apn, CONFIG_MODEM_CELLULAR_APN, sizeof(data->apn) - 1);
		data->apn[sizeof(data->apn) - 1] = '\0';
	}
#endif

	modem_chat_script_init(&data->apn_script);
	modem_chat_script_set_name(&data->apn_script, "modem_celullar_set_apn");
	modem_chat_script_set_abort_matches(&data->apn_script,
					    abort_matches,
					    ARRAY_SIZE(abort_matches));
	modem_chat_script_set_timeout(&data->apn_script, 1);
	modem_chat_script_set_callback(&data->apn_script,
				       modem_cellular_chat_callback_handler);
}

/**
 * @brief Set the active SIM slot to the default one.
 *
 * The default slot is the one with the default-sim property, or the first slot. For slots
 * that the modem selects, also prepares the script that sends the select command.
 *
 * @param data Modem instance data.
 * @param config Modem instance configuration.
 *
 * @retval 0 Success, also when the modem has no sim-slots node.
 * @retval -ENODEV MUX controller of a SIM slot is not ready.
 * @retval -ENOTSUP SIM slots have no MUX, and the modem driver cannot select them.
 */
static int modem_cellular_init_sim_slots(struct modem_cellular_data *data,
					 const struct modem_cellular_config *config)
{
	data->sim_slot_active = NULL;
	data->sim_slot_modem = -1;

	if (modem_cellular_modem_selects_sim(config)) {
		if (config->vendor->sim_select.select_cmd == NULL) {
			LOG_ERR("SIM slots without mux-states, but the modem cannot select them");
			return -ENOTSUP;
		}

		modem_chat_script_init(&data->sim_select_script);
		modem_chat_script_set_name(&data->sim_select_script, "sim_select");
		modem_chat_script_set_abort_matches(&data->sim_select_script, abort_matches,
						    ARRAY_SIZE(abort_matches));
		modem_chat_script_set_timeout(&data->sim_select_script, 10);
		modem_chat_script_set_callback(&data->sim_select_script,
					       modem_cellular_chat_callback_handler);
	}

	/* At boot, the driver takes a modem it can power on as powered off, unless its status
	 * GPIO tells otherwise
	 */
	data->powered_off = (modem_cellular_gpio_is_enabled(&config->reset_gpio) &&
			     config->hold_reset_on_suspend) ||
			    (modem_cellular_gpio_is_enabled(&config->power_gpio) &&
			     !modem_cellular_modem_is_powered(config));

	for (uint8_t i = 0U; i < config->sim_slots_size; i++) {
		const struct modem_cellular_sim_slot *slot = &config->sim_slots[i];

		if ((slot->mux != NULL) && !device_is_ready(slot->mux)) {
			LOG_ERR("MUX of SIM slot %u not ready", slot->index);
			return -ENODEV;
		}

		if ((data->sim_slot_active == NULL) || slot->is_default) {
			data->sim_slot_active = slot;
		}
	}

	return 0;
}

int modem_cellular_init(const struct device *dev)
{
	struct modem_cellular_data *data = (struct modem_cellular_data *)dev->data;
	struct modem_cellular_config *config = (struct modem_cellular_config *)dev->config;
	const struct gpio_dt_spec *dtr_gpio = NULL;

	data->dev = dev;

	__ASSERT_NO_MSG(config->vendor->scripts.init != NULL);
	__ASSERT_NO_MSG(config->vendor->scripts.dial != NULL);

	/* On-demand connect drives the dial off the PPP interface admin state. A
	 * modem whose vendor configuration provides a network chat script instead
	 * waits for registration and dials as soon as it registers, ignoring that
	 * state, so the data call would come up unprompted regardless of the
	 * option. Reject the combination here rather than dial unexpectedly.
	 */
	if (IS_ENABLED(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT) &&
	    modem_cellular_has_network_script(config)) {
		LOG_ERR("on-demand connect is unsupported by modems with a network chat script");
		return -ENOTSUP;
	}

	k_mutex_init(&data->api_lock);
	k_work_init_delayable(&data->timeout_work, modem_cellular_timeout_handler);
	k_work_init(&data->event_dispatch_work, modem_cellular_event_dispatch_handler);
	k_msgq_init(&data->event_queue, (void *)data->event_buf, sizeof(data->event_buf[0]),
		    ARRAY_SIZE(data->event_buf));

	k_sem_init(&data->suspended_sem, 0, 1);

	data->access_tech = CELLULAR_ACCESS_TECHNOLOGY_UNKNOWN;

	if (modem_cellular_gpio_is_enabled(&config->wake_gpio)) {
		gpio_pin_configure_dt(&config->wake_gpio, GPIO_OUTPUT_INACTIVE);
	}

	if (modem_cellular_gpio_is_enabled(&config->power_gpio)) {
		gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_INACTIVE);
	}

	if (modem_cellular_gpio_is_enabled(&config->reset_gpio)) {
		gpio_flags_t flags =
			(config->hold_reset_on_suspend) ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE;
		gpio_pin_configure_dt(&config->reset_gpio, flags);
	}

	if (modem_cellular_gpio_is_enabled(&config->ring_gpio)) {
		int ret;

		ret = gpio_pin_configure_dt(&config->ring_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure ring GPIO (%d)", ret);
			return ret;
		}

		gpio_init_callback(&data->ring_gpio_cb, modem_cellular_ring_gpio_callback,
				   BIT(config->ring_gpio.pin));

		ret = gpio_add_callback(config->ring_gpio.port, &data->ring_gpio_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add ring GPIO callback (%d)", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->ring_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure ring GPIO interrupt (%d)", ret);
			return ret;
		}

		LOG_DBG("Ring GPIO interrupt configured");
	}

	if (modem_cellular_gpio_is_enabled(&config->dtr_gpio)) {
		gpio_pin_configure_dt(&config->dtr_gpio, GPIO_OUTPUT_INACTIVE);
		dtr_gpio = &config->dtr_gpio;
	}

	if (modem_cellular_gpio_is_enabled(&config->status_gpio)) {
		int ret = gpio_pin_configure_dt(&config->status_gpio, GPIO_INPUT);

		if (ret < 0) {
			LOG_ERR("Failed to configure modem status GPIO (%d)", ret);
			return ret;
		}
	}

	{
		int ret = modem_cellular_init_sim_slots(data, config);

		if (ret < 0) {
			return ret;
		}
	}

	{
		const struct modem_backend_uart_config uart_backend_config = {
			.uart = config->uart,
			.dtr_gpio = dtr_gpio,
			.receive_buf = data->uart_backend_receive_buf,
			.receive_buf_size = ARRAY_SIZE(data->uart_backend_receive_buf),
			.transmit_buf = data->uart_backend_transmit_buf,
			.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
		};

		data->uart_pipe = modem_backend_uart_init(&data->uart_backend,
							  &uart_backend_config);

		data->cmd_pipe = NULL;
	}

	{
		const struct modem_cmux_config cmux_config = {
			.callback = modem_cellular_cmux_handler,
			.user_data = data,
			.receive_buf = data->cmux_receive_buf,
			.receive_buf_size = ARRAY_SIZE(data->cmux_receive_buf),
			.transmit_buf = data->cmux_transmit_buf,
			.transmit_buf_size = ARRAY_SIZE(data->cmux_transmit_buf),
			.enable_runtime_power_management = config->cmux_enable_runtime_power_save,
			.close_pipe_on_power_save = config->cmux_close_pipe_on_power_save,
			.no_powersave_handshake = config->cmux_no_powersave_handshake,
			.idle_timeout = config->cmux_idle_timeout,
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

	for (uint8_t i = 0; i < config->user_pipes_size; i++) {
		struct modem_cellular_user_pipe *user_pipe = &config->user_pipes[i];
		const struct modem_cmux_dlci_config user_dlci_config = {
			.dlci_address = user_pipe->dlci_address,
			.receive_buf = user_pipe->dlci_receive_buf,
			.receive_buf_size = user_pipe->dlci_receive_buf_size,
		};

		user_pipe->pipe = modem_cmux_dlci_init(&data->cmux, &user_pipe->dlci,
						       &user_dlci_config);

		modem_pipelink_init(user_pipe->pipelink, user_pipe->pipe);
	}

	{
		const struct modem_chat_config chat_config = {
			.user_data = data,
			.receive_buf = data->chat_receive_buf,
			.receive_buf_size = ARRAY_SIZE(data->chat_receive_buf),
			.delimiter = (const uint8_t *)config->vendor->chat_delimiter,
			.delimiter_size = strlen(config->vendor->chat_delimiter),
			.filter = (const uint8_t *)config->vendor->chat_filter,
			.filter_size = config->vendor->chat_filter
					       ? strlen(config->vendor->chat_filter)
					       : 0,
			.argv = data->chat_argv,
			.argv_size = ARRAY_SIZE(data->chat_argv),
			.unsol_matches = config->vendor->unsol_matches.matches,
			.unsol_matches_size = config->vendor->unsol_matches.size,
		};

		modem_chat_init(&data->chat, &chat_config);
	}

	{
		net_mgmt_init_event_callback(&data->net_mgmt_event_callback, net_mgmt_event_handler,
					     NET_EVENT_PPP_PHASE_DEAD);
		net_mgmt_add_event_callback(&data->net_mgmt_event_callback);
	}

#if defined(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT)
	{
		net_mgmt_init_event_callback(&data->if_event_callback,
					     modem_cellular_if_event_handler,
					     NET_EVENT_IF_ADMIN_UP | NET_EVENT_IF_ADMIN_DOWN);
		net_mgmt_add_event_callback(&data->if_event_callback);
	}
#endif

	modem_cellular_init_apn(data);

	return pm_device_driver_init(dev, modem_cellular_pm_action);
}
