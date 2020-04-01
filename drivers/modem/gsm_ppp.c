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
#include <drivers/console/uart_pipe.h>
#include <drivers/uart.h>

#include "modem_context.h"
#include "modem_iface_uart.h"
#include "modem_cmd_handler.h"

#define GSM_CMD_READ_BUF       128
#define GSM_CMD_AT_TIMEOUT     K_SECONDS(2)
#define GSM_CMD_SETUP_TIMEOUT  K_SECONDS(6)
#define GSM_RX_STACK_SIZE      1024
#define GSM_RECV_MAX_BUF       30
#define GSM_RECV_BUF_SIZE      128
#define GSM_BUF_ALLOC_TIMEOUT  K_SECONDS(1)

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

	bool setup_done;
	u8_t *ppp_recv_buf;
	size_t ppp_recv_buf_len;
	uart_pipe_recv_cb ppp_recv_cb;
	struct k_sem ppp_send_sem;
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

		if (gsm->setup_done == false) {
			gsm->context.cmd_handler.process(
						&gsm->context.cmd_handler,
						&gsm->context.iface);
			continue;
		}

		if (gsm->ppp_recv_cb == NULL || gsm->ppp_recv_buf == NULL ||
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
	/* connect to network */
	SETUP_CMD_NOHANDLE("ATD*99#")
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

static void gsm_configure(struct k_work *work)
{
	int r = -1;
	struct gsm_modem *gsm = CONTAINER_OF(work, struct gsm_modem,
					     gsm_configure_work);

	LOG_DBG("Starting modem %p configuration", gsm);

	while (r < 0) {
		while (true) {
			r = modem_cmd_send(&gsm->context.iface,
					   &gsm->context.cmd_handler,
					   &response_cmds[0],
					   ARRAY_SIZE(response_cmds),
					   "AT", &gsm->sem_response,
					   GSM_CMD_AT_TIMEOUT);
			if (r < 0) {
				LOG_DBG("modem not ready %d", r);
			} else {
				LOG_DBG("connect with modem %d", r);
				(void)gsm_setup_mccmno(gsm);
				break;
			}
		}

		r = modem_cmd_handler_setup_cmds(&gsm->context.iface,
						 &gsm->context.cmd_handler,
						 setup_cmds,
						 ARRAY_SIZE(setup_cmds),
						 &gsm->sem_response,
						 GSM_CMD_SETUP_TIMEOUT);
		if (r < 0) {
			LOG_DBG("modem setup returned %d, %s",
				r, "retrying...");
		} else {
			LOG_DBG("modem setup returned %d, %s",
				r, "enable PPP");
			break;
		}
	}

	gsm->setup_done = true;
	k_sem_give(&gsm->ppp_send_sem);
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

	r = modem_iface_uart_init(&gsm->context.iface,
				  &gsm->gsm_data, CONFIG_MODEM_GSM_UART_NAME);
	if (r < 0) {
		LOG_DBG("iface uart error %d", r);
		return r;
	}

	r = modem_context_register(&gsm->context);
	if (r < 0) {
		LOG_DBG("context error %d", r);
		return r;
	}

	k_thread_create(&gsm_rx_thread, gsm_rx_stack,
			K_THREAD_STACK_SIZEOF(gsm_rx_stack),
			(k_thread_entry_t) gsm_rx,
			gsm, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	k_delayed_work_init(&gsm->gsm_configure_work, gsm_configure);

	(void)k_delayed_work_submit(&gsm->gsm_configure_work, K_NO_WAIT);

	LOG_DBG("iface->read %p iface->write %p",
		gsm->context.iface.read, gsm->context.iface.write);
	return 0;
}

int uart_pipe_send(const u8_t *data, int len)
{
	k_sem_take(&gsm.ppp_send_sem, K_FOREVER);

	(void)gsm.context.iface.write(&gsm.context.iface, data, len);

	k_sem_give(&gsm.ppp_send_sem);

	return 0;
}

void uart_pipe_register(u8_t *buf, size_t len, uart_pipe_recv_cb cb)
{
	gsm.ppp_recv_buf = buf;
	gsm.ppp_recv_buf_len = len;
	gsm.ppp_recv_cb = cb;
}

DEVICE_INIT(gsm_ppp, "modem_gsm", gsm_init, &gsm, NULL, POST_KERNEL,
	    CONFIG_MODEM_GSM_INIT_PRIORITY);
