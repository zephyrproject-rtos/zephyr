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

#include <string.h>
#include <stdlib.h>
#include "hl78xx.h"

#define MAX_SCRIPT_AT_CMD_RETRY 3

LOG_MODULE_REGISTER(hl78xx_dev, CONFIG_MODEM_LOG_LEVEL);

/* RX thread work queue */
K_KERNEL_STACK_DEFINE(modem_workq_stack, CONFIG_MODEM_HL78XX_RX_WORKQ_STACK_SIZE);

static struct k_work_q modem_workq;

static int modem_cmd_send_int(struct modem_hl78xx_data *user_data,
			      modem_chat_script_callback script_user_callback, const uint8_t *cmd,
			      uint16_t cmd_len, const struct modem_chat_match *response_matches,
			      uint16_t matches_size);

static void modem_cellular_enter_state(struct modem_hl78xx_data *data,
				       enum modem_hl78xx_state state);
static void modem_cellular_delegate_event(struct modem_hl78xx_data *data,
					  enum modem_hl78xx_event evt);
static void modem_cellular_event_handler(struct modem_hl78xx_data *data,
					 enum modem_hl78xx_event evt);
static int modem_cellular_on_idle_state_enter(struct modem_hl78xx_data *data);
static void modem_cellular_begin_power_off_pulse(struct modem_hl78xx_data *data);

static const char *modem_cellular_state_str(enum modem_hl78xx_state state)
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
	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		return "await registered";
	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		return "run rat cfg script";
	case MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT:
		return "run enable gprs script";
	case MODEM_HL78XX_STATE_CARRIER_ON:
		return "carrier on";
	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		return "init power off";
	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		return "power off pulse";
	case MODEM_HL78XX_STATE_AWAIT_POWER_OFF:
		return "await power off";
	}

	return "";
}
static const char *modem_cellular_event_str(enum modem_hl78xx_event event)
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
static bool modem_cellular_gpio_is_enabled(const struct gpio_dt_spec *gpio)
{
	return (gpio->port != NULL);
}

static void modem_cellular_log_event(enum modem_hl78xx_event evt)
{
	LOG_DBG("event %s", modem_cellular_event_str(evt));
}
static void modem_cellular_start_timer(struct modem_hl78xx_data *data, k_timeout_t timeout)
{
	k_work_schedule(&data->timeout_work, timeout);
}
static void modem_cellular_stop_timer(struct modem_hl78xx_data *data)
{
	k_work_cancel_delayable(&data->timeout_work);
}
static void modem_cellular_timeout_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_hl78xx_data *data =
		CONTAINER_OF(dwork, struct modem_hl78xx_data, timeout_work);

	modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_TIMEOUT);
}
static void modem_cellular_bus_pipe_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
					    void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_BUS_OPENED);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_BUS_CLOSED);
		break;

	default:
		break;
	}
}
static void modem_cellular_log_state_changed(enum modem_hl78xx_state last_state,
					     enum modem_hl78xx_state new_state)
{
	LOG_DBG("switch from %s to %s", modem_cellular_state_str(last_state),
		modem_cellular_state_str(new_state));
}
static void modem_cellular_event_dispatch_handler(struct k_work *item)
{
	struct modem_hl78xx_data *data =
		CONTAINER_OF(item, struct modem_hl78xx_data, event_dispatch_work);

	uint8_t events[sizeof(data->event_buf)];
	uint8_t events_cnt;

	k_mutex_lock(&data->event_rb_lock, K_FOREVER);

	events_cnt = (uint8_t)ring_buf_get(&data->event_rb, events, sizeof(data->event_buf));

	k_mutex_unlock(&data->event_rb_lock);

	for (uint8_t i = 0; i < events_cnt; i++) {
		modem_cellular_event_handler(data, (enum modem_hl78xx_event)events[i]);
	}
}

static void modem_cellular_delegate_event(struct modem_hl78xx_data *data,
					  enum modem_hl78xx_event evt)
{
	k_mutex_lock(&data->event_rb_lock, K_FOREVER);
	ring_buf_put(&data->event_rb, (uint8_t *)&evt, 1);
	k_mutex_unlock(&data->event_rb_lock);
	k_work_submit_to_queue(&modem_workq, &data->event_dispatch_work);
}

static void modem_cellular_chat_callback_handler(struct modem_chat *chat,
						 enum modem_chat_script_result result,
						 void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_SUCCESS);
	} else {
		modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_FAILED);
	}
}

static void modem_cellular_chat_on_cxreg(struct modem_chat *chat, char **argv, uint16_t argc,
					 void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;
	enum cellular_registration_status registration_status = 0;

	if (argc >= 2) {
		/* +CEREG: <stat>[,<tac>[...]] */
		registration_status = atoi(argv[1]);
	} else {
		return;
	}

	if (strcmp(argv[0], "+CREG: ") == 0 &&
	    data->mdm_registration_status.rat_mode == MDM_RAT_GSM) {
		data->mdm_registration_status.network_state = registration_status;
	} else if (strcmp(argv[0], "+CGREG: ") == 0) {
		data->mdm_registration_status.network_state = registration_status;
	} else { /* CEREG */
		data->mdm_registration_status.network_state = registration_status;
	}

	if (modem_cellular_is_registered(data)) {
		data->mdm_registration_status.is_registered = true;
		modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_REGISTERED);
	} else {
		data->mdm_registration_status.is_registered = false;
	}
}

static void modem_cellular_chat_on_psmev(struct modem_chat *chat, char **argv, uint16_t argc,
					 void *user_data)
{
	LOG_DBG("%d %s %d", __LINE__, __func__, argc);
	if (argc >= 2) {
		/* +CEREG: <stat>[,<tac>[...]] */
		LOG_DBG("%d %s %s", __LINE__, __func__, argv[1]);
	} else {
		LOG_DBG("%d %s %s", __LINE__, __func__, argv[0]);
	}
}

static void modem_cellular_chat_on_ksup(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	char ksup_data[2] = {0};

	strncpy(ksup_data, argv[1], sizeof(ksup_data) - 1);
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("Module status: %s", ksup_data);
#endif
}
static void modem_cellular_chat_on_imei(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("IMEI: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->imei, argv[1], sizeof(data->imei) - 1);
}

static void modem_cellular_chat_on_cgmm(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("cgmm: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->model_id, argv[1], sizeof(data->model_id) - 1);
}
static void modem_cellular_chat_on_imsi(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("IMSI: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->imsi, argv[1], sizeof(data->imsi) - 1);
#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
	/* set the APN automatically */
	modem_detect_apn(data, argv[1]);
#endif
}
static void modem_cellular_chat_on_cgmi(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("cgmi: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->manufacturer, argv[1], sizeof(data->manufacturer) - 1);
}

static void modem_cellular_chat_on_cgmr(struct modem_chat *chat, char **argv, uint16_t argc,
					void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("cgmr: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->fw_version, argv[1], sizeof(data->fw_version) - 1);
}
static void modem_cellular_chat_on_iccid(struct modem_chat *chat, char **argv, uint16_t argc,
					 void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc != 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("ICCID: %s %s", argv[0], argv[1]);
#endif
	strncpy(data->iccid, argv[1], sizeof(data->iccid) - 1);
#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID)
	/* set the APN automatically */
	modem_detect_apn(data, argv[1]);
#endif
}
/* Handler: +KSTATEV: */
static void modem_cellular_chat_on_kstatev(struct modem_chat *chat, char **argv, uint16_t argc,
					   void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc != 3) {
		return;
	}
	LOG_DBG("KSTATEV: %s %s %s", argv[0], argv[1], argv[2]);
	data->mdm_registration_status.rat_mode = ATOI(argv[2], 0, "rat_mode");
}

static void modem_cellular_chat_on_udprcv(struct modem_chat *chat, char **argv, uint16_t argc,
					  void *user_data)
{
	if (argc < 2) {
		return;
	}

	LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
}
static void modem_cellular_chat_on_socknotifydata(struct modem_chat *chat, char **argv,
						  uint16_t argc, void *user_data)
{
	int socket_id = -1, new_total = -1;

	if (argc < 2) {
		return;
	}
	socket_id = ATOI(argv[1], 0, "socket_id");
	new_total = ATOI(argv[2], 0, "length");
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %d %d", __LINE__, __func__, socket_id, new_total);
#endif
	socknotifydata(socket_id, new_total);
}
static void modem_cellular_chat_on_ksrep(struct modem_chat *chat, char **argv, uint16_t argc,
					 void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc < 2) {
		return;
	}

	data->ksrep = (uint8_t)atoi(argv[1]);
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("KSREP: %s %s", argv[0], argv[1]);
#endif
}
static void modem_cellular_chat_on_ksrat(struct modem_chat *chat, char **argv, uint16_t argc,
					 void *user_data)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)user_data;

	if (argc < 2) {
		return;
	}

	data->mdm_registration_status.rat_mode = (uint8_t)atoi(argv[1]);
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("KSREP: %s %s", argv[0], argv[1]);
#endif
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(allow_match, MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH("ERROR", "", NULL));

MODEM_CHAT_MATCHES_DEFINE(
	unsol_matches, MODEM_CHAT_MATCH("+CREG: ", ",", modem_cellular_chat_on_cxreg),
	MODEM_CHAT_MATCH("+CEREG: ", ",", modem_cellular_chat_on_cxreg),
	MODEM_CHAT_MATCH("+CGREG: ", ",", modem_cellular_chat_on_cxreg),
	MODEM_CHAT_MATCH("+PSMEV: ", "", modem_cellular_chat_on_psmev),
	MODEM_CHAT_MATCH("+KSTATEV: ", ",", modem_cellular_chat_on_kstatev),
	MODEM_CHAT_MATCH("+KUDP_DATA: ", ",", modem_cellular_chat_on_socknotifydata),
	MODEM_CHAT_MATCH("+KTCP_DATA: ", ",", modem_cellular_chat_on_socknotifydata),
	MODEM_CHAT_MATCH("+KUDP_RCV: ", ",", modem_cellular_chat_on_udprcv));

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("ERROR", "", NULL));
MODEM_CHAT_MATCH_DEFINE(at_ready_match, "+KSUP: ", "", modem_cellular_chat_on_ksup);
MODEM_CHAT_MATCH_DEFINE(imei_match, "", "", modem_cellular_chat_on_imei);
MODEM_CHAT_MATCH_DEFINE(cgmm_match, "", "", modem_cellular_chat_on_cgmm);
MODEM_CHAT_MATCH_DEFINE(cimi_match, "", "", modem_cellular_chat_on_imsi);
MODEM_CHAT_MATCH_DEFINE(cgmi_match, "", "", modem_cellular_chat_on_cgmi);
MODEM_CHAT_MATCH_DEFINE(cgmr_match, "", "", modem_cellular_chat_on_cgmr);
MODEM_CHAT_MATCH_DEFINE(iccid_match, "+CCID: ", "", modem_cellular_chat_on_iccid);
MODEM_CHAT_MATCH_DEFINE(ksrep_match, "+KSREP: ", ",", modem_cellular_chat_on_ksrep);
MODEM_CHAT_MATCH_DEFINE(ksrat_match, "+KSRAT: ", "", modem_cellular_chat_on_ksrat);

static void hl78xx_init_pipe(const struct device *dev)
{
	const struct modem_hl78xx_config *cfg = dev->config;
	struct modem_hl78xx_data *data = dev->data;

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = cfg->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = sizeof(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
}

static int modem_init_chat(const struct device *dev)
{
	struct modem_hl78xx_data *data = dev->data;

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_receive_buf,
		.receive_buf_size = sizeof(data->chat_receive_buf),
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

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	swir_hl78xx_init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("", at_ready_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KHWIOCFG=3,1,6", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGACT=0", allow_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=4", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSLEEP=2", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPSMS=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEDRXS=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KPATTERN="
				   "\"" EOF_PATTERN "\"",
				   ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CCID", iccid_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CMEE=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+GNSSCONF=10,1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+GNSSNMEA=0,1000,0,4F", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGSN", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMM", cgmm_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMI", cgmi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGMR", cgmr_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CIMI", cimi_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSTATEV=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CGEREP=2", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=5", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(swir_hl78xx_init_chat_script, swir_hl78xx_init_chat_script_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 10);

int modem_cmd_send_int(struct modem_hl78xx_data *user_data,
		       modem_chat_script_callback script_user_callback, const uint8_t *cmd,
		       uint16_t cmd_size, const struct modem_chat_match *response_matches,
		       uint16_t matches_size)
{

	int ret = 0;

	ret = k_mutex_lock(&user_data->tx_lock, K_NO_WAIT);
	if (ret < 0) {
		errno = -ret;
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

	ret = modem_chat_run_script(&user_data->chat, &chat_script);
	if (ret < 0) {
		LOG_ERR("Failed to run chat script: %d", ret);
	} else {
		LOG_DBG("Chat script executed successfully.");
	}
	ret = k_mutex_unlock(&user_data->tx_lock);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

void mdm_vgpio_callback_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	const struct gpio_dt_spec spec = {.port = port, .pin = pins};

	LOG_DBG("VGPIO ISR callback %d", gpio_pin_get_dt(&spec));
}
#if DT_INST_NODE_HAS_PROP(0, mdm_uart_dsr_gpios)
void mdm_uart_dsr_callback_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	const struct gpio_dt_spec spec = {.port = port, .pin = pins};

	LOG_DBG("DSR ISR callback %d", gpio_pin_get_dt(&spec));
}
#endif
void mdm_gpio6_callback_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	const struct gpio_dt_spec spec = {.port = port, .pin = pins};

	LOG_DBG("GPIO6 ISR callback %d", gpio_pin_get_dt(&spec));
}

void mdm_uart_cts_callback_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	const struct gpio_dt_spec spec = {.port = port, .pin = pins};

	LOG_DBG("CTS ISR callback %d", gpio_pin_get_dt(&spec));
}

static int modem_cellular_on_idle_state_leave(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	k_sem_take(&data->suspended_sem, K_NO_WAIT);

	if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_reset)) {
		gpio_pin_set_dt(&config->mdm_gpio_reset, 0);
	}

	if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
	}

	return 0;
}
static int modem_cellular_on_reset_pulse_state_enter(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
	}

	gpio_pin_set_dt(&config->mdm_gpio_reset, 1);
	modem_cellular_start_timer(data, K_MSEC(config->reset_pulse_duration_ms));
	return 0;
}

static void modem_cellular_reset_pulse_event_handler(struct modem_hl78xx_data *data,
						     enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_reset_pulse_state_leave(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	gpio_pin_set_dt(&config->mdm_gpio_reset, 0);

	if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 1);
	}

	modem_cellular_stop_timer(data);
	return 0;
}
static int modem_cellular_on_power_on_pulse_state_enter(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;
	gpio_pin_set_dt(&config->mdm_gpio_pwr_on, 1);
	modem_cellular_start_timer(data, K_MSEC(config->power_pulse_duration_ms));
	return 0;
}

static void modem_cellular_power_on_pulse_event_handler(struct modem_hl78xx_data *data,
							enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_power_on_pulse_state_leave(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;
	gpio_pin_set_dt(&config->mdm_gpio_pwr_on, 0);
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_await_power_on_state_enter(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	modem_cellular_start_timer(data, K_MSEC(config->startup_time_ms));
	return 0;
}

static void modem_cellular_await_power_on_event_handler(struct modem_hl78xx_data *data,
							enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	default:
		break;
	}
}
static int modem_cellular_on_run_init_script_state_enter(struct modem_hl78xx_data *data)
{
	modem_pipe_attach(data->uart_pipe, modem_cellular_bus_pipe_handler, data);
	return modem_pipe_open_async(data->uart_pipe);
}

static void modem_cellular_run_init_script_event_handler(struct modem_hl78xx_data *data,
							 enum modem_hl78xx_event evt)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;
	switch (evt) {
	case MODEM_HL78XX_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		modem_chat_run_script_async(&data->chat, config->init_chat_script);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT);

		break;

	case MODEM_HL78XX_EVENT_BUS_CLOSED:
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		modem_cellular_enter_state(data,
					   MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT);
		break;

	default:
		break;
	}
}
MODEM_CHAT_SCRIPT_CMDS_DEFINE(init_fail_chat_script_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP?", ksrep_match));

MODEM_CHAT_SCRIPT_DEFINE(init_fail_chat_script, init_fail_chat_script_cmds, abort_matches,
			 modem_cellular_chat_callback_handler, 10);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(swir_hl78xx_enable_ksup_urc_cmds,
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP=1", ok_match),
			      MODEM_CHAT_SCRIPT_CMD_RESP("AT+KSREP?", ksrep_match));

MODEM_CHAT_SCRIPT_DEFINE(swir_hl78xx_enable_ksup_urc_chat_script, swir_hl78xx_enable_ksup_urc_cmds,
			 abort_matches, modem_cellular_chat_callback_handler, 4);

static int modem_cellular_on_run_init_diagnose_script_state_enter(struct modem_hl78xx_data *data)
{
	modem_chat_run_script_async(&data->chat, &init_fail_chat_script);
	return 0;
}
static void modem_cellular_run_init_fail_script_event_handler(struct modem_hl78xx_data *data,
							      enum modem_hl78xx_event evt)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		if (data->ksrep == 0) {
			modem_chat_run_script_async(&data->chat,
						    &swir_hl78xx_enable_ksup_urc_chat_script);
			modem_cellular_start_timer(data, K_MSEC(config->shutdown_time_ms));
		} else {
			if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_reset)) {
				modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RESET_PULSE);
			}
		}
		break;
	case MODEM_HL78XX_EVENT_TIMEOUT:
		if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
			modem_cellular_enter_state(data, MODEM_HL78XX_STATE_POWER_ON_PULSE);
			break;
		}

		if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_reset)) {
			modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RESET_PULSE);
			break;
		}

		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;
	case MODEM_HL78XX_EVENT_BUS_CLOSED:
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		if (!modem_cellular_gpio_is_enabled(&config->mdm_gpio_wake)) {
			LOG_ERR("modem wake pin is not enabled, make sure modem low power is "
				"disabled, if you are not sure enable wake up pin by adding it "
				"dts!!");
		}

		if (data->script_fail_counter++ < MAX_SCRIPT_AT_CMD_RETRY) {
			if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
				modem_cellular_enter_state(data, MODEM_HL78XX_STATE_POWER_ON_PULSE);
				break;
			}
			if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_reset)) {
				modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RESET_PULSE);
				break;
			}
		}
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_run_rat_cfg_script_state_enter(struct modem_hl78xx_data *data)
{
	int ret = 0;
	bool modem_require_restart = false;

#if defined(CONFIG_MODEM_HL78XX_AUTO_RAT)
	/* Check autorat status/configs */
	if (IS_ENABLED(CONFIG_MODEM_HL78XX_AUTORAT_OVER_WRITE_PRL) ||
	    (data->kselacq_data.mode != 0 && data->kselacq_data.mode == 0)) {
		char cmd_kselq[] = "AT+KSELACQ=0," CONFIG_MODEM_HL78XX_AUTORAT_PRL_PROFILES;

		/* Re-congfiguring PRL context definition */
		ret = modem_cmd_send_int(data, NULL, cmd_kselq, strlen(cmd_kselq), &ok_match, 1);
		if (ret < 0) {
			goto error;
		}

	} else {
		/* do smth */
	}
#else
	/* Check active rat config  */

	if (data->kselacq_data.mode != 0 && data->kselacq_data.rat1 != 0) {
		char const *cmd_kselq_disable = (const char *)DISABLE_RAT_AUTO;

		/* Re-congfiguring PRL context definition */
		ret = modem_cmd_send_int(data, NULL, cmd_kselq_disable, strlen(cmd_kselq_disable),
					 &ok_match, 1);
		if (ret < 0) {
			goto error;
		}
	}

	char const *cmd_ksrat_query = (const char *)KSRAT_QUERY;

	/* Re-congfiguring PRL context definition */
	ret = modem_cmd_send_int(data, NULL, cmd_ksrat_query, strlen(cmd_ksrat_query), &ksrat_match,
				 1);
	if (ret < 0) {
		goto error;
	}
	enum mdm_hl78xx_rat_mode rat_config_request = MDM_RAT_MODE_NONE;

#if CONFIG_MODEM_HL78XX_RAT_M1
	char *cmd_rat_cat = (const char *)SET_RAT_M1_CMD_LEGACY;

	rat_config_request = MDM_RAT_CAT_M1;
#elif CONFIG_MODEM_HL78XX_RAT_NB1
	const char *cmd_rat_nb = (const char *)SET_RAT_NB1_CMD_LEGACY;

	rat_config_request = MDM_RAT_NB1;
#if CONFIG_MODEM_HL7812
#elif CONFIG_MODEM_HL78XX_RAT_GSM
	char *cmd_rat_gsm = (const char *)SET_RAT_GSM_CMD_LEGACY;

	rat_config_request = MDM_RAT_GSM;
#elif CONFIG_MODEM_HL78XX_RAT_NBNTN
	char *cmd_rat_nbntn = (const char *)SET_RAT_NBNTN_CMD_LEGACY;

	rat_config_request = MDM_RAT_NBNTN;
#endif
#endif
	if (rat_config_request != data->mdm_registration_status.rat_mode) {
		ret = modem_cmd_send_int(data, NULL, cmd_rat_nb, strlen(cmd_rat_nb), &ok_match, 1);
		if (ret < 0) {
			goto error;
		} else {
			modem_require_restart = true;
		}
		if (modem_require_restart) {
			const char *cmd_restart = (const char *)SET_AIRPLANE_MODE_CMD;

			ret = modem_cmd_send_int(data, NULL, cmd_restart, strlen(cmd_restart),
						 &ok_match, 1);
			if (ret < 0) {
				goto error;
			}
			modem_cellular_start_timer(data, K_MSEC(100));
			return 0;
		}
	}

#endif
	modem_cellular_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_SUCCESS, data);
	return 0;
error:
	modem_cellular_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_ABORT, data);
	LOG_ERR("Failed to send command: %d", ret);
	return ret;
}
static void modem_cellular_run_rat_cfg_script_event_handler(struct modem_hl78xx_data *data,
							    enum modem_hl78xx_event evt)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		LOG_DBG("Rebooting modem to apply new RAT settings");
		modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_SCRIPT_REQUIRE_RESTART);
		break;

	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;
	case MODEM_HL78XX_EVENT_SCRIPT_REQUIRE_RESTART:
		if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
			modem_cellular_enter_state(data, MODEM_HL78XX_STATE_POWER_ON_PULSE);
			break;
		}

		if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_reset)) {
			modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RESET_PULSE);
			break;
		}
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;
	default:
		break;
	}
}

static int modem_cellular_on_run_rat_cfg_script_state_leave(struct modem_hl78xx_data *data)
{
	return 0;
}

static int modem_cellular_on_await_power_off_state_enter(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	modem_cellular_start_timer(data, K_MSEC(config->shutdown_time_ms));
	return 0;
}
static void modem_cellular_await_power_off_event_handler(struct modem_hl78xx_data *data,
							 enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_IDLE);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_enable_gprs_state_enter(struct modem_hl78xx_data *data)
{
	int ret = 0;
	char cmd[sizeof("AT+CGDCONT=1,\"\",\"\"") + sizeof(MODEM_HL78XX_ADDRESS_FAMILY) +
		 MDM_APN_MAX_LENGTH];

#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_KCONFIG)
	data->mdm_apn[0] = '\0';
	strncat(data->mdm_apn, CONFIG_MODEM_HL78XX_APN, sizeof(data->mdm_apn) - 1);
#elif defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID) || defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
	/* autodetect APN from IMSI */
	/* the list of SIM profiles. Global scope, so the app can change it */
	/* AT+CCID or AT+CIMI needs to be run here if it is not ran in the init script */
	if (strlen(data->mdm_apn) < 1) {
		LOG_WRN("%d %s APN is left blank", __LINE__, __func__);
	}
#else /* defined(CONFIG_MODEM_HL78XX_APN_SOURCE_NETWORK) */
/* set blank string to get apn from network */
#endif

	snprintk(cmd, sizeof(cmd), "AT+CGDCONT=1,\"%s\",\"%s\"", MODEM_HL78XX_ADDRESS_FAMILY,
		 data->mdm_apn);

	ret = modem_cmd_send_int(data, NULL, cmd, strlen(cmd), &ok_match, 1);
	if (ret < 0) {
		goto error;
	}

	char cmd_string[sizeof("AT+KCNXCFG=1,\"GPRS\",\"\",,,"
			       "\"IPV4V6\"") +
			MDM_APN_MAX_LENGTH] = {0};

	int cmd_max_len = sizeof(cmd_string) - 1;

	snprintk(cmd_string, cmd_max_len,
		 "AT+KCNXCFG=1,\"GPRS\",\"%s\",,,\"" MODEM_HL78XX_ADDRESS_FAMILY "\"",
		 data->mdm_apn);

	ret = modem_cmd_send_int(data, NULL, cmd_string, strlen(cmd_string), &ok_match, 1);
	if (ret < 0) {
		goto error;
	}
#if defined(CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE)
	/* configure modem fully fuctinal without restart  */
	char *cmd_string_cfun = "AT+CFUN=1,0";

	ret = modem_cmd_send_int(data, NULL, cmd_string_cfun, strlen(cmd_string_cfun), &ok_match,
				 1);
	if (ret < 0) {
		goto error;
	}
#endif /* CONFIG_MODEM_HL78XX_BOOT_IN_FULLY_FUNCTIONAL_MODE */
	modem_cellular_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_SUCCESS, data);

	return 0;
error:
	modem_cellular_chat_callback_handler(&data->chat, MODEM_CHAT_SCRIPT_RESULT_ABORT, data);
	LOG_ERR("Failed to send command: %d", ret);
	return ret;
}
static void modem_cellular_enable_gprs_event_handler(struct modem_hl78xx_data *data,
						     enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		modem_cellular_start_timer(data, MODEM_HL78XX_PERIODIC_SCRIPT_TIMEOUT);
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		break;

	case MODEM_HL78XX_EVENT_REGISTERED:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_CARRIER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}
static int modem_cellular_on_await_registered_state_enter(struct modem_hl78xx_data *data)
{
	return 0;
}
static void modem_cellular_await_registered_event_handler(struct modem_hl78xx_data *data,
							  enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		modem_cellular_start_timer(data, MODEM_HL78XX_PERIODIC_SCRIPT_TIMEOUT);
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		break;

	case MODEM_HL78XX_EVENT_REGISTERED:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_CARRIER_ON);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_await_registered_state_leave(struct modem_hl78xx_data *data)
{
	modem_cellular_stop_timer(data);
	return 0;
}

static int modem_cellular_on_carrier_on_state_enter(struct modem_hl78xx_data *data)
{
	iface_status_work_cb(data);
	modem_cellular_start_timer(data, K_SECONDS(1));
	return 0;
}

static void modem_cellular_carrier_on_event_handler(struct modem_hl78xx_data *data,
						    enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_SCRIPT_SUCCESS:
	case MODEM_HL78XX_EVENT_SCRIPT_FAILED:
		break;

	case MODEM_HL78XX_EVENT_TIMEOUT:
		dns_work_cb();
		break;

	case MODEM_HL78XX_EVENT_DEREGISTERED:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_INIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_carrier_on_state_leave(struct modem_hl78xx_data *data)
{
	modem_cellular_stop_timer(data);
	modem_chat_release(&data->chat);
	return 0;
}
static int modem_cellular_on_init_power_off_state_enter(struct modem_hl78xx_data *data)
{
	modem_cellular_start_timer(data, K_MSEC(2000));
	return 0;
}

static void modem_cellular_init_power_off_event_handler(struct modem_hl78xx_data *data,
							enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		modem_cellular_begin_power_off_pulse(data);
		break;

	default:
		break;
	}
}
static int modem_cellular_on_init_power_off_state_leave(struct modem_hl78xx_data *data)
{
	modem_chat_release(&data->chat);
	return 0;
}

static int modem_cellular_on_power_off_pulse_state_enter(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	gpio_pin_set_dt(&config->mdm_gpio_pwr_on, 1);
	modem_cellular_start_timer(data, K_MSEC(config->power_pulse_duration_ms));
	return 0;
}

static void modem_cellular_power_off_pulse_event_handler(struct modem_hl78xx_data *data,
							 enum modem_hl78xx_event evt)
{
	switch (evt) {
	case MODEM_HL78XX_EVENT_TIMEOUT:
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_OFF);
		break;

	default:
		break;
	}
}

static int modem_cellular_on_power_off_pulse_state_leave(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	gpio_pin_set_dt(&config->mdm_gpio_pwr_on, 0);
	modem_cellular_stop_timer(data);
	return 0;
}
static int modem_cellular_on_state_enter(struct modem_hl78xx_data *data)
{
	int ret = 0;

	switch (data->state) {
	case MODEM_HL78XX_STATE_IDLE:
		ret = modem_cellular_on_idle_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_RESET_PULSE:
		ret = modem_cellular_on_reset_pulse_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_POWER_ON_PULSE:
		ret = modem_cellular_on_power_on_pulse_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_POWER_ON:
		LOG_DBG("%d %s", __LINE__, __func__);
		ret = modem_cellular_on_await_power_on_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_SET_BAUDRATE:
		break;

	case MODEM_HL78XX_STATE_RUN_INIT_SCRIPT:
		ret = modem_cellular_on_run_init_script_state_enter(data);
		break;
	case MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT:
		ret = modem_cellular_on_run_init_diagnose_script_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		ret = modem_cellular_on_run_rat_cfg_script_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT:
		ret = modem_cellular_on_enable_gprs_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		ret = modem_cellular_on_await_registered_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_CARRIER_ON:
		ret = modem_cellular_on_carrier_on_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		ret = modem_cellular_on_init_power_off_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		ret = modem_cellular_on_power_off_pulse_state_enter(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_POWER_OFF:
		ret = modem_cellular_on_await_power_off_state_enter(data);
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

static int modem_cellular_on_state_leave(struct modem_hl78xx_data *data)
{
	int ret = 0;

	LOG_DBG("%d %s %d", __LINE__, __func__, data->state);
	switch (data->state) {
	case MODEM_HL78XX_STATE_IDLE:
		ret = modem_cellular_on_idle_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_RESET_PULSE:
		ret = modem_cellular_on_reset_pulse_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_POWER_ON_PULSE:
		ret = modem_cellular_on_power_on_pulse_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		ret = modem_cellular_on_run_rat_cfg_script_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		ret = modem_cellular_on_await_registered_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_CARRIER_ON:
		ret = modem_cellular_on_carrier_on_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		ret = modem_cellular_on_init_power_off_state_leave(data);
		break;

	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		ret = modem_cellular_on_power_off_pulse_state_leave(data);
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}
static void modem_cellular_enter_state(struct modem_hl78xx_data *data,
				       enum modem_hl78xx_state state)
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
static void modem_cellular_begin_power_off_pulse(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	modem_pipe_close_async(data->uart_pipe);

	modem_cellular_enter_state(data, modem_cellular_gpio_is_enabled(&config->mdm_gpio_pwr_on)
						 ? MODEM_HL78XX_STATE_POWER_OFF_PULSE
						 : MODEM_HL78XX_STATE_IDLE);
}

static int modem_cellular_on_idle_state_enter(struct modem_hl78xx_data *data)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;

	if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_wake)) {
		gpio_pin_set_dt(&config->mdm_gpio_wake, 0);
	}

	if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_reset)) {
		gpio_pin_set_dt(&config->mdm_gpio_reset, 1);
	}
	modem_chat_release(&data->chat);
	modem_pipe_close_async(data->uart_pipe);
	k_sem_give(&data->suspended_sem);
	return 0;
}

static void modem_cellular_idle_event_handler(struct modem_hl78xx_data *data,
					      enum modem_hl78xx_event evt)
{
	const struct modem_hl78xx_config *config =
		(const struct modem_hl78xx_config *)data->dev->config;
	switch (evt) {
	case MODEM_HL78XX_EVENT_RESUME:
		if (config->autostarts) {
			modem_cellular_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_ON);
			break;
		}

		if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_pwr_on)) {
			modem_cellular_enter_state(data, MODEM_HL78XX_STATE_POWER_ON_PULSE);
			break;
		}

		if (modem_cellular_gpio_is_enabled(&config->mdm_gpio_reset)) {
			modem_cellular_enter_state(data, MODEM_HL78XX_STATE_AWAIT_POWER_ON);
			break;
		}
		LOG_DBG("%d %s", __LINE__, __func__);
		modem_cellular_enter_state(data, MODEM_HL78XX_STATE_RUN_INIT_SCRIPT);
		break;

	case MODEM_HL78XX_EVENT_SUSPEND:
		k_sem_give(&data->suspended_sem);
		break;

	default:
		break;
	}
}
static void modem_cellular_event_handler(struct modem_hl78xx_data *data,
					 enum modem_hl78xx_event evt)
{
	enum modem_hl78xx_state state;

	state = data->state;

	modem_cellular_log_event(evt);

	switch (data->state) {
	case MODEM_HL78XX_STATE_IDLE:
		modem_cellular_idle_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RESET_PULSE:
		modem_cellular_reset_pulse_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_POWER_ON_PULSE:
		modem_cellular_power_on_pulse_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_AWAIT_POWER_ON:
		modem_cellular_await_power_on_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_SET_BAUDRATE:
		break;

	case MODEM_HL78XX_STATE_RUN_INIT_SCRIPT:
		modem_cellular_run_init_script_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RUN_INIT_FAIL_DIAGNOSTIC_SCRIPT:
		modem_cellular_run_init_fail_script_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RUN_RAT_CONFIG_SCRIPT:
		modem_cellular_run_rat_cfg_script_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT:
		modem_cellular_enable_gprs_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_AWAIT_REGISTERED:
		modem_cellular_await_registered_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_CARRIER_ON:
		modem_cellular_carrier_on_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
		modem_cellular_init_power_off_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
		modem_cellular_power_off_pulse_event_handler(data, evt);
		break;

	case MODEM_HL78XX_STATE_AWAIT_POWER_OFF:
		modem_cellular_await_power_off_event_handler(data, evt);
		break;
	}

	if (state != data->state) {
		modem_cellular_log_state_changed(state, data->state);
	}
}

static int hl78xx_init(const struct device *dev)
{
	int ret;

	struct modem_hl78xx_config *mdm_config = (struct modem_hl78xx_config *)dev->config;
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)dev->data;

	data->dev = dev;

	/* Initialize work queue and event handling */
	k_work_queue_start(&modem_workq, modem_workq_stack,
			   K_KERNEL_STACK_SIZEOF(modem_workq_stack), K_PRIO_COOP(7), NULL);
	k_work_init_delayable(&data->timeout_work, modem_cellular_timeout_handler);
	k_work_init(&data->event_dispatch_work, modem_cellular_event_dispatch_handler);
	ring_buf_init(&data->event_rb, sizeof(data->event_buf), data->event_buf);
	k_sem_init(&data->suspended_sem, 0, 1);
	k_sem_init(&data->script_stopped_sem_tx_int, 0, 1);
	k_sem_init(&data->script_stopped_sem_rx_int, 0, 1);
	data->chat_eof_pattern_size = strlen(data->chat_eof_pattern);

	/* GPIO validation */
	const struct gpio_dt_spec *gpio_pins[] = {
		&mdm_config->mdm_gpio_wake, &mdm_config->mdm_gpio_gpio6,
		&mdm_config->mdm_gpio_reset, &mdm_config->mdm_gpio_vgpio};

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
	} gpio_config[] = {{&mdm_config->mdm_gpio_reset, GPIO_OUTPUT_LOW, "reset"},
			   {&mdm_config->mdm_gpio_wake, GPIO_OUTPUT_HIGH, "wake"},
#if DT_INST_NODE_HAS_PROP(0, mdm_pwr_on_gpios)
			   {&mdm_config->mdm_gpio_pwr_on, GPIO_OUTPUT_HIGH, "pwr_on"},
#endif
#if DT_INST_NODE_HAS_PROP(0, mdm_fast_shutd_gpios)
			   {&mdm_config->mdm_gpio_fast_shtdown, GPIO_OUTPUT_LOW, "fast_shutdown"},
#endif
#if DT_INST_NODE_HAS_PROP(0, mdm_uart_dtr_gpios)
			   {&mdm_config->mdm_gpio_uart_dtr, GPIO_OUTPUT, "DTR"},
#endif
			   {&mdm_config->mdm_gpio_vgpio, GPIO_INPUT, "VGPIO"}};

	for (int i = 0; i < ARRAY_SIZE(gpio_config); i++) {
		ret = gpio_pin_configure_dt(gpio_config[i].spec, gpio_config[i].flags);
		if (ret < 0) {
			LOG_ERR("Failed to configure %s pin", gpio_config[i].name);
			goto error;
		}
	}

	/* Callback setup */
	gpio_init_callback(&data->mdm_vgpio_cb, mdm_vgpio_callback_isr,
			   BIT(mdm_config->mdm_gpio_vgpio.pin));
	ret = gpio_add_callback(mdm_config->mdm_gpio_vgpio.port, &data->mdm_vgpio_cb);
	if (ret) {
		LOG_ERR("Cannot setup VGPIO callback! (%d)", ret);
		goto error;
	}
	ret = gpio_pin_interrupt_configure_dt(&mdm_config->mdm_gpio_vgpio, GPIO_INT_EDGE_BOTH);
	if (ret) {
		LOG_ERR("Error configuring VGPIO interrupt! (%d)", ret);
		goto error;
	}

	(void)hl78xx_init_pipe(dev);

	ret = modem_init_chat(dev);
	if (ret < 0) {
		goto error;
	}

	modem_hl78xx_socket_init(data);

#ifndef CONFIG_PM_DEVICE
	modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
#else
	pm_device_runtime_enable(dev);
#endif /* CONFIG_PM_DEVICE */

	if (!pm_device_is_powered(dev)) {
		pm_device_init_off(dev);
	}

	return 0;

error:
	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int mdm_hl78xx_driver_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct modem_hl78xx_data *data = (struct modem_hl78xx_data *)dev->data;

	int ret = 0;

	LOG_WRN("%d %s PM_DEVICE_ACTION: %d", __LINE__, __func__, action);
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* suspend the device */
		modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_SUSPEND);
		ret = k_sem_take(&data->suspended_sem, K_SECONDS(30));
		break;
	case PM_DEVICE_ACTION_RESUME:
		/* resume the device */
		modem_cellular_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * powered on the device, used when the power
		 * domain this device belongs is resumed.
		 */

		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		/*
		 * power off the device, used when the power
		 * domain this device belongs is suspended.
		 */
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */
static DEVICE_API(cellular, hl78xx_api) = {
	/*
	 *	.get_signal = hl78xx_api_get_signal,
	 *	.get_modem_info = hl78xx_api_get_modem_info,
	 *	.get_registration_status = hl78xx_api_get_registration_status,
	 */
};
#define MODEM_HL78XX_DEFINE_INSTANCE(inst, power_ms, reset_ms, startup_ms, shutdown_ms, start,     \
				     init_script)                                                  \
	static const struct modem_hl78xx_config hl78xx_cfg_##inst = {                              \
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
	};                                                                                         \
                                                                                                   \
	static struct modem_hl78xx_data hl78xx_data_##inst = {                                     \
		.chat_delimiter = "\r\n",                                                          \
		.chat_eof_pattern = EOF_PATTERN,                                                   \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, mdm_hl78xx_driver_pm_action);                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, hl78xx_init, PM_DEVICE_DT_INST_GET(inst), &hl78xx_data_##inst, \
			      &hl78xx_cfg_##inst, POST_KERNEL,                                     \
			      CONFIG_MODEM_HL78XX_DEV_INIT_PRIORITY, &hl78xx_api);

#define MODEM_DEVICE_SWIR_HL78XX(inst)                                                             \
	MODEM_HL78XX_DEFINE_INSTANCE(inst, 1500, 100, 100, 5000, false,                            \
				     &swir_hl78xx_init_chat_script)

#define DT_DRV_COMPAT swir_hl7812
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
