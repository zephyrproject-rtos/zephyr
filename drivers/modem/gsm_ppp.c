/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(modem_gsm, CONFIG_MODEM_LOG_LEVEL);

#include <kernel.h>
#include <device.h>
#include <sys/ring_buffer.h>
#include <sys/util.h>
#include <net/ppp.h>
#include <drivers/uart.h>
#include <drivers/console/uart_pipe.h>
#include <drivers/console/uart_mux.h>

#include "modem_context.h"
#include "modem_iface_uart.h"
#include "modem_cmd_handler.h"
#include "../console/gsm_mux.h"

#define GSM_CMD_READ_BUF       128
#define GSM_CMD_AT_TIMEOUT     K_SECONDS(2)
#define GSM_CMD_SETUP_TIMEOUT  K_SECONDS(6)
#define GSM_RX_STACK_SIZE      1024
#define GSM_RECV_MAX_BUF       30
#define GSM_RECV_BUF_SIZE      128
#define GSM_BUF_ALLOC_TIMEOUT  K_SECONDS(1)

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
	u8_t cmd_read_buf[GSM_CMD_READ_BUF];
	u8_t cmd_match_buf[GSM_CMD_READ_BUF];
	struct k_sem sem_response;

	struct modem_iface_uart_data gsm_data;
	struct k_delayed_work gsm_configure_work;
	char gsm_isr_buf[PPP_MRU];
	char gsm_rx_rb_buf[PPP_MRU * 3];

	u8_t *ppp_recv_buf;
	size_t ppp_recv_buf_len;
	uart_pipe_recv_cb ppp_recv_cb;
	struct k_sem ppp_send_sem;

	enum setup_state state;
	struct device *ppp_dev;
	struct device *at_dev;
	struct device *control_dev;

	bool mux_enabled : 1;
	bool mux_setup_done : 1;
	bool setup_done : 1;
} gsm;

static size_t recv_buf_offset;

NET_BUF_POOL_DEFINE(gsm_recv_pool, GSM_RECV_MAX_BUF, GSM_RECV_BUF_SIZE,
		    0, NULL);
K_THREAD_STACK_DEFINE(gsm_rx_stack, GSM_RX_STACK_SIZE);

struct k_thread gsm_rx_thread;

static void gsm_rx(struct gsm_modem *gsm)
{
	int bytes, r;

	LOG_DBG("starting");

	while (true) {
		k_sem_take(&gsm->gsm_data.rx_sem, K_FOREVER);

		if (gsm->mux_enabled == false) {
			if (gsm->setup_done == false) {
				gsm->context.cmd_handler.process(
						&gsm->context.cmd_handler,
						&gsm->context.iface);
				continue;
			}

			if (gsm->ppp_recv_cb == NULL ||
			    gsm->ppp_recv_buf == NULL ||
			    gsm->ppp_recv_buf_len == 0) {
				return;
			}

			r = gsm->context.iface.read(
					&gsm->context.iface,
					&gsm->ppp_recv_buf[recv_buf_offset],
					gsm->ppp_recv_buf_len -
					recv_buf_offset,
					&bytes);
			if (r < 0 || bytes == 0) {
				continue;
			}

			recv_buf_offset += bytes;

			gsm->ppp_recv_buf = gsm->ppp_recv_cb(gsm->ppp_recv_buf,
							     &recv_buf_offset);
		} else if (IS_ENABLED(CONFIG_GSM_MUX) && gsm->mux_enabled) {
			/* The handler will listen AT channel */
			gsm->context.cmd_handler.process(
						&gsm->context.cmd_handler,
						&gsm->context.iface);
		}
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

static struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", gsm_cmd_ok, 0U, ""),
	MODEM_CMD("ERROR", gsm_cmd_error, 0U, ""),
	MODEM_CMD("CONNECT", gsm_cmd_ok, 0U, ""),
};

#if defined(CONFIG_MODEM_SHELL)
#define MDM_MANUFACTURER_LENGTH  10
#define MDM_MODEL_LENGTH         16
#define MDM_REVISION_LENGTH      64
#define MDM_IMEI_LENGTH          16

struct modem_info {
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];
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
#endif /* CONFIG_MODEM_SHELL */

static struct setup_cmd setup_cmds[] = {
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
	SETUP_CMD("AT+CGSN", "", on_cmd_atcmdinfo_imei, 0U, ""),
#endif

	/* disable unsolicited network registration codes */
	SETUP_CMD_NOHANDLE("AT+CREG=0"),

	/* create PDP context */
	SETUP_CMD_NOHANDLE("AT+CGDCONT=1,\"IP\",\"" CONFIG_MODEM_GSM_APN "\""),
};

static struct setup_cmd connect_cmds[] = {
	/* connect to network */
	SETUP_CMD_NOHANDLE("ATD*99#"),
};

static int gsm_setup_mccmno(struct gsm_modem *gsm)
{
	int ret;

	if (CONFIG_MODEM_GSM_MANUAL_MCCMNO[0]) {
		/* use manual MCC/MNO entry */
		ret = modem_cmd_send(&gsm->context.iface,
				     &gsm->context.cmd_handler,
				     NULL, 0,
				     "AT+COPS=1,2,\""
				     CONFIG_MODEM_GSM_MANUAL_MCCMNO
				     "\"",
				     &gsm->sem_response,
				     GSM_CMD_AT_TIMEOUT);
	} else {
		/* register operator automatically */
		ret = modem_cmd_send(&gsm->context.iface,
				     &gsm->context.cmd_handler,
				     NULL, 0, "AT+COPS=0,0",
				     &gsm->sem_response,
				     GSM_CMD_AT_TIMEOUT);
	}

	if (ret < 0) {
		LOG_ERR("AT+COPS ret:%d", ret);
	}

	return ret;
}

static void set_ppp_carrier_on(struct gsm_modem *gsm)
{
	struct device *ppp_dev = device_get_binding(CONFIG_NET_PPP_DRV_NAME);
	struct net_if *iface;

	if (!ppp_dev) {
		LOG_ERR("Cannot find PPP %s!", "device");
		return;
	}

	iface = net_if_lookup_by_dev(ppp_dev);
	if (!iface) {
		LOG_ERR("Cannot find PPP %s!", "network interface");
		return;
	}

	net_ppp_carrier_on(iface);
}

static void gsm_finalize_connection(struct gsm_modem *gsm)
{
	int ret;

	if (IS_ENABLED(CONFIG_GSM_MUX) && gsm->mux_enabled) {
		ret = modem_cmd_send(&gsm->context.iface,
				     &gsm->context.cmd_handler,
				     &response_cmds[0],
				     ARRAY_SIZE(response_cmds),
				     "AT", &gsm->sem_response,
				     GSM_CMD_AT_TIMEOUT);
		if (ret < 0) {
			LOG_DBG("modem setup returned %d, %s",
				ret, "retrying...");
			(void)k_delayed_work_submit(&gsm->gsm_configure_work,
						    K_SECONDS(1));
			return;
		}
	}

	(void)gsm_setup_mccmno(gsm);

	ret = modem_cmd_handler_setup_cmds(&gsm->context.iface,
					   &gsm->context.cmd_handler,
					   setup_cmds,
					   ARRAY_SIZE(setup_cmds),
					   &gsm->sem_response,
					   GSM_CMD_SETUP_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("modem setup returned %d, %s",
			ret, "retrying...");
		(void)k_delayed_work_submit(&gsm->gsm_configure_work,
					    K_SECONDS(1));
		return;
	}

	LOG_DBG("modem setup returned %d, %s", ret, "enable PPP");

	ret = modem_cmd_handler_setup_cmds(&gsm->context.iface,
					   &gsm->context.cmd_handler,
					   connect_cmds,
					   ARRAY_SIZE(connect_cmds),
					   &gsm->sem_response,
					   GSM_CMD_SETUP_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("modem setup returned %d, %s",
			ret, "retrying...");
		(void)k_delayed_work_submit(&gsm->gsm_configure_work,
					    K_SECONDS(1));
		return;
	}

	gsm->setup_done = true;

	/* FIXME: This will let PPP to start send data. We should actually
	 * change this so that the PPP L2 is initialized after the GSM modem
	 * is working and connection is created. TBDL.
	 */
	k_sem_give(&gsm->ppp_send_sem);

	set_ppp_carrier_on(gsm);
}

static int mux_enable(struct gsm_modem *gsm)
{
	int ret;

	/* Turn on muxing */
	if (IS_ENABLED(CONFIG_MODEM_GSM_SIMCOM)) {
		ret = modem_cmd_send(
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
		ret = modem_cmd_send(&gsm->context.iface,
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
	(void)k_delayed_work_submit(&gsm->gsm_configure_work, K_MSEC(1));
}

static void mux_attach_cb(struct device *mux, int dlci_address,
			  bool connected, void *user_data)
{
	LOG_DBG("DLCI %d to %s %s", dlci_address, mux->config->name,
		connected ? "connected" : "disconnected");

	if (connected) {
		uart_irq_rx_enable(mux);
		uart_irq_tx_enable(mux);
	}

	mux_setup_next(user_data);
}

static int mux_attach(struct device *mux, struct device *uart,
		      int dlci_address, void *user_data)
{
	int ret = uart_mux_attach(mux, uart, dlci_address, mux_attach_cb,
				  user_data);
	if (ret < 0) {
		LOG_ERR("Cannot attach DLCI %d (%s) to %s (%d)", dlci_address,
			mux->config->name, uart->config->name, ret);
		return ret;
	}

	return 0;
}

static void mux_setup(struct k_work *work)
{
	struct gsm_modem *gsm = CONTAINER_OF(work, struct gsm_modem,
					     gsm_configure_work);
	struct device *uart = device_get_binding(CONFIG_MODEM_GSM_UART_NAME);
	int ret;

	switch (gsm->state) {
	case STATE_CONTROL_CHANNEL:
		/* Get UART device. There is one dev / DLCI */
		gsm->control_dev = uart_mux_alloc();
		if (gsm->control_dev == NULL) {
			LOG_DBG("Cannot get UART mux for %s channel",
				"control");
			goto fail;
		}

		gsm->state = STATE_PPP_CHANNEL;

		ret = mux_attach(gsm->control_dev, uart, DLCI_CONTROL, gsm);
		if (ret < 0) {
			goto fail;
		}

		break;

	case STATE_PPP_CHANNEL:
		gsm->ppp_dev = uart_mux_alloc();
		if (gsm->ppp_dev == NULL) {
			LOG_DBG("Cannot get UART mux for %s channel", "PPP");
			goto fail;
		}

		gsm->state = STATE_AT_CHANNEL;

		ret = mux_attach(gsm->ppp_dev, uart, DLCI_PPP, gsm);
		if (ret < 0) {
			goto fail;
		}

		break;

	case STATE_AT_CHANNEL:
		gsm->at_dev = uart_mux_alloc();
		if (gsm->at_dev == NULL) {
			LOG_DBG("Cannot get UART mux for %s channel", "AT");
			goto fail;
		}

		gsm->state = STATE_DONE;

		ret = mux_attach(gsm->at_dev, uart, DLCI_AT, gsm);
		if (ret < 0) {
			goto fail;
		}

		break;

	case STATE_DONE:
		/* Re-use the original iface for AT channel */
		ret = modem_iface_uart_init(&gsm->context.iface,
					    &gsm->gsm_data,
					    gsm->at_dev->config->name);
		if (ret < 0) {
			LOG_DBG("iface %suart error %d", "mux ", ret);
			gsm->mux_enabled = false;
			goto fail;
		}

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

	ret = modem_cmd_send(&gsm->context.iface,
			     &gsm->context.cmd_handler,
			     &response_cmds[0],
			     ARRAY_SIZE(response_cmds),
			     "AT", &gsm->sem_response,
			     GSM_CMD_AT_TIMEOUT);
	if (ret < 0) {
		LOG_DBG("modem not ready %d", ret);

		(void)k_delayed_work_submit(&gsm->gsm_configure_work, 0);

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
		}

		LOG_DBG("GSM muxing %s", gsm->mux_enabled ? "enabled" :
							    "disabled");

		if (gsm->mux_enabled) {
			gsm->state = STATE_INIT;

			k_delayed_work_init(&gsm->gsm_configure_work,
					    mux_setup);

			(void)k_delayed_work_submit(&gsm->gsm_configure_work,
						    0);
			return;
		}
	}

	gsm_finalize_connection(gsm);
}

static int gsm_init(struct device *device)
{
	struct gsm_modem *gsm = device->driver_data;
	int r;

	LOG_DBG("Generic GSM modem (%p)", gsm);

	k_sem_init(&gsm->ppp_send_sem, 0, 1);

	gsm->cmd_handler_data.cmds[CMD_RESP] = response_cmds;
	gsm->cmd_handler_data.cmds_len[CMD_RESP] = ARRAY_SIZE(response_cmds);
	gsm->cmd_handler_data.read_buf = &gsm->cmd_read_buf[0];
	gsm->cmd_handler_data.read_buf_len = sizeof(gsm->cmd_read_buf);
	gsm->cmd_handler_data.match_buf = &gsm->cmd_match_buf[0];
	gsm->cmd_handler_data.match_buf_len = sizeof(gsm->cmd_match_buf);
	gsm->cmd_handler_data.buf_pool = &gsm_recv_pool;
	gsm->cmd_handler_data.alloc_timeout = GSM_BUF_ALLOC_TIMEOUT;
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
#endif

	gsm->gsm_data.isr_buf = &gsm->gsm_isr_buf[0];
	gsm->gsm_data.isr_buf_len = sizeof(gsm->gsm_isr_buf);
	gsm->gsm_data.rx_rb_buf = &gsm->gsm_rx_rb_buf[0];
	gsm->gsm_data.rx_rb_buf_len = sizeof(gsm->gsm_rx_rb_buf);

	r = modem_iface_uart_init(&gsm->context.iface, &gsm->gsm_data,
				  CONFIG_MODEM_GSM_UART_NAME);
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
			K_THREAD_STACK_SIZEOF(gsm_rx_stack),
			(k_thread_entry_t) gsm_rx,
			gsm, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	k_delayed_work_init(&gsm->gsm_configure_work, gsm_configure);

	(void)k_delayed_work_submit(&gsm->gsm_configure_work, K_NO_WAIT);

	return 0;
}

int uart_pipe_send(const u8_t *buf, int len)
{
	k_sem_take(&gsm.ppp_send_sem, K_FOREVER);

	(void)gsm.context.iface.write(&gsm.context.iface, buf, len);

	k_sem_give(&gsm.ppp_send_sem);

	return 0;
}


/* Setup the connection to PPP. PPP driver will call this function. */
void uart_pipe_register(u8_t *buf, size_t len, uart_pipe_recv_cb cb)
{
	gsm.ppp_recv_buf = buf;
	gsm.ppp_recv_buf_len = len;
	gsm.ppp_recv_cb = cb;
}

DEVICE_INIT(gsm_ppp, "modem_gsm", gsm_init, &gsm, NULL, POST_KERNEL,
	    CONFIG_MODEM_GSM_INIT_PRIORITY);
