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

#define MAX_SCRIPT_AT_CMD_RETRY 3

#define MDM_NODE DT_ALIAS(modem)

/* GPIO availability macros */
#define HAS_PWR_ON_GPIO     DT_NODE_HAS_PROP(MDM_NODE, mdm_pwr_on_gpios)
#define HAS_FAST_SHUTD_GPIO DT_NODE_HAS_PROP(MDM_NODE, mdm_fast_shutd_gpios)
#define HAS_UART_DTR_GPIO   DT_NODE_HAS_PROP(MDM_NODE, mdm_uart_dtr_gpios)
#define HAS_GPIO6_GPIO      DT_NODE_HAS_PROP(MDM_NODE, mdm_gpio6_gpios)
#define HAS_UART_DSR_GPIO   DT_NODE_HAS_PROP(MDM_NODE, mdm_uart_dsr_gpios)

/* GPIO count macro */
#define GPIO_CONFIG_LEN                                                                            \
	(3 /* reset, wake, vgpio */ + HAS_PWR_ON_GPIO + HAS_FAST_SHUTD_GPIO + HAS_UART_DTR_GPIO +  \
	 HAS_GPIO6_GPIO + HAS_UART_DSR_GPIO)

LOG_MODULE_REGISTER(hl78xx_dev, CONFIG_MODEM_LOG_LEVEL);

/* RX thread work queue */
K_KERNEL_STACK_DEFINE(modem_workq_stack, CONFIG_MODEM_HL78XX_RX_WORKQ_STACK_SIZE);

static struct k_work_q modem_workq;
hl78xx_evt_monitor_handler_t event_dispatcher;

static void hl78xx_event_handler(struct hl78xx_data *data, enum hl78xx_event evt);
static int hl78xx_on_idle_state_enter(struct hl78xx_data *data);
static int hl78xx_on_state_enter(struct hl78xx_data *data);
static int hl78xx_on_state_leave(struct hl78xx_data *data);
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
static int hl78xx_pwr_dwn_feed_timer(struct hl78xx_data *data);
#endif
static void event_dispatcher_dispatch(struct hl78xx_evt *notif)
{
	if (event_dispatcher != NULL) {
		event_dispatcher(notif);
	}
}

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
	case MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT:
		return "run pmc cfg script";
	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		return "run rat cfg script";
	case MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT:
		return "run enable gprs script";
	case MODEM_HL78XX_STATE_AWAIT_WAKEUP:
		return "await wakeup";
	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		return "await registered";
	case MODEM_HL78XX_STATE_CARRIER_ON:
		return "carrier on";
	case MODEM_HL78XX_STATE_CARRIER_OFF:
		return "carrier on";
	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		return "init power off";
	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		return "power off pulse";
	case MODEM_HL78XX_STATE_SIM_POWER_OFF:
		return "sim power off";
	case MODEM_HL78XX_STATE_AIRPLANE:
		return "airplane mode";
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
	case MODEM_HL78XX_EVENT_SOCKET_CLOSED:
		return "socket closed";
	case MODEM_HL78XX_EVENT_DEVICE_AWAKE:
		return "device awake";
	case MODEM_HL78XX_EVENT_DEVICE_ASLEEP:
		return "device asleep";
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
	LOG_DBG("switch from %s to %s", hl78xx_state_str(last_state), hl78xx_state_str(new_state));
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

static void hl78xx_chat_callback_handler(struct modem_chat *chat,
					 enum modem_chat_script_result result, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_SUCCESS);
	} else {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_FAILED);
	}
}

static void hl78xx_on_cxreg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	enum hl78xx_registration_status registration_status = 0;

	if (argc < 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %d %s", __LINE__, __func__, argc, argv[0]);
#endif
	if (argc > 2 && strlen(argv[1]) == 1 && strlen(argv[2]) == 1) {
		/* This is a condition to distinguish received message between URC message and User
		 * request network status request. If the message is User message, then argv[1] and
		 * argv[2] will be 1 character long. If the message is URC request network status
		 * request, then argv[1] will be 1 character long while argv[2] will be 2 characters
		 * long.
		 */
		registration_status = atoi(argv[2]);
	} else {
		registration_status = atoi(argv[1]);
	}

	if (registration_status == data->status.registration.network_state_current) {
		LOG_DBG("Registration status unchanged: %d", registration_status);
		return;
	}

	data->status.registration.network_state_previous =
		data->status.registration.network_state_current;
	/* Update the current registration state */
	data->status.registration.network_state_current = registration_status;
	struct hl78xx_evt event = {.type = HL78XX_LTE_REGISTRATION_STAT_UPDATE,
				   .content.reg_status =
					   data->status.registration.network_state_current};

	event_dispatcher_dispatch(&event);
	data->status.registration.is_registered_previously =
		data->status.registration.is_registered_currently;
	if (hl78xx_is_registered(data)) {
		data->status.registration.is_registered_currently = true;
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
#if defined CONFIG_MODEM_HL78XX_POWER_DOWN &&                                                      \
	defined CONFIG_MODEM_HL78XX_USE_ACTIVE_TIME_BASED_POWER_DOWN
		hl78xx_pwr_dwn_feed_timer(data);
#endif
	} else {
		data->status.registration.is_registered_currently = false;
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEREGISTERED);
	}
}

static void hl78xx_on_psmev(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %d", __LINE__, __func__, argc);
#endif
	if (argc < 2) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	data->status.psm.psmev_previous = data->status.psm.psmev_current;
	data->status.psm.psmev_current = ATOI(argv[1], 0, "psmev");
	struct hl78xx_evt event = {.type = HL78XX_LTE_PSMEV,
				   .content.psm_event = data->status.psm.psmev_current};

	event_dispatcher_dispatch(&event);
}

static void hl78xx_on_ksup(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc != 2) {
		return;
	}

	int module_status = ATOI(argv[1], 0, "ksup");

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("Module status: %d", module_status);
#endif
	struct hl78xx_evt event = {.type = HL78XX_LTE_MODEM_STARTUP,
				   .content.value = module_status};

	event_dispatcher_dispatch(&event);
}

static void hl78xx_on_imei(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("IMEI: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->identity.imei, argv[1], sizeof(data->identity.imei) - 1);
}

static void hl78xx_on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("cgmm: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->identity.model_id, argv[1], sizeof(data->identity.model_id) - 1);
}

static void hl78xx_on_imsi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("IMSI: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->identity.imsi, argv[1], sizeof(data->identity.imsi) - 1);
#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
	/* set the APN automatically */
	modem_detect_apn(data, argv[1]);
#endif
}

static void hl78xx_on_cgmi(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("cgmi: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->identity.manufacturer, argv[1], sizeof(data->identity.manufacturer) - 1);
}

static void hl78xx_on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("cgmr: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->identity.fw_version, argv[1], sizeof(data->identity.fw_version) - 1);
}

static void hl78xx_on_iccid(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("ICCID: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->identity.iccid, argv[1], sizeof(data->identity.iccid) - 1);
#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID)
	/* set the APN automatically */
	modem_detect_apn(data, argv[1]);
#endif
}

/* Handler: +KSTATEV: */
static void hl78xx_on_kstatev(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	enum hl78xx_cell_rat_mode rat_mode = HL78XX_RAT_MODE_NONE;

	if (argc != 3) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("KSTATEV: %s %s %s", argv[0], argv[1], argv[2]);
#endif
	rat_mode = ATOI(argv[2], 0, "rat_mode");

	hl78xx_on_kstatev_parser(data, ATOI(argv[1], 0, "status"));
	if (rat_mode != data->status.registration.rat_mode) {
		struct hl78xx_evt event = {.type = HL78XX_RAT_UPDATE,
					   .content.rat_mode = data->status.registration.rat_mode};

		event_dispatcher_dispatch(&event);
	}
	data->status.registration.rat_mode = rat_mode;
}

static void hl78xx_on_udprcv(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
#endif
}

static void hl78xx_on_socknotifydata(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	int socket_id = -1;
	int new_total = -1;

	if (argc < 2) {
		return;
	}
	socket_id = ATOI(argv[1], -1, "socket_id");
	new_total = ATOI(argv[2], -1, "length");
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d %d", __LINE__, socket_id, new_total);
#endif
	socket_notify_data(socket_id, new_total);
}

static void hl78xx_on_ktcpnotif(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	int socket_id = -1;
	int tcp_notif = -1;

	if (argc < 2) {
		return;
	}
	socket_id = ATOI(argv[1], -1, "socket_id");
	tcp_notif = ATOI(argv[2], -1, "tcp_notif");
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d %d", __LINE__, socket_id, tcp_notif);
#endif

	if (tcp_notif == -1) {
		return;
	}

	tcp_notify_data(socket_id, tcp_notif);
}

static void hl78xx_on_ksrep(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 2) {
		return;
	}

	data->status.ksrep = (uint8_t)atoi(argv[1]);
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("KSREP: %s %s", argv[0], argv[1]);
#endif
}
#ifndef CONFIG_MODEM_HL78XX_AUTORAT
static void hl78xx_on_ksrat(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	if (argc < 2) {
		return;
	}

	data->status.registration.rat_mode = (uint8_t)atoi(argv[1]);

	struct hl78xx_evt event = {.type = HL78XX_RAT_UPDATE,
				   .content.rat_mode = data->status.registration.rat_mode};

	event_dispatcher_dispatch(&event);

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("KSRAT: %s %s", argv[0], argv[1]);
#endif
}
#else
static void hl78xx_on_kselacq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}

	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

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
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%d] [%d] [%d] [%d]", __LINE__, argc, argv[0], data->kselacq_data.mode,
		data->kselacq_data.rat1, data->kselacq_data.rat2, data->kselacq_data.rat3);
#endif
}
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

static void hl78xx_on_kbndcfg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 3) {
		return;
	}

	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
#endif

	uint8_t rat_id = ATOI(argv[1], 0, "rat");
	uint8_t kbnd_bitmap_size = strlen(argv[2]);

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

static void hl78xx_on_csq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 3) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
#endif

	data->status.rssi = ATOI(argv[1], 0, "rssi");
}

static void hl78xx_on_cesq(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 7) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
#endif

	data->status.rsrq = ATOI(argv[5], 0, "rssq");
	data->status.rsrp = ATOI(argv[6], 0, "rssp");
}

static void hl78xx_on_kcellmeas(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	if (argc < 5) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
#endif
	data->status.rsrp = (int)ATOD(argv[1], 0, "rsrp");
	LOG_DBG("%d %s RSRP: %d", __LINE__, __func__, data->status.rsrp);
	if (hl78xx_is_rsrp_valid(data)) {
		data->status.registration.is_registered_currently = true;
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
		hl78xx_release_socket_comms();
	}
}

static void hl78xx_on_cfun(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] ", __LINE__, argc, argv[0], argv[1]);
#endif

	data->status.phone_functionality = ATOI(argv[1], 0, "phone_func");
}

static void hl78xx_on_ksleep(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] ", __LINE__, argc, argv[0], argv[1]);
#endif
	if (argc > 3) {
		data->status.pmc_sleep.mngt = ATOI(argv[1], 0, "mngt");
		data->status.pmc_sleep.level = ATOI(argv[2], 0, "level");
		data->status.pmc_sleep.delay = ATOI(argv[3], 0, "delay");
	} else {
		/* only the case when the modem sleep mode is always disabled
		 * AT+KSLEEP?
		 * +KSLEEP: 2
		 * OK
		 */
		data->status.pmc_sleep.mngt = ATOI(argv[1], 0, "mngt");
		data->status.pmc_sleep.level = 0;
		data->status.pmc_sleep.delay = 0;
	}
}
static void hl78xx_on_cpsms(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[4], argv[5]);
#endif
	if (argc > 3) {
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
static void hl78xx_on_kedrxcfg(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 3) {
		return;
	}
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	int act_type = ATOI(argv[2], -1, "act_type");
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d [%s] [%s] ", __LINE__, argc, argv[0], argv[1]);
#endif
	if (act_type < 3 || act_type > 5) {
		LOG_ERR("Invalid act_type %d", act_type);
		return;
	}

	if (argc > 3) {
		data->status.pmc_kedrxcfg[act_type - HL78XX_ACT_TYPE_RAT_MASK].mode =
			ATOI(argv[1], 0, "mode");
		data->status.pmc_kedrxcfg[act_type - HL78XX_ACT_TYPE_RAT_MASK].ack_type =
			ATOI(argv[2], 0, "ack_type");
		data->status.pmc_kedrxcfg[act_type - HL78XX_ACT_TYPE_RAT_MASK].requested_edrx =
			ATOI(argv[3], 0, "r_edrx");
	} else {
		data->status.pmc_kedrxcfg[act_type - HL78XX_ACT_TYPE_RAT_MASK].mode =
			ATOI(argv[1], 0, "mode");
		data->status.pmc_kedrxcfg[act_type - HL78XX_ACT_TYPE_RAT_MASK].ack_type =
			ATOI(argv[2], 0, "ack_type");
		data->status.pmc_kedrxcfg[act_type - HL78XX_ACT_TYPE_RAT_MASK].requested_edrx = 0;
	}
}
MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(allow_match, MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH("+CME ERROR: ", "", NULL));

MODEM_CHAT_MATCHES_DEFINE(unsol_matches, MODEM_CHAT_MATCH("+CREG: ", ",", hl78xx_on_cxreg),
			  MODEM_CHAT_MATCH("+CEREG: ", ",", hl78xx_on_cxreg),
			  MODEM_CHAT_MATCH("+CGREG: ", ",", hl78xx_on_cxreg),
			  MODEM_CHAT_MATCH("+PSMEV: ", "", hl78xx_on_psmev),
			  MODEM_CHAT_MATCH("+KSTATEV: ", ",", hl78xx_on_kstatev),
			  MODEM_CHAT_MATCH("+KUDP_DATA: ", ",", hl78xx_on_socknotifydata),
			  MODEM_CHAT_MATCH("+KTCP_DATA: ", ",", hl78xx_on_socknotifydata),
			  MODEM_CHAT_MATCH("+KTCP_NOTIF: ", ",", hl78xx_on_ktcpnotif),
			  MODEM_CHAT_MATCH("+KUDP_RCV: ", ",", hl78xx_on_udprcv),
			  MODEM_CHAT_MATCH("+KBNDCFG: ", ",", hl78xx_on_kbndcfg),
			  MODEM_CHAT_MATCH("+CSQ: ", ",", hl78xx_on_csq),
			  MODEM_CHAT_MATCH("+CESQ: ", ",", hl78xx_on_cesq),
			  MODEM_CHAT_MATCH("+KCELLMEAS: ", ",", hl78xx_on_kcellmeas),
			  MODEM_CHAT_MATCH("+CFUN: ", "", hl78xx_on_cfun),
			  MODEM_CHAT_MATCH("+KEDRXCFG: ", ",", hl78xx_on_kedrxcfg));

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("ERROR", "", NULL));
MODEM_CHAT_MATCH_DEFINE(at_ready_match, "+KSUP: ", "", hl78xx_on_ksup);
MODEM_CHAT_MATCH_DEFINE(imei_match, "", "", hl78xx_on_imei);
MODEM_CHAT_MATCH_DEFINE(cgmm_match, "", "", hl78xx_on_cgmm);
MODEM_CHAT_MATCH_DEFINE(cimi_match, "", "", hl78xx_on_imsi);
MODEM_CHAT_MATCH_DEFINE(cgmi_match, "", "", hl78xx_on_cgmi);
MODEM_CHAT_MATCH_DEFINE(cgmr_match, "", "", hl78xx_on_cgmr);
MODEM_CHAT_MATCH_DEFINE(iccid_match, "+CCID: ", "", hl78xx_on_iccid);
MODEM_CHAT_MATCH_DEFINE(ksrep_match, "+KSREP: ", ",", hl78xx_on_ksrep);
#ifndef CONFIG_MODEM_HL78XX_AUTORAT
MODEM_CHAT_MATCH_DEFINE(ksrat_match, "+KSRAT: ", "", hl78xx_on_ksrat);
#else
MODEM_CHAT_MATCH_DEFINE(kselacq_match, "+KSELACQ: ", ",", hl78xx_on_kselacq);
#endif /* CONFIG_MODEM_HL78XX_RAT */
MODEM_CHAT_MATCH_DEFINE(ksleep_match, "+KSLEEP: ", ",", hl78xx_on_ksleep);
MODEM_CHAT_MATCH_DEFINE(cpsms_match, "+CPSMS: ", ",", hl78xx_on_cpsms);

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
		.argv_size = ARRAY_SIZE(data->buffers.argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	return modem_chat_init(&data->chat, &chat_config);
}
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_periodic_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSIMDET?", ok_match),
			      /*  MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG?", ok_match) */);

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_periodic_chat_script, hl78xx_periodic_chat_script_cmds,
			 abort_matches, hl78xx_chat_callback_handler, 4);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	hl78xx_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("", at_ready_match),
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KHWIOCFG=3,1,6", ok_match),
#endif
	MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0", allow_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSLEEP?", ksleep_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPSMS?", cpsms_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KEDRXCFG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KPATTERN="
				   "\"" EOF_PATTERN "\"",
				   ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CCID", iccid_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+GNSSCONF=10,1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+GNSSNMEA=0,1000,0,104F", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSTATEV=1", ok_match),
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KPSMEV=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KCELLMEAS=1,35", ok_match),
#endif
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGEREP=2", ok_match),
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSELACQ?", kselacq_match),
#else
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSRAT?", ksrat_match),
#endif
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KBNDCFG?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGACT?", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP(
		"AT+CEREG=" CONFIG_MODEM_HL78XX_NETWORK_REG_STATUS_REPORT_CFG_CODE, ok_match));

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_init_chat_script, hl78xx_init_chat_script_cmds, abort_matches,
			 hl78xx_chat_callback_handler, 100);

int modem_cmd_send_int(struct hl78xx_data *data, modem_chat_script_callback script_user_callback,
		       const uint8_t *cmd, uint16_t cmd_size,
		       const struct modem_chat_match *response_matches, uint16_t matches_size,
		       bool user_cmd)
{
	int ret = 0;
#if defined CONFIG_MODEM_HL78XX_POWER_DOWN && defined CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN
	if (data->status.state == MODEM_HL78XX_STATE_CARRIER_ON) {
		hl78xx_pwr_dwn_feed_timer(data);
	}
#endif
	ret = k_mutex_lock(&data->tx_lock, K_NO_WAIT);
	if (ret < 0) {
		if (user_cmd == false) {
			errno = -ret;
		}
		return -1;
	}
	/* Optional logic: only set capture mode for CONNECT-type commands */
	struct modem_chat_script_chat dynamic_script = {
		.request = cmd,
		.request_size = cmd_size,
		.response_matches = response_matches,
		.response_matches_size = matches_size,
		.timeout = 1000,
	};
	struct modem_chat_script chat_script = {.name = "dynamic_script",
						.script_chats = &dynamic_script,
						.script_chats_size = 1,
						.abort_matches = abort_matches,
						.abort_matches_size = ARRAY_SIZE(abort_matches),
						.callback = script_user_callback,
						.timeout = 1000};

	ret = modem_chat_run_script(&data->chat, &chat_script);
	if (ret < 0) {
		LOG_ERR("%d %s Failed to run at command: %d", __LINE__, __func__, ret);
	} else {
		LOG_DBG("Chat script executed successfully.");
	}
	ret = k_mutex_unlock(&data->tx_lock);
	if (ret < 0) {
		if (user_cmd == false) {
			errno = -ret;
		}
		return -1;
	}
	return ret;
}

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
	enum pm_device_state state;
	int rc = 0;

	if (spec == NULL || spec->port == NULL) {
		LOG_ERR("GPIO6 GPIO spec is not configured properly");
		return;
	}

	if (!(pins & BIT(spec->pin))) {
		return; /*  not our pin */
	}

	bool pin_state = gpio_pin_get_dt(spec);

	LOG_DBG("GPIO6 ISR callback %s %d %d", spec->port->name, spec->pin, pin_state);

	rc = pm_device_state_get(config->uart, &state);
	if (rc == 0) {
		LOG_DBG("PM state %d ret: %d", state, rc);
	}

	if (!pin_state) {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_ASLEEP);
		data->status.pmc_power_down.status_previously =
			data->status.pmc_power_down.status_currently;
		data->status.pmc_power_down.status_currently = true;
	} else {
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_DEVICE_AWAKE);
		data->status.pmc_power_down.status_previously =
			data->status.pmc_power_down.status_currently;
		data->status.pmc_power_down.status_currently = false;
	}
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
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	switch (evt) {
	case MODEM_HL78XX_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		modem_chat_run_script_async(&data->chat, config->init_chat_script);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT);

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

MODEM_CHAT_SCRIPT_CMDS_DEFINE(init_fail_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP?", ksrep_match));

MODEM_CHAT_SCRIPT_DEFINE(init_fail_script, init_fail_script_cmds, abort_matches,
			 hl78xx_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(swir_hl78xx_enable_ksup_urc_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP?", ksrep_match));

MODEM_CHAT_SCRIPT_DEFINE(swir_hl78xx_enable_ksup_urc_script, swir_hl78xx_enable_ksup_urc_cmds,
			 abort_matches, hl78xx_chat_callback_handler, 4);

static int hl78xx_on_run_init_diagnose_script_state_enter(struct hl78xx_data *data)
{
	modem_chat_run_script_async(&data->chat, &init_fail_script);
	return 0;
}
static void hl78xx_run_init_fail_script_event_handler(struct hl78xx_data *data,
						      enum hl78xx_event evt)
{
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;

	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		if (data->status.ksrep == 0) {
			modem_chat_run_script_async(&data->chat,
						    &swir_hl78xx_enable_ksup_urc_script);
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

static int hl78xx_rat_cfg(struct hl78xx_data *data, bool *modem_require_restart,
			  enum hl78xx_cell_rat_mode *rat_request)
{
	int ret = 0;
#if defined(CONFIG_MODEM_HL78XX_AUTORAT)
	/* Check autorat status/configs */
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_AUTORAT_OVER_WRITE_PRL) ||
	    (data->kselacq_data.rat1 == 0 && data->kselacq_data.rat2 == 0 &&
	     data->kselacq_data.rat3 == 0)) {
		char cmd_kselq[] = "AT+KSELACQ=0," CONFIG_MODEM_HL78XX_AUTORAT_PRL_PROFILES;
		/* Re-congfiguring PRL context definition */
		ret = modem_cmd_send_int(data, NULL, cmd_kselq, strlen(cmd_kselq), &ok_match, 1,
					 false);
		if (ret < 0) {
			goto error;
		} else {
			*modem_require_restart = true;
		}
	}

	*rat_request = HL78XX_RAT_MODE_AUTO;
#else
	/* Check a active rat config  */
	if (data->kselacq_data.rat1 != 0 && data->kselacq_data.rat2 != 0 &&
	    data->kselacq_data.rat3 != 0) {
		char const *cmd_kselq_disable = (const char *)DISABLE_RAT_AUTO;

		/* Re-congfiguring PRL context definition */
		ret = modem_cmd_send_int(data, NULL, cmd_kselq_disable, strlen(cmd_kselq_disable),
					 &ok_match, 1, false);
		if (ret < 0) {
			goto error;
		}
	}

	char const *cmd_ksrat_query = (const char *)KSRAT_QUERY;

	/* Re-congfiguring PRL context definition */
	ret = modem_cmd_send_int(data, NULL, cmd_ksrat_query, strlen(cmd_ksrat_query), &ksrat_match,
				 1, false);
	if (ret < 0) {
		goto error;
	}
#if !defined(CONFIG_MODEM_HL78XX_RAT_M1) && !defined(CONFIG_MODEM_HL78XX_RAT_NB1) &&               \
	!defined(CONFIG_MODEM_HL78XX_RAT_GSM) && !defined(CONFIG_MODEM_HL78XX_RAT_NBNTN)
#error "No rat has been selected."
#endif
	const char *cmd_set_rat = NULL;

	if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_M1)) {
		cmd_set_rat = (const char *)SET_RAT_M1_CMD_LEGACY;

		*rat_request = HL78XX_RAT_CAT_M1;
	} else if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_NB1)) {
		cmd_set_rat = (const char *)SET_RAT_NB1_CMD_LEGACY;

		*rat_request = HL78XX_RAT_NB1;
	}
#ifdef CONFIG_MODEM_HL7812
	else if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_GSM)) {

		cmd_set_rat = (const char *)SET_RAT_GSM_CMD_LEGACY;

		*rat_request = HL78XX_RAT_GSM;
	}

#ifdef CONFIG_MODEM_FW_R6
	else if (IS_ENABLED(CONFIG_MODEM_HL78XX_RAT_NBNTN)) {
		cmd_set_rat = (const char *)SET_RAT_NBNTN_CMD_LEGACY;

		*rat_request = HL78XX_RAT_NBNTN;
	}
#endif /* CONFIG_MODEM_FW_R6 */

#endif
	else {
		LOG_ERR("%d %s No rat has been selected.", __LINE__, __func__);
	}

	if (cmd_set_rat == NULL || *rat_request == HL78XX_RAT_MODE_NONE) {
		ret = -EINVAL;
		goto error;
	}

	if (*rat_request != data->status.registration.rat_mode) {
		ret = modem_cmd_send_int(data, NULL, cmd_set_rat, strlen(cmd_set_rat), &ok_match, 1,
					 false);
		if (ret < 0) {
			goto error;
		} else {
			*modem_require_restart = true;
		}
	}

#endif /*  CONFIG_MODEM_HL78XX_AUTORAT  */

error:
	return ret;
}

static int hl78xx_band_cfg(struct hl78xx_data *data, bool *modem_require_restart,
			   enum hl78xx_cell_rat_mode rat_config_request)
{
	int ret = 0;
	char bnd_bitmap[MDM_BAND_HEX_STR_LEN] = {0};

	if (rat_config_request == HL78XX_RAT_MODE_NONE) {
		return -EINVAL;
	}
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	/* In Auto-RAT mode, configure both M1 and NB-IoT band configs */
	for (int rat = HL78XX_RAT_CAT_M1; rat <= HL78XX_RAT_NB1; rat++) {
		if (rat == HL78XX_RAT_GSM) {
			continue; /* skip unsupported RAT for band config */
		}
#else
	/* Otherwise, just configure for the requested RAT */
	int rat = rat_config_request;
#endif
		ret = hl78xx_get_band_default_config_for_rat(rat, bnd_bitmap,
							     ARRAY_SIZE(bnd_bitmap));
		if (ret) {
			LOG_ERR("%d %s error get band default config %d", __LINE__, __func__, ret);
			goto error;
		}
		const char *modem_trimmed =
			hl78xx_trim_leading_zeros(data->status.kbndcfg[rat].bnd_bitmap);
		const char *expected_trimmed = hl78xx_trim_leading_zeros(bnd_bitmap);

		if (strcmp(modem_trimmed, expected_trimmed) != 0) {
			char cmd_bnd[80] = {0};

			snprintf(cmd_bnd, sizeof(cmd_bnd), "AT+KBNDCFG=%d,%s", rat,
				 bnd_bitmap); /*  RAT=0 for CAT-M1 */
			ret = modem_cmd_send_int(data, NULL, cmd_bnd, strlen(cmd_bnd), &ok_match, 1,
						 false);
			if (ret < 0) {
				goto error;
			} else {
				*modem_require_restart |= true;
			}
		} else {
			LOG_DBG("The band configs (%s) matched with exist configs (%s) for rat: "
				"[%d]",
				modem_trimmed, expected_trimmed, rat);
		}
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	}
#endif
error:
	return ret;
}

static int hl78xx_on_rat_cfg_script_state_enter(struct hl78xx_data *data)
{
	int ret = 0;
	bool modem_require_restart = false;
	const struct hl78xx_config *config = (const struct hl78xx_config *)data->dev->config;
	enum hl78xx_cell_rat_mode rat_config_request = HL78XX_RAT_MODE_NONE;

	ret = hl78xx_rat_cfg(data, &modem_require_restart, &rat_config_request);
	if (ret < 0) {
		goto error;
	}

	ret = hl78xx_band_cfg(data, &modem_require_restart, rat_config_request);
	if (ret < 0) {
		goto error;
	}

	if (modem_require_restart) {
		const char *cmd_restart = (const char *)SET_AIRPLANE_MODE_CMD;

		ret = modem_cmd_send_int(data, NULL, cmd_restart, strlen(cmd_restart), &ok_match, 1,
					 false);
		if (ret < 0) {
			goto error;
		}
		hl78xx_start_timer(data, K_MSEC(config->shutdown_time_ms));
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
	char const *cmd_ksrat_query = (const char *)KSRAT_QUERY;

	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		LOG_DBG("Rebooting modem to apply new RAT settings");
		ret = modem_cmd_send_int(data, NULL, NULL, 0, &at_ready_match, 1, false);
		if (ret < 0) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SUSPEND);
		}

		/* Re-check if rat config is correct */
		ret = modem_cmd_send_int(data, NULL, cmd_ksrat_query, strlen(cmd_ksrat_query),
					 &ksrat_match, 1, false);
		if (ret < 0) {
			hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SUSPEND);
		}

		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_SUCCESS);
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

static int hl78xx_on_run_rat_cfg_script_state_leave(struct hl78xx_data *data)
{
	return 0;
}
#ifndef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
MODEM_CHAT_SCRIPT_CMDS_DEFINE(hl78xx_disable_pmc_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSLEEP=2", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPSMS=0", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KEDRXCFG=0", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(hl78xx_disable_pmc_chat_script, hl78xx_disable_pmc_chat_script_cmds,
			 abort_matches, hl78xx_chat_callback_handler, 10);

static int hl78xx_disabe_pmc(struct hl78xx_data *data)
{
	LOG_DBG("%d Disabling Power Management Config", __LINE__);
	return modem_chat_run_script_async(&data->chat, &hl78xx_disable_pmc_chat_script);
}
#else
static int hl78xx_enable_pmc(struct hl78xx_data *data)
{
	const char *turn_on_pmc_cmd = "AT+KSLEEP=1,2,0";

	LOG_DBG("%d Enabling Power Management Config", __LINE__);
	return modem_cmd_send_int(data, NULL, turn_on_pmc_cmd, strlen(turn_on_pmc_cmd), &ok_match,
				  1, false);
}

static int hl78xx_psm_settings(struct hl78xx_data *data)
{
	if (data->status.registration.rat_mode != HL78XX_RAT_NB1 &&
	    data->status.registration.rat_mode != HL78XX_RAT_CAT_M1) {
		LOG_DBG("PSM is not supported for RAT mode: %d",
			data->status.registration.rat_mode);
		return 0;
	}
#ifdef CONFIG_MODEM_HL78XX_PSM
	if (data->status.pmc_cpsms.mode == false) {
		const char *turn_on_psm_cmd = "AT+CPSMS=1,,,\"" CONFIG_MODEM_HL78XX_PSM_PERIODIC_TAU
					      "\",\"" CONFIG_MODEM_HL78XX_PSM_ACTIVE_TIME "\"";

		return modem_cmd_send_int(data, NULL, turn_on_psm_cmd, strlen(turn_on_psm_cmd),
					  &ok_match, 1, false);
	}
#else
	if (data->status.pmc_cpsms.mode == true) {
		const char *turn_off_psm_cmd = "AT+CPSMS=0";

		return modem_cmd_send_int(data, NULL, turn_off_psm_cmd, strlen(turn_off_psm_cmd),
					  &ok_match, 1, false);
	}
#endif
	LOG_DBG("PSM is already configured for RAT mode: %d", data->status.registration.rat_mode);
	return 0; /* PSM already disabled */
}

static int hl78xx_edrx_settings(struct hl78xx_data *data)
{
	if (data->status.registration.rat_mode != HL78XX_RAT_NB1 &&
	    data->status.registration.rat_mode != HL78XX_RAT_CAT_M1) {
		LOG_DBG("eDRX is not supported for RAT mode: %d",
			data->status.registration.rat_mode);
		return 0;
	}
#ifdef CONFIG_MODEM_HL78XX_EDRX
	if (data->status.pmc_kedrxcfg[data->status.registration.rat_mode].mode ==
		    HL78XX_KEDRX_MODE_DISABLE ||
	    data->status.pmc_kedrxcfg[data->status.registration.rat_mode].mode ==
		    HL78XX_KEDRX_MODE_DISABLE_AND_ERASE_CFG) {
		char turn_on_edrx_cmd[sizeof("AT+KEDRXCFG=1,X,XXXX,XXXX")] = {0};
		uint8_t ack_type = 4;

		ack_type = (data->status.registration.rat_mode == HL78XX_RAT_NB1) ? 5 : ack_type;

		snprintf(turn_on_edrx_cmd, sizeof(turn_on_edrx_cmd), "AT+KEDRXCFG=1,%hhu,%d,%d",
			 ack_type, CONFIG_MODEM_HL78XX_EDRX_VALUE, CONFIG_MODEM_HL78XX_PTW_VALUE);

		return modem_cmd_send_int(data, NULL, turn_on_edrx_cmd, strlen(turn_on_edrx_cmd),
					  &ok_match, 1, false);
	}
#else
	if (data->status.pmc_kedrxcfg[data->status.registration.rat_mode].mode ==
		    HL78XX_KEDRX_MODE_ENABLE ||
	    data->status.pmc_kedrxcfg[data->status.registration.rat_mode].mode ==
		    HL78XX_KEDRX_MODE_ENABLE_W_URC) {
		char *turn_off_edrx_cmd = "AT+KEDRXCFG=0";

		return modem_cmd_send_int(data, NULL, turn_off_edrx_cmd, strlen(turn_off_edrx_cmd),
					  &ok_match, 1, false);
	}
#endif

	LOG_DBG("eDRX is already configured for RAT mode: %d", data->status.registration.rat_mode);
	return 0;
}
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
void hl78xx_pwr_dwn_work_handler(struct k_work *work_item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work_item);
	struct hl78xx_data *data = CONTAINER_OF(dwork, struct hl78xx_data, hl78xx_pwr_dwn_work);

	LOG_DBG("%d %s: Power down work handler called", __LINE__, __func__);
	hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
	data->status.pmc_power_down.requested_previously =
		data->status.pmc_power_down.requested_currently;
	data->status.pmc_power_down.requested_currently = true;
}

static int hl78xx_pwr_dwn_feed_timer(struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_USE_DELAY_BASED_POWER_DOWN
	k_work_reschedule(&data->hl78xx_pwr_dwn_work,
			  K_SECONDS(CONFIG_MODEM_HL78XX_POWER_DOWN_DELAY));
#else
	k_work_reschedule(&data->hl78xx_pwr_dwn_work,
			  K_SECONDS(CONFIG_MODEM_HL78XX_POWER_DOWN_ACTIVE_TIME));
#endif
	data->status.pmc_power_down.requested_previously =
		data->status.pmc_power_down.requested_currently;
	data->status.pmc_power_down.requested_currently = false;

	return 0;
}

static int hl78xx_pwr_dwn_settings(struct hl78xx_data *data)
{
	LOG_DBG("%d Modem Power Down Settings", __LINE__);
	return 0;
}
#endif
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

static int hl78xx_on_pmc_cfg_script_state_enter(struct hl78xx_data *data)
{
	int ret = 0;
	bool modem_require_restart = false;

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	hl78xx_enable_pmc(data);
	hl78xx_psm_settings(data);
	hl78xx_edrx_settings(data);
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	hl78xx_pwr_dwn_settings(data);
#endif
#else
	hl78xx_disabe_pmc(data);
	LOG_DBG("%d Disabling Power Management Config", __LINE__);
#endif
	if (modem_require_restart) {
		const char *cmd_restart = (const char *)SET_AIRPLANE_MODE_CMD;

		LOG_DBG("%d Reset required", __LINE__);

		ret = modem_cmd_send_int(data, NULL, cmd_restart, strlen(cmd_restart), &ok_match, 1,
					 false);
		if (ret < 0) {
			goto error;
		}
		hl78xx_start_timer(data, K_MSEC(100));
		return 0;
	}
#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
	hl78xx_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_SUCCESS, data);
#endif
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
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT);
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

#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_KCONFIG)
	snprintf(data->identity.apn, sizeof(data->identity.apn), "%s", CONFIG_MODEM_HL78XX_APN);
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
static int hl78xx_on_await_wakeup_state_enter(struct hl78xx_data *data)
{
	hl78xx_start_timer(data, MODEM_HL78XX_PERIODIC_SCRIPT_TIMEOUT);
	return 0;
}

static void hl78xx_await_wakeup_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		break;
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		break;
	case MODEM_HL78XX_EVENT_DEVICE_ASLEEP:
		break;
	case MODEM_HL78XX_EVENT_DEVICE_AWAKE:
		hl78xx_stop_timer(data);
		modem_pipe_attach(data->uart_pipe, hl78xx_bus_pipe_handler, data);
		modem_pipe_open_async(data->uart_pipe);
		break;
	case MODEM_HL78XX_EVENT_TIMEOUT:
		/* If timeout occurs, that means the device could not be waken up for
		 * MODEM_HL78XX_PERIODIC_SCRIPT_TIMEOUT,
		 * so take some action here
		 */
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
	/* Start timer to check if psm event received */
	hl78xx_start_timer(data, K_MSEC(MDM_PSMEVT_RECEIVE_TIMEOUT));
	return 0;
}

static void hl78xx_await_registered_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		if (data->status.psm.psmev_current == HL78XX_PSM_EVENT_ENTER) {
			/* Just confirm the previous state was registered */
			if (data->status.registration.is_registered_previously) {
				LOG_DBG("%d: PSM event received, modem is in PSM mode", __LINE__);
				hl78xx_enter_state(data, MODEM_HL78XX_STATE_IDLE);
			}
		} else {
			if (data->status.registration.is_registered_previously) {
				if (data->status.psm.psmev_current == HL78XX_PSM_EVENT_ENTER) {
					LOG_DBG("%d: PSM exit requested, waiting for "
						"registration",
						__LINE__);
				}
				/* Most likely out of coverage. take a action here */
			} else {
				/* There was no previous registration state */
				/* Do nothing wait for it if searching is in progress */
				LOG_DBG("%d waiting for registration : Current network "
					"state: %d",
					__LINE__, data->status.registration.network_state_current);
			}
		}
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
	LOG_DBG("%d %s: Entering carrier on state %d %d %d %d", __LINE__, __func__,
		data->status.registration.is_registered_previously,
		data->status.registration.is_registered_currently,
		data->status.registration.network_state_current,
		data->status.registration.network_state_previous);
	if (!data->status.psm.psmev_previous && !data->status.psm.psmev_current &&
	    data->status.registration.network_state_previous != HL78XX_REGISTRATION_UNKNOWN) {
		iface_status_work_cb(data, hl78xx_chat_callback_handler);
	}

	notif_carrier_on();
	LOG_DBG("%d psm %d %d", __LINE__, data->status.psm.psmev_previous,
		data->status.psm.psmev_current);
	return 0;
}

static void hl78xx_carrier_on_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		if (!data->status.psm.psmev_previous && !data->status.psm.psmev_current &&
		    data->status.registration.network_state_previous !=
			    HL78XX_REGISTRATION_UNKNOWN) {
			hl78xx_start_timer(data, K_SECONDS(2));
		}
		break;
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		break;
	case MODEM_HL78XX_EVENT_TIMEOUT:
		LOG_DBG("%d %d %d %d", __LINE__, data->status.psm.psmev_previous,
			data->status.psm.psmev_current,
			data->status.registration.network_state_previous);
		LOG_DBG("%d %d %d", __LINE__, data->status.pmc_power_down.requested_previously,
			data->status.pmc_power_down.requested_currently);
		if (data->status.pmc_power_down.requested_previously &&
		    !data->status.pmc_power_down.requested_currently) {
			dns_work_cb(true);
			hl78xx_release_socket_comms();

		} else if ((!data->status.psm.psmev_previous && !data->status.psm.psmev_current &&
			    data->status.registration.network_state_previous !=
				    HL78XX_REGISTRATION_UNKNOWN)) {
			dns_work_cb(false);
		} else {
			LOG_DBG("%d Unexpected condition in carrier on state: %d", __LINE__, evt);
		}
		break;
	case MODEM_HL78XX_EVENT_DEREGISTERED:
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_REGISTERED);
		break;
	case MODEM_HL78XX_EVENT_SUSPEND:
		LOG_DBG("%d %s: Modem is suspended, entering power off state", __LINE__, __func__);
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;
	case MODEM_HL78XX_EVENT_SOCKET_CLOSED:
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
	LOG_DBG("%d carrier off", __LINE__);
	notif_carrier_off();
	/* Check whether or not there is any sockets are connected,
	 * if true, wait until sockets are closed properly
	 */
	hl78xx_start_timer(data, K_MSEC(100));
	if (check_if_any_socket_connected() == false) {
		LOG_DBG("%d THERE is socket connected", __LINE__);
	}
	return 0;
}

static void hl78xx_carrier_off_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
	case MODEM_HL78XX_EVENT_TIMEOUT:
		notif_carrier_on();
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_CARRIER_ON);
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

MODEM_CHAT_SCRIPT_CMDS_DEFINE(swir_hl78xx_pwroff_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP(SET_AIRPLANE_MODE_CMD_LEGACY, ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPWROFF", ok_match),
			      /*  MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP?", ksrep_match) */);

MODEM_CHAT_SCRIPT_DEFINE(swir_hl78xx_pwroff_script, swir_hl78xx_pwroff_cmds, abort_matches,
			 hl78xx_chat_callback_handler, 4);

static int hl78xx_on_init_power_off_state_enter(struct hl78xx_data *data)
{
	return modem_chat_run_script_async(&data->chat, &swir_hl78xx_pwroff_script);
}

static void hl78xx_init_power_off_event_handler(struct hl78xx_data *data, enum hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		notif_carrier_off();
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
	modem_chat_release(&data->chat);
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
		LOG_DBG("%d: Wake pin is enabled, setting it to low", __LINE__);
		gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
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
#if defined CONFIG_MODEM_HL78XX_LOW_POWER_MODE && defined CONFIG_PM_DEVICE
	enum pm_device_state state;
	enum pm_device_action action;
	int rc;
#endif
	switch (evt) {
	case MODEM_HL78XX_EVENT_BUS_CLOSED:
#if defined CONFIG_MODEM_HL78XX_LOW_POWER_MODE && defined CONFIG_PM_DEVICE
		rc = pm_device_state_get(config->uart, &state);
		if (rc == 0) {
			LOG_DBG("PM state %d ret: %d", state, rc);
		}
		action = PM_DEVICE_ACTION_SUSPEND;
		uart_irq_rx_disable(config->uart);
		rc = pm_device_action_run(config->uart, action);
		if (rc == 0 || rc == -EALREADY) {
			LOG_DBG("PM action run: %d", rc);
		} else {
			LOG_DBG("PM action run failed: %d", rc);
		}
#else
		LOG_DBG("%d: Bus closed, entering idle state", __LINE__);
#endif
		break;

	case MODEM_HL78XX_EVENT_RESUME:
		if (data->status.psm.psmev_current == HL78XX_PSM_EVENT_ENTER) {
			hl78xx_enter_state(data, MODEM_HL78XX_STATE_AWAIT_WAKEUP);

			break;
		}
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
		hl78xx_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
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
#if defined CONFIG_MODEM_HL78XX_LOW_POWER_MODE && defined CONFIG_PM_DEVICE
	enum pm_device_state state;
	enum pm_device_action action;
	int rc = 0;
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE && CONFIG_PM_DEVICE */
	k_sem_take(&data->suspended_sem, K_NO_WAIT);

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
		gpio_pin_set_dt(&config->mdm_gpio_reset, 0);
	}

	if (hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
	}
#if defined CONFIG_MODEM_HL78XX_LOW_POWER_MODE && defined CONFIG_PM_DEVICE
	rc = pm_device_state_get(config->uart, &state);
	if (rc == 0) {
		LOG_DBG("PM state: %d ret: %d", state, rc);
	} else {
		LOG_DBG("PM state get failed: %d", rc);
	}
	action = PM_DEVICE_ACTION_RESUME;
	rc = pm_device_action_run(config->uart, action);
	if (rc == 0 || rc == -EALREADY) {
		LOG_DBG("PM action run: %d", rc);
	} else {
		LOG_DBG("PM action run failed: %d", rc);
	}
	uart_irq_rx_enable(config->uart);
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE && CONFIG_PM_DEVICE */
	return 0;
}

static int hl78xx_on_state_enter(struct hl78xx_data *data)
{
	int ret = 0;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d", __LINE__, data->status.state);
#endif
	switch (data->status.state) {
	case MODEM_HL78XX_STATE_IDLE:
		ret = hl78xx_on_idle_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_RESET_PULSE:
		ret = hl78xx_on_reset_pulse_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_POWER_ON_PULSE:
		ret = hl78xx_on_power_on_pulse_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_POWER_ON:
		ret = hl78xx_on_await_power_on_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_SET_BAUDRATE:
		break;

	case MODEM_HL78XX_STATE_RUN_INIT_SCRIPT:
		ret = hl78xx_on_run_init_script_state_enter(data);
		break;
	case MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT:
		ret = hl78xx_on_run_init_diagnose_script_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		ret = hl78xx_on_rat_cfg_script_state_enter(data);
		break;
	case MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT:
		ret = hl78xx_on_pmc_cfg_script_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT:
		ret = hl78xx_on_enable_gprs_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_WAKEUP:
		ret = hl78xx_on_await_wakeup_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		ret = hl78xx_on_await_registered_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_CARRIER_ON:
		ret = hl78xx_on_carrier_on_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_CARRIER_OFF:
		ret = hl78xx_on_carrier_off_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		ret = hl78xx_on_init_power_off_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		ret = hl78xx_on_power_off_pulse_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_POWER_OFF:
		ret = hl78xx_on_await_power_off_state_enter(data);
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

static int hl78xx_on_state_leave(struct hl78xx_data *data)
{
	int ret = 0;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d", __LINE__, data->status.state);
#endif
	switch (data->status.state) {
	case MODEM_HL78XX_STATE_IDLE:
		ret = hl78xx_on_idle_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_RESET_PULSE:
		ret = hl78xx_on_reset_pulse_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_POWER_ON_PULSE:
		ret = hl78xx_on_power_on_pulse_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		ret = hl78xx_on_run_rat_cfg_script_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT:
		ret = hl78xx_on_run_pmc_cfg_script_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		ret = hl78xx_on_await_registered_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_CARRIER_ON:
		ret = hl78xx_on_carrier_on_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_CARRIER_OFF:
		ret = hl78xx_on_carrier_off_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		ret = hl78xx_on_init_power_off_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		ret = hl78xx_on_power_off_pulse_state_leave(data);
		break;

	default:
		ret = 0;
		break;
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

	state = data->status.state;

	hl78xx_log_event(evt);

	switch (data->status.state) {
	case MODEM_HL78XX_STATE_IDLE:
		hl78xx_idle_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RESET_PULSE:
		hl78xx_reset_pulse_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_POWER_ON_PULSE:
		hl78xx_power_on_pulse_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_AWAIT_POWER_ON:
		hl78xx_await_power_on_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_SET_BAUDRATE:
		break;

	case MODEM_HL78XX_STATE_RUN_INIT_SCRIPT:
		hl78xx_run_init_script_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT:
		hl78xx_run_init_fail_script_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		hl78xx_run_rat_cfg_script_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RUN_PMC_CONFIG_SCRIPT:
		hl78xx_run_pmc_cfg_script_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT:
		hl78xx_enable_gprs_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_AWAIT_WAKEUP:
		hl78xx_await_wakeup_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		hl78xx_await_registered_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_CARRIER_ON:
		hl78xx_carrier_on_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_CARRIER_OFF:
		hl78xx_carrier_off_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		hl78xx_init_power_off_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		hl78xx_power_off_pulse_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_AWAIT_POWER_OFF:
		hl78xx_await_power_off_event_handler(data, evt);
		break;
	default:
		LOG_ERR("%d %s unknown event", __LINE__, __func__);
		break;
	}

	if (state != data->status.state) {
		hl78xx_log_state_changed(state, data->status.state);
	}
}

#ifdef CONFIG_PM_DEVICE
static int hl78xx_driver_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	int ret = 0;

	LOG_WRN("%d %s PM_DEVICE_ACTION: %d", __LINE__, __func__, action);
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* suspend the device */
		LOG_DBG("%d %s PM_DEVICE_ACTION_SUSPEND", __LINE__, __func__);
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_SUSPEND);
		ret = k_sem_take(&data->suspended_sem, K_SECONDS(30));
		break;
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("%d %s PM_DEVICE_ACTION_RESUME", __LINE__, __func__);
		/* resume the device */
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * powered on the device, used when the power
		 * domain this device belongs is resumed.
		 */
		LOG_DBG("%d %s PM_DEVICE_ACTION_TURN_ON", __LINE__, __func__);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		/*
		 * power off the device, used when the power
		 * domain this device belongs is suspended.
		 */
		LOG_DBG("%d %s PM_DEVICE_ACTION_TURN_OFF", __LINE__, __func__);
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int hl78xx_init(const struct device *dev)
{
	int ret;
	const struct hl78xx_config *config = (const struct hl78xx_config *)dev->config;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	data->dev = dev;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	/* Initialize work queue and event handling */
	k_work_queue_start(&modem_workq, modem_workq_stack,
			   K_KERNEL_STACK_SIZEOF(modem_workq_stack), K_PRIO_COOP(7), NULL);
	k_work_init_delayable(&data->timeout_work, hl78xx_timeout_handler);
	k_work_init(&data->events.event_dispatch_work, hl78xx_event_dispatch_handler);
	ring_buf_init(&data->events.event_rb, sizeof(data->events.event_buf),
		      data->events.event_buf);
	k_sem_init(&data->suspended_sem, 0, 1);
	k_sem_init(&data->script_stopped_sem_tx_int, 0, 1);
	k_sem_init(&data->script_stopped_sem_rx_int, 0, 1);
#ifdef CONFIG_MODEM_HL78XX_POWER_DOWN
	k_work_init_delayable(&data->hl78xx_pwr_dwn_work, hl78xx_pwr_dwn_work_handler);
#endif
	/* reset to default  */
	data->buffers.eof_pattern_size = strlen(data->buffers.eof_pattern);
	memset(data->identity.apn, 0, MDM_APN_MAX_LENGTH);

	/* GPIO validation */
	const struct gpio_dt_spec *gpio_pins[GPIO_CONFIG_LEN] = {
		&config->mdm_gpio_reset,        &config->mdm_gpio_wake, &config->mdm_gpio_vgpio,
#if HAS_GPIO6_GPIO
		&config->mdm_gpio_gpio6,
#endif
#if HAS_PWR_ON_GPIO
		&config->mdm_gpio_pwr_on,
#endif
#if HAS_FAST_SHUTD_GPIO
		&config->mdm_gpio_fast_shtdown,
#endif
#if HAS_UART_DTR_GPIO
		&config->mdm_gpio_uart_dtr,
#endif
	};

	for (int i = 0; i < ARRAY_SIZE(gpio_pins); i++) {
		if (!gpio_is_ready_dt(gpio_pins[i])) {
			LOG_ERR("GPIO port (%s) not ready!", gpio_pins[i]->port->name);
			return -ENODEV;
		}
	}

	/* GPIO configuration */
	struct {
		const struct gpio_dt_spec *spec;
		gpio_flags_t flags;
		const char *name;
	} gpio_config[GPIO_CONFIG_LEN] = {
		{&config->mdm_gpio_reset, GPIO_OUTPUT, "reset"},
		{&config->mdm_gpio_wake, GPIO_OUTPUT, "wake"},
		{&config->mdm_gpio_vgpio, GPIO_INPUT, "VGPIO"},
#if HAS_PWR_ON_GPIO
		{&config->mdm_gpio_pwr_on, GPIO_OUTPUT, "pwr_on"},
#endif
#if HAS_FAST_SHUTD_GPIO
		{&config->mdm_gpio_fast_shtdown, GPIO_OUTPUT, "fast_shutdown"},
#endif
#if HAS_UART_DTR_GPIO
		{&config->mdm_gpio_uart_dtr, GPIO_OUTPUT, "DTR"},
#endif
#if HAS_GPIO6_GPIO
		{&config->mdm_gpio_gpio6, GPIO_INPUT, "GPIO6"},
#endif
	};

	for (int i = 0; i < ARRAY_SIZE(gpio_config); i++) {
		ret = gpio_pin_configure_dt(gpio_config[i].spec, gpio_config[i].flags);
		if (ret < 0) {
			LOG_ERR("Failed to configure %s pin", gpio_config[i].name);
			goto error;
		}
	}

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
#if HAS_GPIO6_GPIO
	LOG_DBG("%d %s: GPIO6 is enabled", __LINE__, __func__);
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
	(void)hl78xx_init_pipe(dev);

	ret = modem_init_chat(dev);
	if (ret < 0) {
		goto error;
	}

	hl78xx_socket_init(data);

#ifndef CONFIG_PM_DEVICE
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
#else
	pm_device_init_suspended(dev);
#endif /* CONFIG_PM_DEVICE */

	return 0;

error:
	return ret;
}

int hl78xx_evt_notif_handler_set(hl78xx_evt_monitor_handler_t handler)
{
	event_dispatcher = handler;
	return 0;
}

static DEVICE_API(hl78xx, hl78xx_api) = {

	.get_signal = hl78xx_api_func_get_signal,
	.get_registration_status = hl78xx_api_func_get_registration_status,
	.get_modem_info = hl78xx_api_func_get_modem_info,
	.set_apn = hl78xx_api_func_set_apn,
	.set_phone_functionality = hl78xx_api_func_set_phone_functionality,
	.get_phone_functionality = hl78xx_api_func_get_phone_functionality,
	.send_at_cmd = hl78xx_api_func_modem_cmd_send_int,
};

#define MODEM_HL78XX_DEFINE_INSTANCE(inst, power_ms, reset_ms, startup_ms, shutdown_ms, start,     \
				     init_script, periodic_script)                                 \
	static const struct hl78xx_config hl78xx_cfg_##inst = {                                    \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.mdm_gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),             \
		.mdm_gpio_wake = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_wake_gpios, {}),               \
		.mdm_gpio_pwr_on = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_pwr_on_gpios, {}),           \
		.mdm_gpio_fast_shtdown = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_fast_shutd_gpios, {}), \
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
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, hl78xx_driver_pm_action);                                   \
	DEVICE_DT_INST_DEFINE(inst, hl78xx_init, PM_DEVICE_DT_INST_GET(inst), &hl78xx_data_##inst, \
			      &hl78xx_cfg_##inst, POST_KERNEL,                                     \
			      CONFIG_MODEM_HL78XX_DEV_INIT_PRIORITY, &hl78xx_api);

#define MODEM_DEVICE_SWIR_HL78XX(inst)                                                             \
	MODEM_HL78XX_DEFINE_INSTANCE(inst, CONFIG_MODEM_HL78XX_DEV_POWER_PULSE_DURATION,           \
				     CONFIG_MODEM_HL78XX_DEV_RESET_PULSE_DURATION,                 \
				     CONFIG_MODEM_HL78XX_DEV_STARTUP_TIME,                         \
				     CONFIG_MODEM_HL78XX_DEV_SHUTDOWN_TIME, false,                 \
				     &hl78xx_init_chat_script, &hl78xx_periodic_chat_script)

#define DT_DRV_COMPAT swir_hl7812
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
