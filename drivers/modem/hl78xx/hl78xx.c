/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hl78xx.h"
#include "hl78xx_chat.h"
#include "hl78xx_cfg.h"

#define MAX_SCRIPT_AT_CMD_RETRY 3

#define MDM_NODE                         DT_ALIAS(modem)
/* Check phandle target status for a specific phandle index */
#define HAS_GPIO_IDX(node_id, prop, idx) DT_PROP_HAS_IDX(node_id, prop, idx)

/* GPIO availability macros */
#define HAS_RESET_GPIO      HAS_GPIO_IDX(MDM_NODE, mdm_reset_gpios, 0)
#define HAS_WAKE_GPIO       HAS_GPIO_IDX(MDM_NODE, mdm_wake_gpios, 0)
#define HAS_VGPIO_GPIO      HAS_GPIO_IDX(MDM_NODE, mdm_vgpio_gpios, 0)
#define HAS_UART_CTS_GPIO   HAS_GPIO_IDX(MDM_NODE, mdm_uart_cts_gpios, 0)
#define HAS_GPIO6_GPIO      HAS_GPIO_IDX(MDM_NODE, mdm_gpio6_gpios, 0)
#define HAS_PWR_ON_GPIO     HAS_GPIO_IDX(MDM_NODE, mdm_pwr_on_gpios, 0)
#define HAS_FAST_SHUTD_GPIO HAS_GPIO_IDX(MDM_NODE, mdm_fast_shutd_gpios, 0)
#define HAS_UART_DSR_GPIO   HAS_GPIO_IDX(MDM_NODE, mdm_uart_dsr_gpios, 0)
#define HAS_UART_DTR_GPIO   HAS_GPIO_IDX(MDM_NODE, mdm_uart_dtr_gpios, 0)
#define HAS_GPIO8_GPIO      HAS_GPIO_IDX(MDM_NODE, mdm_gpio8_gpios, 0)
#define HAS_SIM_SWITCH_GPIO HAS_GPIO_IDX(MDM_NODE, mdm_sim_switch_gpios, 0)

/* GPIO count macro */
#define GPIO_CONFIG_LEN                                                                            \
	(HAS_RESET_GPIO + HAS_WAKE_GPIO + HAS_VGPIO_GPIO + HAS_UART_CTS_GPIO + HAS_GPIO6_GPIO +    \
	 HAS_PWR_ON_GPIO + HAS_FAST_SHUTD_GPIO + HAS_UART_DSR_GPIO + HAS_UART_DTR_GPIO +           \
	 HAS_GPIO8_GPIO + HAS_SIM_SWITCH_GPIO)

LOG_MODULE_REGISTER(hl78xx_dev, CONFIG_MODEM_LOG_LEVEL);
/* RX thread work queue */
K_KERNEL_STACK_DEFINE(modem_workq_stack, CONFIG_MODEM_HL78XX_RX_WORKQ_STACK_SIZE);

static struct k_work_q modem_workq;
hl78xx_evt_monitor_dispatcher_t event_dispatcher;

static void hl78xx_event_handler(struct hl78xx_data *data, enum hl78xx_event evt);
static int hl78xx_on_idle_state_enter(struct hl78xx_data *data);

struct hl78xx_state_handlers {
	int (*on_enter)(struct hl78xx_data *data);
	int (*on_leave)(struct hl78xx_data *data);
	void (*on_event)(struct hl78xx_data *data, enum hl78xx_event evt);
};

/* Forward declare the table so functions earlier in this file can reference
 * it. The table itself is defined later in the file (without 'static').
 */
const static struct hl78xx_state_handlers hl78xx_state_table[];
/** Dispatch an event to the registered event dispatcher, if any.
 *
 */
static void event_dispatcher_dispatch(struct hl78xx_evt *notif)
{
	if (event_dispatcher != NULL) {
		event_dispatcher(notif);
	}
}
/* -------------------------------------------------------------------------
 * Utilities
 * - small helpers and local utility functions
 * -------------------------------------------------------------------------
 */

static const char *hl78xx_state_str(enum hl78xx_state state)
{
	switch (state) {
	case MODEM_HL78XX_STATE_IDLE:
		return "idle";
	case MODEM_HL78XX_STATE_RESET_PULSE:
		return "reset pulse";
	case MODEM_HL78XX_STATE_POWER_ON_PULSE:
		return "power pulse";
	case MODEM_HL78XX_STATE_AWAIT_POWER_ON:
		return "await power on";
	case MODEM_HL78XX_STATE_SET_BAUDRATE:
		return "set baudrate";
	case MODEM_HL78XX_STATE_RUN_INIT_SCRIPT:
		return "run init script";
	case MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT:
		return "init fail diagnostic script ";
	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		return "run rat cfg script";
	case MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT:
		return "run enable gprs script";
	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		return "await registered";
	case MODEM_HL78XX_STATE_CARRIER_ON:
		return "carrier on";
	case MODEM_HL78XX_STATE_CARRIER_OFF:
		return "carrier off";
	case MODEM_HL78XX_STATE_SIM_POWER_OFF:
		return "sim power off";
	case MODEM_HL78XX_STATE_AIRPLANE:
		return "airplane mode";
	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		return "init power off";
	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		return "power off pulse";
	case MODEM_HL78XX_STATE_AWAIT_POWER_OFF:
		return "await power off";
	default:
		return "UNKNOWN state";
	}

	return "";
}

static const char *hl78xx_event_str(enum hl78xx_event event)
{
	switch (event) {
	case MODEM_HL78XX_EVENT_RESUME:
		return "resume";
	case MODEM_HL78XX_EVENT_SUSPEND:
		return "suspend";
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		return "script success";
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		return "script failed";
	case MODEM_HL78XX_EVENT_SCRIPT_REQUIRE_RESTART:
		return "script require restart";
	case MODEM_HL78XX_EVENT_TIMEOUT:
		return "timeout";
	case MODEM_HL78XX_EVENT_REGISTERED:
		return "registered";
	case MODEM_HL78XX_EVENT_DEREGISTERED:
		return "deregistered";
	case MODEM_HL78XX_EVENT_BUS_OPENED:
		return "bus opened";
	case MODEM_HL78XX_EVENT_BUS_CLOSED:
		return "bus closed";
	case MODEM_HL78XX_EVENT_SOCKET_READY:
		return "socket ready";
	default:
		return "unknown event";
	}

	return "";
}

static bool hl78xx_gpio_is_enabled(const struct gpio_dt_spec *gpio)
{
	return (gpio->port != NULL);
}

static void hl78xx_log_event(enum hl78xx_event evt)
{
	LOG_DBG("event %s", hl78xx_event_str(evt));
}

static void hl78xx_start_timer(struct hl78xx_data *data, k_timeout_t timeout)
{
	k_work_schedule(&data->timeout_work, timeout);
}

static void hl78xx_stop_timer(struct hl78xx_data *data)
{
	k_work_cancel_delayable(&data->timeout_work);
}

static void hl78xx_timeout_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct hl78xx_data *data = CONTAINER_OF(dwork, struct hl78xx_data, timeout_work);

	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_TIMEOUT);
}

static void hl78xx_bus_pipe_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
				    void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_BUS_OPENED);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_BUS_CLOSED);
		break;

	default:
		break;
	}
}

static void hl78xx_log_state_changed(enum hl78xx_state last_state, enum hl78xx_state new_state)
{
	LOG_INF("switch from %s to %s", hl78xx_state_str(last_state), hl78xx_state_str(new_state));
}

static void hl78xx_event_dispatch_handler(struct k_work *item)
{
	struct hl78xx_data *data =
		CONTAINER_OF(item, struct hl78xx_data, events.event_dispatch_work);
	uint8_t events[sizeof(data->events.event_buf)];
	uint8_t events_cnt;

	k_mutex_lock(&data->events.event_rb_lock, K_FOREVER);
	events_cnt = (uint8_t)ring_buf_get(&data->events.event_rb, events,
					   sizeof(data->events.event_buf));
	k_mutex_unlock(&data->events.event_rb_lock);
	LOG_DBG("dequeued %d events", events_cnt);

	for (uint8_t i = 0; i < events_cnt; i++) {
		hl78xx_event_handler(data, (enum hl78xx_event)events[i]);
	}
}

void hl78xx_delegate_event(struct hl78xx_data *data, enum hl78xx_event evt)
{
	k_mutex_lock(&data->events.event_rb_lock, K_FOREVER);
	ring_buf_put(&data->events.event_rb, (uint8_t *)&evt, 1);
	k_mutex_unlock(&data->events.event_rb_lock);
	k_work_submit_to_queue(&modem_workq, &data->events.event_dispatch_work);
}
/* -------------------------------------------------------------------------
 * Chat callbacks / URC handlers
 * - unsolicited response handlers and chat-related parsers
 * -------------------------------------------------------------------------
 */
void hl78xx_on_cxreg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	enum cellular_registration_status registration_status = 0;
	struct hl78xx_evt event = {.type = HL78XX_LTE_REGISTRATION_STAT_UPDATE};
#ifndef CONFIG_MODEM_HL78XX_12
	enum hl78xx_cell_rat_mode rat_mode = HL78XX_RAT_MODE_NONE;
	struct hl78xx_evt rat_event;
	bool rat_mode_updated = false;
	int act_value = -1;
#endif /* CONFIG_MODEM_HL78XX_12 */
	if (argc < 2) {
		return;
	}
	/* +CXREG: <stat>[,<tac>[...]] */
	if (argc > 2 && strlen(argv[1]) == 1 && strlen(argv[2]) == 1) {
		/* This is a condition to distinguish received message between URC message and User
		 * request network status request. If the message is User message, then argv[1] and
		 * argv[2] will be 1 character long. If the message is URC request network status
		 * request, then argv[1] will be 1 character long while argv[2] will be 2 characters
		 * long.
		 */
		registration_status = ATOI(argv[2], 0, "registration_status");
#ifndef CONFIG_MODEM_HL78XX_12
		if (argc > 4 && strlen(argv[5]) == 1) {
			act_value = ATOI(argv[5], -1, "act_value");
			LOG_DBG("act_value: %d, argc: %d, argv[5]: %s", act_value, argc, argv[5]);
			switch (act_value) {
			case 7:
				rat_mode = HL78XX_RAT_CAT_M1;
				break;
			case 9:
				rat_mode = HL78XX_RAT_NB1;
				break;
			default:
				rat_mode = HL78XX_RAT_MODE_NONE;
				break;
			}
			rat_mode_updated = true;
			LOG_DBG("RAT mode from response: %d", rat_mode);
		}
#endif /* CONFIG_MODEM_HL78XX_12 */
	} else {
		registration_status = ATOI(argv[1], 0, "registration_status");
#ifndef CONFIG_MODEM_HL78XX_12
		if (argc > 3 && strlen(argv[4]) == 1) {
			act_value = ATOI(argv[4], -1, "act_value");
			LOG_DBG("act_value: %d, argc: %d, argv[4]: %s", act_value, argc, argv[4]);
			switch (act_value) {
			case 7:
				rat_mode = HL78XX_RAT_CAT_M1;
				break;
			case 9:
				rat_mode = HL78XX_RAT_NB1;
				break;
			default:
				rat_mode = HL78XX_RAT_MODE_NONE;
				break;
			}
			rat_mode_updated = true;
			LOG_DBG("RAT mode from URC: %d", rat_mode);
		}
#endif /* CONFIG_MODEM_HL78XX_12 */
	}
	HL78XX_LOG_DBG("%s: %d", argv[0], registration_status);
	if (registration_status == data->status.registration.network_state_current) {
#ifndef CONFIG_MODEM_HL78XX_12
		/* Check if RAT mode changed even if registration status didn't */
		if (rat_mode_updated && rat_mode != -1 &&
		    rat_mode != data->status.registration.rat_mode) {
			data->status.registration.rat_mode = rat_mode;
			rat_event.type = HL78XX_LTE_RAT_UPDATE;
			rat_event.content.rat_mode = rat_mode;
			event_dispatcher_dispatch(&rat_event);
		}
#endif /* CONFIG_MODEM_HL78XX_12 */
		return;
	}
	/* Update the previous registration state */
	data->status.registration.network_state_previous =
		data->status.registration.network_state_current;
	/* Update the current registration state */
	data->status.registration.network_state_current = registration_status;
	event.content.reg_status = data->status.registration.network_state_current;

	data->status.registration.is_registered_previously =
		data->status.registration.is_registered_currently;
#ifndef CONFIG_MODEM_HL78XX_12
	/* Update RAT mode if parsed */
	if (rat_mode_updated && rat_mode != -1 && rat_mode != data->status.registration.rat_mode) {
		data->status.registration.rat_mode = rat_mode;
		rat_event.type = HL78XX_LTE_RAT_UPDATE;
		rat_event.content.rat_mode = rat_mode;
		event_dispatcher_dispatch(&rat_event);
	}
#endif /* CONFIG_MODEM_HL78XX_12 */
	/* Update current registration flag */
	if (hl78xx_is_registered(data)) {
		data->status.registration.is_registered_currently = true;
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
#ifdef CONFIG_MODEM_HL78XX_STAY_IN_BOOT_MODE_FOR_ROAMING
		k_sem_give(&data->stay_in_boot_mode_sem);
#endif /* CONFIG_MODEM_HL78XX_STAY_IN_BOOT_MODE_FOR_ROAMING */
	} else {
		data->status.registration.is_registered_currently = false;
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEREGISTERED);
	}
	event_dispatcher_dispatch(&event);
}

void hl78xx_on_ksup(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int module_status;
	struct hl78xx_evt event = {.type = HL78XX_LTE_MODEM_STARTUP};

	if (argc != 2) {
		return;
	}
	module_status = ATOI(argv[1], 0, "module_status");
	event.content.value = module_status;
	event_dispatcher_dispatch(&event);
	HL78XX_LOG_DBG("Module status: %d", module_status);
}

void hl78xx_on_imei(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
	HL78XX_LOG_DBG("IMEI: %s %s", argv[0], argv[1]);
	k_mutex_lock(&data->api_lock, K_FOREVER);
	safe_strncpy((char *)data->identity.imei, argv[1], sizeof(data->identity.imei));
	k_mutex_unlock(&data->api_lock);
}

void hl78xx_on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
	HL78XX_LOG_DBG("cgmm: %s %s", argv[0], argv[1]);
	k_mutex_lock(&data->api_lock, K_FOREVER);
	safe_strncpy((char *)data->identity.model_id, argv[1], sizeof(data->identity.model_id));
	k_mutex_unlock(&data->api_lock);
}

void hl78xx_on_imsi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
	HL78XX_LOG_DBG("IMSI: %s %s", argv[0], argv[1]);
	k_mutex_lock(&data->api_lock, K_FOREVER);
	safe_strncpy((char *)data->identity.imsi, argv[1], sizeof(data->identity.imsi));
	k_mutex_unlock(&data->api_lock);
#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
	/* set the APN automatically */
	modem_detect_apn(data, argv[1]);
#endif /* CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI */
}

void hl78xx_on_cgmi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
	HL78XX_LOG_DBG("cgmi: %s %s", argv[0], argv[1]);
	k_mutex_lock(&data->api_lock, K_FOREVER);
	safe_strncpy((char *)data->identity.manufacturer, argv[1],
		     sizeof(data->identity.manufacturer));
	k_mutex_unlock(&data->api_lock);
}

void hl78xx_on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
	HL78XX_LOG_DBG("cgmr: %s %s", argv[0], argv[1]);
	k_mutex_lock(&data->api_lock, K_FOREVER);
	safe_strncpy((char *)data->identity.fw_version, argv[1], sizeof(data->identity.fw_version));
	k_mutex_unlock(&data->api_lock);
}

void hl78xx_on_iccid(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
	HL78XX_LOG_DBG("ICCID: %s %s", argv[0], argv[1]);
	k_mutex_lock(&data->api_lock, K_FOREVER);
	safe_strncpy((char *)data->identity.iccid, argv[1], sizeof(data->identity.iccid));
	k_mutex_unlock(&data->api_lock);

#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID)
	/* set the APN automatically */
	modem_detect_apn(data, argv[1]);
#endif /* CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID */
}

#if defined(CONFIG_MODEM_HL78XX_12)
void hl78xx_on_kstatev(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	enum hl78xx_cell_rat_mode rat_mode = HL78XX_RAT_MODE_NONE;
	struct hl78xx_evt event = {.type = HL78XX_LTE_RAT_UPDATE};

	if (argc != 3) {
		return;
	}
	rat_mode = ATOI(argv[2], 0, "rat_mode");
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	hl78xx_on_kstatev_parser(data, (enum hl78xx_info_transfer_event)ATOI(argv[1], 0, "status"),
				 rat_mode);
#endif /* CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG */
	if (rat_mode != data->status.registration.rat_mode) {
		data->status.registration.rat_mode = rat_mode;
		event.content.rat_mode = data->status.registration.rat_mode;
		event_dispatcher_dispatch(&event);
	}
}
#endif

void hl78xx_on_ksrep(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 2) {
		return;
	}
	data->status.ksrep = ATOI(argv[1], 0, "ksrep");
	HL78XX_LOG_DBG("KSREP: %s %s", argv[0], argv[1]);
}
void hl78xx_on_ksrat(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_evt event = {.type = HL78XX_LTE_RAT_UPDATE};

	if (argc < 2) {
		return;
	}
	data->status.registration.rat_mode = (uint8_t)ATOI(argv[1], 0, "rat_mode");
	event.content.rat_mode = data->status.registration.rat_mode;
	event_dispatcher_dispatch(&event);
	HL78XX_LOG_DBG("KSRAT: %s %s", argv[0], argv[1]);
}

void hl78xx_on_kselacq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 2) {
		return;
	}
	if (argc > 3) {
		data->kselacq_data.mode = 0;
		data->kselacq_data.rat1 = ATOI(argv[1], 0, "rat1");
		data->kselacq_data.rat2 = ATOI(argv[2], 0, "rat2");
		data->kselacq_data.rat3 = ATOI(argv[3], 0, "rat3");
	} else {
		data->kselacq_data.mode = 0;
		data->kselacq_data.rat1 = 0;
		data->kselacq_data.rat2 = 0;
		data->kselacq_data.rat3 = 0;
	}
}

void hl78xx_on_kbndcfg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	uint8_t rat_id;
	uint8_t kbnd_bitmap_size;

	if (argc < 3) {
		return;
	}

	rat_id = ATOI(argv[1], 0, "rat");
	kbnd_bitmap_size = strlen(argv[2]);
	HL78XX_LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
	if (kbnd_bitmap_size >= MDM_BAND_HEX_STR_LEN) {
		LOG_ERR("%d %s Unexpected band bitmap length of %d", __LINE__, __func__,
			kbnd_bitmap_size);
		return;
	}
	if (rat_id >= HL78XX_RAT_COUNT) {
		return;
	}
	data->status.kbndcfg[rat_id].rat = rat_id;
	strncpy(data->status.kbndcfg[rat_id].bnd_bitmap, argv[2], kbnd_bitmap_size);
	data->status.kbndcfg[rat_id].bnd_bitmap[kbnd_bitmap_size] = '\0';
}

void hl78xx_on_csq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 3) {
		return;
	}
	data->status.rssi = ATOI(argv[1], 0, "rssi");
}

void hl78xx_on_cesq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 7) {
		return;
	}
	data->status.rsrq = ATOI(argv[5], 0, "rsrq");
	data->status.rsrp = ATOI(argv[6], 0, "rsrp");
}

void hl78xx_on_cfun(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 2) {
		return;
	}
	data->status.phone_functionality.functionality = ATOI(argv[1], 0, "phone_func");
	data->status.phone_functionality.in_progress = false;
}

void hl78xx_on_cops(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 3) {
		return;
	}
	safe_strncpy((char *)data->status.network_operator.operator, argv[3],
		     sizeof(data->status.network_operator.operator));
	data->status.network_operator.format = ATOI(argv[2], 0, "network_operator_format");
}

/* -------------------------------------------------------------------------
 * Pipe & chat initialization
 * - modem backend pipe setup and chat initialisation helpers
 * -------------------------------------------------------------------------
 */
static void hl78xx_init_pipe(const struct device *dev)
{
	const struct hl78xx_config *cfg = dev->config;
	struct hl78xx_data *data = dev->data;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = cfg->uart,
		.receive_buf = data->buffers.uart_rx,
		.receive_buf_size = sizeof(data->buffers.uart_rx),
		.transmit_buf = data->buffers.uart_tx,
		.transmit_buf_size = ARRAY_SIZE(data->buffers.uart_tx),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

/* Initialize the modem chat subsystem using wrappers from hl78xx_chat.c */
static int modem_init_chat(const struct device *dev)
{
	struct hl78xx_data *data = dev->data;

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->buffers.chat_rx,
		.receive_buf_size = sizeof(data->buffers.chat_rx),
		.delimiter = data->buffers.delimiter,
		.delimiter_size = strlen(data->buffers.delimiter),
		.filter = data->buffers.filter,
		.filter_size = data->buffers.filter ? strlen(data->buffers.filter) : 0,
		.argv = data->buffers.argv,
		.argv_size = (uint16_t)ARRAY_SIZE(data->buffers.argv),
		.unsol_matches = hl78xx_get_unsol_matches(),
		.unsol_matches_size = (uint16_t)hl78xx_get_unsol_matches_size(),
	};

	return modem_chat_init(&data->chat, &chat_config);
}

/* clang-format off */
int modem_dynamic_cmd_send(
			struct hl78xx_data *data,
			modem_chat_script_callback script_user_callback,
			const uint8_t *cmd, uint16_t cmd_size,
			const struct modem_chat_match *response_matches, uint16_t matches_size,
			bool user_cmd
		)
{
	int ret = 0;
	int script_ret = 0;
	/* validate input parameters */
	if (data == NULL) {
		LOG_ERR("%d %s Invalid parameter", __LINE__, __func__);
		errno = EINVAL;
		return -1;
	}
	struct modem_chat_script_chat dynamic_script = {
		.request = cmd,
		.request_size = cmd_size,
		.response_matches = response_matches,
		.response_matches_size = matches_size,
		.timeout = 1000,
	};
	struct modem_chat_script chat_script = {
		.name = "dynamic_script",
		.script_chats = &dynamic_script,
		.script_chats_size = 1,
		.abort_matches = hl78xx_get_abort_matches(),
		.abort_matches_size = hl78xx_get_abort_matches_size(),
		.callback = script_user_callback,
		.timeout = 1000
	};

	ret = k_mutex_lock(&data->tx_lock, K_NO_WAIT);
	if (ret < 0) {
		if (user_cmd == false) {
			errno = -ret;
		}
		return -1;
	}
	/* run the chat script */
	script_ret = modem_chat_run_script(&data->chat, &chat_script);
	if (script_ret < 0) {
		LOG_ERR("%d %s Failed to run at command: %d", __LINE__, __func__, script_ret);
	} else {
		LOG_DBG("Chat script executed successfully.");
	}
	ret = k_mutex_unlock(&data->tx_lock);
	if (ret < 0) {
		if (user_cmd == false) {
			errno = -ret;
		}
		/* we still return the script result if available, prioritize script_ret */
		return script_ret < 0 ? -1 : script_ret;
	}
	return script_ret;
}
/* clang-format on */

/* -------------------------------------------------------------------------
 * GPIO ISR callbacks
 * - lightweight wrappers for GPIO interrupts (logging & event dispatch)
 * -------------------------------------------------------------------------
 */
void mdm_vgpio_callback_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct hl78xx_data *data = CONTAINER_OF(cb, struct hl78xx_data, gpio_cbs.vgpio_cb);
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	const struct gpio_dt_spec *spec = &config->mdm_gpio_vgpio;

	if (spec == NULL || spec->port == NULL) {
		LOG_ERR("VGPIO GPIO spec is not configured properly");
		return;
	}
	if (!(pins & BIT(spec->pin))) {
		return; /*  not our pin */
	}
	LOG_DBG("VGPIO ISR callback %s %d %d", spec->port->name, spec->pin, gpio_pin_get_dt(spec));
}

#if HAS_UART_DSR_GPIO
void mdm_uart_dsr_callback_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct hl78xx_data *data = CONTAINER_OF(cb, struct hl78xx_data, gpio_cbs.vgpio_cb);
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	const struct gpio_dt_spec *spec = &config->mdm_gpio_uart_dsr;

	if (spec == NULL || spec->port == NULL) {
		LOG_ERR("DSR GPIO spec is not configured properly");
		return;
	}
	if (!(pins & BIT(spec->pin))) {
		return; /*  not our pin */
	}
	LOG_DBG("DSR ISR callback %d", gpio_pin_get_dt(spec));
}
#endif

void mdm_gpio6_callback_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct hl78xx_data *data = CONTAINER_OF(cb, struct hl78xx_data, gpio_cbs.gpio6_cb);
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	const struct gpio_dt_spec *spec = &config->mdm_gpio_gpio6;

	if (spec == NULL || spec->port == NULL) {
		LOG_ERR("GPIO6 GPIO spec is not configured properly");
		return;
	}
	if (!(pins & BIT(spec->pin))) {
		return; /*  not our pin */
	}
	LOG_DBG("GPIO6 ISR callback %s %d %d", spec->port->name, spec->pin, gpio_pin_get_dt(spec));
}

void mdm_uart_cts_callback_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct hl78xx_data *data = CONTAINER_OF(cb, struct hl78xx_data, gpio_cbs.gpio6_cb);
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	const struct gpio_dt_spec *spec = &config->mdm_gpio_uart_cts;

	if (spec == NULL || spec->port == NULL) {
		LOG_ERR("CTS GPIO spec is not configured properly");
		return;
	}
	if (!(pins & BIT(spec->pin))) {
		return; /*  not our pin */
	}
	LOG_DBG("CTS ISR callback %d", gpio_pin_get_dt(spec));
}

bool hl78xx_is_registered(struct hl78xx_data *data)
{
	return (data->status.registration.network_state_current ==
		CELLULAR_REGISTRATION_REGISTERED_HOME) ||
	       (data->status.registration.network_state_current ==
		CELLULAR_REGISTRATION_REGISTERED_ROAMING);
}

/*
 * hl78xx_is_registered - convenience helper
 *
 * Simple predicate to test if the modem reports a registered state.
 */

static int hl78xx_on_reset_pulse_state_enter(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
	}
	gpio_pin_set_dt(&config->mdm_gpio_reset, 1);
	hl78xx_start_timer(data, K_MSEC(config->reset_pulse_duration_ms));
	return 0;
}

/* -------------------------------------------------------------------------
 * State machine handlers
 * - state enter/leave and per-state event handlers
 * -------------------------------------------------------------------------
 */

static void hl78xx_reset_pulse_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int hl78xx_on_reset_pulse_state_leave(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
		gpio_pin_set_dt(&config->mdm_gpio_reset, 0);
	}
	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
	}
	hl78xx_stop_timer(data);
	return 0;
}

static int hl78xx_on_power_on_pulse_state_enter(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
		gpio_pin_set_dt(&config->mdm_gpio_pwr_on, 1);
	}
	hl78xx_start_timer(data, K_MSEC(config->power_pulse_duration_ms));
	return 0;
}

static void hl78xx_power_on_pulse_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int hl78xx_on_power_on_pulse_state_leave(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
		gpio_pin_set_dt(&config->mdm_gpio_pwr_on, 0);
	}
	hl78xx_stop_timer(data);
	return 0;
}

static int hl78xx_on_await_power_on_state_enter(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	hl78xx_start_timer(data, K_MSEC(config->startup_time_ms));
	return 0;
}

static void hl78xx_await_power_on_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	default:
		break;
	}
}
static int hl78xx_on_run_init_script_state_enter(struct hl78xx_data *data)
{
	modem_pipe_attach(data->uart_pipe, hl78xx_bus_pipe_handler, data);
	return modem_pipe_open_async(data->uart_pipe);
}

static void hl78xx_run_init_script_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		/* Run init script via chat TU wrapper (script symbols live in hl78xx_chat.c) */
		hl78xx_run_init_script_async(data);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_BUS_CLOSED:
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT);
		break;

	default:
		break;
	}
}

static int hl78xx_on_run_init_diagnose_script_state_enter(struct hl78xx_data *data)
{
	hl78xx_run_init_fail_script_async(data);
	return 0;
}

static void hl78xx_run_init_fail_script_event_handler(struct hl78xx_data *data,
						      enum hl78xx_event evt)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		if (data->status.ksrep == 0) {
			hl78xx_run_enable_ksup_urc_script_async(data);
			hl78xx_start_timer(data, K_MSEC(config->shutdown_time_ms));
		} else {
			if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
				hl78xx_enter_state(data, MODEM_HL78XX_STATE_RESET_PULSE);
			}
		}
		break;
	case MODEM_HL78XX_EVENT_TIMEOUT:
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_POWER_ON_PULSE);
			break;
		}

		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_RESET_PULSE);
			break;
		}

		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;
	case MODEM_HL78XX_EVENT_BUS_CLOSED:
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		if (!hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			LOG_ERR("modem wake pin is not enabled, make sure modem low power is "
				"disabled, if you are not sure enable wake up pin by adding it "
				"dts!!");
		}

		if (data->status.script_fail_counter++ < MAX_SCRIPT_AT_CMD_RETRY) {
			if (hl78xx_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
				hl78xx_enter_state(data, MODEM_HL78XX_STATE_POWER_ON_PULSE);
				break;
			}
			if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
				hl78xx_enter_state(data, MODEM_HL78XX_STATE_RESET_PULSE);
				break;
			}
		}
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;
	default:
		break;
	}
}

static int hl78xx_on_rat_cfg_script_state_enter(struct hl78xx_data *data)
{
	int ret = 0;
	bool modem_require_restart = false;
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	enum hl78xx_cell_rat_mode rat_config_request = HL78XX_RAT_MODE_NONE;
	const char *cmd_restart = (const char *)SET_AIRPLANE_MODE_CMD;

	ret = hl78xx_rat_cfg(data, &modem_require_restart, &rat_config_request);
	if (ret < 0) {
		goto error;
	}

	ret = hl78xx_band_cfg(data, &modem_require_restart, rat_config_request);
	if (ret < 0) {
		goto error;
	}

	if (modem_require_restart) {
		ret = modem_dynamic_cmd_send(data, NULL, cmd_restart, strlen(cmd_restart),
					     hl78xx_get_ok_match(), 1, false);
		if (ret < 0) {
			goto error;
		}
		hl78xx_start_timer(data,
				   K_MSEC(config->shutdown_time_ms + config->startup_time_ms));
		return 0;
	}
	hl78xx_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_SUCCESS, data);
	return 0;
error:
	hl78xx_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_ABORT, data);
	LOG_ERR("%d %s Failed to send command: %d", __LINE__, __func__, ret);
	return ret;
}

static void hl78xx_run_rat_cfg_script_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	int ret = 0;

	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		LOG_DBG("Rebooting modem to apply new RAT settings");
		ret = hl78xx_run_post_restart_script_async(data);
		if (ret < 0) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SUSPEND);
		}
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;
	default:
		break;
	}
}

static int hl78xx_on_await_power_off_state_enter(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	hl78xx_start_timer(data, K_MSEC(config->shutdown_time_ms));
	return 0;
}

static void hl78xx_await_power_off_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	if (evt == MODEM_HL78XX_EVENT_TIMEOUT) {
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
	}
}

static int hl78xx_on_enable_gprs_state_enter(struct hl78xx_data *data)
{
	int ret = 0;
	/* Apply the APN if not configured yet */
	if (data->status.apn.state == APN_STATE_REFRESH_REQUESTED) {
		/* APN has been updated by the user, apply the new APN */
		HL78XX_LOG_DBG("APN refresh requested, applying new APN: \"%s\"",
			       data->identity.apn);
		data->status.apn.state = APN_STATE_NOT_CONFIGURED;
	} else {
#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_KCONFIG)
		snprintf(data->identity.apn, sizeof(data->identity.apn), "%s",
			 CONFIG_MODEM_HL78XX_APN);
#elif defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID) || defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
		/* autodetect APN from IMSI */
		/* the list of SIM profiles. Global scope, so the app can change it */
		/* AT+CCID or AT+CIMI needs to be run here if it is not ran in the init script */
		if (strlen(data->identity.apn) < 1) {
			LOG_WRN("%d %s APN is left blank", __LINE__, __func__);
		}
#else /* defined(CONFIG_MODEM_HL78XX_APN_SOURCE_NETWORK) */
/* set blank string to get apn from network */
#endif
	}
	ret = hl78xx_api_func_set_phone_functionality(data->dev, HL78XX_AIRPLANE, false);
	if (ret) {
		goto error;
	}
	ret = hl78xx_set_apn_internal(data, data->identity.apn, strlen(data->identity.apn));
	if (ret) {
		goto error;
	}
#if defined(CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE)
	ret = hl78xx_api_func_set_phone_functionality(data->dev, HL78XX_FULLY_FUNCTIONAL, false);
	if (ret) {
		goto error;
	}
#endif /* CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE */
	hl78xx_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_SUCCESS, data);
	return 0;
error:
	hl78xx_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_ABORT, data);
	LOG_ERR("%d %s Failed to send command: %d", __LINE__, __func__, ret);
	return ret;
}

static void hl78xx_enable_gprs_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		hl78xx_start_timer(data, MODEM_HL78XX_PERIODIC_SCRIPT_TIMEOUT);
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		break;

	case MODEM_HL78XX_EVENT_REGISTERED:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_CARRIER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int hl78xx_on_await_registered_state_enter(struct hl78xx_data *data)
{
	return 0;
}

static void hl78xx_await_registered_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		hl78xx_start_timer(data, K_SECONDS(MDM_REGISTRATION_TIMEOUT));
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		/**
		 * No need to run periodic script to check registration status because URC is used
		 * to notify the status change.
		 *
		 * If the modem is not registered within the timeout period, it will stay in this
		 * state forever.
		 *
		 * @attention MDM_REGISTRATION_TIMEOUT should be long enough to allow the modem to
		 * register to the network, especially for the first time registration. And also
		 * consider the network conditions / number of bands etc.. that may affect
		 * the registration process.
		 *
		 * TODO: add a mechanism to exit this state and retry the registration process
		 *
		 */
		LOG_WRN("Modem failed to register to the network within %d seconds",
			MDM_REGISTRATION_TIMEOUT);

		break;

	case MODEM_HL78XX_EVENT_REGISTERED:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_CARRIER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int hl78xx_on_await_registered_state_leave(struct hl78xx_data *data)
{
	hl78xx_stop_timer(data);
	return 0;
}

static int hl78xx_on_carrier_on_state_enter(struct hl78xx_data *data)
{
	notif_carrier_on(data->dev);
	iface_status_work_cb(data, hl78xx_chat_callback_handler);
	return 0;
}

static void hl78xx_carrier_on_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_start_timer(data, K_SECONDS(2));

		break;
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		dns_work_cb(data->dev, true);
		break;

	case MODEM_HL78XX_EVENT_DEREGISTERED:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int hl78xx_on_carrier_on_state_leave(struct hl78xx_data *data)
{
	hl78xx_stop_timer(data);
	return 0;
}

static int hl78xx_on_carrier_off_state_enter(struct hl78xx_data *data)
{
	notif_carrier_off(data->dev);
	/* Check whether or not there is any sockets are connected,
	 * if true, wait until sockets are closed properly
	 */
	if (check_if_any_socket_connected(data->dev) == false) {
		hl78xx_start_timer(data, K_MSEC(100));
	} else {
		/* There are still active sockets, wait until they are closed */
		hl78xx_start_timer(data, K_MSEC(5000));
	}
	return 0;
}

static void hl78xx_carrier_off_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
	case MODEM_HL78XX_EVENT_TIMEOUT:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_DEREGISTERED:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int hl78xx_on_carrier_off_state_leave(struct hl78xx_data *data)
{
	hl78xx_stop_timer(data);
	return 0;
}

/* pwroff script moved to hl78xx_chat.c */
static int hl78xx_on_init_power_off_state_enter(struct hl78xx_data *data)
{
	/**
	 * Eventhough you have power switch or etc.., start the power off script first
	 * to gracefully disconnect from the network
	 *
	 * IMSI detach before powering down IS recommended by the AT command manual
	 *
	 */
	return hl78xx_run_pwroff_script_async(data);
}

static void hl78xx_init_power_off_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		break;

	case MODEM_HL78XX_EVENT_DEREGISTERED:
		hl78xx_stop_timer(data);
		break;

	default:
		break;
	}
}

static int hl78xx_on_init_power_off_state_leave(struct hl78xx_data *data)
{
	return 0;
}

static int hl78xx_on_power_off_pulse_state_enter(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
		gpio_pin_set_dt(&config->mdm_gpio_pwr_on, 1);
	}
	hl78xx_start_timer(data, K_MSEC(config->power_pulse_duration_ms));
	return 0;
}

static void hl78xx_power_off_pulse_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	if (evt == MODEM_HL78XX_EVENT_TIMEOUT) {
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_OFF);
	}
}

static int hl78xx_on_power_off_pulse_state_leave(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
		gpio_pin_set_dt(&config->mdm_gpio_pwr_on, 0);
	}
	hl78xx_stop_timer(data);
	return 0;
}

static int hl78xx_on_idle_state_enter(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
	}

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
		gpio_pin_set_dt(&config->mdm_gpio_reset, 1);
	}
	modem_chat_release(&data->chat);
	modem_pipe_attach(data->uart_pipe, hl78xx_bus_pipe_handler, data);
	modem_pipe_close_async(data->uart_pipe);
	k_sem_give(&data->suspended_sem);
	return 0;
}

static void hl78xx_idle_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	switch (evt) {
	case MODEM_HL78XX_EVENT_BUS_CLOSED:
		break;
	case MODEM_HL78XX_EVENT_RESUME:
		if (config->autostarts) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_ON);
			break;
		}

		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_POWER_ON_PULSE);
			break;
		}

		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_ON);
			break;
		}
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		k_sem_give(&data->suspended_sem);
		break;

	default:
		break;
	}
}

static int hl78xx_on_idle_state_leave(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	k_sem_take(&data->suspended_sem, K_NO_WAIT);

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
		gpio_pin_set_dt(&config->mdm_gpio_reset, 0);
	}
	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
	}

	return 0;
}

static int hl78xx_on_state_enter(struct hl78xx_data *data)
{
	int ret = 0;
	enum hl78xx_state s = data->status.state;

	/* Use an explicit bounds check against the last enum value so this
	 * code can reference the table even though the table is defined later
	 * in the file. MODEM_HL78XX_STATE_AWAIT_POWER_OFF is the last value in
	 * the `enum hl78xx_state`.
	 */
	if ((int)s <= MODEM_HL78XX_STATE_AWAIT_POWER_OFF && hl78xx_state_table[s].on_enter) {
		ret = hl78xx_state_table[s].on_enter(data);
	}

	return ret;
}

static int hl78xx_on_state_leave(struct hl78xx_data *data)
{
	int ret = 0;
	enum hl78xx_state s = data->status.state;

	if ((int)s <= MODEM_HL78XX_STATE_AWAIT_POWER_OFF && hl78xx_state_table[s].on_leave) {
		ret = hl78xx_state_table[s].on_leave(data);
	}

	return ret;
}

void hl78xx_enter_state(struct hl78xx_data *data, enum hl78xx_state state)
{
	int ret;

	ret = hl78xx_on_state_leave(data);

	if (ret < 0) {
		LOG_WRN("failed to leave state, error: %i", ret);

		return;
	}

	data->status.state = state;
	ret = hl78xx_on_state_enter(data);

	if (ret < 0) {
		LOG_WRN("failed to enter state error: %i", ret);
	}
}

static void hl78xx_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	enum hl78xx_state state;
	enum hl78xx_state s;

	hl78xx_log_event(evt);
	s = data->status.state;
	state = data->status.state;
	if ((int)s <= MODEM_HL78XX_STATE_AWAIT_POWER_OFF && hl78xx_state_table[s].on_event) {
		hl78xx_state_table[s].on_event(data, evt);
	} else {
		LOG_ERR("%d %s unknown event", __LINE__, __func__);
	}
	if (state != s) {
		hl78xx_log_state_changed(state, s);
	}
}

#ifdef CONFIG_PM_DEVICE

/* -------------------------------------------------------------------------
 * Power management
 * -------------------------------------------------------------------------
 */

static int hl78xx_driver_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	int ret = 0;

	LOG_WRN("%d %s PM_DEVICE_ACTION: %d", __LINE__, __func__, action);
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* suspend the device */
		LOG_DBG("%d PM_DEVICE_ACTION_SUSPEND", __LINE__);
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SUSPEND);
		ret = k_sem_take(&data->suspended_sem, K_SECONDS(30));
		break;
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("%d PM_DEVICE_ACTION_RESUME", __LINE__);
		/* resume the device */
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * powered on the device, used when the power
		 * domain this device belongs is resumed.
		 */
		LOG_DBG("%d PM_DEVICE_ACTION_TURN_ON", __LINE__);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		/*
		 * power off the device, used when the power
		 * domain this device belongs is suspended.
		 */
		LOG_DBG("%d PM_DEVICE_ACTION_TURN_OFF", __LINE__);
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/* -------------------------------------------------------------------------
 * Initialization
 * -------------------------------------------------------------------------
 */
static int hl78xx_init(const struct device *dev)
{
	int ret;
	const struct hl78xx_config *config = (const struct hl78xx_config *)dev->config;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	k_mutex_init(&data->api_lock);
	k_mutex_init(&data->tx_lock);
	/* Initialize work queue and event handling */
	k_work_queue_start(&modem_workq, modem_workq_stack,
			   K_KERNEL_STACK_SIZEOF(modem_workq_stack), K_PRIO_COOP(7), NULL);
	k_work_init_delayable(&data->timeout_work, hl78xx_timeout_handler);
	k_work_init(&data->events.event_dispatch_work, hl78xx_event_dispatch_handler);
	ring_buf_init(&data->events.event_rb, sizeof(data->events.event_buf),
		      data->events.event_buf);
	k_sem_init(&data->suspended_sem, 0, 1);
#ifdef CONFIG_MODEM_HL78XX_STAY_IN_BOOT_MODE_FOR_ROAMING
	k_sem_init(&data->stay_in_boot_mode_sem, 0, 1);
#endif /* CONFIG_MODEM_HL78XX_STAY_IN_BOOT_MODE_FOR_ROAMING */
	k_sem_init(&data->script_stopped_sem_tx_int, 0, 1);
	k_sem_init(&data->script_stopped_sem_rx_int, 0, 1);
	data->dev = dev;
	/* reset to default  */
	data->buffers.eof_pattern_size = strlen(data->buffers.eof_pattern);
	data->buffers.termination_pattern_size = strlen(data->buffers.termination_pattern);
	memset(data->identity.apn, 0, MDM_APN_MAX_LENGTH);
	/* GPIO validation */
	const struct gpio_dt_spec *gpio_pins[GPIO_CONFIG_LEN] = {
#if HAS_RESET_GPIO
		&config->mdm_gpio_reset,
#endif
#if HAS_WAKE_GPIO
		&config->mdm_gpio_wake,
#endif
#if HAS_VGPIO_GPIO
		&config->mdm_gpio_vgpio,
#endif
#if HAS_UART_CTS_GPIO
		&config->mdm_gpio_uart_cts,
#endif
#if HAS_GPIO6_GPIO
		&config->mdm_gpio_gpio6,
#endif
#if HAS_PWR_ON_GPIO
		&config->mdm_gpio_pwr_on,
#endif
#if HAS_FAST_SHUTD_GPIO
		&config->mdm_gpio_fast_shutdown,
#endif
#if HAS_UART_DSR_GPIO
		&config->mdm_gpio_uart_dsr,
#endif
#if HAS_UART_DTR_GPIO
		&config->mdm_gpio_uart_dtr,
#endif
#if HAS_GPIO8_GPIO
		&config->mdm_gpio_gpio8,
#endif
#if HAS_SIM_SWITCH_GPIO
		&config->mdm_gpio_sim_switch,
#endif
	};
	for (int i = 0; i < ARRAY_SIZE(gpio_pins); i++) {
		if (gpio_pins[i] == NULL || !gpio_is_ready_dt(gpio_pins[i])) {
			const char *port_name = "unknown";

			if (gpio_pins[i] != NULL && gpio_pins[i]->port != NULL) {
				port_name = gpio_pins[i]->port->name;
			}
			LOG_ERR("GPIO port (%s) not ready!", port_name);
			return -ENODEV;
		}
	}
	/* GPIO configuration */
	struct {
		const struct gpio_dt_spec *spec;
		gpio_flags_t flags;
		const char *name;
	} gpio_config[GPIO_CONFIG_LEN] = {
#if HAS_RESET_GPIO
		{&config->mdm_gpio_reset, GPIO_OUTPUT, "reset"},
#endif
#if HAS_WAKE_GPIO
		{&config->mdm_gpio_wake, GPIO_OUTPUT, "wake"},
#endif
#if HAS_VGPIO_GPIO
		{&config->mdm_gpio_vgpio, GPIO_INPUT, "VGPIO"},
#endif
#if HAS_UART_CTS_GPIO
		{&config->mdm_gpio_uart_cts, GPIO_INPUT, "CTS"},
#endif
#if HAS_GPIO6_GPIO
		{&config->mdm_gpio_gpio6, GPIO_INPUT, "GPIO6"},
#endif
#if HAS_PWR_ON_GPIO
		{&config->mdm_gpio_pwr_on, GPIO_OUTPUT, "pwr_on"},
#endif
#if HAS_FAST_SHUTD_GPIO
		{&config->mdm_gpio_fast_shutdown, GPIO_OUTPUT, "fast_shutdown"},
#endif
#if HAS_UART_DSR_GPIO
		{&config->mdm_gpio_uart_dsr, GPIO_INPUT, "DSR"},
#endif
#if HAS_UART_DTR_GPIO
		{&config->mdm_gpio_uart_dtr, GPIO_OUTPUT, "DTR"},
#endif
#if HAS_GPIO8_GPIO
		{&config->mdm_gpio_gpio8, GPIO_INPUT, "GPIO8"},
#endif
#if HAS_SIM_SWITCH_GPIO
		{&config->mdm_gpio_sim_switch, GPIO_INPUT, "SIM_SWITCH"},
#endif
	};
	for (int i = 0; i < ARRAY_SIZE(gpio_config); i++) {
		ret = gpio_pin_configure_dt(gpio_config[i].spec, gpio_config[i].flags);
		if (ret < 0) {
			LOG_ERR("Failed to configure %s pin", gpio_config[i].name);
			goto error;
		}
	}
#if HAS_VGPIO_GPIO
	/* VGPIO interrupt setup */
	gpio_init_callback(&data->gpio_cbs.vgpio_cb, mdm_vgpio_callback_isr,
			   BIT(config->mdm_gpio_vgpio.pin));

	ret = gpio_add_callback(config->mdm_gpio_vgpio.port, &data->gpio_cbs.vgpio_cb);
	if (ret) {
		LOG_ERR("Cannot setup VGPIO callback! (%d)", ret);
		goto error;
	}
	ret = gpio_pin_interrupt_configure_dt(&config->mdm_gpio_vgpio, GPIO_INT_EDGE_BOTH);
	if (ret) {
		LOG_ERR("Error configuring VGPIO interrupt! (%d)", ret);
		goto error;
	}
#endif /* HAS_VGPIO_GPIO */
#if HAS_GPIO6_GPIO
	/* GPIO6 interrupt setup */
	gpio_init_callback(&data->gpio_cbs.gpio6_cb, mdm_gpio6_callback_isr,
			   BIT(config->mdm_gpio_gpio6.pin));

	ret = gpio_add_callback(config->mdm_gpio_gpio6.port, &data->gpio_cbs.gpio6_cb);
	if (ret) {
		LOG_ERR("Cannot setup GPIO6 callback! (%d)", ret);
		goto error;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->mdm_gpio_gpio6, GPIO_INT_EDGE_BOTH);
	if (ret) {
		LOG_ERR("Error configuring GPIO6 interrupt! (%d)", ret);
		goto error;
	}
#endif /* HAS_GPIO6_GPIO */
	/* UART pipe initialization */
	(void)hl78xx_init_pipe(dev);

	ret = modem_init_chat(dev);
	if (ret < 0) {
		goto error;
	}

#ifndef CONFIG_PM_DEVICE
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
#else
	pm_device_init_suspended(dev);
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_MODEM_HL78XX_STAY_IN_BOOT_MODE_FOR_ROAMING
	k_sem_take(&data->stay_in_boot_mode_sem, K_FOREVER);
#endif
	return 0;
error:
	return ret;
}

int hl78xx_evt_notif_handler_set(hl78xx_evt_monitor_dispatcher_t handler)
{
	event_dispatcher = handler;
	return 0;
}

/*
 * State handler table
 * Maps each hl78xx_state to optional enter/leave/event handlers. NULL
 * entries mean the state has no action for that phase.
 */

/* clang-format off */
const static struct hl78xx_state_handlers hl78xx_state_table[] = {
	[MODEM_HL78XX_STATE_IDLE] = {
		hl78xx_on_idle_state_enter,
		hl78xx_on_idle_state_leave,
		hl78xx_idle_event_handler
	},
	[MODEM_HL78XX_STATE_RESET_PULSE] = {
		hl78xx_on_reset_pulse_state_enter,
		hl78xx_on_reset_pulse_state_leave,
		hl78xx_reset_pulse_event_handler
	},
	[MODEM_HL78XX_STATE_POWER_ON_PULSE] = {
		hl78xx_on_power_on_pulse_state_enter,
		hl78xx_on_power_on_pulse_state_leave,
		hl78xx_power_on_pulse_event_handler
	},
	[MODEM_HL78XX_STATE_AWAIT_POWER_ON] = {
		hl78xx_on_await_power_on_state_enter,
		NULL,
		hl78xx_await_power_on_event_handler
	},
	[MODEM_HL78XX_STATE_SET_BAUDRATE] = {
		NULL,
		NULL,
		NULL
	},
	[MODEM_HL78XX_STATE_RUN_INIT_SCRIPT] = {
		hl78xx_on_run_init_script_state_enter,
		NULL,
		hl78xx_run_init_script_event_handler
	},
	[MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT] = {
		hl78xx_on_run_init_diagnose_script_state_enter,
		NULL,
		hl78xx_run_init_fail_script_event_handler
	},
	[MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT] = {
		hl78xx_on_rat_cfg_script_state_enter,
		NULL,
		hl78xx_run_rat_cfg_script_event_handler
	},
	[MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT] = {
		hl78xx_on_enable_gprs_state_enter,
		NULL,
		hl78xx_enable_gprs_event_handler
	},
	[MODEM_HL78XX_STATE_AWAIT_REGISTERED] = {
		hl78xx_on_await_registered_state_enter,
		hl78xx_on_await_registered_state_leave,
		hl78xx_await_registered_event_handler
	},
	[MODEM_HL78XX_STATE_CARRIER_ON] = {
		hl78xx_on_carrier_on_state_enter,
		hl78xx_on_carrier_on_state_leave,
		hl78xx_carrier_on_event_handler
	},
	[MODEM_HL78XX_STATE_CARRIER_OFF] = {
		hl78xx_on_carrier_off_state_enter,
		hl78xx_on_carrier_off_state_leave,
		hl78xx_carrier_off_event_handler
	},
	[MODEM_HL78XX_STATE_SIM_POWER_OFF] = {
		NULL,
		NULL,
		NULL
	},
	[MODEM_HL78XX_STATE_AIRPLANE] = {
		NULL,
		NULL,
		NULL
	},
	[MODEM_HL78XX_STATE_INIT_POWER_OFF] = {
		hl78xx_on_init_power_off_state_enter,
		hl78xx_on_init_power_off_state_leave,
		hl78xx_init_power_off_event_handler
	},
	[MODEM_HL78XX_STATE_POWER_OFF_PULSE] = {
		hl78xx_on_power_off_pulse_state_enter,
		hl78xx_on_power_off_pulse_state_leave,
		hl78xx_power_off_pulse_event_handler
	},
	[MODEM_HL78XX_STATE_AWAIT_POWER_OFF] = {
		hl78xx_on_await_power_off_state_enter,
		NULL,
		hl78xx_await_power_off_event_handler
	},
};
/* clang-format on */
static DEVICE_API(cellular, hl78xx_api) = {
	.get_signal = hl78xx_api_func_get_signal,
	.get_modem_info = hl78xx_api_func_get_modem_info_standard,
	.get_registration_status = hl78xx_api_func_get_registration_status,
	.set_apn = hl78xx_api_func_set_apn,
	.set_callback = NULL,
};
/* -------------------------------------------------------------------------
 * Device API and DT registration
 * -------------------------------------------------------------------------
 */
#define MODEM_HL78XX_DEFINE_INSTANCE(inst, power_ms, reset_ms, startup_ms, shutdown_ms, start,     \
				     init_script, periodic_script)                                 \
	static const struct hl78xx_config hl78xx_cfg_##inst = {                                    \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.mdm_gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),             \
		.mdm_gpio_wake = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_wake_gpios, {}),               \
		.mdm_gpio_pwr_on = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_pwr_on_gpios, {}),           \
		.mdm_gpio_fast_shutdown =                                                          \
			GPIO_DT_SPEC_INST_GET_OR(inst, mdm_fast_shutd_gpios, {}),                  \
		.mdm_gpio_uart_dtr = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_uart_dtr_gpios, {}),       \
		.mdm_gpio_uart_dsr = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_uart_dsr_gpios, {}),       \
		.mdm_gpio_uart_cts = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_uart_cts_gpios, {}),       \
		.mdm_gpio_vgpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_vgpio_gpios, {}),             \
		.mdm_gpio_gpio6 = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_gpio6_gpios, {}),             \
		.mdm_gpio_gpio8 = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_gpio8_gpios, {}),             \
		.mdm_gpio_sim_switch = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_sim_select_gpios, {}),   \
		.power_pulse_duration_ms = (power_ms),                                             \
		.reset_pulse_duration_ms = (reset_ms),                                             \
		.startup_time_ms = (startup_ms),                                                   \
		.shutdown_time_ms = (shutdown_ms),                                                 \
		.autostarts = (start),                                                             \
		.init_chat_script = (init_script),                                                 \
		.periodic_chat_script = (periodic_script),                                         \
	};                                                                                         \
	static struct hl78xx_data hl78xx_data_##inst = {                                           \
		.buffers.delimiter = "\r\n",                                                       \
		.buffers.eof_pattern = EOF_PATTERN,                                                \
		.buffers.termination_pattern = TERMINATION_PATTERN,                                \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, hl78xx_driver_pm_action);                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, hl78xx_init, PM_DEVICE_DT_INST_GET(inst), &hl78xx_data_##inst, \
			      &hl78xx_cfg_##inst, POST_KERNEL,                                     \
			      CONFIG_MODEM_HL78XX_DEV_INIT_PRIORITY, &hl78xx_api);

#define MODEM_DEVICE_SWIR_HL78XX(inst)                                                             \
	MODEM_HL78XX_DEFINE_INSTANCE(inst, CONFIG_MODEM_HL78XX_DEV_POWER_PULSE_DURATION,           \
				     CONFIG_MODEM_HL78XX_DEV_RESET_PULSE_DURATION,                 \
				     CONFIG_MODEM_HL78XX_DEV_STARTUP_TIME,                         \
				     CONFIG_MODEM_HL78XX_DEV_SHUTDOWN_TIME, false, NULL, NULL)

#define DT_DRV_COMPAT swir_hl7812
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
