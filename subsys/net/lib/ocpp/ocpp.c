/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocpp_i.h"

#include <time.h>

#include <zephyr/sys/clock.h>

LOG_MODULE_REGISTER(ocpp, CONFIG_OCPP_LOG_LEVEL);

#define OCPP_UPSTREAM_PRIORITY		7
#define OCPP_WS_TIMEOUT			5000
#define OCPP_INTERNAL_MSG_SIZE		(sizeof(struct internal_msg))
#define FILL_PDU_TABLE(_pdu, _spdu)[_pdu] = {_pdu, _spdu, }

struct ocpp_msg_table {
	enum ocpp_pdu_msg pdu;
	char *spdu;
};

static K_THREAD_STACK_DEFINE(ocpp_int_handler_stack, CONFIG_OCPP_INT_THREAD_STACKSIZE);
static K_THREAD_STACK_DEFINE(ocpp_wsreader_stack, CONFIG_OCPP_WSREADER_THREAD_STACKSIZE);

K_MSGQ_DEFINE(ocpp_iq, OCPP_INTERNAL_MSG_SIZE, CONFIG_OCPP_INTERNAL_MSGQ_CNT, sizeof(uint32_t));

struct ocpp_info *gctx;

static struct ocpp_msg_table pdu_msg_table[] = {
	FILL_PDU_TABLE(PDU_BOOTNOTIFICATION, "BootNotification"),
	FILL_PDU_TABLE(PDU_AUTHORIZE, "Authorize"),
	FILL_PDU_TABLE(PDU_START_TRANSACTION, "StartTransaction"),
	FILL_PDU_TABLE(PDU_STOP_TRANSACTION, "StopTransaction"),
	FILL_PDU_TABLE(PDU_HEARTBEAT, "Heartbeat"),
	FILL_PDU_TABLE(PDU_METER_VALUES, "MeterValues"),
	FILL_PDU_TABLE(PDU_CLEAR_CACHE, "ClearCache"),
	FILL_PDU_TABLE(PDU_REMOTE_START_TRANSACTION, "RemoteStartTransaction"),
	FILL_PDU_TABLE(PDU_REMOTE_STOP_TRANSACTION, "RemoteStopTransaction"),
	FILL_PDU_TABLE(PDU_GET_CONFIGURATION, "GetConfiguration"),
	FILL_PDU_TABLE(PDU_CHANGE_CONFIGURATION, "ChangeConfiguration"),
	FILL_PDU_TABLE(PDU_CHANGE_AVAILABILITY, "ChangeAvailability"),
	FILL_PDU_TABLE(PDU_UNLOCK_CONNECTOR, "UnlockConnector"),
	FILL_PDU_TABLE(PDU_RESET, "Reset"),
};

const char *ocpp_get_pdu_literal(enum ocpp_pdu_msg pdu)
{
	if (pdu >= PDU_MSG_END) {
		return "";
	}

	return pdu_msg_table[pdu].spdu;
}

int ocpp_find_pdu_from_literal(const char *msg)
{
	struct ocpp_msg_table *mt = pdu_msg_table;
	int i;

	for (i = 0; i < PDU_MSG_END; i++) {
		if (strncmp(mt[i].spdu, msg, strlen(mt[i].spdu)) == 0) {
			break;
		}
	}

	return i;
}

void ocpp_get_utc_now(char utc[CISTR25])
{
	struct timespec ts;
	struct tm htime = {0};

	sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);
	gmtime_r(&ts.tv_sec, &htime);

	snprintk(utc, CISTR25, "%04hu-%02hu-%02huT%02hu:%02hu:%02huZ",
		htime.tm_year + 1900,
		htime.tm_mon + 1,
		htime.tm_mday,
		htime.tm_hour,
		htime.tm_min,
		htime.tm_sec);
}

bool ocpp_session_is_valid(struct ocpp_session *sh)
{
	sys_snode_t *node;
	struct ocpp_session *lsh;
	bool is_found = false;

	if (sh == NULL) {
		return is_found;
	}

	k_mutex_lock(&gctx->ilock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&gctx->slist, node) {
		lsh = to_session(node);
		if (lsh == sh) {
			is_found = true;
			break;
		}
	}
	k_mutex_unlock(&gctx->ilock);

	return is_found;
}

static int ocpp_ws_connect_cb(int ws_sock, struct http_request *req,
			      void *user_data)
{
	struct ocpp_info *ctx = user_data;

	ctx->is_cs_offline = false;
	return 0;
}

static int ocpp_connect_to_cs(struct ocpp_info *ctx)
{
	int ret;
	struct websocket_request config = {0};
	struct ocpp_upstream_info *ui = &ctx->ui;
	struct sockaddr addr_buf;
	struct sockaddr *addr = &addr_buf;
	int addr_size;

	if (ui->csi.sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		addr_size = sizeof(struct sockaddr_in);
		addr->sa_family = ui->csi.sa_family;
		net_sin(addr)->sin_port = htons(ui->csi.port);
		zsock_inet_pton(addr->sa_family, ui->csi.cs_ip,
				&net_sin(addr)->sin_addr);
#else
		return -EAFNOSUPPORT;
#endif
	} else {
#if defined(CONFIG_NET_IPV6)
		addr_size = sizeof(struct sockaddr_in6);
		addr->sa_family = ui->csi.sa_family;
		net_sin6(addr)->sin6_port = htons(ui->csi.port);
		zsock_inet_pton(addr->sa_family, ui->csi.cs_ip,
				&net_sin6(addr)->sin6_addr);
#else
		return -EAFNOSUPPORT;
#endif
	}

	if (ui->tcpsock >= 0) {
		zsock_close(ui->tcpsock);
	}

	ui->tcpsock = zsock_socket(ui->csi.sa_family, SOCK_STREAM,
				   IPPROTO_TCP);
	if (ui->tcpsock < 0) {
		return -errno;
	}

	ret = zsock_connect(ui->tcpsock, addr, addr_size);
	if (ret < 0 && errno != EALREADY && errno != EISCONN) {
		LOG_ERR("tcp socket connect fail %d %d", ret, errno);
		return ret;
	}

	if (ui->wssock >= 0) {
		websocket_disconnect(ui->wssock);
		ui->wssock = -1;
	}

	if (ui->wssock < 0) {
		char buf[128];
		char const *optional_hdr[] = {
					"Sec-WebSocket-Protocol: ocpp1.6\r\n",
					NULL };

		snprintk(buf, sizeof(buf), "%s:%u", ui->csi.cs_ip, ui->csi.port);
		config.url = ui->csi.ws_url;
		config.host = buf;
		config.tmp_buf = ui->wsrecv_buf;
		config.tmp_buf_len = sizeof(ui->wsrecv_buf);
		config.cb = ocpp_ws_connect_cb;
		config.optional_headers = optional_hdr;

		ret = websocket_connect(ui->tcpsock, &config,
					OCPP_WS_TIMEOUT, ctx);
		if (ret < 0) {
			LOG_ERR("Websocket connect fail %d", ret);
			return ret;
		}
		ui->wssock = ret;
	}

	LOG_DBG("WS connect success %d", ui->wssock);
	return 0;
}

static inline void bootnotification_free_resource(struct ocpp_cp_info *cpi)
{
	free(cpi->model);
	free(cpi->vendor);
	free(cpi->sl_no);
	free(cpi->box_sl_no);
	free(cpi->fw_ver);
	free(cpi->iccid);
	free(cpi->imsi);
	free(cpi->meter_sl_no);
	free(cpi->meter_type);
	k_free(cpi);
}

static void ocpp_internal_handler(void *p1, void *p2, void *p3)
{
	struct ocpp_info *ctx = p1;
	struct ocpp_cp_info *cpi = p2;
	ocpp_session_handle_t sh;
	struct internal_msg msg = {0};
	int ret;
	int i;

	/* open internal session */
	ocpp_session_open(&sh);

	while (!k_msgq_get(ctx->msgq, &msg, K_FOREVER)) {
		switch (msg.msgtype) {
		case PDU_BOOTNOTIFICATION:

			if (!ctx->is_cs_offline && cpi) {
				ret = ocpp_boot_notification(sh, cpi);
				if (ret == 0) {
					bootnotification_free_resource(cpi);
					cpi = NULL;
					ctx->state = CP_STATE_READY;
				}
			}

			k_timer_start(&ctx->hb_timer,
				      K_SECONDS(ctx->hb_sec),
				      K_NO_WAIT);
			break;

		case PDU_METER_VALUES:
		{
			union ocpp_io_value io;
			sys_snode_t *node;
			struct ocpp_session *lsh;

			/* list of all active session */
			k_mutex_lock(&ctx->ilock, K_FOREVER);
			SYS_SLIST_FOR_EACH_NODE(&ctx->slist, node) {
				lsh = to_session(node);
				if (lsh == sh ||
				    !lsh->is_active) {
					continue;
				}

				k_mutex_lock(&lsh->slock, K_FOREVER);
				k_mutex_unlock(&ctx->ilock);
				io.meter_val.id_con = lsh->idcon;
				for (i = 0; i < OCPP_OMM_END; i++) {
					io.meter_val.mes = i;
					ret = ctx->cb(OCPP_USR_GET_METER_VALUE,
						      &io, ctx->user_data);
					if (ret < 0) {
						continue;
					}
					ocpp_meter_values(lsh, i,
							  io.meter_val.val);
				}
				k_mutex_lock(&ctx->ilock, K_FOREVER);
				k_mutex_unlock(&lsh->slock);
			}
			k_mutex_unlock(&ctx->ilock);
			break;
		}
		case PDU_HEARTBEAT:
			ocpp_heartbeat(sh);
			/* adjust local time with cs time ! */
			k_timer_start(&ctx->hb_timer,
				      K_SECONDS(ctx->hb_sec),
				      K_NO_WAIT);
			break;

		case PDU_CS_ONLINE:
			/* check offline msg and do nothing on empty
			 * else read msg and send to server
			 */
			break;

		case PDU_REMOTE_START_TRANSACTION:
			ctx->cb(OCPP_USR_START_CHARGING, &msg.usr, ctx->user_data);
			break;

		case PDU_REMOTE_STOP_TRANSACTION:
			ctx->cb(OCPP_USR_STOP_CHARGING, &msg.usr, ctx->user_data);
			break;

		case PDU_UNLOCK_CONNECTOR:
			ctx->cb(OCPP_USR_UNLOCK_CONNECTOR, &msg.usr, ctx->user_data);
			break;

		default:
			break;
		}
	}
}

static int ocpp_process_server_msg(struct ocpp_info *ctx)
{
	ocpp_msg_fp_t fn;
	struct ocpp_session *sh = NULL;
	struct ocpp_upstream_info *ui = &ctx->ui;
	char *buf, *tmp;
	char uid[128];
	int ret, idtxn, pdu, i;
	bool is_rsp;
	struct internal_msg msg;

	ret = parse_rpc_msg(ui->recv_buf, sizeof(ui->recv_buf),
			    uid, sizeof(uid), &pdu, &is_rsp);
	if (ret < 0) {
		return ret;
	}

	if (is_rsp) {
		buf = strtok_r(uid, "-", &tmp);
		sh = (struct ocpp_session *)(uintptr_t)atoi(buf);

		buf = strtok_r(NULL, "-", &tmp);
		pdu = atoi(buf);

		if (!ocpp_session_is_valid(sh)) {
			sh = NULL;
		}
	}

	if (pdu >= PDU_MSG_END) {
		return -EINVAL;
	}

	fn = ctx->pfn[pdu];

	switch (pdu) {
	char skey[CISTR50];

	case PDU_BOOTNOTIFICATION:
	{
		struct boot_notif binfo;

		ret = fn(ui->recv_buf, &binfo);
		if (ret == 0 && sh != NULL) {
			sh->resp_status = binfo.status;
			ctx->hb_sec = binfo.interval;
		}
		break;
	}
	case PDU_AUTHORIZE:
	case PDU_STOP_TRANSACTION:
	{
		struct ocpp_idtag_info idinfo = {0};

		if (sh == NULL) {
			break;
		}

		ret = fn(ui->recv_buf, &idinfo);
		if (ret == 0) {
			sh->resp_status = idinfo.auth_status;
		}
		break;
	}
	case PDU_START_TRANSACTION:
	{
		struct ocpp_idtag_info idinfo = {0};
		if (sh == NULL) {
			break;
		}

		ret = fn(ui->recv_buf, &idtxn, &idinfo);
		if (ret == 0) {
			sh->idtxn = idtxn;
			sh->resp_status = idinfo.auth_status;
		}
		break;
	}
	case PDU_GET_CONFIGURATION:
		memset(skey, 0, sizeof(skey));

		ret = fn(ui->recv_buf, skey);
		if (ret != 0) {
			break;
		}

		if (*skey != 0) {
			enum ocpp_key key;

			key = ocpp_key_to_cfg(skey);
			ocpp_get_configuration(key, ctx, uid);
		} else {
			for (i = 0; i < OCPP_CFG_END; i++) {
				ocpp_get_configuration(i, ctx, uid);
			}
		}

		break;

	case PDU_CHANGE_CONFIGURATION:
	{
		char sval[CISTR500] = {0};

		memset(skey, 0, sizeof(skey));
		ret = fn(ui->recv_buf, skey, sval);
		if (ret < 0) {
			break;
		}

		ocpp_change_configuration(skey, ctx, sval, uid);
		break;
	}
	case PDU_HEARTBEAT:
		/* todo : sync time */
		break;

	case PDU_REMOTE_START_TRANSACTION:
		memset(&msg, 0, OCPP_INTERNAL_MSG_SIZE);
		msg.msgtype = PDU_REMOTE_START_TRANSACTION;

		ret = fn(ui->recv_buf, &msg.usr.start_charge.id_con,
			 msg.usr.start_charge.idtag);
		if (ret < 0) {
			break;
		}

		ocpp_remote_start_transaction(ctx, &msg, uid);
		break;

	case PDU_REMOTE_STOP_TRANSACTION:
		memset(&msg, 0, OCPP_INTERNAL_MSG_SIZE);
		msg.msgtype = PDU_REMOTE_STOP_TRANSACTION;

		ret = fn(ui->recv_buf, &idtxn);
		if (ret < 0) {
			break;
		}

		ocpp_remote_stop_transaction(ctx, &msg, idtxn, uid);
		break;

	case PDU_UNLOCK_CONNECTOR:
		memset(&msg, 0, OCPP_INTERNAL_MSG_SIZE);
		msg.msgtype = PDU_UNLOCK_CONNECTOR;

		ret = fn(ui->recv_buf, &msg.usr.unlock_con.id_con);
		if (ret < 0) {
			break;
		}

		ocpp_unlock_connector(ctx, &msg, uid);
		break;

	case PDU_METER_VALUES:
	default:
		break;
	}

	if (is_rsp) {
		k_poll_signal_raise(&ui->ws_rspsig, 0);
	}
	return 0;
}

#define TCP_CONNECT_AFTER 20 /* fixme arbitrary */
static void ocpp_wsreader(void *p1, void *p2, void *p3)
{
	struct ocpp_info *ctx = p1;
	struct ocpp_upstream_info *ui = &ctx->ui;
	struct zsock_pollfd tcpfd;
	int ret;
	uint8_t retry_cnt = 0;

	ctx->is_cs_offline = true;
	tcpfd.events = ZSOCK_POLLIN;

	while (1) {

		if (ctx->is_cs_offline) {
			if ((retry_cnt++ % TCP_CONNECT_AFTER) == 0) {
				k_mutex_lock(&ctx->ilock, K_FOREVER);
				ret = ocpp_connect_to_cs(ctx);
				k_mutex_unlock(&ctx->ilock);

				if (ret != 0) {
					continue;
				}
			} else {
				/* delay reconnection attempt */
				k_msleep(200);
				continue;
			}
		}

		tcpfd.fd = ui->tcpsock;
		ret = zsock_poll(&tcpfd, 1, 200);
		if (ret <= 0) {
			continue;
		}

		if ((tcpfd.revents & ZSOCK_POLLERR) ||
		    (tcpfd.revents & ZSOCK_POLLNVAL)) {
			LOG_ERR("poll err %d", tcpfd.revents);
			ctx->is_cs_offline = true;
			continue;
		}

		if (tcpfd.revents & ZSOCK_POLLIN) {
			struct ocpp_wamp_rpc_msg rcv = {0};
			uint32_t msg_type;

			rcv.ctx = ctx;
			rcv.msg = ui->recv_buf;
			rcv.sndlock = &ui->ws_sndlock;
			rcv.msg_len = sizeof(ui->recv_buf);

			memset(ui->recv_buf, 0, rcv.msg_len);
			ret = ocpp_receive_from_server(&rcv, &msg_type, 200);

			if (ret < 0) {
				if (ret == -ENOTCONN) {
					ctx->is_cs_offline = true;
				}
				continue;
			}

			if (msg_type & WEBSOCKET_FLAG_PING) {
				websocket_send_msg(ctx->ui.wssock, NULL, 0,
						   WEBSOCKET_OPCODE_PONG, true,
						   true, 100);
			} else if (msg_type & WEBSOCKET_FLAG_CLOSE) {
				ctx->is_cs_offline = true;
			} else {
				ocpp_process_server_msg(ctx);
			}

		}

		if (tcpfd.revents & ZSOCK_POLLHUP) {
			LOG_ERR("poll err %d", tcpfd.revents);
			ctx->is_cs_offline = true;
		}
	}
}

int ocpp_upstream_init(struct ocpp_info *ctx, struct ocpp_cs_info *csi)
{
	struct ocpp_upstream_info *ui = &ctx->ui;

	LOG_INF("upstream init");

	ui->csi.ws_url = strdup(csi->ws_url);
	ui->csi.cs_ip = strdup(csi->cs_ip);
	ui->csi.port = csi->port;
	ui->csi.sa_family = csi->sa_family;
	ui->tcpsock = -1;

	k_mutex_init(&ui->ws_sndlock);
	k_poll_signal_init(&ui->ws_rspsig);
	ui->wssock = -1;

	websocket_init();

	k_thread_create(&ui->tinfo, ocpp_wsreader_stack,
			CONFIG_OCPP_WSREADER_THREAD_STACKSIZE,
			ocpp_wsreader, ctx, NULL, NULL,
			OCPP_UPSTREAM_PRIORITY,	0, K_MSEC(100));

	return 0;
}

static void timer_heartbeat_cb(struct k_timer *t)
{
	struct ocpp_info *ctx;
	struct internal_msg msg = {PDU_HEARTBEAT};

	ctx = k_timer_user_data_get(t);
	if (ctx->state == CP_STATE_BOOTNOTIF) {
		msg.msgtype = PDU_BOOTNOTIFICATION;
	}

	k_msgq_put(ctx->msgq, &msg, K_NO_WAIT);
}

static void timer_meter_cb(struct k_timer *t)
{
	struct ocpp_info *ctx;
	struct internal_msg msg = {PDU_METER_VALUES};

	ctx = k_timer_user_data_get(t);

	k_msgq_put(ctx->msgq, &msg, K_NO_WAIT);
}

static inline void bootnotification_fill_resource(struct ocpp_cp_info *cp,
						  struct ocpp_cp_info *cpi,
						  struct ocpp_info *ctx)
{
	struct k_timer *hbt = &ctx->hb_timer;

	cp->model = strdup(cpi->model);
	cp->vendor = strdup(cpi->vendor);
	if (cp->sl_no != NULL) {
		cp->sl_no = strdup(cpi->sl_no);
	}

	if (cp->box_sl_no != NULL) {
		cp->box_sl_no = strdup(cpi->box_sl_no);
	}

	if (cp->fw_ver != NULL) {
		cp->fw_ver = strdup(cpi->fw_ver);
	}

	if (cp->iccid != NULL) {
		cp->iccid = strdup(cpi->iccid);
	}

	if (cp->imsi != NULL) {
		cp->imsi = strdup(cpi->imsi);
	}

	if (cp->meter_sl_no != NULL) {
		cp->meter_sl_no = strdup(cpi->meter_sl_no);
	}

	if (cp->meter_type != NULL) {
		cp->meter_type = strdup(cpi->meter_type);
	}

	ctx->state = CP_STATE_BOOTNOTIF;
	ctx->hb_sec = 10;
	k_timer_start(hbt, K_SECONDS(1), K_NO_WAIT);
}

int ocpp_session_open(ocpp_session_handle_t *hndl)
{
	struct ocpp_session *sh;

	if (hndl == NULL) {
		return -EINVAL;
	}

	sh = (struct ocpp_session *)k_calloc(1, sizeof(struct ocpp_session));
	if (sh == NULL) {
		return -ENOMEM;
	}

	sh->is_active = false;
	sh->idcon = INVALID_CONN_ID;
	sh->idtxn = INVALID_TXN_ID;
	sh->ctx = gctx;
	k_mutex_init(&sh->slock);

	k_mutex_lock(&gctx->ilock, K_FOREVER);
	sys_slist_append(&gctx->slist, &sh->node);
	k_mutex_unlock(&gctx->ilock);

	*hndl = (void *)sh;

	return 0;
}

void ocpp_session_close(ocpp_session_handle_t hndl)
{
	bool is_removed;
	struct ocpp_session *sh = (struct ocpp_session *)hndl;

	if (sh == NULL) {
		return;
	}

	k_mutex_lock(&gctx->ilock, K_FOREVER);
	is_removed = sys_slist_find_and_remove(&gctx->slist, &sh->node);
	k_mutex_unlock(&gctx->ilock);

	if (is_removed) {
		k_mutex_lock(&sh->slock, K_FOREVER);
		k_free(sh);
	}
}

int ocpp_init(struct ocpp_cp_info *cpi,
	      struct ocpp_cs_info *csi,
	      ocpp_user_notify_callback_t cb,
	      void *user_data)
{
	struct ocpp_info *ctx;
	struct ocpp_cp_info *cp;
	union ocpp_keyval val;
	int ret;

	if (cpi == NULL ||
	    cpi->model == NULL ||
	    cpi->vendor == NULL ||
	    csi == NULL ||
	    csi->cs_ip == NULL ||
	    csi->ws_url == NULL ||
	    cb == NULL ||
	    cpi->num_of_con < 1) {
		return -EINVAL;
	}

	ctx = (struct ocpp_info *)k_calloc(1, sizeof(struct ocpp_info));
	if (ctx == NULL) {
		return -ENOMEM;
	}

	gctx = ctx;
	k_mutex_init(&ctx->ilock);
	sys_slist_init(&ctx->slist);
	ocpp_parser_init(&ctx->cfn, &ctx->pfn);

	ctx->state = CP_STATE_INIT;
	ctx->msgq = &ocpp_iq;

	k_timer_init(&ctx->hb_timer, timer_heartbeat_cb, NULL);
	k_timer_user_data_set(&ctx->hb_timer, ctx);
	k_timer_init(&ctx->mtr_timer, timer_meter_cb, NULL);
	k_timer_user_data_set(&ctx->mtr_timer, ctx);
	atomic_set(&ctx->mtr_timer_ref_cnt, 0);

	ctx->user_data = user_data;
	ctx->cb = cb;
	val.ival = cpi->num_of_con;
	ocpp_set_cfg_val(CFG_NO_OF_CONNECTORS, &val);

	ret = ocpp_upstream_init(ctx, csi);
	if (ret < 0) {
		LOG_ERR("ocpp upstream init fail %d", ret);
		goto out;
	}

	/* free after success of bootnotification to CS */
	cp = (struct ocpp_cp_info *)k_calloc(1, sizeof(struct ocpp_cp_info));
	if (cp == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	bootnotification_fill_resource(cp, cpi, ctx);

	k_thread_create(&ctx->tinfo, ocpp_int_handler_stack,
			CONFIG_OCPP_INT_THREAD_STACKSIZE,
			ocpp_internal_handler, ctx,
			cp, NULL, OCPP_UPSTREAM_PRIORITY,
			0, K_NO_WAIT);

	LOG_INF("ocpp init success");
	return 0;

out:
	k_free(ctx);
	return ret;
}
