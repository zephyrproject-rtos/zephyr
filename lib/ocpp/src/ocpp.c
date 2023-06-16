/*
 * Copyright (c) 2023 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocpp.h"
#include "ocpp_i.h"

LOG_MODULE_REGISTER(ocpp, LOG_LEVEL_INF);
#define OCPP_UPSTREAM_PRIORITY          7
#define FILL_PDU_TABLE(_pdu)  [_pdu] = {_pdu, #_pdu, }

K_THREAD_STACK_DEFINE(ocpp_int_handler_stack, CONFIG_OCPP_INT_THREAD_STACKSIZE);
K_THREAD_STACK_DEFINE(ocpp_wsreader_stack, CONFIG_OCPP_WSREADER_THREAD_STACKSIZE);

ocpp_info_t *gctx;
static ocpp_msg_table_t pdu_msg_table[] = {
	FILL_PDU_TABLE(BootNotification),
	FILL_PDU_TABLE(Authorize),
	FILL_PDU_TABLE(StartTransaction),
	FILL_PDU_TABLE(StopTransaction),
	FILL_PDU_TABLE(Heartbeat),
	FILL_PDU_TABLE(MeterValues),
	FILL_PDU_TABLE(ClearCache),
	FILL_PDU_TABLE(RemoteStartTransaction),
	FILL_PDU_TABLE(RemoteStopTransaction),
	FILL_PDU_TABLE(GetConfiguration),
	FILL_PDU_TABLE(ChangeConfiguration),
	FILL_PDU_TABLE(ChangeAvailability),
	FILL_PDU_TABLE(UnlockConnector),
	FILL_PDU_TABLE(Reset),
};

char *ocpp_get_pdu_literal(ocpp_pdu_msg_t pdu)
{
	if (pdu >= PduMsgEnd) {
		return "";
	}

	return pdu_msg_table[pdu].spdu;
}

int ocpp_find_pdu_from_literal(char *msg)
{
	ocpp_msg_table_t *mt = pdu_msg_table;
	int i;

	for (i = 0; i < PduMsgEnd; i++) {
		if (!strncmp(mt[i].spdu, msg, strlen(mt[i].spdu))) {
			break;
		}
	}

	return i;
}

bool ocpp_session_is_valid(ocpp_session_t *sh)
{
	sys_snode_t *node;
	ocpp_session_t *lsh;
	bool is_found = false;

	if (!sh)
		return is_found;

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

static int ocpp_live()
{
	/* ping pong */
}

static int ocpp_ws_connect_cb(int ws_sock, struct http_request *req,
			      void *user_data)
{
	ocpp_info_t *ctx = user_data;

	ctx->is_cs_offline = false;
	return 0;
}

static int ocpp_connect_to_cs(ocpp_info_t *ctx)
{
	int ret;
	struct websocket_request config = {0};
	ocpp_upstream_info_t *ui = ctx->ui;
	struct sockaddr addr4;
	struct sockaddr_in6 addr6;
	struct sockaddr *addr;
	int addr_size;

	if (ui->csi.sa_family == AF_INET) {
		addr = &addr4;
		addr_size = sizeof(addr4);
		addr->sa_family = ui->csi.sa_family;
		net_sin(addr)->sin_port = htons(ui->csi.port);
		zsock_inet_pton(addr->sa_family, ui->csi.cs_ip,
				&net_sin(addr)->sin_addr);
	} else {
		addr = (struct sockaddr *)&addr6;
		addr_size = sizeof(addr6);
		addr->sa_family = ui->csi.sa_family;
		net_sin6(addr)->sin6_port = htons(ui->csi.port);
		zsock_inet_pton(addr->sa_family, ui->csi.cs_ip,
				&net_sin6(&addr)->sin6_addr);
	}

	ret = zsock_connect(ui->tcpsock, addr, addr_size);
	if (ret < 0 && errno != -EALREADY && errno != EISCONN) {
		LOG_ERR("tcp socket connect fail %d", ret);
		return ret;
	}

	if (ui->wssock < 0) {
		char buf[128];
		char *optional_hdr[] = {
					"Sec-WebSocket-Protocol: ocpp1.6\r\n",
					NULL };

		snprintf(buf, sizeof(buf), "%s:%u", ui->csi.cs_ip, ui->csi.port);
		config.url = ui->csi.ws_url;
		config.host = buf;
		config.tmp_buf = ui->wsrecv_buf;
		config.tmp_buf_len = CONFIG_OCPP_RECV_BUFFER_SIZE;
		config.cb = ocpp_ws_connect_cb;
		config.optional_headers = optional_hdr;

		ret = websocket_connect(ui->tcpsock, &config, 5000, ctx);
		if (ret < 0) {
			LOG_ERR("Websocket connect fail %d", ret);
			return ret;
		}
		ui->wssock = ret;
	}

	LOG_DBG("WS connect success");
	return 0;
}

static inline void bootnotification_free_resource(ocpp_cp_info_t *cpi)
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
	ocpp_info_t *ctx = p1;
	ocpp_cp_info_t *cpi = p2;
	ocpp_upstream_info_t *ui = ctx->ui;
	ocpp_session_handle_t sh;
	internal_msg_t msg = {0};
	int ret;
	int i;

	// open internal session !!
	ocpp_session_open(&sh);

	while (!k_msgq_get(ctx->msgq, &msg, K_FOREVER)) {
		switch (msg.msgtype) {
		case BootNotification:

			if (!ctx->is_cs_offline) {
				ret = ocpp_boot_notification(sh, cpi);
				if (!ret) {
					bootnotification_free_resource(cpi);
					cpi = NULL;
					ctx->state = CP_STATE_READY;
				}
			}

			k_timer_start(&ctx->hb_timer,
				      K_SECONDS(ctx->hb_sec),
				      K_NO_WAIT);
			break;

		case MeterValues:
			ocpp_io_value_t io;
			sys_snode_t *node;
			ocpp_session_t *lsh;

			// list of all active session
			k_mutex_lock(&ctx->ilock, K_FOREVER);
			SYS_SLIST_FOR_EACH_NODE(&ctx->slist, node) {
				lsh = to_session(node);
				if (lsh == sh ||
				    !lsh->is_active) {
					continue;
				}

				io.meter_val.id_con = lsh->idcon;
				for (i = 0; i < OMM_END; i++) {
					io.meter_val.mes = i;
					ret = ctx->cb(OCPP_USR_GET_METER_VALUE,
						      &io, ctx->user_data);
					if (!ret) {
						continue;
					}
					ocpp_meter_values(sh, i,
							  io.meter_val.val);
				}
			}
			k_mutex_unlock(&ctx->ilock);
			break;

		case Heartbeat:
			ocpp_heartbeat(sh);
			// adjust local time with cs time !
			k_timer_start(&ctx->hb_timer,
				      K_SECONDS(ctx->hb_sec),
				      K_NO_WAIT);
			break;

		case CS_Online:
			//check offline msg and do nothing on empty
			//else read msg and send to server
			break;

		case RemoteStartTransaction:
		case RemoteStopTransaction:
		case UnlockConnector:
			ctx->cb(msg.msgtype, &msg.usr, ctx->user_data);
			break;

		default:
			break;
		}
	}
}

static int ocpp_process_server_msg(ocpp_info_t *ctx)
{
	ocpp_msg_fp_t fn;
	ocpp_session_t *sh = NULL;
	ocpp_upstream_info_t *ui = ctx->ui;
	char *buf, *tmp;
	char uid[128];
	int ret, idtxn, pdu, i;
	bool is_rsp;
	internal_msg_t msg;

	ret = parse_rpc_msg(ui->recv_buf, CONFIG_OCPP_RECV_BUFFER_SIZE,
			    uid, sizeof(uid), &pdu, &is_rsp);
	if (ret) {
		return ret;
	}

	if (is_rsp) {
		buf = strtok_r(uid, "-", &tmp);
		sh = atoi(buf);

		buf = strtok_r(NULL, "-", &tmp);
		pdu = atoi(buf);

		if (!ocpp_session_is_valid(sh)) {
			sh = NULL;
		}
	}

	if (pdu >= PduMsgEnd) {
		return -EINVAL;
	}

	fn = ctx->pfn[pdu];
	switch (pdu) {

	char skey[CISTR50];

	case BootNotification:
		boot_notif_t binfo;

		ret = fn(ui->recv_buf, &binfo);
		if (!ret && sh) {
			sh->resp_status = binfo.status;
			ctx->hb_sec = binfo.interval;
		}
		break;

	case Authorize:
	case StopTransaction:
		ocpp_idtag_info_t idinfo = {0};

		if (!sh) {
			break;
		}

		ret = fn(ui->recv_buf, &idinfo);
		if (!ret) {
			// update auth cache
			sh->resp_status = idinfo.auth_status;
		}
		break;

	case StartTransaction:
		if (!sh) {
			break;
		}

		ret = fn(ui->recv_buf, &idtxn, &idinfo);
		if (!ret) {
			// update auth cache
			sh->idtxn = idtxn;
			sh->resp_status = idinfo.auth_status;
		}
		break;

	case GetConfiguration:
		memset(skey, 0, sizeof(skey));

		ret = fn(ui->recv_buf, skey);
		if (ret) {
			break;
		}

		if (*skey) {
			ocpp_key_t key;

			key = ocpp_key_to_cfg(skey);
			ocpp_get_configuration(key, ctx, uid);
		} else {
			for(i = 0; i < OCPP_CFG_END; i++) {
				ocpp_get_configuration(i, ctx, uid);
			}
		}

		break;

	case ChangeConfiguration:
		char sval[CISTR500] = {0};

		memset(skey, 0, sizeof(skey));
		ret = fn(ui->recv_buf, skey, sval);
		if (ret) {
			break;
		}

		ocpp_change_configuration(skey, ctx, sval, uid);
		break;

	case Heartbeat:
		//sync time
		break;

	case RemoteStartTransaction:
		memset(&msg, 0, sizeof(internal_msg_t));
		msg.msgtype = RemoteStartTransaction;

		ret = fn(ui->recv_buf, &msg.usr.start_charge.id_con,
			 msg.usr.start_charge.idtag);
		if (ret) {
			break;
		}

		ocpp_remote_start_transaction(ctx, &msg, uid);
		break;

	case RemoteStopTransaction:
		memset(&msg, 0, sizeof(internal_msg_t));
		msg.msgtype = RemoteStopTransaction;

		ret = fn(ui->recv_buf, &idtxn);
		if (ret) {
			break;
		}

		ocpp_remote_stop_transaction(ctx, &msg, idtxn, uid);
		break;

	case UnlockConnector:
		memset(&msg, 0, sizeof(internal_msg_t));
		msg.msgtype = RemoteStopTransaction;

		ret = fn(ui->recv_buf, &msg.usr.unlock_con.id_con);
		if (ret) {
			break;
		}

		ocpp_remote_stop_transaction(ctx, &msg, uid);

		break;

	case MeterValues: //do nothing
	default :
		break;
	}

	if (is_rsp) {
		k_poll_signal_raise(&ui->ws_rspsig, 0);
	}
	return 0;
}

#define TCP_CONNECT_AFTER 20 //fixme arbitrary
static void ocpp_wsreader(void *p1, void *p2, void *p3)
{
	ocpp_info_t *ctx = p1;
	ocpp_upstream_info_t *ui = ctx->ui;
	struct zsock_pollfd tcpfd;
	uint8_t ret;
	uint8_t retry_cnt = 0;

	ctx->is_cs_offline = true;
	tcpfd.fd = ui->tcpsock;
	tcpfd.events = ZSOCK_POLLIN | ZSOCK_POLLERR
			| ZSOCK_POLLHUP | ZSOCK_POLLNVAL;

	while (1) {

		if (ctx->is_cs_offline &&
		    !(retry_cnt++ % TCP_CONNECT_AFTER)) {

			ui->wssock = -1;
			k_mutex_lock(&ctx->ilock, K_FOREVER);
			ocpp_connect_to_cs(ctx);
			k_mutex_unlock(&ctx->ilock);
		}

		ret = zsock_poll(&tcpfd, 1, 200);
		if (ret <= 0) {
			continue;
		}

		if ((tcpfd.revents & ZSOCK_POLLERR) ||
		    (tcpfd.revents & ZSOCK_POLLHUP) ||
		    (tcpfd.revents & ZSOCK_POLLNVAL) ) {
			LOG_ERR("poll err %d", tcpfd.revents);
			ctx->is_cs_offline = true;
			continue;
		}

		if (tcpfd.revents & ZSOCK_POLLIN) {
			ocpp_wamp_rpc_msg_t rcv = {0};

			rcv.ctx = ctx;
			rcv.msg = ui->recv_buf;
			rcv.sndlock = &ui->ws_sndlock;
			rcv.msg_len = CONFIG_OCPP_RECV_BUFFER_SIZE;

			memset(ui->recv_buf, 0, rcv.msg_len);
			ret = ocpp_receive_from_server(&rcv, 200);
			if (ret < 0) {
				continue;
			}

			ocpp_process_server_msg(ctx);
		}
	}
}

int ocpp_upstream_init(ocpp_info_t *ctx, ocpp_cs_info_t *csi)
{
	int ret;
	ocpp_upstream_info_t *ui;
	uint8_t retry = 5;

	LOG_INF("upstream init");

	ui = k_calloc(1, sizeof(ocpp_upstream_info_t));
	if (!ui) {
		return -ENOMEM;
	}

	ui->wsrecv_buf = k_calloc(1, CONFIG_OCPP_RECV_BUFFER_SIZE * 2);
	if (!ui->wsrecv_buf) {
		ret = -ENOMEM;
		goto out;
	}

	ui->recv_buf = k_calloc(1, CONFIG_OCPP_RECV_BUFFER_SIZE);
	if (!ui->recv_buf) {
		ret = -ENOMEM;
		goto out;
	}

	ui->csi.ws_url = strdup(csi->ws_url);
	ui->csi.cs_ip = strdup(csi->cs_ip);
	ui->csi.port = csi->port;
	ui->csi.sa_family = csi->sa_family;
	ui->tcpsock = zsock_socket(csi->sa_family, SOCK_STREAM,
				   IPPROTO_TCP);
	if (ui->tcpsock < 0) {
		ret = -errno;
		goto out;
	}

	k_mutex_init(&ui->ws_sndlock);
	k_poll_signal_init(&ui->ws_rspsig);
	ui->wssock = -1;
	ctx->ui = ui;

	websocket_init();

	k_thread_create(&ui->tinfo, ocpp_wsreader_stack,
			CONFIG_OCPP_WSREADER_THREAD_STACKSIZE,
			ocpp_wsreader, ctx, NULL, NULL,
			OCPP_UPSTREAM_PRIORITY,	0, K_MSEC(100));

	return 0;

out:
	if (ui) {
		k_free(ui->wsrecv_buf);
		k_free(ui->recv_buf);
	}
	k_free(ui);

	return ret;
}

static void timer_heartbeat_cb(struct k_timer *t)
{
	ocpp_info_t *ctx;
	internal_msg_t msg = {Heartbeat};

	ctx = k_timer_user_data_get(t);
	if (ctx->state == CP_STATE_BOOTNOTIF) {
		msg.msgtype = BootNotification;
	}

	k_msgq_put(ctx->msgq, &msg, K_NO_WAIT);
}

static void timer_meter_cb(struct k_timer *t)
{
	ocpp_info_t *ctx;
	internal_msg_t msg = {MeterValues};

	ctx = k_timer_user_data_get(t);

	k_msgq_put(ctx->msgq, &msg, K_NO_WAIT);
}

static inline void bootnotification_fill_resource(ocpp_cp_info_t *cp,
						  ocpp_cp_info_t *cpi,
						  ocpp_info_t *ctx)
{
	struct k_timer *hbt = &ctx->hb_timer;

	cp->model = strdup(cpi->model);
	cp->vendor = strdup(cpi->vendor);
	if (cp->sl_no) {
		cp->sl_no = strdup(cpi->sl_no);
	}

	if (cp->box_sl_no) {
		cp->box_sl_no = strdup(cpi->box_sl_no);
	}

	if (cp->fw_ver) {
		cp->fw_ver = strdup(cpi->fw_ver);
	}

	if (cp->iccid) {
		cp->iccid = strdup(cpi->iccid);
	}

	if (cp->imsi) {
		cp->imsi = strdup(cpi->imsi);
	}

	if (cp->meter_sl_no) {
		cp->meter_sl_no = strdup(cpi->meter_sl_no);
	}

	if (cp->meter_type) {
		cp->meter_type = strdup(cpi->meter_type);
	}

	ctx->state = CP_STATE_BOOTNOTIF;
	ctx->hb_sec = 10;
	k_timer_start(hbt, K_SECONDS(5), K_NO_WAIT);
}

int ocpp_session_open(ocpp_session_handle_t *hndl)
{
	ocpp_session_t *sh;

	if (!hndl)
		return -EINVAL;

	sh = (ocpp_session_t *)k_calloc(1, sizeof(ocpp_session_t));
	if (!sh)
		return -ENOMEM;

	sh->is_active = false;
	sh->idcon = INVALID_CONN_ID;
	sh->idtxn = INVALID_TXN_ID;
	sh->ctx = gctx;

	k_mutex_lock(&gctx->ilock, K_FOREVER);
	sys_slist_append(&gctx->slist, &sh->node);
	k_mutex_unlock(&gctx->ilock);

	*hndl = (void *)sh;

	return 0;
}

void ocpp_session_close(ocpp_session_handle_t hndl)
{
	bool is_removed;
	ocpp_session_t *sh = (ocpp_session_t *)hndl;

	if (!sh) {
		return;
	}

	k_mutex_lock(&gctx->ilock, K_FOREVER);
	is_removed = sys_slist_find_and_remove(&gctx->slist, &sh->node);
	k_mutex_unlock(&gctx->ilock);

	if (is_removed)
		k_free(sh);
}

K_MSGQ_DEFINE(ocpp_iq, sizeof(internal_msg_t), 		\
	 	CONFIG_OCPP_INTERNAL_MSGQ_CNT, sizeof(uint32_t));

int ocpp_init(ocpp_cp_info_t *cpi,
	      ocpp_cs_info_t *csi,
	      ocpp_user_notify_callback_t cb,
	      void *user_data)
{
	ocpp_info_t *ctx;
	ocpp_cp_info_t *cp;
	ocpp_keyval_t val;
	int ret;

	if (!cpi ||
	    !cpi->model ||
	    !cpi->vendor ||
	    !csi ||
	    !csi->cs_ip ||
	    !csi->ws_url ||
	    !cb ||
	    cpi->num_of_con < 1) {
		return -EINVAL;
	}
 
	ctx = (ocpp_info_t *)k_calloc(1, sizeof(ocpp_info_t));
	if (!ctx) {
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

	ctx->user_data = user_data;
	ctx->cb = cb;
	val.ival = cpi->num_of_con;
	ocpp_set_cfg_val(CFG_NO_OF_CONNECTORS, &val);

	ret = ocpp_upstream_init(ctx, csi);
	if (ret) {
		LOG_ERR("ocpp upstream init fail %d", ret);
		goto out;
	}

	// free after success of bootnotification to CS
	cp = (ocpp_cp_info_t *)k_calloc(1, sizeof(ocpp_cp_info_t));
	if (!cp) {
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
