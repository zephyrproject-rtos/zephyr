/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(modem_gsm, CONFIG_MODEM_LOG_LEVEL);

#include <stdlib.h>
#include <kernel.h>
#include <device.h>
#include <sys/ring_buffer.h>
#include <sys/util.h>
#include <net/ppp.h>
#include <drivers/gsm_ppp.h>
#include <drivers/uart.h>
#include <drivers/console/uart_mux.h>

#include "modem_context.h"
#include "modem_iface_uart.h"
#include "modem_cmd_handler.h"
#include "../console/gsm_mux.h"

#include <stdio.h>

#define GSM_CMD_READ_BUF                128
#define GSM_CMD_AT_TIMEOUT              K_SECONDS(2)
#define GSM_CMD_SETUP_TIMEOUT           K_SECONDS(6)
#define GSM_RX_STACK_SIZE               CONFIG_MODEM_GSM_RX_STACK_SIZE
#define GSM_RECV_MAX_BUF                30
#define GSM_RECV_BUF_SIZE               128
#define GSM_ATTACH_RETRY_DELAY_MSEC     1000

#define GSM_RSSI_RETRY_DELAY_MSEC       2000
#define GSM_RSSI_RETRIES                10
#define GSM_RSSI_INVALID                -1000

#if defined(CONFIG_MODEM_GSM_ENABLE_CESQ_RSSI)
	#define GSM_RSSI_MAXVAL          0
#else
	#define GSM_RSSI_MAXVAL         -51
#endif

/* During the modem setup, we first create DLCI control channel and then
 * PPP and AT channels. Currently the modem does not create possible GNSS
 * channel.
 */
enum setup_state {
	STATE_INIT = 0,
	STATE_CONTROL_CHANNEL = 0,
	STATE_PPP_CHANNEL,
	STATE_AT_CHANNEL,
	STATE_DONE
};

static struct gsm_modem {
	struct modem_context context;

	struct modem_cmd_handler_data cmd_handler_data;
	uint8_t cmd_match_buf[GSM_CMD_READ_BUF];
	struct k_sem sem_response;

	struct modem_iface_uart_data gsm_data;
	struct k_work_delayable gsm_configure_work;
	char gsm_rx_rb_buf[PPP_MRU * 3];

	uint8_t *ppp_recv_buf;
	size_t ppp_recv_buf_len;

	enum setup_state state;
	const struct device *ppp_dev;
	const struct device *at_dev;
	const struct device *control_dev;

	struct net_if *iface;

	int rssi_retries;
	int attach_retries;
	bool mux_enabled : 1;
	bool mux_setup_done : 1;
	bool setup_done : 1;
	bool attached : 1;
} gsm;

NET_BUF_POOL_DEFINE(gsm_recv_pool, GSM_RECV_MAX_BUF, GSM_RECV_BUF_SIZE,
		    0, NULL);
K_KERNEL_STACK_DEFINE(gsm_rx_stack, GSM_RX_STACK_SIZE);

struct k_thread gsm_rx_thread;
static struct k_work_delayable rssi_work_handle;

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
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", log_strdup(s),
			 log_strdup(desc), log_strdup(func));
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
	modem_cmd_handler_set_error(data, 0);
	LOG_DBG("ok");
	k_sem_give(&gsm.sem_response);
	return 0;
}

MODEM_CMD_DEFINE(gsm_cmd_error)
{
	modem_cmd_handler_set_error(data, -EINVAL);
	LOG_DBG("error");
	k_sem_give(&gsm.sem_response);
	return 0;
}

static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", gsm_cmd_ok, 0U, ""),
	MODEM_CMD("ERROR", gsm_cmd_error, 0U, ""),
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
	if (argc >= 3) {
#if defined(CONFIG_MODEM_CELL_INFO)
		gsm.context.data_operator = unquoted_atoi(argv[2], 10);
		LOG_INF("operator: %u",
			gsm.context.data_operator);
#endif
		if (unquoted_atoi(argv[0], 10) == 0) {
			gsm.context.is_automatic_oper = true;
		} else {
			gsm.context.is_automatic_oper = false;
		}
	}

	return 0;
}

#if defined(CONFIG_MODEM_SHELL)
#define MDM_MANUFACTURER_LENGTH  10
#define MDM_MODEL_LENGTH         16
#define MDM_REVISION_LENGTH      64
#define MDM_IMEI_LENGTH          16
#define MDM_IMSI_LENGTH          16
#define MDM_ICCID_LENGTH         32

struct modem_info {
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	char mdm_imsi[MDM_IMSI_LENGTH];
	char mdm_iccid[MDM_ICCID_LENGTH];
#endif
};

static struct modem_info minfo;

/*
 * Provide modem info if modem shell is enabled. This can be shown with
 * "modem list" shell command.
 */

/* Handler: <manufacturer> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_manufacturer)
{
	size_t out_len;

	out_len = net_buf_linearize(minfo.mdm_manufacturer,
				    sizeof(minfo.mdm_manufacturer) - 1,
				    data->rx_buf, 0, len);
	minfo.mdm_manufacturer[out_len] = '\0';
	LOG_INF("Manufacturer: %s", log_strdup(minfo.mdm_manufacturer));

	return 0;
}

/* Handler: <model> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_model)
{
	size_t out_len;

	out_len = net_buf_linearize(minfo.mdm_model,
				    sizeof(minfo.mdm_model) - 1,
				    data->rx_buf, 0, len);
	minfo.mdm_model[out_len] = '\0';
	LOG_INF("Model: %s", log_strdup(minfo.mdm_model));

	return 0;
}

/* Handler: <rev> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_revision)
{
	size_t out_len;

	out_len = net_buf_linearize(minfo.mdm_revision,
				    sizeof(minfo.mdm_revision) - 1,
				    data->rx_buf, 0, len);
	minfo.mdm_revision[out_len] = '\0';
	LOG_INF("Revision: %s", log_strdup(minfo.mdm_revision));

	return 0;
}

/* Handler: <IMEI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imei)
{
	size_t out_len;

	out_len = net_buf_linearize(minfo.mdm_imei, sizeof(minfo.mdm_imei) - 1,
				    data->rx_buf, 0, len);
	minfo.mdm_imei[out_len] = '\0';
	LOG_INF("IMEI: %s", log_strdup(minfo.mdm_imei));

	return 0;
}

#if defined(CONFIG_MODEM_SIM_NUMBERS)
/* Handler: <IMSI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imsi)
{
	size_t out_len;

	out_len = net_buf_linearize(minfo.mdm_imsi, sizeof(minfo.mdm_imsi) - 1,
				    data->rx_buf, 0, len);
	minfo.mdm_imsi[out_len] = '\0';
	LOG_INF("IMSI: %s", log_strdup(minfo.mdm_imsi));

	return 0;
}

/* Handler: <ICCID> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_iccid)
{
	size_t out_len;

	out_len = net_buf_linearize(minfo.mdm_iccid, sizeof(minfo.mdm_iccid) - 1,
				    data->rx_buf, 0, len);
	minfo.mdm_iccid[out_len] = '\0';
	if (minfo.mdm_iccid[0] == '+') {
		/* Seen on U-blox SARA: "+CCID: nnnnnnnnnnnnnnnnnnnn".
		 * Skip over the +CCID bit, which other modems omit.
		 */
		char *p = strchr(minfo.mdm_iccid, ' ');

		if (p) {
			size_t len = strlen(p+1);

			memmove(minfo.mdm_iccid, p+1, len+1);
		}
	}
	LOG_INF("ICCID: %s", log_strdup(minfo.mdm_iccid));

	return 0;
}
#endif /* CONFIG_MODEM_SIM_NUMBERS */

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
#endif /* CONFIG_MODEM_SHELL */

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

	if (rsrp >= 0 && rsrp <= 97) {
		gsm.context.data_rssi = -140 + (rsrp - 1);
		LOG_INF("RSRP: %d", gsm.context.data_rssi);
	} else if (rscp >= 0 && rscp <= 96) {
		gsm.context.data_rssi = -120 + (rscp - 1);
		LOG_INF("RSCP: %d", gsm.context.data_rssi);
	} else if (rxlev >= 0 && rxlev <= 63) {
		gsm.context.data_rssi = -110 + (rxlev - 1);
		LOG_INF("RSSI: %d", gsm.context.data_rssi);
	} else {
		gsm.context.data_rssi = GSM_RSSI_INVALID;
		LOG_INF("RSRP/RSCP/RSSI not known");
	}

	return 0;
}
#else
/* Handler: +CSQ: <signal_power>[0],<qual>[1] */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_rssi_csq)
{
	/* Expected response is "+CSQ: <signal_power>,<qual>" */
	if (argc) {
		int rssi = atoi(argv[0]);

		if (rssi >= 0 && rssi <= 31) {
			rssi = -113 + (rssi * 2);
		} else {
			rssi = GSM_RSSI_INVALID;
		}

		gsm.context.data_rssi = rssi;
		LOG_INF("RSSI: %d", rssi);
	}

	k_sem_give(&gsm.sem_response);

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

static const struct setup_cmd setup_cmds[] = {
	/* no echo */
	SETUP_CMD_NOHANDLE("ATE0"),
	/* hang up */
	SETUP_CMD_NOHANDLE("ATH"),
	/* extender errors in numeric form */
	SETUP_CMD_NOHANDLE("AT+CMEE=1"),

#if defined(CONFIG_MODEM_SHELL)
	/* query modem info */
	SETUP_CMD("AT+CGMI", "", on_cmd_atcmdinfo_manufacturer, 0U, ""),
	SETUP_CMD("AT+CGMM", "", on_cmd_atcmdinfo_model, 0U, ""),
	SETUP_CMD("AT+CGMR", "", on_cmd_atcmdinfo_revision, 0U, ""),
# if defined(CONFIG_MODEM_SIM_NUMBERS)
	SETUP_CMD("AT+CIMI", "", on_cmd_atcmdinfo_imsi, 0U, ""),
	SETUP_CMD("AT+CCID", "", on_cmd_atcmdinfo_iccid, 0U, ""),
# endif
	SETUP_CMD("AT+CGSN", "", on_cmd_atcmdinfo_imei, 0U, ""),
#endif

	/* disable unsolicited network registration codes */
	SETUP_CMD_NOHANDLE("AT+CREG=0"),

	/* create PDP context */
	SETUP_CMD_NOHANDLE("AT+CGDCONT=1,\"IP\",\"" CONFIG_MODEM_GSM_APN "\""),
};

MODEM_CMD_DEFINE(on_cmd_atcmdinfo_attached)
{
	int error = -EAGAIN;

	/* Expected response is "+CGATT: 0|1" so simply look for '1' */
	if (argc && atoi(argv[0]) == 1) {
		error = 0;
		LOG_INF("Attached to packet service!");
	}

	modem_cmd_handler_set_error(data, error);
	k_sem_give(&gsm.sem_response);

	return 0;
}


static const struct modem_cmd read_cops_cmd =
	MODEM_CMD("+COPS", on_cmd_atcmdinfo_cops, 3U, ",");

static const struct modem_cmd check_attached_cmd =
	MODEM_CMD("+CGATT:", on_cmd_atcmdinfo_attached, 1U, ",");

static const struct setup_cmd connect_cmds[] = {
	/* connect to network */
	SETUP_CMD_NOHANDLE("ATD*99#"),
};

static int gsm_setup_mccmno(struct gsm_modem *gsm)
{
	int ret = 0;

	if (CONFIG_MODEM_GSM_MANUAL_MCCMNO[0]) {
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
	const struct device *ppp_dev =
		device_get_binding(CONFIG_NET_PPP_DRV_NAME);
	struct net_if *iface = gsm->iface;
	int ret;

	if (!ppp_dev) {
		LOG_ERR("Cannot find PPP %s!", CONFIG_NET_PPP_DRV_NAME);
		return;
	}

	if (!api) {
		api = (const struct ppp_api *)ppp_dev->api;

		/* For the first call, we want to call ppp_start()... */
		ret = api->start(ppp_dev);
		if (ret) {
			LOG_ERR("ppp start returned %d", ret);
		}
	} else {
		/* ...but subsequent calls should be to ppp_enable() */
		ret = net_if_l2(iface)->enable(iface, true);
		if (ret) {
			LOG_ERR("ppp l2 enable returned %d", ret);
		}
	}
}

static void rssi_handler(struct k_work *work)
{
	int ret;
#if defined(CONFIG_MODEM_GSM_ENABLE_CESQ_RSSI)
	ret = modem_cmd_send_nolock(&gsm.context.iface, &gsm.context.cmd_handler,
		&read_rssi_cmd, 1, "AT+CESQ", &gsm.sem_response, GSM_CMD_SETUP_TIMEOUT);
#else
	ret = modem_cmd_send_nolock(&gsm.context.iface, &gsm.context.cmd_handler,
		&read_rssi_cmd, 1, "AT+CSQ", &gsm.sem_response, GSM_CMD_SETUP_TIMEOUT);
#endif

	if (ret < 0) {
		LOG_DBG("No answer to RSSI readout, %s", "ignoring...");
	}

#if defined(CONFIG_GSM_MUX)
#if defined(CONFIG_MODEM_CELL_INFO)
	(void) gsm_query_cellinfo(&gsm);
#endif
	k_work_reschedule(&rssi_work_handle, K_SECONDS(CONFIG_MODEM_GSM_RSSI_POLLING_PERIOD));
#endif

}

static void gsm_finalize_connection(struct gsm_modem *gsm)
{
	int ret = 0;

	/* If already attached, jump right to RSSI readout */
	if (gsm->attached) {
		goto attached;
	}

	/* If attach check failed, we should not redo every setup step */
	if (gsm->attach_retries) {
		goto attaching;
	}

	if (IS_ENABLED(CONFIG_GSM_MUX) && gsm->mux_enabled) {
		ret = modem_cmd_send_nolock(&gsm->context.iface,
					    &gsm->context.cmd_handler,
					    &response_cmds[0],
					    ARRAY_SIZE(response_cmds),
					    "AT", &gsm->sem_response,
					    GSM_CMD_AT_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("modem setup returned %d, %s",
				ret, "retrying...");
			(void)k_work_reschedule(&gsm->gsm_configure_work,
						K_SECONDS(1));
			return;
		}
	}

	if (IS_ENABLED(CONFIG_MODEM_GSM_FACTORY_RESET_AT_BOOT)) {
		(void)modem_cmd_send_nolock(&gsm->context.iface,
					    &gsm->context.cmd_handler,
					    &response_cmds[0],
					    ARRAY_SIZE(response_cmds),
					    "AT&F", &gsm->sem_response,
					    GSM_CMD_AT_TIMEOUT);
		k_sleep(K_SECONDS(1));
	}

	ret = gsm_setup_mccmno(gsm);

	if (ret < 0) {
		LOG_ERR("modem setup returned %d, %s",
				ret, "retrying...");

		(void)k_work_reschedule(&gsm->gsm_configure_work,
							K_SECONDS(1));
		return;
	}

	ret = modem_cmd_handler_setup_cmds_nolock(&gsm->context.iface,
						  &gsm->context.cmd_handler,
						  setup_cmds,
						  ARRAY_SIZE(setup_cmds),
						  &gsm->sem_response,
						  GSM_CMD_SETUP_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("modem setup returned %d, %s",
			ret, "retrying...");
		(void)k_work_reschedule(&gsm->gsm_configure_work, K_SECONDS(1));
		return;
	}

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
		 * attach_retries not set        -> trigger N attach retries
		 * attach_retries set            -> decrement and retry
		 * attach_retries set, becomes 0 -> trigger full retry
		 */
		if (!gsm->attach_retries) {
			gsm->attach_retries = CONFIG_MODEM_GSM_ATTACH_TIMEOUT *
				MSEC_PER_SEC / GSM_ATTACH_RETRY_DELAY_MSEC;
		} else {
			gsm->attach_retries--;
		}

		LOG_DBG("Not attached, %s", "retrying...");

		(void)k_work_reschedule(&gsm->gsm_configure_work,
					K_MSEC(GSM_ATTACH_RETRY_DELAY_MSEC));
		return;
	}

	/* Attached, clear retry counter */
	gsm->attached = true;
	gsm->attach_retries = 0;

	LOG_DBG("modem attach returned %d, %s", ret, "read RSSI");
	gsm->rssi_retries = GSM_RSSI_RETRIES;

 attached:

	if (!IS_ENABLED(CONFIG_GSM_MUX)) {
		/* Read connection quality (RSSI) before PPP carrier is ON */
		rssi_handler(NULL);

		if (!(gsm->context.data_rssi && gsm->context.data_rssi != GSM_RSSI_INVALID &&
			gsm->context.data_rssi < GSM_RSSI_MAXVAL)) {

			LOG_DBG("Not valid RSSI, %s", "retrying...");
			if (gsm->rssi_retries-- > 0) {
				(void)k_work_reschedule(&gsm->gsm_configure_work,
							K_MSEC(GSM_RSSI_RETRY_DELAY_MSEC));
				return;
			}
		}
#if defined(CONFIG_MODEM_CELL_INFO)
		(void) gsm_query_cellinfo(gsm);
#endif
	}

	LOG_DBG("modem setup returned %d, %s", ret, "enable PPP");

	ret = modem_cmd_handler_setup_cmds_nolock(&gsm->context.iface,
						  &gsm->context.cmd_handler,
						  connect_cmds,
						  ARRAY_SIZE(connect_cmds),
						  &gsm->sem_response,
						  GSM_CMD_SETUP_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("modem setup returned %d, %s",
			ret, "retrying...");
		(void)k_work_reschedule(&gsm->gsm_configure_work, K_SECONDS(1));
		return;
	}

	gsm->setup_done = true;

	set_ppp_carrier_on(gsm);

	if (IS_ENABLED(CONFIG_GSM_MUX) && gsm->mux_enabled) {
		/* Re-use the original iface for AT channel */
		ret = modem_iface_uart_init_dev(&gsm->context.iface,
						gsm->at_dev);
		if (ret < 0) {
			LOG_DBG("iface %suart error %d", "AT ", ret);
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
				LOG_WRN("modem setup returned %d, %s",
					ret, "AT cmds failed");
			} else {
				LOG_INF("AT channel %d connected to %s",
					DLCI_AT, gsm->at_dev->name);
			}
		}
		modem_cmd_handler_tx_unlock(&gsm->context.cmd_handler);
		k_work_schedule(&rssi_work_handle, K_SECONDS(CONFIG_MODEM_GSM_RSSI_POLLING_PERIOD));
	}
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
	(void)k_work_reschedule(&gsm->gsm_configure_work, K_MSEC(1));
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
	struct gsm_modem *gsm = CONTAINER_OF(work, struct gsm_modem,
					     gsm_configure_work);
	const struct device *uart = device_get_binding(CONFIG_MODEM_GSM_UART_NAME);
	int ret;

	/* We need to call this to reactivate mux ISR. Note: This is only called
	 * after re-initing gsm_ppp.
	 */
	if (IS_ENABLED(CONFIG_GSM_MUX) &&
	    gsm->ppp_dev && gsm->state == STATE_CONTROL_CHANNEL) {
		uart_mux_enable(gsm->ppp_dev);
	}

	switch (gsm->state) {
	case STATE_CONTROL_CHANNEL:
		/* Get UART device. There is one dev / DLCI */
		if (gsm->control_dev == NULL) {
			gsm->control_dev = uart_mux_alloc();
			if (gsm->control_dev == NULL) {
				LOG_DBG("Cannot get UART mux for %s channel",
					"control");
				goto fail;
			}
		}

		gsm->state = STATE_PPP_CHANNEL;

		ret = mux_attach(gsm->control_dev, uart, DLCI_CONTROL, gsm);
		if (ret < 0) {
			goto fail;
		}

		break;

	case STATE_PPP_CHANNEL:
		if (gsm->ppp_dev == NULL) {
			gsm->ppp_dev = uart_mux_alloc();
			if (gsm->ppp_dev == NULL) {
				LOG_DBG("Cannot get UART mux for %s channel",
					"PPP");
				goto fail;
			}
		}

		gsm->state = STATE_AT_CHANNEL;

		ret = mux_attach(gsm->ppp_dev, uart, DLCI_PPP, gsm);
		if (ret < 0) {
			goto fail;
		}

		break;

	case STATE_AT_CHANNEL:
		if (gsm->at_dev == NULL) {
			gsm->at_dev = uart_mux_alloc();
			if (gsm->at_dev == NULL) {
				LOG_DBG("Cannot get UART mux for %s channel",
					"AT");
				goto fail;
			}
		}

		gsm->state = STATE_DONE;

		ret = mux_attach(gsm->at_dev, uart, DLCI_AT, gsm);
		if (ret < 0) {
			goto fail;
		}

		break;

	case STATE_DONE:
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
			gsm->mux_enabled = false;
			goto fail;
		}

		LOG_INF("PPP channel %d connected to %s",
			DLCI_PPP, gsm->ppp_dev->name);

		gsm_finalize_connection(gsm);
		break;
	}

	return;

fail:
	gsm->state = STATE_INIT;
	gsm->mux_enabled = false;
}

static void gsm_configure(struct k_work *work)
{
	struct gsm_modem *gsm = CONTAINER_OF(work, struct gsm_modem,
					     gsm_configure_work);
	int ret = -1;

	LOG_DBG("Starting modem %p configuration", gsm);

	ret = modem_cmd_send_nolock(&gsm->context.iface,
				    &gsm->context.cmd_handler,
				    &response_cmds[0],
				    ARRAY_SIZE(response_cmds),
				    "AT", &gsm->sem_response,
				    GSM_CMD_AT_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("modem not ready %d", ret);

		(void)k_work_reschedule(&gsm->gsm_configure_work, K_NO_WAIT);

		return;
	}

	if (IS_ENABLED(CONFIG_GSM_MUX) && ret == 0 &&
	    gsm->mux_enabled == false) {
		gsm->mux_setup_done = false;

		ret = mux_enable(gsm);
		if (ret == 0) {
			gsm->mux_enabled = true;
		} else {
			gsm->mux_enabled = false;
			(void)k_work_reschedule(&gsm->gsm_configure_work,
						K_NO_WAIT);
			return;
		}

		LOG_DBG("GSM muxing %s", gsm->mux_enabled ? "enabled" :
							    "disabled");

		if (gsm->mux_enabled) {
			gsm->state = STATE_INIT;

			k_work_init_delayable(&gsm->gsm_configure_work,
					      mux_setup);

			(void)k_work_reschedule(&gsm->gsm_configure_work,
						K_NO_WAIT);
			return;
		}
	}

	gsm_finalize_connection(gsm);
}

void gsm_ppp_start(const struct device *dev)
{
	struct gsm_modem *gsm = dev->data;

	/* Re-init underlying UART comms */
	int r = modem_iface_uart_init_dev(&gsm->context.iface,
				device_get_binding(CONFIG_MODEM_GSM_UART_NAME));
	if (r) {
		LOG_ERR("modem_iface_uart_init returned %d", r);
		return;
	}

	k_work_init_delayable(&gsm->gsm_configure_work, gsm_configure);
	(void)k_work_reschedule(&gsm->gsm_configure_work, K_NO_WAIT);

#if defined(CONFIG_GSM_MUX)
	k_work_init_delayable(&rssi_work_handle, rssi_handler);
#endif
}

void gsm_ppp_stop(const struct device *dev)
{
	struct gsm_modem *gsm = dev->data;
	struct net_if *iface = gsm->iface;

	net_if_l2(iface)->enable(iface, false);

	if (IS_ENABLED(CONFIG_GSM_MUX)) {
		/* Lower mux_enabled flag to trigger re-sending AT+CMUX etc */
		gsm->mux_enabled = false;

		if (gsm->ppp_dev) {
			uart_mux_disable(gsm->ppp_dev);
		}
	}

	if (modem_cmd_handler_tx_lock(&gsm->context.cmd_handler,
				      K_SECONDS(10))) {
		LOG_WRN("Failed locking modem cmds!");
	}
}

static int gsm_init(const struct device *dev)
{
	struct gsm_modem *gsm = dev->data;
	int r;

	LOG_DBG("Generic GSM modem (%p)", gsm);

	gsm->cmd_handler_data.cmds[CMD_RESP] = response_cmds;
	gsm->cmd_handler_data.cmds_len[CMD_RESP] = ARRAY_SIZE(response_cmds);
	gsm->cmd_handler_data.match_buf = &gsm->cmd_match_buf[0];
	gsm->cmd_handler_data.match_buf_len = sizeof(gsm->cmd_match_buf);
	gsm->cmd_handler_data.buf_pool = &gsm_recv_pool;
	gsm->cmd_handler_data.alloc_timeout = K_NO_WAIT;
	gsm->cmd_handler_data.eol = "\r";

	k_sem_init(&gsm->sem_response, 0, 1);

	r = modem_cmd_handler_init(&gsm->context.cmd_handler,
				   &gsm->cmd_handler_data);
	if (r < 0) {
		LOG_DBG("cmd handler error %d", r);
		return r;
	}

#if defined(CONFIG_MODEM_SHELL)
	/* modem information storage */
	gsm->context.data_manufacturer = minfo.mdm_manufacturer;
	gsm->context.data_model = minfo.mdm_model;
	gsm->context.data_revision = minfo.mdm_revision;
	gsm->context.data_imei = minfo.mdm_imei;
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	gsm->context.data_imsi = minfo.mdm_imsi;
	gsm->context.data_iccid = minfo.mdm_iccid;
#endif	/* CONFIG_MODEM_SIM_NUMBERS */
#endif	/* CONFIG_MODEM_SHELL */

	gsm->context.is_automatic_oper = false;
	gsm->gsm_data.rx_rb_buf = &gsm->gsm_rx_rb_buf[0];
	gsm->gsm_data.rx_rb_buf_len = sizeof(gsm->gsm_rx_rb_buf);

	r = modem_iface_uart_init(&gsm->context.iface, &gsm->gsm_data,
				device_get_binding(CONFIG_MODEM_GSM_UART_NAME));
	if (r < 0) {
		LOG_DBG("iface uart error %d", r);
		return r;
	}

	r = modem_context_register(&gsm->context);
	if (r < 0) {
		LOG_DBG("context error %d", r);
		return r;
	}

	LOG_DBG("iface->read %p iface->write %p",
		gsm->context.iface.read, gsm->context.iface.write);

	k_thread_create(&gsm_rx_thread, gsm_rx_stack,
			K_KERNEL_STACK_SIZEOF(gsm_rx_stack),
			(k_thread_entry_t) gsm_rx,
			gsm, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_name_set(&gsm_rx_thread, "gsm_rx");

	gsm->iface = ppp_net_if();
	if (!gsm->iface) {
		LOG_ERR("Couldn't find ppp net_if!");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_GSM_PPP_AUTOSTART)) {
		gsm_ppp_start(dev);
	}

	return 0;
}

DEVICE_DEFINE(gsm_ppp, GSM_MODEM_DEVICE_NAME, gsm_init, NULL, &gsm, NULL,
	      POST_KERNEL, CONFIG_MODEM_GSM_INIT_PRIORITY, NULL);
