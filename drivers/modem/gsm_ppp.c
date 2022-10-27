/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_gsm_ppp

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_gsm, CONFIG_MODEM_LOG_LEVEL);

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/ppp.h>
#include <zephyr/drivers/modem/gsm_ppp.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/console/uart_mux.h>

#include "modem_context.h"
#include "modem_iface_uart.h"
#include "modem_cmd_handler.h"
#include "../console/gsm_mux.h"

#include <stdio.h>

#define GSM_UART_NODE                   DT_INST_BUS(0)
#define GSM_CMD_READ_BUF                128
#define GSM_CMD_AT_TIMEOUT              K_SECONDS(2)
#define GSM_CMD_SETUP_TIMEOUT           K_SECONDS(6)
/* GSM_CMD_LOCK_TIMEOUT should be longer than GSM_CMD_AT_TIMEOUT & GSM_CMD_SETUP_TIMEOUT,
 * otherwise the gsm_ppp_stop might fail to lock tx.
 */
#define GSM_CMD_LOCK_TIMEOUT            K_SECONDS(10)
#define GSM_RECV_MAX_BUF                30
#define GSM_RECV_BUF_SIZE               128
#define GSM_ATTACH_RETRY_DELAY_MSEC     1000
#define GSM_REGISTER_DELAY_MSEC         1000
#define GSM_RETRY_DELAY                 K_SECONDS(1)

#define GSM_RSSI_RETRY_DELAY_MSEC       2000
#define GSM_RSSI_RETRIES                10
#define GSM_RSSI_INVALID                -1000

#if defined(CONFIG_MODEM_GSM_ENABLE_CESQ_RSSI)
	#define GSM_RSSI_MAXVAL          0
#else
	#define GSM_RSSI_MAXVAL         -51
#endif

/* Modem network registration state */
enum network_state {
	GSM_NET_INIT = -1,
	GSM_NET_NOT_REGISTERED,
	GSM_NET_HOME_NETWORK,
	GSM_NET_SEARCHING,
	GSM_NET_REGISTRATION_DENIED,
	GSM_NET_UNKNOWN,
	GSM_NET_ROAMING,
};

static struct gsm_modem {
	struct k_mutex lock;
	const struct device *dev;
	struct modem_context context;

	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_match_buf[GSM_CMD_READ_BUF];
	struct k_sem sem_response;
	struct k_sem sem_if_down;

	struct modem_iface_uart_data gsm_data;
	struct k_work_delayable gsm_configure_work;
	char gsm_rx_rb_buf[PPP_MRU * 3];

	uint8_t *ppp_recv_buf;
	size_t ppp_recv_buf_len;

	enum gsm_ppp_state {
		GSM_PPP_START,
		GSM_PPP_WAIT_AT,
		GSM_PPP_AT_RDY,
		GSM_PPP_STATE_INIT,
		GSM_PPP_STATE_CONTROL_CHANNEL = GSM_PPP_STATE_INIT,
		GSM_PPP_STATE_PPP_CHANNEL,
		GSM_PPP_STATE_AT_CHANNEL,
		GSM_PPP_STATE_DONE,
		GSM_PPP_SETUP = GSM_PPP_STATE_DONE,
		GSM_PPP_REGISTERING,
		GSM_PPP_ATTACHING,
		GSM_PPP_ATTACHED,
		GSM_PPP_SETUP_DONE,
		GSM_PPP_STOP,
		GSM_PPP_STATE_ERROR,
	} state;

	const struct device *ppp_dev;
	const struct device *at_dev;
	const struct device *control_dev;

	struct net_if *iface;

	struct k_thread rx_thread;
	struct k_work_q workq;
	struct k_work_delayable rssi_work_handle;
	struct gsm_ppp_modem_info minfo;

	enum network_state net_state;

	int retries;
	bool modem_info_queried : 1;

	void *user_data;

	gsm_modem_power_cb modem_on_cb;
	gsm_modem_power_cb modem_off_cb;
	struct net_mgmt_event_callback gsm_mgmt_cb;
} gsm;

NET_BUF_POOL_DEFINE(gsm_recv_pool, GSM_RECV_MAX_BUF, GSM_RECV_BUF_SIZE, 0, NULL);
K_KERNEL_STACK_DEFINE(gsm_rx_stack, CONFIG_MODEM_GSM_RX_STACK_SIZE);
K_KERNEL_STACK_DEFINE(gsm_workq_stack, CONFIG_MODEM_GSM_WORKQ_STACK_SIZE);

static inline void gsm_ppp_lock(struct gsm_modem *gsm)
{
	(void)k_mutex_lock(&gsm->lock, K_FOREVER);
}

static inline void gsm_ppp_unlock(struct gsm_modem *gsm)
{
	(void)k_mutex_unlock(&gsm->lock);
}

static inline int gsm_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_reschedule_for_queue(&gsm.workq, dwork, delay);
}

#if defined(CONFIG_MODEM_GSM_ENABLE_CESQ_RSSI)
	/* helper macro to keep readability */
#define ATOI(s_, value_, desc_) modem_atoi(s_, value_, desc_, __func__)

/**
 * @brief  Convert string to long integer, but handle errors
 *
 * @param  s: string with representation of integer number
 * @param  err_value: on error return this value instead
 * @param  desc: name the string being converted
 * @param  func: function where this is called (typically __func__)
 *
 * @retval return integer conversion on success, or err_value on error
 */
static int modem_atoi(const char *s, const int err_value,
				const char *desc, const char *func)
{
	int ret;
	char *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if ((endptr == NULL) || (*endptr != '\0')) {
		LOG_ERR("bad %s '%s' in %s", s,
			 desc, func);
		return err_value;
	}

	return ret;
}
#endif

static void gsm_rx(struct gsm_modem *gsm)
{
	LOG_DBG("starting");

	while (true) {
		(void)k_sem_take(&gsm->gsm_data.rx_sem, K_FOREVER);

		/* The handler will listen AT channel */
		gsm->context.cmd_handler.process(&gsm->context.cmd_handler,
						 &gsm->context.iface);
	}
}

MODEM_CMD_DEFINE(gsm_cmd_ok)
{
	(void)modem_cmd_handler_set_error(data, 0);
	LOG_DBG("ok");
	k_sem_give(&gsm.sem_response);
	return 0;
}

MODEM_CMD_DEFINE(gsm_cmd_error)
{
	(void)modem_cmd_handler_set_error(data, -EINVAL);
	LOG_DBG("error");
	k_sem_give(&gsm.sem_response);
	return 0;
}

/* Handler: +CME Error: <err>[0] */
MODEM_CMD_DEFINE(gsm_cmd_exterror)
{
	/* TODO: map extended error codes to values */
	(void)modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&gsm.sem_response);
	return 0;
}

static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", gsm_cmd_ok, 0U, ""),
	MODEM_CMD("ERROR", gsm_cmd_error, 0U, ""),
	MODEM_CMD("+CME ERROR: ", gsm_cmd_exterror, 1U, ""),
	MODEM_CMD("CONNECT", gsm_cmd_ok, 0U, ""),
};

static int unquoted_atoi(const char *s, int base)
{
	if (*s == '"') {
		s++;
	}

	return strtol(s, NULL, base);
}

/*
 * Handler: +COPS: <mode>[0],<format>[1],<oper>[2]
 */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_cops)
{
	if (argc >= 1) {
#if defined(CONFIG_MODEM_CELL_INFO)
		if (argc >= 3) {
			gsm.context.data_operator = unquoted_atoi(argv[2], 10);
			LOG_INF("operator: %u",
				gsm.context.data_operator);
		}
#endif
		if (unquoted_atoi(argv[0], 10) == 0) {
			gsm.context.is_automatic_oper = true;
		} else {
			gsm.context.is_automatic_oper = false;
		}
	}

	return 0;
}

/*
 * Provide modem info if modem shell is enabled. This can be shown with
 * "modem list" shell command.
 */

/* Handler: <manufacturer> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_manufacturer)
{
	size_t out_len;

	out_len = net_buf_linearize(gsm.minfo.mdm_manufacturer,
				    sizeof(gsm.minfo.mdm_manufacturer) - 1,
				    data->rx_buf, 0, len);
	gsm.minfo.mdm_manufacturer[out_len] = '\0';
	LOG_INF("Manufacturer: %s", gsm.minfo.mdm_manufacturer);

	return 0;
}

/* Handler: <model> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_model)
{
	size_t out_len;

	out_len = net_buf_linearize(gsm.minfo.mdm_model,
				    sizeof(gsm.minfo.mdm_model) - 1,
				    data->rx_buf, 0, len);
	gsm.minfo.mdm_model[out_len] = '\0';
	LOG_INF("Model: %s", gsm.minfo.mdm_model);

	return 0;
}

/* Handler: <rev> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_revision)
{
	size_t out_len;

	out_len = net_buf_linearize(gsm.minfo.mdm_revision,
				    sizeof(gsm.minfo.mdm_revision) - 1,
				    data->rx_buf, 0, len);
	gsm.minfo.mdm_revision[out_len] = '\0';
	LOG_INF("Revision: %s", gsm.minfo.mdm_revision);

	return 0;
}

/* Handler: <IMEI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imei)
{
	size_t out_len;

	out_len = net_buf_linearize(gsm.minfo.mdm_imei, sizeof(gsm.minfo.mdm_imei) - 1,
				    data->rx_buf, 0, len);
	gsm.minfo.mdm_imei[out_len] = '\0';
	LOG_INF("IMEI: %s", gsm.minfo.mdm_imei);

	return 0;
}

#if defined(CONFIG_MODEM_SIM_NUMBERS)
/* Handler: <IMSI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imsi)
{
	size_t out_len;

	out_len = net_buf_linearize(gsm.minfo.mdm_imsi, sizeof(gsm.minfo.mdm_imsi) - 1,
				    data->rx_buf, 0, len);
	gsm.minfo.mdm_imsi[out_len] = '\0';
	LOG_INF("IMSI: %s", gsm.minfo.mdm_imsi);

	return 0;
}

/* Handler: <ICCID> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_iccid)
{
	size_t out_len;

	out_len = net_buf_linearize(gsm.minfo.mdm_iccid, sizeof(gsm.minfo.mdm_iccid) - 1,
				    data->rx_buf, 0, len);
	gsm.minfo.mdm_iccid[out_len] = '\0';
	if (gsm.minfo.mdm_iccid[0] == '+') {
		/* Seen on U-blox SARA: "+CCID: nnnnnnnnnnnnnnnnnnnn".
		 * Skip over the +CCID bit, which other modems omit.
		 */
		char *p = strchr(gsm.minfo.mdm_iccid, ' ');

		if (p) {
			size_t len = strlen(p+1);

			(void)memmove(gsm.minfo.mdm_iccid, p+1, len+1);
		}
	}
	LOG_INF("ICCID: %s", gsm.minfo.mdm_iccid);

	return 0;
}
#endif /* CONFIG_MODEM_SIM_NUMBERS */

MODEM_CMD_DEFINE(on_cmd_net_reg_sts)
{
	gsm.net_state = (enum network_state)atoi(argv[1]);

	switch (gsm.net_state) {
	case GSM_NET_NOT_REGISTERED:
		LOG_DBG("Network %s.", "not registered");
		break;
	case GSM_NET_HOME_NETWORK:
		LOG_DBG("Network %s.", "registered, home network");
		break;
	case GSM_NET_SEARCHING:
		LOG_DBG("Searching for network...");
		break;
	case GSM_NET_REGISTRATION_DENIED:
		LOG_DBG("Network %s.", "registration denied");
		break;
	case GSM_NET_UNKNOWN:
		LOG_DBG("Network %s.", "unknown");
		break;
	case GSM_NET_ROAMING:
		LOG_DBG("Network %s.", "registered, roaming");
		break;
	default:
		break;
	}

	return 0;
}

#if defined(CONFIG_MODEM_CELL_INFO)

/*
 * Handler: +CEREG: <n>[0],<stat>[1],<tac>[2],<ci>[3],<AcT>[4]
 */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_cereg)
{
	if (argc >= 4) {
		gsm.context.data_lac = unquoted_atoi(argv[2], 16);
		gsm.context.data_cellid = unquoted_atoi(argv[3], 16);
		LOG_INF("lac: %u, cellid: %u",
			gsm.context.data_lac,
			gsm.context.data_cellid);
	}

	return 0;
}

static const struct setup_cmd query_cellinfo_cmds[] = {
	SETUP_CMD_NOHANDLE("AT+CEREG=2"),
	SETUP_CMD("AT+CEREG?", "", on_cmd_atcmdinfo_cereg, 5U, ","),
	SETUP_CMD_NOHANDLE("AT+COPS=3,2"),
	SETUP_CMD("AT+COPS?", "", on_cmd_atcmdinfo_cops, 3U, ","),
};

static int gsm_query_cellinfo(struct gsm_modem *gsm)
{
	int ret;

	ret = modem_cmd_handler_setup_cmds_nolock(&gsm->context.iface,
						  &gsm->context.cmd_handler,
						  query_cellinfo_cmds,
						  ARRAY_SIZE(query_cellinfo_cmds),
						  &gsm->sem_response,
						  GSM_CMD_SETUP_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("modem query for cell info returned %d", ret);
	}

	return ret;
}
#endif /* CONFIG_MODEM_CELL_INFO */

#if defined(CONFIG_MODEM_GSM_ENABLE_CESQ_RSSI)
/*
 * Handler: +CESQ: <rxlev>[0],<ber>[1],<rscp>[2],<ecn0>[3],<rsrq>[4],<rsrp>[5]
 */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_rssi_cesq)
{
	int rsrp, rscp, rxlev;

	rsrp = ATOI(argv[5], 0, "rsrp");
	rscp = ATOI(argv[2], 0, "rscp");
	rxlev = ATOI(argv[0], 0, "rxlev");

	if ((rsrp >= 0) && (rsrp <= 97)) {
		gsm.minfo.mdm_rssi = -140 + (rsrp - 1);
		LOG_DBG("RSRP: %d", gsm.minfo.mdm_rssi);
	} else if ((rscp >= 0) && (rscp <= 96)) {
		gsm.minfo.mdm_rssi = -120 + (rscp - 1);
		LOG_DBG("RSCP: %d", gsm.minfo.mdm_rssi);
	} else if ((rxlev >= 0) && (rxlev <= 63)) {
		gsm.minfo.mdm_rssi = -110 + (rxlev - 1);
		LOG_DBG("RSSI: %d", gsm.minfo.mdm_rssi);
	} else {
		gsm.minfo.mdm_rssi = GSM_RSSI_INVALID;
		LOG_DBG("RSRP/RSCP/RSSI not known");
	}

	return 0;
}
#else
/* Handler: +CSQ: <signal_power>[0],<qual>[1] */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_rssi_csq)
{
	/* Expected response is "+CSQ: <signal_power>,<qual>" */
	if (argc > 0) {
		int rssi = atoi(argv[0]);

		if ((rssi >= 0) && (rssi <= 31)) {
			rssi = -113 + (rssi * 2);
		} else {
			rssi = GSM_RSSI_INVALID;
		}

		gsm.minfo.mdm_rssi = rssi;
		LOG_DBG("RSSI: %d", rssi);
	}

	return 0;
}
#endif

#if defined(CONFIG_MODEM_GSM_ENABLE_CESQ_RSSI)
static const struct modem_cmd read_rssi_cmd =
	MODEM_CMD("+CESQ:", on_cmd_atcmdinfo_rssi_cesq, 6U, ",");
#else
static const struct modem_cmd read_rssi_cmd =
	MODEM_CMD("+CSQ:", on_cmd_atcmdinfo_rssi_csq, 2U, ",");
#endif

static const struct setup_cmd setup_modem_info_cmds[] = {
	/* query modem info */
	SETUP_CMD("AT+CGMI", "", on_cmd_atcmdinfo_manufacturer, 0U, ""),
	SETUP_CMD("AT+CGMM", "", on_cmd_atcmdinfo_model, 0U, ""),
	SETUP_CMD("AT+CGMR", "", on_cmd_atcmdinfo_revision, 0U, ""),
	SETUP_CMD("AT+CGSN", "", on_cmd_atcmdinfo_imei, 0U, ""),
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	SETUP_CMD("AT+CIMI", "", on_cmd_atcmdinfo_imsi, 0U, ""),
	SETUP_CMD("AT+CCID", "", on_cmd_atcmdinfo_iccid, 0U, ""),
#endif
};

static const struct setup_cmd setup_cmds[] = {
	/* no echo */
	SETUP_CMD_NOHANDLE("ATE0"),
	/* hang up */
	SETUP_CMD_NOHANDLE("ATH"),
	/* extended errors in numeric form */
	SETUP_CMD_NOHANDLE("AT+CMEE=1"),
	/* disable unsolicited network registration codes */
	SETUP_CMD_NOHANDLE("AT+CREG=0"),
	/* create PDP context */
	SETUP_CMD_NOHANDLE("AT+CGDCONT=1,\"IP\",\"" CONFIG_MODEM_GSM_APN "\""),
#if IS_ENABLED(DT_PROP(GSM_UART_NODE, hw_flow_control))
	/* enable hardware flow control */
	SETUP_CMD_NOHANDLE("AT+IFC=2,2"),
#endif
};

MODEM_CMD_DEFINE(on_cmd_atcmdinfo_attached)
{
	/* Expected response is "+CGATT: 0|1" so simply look for '1' */
	if ((argc > 0) && (atoi(argv[0]) == 1)) {
		LOG_INF("Attached to packet service!");
	}

	return 0;
}


static const struct modem_cmd read_cops_cmd =
	MODEM_CMD_ARGS_MAX("+COPS:", on_cmd_atcmdinfo_cops, 1U, 4U, ",");

static const struct modem_cmd check_net_reg_cmd =
	MODEM_CMD("+CREG: ", on_cmd_net_reg_sts, 2U, ",");

static const struct modem_cmd check_attached_cmd =
	MODEM_CMD("+CGATT:", on_cmd_atcmdinfo_attached, 1U, ",");

static const struct setup_cmd connect_cmds[] = {
	/* connect to network */
	SETUP_CMD_NOHANDLE("ATD*99#"),
};

static int gsm_query_modem_info(struct gsm_modem *gsm)
{
	int ret;

	if (gsm->modem_info_queried) {
		return 0;
	}

	ret = modem_cmd_handler_setup_cmds_nolock(&gsm->context.iface,
						  &gsm->context.cmd_handler,
						  setup_modem_info_cmds,
						  ARRAY_SIZE(setup_modem_info_cmds),
						  &gsm->sem_response,
						  GSM_CMD_SETUP_TIMEOUT);

	if (ret < 0) {
		return ret;
	}

	gsm->modem_info_queried = true;

	return 0;
}

static int gsm_setup_mccmno(struct gsm_modem *gsm)
{
	int ret = 0;

	if (CONFIG_MODEM_GSM_MANUAL_MCCMNO[0] != '\0') {
		/* use manual MCC/MNO entry */
		ret = modem_cmd_send_nolock(&gsm->context.iface,
					    &gsm->context.cmd_handler,
					    NULL, 0,
					    "AT+COPS=1,2,\""
					    CONFIG_MODEM_GSM_MANUAL_MCCMNO
					    "\"",
					    &gsm->sem_response,
					    GSM_CMD_AT_TIMEOUT);
	} else {

/* First AT+COPS? is sent to check if automatic selection for operator
 * is already enabled, if yes we do not send the command AT+COPS= 0,0.
 */

		ret = modem_cmd_send_nolock(&gsm->context.iface,
					    &gsm->context.cmd_handler,
					    &read_cops_cmd,
					    1, "AT+COPS?",
					    &gsm->sem_response,
					    GSM_CMD_SETUP_TIMEOUT);

		if (ret < 0) {
			return ret;
		}

		if (!gsm->context.is_automatic_oper) {
			/* register operator automatically */
			ret = modem_cmd_send_nolock(&gsm->context.iface,
						    &gsm->context.cmd_handler,
						    NULL, 0, "AT+COPS=0,0",
						    &gsm->sem_response,
						    GSM_CMD_AT_TIMEOUT);
		}
	}

	if (ret < 0) {
		LOG_ERR("AT+COPS ret:%d", ret);
	}

	return ret;
}

static struct net_if *ppp_net_if(void)
{
	return net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
}

static void set_ppp_carrier_on(struct gsm_modem *gsm)
{
	static const struct ppp_api *api;
	const struct device *ppp_dev = device_get_binding(CONFIG_NET_PPP_DRV_NAME);
	struct net_if *iface = gsm->iface;
	int ret;

	if (ppp_dev == NULL) {
		LOG_ERR("Cannot find PPP %s!", CONFIG_NET_PPP_DRV_NAME);
		return;
	}

	if (api == NULL) {
		api = (const struct ppp_api *)ppp_dev->api;

		/* For the first call, we want to call ppp_start()... */
		ret = api->start(ppp_dev);
		if (ret < 0) {
			LOG_ERR("ppp start returned %d", ret);
		}
	} else {
		/* ...but subsequent calls should be to ppp_enable() */
		ret = net_if_l2(iface)->enable(iface, true);
		if (ret < 0) {
			LOG_ERR("ppp l2 enable returned %d", ret);
		}
	}
}

static void query_rssi(struct gsm_modem *gsm, bool lock)
{
	int ret;

#if defined(CONFIG_MODEM_GSM_ENABLE_CESQ_RSSI)
	ret = modem_cmd_send_ext(&gsm->context.iface, &gsm->context.cmd_handler, &read_rssi_cmd, 1,
				 "AT+CESQ", &gsm->sem_response, GSM_CMD_SETUP_TIMEOUT,
				 lock ? 0 : MODEM_NO_TX_LOCK);
#else
	ret = modem_cmd_send_ext(&gsm->context.iface, &gsm->context.cmd_handler, &read_rssi_cmd, 1,
				 "AT+CSQ", &gsm->sem_response, GSM_CMD_SETUP_TIMEOUT,
				 lock ? 0 : MODEM_NO_TX_LOCK);
#endif

	if (ret < 0) {
		LOG_DBG("No answer to RSSI readout, %s", "ignoring...");
	}
}

static inline void query_rssi_lock(struct gsm_modem *gsm)
{
	query_rssi(gsm, true);
}

static inline void query_rssi_nolock(struct gsm_modem *gsm)
{
	query_rssi(gsm, false);
}

static void rssi_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gsm_modem *gsm = CONTAINER_OF(dwork, struct gsm_modem, rssi_work_handle);

	gsm_ppp_lock(gsm);
	query_rssi_lock(gsm);

#if defined(CONFIG_MODEM_CELL_INFO)
	(void)gsm_query_cellinfo(gsm);
#endif
	(void)gsm_work_reschedule(&gsm->rssi_work_handle,
				  K_SECONDS(CONFIG_MODEM_GSM_RSSI_POLLING_PERIOD));
	gsm_ppp_unlock(gsm);
}

static void gsm_finalize_connection(struct k_work *work)
{
	int ret = 0;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gsm_modem *gsm = CONTAINER_OF(dwork, struct gsm_modem, gsm_configure_work);

	gsm_ppp_lock(gsm);

	/* If already attached, jump right to RSSI readout */
	if (gsm->state == GSM_PPP_ATTACHED) {
		goto attached;
	}

	/* If attach check failed, we should not redo every setup step */
	if (gsm->state == GSM_PPP_ATTACHING) {
		goto attaching;
	}

	/* If modem is searching for network, we should skip the setup step */
	if (gsm->state == GSM_PPP_REGISTERING) {
		goto registering;
	}

	if (IS_ENABLED(CONFIG_GSM_MUX)) {
		ret = modem_cmd_send_nolock(&gsm->context.iface,
					    &gsm->context.cmd_handler,
					    &response_cmds[0],
					    ARRAY_SIZE(response_cmds),
					    "AT", &gsm->sem_response,
					    GSM_CMD_AT_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%s returned %d, %s", "AT", ret, "retrying...");
			(void)gsm_work_reschedule(&gsm->gsm_configure_work, GSM_RETRY_DELAY);
			goto unlock;
		}
	}
	gsm->state = GSM_PPP_SETUP;

	if (IS_ENABLED(CONFIG_MODEM_GSM_FACTORY_RESET_AT_BOOT)) {
		(void)modem_cmd_send_nolock(&gsm->context.iface,
					    &gsm->context.cmd_handler,
					    &response_cmds[0],
					    ARRAY_SIZE(response_cmds),
					    "AT&F", &gsm->sem_response,
					    GSM_CMD_AT_TIMEOUT);
		(void)k_sleep(K_SECONDS(1));
	}

	ret = gsm_setup_mccmno(gsm);
	if (ret < 0) {
		LOG_ERR("%s returned %d, %s", "gsm_setup_mccmno", ret, "retrying...");

		(void)gsm_work_reschedule(&gsm->gsm_configure_work, GSM_RETRY_DELAY);
		goto unlock;
	}

	ret = modem_cmd_handler_setup_cmds_nolock(&gsm->context.iface,
						  &gsm->context.cmd_handler,
						  setup_cmds,
						  ARRAY_SIZE(setup_cmds),
						  &gsm->sem_response,
						  GSM_CMD_SETUP_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("%s returned %d, %s", "setup_cmds", ret, "retrying...");
		(void)gsm_work_reschedule(&gsm->gsm_configure_work, GSM_RETRY_DELAY);
		goto unlock;
	}

	ret = gsm_query_modem_info(gsm);
	if (ret < 0) {
		LOG_DBG("Unable to query modem information %d", ret);
		(void)gsm_work_reschedule(&gsm->gsm_configure_work, GSM_RETRY_DELAY);
		goto unlock;
	}

	gsm->state = GSM_PPP_REGISTERING;
registering:
	/* Wait for cell tower registration */
	ret = modem_cmd_send_nolock(&gsm->context.iface,
				    &gsm->context.cmd_handler,
				    &check_net_reg_cmd, 1,
				    "AT+CREG?",
				    &gsm->sem_response,
				    GSM_CMD_SETUP_TIMEOUT);
	if ((ret < 0) || ((gsm->net_state != GSM_NET_ROAMING) &&
			 (gsm->net_state != GSM_NET_HOME_NETWORK))) {
		if (gsm->retries == 0) {
			gsm->retries = CONFIG_MODEM_GSM_REGISTER_TIMEOUT *
				(MSEC_PER_SEC / GSM_REGISTER_DELAY_MSEC);
		} else {
			gsm->retries--;
		}

		(void)gsm_work_reschedule(&gsm->gsm_configure_work,
					  K_MSEC(GSM_REGISTER_DELAY_MSEC));
		goto unlock;
	}

	gsm->retries = 0;
	gsm->state = GSM_PPP_ATTACHING;
attaching:
	/* Don't initialize PPP until we're attached to packet service */
	ret = modem_cmd_send_nolock(&gsm->context.iface,
				    &gsm->context.cmd_handler,
				    &check_attached_cmd, 1,
				    "AT+CGATT?",
				    &gsm->sem_response,
				    GSM_CMD_SETUP_TIMEOUT);
	if (ret < 0) {
		/*
		 * retries not set        -> trigger N attach retries
		 * retries set            -> decrement and retry
		 * retries set, becomes 0 -> trigger full retry
		 */
		if (gsm->retries == 0) {
			gsm->retries = CONFIG_MODEM_GSM_ATTACH_TIMEOUT *
				(MSEC_PER_SEC / GSM_ATTACH_RETRY_DELAY_MSEC);
		} else {
			gsm->retries--;
		}

		LOG_DBG("Not attached, %s", "retrying...");

		(void)gsm_work_reschedule(&gsm->gsm_configure_work,
					K_MSEC(GSM_ATTACH_RETRY_DELAY_MSEC));
		goto unlock;
	}

	/* Attached, clear retry counter */
	LOG_DBG("modem attach returned %d, %s", ret, "read RSSI");
	gsm->state = GSM_PPP_ATTACHED;
	gsm->retries = GSM_RSSI_RETRIES;

 attached:

	if (!IS_ENABLED(CONFIG_GSM_MUX)) {
		/* Read connection quality (RSSI) before PPP carrier is ON */
		query_rssi_nolock(gsm);

		if (!((gsm->minfo.mdm_rssi > 0) && (gsm->minfo.mdm_rssi != GSM_RSSI_INVALID) &&
			(gsm->minfo.mdm_rssi < GSM_RSSI_MAXVAL))) {

			LOG_DBG("Not valid RSSI, %s", "retrying...");
			if (gsm->retries-- > 0) {
				(void)gsm_work_reschedule(&gsm->gsm_configure_work,
							K_MSEC(GSM_RSSI_RETRY_DELAY_MSEC));
				goto unlock;
			}
		}
#if defined(CONFIG_MODEM_CELL_INFO)
		(void)gsm_query_cellinfo(gsm);
#endif
	}

	LOG_DBG("modem RSSI: %d, %s", gsm->minfo.mdm_rssi, "enable PPP");

	ret = modem_cmd_handler_setup_cmds_nolock(&gsm->context.iface,
						  &gsm->context.cmd_handler,
						  connect_cmds,
						  ARRAY_SIZE(connect_cmds),
						  &gsm->sem_response,
						  GSM_CMD_SETUP_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("%s returned %d, %s", "connect_cmds", ret, "retrying...");
		(void)gsm_work_reschedule(&gsm->gsm_configure_work, GSM_RETRY_DELAY);
		goto unlock;
	}

	gsm->state = GSM_PPP_SETUP_DONE;
	set_ppp_carrier_on(gsm);

	if (IS_ENABLED(CONFIG_GSM_MUX)) {
		/* Re-use the original iface for AT channel */
		ret = modem_iface_uart_init_dev(&gsm->context.iface,
						gsm->at_dev);
		if (ret < 0) {
			LOG_DBG("iface %suart error %d", "AT ", ret);
			gsm->state = GSM_PPP_STATE_ERROR;
		} else {
			/* Do a test and try to send AT command to modem */
			ret = modem_cmd_send_nolock(
				&gsm->context.iface,
				&gsm->context.cmd_handler,
				&response_cmds[0],
				ARRAY_SIZE(response_cmds),
				"AT", &gsm->sem_response,
				GSM_CMD_AT_TIMEOUT);
			if (ret < 0) {
				LOG_WRN("%s returned %d, %s", "AT", ret, "iface failed");
				gsm->state = GSM_PPP_STATE_ERROR;
			} else {
				LOG_INF("AT channel %d connected to %s",
					DLCI_AT, gsm->at_dev->name);
			}
		}

		modem_cmd_handler_tx_unlock(&gsm->context.cmd_handler);
		if (gsm->state != GSM_PPP_STATE_ERROR) {
			(void)gsm_work_reschedule(&gsm->rssi_work_handle,
						K_SECONDS(CONFIG_MODEM_GSM_RSSI_POLLING_PERIOD));
		}
	}

unlock:
	gsm_ppp_unlock(gsm);
}

static int mux_enable(struct gsm_modem *gsm)
{
	int ret;

	/* Turn on muxing */
	if (IS_ENABLED(CONFIG_MODEM_GSM_SIMCOM)) {
		ret = modem_cmd_send_nolock(
			&gsm->context.iface,
			&gsm->context.cmd_handler,
			&response_cmds[0],
			ARRAY_SIZE(response_cmds),
#if defined(SIMCOM_LTE)
			/* FIXME */
			/* Some SIMCOM modems can set the channels */
			/* Control channel always at DLCI 0 */
			"AT+CMUXSRVPORT=0,0;"
			/* PPP should be at DLCI 1 */
			"+CMUXSRVPORT=" STRINGIFY(DLCI_PPP) ",1;"
			/* AT should be at DLCI 2 */
			"+CMUXSRVPORT=" STRINGIFY(DLCI_AT) ",1;"
#else
			"AT"
#endif
			"+CMUX=0,0,5,"
			STRINGIFY(CONFIG_GSM_MUX_MRU_DEFAULT_LEN),
			&gsm->sem_response,
			GSM_CMD_AT_TIMEOUT);
	} else if (IS_ENABLED(CONFIG_MODEM_GSM_QUECTEL)) {
		ret = modem_cmd_send_nolock(&gsm->context.iface,
				    &gsm->context.cmd_handler,
				    &response_cmds[0],
				    ARRAY_SIZE(response_cmds),
				    "AT+CMUX=0,0,5,"
				    STRINGIFY(CONFIG_GSM_MUX_MRU_DEFAULT_LEN),
				    &gsm->sem_response,
				    GSM_CMD_AT_TIMEOUT);

		/* Arbitrary delay for Quectel modems to initialize the CMUX,
		 * without this the AT cmd will fail.
		 */
		(void)k_sleep(K_SECONDS(1));
	} else {
		/* Generic GSM modem */
		ret = modem_cmd_send_nolock(&gsm->context.iface,
				     &gsm->context.cmd_handler,
				     &response_cmds[0],
				     ARRAY_SIZE(response_cmds),
				     "AT+CMUX=0", &gsm->sem_response,
				     GSM_CMD_AT_TIMEOUT);
	}

	if (ret < 0) {
		LOG_ERR("AT+CMUX ret:%d", ret);
	}

	return ret;
}

static void mux_setup_next(struct gsm_modem *gsm)
{
	(void)gsm_work_reschedule(&gsm->gsm_configure_work, K_MSEC(1));
}

static void mux_attach_cb(const struct device *mux, int dlci_address,
			  bool connected, void *user_data)
{
	LOG_DBG("DLCI %d to %s %s", dlci_address, mux->name,
		connected ? "connected" : "disconnected");

	if (connected) {
		uart_irq_rx_enable(mux);
		uart_irq_tx_enable(mux);
	}

	mux_setup_next(user_data);
}

static int mux_attach(const struct device *mux, const struct device *uart,
		      int dlci_address, void *user_data)
{
	int ret = uart_mux_attach(mux, uart, dlci_address, mux_attach_cb,
				  user_data);
	if (ret < 0) {
		LOG_ERR("Cannot attach DLCI %d (%s) to %s (%d)", dlci_address,
			mux->name, uart->name, ret);
		return ret;
	}

	return 0;
}

static void mux_setup(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gsm_modem *gsm = CONTAINER_OF(dwork, struct gsm_modem,
					     gsm_configure_work);
	const struct device *const uart = DEVICE_DT_GET(GSM_UART_NODE);
	int ret;

	gsm_ppp_lock(gsm);

	switch (gsm->state) {
	case GSM_PPP_STATE_CONTROL_CHANNEL:
		/* We need to call this to reactivate mux ISR. Note: This is only called
		 * after re-initing gsm_ppp.
		 */
		if (gsm->ppp_dev != NULL) {
			uart_mux_enable(gsm->ppp_dev);
		}

		/* Get UART device. There is one dev / DLCI */
		if (gsm->control_dev == NULL) {
			gsm->control_dev = uart_mux_alloc();
			if (gsm->control_dev == NULL) {
				LOG_DBG("Cannot get UART mux for %s channel",
					"control");
				goto fail;
			}
		}

		ret = mux_attach(gsm->control_dev, uart, DLCI_CONTROL, gsm);
		if (ret < 0) {
			goto fail;
		}

		gsm->state = GSM_PPP_STATE_PPP_CHANNEL;
		goto unlock;

	case GSM_PPP_STATE_PPP_CHANNEL:
		if (gsm->ppp_dev == NULL) {
			gsm->ppp_dev = uart_mux_alloc();
			if (gsm->ppp_dev == NULL) {
				LOG_DBG("Cannot get UART mux for %s channel",
					"PPP");
				goto fail;
			}
		}

		ret = mux_attach(gsm->ppp_dev, uart, DLCI_PPP, gsm);
		if (ret < 0) {
			goto fail;
		}

		gsm->state = GSM_PPP_STATE_AT_CHANNEL;
		goto unlock;

	case GSM_PPP_STATE_AT_CHANNEL:
		if (gsm->at_dev == NULL) {
			gsm->at_dev = uart_mux_alloc();
			if (gsm->at_dev == NULL) {
				LOG_DBG("Cannot get UART mux for %s channel",
					"AT");
				goto fail;
			}
		}

		ret = mux_attach(gsm->at_dev, uart, DLCI_AT, gsm);
		if (ret < 0) {
			goto fail;
		}

		gsm->state = GSM_PPP_STATE_DONE;
		goto unlock;

	case GSM_PPP_STATE_DONE:
		/* At least the SIMCOM modem expects that the Internet
		 * connection is created in PPP channel. We will need
		 * to attach the AT channel to context iface after the
		 * PPP connection is established in order to give AT commands
		 * to the modem.
		 */
		ret = modem_iface_uart_init_dev(&gsm->context.iface,
						gsm->ppp_dev);
		if (ret < 0) {
			LOG_DBG("iface %suart error %d", "PPP ", ret);
			goto fail;
		}

		LOG_INF("PPP channel %d connected to %s",
			DLCI_PPP, gsm->ppp_dev->name);

		k_work_init_delayable(&gsm->gsm_configure_work, gsm_finalize_connection);
		(void)gsm_work_reschedule(&gsm->gsm_configure_work, K_NO_WAIT);
		goto unlock;
	default:
		__ASSERT(0, "%s while in state: %d", "mux_setup", gsm->state);
		/* In case CONFIG_ASSERT is off, goto fail */
		goto fail;
	}

fail:
	gsm->state = GSM_PPP_STATE_ERROR;
unlock:
	gsm_ppp_unlock(gsm);
}

static void gsm_configure(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gsm_modem *gsm = CONTAINER_OF(dwork, struct gsm_modem,
					     gsm_configure_work);
	int ret = -1;

	gsm_ppp_lock(gsm);

	if (gsm->state == GSM_PPP_WAIT_AT) {
		goto wait_at;
	}

	if (gsm->state == GSM_PPP_START) {
		LOG_DBG("Starting modem %p configuration", gsm);

		if (gsm->modem_on_cb != NULL) {
			gsm->modem_on_cb(gsm->dev, gsm->user_data);
		}

		gsm->state = GSM_PPP_WAIT_AT;
	}

wait_at:
	ret = modem_cmd_send_nolock(&gsm->context.iface,
				    &gsm->context.cmd_handler,
				    &response_cmds[0],
				    ARRAY_SIZE(response_cmds),
				    "AT", &gsm->sem_response,
				    GSM_CMD_AT_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("modem not ready %d", ret);
		goto retry;
	}

	gsm->state = GSM_PPP_AT_RDY;

	if (IS_ENABLED(CONFIG_GSM_MUX)) {
		if (mux_enable(gsm) == 0) {
			LOG_DBG("GSM muxing %s", "enabled");
		} else {
			LOG_DBG("GSM muxing %s", "disabled");
			goto retry;
		}

		gsm->state = GSM_PPP_STATE_INIT;

		k_work_init_delayable(&gsm->gsm_configure_work, mux_setup);
	} else {
		k_work_init_delayable(&gsm->gsm_configure_work, gsm_finalize_connection);
	}

retry:
	(void)gsm_work_reschedule(&gsm->gsm_configure_work, K_NO_WAIT);
	gsm_ppp_unlock(gsm);
}

void gsm_ppp_start(const struct device *dev)
{
	int ret;
	struct gsm_modem *gsm = dev->data;

	gsm_ppp_lock(gsm);

	if (gsm->state != GSM_PPP_STOP) {
		LOG_ERR("gsm_ppp is already %s", "started");
		goto unlock;
	}

	gsm->state = GSM_PPP_START;

	/* Re-init underlying UART comms */
	ret = modem_iface_uart_init_dev(&gsm->context.iface, DEVICE_DT_GET(GSM_UART_NODE));
	if (ret < 0) {
		LOG_ERR("modem_iface_uart_init returned %d", ret);
		gsm->state = GSM_PPP_STATE_ERROR;
		goto unlock;
	}

	k_work_init_delayable(&gsm->gsm_configure_work, gsm_configure);
	(void)gsm_work_reschedule(&gsm->gsm_configure_work, K_NO_WAIT);

unlock:
	gsm_ppp_unlock(gsm);
}

void gsm_ppp_stop(const struct device *dev)
{
	struct gsm_modem *gsm = dev->data;
	struct net_if *iface = gsm->iface;
	struct k_work_sync work_sync;

	if (gsm->state == GSM_PPP_STOP) {
		LOG_ERR("gsm_ppp is already %s", "stopped");
		return;
	}

	(void)k_work_cancel_delayable_sync(&gsm->gsm_configure_work, &work_sync);
	if (IS_ENABLED(CONFIG_GSM_MUX)) {
		(void)k_work_cancel_delayable_sync(&gsm->rssi_work_handle, &work_sync);
	}

	gsm_ppp_lock(gsm);

	/* wait for the interface to be properly down */
	if (net_if_is_up(iface)) {
		(void)(net_if_l2(iface)->enable(iface, false));
		(void)k_sem_take(&gsm->sem_if_down, K_FOREVER);
	}

	if (IS_ENABLED(CONFIG_GSM_MUX)) {
		if (gsm->ppp_dev != NULL) {
			uart_mux_disable(gsm->ppp_dev);
		}

		if (modem_cmd_handler_tx_lock(&gsm->context.cmd_handler,
								GSM_CMD_LOCK_TIMEOUT) < 0) {
			LOG_WRN("Failed locking modem cmds!");
		}
	}

	if (gsm->modem_off_cb != NULL) {
		gsm->modem_off_cb(gsm->dev, gsm->user_data);
	}

	gsm->net_state = GSM_NET_INIT;
	gsm_ppp_unlock(gsm);
}

void gsm_ppp_register_modem_power_callback(const struct device *dev,
					   gsm_modem_power_cb modem_on,
					   gsm_modem_power_cb modem_off,
					   void *user_data)
{
	struct gsm_modem *gsm = dev->data;

	gsm_ppp_lock(gsm);

	gsm->modem_on_cb = modem_on;
	gsm->modem_off_cb = modem_off;

	gsm->user_data = user_data;
	gsm_ppp_unlock(gsm);
}

const struct gsm_ppp_modem_info *gsm_ppp_modem_info(const struct device *dev)
{
	struct gsm_modem *gsm = dev->data;

	return &gsm->minfo;
}

static void gsm_mgmt_event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & NET_EVENT_IF_DOWN) != mgmt_event) {
		return;
	}

	/* Right now we only support 1 GSM instance */
	if (iface != gsm.iface) {
		return;
	}

	if (mgmt_event == NET_EVENT_IF_DOWN) {
		LOG_INF("GSM network interface down");
		/* raise semaphore to indicate the interface is down */
		k_sem_give(&gsm.sem_if_down);
		return;
	}
}

static int gsm_init(const struct device *dev)
{
	struct gsm_modem *gsm = dev->data;
	int ret;

	LOG_DBG("Generic GSM modem (%p)", gsm);

	(void)k_mutex_init(&gsm->lock);
	gsm->dev = dev;

	gsm->cmd_handler_data.cmds[CMD_RESP] = response_cmds;
	gsm->cmd_handler_data.cmds_len[CMD_RESP] = ARRAY_SIZE(response_cmds);
	gsm->cmd_handler_data.match_buf = &gsm->cmd_match_buf[0];
	gsm->cmd_handler_data.match_buf_len = sizeof(gsm->cmd_match_buf);
	gsm->cmd_handler_data.buf_pool = &gsm_recv_pool;
	gsm->cmd_handler_data.alloc_timeout = K_NO_WAIT;
	gsm->cmd_handler_data.eol = "\r";

	(void)k_sem_init(&gsm->sem_response, 0, 1);
	(void)k_sem_init(&gsm->sem_if_down, 0, 1);

	ret = modem_cmd_handler_init(&gsm->context.cmd_handler,
				   &gsm->cmd_handler_data);
	if (ret < 0) {
		LOG_DBG("cmd handler error %d", ret);
		return ret;
	}

#if defined(CONFIG_MODEM_SHELL)
	/* modem information storage */
	gsm->context.data_manufacturer = gsm->minfo.mdm_manufacturer;
	gsm->context.data_model = gsm->minfo.mdm_model;
	gsm->context.data_revision = gsm->minfo.mdm_revision;
	gsm->context.data_imei = gsm->minfo.mdm_imei;
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	gsm->context.data_imsi = gsm->minfo.mdm_imsi;
	gsm->context.data_iccid = gsm->minfo.mdm_iccid;
#endif	/* CONFIG_MODEM_SIM_NUMBERS */
	gsm->context.data_rssi = &gsm->minfo.mdm_rssi;
#endif	/* CONFIG_MODEM_SHELL */

	gsm->context.is_automatic_oper = false;
	gsm->gsm_data.rx_rb_buf = &gsm->gsm_rx_rb_buf[0];
	gsm->gsm_data.rx_rb_buf_len = sizeof(gsm->gsm_rx_rb_buf);
	gsm->gsm_data.hw_flow_control = DT_PROP(GSM_UART_NODE,
						hw_flow_control);

	ret = modem_iface_uart_init(&gsm->context.iface, &gsm->gsm_data,
				DEVICE_DT_GET(GSM_UART_NODE));
	if (ret < 0) {
		LOG_DBG("iface uart error %d", ret);
		return ret;
	}

	ret = modem_context_register(&gsm->context);
	if (ret < 0) {
		LOG_DBG("context error %d", ret);
		return ret;
	}

	/* Initialize to stop state so that it can be started later */
	gsm->state = GSM_PPP_STOP;

	gsm->net_state = GSM_NET_INIT;

	LOG_DBG("iface->read %p iface->write %p",
		gsm->context.iface.read, gsm->context.iface.write);

	(void)k_thread_create(&gsm->rx_thread, gsm_rx_stack,
			      K_KERNEL_STACK_SIZEOF(gsm_rx_stack),
			      (k_thread_entry_t) gsm_rx,
			      gsm, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	(void)k_thread_name_set(&gsm->rx_thread, "gsm_rx");

	/* initialize the work queue */
	k_work_queue_init(&gsm->workq);
	k_work_queue_start(&gsm->workq, gsm_workq_stack, K_KERNEL_STACK_SIZEOF(gsm_workq_stack),
			   K_PRIO_COOP(7), NULL);
	(void)k_thread_name_set(&gsm->workq.thread, "gsm_workq");

	if (IS_ENABLED(CONFIG_GSM_MUX)) {
		k_work_init_delayable(&gsm->rssi_work_handle, rssi_handler);
	}

	gsm->iface = ppp_net_if();
	if (gsm->iface == NULL) {
		LOG_ERR("Couldn't find ppp net_if!");
		return -ENODEV;
	}

	net_mgmt_init_event_callback(&gsm->gsm_mgmt_cb, gsm_mgmt_event_handler,
				     NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&gsm->gsm_mgmt_cb);

	if (IS_ENABLED(CONFIG_GSM_PPP_AUTOSTART)) {
		gsm_ppp_start(dev);
	}

	return 0;
}

DEVICE_DT_DEFINE(DT_INST(0, zephyr_gsm_ppp), gsm_init, NULL, &gsm, NULL,
		 POST_KERNEL, CONFIG_MODEM_GSM_INIT_PRIORITY, NULL);
