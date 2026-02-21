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
#ifdef CONFIG_HL78XX_GNSS
#include "hl78xx_gnss.h"
#endif /* CONFIG_HL78XX_GNSS */
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
void event_dispatcher_dispatch(struct hl78xx_evt *notif)
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
	case MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT:
		return "run pmc cfg script";
	case MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT:
		return "run enable gprs script";
	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		return "await registered";
	case MODEM_HL78XX_STATE_CARRIER_ON:
		return "carrier on";
	case MODEM_HL78XX_STATE_FOTA:
		return "fota";
	case MODEM_HL78XX_STATE_CARRIER_OFF:
		return "carrier off";
	case MODEM_HL78XX_STATE_SIM_POWER_OFF:
		return "sim power off";
	case MODEM_HL78XX_STATE_SLEEP:
		return "sleep";
	case MODEM_HL78XX_STATE_AIRPLANE:
		return "airplane mode";
#ifdef CONFIG_HL78XX_GNSS
	case MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT:
		return "run gnss init script";
#endif /* CONFIG_HL78XX_GNSS */
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
#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN
	case MODEM_HL78XX_EVENT_NTN_POSREQ:
		return "ntn posreq";
#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */
	case MODEM_HL78XX_EVENT_PHONE_FUNCTIONALITY_CHANGED:
		return "phone functionality changed";
#ifdef CONFIG_HL78XX_GNSS
	case MODEM_HL78XX_EVENT_GNSS_START_REQUESTED:
		return "gnss event start requested";
	case MODEM_HL78XX_EVENT_GNSS_SEARCH_STARTED:
		return "gnss search started";
	case MODEM_HL78XX_EVENT_GNSS_SEARCH_STARTED_FAILED:
		return "gnss search started failed";
	case MODEM_HL78XX_EVENT_GNSS_FIX_ACQUIRED:
		return "gnss fix acquired";
	case MODEM_HL78XX_EVENT_GNSS_FIX_LOST:
		return "gnss fix lost";
	case MODEM_HL78XX_EVENT_GNSS_STOP_REQUESTED:
		return "gnss stop requested";
	case MODEM_HL78XX_EVENT_GNSS_STOPPED:
		return "gnss stopped";
	case MODEM_HL78XX_EVENT_GNSS_MODE_ENTER_REQUESTED:
		return "gnss mode enter requested";
	case MODEM_HL78XX_EVENT_GNSS_MODE_EXIT_REQUESTED:
		return "gnss mode exit requested";
#endif /* CONFIG_HL78XX_GNSS */
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	case MODEM_HL78XX_EVENT_DEVICE_ASLEEP:
		return "device asleep";
	case MODEM_HL78XX_EVENT_DEVICE_AWAKE:
		return "device awake";
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
	case MODEM_HL78XX_EVENT_WDSI_UPDATE:
		return "wdsi update";
	case MODEM_HL78XX_EVENT_WDSI_RESTART:
		return "wdsi restart";
	case MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_REQUEST:
		return "wdsi download request";
	case MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_PROGRESS:
		return "wdsi download progress";
	case MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_COMPLETE:
		return "wdsi download complete";
	case MODEM_HL78XX_EVENT_WDSI_INSTALL_REQUEST:
		return "wdsi install request";
	case MODEM_HL78XX_EVENT_WDSI_INSTALLING_FIRMWARE:
		return "wdsi installing firmware";
	case MODEM_HL78XX_EVENT_WDSI_FIRMWARE_INSTALL_SUCCEEDED:
		return "wdsi firmware install succeeded";
	case MODEM_HL78XX_EVENT_WDSI_FIRMWARE_INSTALL_FAILED:
		return "wdsi firmware install failed";
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
	case MODEM_HL78XX_EVENT_MDM_RESTART:
		return "modem unexpected restart";
	case MODEM_HL78XX_EVENT_AT_CMD_TIMEOUT:
		return "AT command timeout";
	default:
		return "unknown event";
	}

	return "";
}

static void hl78xx_log_event(enum hl78xx_event evt)
{
	LOG_DBG("event %s", hl78xx_event_str(evt));
}

void hl78xx_start_timer(struct hl78xx_data *data, k_timeout_t timeout)
{
	k_work_schedule(&data->timeout_work, timeout);
}

void hl78xx_stop_timer(struct hl78xx_data *data)
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
/* +CEREG/+CREG: */
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
	LOG_DBG("Received %s URC with argc: %d", argv[0], argc);
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
#if !defined(CONFIG_MODEM_HL78XX_EDRX)
		return;
#endif
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
#if defined(CONFIG_MODEM_HL78XX_POWER_DOWN) &&                                                     \
	defined(CONFIG_MODEM_HL78XX_USE_ACTIVE_TIME_BASED_POWER_DOWN)
		hl78xx_power_down_feed_timer(data, 0);
#endif
#if defined(CONFIG_MODEM_HL78XX_EDRX)
		hl78xx_edrx_idle_feed_timer(data, 0);
#endif
#if defined(CONFIG_HL78XX_GNSS) && defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE)
		/*
		 * When GNSS is enabled, +KCELLMEAS must be disabled because
		 * it triggers LTE activity that blocks GNSS searches (LTE has
		 * higher priority than GNSS on the shared RF path).
		 *
		 * For HL7812 without PSM, use +CEREG as the "ready for data"
		 * signal instead and release socket comms here.
		 */
#if !defined(CONFIG_MODEM_HL78XX_00) && !defined(CONFIG_MODEM_HL78XX_PSM)
		hl78xx_release_socket_comms(data);
#endif /* !CONFIG_MODEM_HL78XX_00 && !CONFIG_MODEM_HL78XX_PSM */
#endif /* CONFIG_HL78XX_GNSS && CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	} else {
		data->status.registration.is_registered_currently = false;
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEREGISTERED);
#if defined(CONFIG_MODEM_HL78XX_EDRX)
		if (hl78xx_is_edrx_idle_scheduled(data)) {
			hl78xx_edrx_idle_cancel(data);
		}
#endif
	}
	event_dispatcher_dispatch(&event);
}

void hl78xx_on_ksup(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int module_status;
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_evt event = {.type = HL78XX_LTE_MODEM_STARTUP};

	if (argc != 2) {
		return;
	}
	module_status = ATOI(argv[1], 0, "module_status");
	data->status.boot.status = module_status;
	/* Check for unexpected restart */
	if (data->status.boot.is_booted_previously == true &&
	    module_status == (int)HL78XX_MODULE_READY) {
#if defined(CONFIG_MODEM_HL78XX_00) && defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE)
/* HL7800 generates +KSUP on PSM/eDRX exit.
 * this KSUP is informational. AT settings are preserved except KCNXCFG, DNS setup and sockets,
 * the modem just needs to camp on a cell (+KCELLMEAS) before data transmission.
 *
 * GPIO6 is the primary wake indicator.
 */
#ifdef CONFIG_HL78XX_GNSS
		if (data->status.state == MODEM_HL78XX_STATE_SLEEP) {
			if (hl78xx_gnss_is_pending(data)) {
				hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
			}
		} else if (data->status.state == MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_AWAKE);
		} else {

#endif /* CONFIG_HL78XX_GNSS */
			if (data->status.state == MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT ||
			    data->status.state == MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT) {
				/* KSUP during RAT_CFG or PMC_CFG state: these states
				 * explicitly send AT+CFUN=4,1 and are waiting for the
				 * modem to reboot to complete configuration.
				 */
				LOG_DBG("KSUP after config restart (state=%d) - "
					"dispatching MDM_RESTART",
					data->status.state);
				hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_MDM_RESTART);
			} else {
				/* All other states: KSUP is informational. HL7800 generates KSUP on
				 * PSM/eDRX exit so this makes harder to detect unexpected restart
				 * and react. Just log it now.
				 */
				LOG_DBG("KSUP (state=%d) - informational, "
					"init already running or PSM/eDRX exit",
					data->status.state);
			}
#ifdef CONFIG_HL78XX_GNSS
		}
#endif /* CONFIG_HL78XX_GNSS */
#else
		LOG_DBG("Modem unexpected restart detected %d", module_status);
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_MDM_RESTART);
#endif /* CONFIG_MODEM_HL78XX_00 && CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	} else if (data->status.boot.is_booted_previously == true &&
		   module_status != (int)HL78XX_MODULE_READY) {
		LOG_DBG("Modem failed to start %d", module_status);
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SUSPEND);
	} else {
		data->status.boot.is_booted_previously = true;
		LOG_DBG("Modem started successfully %d %d", module_status,
			data->status.boot.is_booted_previously);
	}
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

void hl78xx_on_serial_number(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
	HL78XX_LOG_DBG("Serial Number: %s %s", argv[0], argv[1]);
	k_mutex_lock(&data->api_lock, K_FOREVER);
	safe_strncpy((char *)data->identity.serial_number, argv[1],
		     sizeof(data->identity.serial_number));
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

void hl78xx_on_cgact(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	int act_status = -1;
	int cid = -1;

	if (argc != 3) {
		return;
	}
	cid = ATOI(argv[1], -1, "cid");
	act_status = ATOI(argv[2], -1, "act_status");
	if (cid == -1 || act_status == -1 || cid > CONFIG_MODEM_HL78XX_MAX_PDP_CONTEXTS) {
		/* Invalid parameters */
		return;
	}

	data->status.gprs[cid - 1].is_active = (act_status == 1) ? true : false;
	data->status.gprs[cid - 1].cid = cid;
	HL78XX_LOG_DBG("CGACT: %s %s", argv[0], argv[2]);
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
		data->kselacq_data.mode = false;
		data->kselacq_data.rat1 = ATOI(argv[1], 0, "rat1");
		data->kselacq_data.rat2 = ATOI(argv[2], 0, "rat2");
		data->kselacq_data.rat3 = ATOI(argv[3], 0, "rat3");
	} else {
		data->kselacq_data.mode = false;
		data->kselacq_data.rat1 = 0;
		data->kselacq_data.rat2 = 0;
		data->kselacq_data.rat3 = 0;
	}
}
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
#ifdef CONFIG_MODEM_HL78XX_PSM
#ifdef CONFIG_MODEM_HL78XX_12
void hl78xx_on_psmev(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	struct hl78xx_evt event = {.type = HL78XX_LTE_PSMEV_UPDATE};

	if (argc < 2) {
		return;
	}

	data->status.psmev.previous = data->status.psmev.current;
	data->status.psmev.current = ATOI(argv[1], 0, "psmev");
	event.content.psm_event = data->status.psmev.current;

	event_dispatcher_dispatch(&event);

	if (data->status.psmev.current == HL78XX_PSM_EVENT_ENTER) {
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
			LOG_DBG("Set WAKE pin to 0");
		}
		if (data->status.state != MODEM_HL78XX_STATE_SLEEP &&
		    data->status.state != MODEM_HL78XX_STATE_IDLE) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_ASLEEP);
		}
	} else if (data->status.psmev.current == HL78XX_PSM_EVENT_EXIT) {
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
			LOG_DBG("Set WAKE pin to 1");
		}
	} else {
		LOG_DBG("Unknown PSM event value: %d", data->status.psmev.current);
	}
}
#endif /* CONFIG_MODEM_HL78XX_12 */
#endif /* CONFIG_MODEM_HL78XX_PSM */

void hl78xx_on_cpsms(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc > 3) {
		HL78XX_LOG_DBG("%d %d [%s] [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1],
			       argv[4], argv[5]);
		data->status.pmc_cpsms.mode = ATOI(argv[1], 0, "mode");
		int8_t active_time = binary_str_to_byte(argv[5]);
		int8_t periodic_tau = binary_str_to_byte(argv[4]);

		data->status.pmc_cpsms.active_time = (active_time == -EINVAL) ? 0 : active_time;
		data->status.pmc_cpsms.periodic_tau = (periodic_tau == -EINVAL) ? 0 : periodic_tau;
	} else {
		data->status.pmc_cpsms.mode = ATOI(argv[1], 0, "mode");
		data->status.pmc_cpsms.active_time = 0;
		data->status.pmc_cpsms.periodic_tau = 0;
	}
}

#if defined(CONFIG_MODEM_HL78XX_00) && defined(CONFIG_HL78XX_GNSS)
void hl78xx_on_rrc_status(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 2 || !argv[1]) {
		LOG_WRN("RRC status: invalid response");
		return;
	}

	data->status.rrc_idle = (strcmp(argv[1], "IDLE") == 0);
	LOG_INF("RRC status: %s (idle=%d)", argv[1], data->status.rrc_idle);
}
#endif /* CONFIG_MODEM_HL78XX_00 && CONFIG_HL78XX_GNSS */

void hl78xx_on_kcellmeas(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_evt event = {.type = HL78XX_CELLMEAS_UPDATE};

	if (argc < 2) {
		return;
	}
	LOG_DBG("%d argc: %d", __LINE__, argc);
	if (argc == 2) {
		data->status.kcellmeas_timeout = ATOI(argv[1], 0, "kcellmeas");
		return;
	}
	/* If there are more parameters, it means the modem is configured to report more
	 * detailed cell measurement results. In this case, we can consider the modem is
	 * active and in good signal condition.
	 */
	if (argc < 5) {
		return;
	}
	HL78XX_LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
	data->status.rsrp = (int)ATOD(argv[1], 0, "rsrp");
	if (hl78xx_is_rsrp_valid(data)) {
		data->status.registration.is_registered_currently = true;
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
#ifdef CONFIG_MODEM_HL78XX_12
		/* HL7812: +KCELLMEAS is the authoritative "ready for data" signal.
		 * Release the socket semaphore so blocked operations can proceed.
		 * For HL7800, the semaphore is released later by the CARRIER_ON
		 * TIMEOUT handler after KCNXCFG and DNS setup are complete.
		 */
		hl78xx_release_socket_comms(data);
#endif /* CONFIG_MODEM_HL78XX_12 */
	} else {
		LOG_DBG("Invalid RSRP value: %d", data->status.rsrp);
	}
	event.content.cellmeas.rsrp = data->status.rsrp;
	event_dispatcher_dispatch(&event);
}
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

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
#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN

void hl78xx_on_kntncfg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 2) {
		return;
	}
	safe_strncpy((char *)data->status.ntn_rat.pos_mode, argv[1],
		     sizeof(data->status.ntn_rat.pos_mode));
	data->status.ntn_rat.is_dynamic = ATOI(argv[2], 0, "is_dynamic");
}

void hl78xx_on_kntn_posreq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 1) {
		return;
	}
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_NTN_POSREQ);
}

#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */

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
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_PHONE_FUNCTIONALITY_CHANGED);
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

#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE

void hl78xx_on_wdsi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_evt event = {.type = HL78XX_LTE_FOTA_UPDATE_STATUS};
	int wsi_status = -1;
	uint32_t wsi_data = 0;

	if (argc < 2) {
		return;
	}
	wsi_status = ATOI(argv[1], -1, "wdsi");
	if (wsi_status == -1) {
		return;
	}
	data->status.wdsi.level = wsi_status;
	if (argc == 3) {
		wsi_data = ATOI(argv[2], 0, "data");
		data->status.wdsi.data = wsi_data;
	}
	if (data->status.wdsi.level == WDSI_FIRMWARE_AVAILABLE) {
		data->status.wdsi.fota_size = wsi_data;
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_UPDATE);
	} else if (data->status.wdsi.level == WDSI_BOOTSTRAP_CREDENTIALS_PRESENT) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_RESTART);
	} else if (data->status.wdsi.level == WDSI_FIRMWARE_DOWNLOAD_REQUEST) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_REQUEST);
	} else if (data->status.wdsi.level == WDSI_DOWNLOAD_IN_PROGRESS) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_PROGRESS);
		data->status.wdsi.progress = wsi_data;
	} else if (data->status.wdsi.level == WDSI_FIRMWARE_DOWNLOADED) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_COMPLETE);
	} else if (data->status.wdsi.level == WDSI_FIRMWARE_INSTALL_REQUEST) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_INSTALL_REQUEST);
	} else if (data->status.wdsi.level == WDSI_FIRMWARE_UPDATE_START) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_INSTALLING_FIRMWARE);
	} else if (data->status.wdsi.level == WDSI_FIRMWARE_UPDATE_SUCCESS) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_FIRMWARE_INSTALL_SUCCEEDED);
	} else if (data->status.wdsi.level == WDSI_FIRMWARE_UPDATE_FAILED) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_FIRMWARE_INSTALL_FAILED);
	} else {
		/* other WDSI levels during FOTA can be handled here if needed */
	}
	if ((data->status.wdsi.level == WDSI_DOWNLOAD_IN_PROGRESS &&
	     (data->status.wdsi.progress == 0 || data->status.wdsi.progress == 100)) ||
	    data->status.wdsi.level != WDSI_DOWNLOAD_IN_PROGRESS) {
		event.content.wdsi_indication = data->status.wdsi.level;
		event_dispatcher_dispatch(&event);
	}
	HL78XX_LOG_DBG("WDSI: %d %d", wsi_status, wsi_data);
}

#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
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
int modem_dynamic_cmd_send(struct hl78xx_data *data,
			modem_chat_script_callback script_user_callback, const uint8_t *cmd,
			uint16_t cmd_size, const struct modem_chat_match *response_matches,
			uint16_t matches_size, uint16_t response_timeout, bool user_cmd)
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
		.timeout = 0, /* Has no effect */
	};
	struct modem_chat_script chat_script = {
		.name = "dynamic_script",
		.script_chats = &dynamic_script,
		.script_chats_size = 1,
		.abort_matches = hl78xx_get_abort_matches(),
		.abort_matches_size = hl78xx_get_abort_matches_size(),
		.callback = script_user_callback,
		.timeout = response_timeout, /* overall script timeout */
	};
#if defined(CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN)
	if (hl78xx_power_down_is_ignoring_feeding(data) == false) {
		hl78xx_power_down_feed_timer(data, response_timeout);
	} else {
		hl78xx_power_down_allow_feeding(data);
	}
#endif /* CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN */
#if defined(CONFIG_MODEM_HL78XX_EDRX)
	if (hl78xx_edrx_idle_is_ignoring_feeding(data) == false) {
		hl78xx_edrx_idle_feed_timer(data, response_timeout);
	} else {
		hl78xx_edrx_idle_allow_feeding(data);
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */
	ret = k_mutex_lock(&data->tx_lock, K_NO_WAIT);
	if (ret < 0) {
		if (user_cmd == false) {
			errno = -ret;
		}
		LOG_DBG("Failed to acquire tx_lock for sending command: %d", ret);
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

int modem_dynamic_cmd_send_async(struct hl78xx_data *data,
			modem_chat_script_callback script_user_callback, const uint8_t *cmd,
			uint16_t cmd_size, const struct modem_chat_match *response_matches,
			uint16_t matches_size, uint16_t response_timeout, bool user_cmd)
{
	int ret = 0;
	int script_ret = 0;

	if (data == NULL) {
		LOG_ERR("%d %s Invalid parameter", __LINE__, __func__);
		errno = EINVAL;
		return -1;
	}
	ret = k_mutex_lock(&data->tx_lock, K_NO_WAIT);
	if (ret < 0) {
		if (user_cmd == false) {
			errno = -ret;
		}
		return -1;
	}
	LOG_DBG("Executing async command: %.*s", cmd_size, cmd);
	/* prepare the dynamic script */
	data->buffers.cmd_len = snprintf(data->buffers.cmd_buffer, sizeof(data->buffers.cmd_buffer),
				      "%.*s", cmd_size, cmd);
	data->dynamic_chat.response_matches = response_matches;
	data->dynamic_chat.response_matches_size = matches_size;
	data->dynamic_chat.timeout = 0; /* Has no effect */
	data->dynamic_script.name = "dynamic_script";
	data->dynamic_script.script_chats = &data->dynamic_chat;
	data->dynamic_script.script_chats_size = 1;
	data->dynamic_script.abort_matches = hl78xx_get_abort_matches();
	data->dynamic_script.abort_matches_size = hl78xx_get_abort_matches_size();
	data->dynamic_script.callback = script_user_callback;
	data->dynamic_script.timeout = response_timeout; /* overall script timeout */
	data->dynamic_chat.request = data->buffers.cmd_buffer;
	data->dynamic_chat.request_size = data->buffers.cmd_len;
	/* run the chat script */
	script_ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
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
	bool pin_state;

	if (spec == NULL || spec->port == NULL) {
		LOG_ERR("GPIO6 GPIO spec is not configured properly");
		return;
	}
	if (!(pins & BIT(spec->pin))) {
		return; /*  not our pin */
	}
	pin_state = gpio_pin_get_dt(spec);

	LOG_DBG("GPIO6 ISR callback %s %d %d", spec->port->name, spec->pin, pin_state);

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
#ifdef CONFIG_MODEM_HL78XX_12
	if (!pin_state) {
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
		data->status.power_down.previous = data->status.power_down.current;
		if (data->status.power_down.is_power_down_requested) {
			data->status.power_down.current = POWER_DOWN_EVENT_ENTER;
		}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#ifdef CONFIG_MODEM_HL78XX_EDRX
		data->status.edrxev.previous = data->status.edrxev.current;
		data->status.edrxev.current = HL78XX_EDRX_EVENT_IDLE_ENTER;
		data->status.edrxev.is_edrx_idle_requested = false;

		struct hl78xx_evt edrx_evt = {.type = HL78XX_EDRX_IDLE_UPDATE,
					      .content.edrx_event = data->status.edrxev.current};

		event_dispatcher_dispatch(&edrx_evt);
#endif /* CONFIG_MODEM_HL78XX_EDRX */
		if (data->status.state != MODEM_HL78XX_STATE_SLEEP &&
		    data->status.state != MODEM_HL78XX_STATE_IDLE) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_ASLEEP);
		}
	} else {
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
		data->status.power_down.previous = data->status.power_down.current;
		if (data->status.power_down.is_power_down_requested) {
			data->status.power_down.current = POWER_DOWN_EVENT_EXIT;
			LOG_DBG("GPIO6 indicates wake, set power down event to EXIT");
		}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#ifdef CONFIG_MODEM_HL78XX_EDRX
		data->status.edrxev.previous = data->status.edrxev.current;
		data->status.edrxev.current = HL78XX_EDRX_EVENT_IDLE_EXIT;
		data->status.edrxev.is_edrx_idle_requested = false;

		struct hl78xx_evt edrx_evt = {.type = HL78XX_EDRX_IDLE_UPDATE,
					      .content.edrx_event = data->status.edrxev.current};

		event_dispatcher_dispatch(&edrx_evt);
#endif /* CONFIG_MODEM_HL78XX_EDRX */
		/* Only dispatch wake event if currently in sleep state */
		if (data->status.state == MODEM_HL78XX_STATE_SLEEP) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_AWAKE);
		}
	}
#endif /* CONFIG_MODEM_HL78XX_12 */
#ifdef CONFIG_MODEM_HL78XX_00
	/* HL7800: GPIO6 is the authoritative indicator for sleep/wake transitions.
	 * Unlike HL7812, the HL7800 does not generate +KPSMEV or +KEDRXEV URCs.
	 * GPIO6 LOW = modem entering sleep, GPIO6 HIGH = modem waking up.
	 * We drive the state machine directly from this ISR.
	 */
	if (!pin_state) {
		/* GPIO6 LOW: modem is entering sleep */
#ifdef CONFIG_MODEM_HL78XX_PSM
		data->status.psmev.previous = data->status.psmev.current;
		data->status.psmev.current = HL78XX_PSM_EVENT_ENTER;

		struct hl78xx_evt psm_evt = {.type = HL78XX_LTE_PSMEV_UPDATE,
					     .content.psm_event = data->status.psmev.current};

		event_dispatcher_dispatch(&psm_evt);

#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
		data->status.power_down.previous = data->status.power_down.current;
		if (data->status.power_down.is_power_down_requested) {
			data->status.power_down.current = POWER_DOWN_EVENT_ENTER;
		}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#ifdef CONFIG_MODEM_HL78XX_EDRX
		data->status.edrxev.previous = data->status.edrxev.current;
		if (data->status.edrxev.is_edrx_idle_requested) {
			data->status.edrxev.current = HL78XX_EDRX_EVENT_IDLE_ENTER;
		}

		struct hl78xx_evt edrx_evt = {.type = HL78XX_EDRX_IDLE_UPDATE,
					      .content.edrx_event = data->status.edrxev.current};

		event_dispatcher_dispatch(&edrx_evt);

#endif /* CONFIG_MODEM_HL78XX_EDRX */
		/* Dispatch sleep event to drive state machine into SLEEP */
		if (data->status.state != MODEM_HL78XX_STATE_SLEEP &&
		    data->status.state != MODEM_HL78XX_STATE_IDLE) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_ASLEEP);
		}
	} else {
		/* GPIO6 HIGH: modem is waking up */
#ifdef CONFIG_MODEM_HL78XX_PSM
		data->status.psmev.previous = data->status.psmev.current;
		data->status.psmev.current = HL78XX_PSM_EVENT_EXIT;

		struct hl78xx_evt psm_evt = {.type = HL78XX_LTE_PSMEV_UPDATE,
					     .content.psm_event = data->status.psmev.current};

		event_dispatcher_dispatch(&psm_evt);

#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
		data->status.power_down.previous = data->status.power_down.current;
		if (data->status.power_down.is_power_down_requested) {
			data->status.power_down.current = POWER_DOWN_EVENT_EXIT;
			LOG_DBG("GPIO6 indicates wake, set power down event to EXIT");
		}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#ifdef CONFIG_MODEM_HL78XX_EDRX
		data->status.edrxev.previous = data->status.edrxev.current;
		data->status.edrxev.current = HL78XX_EDRX_EVENT_IDLE_EXIT;
		if (data->status.edrxev.is_edrx_idle_requested) {
			data->status.edrxev.is_edrx_idle_requested = false;
		}

		struct hl78xx_evt edrx_evt = {.type = HL78XX_EDRX_IDLE_UPDATE,
					      .content.edrx_event = data->status.edrxev.current};

		event_dispatcher_dispatch(&edrx_evt);

#endif /* CONFIG_MODEM_HL78XX_EDRX */
		if (data->status.state == MODEM_HL78XX_STATE_SLEEP) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_AWAKE);
		}
	}
#endif /* CONFIG_MODEM_HL78XX_00 */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
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
		modem_pipe_attach(data->uart_pipe, hl78xx_bus_pipe_handler, data);
		modem_pipe_open_async(data->uart_pipe);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	case MODEM_HL78XX_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
#ifdef CONFIG_MODEM_HL78XX_AUTOBAUD_AT_BOOT
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_SET_BAUDRATE);
#else
		hl78xx_run_post_restart_script_async(data);
#endif /* CONFIG_MODEM_HL78XX_AUTOBAUD_AT_BOOT */
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		(void)hl78xx_get_uart_config(data);
		LOG_DBG("Current baudrate after post-restart script: %d",
			data->status.uart.current_baudrate);
#if defined(CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL) ||                                    \
	!defined(CONFIG_MODEM_HL78XX_AUTO_BAUDRATE)
#ifdef CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
#else
		/* Leave the modem in airplane mode, the caller will enable it later */
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AIRPLANE);
#endif /* CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE */
#else
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_SET_BAUDRATE);
#endif /* CONFIG_MODEM_HL78XX_AUTOBAUD_ONLY_IF_COMMS_FAIL */
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_SET_BAUDRATE);
#else
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT);
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */
	case MODEM_HL78XX_EVENT_AT_CMD_TIMEOUT:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT);
		break;

	default:
		break;
	}
}

#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
static int hl78xx_on_set_baudrate_state_enter(struct hl78xx_data *data)
{
	int ret;

	data->status.uart.target_baudrate = CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_VALUE;

	/* Detect current baud rate */
	ret = hl78xx_detect_current_baudrate(data);
	if (ret < 0) {
		LOG_ERR("Baud rate detection failed");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_FAILED);
		return ret;
	}

	/* Switch to target baud rate if different */
	ret = hl78xx_switch_baudrate(data, data->status.uart.target_baudrate);
	if (ret < 0) {
		LOG_ERR("Failed to switch baud rate: %d", ret);
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_FAILED);
		return ret;
	}
	data->status.boot.is_booted_previously = true;
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_SUCCESS);
	return 0;
}

static void hl78xx_set_baudrate_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		/* Increment retry counter */
		data->status.uart.baudrate_detection_retry++;

		/* Retry detection or give up */
		if (data->status.uart.baudrate_detection_retry <
		    CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT) {
			LOG_WRN("Retrying baud rate detection (attempt %d/%d)",
				data->status.uart.baudrate_detection_retry + 1,
				CONFIG_MODEM_HL78XX_AUTOBAUD_RETRY_COUNT);
			k_sleep(K_MSEC(500));
			hl78xx_on_set_baudrate_state_enter(data);
		} else {
			LOG_ERR("Baud rate configuration failed after %d attempts",
				data->status.uart.baudrate_detection_retry);
			hl78xx_enter_state(data,
					   MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT);
		}
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	default:
		break;
	}
}
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */

static int hl78xx_on_run_init_script_state_enter(struct hl78xx_data *data)
{
	return hl78xx_run_init_script_async(data);
}

static void hl78xx_run_init_script_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
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
		LOG_ERR("Modem initialization failed after diagnostic script");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;
	case MODEM_HL78XX_EVENT_AT_CMD_TIMEOUT:
#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_SET_BAUDRATE);
#else
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_POWER_ON_PULSE);
			break;
		}

		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_RESET_PULSE);
			break;
		}

		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */
		break;
	case MODEM_HL78XX_EVENT_BUS_CLOSED:
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		if (!hl78xx_gpio_is_enabled(&config->mdm_gpio_wake) &&
		    IS_ENABLED(CONFIG_MODEM_HL78XX_LOW_POWER_MODE)) {
			LOG_ERR("The modem wake pin is not enabled. Make sure that modem low-power "
				"mode is disabled. If you’re unsure, enable it by adding the "
				"corresponding DTS configuration entry.");
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
#ifdef CONFIG_HL78XX_GNSS
	/* Disable kcellmeas */
	const uint16_t ACQUIRE_TIMEOUT_S = 0;
#else
	const uint16_t ACQUIRE_TIMEOUT_S = 35;
#endif /* CONFIG_HL78XX_GNSS */
	ret = hl78xx_enable_lte_coverage_urc(data, &modem_require_restart, ACQUIRE_TIMEOUT_S);
	if (ret < 0) {
		goto error;
	}
	ret = hl78xx_rat_cfg(data, &modem_require_restart, &rat_config_request);
	if (ret < 0) {
		goto error;
	}

	ret = hl78xx_band_cfg(data, &modem_require_restart, rat_config_request);
	if (ret < 0) {
		goto error;
	}

#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN

	ret = hl78xx_rat_ntn_cfg(data, &modem_require_restart, rat_config_request);
	if (ret < 0) {
		goto error;
	}

#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */
	if (modem_require_restart) {
		HL78XX_LOG_DBG("Modem restart required to apply new RAT/Band settings");
		ret = modem_dynamic_cmd_send(data, NULL, cmd_restart, strlen(cmd_restart),
					     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
					     MDM_CMD_TIMEOUT, false);
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

static int hl78xx_on_rat_cfg_script_state_leave(struct hl78xx_data *data)
{
	hl78xx_stop_timer(data);
	return 0;
}

static void hl78xx_run_rat_cfg_script_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
		if (IS_ENABLED(CONFIG_MODEM_HL78XX_AUTOBAUD_CHANGE_PERSISTENT) == false) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_SET_BAUDRATE);
			break;
		}
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */
		break;
	case MODEM_HL78XX_EVENT_MDM_RESTART:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
		break;
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT);
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

static int hl78xx_on_pmc_cfg_script_state_enter(struct hl78xx_data *data)
{
	int ret = 0;
	bool modem_require_restart = false;

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	hl78xx_enable_pmc(data);
	hl78xx_psm_settings(data);
	hl78xx_edrx_settings(data);
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	hl78xx_power_down_settings(data);
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#else
	hl78xx_disable_pmc(data);
	LOG_DBG("%d Disabling Power Management Config", __LINE__);
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	if (modem_require_restart) {
		const char *cmd_restart = (const char *)SET_AIRPLANE_MODE_CMD;

		LOG_DBG("%d Reset required", __LINE__);

		ret = modem_dynamic_cmd_send(data, NULL, cmd_restart, strlen(cmd_restart),
					     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
					     MDM_CMD_TIMEOUT, false);
		if (ret < 0) {
			goto error;
		}
		hl78xx_start_timer(data, K_MSEC(100));
		return 0;
	}
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	hl78xx_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_SUCCESS, data);
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	return 0;
error:
	hl78xx_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_ABORT, data);
	LOG_ERR("Failed to send command: %d", ret);
	return ret;
}

static void hl78xx_run_pmc_cfg_script_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		LOG_DBG("Rebooting modem to apply new RAT settings");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_REQUIRE_RESTART);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;
	case MODEM_HL78XX_EVENT_SCRIPT_REQUIRE_RESTART:
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
	default:
		break;
	}
}

static int hl78xx_on_run_pmc_cfg_script_state_leave(struct hl78xx_data *data)
{
	return 0;
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
		/* autodetect APN from IMSI
		 * the list of SIM profiles. Global scope, so the app can change it
		 * AT+CCID or AT+CIMI needs to be run here if it is not ran in the init
		 * script
		 */
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
	ret = hl78xx_api_func_set_phone_functionality(data->dev, HL78XX_FULLY_FUNCTIONAL, false);
	if (ret) {
		goto error;
	}

	modem_dynamic_cmd_send(data, NULL, (const char *)GET_FULLFUNCTIONAL_MODE_CMD,
			       strlen(GET_FULLFUNCTIONAL_MODE_CMD), hl78xx_get_ok_match(),
			       hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);

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
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
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
#if defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE)
#ifdef CONFIG_MODEM_HL78XX_00
#ifdef CONFIG_MODEM_HL78XX_EDRX
	if (data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_ENTER ||
	    data->status.edrxev.is_edrx_idle_requested) {
		LOG_INF("eDRX wake: waiting for modem to become responsive");
		hl78xx_start_timer(data, K_SECONDS(3));
		return 0;
	}
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#ifdef CONFIG_MODEM_HL78XX_PSM
	/* wake the LTE layer so the modem can re-register */
	if (data->status.psmev.current != HL78XX_PSM_EVENT_NONE && IS_ENABLED(CONFIG_HL78XX_GNSS)) {
		modem_dynamic_cmd_send(data, NULL, (const char *)WAKE_LTE_LAYER_CMD,
				       strlen(WAKE_LTE_LAYER_CMD), NULL, 0, MDM_CMD_TIMEOUT, false);
	}
#endif /* CONFIG_MODEM_HL78XX_PSM */
	if (hl78xx_is_registered(data)) {
		LOG_INF("Already registered on entry - proceeding to CARRIER_ON");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
		return 0;
	}
#else /* CONFIG_MODEM_HL78XX_12 */
#ifdef CONFIG_MODEM_HL78XX_PSM
	LOG_DBG("PSM event: previous=%d current=%d", data->status.psmev.previous,
		data->status.psmev.current);

	if (hl78xx_psm_is_active(data) && IS_ENABLED(CONFIG_HL78XX_GNSS)) {
		modem_dynamic_cmd_send(data, NULL, (const char *)WAKE_LTE_LAYER_CMD,
				       strlen(WAKE_LTE_LAYER_CMD), NULL, 0, MDM_CMD_TIMEOUT, false);
		return 0;
	}
#endif /* CONFIG_MODEM_HL78XX_PSM */
#endif /* CONFIG_MODEM_HL78XX_00 */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	if (hl78xx_is_registered(data) && IS_ENABLED(CONFIG_MODEM_HL78XX_EDRX) &&
	    !IS_ENABLED(CONFIG_MODEM_HL78XX_PSM) && !IS_ENABLED(CONFIG_MODEM_HL78XX_00)) {
		modem_dynamic_cmd_send(data, NULL, (const char *)CHECK_LTE_COVERAGE_CMD,
				       strlen(CHECK_LTE_COVERAGE_CMD), hl78xx_get_ok_match(),
				       hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
	}
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
#if defined(CONFIG_MODEM_HL78XX_00) && defined(CONFIG_MODEM_HL78XX_EDRX)
		if (hl78xx_is_registered(data) &&
		    (data->status.edrxev.previous == HL78XX_EDRX_EVENT_IDLE_ENTER ||
		     data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_ENTER)) {
			LOG_INF("eDRX wake settling complete - proceeding to CARRIER_ON");
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_CARRIER_ON);
			break;
		}
#endif /* CONFIG_MODEM_HL78XX_00 && CONFIG_MODEM_HL78XX_EDRX */
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

	case MODEM_HL78XX_EVENT_MDM_RESTART:
		LOG_DBG("Modem restart detected %d", __LINE__);
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_REGISTERED:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_CARRIER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;
#ifdef CONFIG_MODEM_HL78XX_RAT_NBNTN
#ifdef CONFIG_NTN_POSITION_SOURCE_MANUAL
	case MODEM_HL78XX_EVENT_NTN_POSREQ:
		hl78xx_run_ntn_pos_script_async(data);
		break;
#endif /* CONFIG_NTN_POSITION_SOURCE_MANUAL */
#endif /* CONFIG_MODEM_HL78XX_RAT_NBNTN */
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	case MODEM_HL78XX_EVENT_DEVICE_ASLEEP:
		/* Modem entered PSM before we got registered (e.g. T3324 expired) */
		LOG_DBG("Modem sleeping before registration complete");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_SLEEP);
		break;
	case MODEM_HL78XX_EVENT_DEVICE_AWAKE:
		LOG_DBG("Modem woke up but still waiting for registration");
		/* Stay in AWAIT_REGISTERED and wait for the next URC
		 * to determine the registration status
		 */
		if (IS_ENABLED(CONFIG_MODEM_HL78XX_00) && IS_ENABLED(CONFIG_MODEM_HL78XX_EDRX)) {
			hl78xx_run_periodic_script_async(data);
		}
		break;
	case MODEM_HL78XX_EVENT_RESUME:
		bool match = false;

#ifdef CONFIG_MODEM_HL78XX_PSM
		match = match || (data->status.psmev.current == HL78XX_PSM_EVENT_ENTER);
#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
		match = match || (data->status.power_down.current == POWER_DOWN_EVENT_ENTER);
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#ifdef CONFIG_MODEM_HL78XX_EDRX
		match = match || (data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_ENTER);
#endif /* CONFIG_MODEM_HL78XX_EDRX */
		if (match) {
			LOG_DBG("Modem is sleeping, transitioning to SLEEP for wakeup");
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_SLEEP);
			/* Re-queue RESUME so the sleep handler processes the wakeup */
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
		}
		break;
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

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
#ifdef CONFIG_HL78XX_GNSS
	/* Check and process any pending GNSS mode entry request */
	if (hl78xx_gnss_is_pending(data)) {
#ifdef CONFIG_MODEM_HL78XX_00
		LOG_INF("HL7800 GNSS pending - routing through carrier_off/airplane");
		notif_carrier_on(data->dev);
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_GNSS_MODE_ENTER_REQUESTED);
		return 0;
#else
		LOG_INF("Processing pending GNSS mode request (queued before modem ready)");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
		return 0;
#endif /* CONFIG_MODEM_HL78XX_00 */
	}
#endif /* CONFIG_HL78XX_GNSS */

#ifdef CONFIG_MODEM_HL78XX_RAT_GSM
	/* Activate the PDP context */
	int ret = hl78xx_gsm_pdp_activate(data);

	if (ret) {
		LOG_ERR("Failed to activate PDP context: %d", ret);
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_FAILED);
		return ret;
	}
#endif /* CONFIG_MODEM_HL78XX_RAT_GSM */
	notif_carrier_on(data->dev);
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	bool is_lpm = false;

#ifdef CONFIG_MODEM_HL78XX_PSM
	LOG_DBG("PSMEV previous: %d, current: %d", data->status.psmev.previous,
		data->status.psmev.current);

	is_lpm = is_lpm || ((data->status.psmev.previous == HL78XX_PSM_EVENT_NONE &&
			     data->status.psmev.current == HL78XX_PSM_EVENT_NONE) &&
			    data->status.registration.network_state_previous !=
				    CELLULAR_REGISTRATION_UNKNOWN);
#ifdef CONFIG_MODEM_HL78XX_00
	/* HL7800 PSM: sockets, PDP context and KCNXCFG are all destroyed
	 * when the modem enters PSM sleep.  Network setup (CGCONTRDP,
	 * DNS, KCNXCFG) is therefore needed on EVERY CARRIER_ON entry:
	 *   - First boot: modem just registered for the first time.
	 *   - PSM wake: contexts lost during sleep.
	 * Always set is_lpm = true (same rationale as POWER_DOWN).
	 *
	 */
	is_lpm = true;
#endif /* CONFIG_MODEM_HL78XX_00 */
#ifdef CONFIG_MODEM_HL78XX_12
	/* HL7812 PSM: sockets, IP and DNS are all retained after PSM exit.
	 * +KCELLMEAS releases the socket semaphore, so there is no need to
	 * re-run CGCONTRDP or refresh the DNS resolver.
	 *
	 * When +CEREG:5 fires (triggering CARRIER_ON) in psm.current psmev is
	 * still ENTER (set by +PSMEV:1 before sleep) because the +PSMEV:0
	 * URC hasn't arrived yet.  Detect this: if current is ENTER and
	 * we're already in CARRIER_ON, the modem woke from PSM.  Clear the
	 * state and return data can flow as soon as +KCELLMEAS fires.
	 */
	if (data->status.psmev.current == HL78XX_PSM_EVENT_ENTER) {
		LOG_DBG("HL7812 PSM wake: sockets retained, skipping CGCONTRDP/DNS");
		data->status.psmev.previous = HL78XX_PSM_EVENT_ENTER;
		data->status.psmev.current = HL78XX_PSM_EVENT_EXIT;
		return 0;
	}
#endif /* CONFIG_MODEM_HL78XX_12 */
#endif /* CONFIG_MODEM_HL78XX_PSM */

#ifdef CONFIG_MODEM_HL78XX_EDRX
	LOG_DBG("eDRX event previous: %d, current: %d, is_lpm: %d", data->status.edrxev.previous,
		data->status.edrxev.current, is_lpm);
#ifdef CONFIG_MODEM_HL78XX_12
	is_lpm = is_lpm || ((data->status.edrxev.previous == HL78XX_EDRX_EVENT_IDLE_NONE &&
			     data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_EXIT));
#else  /* CONFIG_MODEM_HL78XX_00 */
	is_lpm = is_lpm || (data->status.edrxev.current == HL78XX_EDRX_EVENT_IDLE_EXIT);
#endif /* CONFIG_MODEM_HL78XX_12 */
#endif /* CONFIG_MODEM_HL78XX_EDRX */

#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	LOG_DBG("POWER_DOWN: current=%d previous=%d", data->status.power_down.current,
		data->status.power_down.previous);
	is_lpm = true;
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */

	if (is_lpm) {
		LOG_DBG("LPM wake: deferring APN/CGCONTRDP to settling timer");
		data->status.lpm_restore_pending = true;
		hl78xx_start_timer(data, K_SECONDS(2));
	}
#else
	iface_status_work_cb(data, hl78xx_chat_callback_handler);
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	return 0;
}

static void hl78xx_carrier_on_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	int ret = 0;

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
#endif

	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_start_timer(data, K_SECONDS(2));

		break;
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		LOG_WRN("carrier_on script failed, retrying after backoff");
		hl78xx_start_timer(data, K_SECONDS(5));
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
#ifdef CONFIG_MODEM_HL78XX_PSM
		if (data->status.awaiting_psm_confirmation) {
			LOG_WRN("PSM confirmation timeout - deregistration likely due to "
				"coverage loss, not low power transition");
			data->status.awaiting_psm_confirmation = false;
			return;
		}
#endif /* CONFIG_MODEM_HL78XX_PSM */

		if (data->status.lpm_restore_pending) {
			data->status.lpm_restore_pending = false;
#if defined(CONFIG_MODEM_HL78XX_00) &&                                                             \
	(defined(CONFIG_MODEM_HL78XX_PSM) || defined(CONFIG_MODEM_HL78XX_EDRX))
			LOG_DBG("LPM restore: re-applying APN/KCNXCFG");
			ret = hl78xx_set_apn_internal(data, data->identity.apn,
						      strlen(data->identity.apn));
			if (ret) {
				LOG_ERR("LPM APN restore failed: %d, retrying", ret);
				data->status.lpm_restore_pending = true;
				hl78xx_start_timer(data, K_SECONDS(2));
				break;
			}
#endif /* CONFIG_MODEM_HL78XX_00 && (PSM || EDRX) */
			iface_status_work_cb(data, hl78xx_chat_callback_handler);
			break;
		}
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
		ret = dns_work_cb(data->dev, true);
		if (ret == -EAGAIN) {
			LOG_ERR("DNS work callback failed: rescheduling... %d", ret);
			hl78xx_start_timer(data, K_SECONDS(2));
			break;
		}
		if (ret < 0) {
			LOG_ERR("DNS work callback failed: %d", ret);
		}
		if (ret == 0) {
			LOG_DBG("DNS work callback succeeded");
#if (defined(CONFIG_MODEM_HL78XX_PSM) || defined(CONFIG_MODEM_HL78XX_EDRX)) &&                     \
	defined(CONFIG_MODEM_HL78XX_00)
#ifdef CONFIG_MODEM_HL78XX_PSM
			data->status.psmev.previous = HL78XX_PSM_EVENT_EXIT;
#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_EDRX
			data->status.edrxev.previous = HL78XX_EDRX_EVENT_IDLE_EXIT;
#endif /* CONFIG_MODEM_HL78XX_EDRX */
			hl78xx_release_socket_comms(data);
#endif /* (CONFIG_MODEM_HL78XX_PSM || CONFIG_MODEM_HL78XX_EDRX) && CONFIG_MODEM_HL78XX_00 */
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
			LOG_DBG("%d %d %d", __LINE__, data->status.power_down.previous,
				data->status.power_down.current);
			if (data->status.power_down.current != POWER_DOWN_EVENT_NONE) {
				modem_dynamic_cmd_send(
					data, NULL, (const char *)CHECK_LTE_COVERAGE_CMD,
					strlen(CHECK_LTE_COVERAGE_CMD), hl78xx_get_ok_match(),
					hl78xx_get_ok_match_size(), MDM_CMD_TIMEOUT, false);
				data->status.power_down.current = POWER_DOWN_EVENT_NONE;
				hl78xx_release_socket_comms(data);
			}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
		}
		break;

	case MODEM_HL78XX_EVENT_DEREGISTERED:
		/* If phone is being set to airplane mode, don't react to deregistration */
		if (data->status.phone_functionality.in_progress) {
			break;
		}
		/* If airplane mode is active, this is expected */
		if (data->status.phone_functionality.functionality == HL78XX_AIRPLANE) {
			break;
		}
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
		/* Start a brief timer to distinguish PSM-entry from real network loss.
		 * If GPIO6 fires DEVICE_ASLEEP within this window, the modem is entering
		 * PSM and we transition to SLEEP. Otherwise, the deregistration is due
		 * to coverage loss or SIM issue and we go to AWAIT_REGISTERED.
		 */
		hl78xx_start_timer(data, K_SECONDS(MDM_REGISTRATION_TIMEOUT));
#ifdef CONFIG_MODEM_HL78XX_PSM
		data->status.awaiting_psm_confirmation = true;
#ifdef CONFIG_MODEM_HL78XX_00
		LOG_DBG("Network state: %d", data->status.registration.network_state_current);
		if (data->status.registration.network_state_current ==
		    CELLULAR_REGISTRATION_UNKNOWN) {
			if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
				gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
			}
			LOG_DBG("Deregistered - WAKE LOW, awaiting GPIO6 LOW to confirm PSM");
		}
#else
		LOG_DBG("Deregistered - awaiting GPIO6 LOW to confirm PSM entry");
#endif /* CONFIG_MODEM_HL78XX_00 */
#endif /* CONFIG_MODEM_HL78XX_PSM */
		break;
#else
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
		break;
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	case MODEM_HL78XX_EVENT_DEVICE_ASLEEP:
		hl78xx_stop_timer(data);
#ifdef CONFIG_MODEM_HL78XX_PSM
		data->status.awaiting_psm_confirmation = false;
#endif /* CONFIG_MODEM_HL78XX_PSM */
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
		}
		LOG_DBG("%d reg status: %d", __LINE__,
			data->status.registration.network_state_current);
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_SLEEP);
		break;
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	case MODEM_HL78XX_EVENT_MDM_RESTART:
		LOG_DBG("Modem restart detected %d", __LINE__);
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_PHONE_FUNCTIONALITY_CHANGED:
		LOG_DBG("Phone functionality changed to %d in CARRIER_ON state",
			data->status.phone_functionality.functionality);
		/*
		 * If user manually sets airplane mode (not through hl78xx_enter_gnss_mode),
		 * we don't automatically enter GNSS mode. User must explicitly request it.
		 */
		if (data->status.phone_functionality.functionality == HL78XX_AIRPLANE) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_AIRPLANE);
		}
		break;

#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
	case MODEM_HL78XX_EVENT_WDSI_UPDATE:
	case MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_REQUEST:
	case MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_PROGRESS:
	case MODEM_HL78XX_EVENT_WDSI_INSTALL_REQUEST:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_FOTA);
		data->status.wdsi.in_progress = true;
		break;
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */

#ifdef CONFIG_HL78XX_GNSS
	case MODEM_HL78XX_EVENT_GNSS_MODE_ENTER_REQUESTED:
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
		/* In low power mode (PSM/eDRX), GNSS can operate when LTE is off.
		 * Instead of going through carrier_off -> airplane -> GNSS (as in
		 * non-LPM mode), we wait for the modem to enter PSM sleep.
		 * The gnss_mode_enter_pending flag is already set by the caller.
		 *
		 * In both modes the RF path is free for GNSS (LTE hasn't
		 * re-registered). No CFUN=4 is needed.
		 *
		 * PRE-PSM:  sleep handler auto-wakes modem on PSM entry
		 *           (BUS_CLOSED) to run GNSS immediately.
		 * POST-PSM: GNSS is deferred until the user wakes the modem
		 *           (e.g. via hl78xx_wakeup_modem). The wakeup
		 *           dispatches RESUME, and BUS_OPENED routes to GNSS
		 *           init before LTE re-registers.
		 */
#if defined(CONFIG_MODEM_HL78XX_PSM) || defined(CONFIG_MODEM_HL78XX_EDRX)
#ifdef CONFIG_MODEM_HL78XX_00
		LOG_INF("HL7800 GNSS requested in LPM - will start on "
			"PSM/EDRX entry");
		return;
#else /* HL7812 */
#ifdef CONFIG_MODEM_HL78XX_PSM
#if defined(CONFIG_HL78XX_GNSS_PROCESS_PRE_PSM)
		LOG_INF("GNSS mode requested in LPM - will start on PSM entry (pre-PSM)");
#else
		LOG_INF("GNSS mode requested in LPM - will start on user wakeup (post-PSM)");
#endif
#else
		LOG_INF("GNSS mode requested in LPM - will start on eDRX entry");
#endif /* CONFIG_MODEM_HL78XX_PSM */
		return;
#endif /* CONFIG_MODEM_HL78XX_00 */
#endif /* CONFIG_MODEM_HL78XX_PSM || CONFIG_MODEM_HL78XX_EDRX */

#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
		LOG_INF("GNSS mode requested - transitioning to carrier_off first");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_CARRIER_OFF);
		break;
#endif /* CONFIG_HL78XX_GNSS */

	default:
		break;
	}
}

static int hl78xx_on_carrier_on_state_leave(struct hl78xx_data *data)
{
	hl78xx_stop_timer(data);
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	data->status.lpm_restore_pending = false;
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	return 0;
}

#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE

static int hl78xx_on_fota_state_enter(struct hl78xx_data *data)
{
	HL78XX_LOG_DBG("Entering FOTA state");
	hl78xx_start_timer(data, K_MSEC(100));
	/* Decide best time to start FOTA */
	return 0;
}

static void hl78xx_fota_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		/* Start FOTA process */
		if (data->status.wdsi.in_progress == true) {
			if (data->status.wdsi.level == WDSI_FIRMWARE_DOWNLOAD_REQUEST &&
			    data->status.wdsi.fota_state != HL78XX_WDSI_FOTA_DOWNLOADING) {
				LOG_INF("FOTA available, notifying modem...");
				hl78xx_delegate_event(data,
						      MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_REQUEST);
			} else if (data->status.wdsi.level == WDSI_FIRMWARE_INSTALL_REQUEST &&
				   data->status.wdsi.fota_state != HL78XX_WDSI_FOTA_INSTALLING) {
				LOG_INF("FOTA downloaded, notifying modem...");
				hl78xx_delegate_event(data,
						      MODEM_HL78XX_EVENT_WDSI_INSTALL_REQUEST);
			} else {
				HL78XX_LOG_DBG("FOTA in progress, notifying modem...");
				hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_WDSI_UPDATE);
			}
		}
		break;
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		HL78XX_LOG_DBG("FOTA script completed successfully.");
		break;
	case MODEM_HL78XX_EVENT_WDSI_UPDATE:
		if (data->status.wdsi.level == WDSI_DM_SESSION_CLOSED) {
			LOG_INF("FOTA DM session closed.");
			if (data->status.wdsi.in_progress == true) {
				LOG_INF("FOTA update process completed, rebooting modem...");
			} else {
				LOG_INF("FOTA update process not started, returning to "
					"carrier on state...");
			}
			break;
		}
		break;
	case MODEM_HL78XX_EVENT_WDSI_RESTART:
		LOG_INF("FOTA modem restart occurred...%d", data->status.boot.is_booted_previously);
		break;
	case MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_REQUEST:
		if (data->status.wdsi.fota_state == HL78XX_WDSI_FOTA_DOWNLOADING) {
			return;
		}
		LOG_INF("FOTA download requested File size %d bytes, starting download...",
			data->status.wdsi.fota_size);
		data->status.wdsi.fota_state = HL78XX_WDSI_FOTA_DOWNLOADING;
		hl78xx_run_fota_script_download_accept_async(data);
		break;
	case MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_PROGRESS:
		LOG_INF("FOTA update in progress, completion... %d%%", data->status.wdsi.progress);
		break;
	case MODEM_HL78XX_EVENT_WDSI_DOWNLOAD_COMPLETE:
		LOG_INF("FOTA download completed...");
		data->status.wdsi.fota_state = HL78XX_WDSI_FOTA_DOWNLOAD_COMPLETED;
		break;
	case MODEM_HL78XX_EVENT_WDSI_INSTALL_REQUEST:
		LOG_INF("FOTA install request received...");
		data->status.wdsi.fota_state = HL78XX_WDSI_FOTA_INSTALLING;
		hl78xx_run_fota_script_install_accept_async(data);
		break;
	case MODEM_HL78XX_EVENT_WDSI_INSTALLING_FIRMWARE:
		LOG_INF("Install accepted, FOTA update installing...");
		LOG_INF("This may take several minutes, please wait...");
		LOG_INF("Do not power off the modem!!!");
		break;
	case MODEM_HL78XX_EVENT_WDSI_FIRMWARE_INSTALL_SUCCEEDED:
		LOG_INF("FOTA firmware install succeeded...Waiting for modem +KSUP");
		data->status.wdsi.fota_state = HL78XX_WDSI_FOTA_INSTALL_COMPLETED;
		data->status.wdsi.in_progress = false;
		data->status.wdsi.progress = 0;
		break;
	case MODEM_HL78XX_EVENT_WDSI_FIRMWARE_INSTALL_FAILED:
		LOG_INF("FOTA firmware install failed...");
		data->status.wdsi.in_progress = false;
		data->status.wdsi.progress = 0;
		/* TODO: fail, do something */
		break;
	case MODEM_HL78XX_EVENT_MDM_RESTART:
		if (data->status.wdsi.in_progress == true) {
			/* FOTA update in progress, waiting for it to complete */
			data->status.wdsi.fota_state = HL78XX_WDSI_FOTA_IDLE;
		} else {
			LOG_INF("Exiting FOTA state, re-initializing modem...");
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
		}
		break;
	case MODEM_HL78XX_EVENT_REGISTERED:
		/* stay in FOTA state */
		hl78xx_start_timer(data, K_MSEC(100));
		break;
	default:
		break;
	}
}

static int hl78xx_on_fota_state_leave(struct hl78xx_data *data)
{
	HL78XX_LOG_DBG("Exiting FOTA state");
	hl78xx_stop_timer(data);
	return 0;
}

#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
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
#ifdef CONFIG_HL78XX_GNSS
		/*
		 * Check if GNSS mode entry is pending - if so, transition to airplane mode
		 * instead of returning to GPRS/LTE
		 */
		if (hl78xx_gnss_is_pending(data)) {
			LOG_INF("Carrier off complete, transitioning to airplane mode for GNSS");
			modem_dynamic_cmd_send(data, NULL, GET_FULLFUNCTIONAL_MODE_CMD,
					       strlen(GET_FULLFUNCTIONAL_MODE_CMD),
					       hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
					       MDM_CMD_TIMEOUT, false);
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_AIRPLANE);
			break;
		}
#endif /* CONFIG_HL78XX_GNSS */
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

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
static int hl78xx_on_sleep_state_enter(struct hl78xx_data *data)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
		LOG_DBG("Set WAKE pin to 0");
	}
#ifdef CONFIG_MODEM_HL78XX_EDRX
	/* Cancel the eDRX idle timer since we are already entering sleep */
	hl78xx_edrx_idle_cancel(data);
#endif

	modem_chat_release(&data->chat);
	modem_pipe_attach(data->uart_pipe, hl78xx_bus_pipe_handler, data);
	modem_pipe_close_async(data->uart_pipe);

#if defined(CONFIG_MODEM_HL78XX_00)
	/* HL7800 destroys all TCP/UDP socket contexts when entering PSM/eDRX.
	 * Invalidate modem-side socket IDs now so they will be transparently
	 * re-created when the application uses them after wake.
	 */
	hl78xx_invalidate_socket_contexts(data);
#endif /* CONFIG_MODEM_HL78XX_00 */

	k_sem_give(&data->suspended_sem);
	return 0;
}

static void hl78xx_sleep_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	int rc;

	switch (evt) {
	case MODEM_HL78XX_EVENT_BUS_CLOSED:
		/* Modem bus is closed, suspend UART to save power */
		LOG_DBG("Modem bus closed, suspending UART");
		uart_irq_rx_disable(config->uart);
		rc = pm_device_action_run(config->uart, PM_DEVICE_ACTION_SUSPEND);
		if (rc < 0 && rc != -EALREADY) {
			LOG_ERR("UART suspend failed: %d", rc);
		}
#ifdef CONFIG_HL78XX_GNSS
		if (hl78xx_gnss_is_pending(data)) {
#if defined(CONFIG_HL78XX_GNSS_PROCESS_PRE_PSM)
			/* PRE-PSM mode: wake modem immediately to start GNSS
			 * while LTE is still hibernating in PSM.
			 */
			LOG_INF("GNSS pending - waking modem for GNSS search (pre-PSM)");
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
#else
			LOG_INF("GNSS pending - will process after user wakeup (post-PSM)");
#endif /* CONFIG_HL78XX_GNSS_PROCESS_PRE_PSM */
		}
#endif /* CONFIG_HL78XX_GNSS */
		break;
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		/* Successfully entered sleep mode */
		break;
	case MODEM_HL78XX_EVENT_RESUME:
		/* Modem is waking up from sleep */
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
			gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
			LOG_DBG("Set WAKE pin to 1");
		}
		rc = pm_device_action_run(config->uart, PM_DEVICE_ACTION_RESUME);
		if (rc < 0 && rc != -EALREADY) {
			LOG_ERR("UART resume failed: %d", rc);
		}
		uart_irq_rx_enable(config->uart);
		hl78xx_start_timer(data, K_MSEC(config->startup_time_ms));
		break;
#ifdef CONFIG_MODEM_HL78XX_00
	case MODEM_HL78XX_EVENT_DEVICE_AWAKE:
		/* GPIO6 detected modem wake (HL7800) or PSMEV/GPIO6 (HL7812).
		 * Initiate the resume sequence to reopen UART and chat.
		 */
		LOG_DBG("Modem woke from sleep (DEVICE_AWAKE) - starting resume");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
		break;
#endif /* CONFIG_MODEM_HL78XX_00 */
	case MODEM_HL78XX_EVENT_TIMEOUT:
		LOG_DBG("Modem wakeup settling complete, reopening UART pipe");
		modem_pipe_attach(data->uart_pipe, hl78xx_bus_pipe_handler, data);
		modem_pipe_open_async(data->uart_pipe);
		break;
	case MODEM_HL78XX_EVENT_BUS_OPENED:
		LOG_DBG("UART pipe opened, re-attaching modem chat");
		modem_chat_attach(&data->chat, data->uart_pipe);
		/* After wakeup, wait for +CEREG and +KCELLMEAS URCs before
		 * allowing data transmission (per HL78xx app note).
		 * AWAIT_REGISTERED will transition to CARRIER_ON over +CEREG URC.
		 */
#ifdef CONFIG_HL78XX_GNSS
		if (hl78xx_gnss_is_pending(data)) {
#if defined(CONFIG_MODEM_HL78XX_00) && defined(CONFIG_MODEM_HL78XX_PSM)
			break;
#else
			/* Process pending GNSS mode entry request */
			LOG_INF("GNSS pending from sleep - transitioning to GNSS init");
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
			break;

#endif /* CONFIG_MODEM_HL78XX_00 && CONFIG_MODEM_HL78XX_PSM */
		} else {
#endif /* CONFIG_HL78XX_GNSS */

#if defined(CONFIG_MODEM_HL78XX_00) && defined(CONFIG_MODEM_HL78XX_LOW_POWER_MODE) &&              \
	!defined(CONFIG_MODEM_HL78XX_POWER_DOWN)
			LOG_INF("HL7800 warm wake: waiting for +KCELLMEAS");
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
#elif defined(CONFIG_MODEM_HL78XX_POWER_DOWN)
		/* POWER_DOWN mode: modem reboots on wake, need full init.
		 * Run init script from scratch (like first power-up).
		 */
		LOG_INF("Power-down wake: running init script");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
#else
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
#endif
#ifdef CONFIG_HL78XX_GNSS
		}
#endif /* CONFIG_HL78XX_GNSS */
		break;
#ifdef CONFIG_HL78XX_GNSS
	case MODEM_HL78XX_EVENT_GNSS_MODE_ENTER_REQUESTED:
		LOG_INF("GNSS mode requested from sleep - waking modem first");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
		break;
#endif /* CONFIG_HL78XX_GNSS */
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		LOG_ERR("Failed to enter sleep mode");
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		/* Already in sleep mode, ignore */
		break;

	default:
		break;
	}
}

static int hl78xx_on_sleep_state_leave(struct hl78xx_data *data)
{
	return 0;
}
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

static int hl78xx_on_airplane_mode_state_enter(struct hl78xx_data *data)
{
	if (data->status.phone_functionality.functionality == HL78XX_AIRPLANE) {
		/* Already in airplane mode */
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_SUCCESS);
		return 0;
	}
	HL78XX_LOG_DBG("Setting airplane mode (CFUN=4)...");
	hl78xx_api_func_set_phone_functionality(data->dev, HL78XX_AIRPLANE, false);

	return hl78xx_run_cfun_query_script_async(data);
}

static void hl78xx_airplane_mode_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
#ifdef CONFIG_HL78XX_GNSS
		/* Check and process any pending GNSS mode entry request */
		if (hl78xx_gnss_is_pending(data)) {
			LOG_INF("Processing pending GNSS mode request");
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
			break;
		}
#endif /* CONFIG_HL78XX_GNSS */
		break;

#ifdef CONFIG_HL78XX_GNSS
	case MODEM_HL78XX_EVENT_GNSS_MODE_ENTER_REQUESTED:
		/* User explicitly requested GNSS mode while in airplane state */
		LOG_INF("GNSS mode requested - transitioning to GNSS init state");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT);
		break;
#endif /* CONFIG_HL78XX_GNSS */

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		LOG_ERR("Failed to set airplane mode");
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int hl78xx_on_airplane_mode_state_leave(struct hl78xx_data *data)
{
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
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	if (data->status.power_down.current == POWER_DOWN_EVENT_NONE &&
	    !data->status.power_down.is_power_down_requested) {
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
		if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
			gpio_pin_set_dt(&config->mdm_gpio_reset, 1);
		}
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	}
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
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
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	data->status.power_down.is_power_down_requested = false;
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
	return 0;
}

static int hl78xx_on_state_enter(struct hl78xx_data *data)
{
	int ret = 0;
	enum hl78xx_state s = data->status.state;

	/* Use an explicit bounds check against the last enum value so this
	 * code can reference the table even though the table is defined later
	 * in the file. MODEM_HL78XX_STATE_COUNT is the last value in
	 * the `enum hl78xx_state`.
	 */
	if ((int)s < MODEM_HL78XX_STATE_COUNT && hl78xx_state_table[s].on_enter) {
		ret = hl78xx_state_table[s].on_enter(data);
	}

	return ret;
}

static int hl78xx_on_state_leave(struct hl78xx_data *data)
{
	int ret = 0;
	enum hl78xx_state s = data->status.state;

	if ((int)s < MODEM_HL78XX_STATE_COUNT && hl78xx_state_table[s].on_leave) {
		ret = hl78xx_state_table[s].on_leave(data);
	}

	return ret;
}

void hl78xx_enter_state(struct hl78xx_data *data, enum hl78xx_state state)
{
	int ret;

	LOG_DBG("Transitioning from state %d to state %d", data->status.state, state);
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
	if ((int)s < MODEM_HL78XX_STATE_COUNT && hl78xx_state_table[s].on_event) {
		hl78xx_state_table[s].on_event(data, evt);
	} else {
		LOG_ERR("%d unknown event %d", __LINE__, evt);
	}
	if (state != s) {
		hl78xx_log_state_changed(state, s);
	}
}

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
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
#ifdef CONFIG_MODEM_HL78XX_PSM
	hl78xx_psmev_init(data);
#endif /* CONFIG_MODEM_HL78XX_PSM */
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	hl78xx_init_power_down(data);
#endif /* CONFIG_MODEM_HL78XX_POWER_DOWN */
#ifdef CONFIG_MODEM_HL78XX_EDRX
	hl78xx_edrx_idle_init(data);
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	data->dev = dev;
	/* reset to default  */
	data->buffers.eof_pattern_size = strlen(data->buffers.eof_pattern);
	data->buffers.termination_pattern_size = strlen(data->buffers.termination_pattern);
	memset(data->identity.apn, 0, MDM_APN_MAX_LENGTH);
#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
	data->status.uart.current_baudrate = 0;
	data->status.uart.target_baudrate = CONFIG_MODEM_HL78XX_TARGET_BAUDRATE_VALUE;
	data->status.uart.baudrate_detection_retry = 0;
#ifdef CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE
	/* Update baud rate */
	configure_uart_for_auto_baudrate(data, data->status.uart.target_baudrate);
#endif /* CONFIG_MODEM_HL78XX_AUTOBAUD_START_WITH_TARGET_BAUDRATE */
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */
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

#ifdef CONFIG_MODEM_HL78XX_STAY_IN_BOOT_MODE_FOR_ROAMING
	k_sem_take(&data->stay_in_boot_mode_sem, K_FOREVER);
#endif
	LOG_INF("Modem HL78xx initialized");
	/* Register the power management handler */
	return pm_device_driver_init(dev, hl78xx_driver_pm_action);

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
#ifdef CONFIG_MODEM_HL78XX_AUTO_BAUDRATE
	[MODEM_HL78XX_STATE_SET_BAUDRATE] = {
		hl78xx_on_set_baudrate_state_enter,
		NULL,
		hl78xx_set_baudrate_event_handler
	},
#endif /* CONFIG_MODEM_HL78XX_AUTO_BAUDRATE */
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
		hl78xx_on_rat_cfg_script_state_leave,
		hl78xx_run_rat_cfg_script_event_handler
	},
	[MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT] = {
		hl78xx_on_pmc_cfg_script_state_enter,
		hl78xx_on_run_pmc_cfg_script_state_leave,
		hl78xx_run_pmc_cfg_script_event_handler
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
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
	[MODEM_HL78XX_STATE_FOTA] = {
		hl78xx_on_fota_state_enter,
		hl78xx_on_fota_state_leave,
		hl78xx_fota_event_handler
	},
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */
	[MODEM_HL78XX_STATE_CARRIER_OFF] = {
		hl78xx_on_carrier_off_state_enter,
		hl78xx_on_carrier_off_state_leave,
		hl78xx_carrier_off_event_handler
	},
#ifdef CONFIG_HL78XX_GNSS
	[MODEM_HL78XX_STATE_RUN_GNSS_INIT_SCRIPT] = {
		hl78xx_on_run_gnss_init_script_state_enter,
		hl78xx_on_run_gnss_init_script_state_leave,
		hl78xx_run_gnss_init_script_event_handler
	},
	[MODEM_HL78XX_STATE_GNSS_SEARCH_STARTED] = {
		hl78xx_on_gnss_search_started_state_enter,
		hl78xx_on_gnss_search_started_state_leave,
		hl78xx_gnss_search_started_event_handler
	},
#endif /* CONFIG_HL78XX_GNSS */
	[MODEM_HL78XX_STATE_SIM_POWER_OFF] = {
		NULL,
		NULL,
		NULL
	},
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	[MODEM_HL78XX_STATE_SLEEP] = {
		hl78xx_on_sleep_state_enter,
		hl78xx_on_sleep_state_leave,
		hl78xx_sleep_event_handler
	},
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */
	[MODEM_HL78XX_STATE_AIRPLANE] = {
		hl78xx_on_airplane_mode_state_enter,
		hl78xx_on_airplane_mode_state_leave,
		hl78xx_airplane_mode_event_handler
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
	MODEM_HL78XX_DEFINE_INSTANCE(inst, CONFIG_MODEM_HL78XX_DEV_POWER_PULSE_DURATION_MS,        \
				     CONFIG_MODEM_HL78XX_DEV_RESET_PULSE_DURATION_MS,              \
				     CONFIG_MODEM_HL78XX_DEV_STARTUP_TIME_MS,                      \
				     CONFIG_MODEM_HL78XX_DEV_SHUTDOWN_TIME_MS, false, NULL, NULL)

#define DT_DRV_COMPAT swir_hl7812
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
