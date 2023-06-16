/*
 * Copyright (c) 2023 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocpp.h"
#include "ocpp_i.h"

#define FILL_METER_TABLE(_mes, _smes, _unit) [_mes] = {.mes = _mes,	\
						 .smes = _smes, .unit = _unit}

static struct {
	ocpp_meter_measurand_t	mes;
	char *smes;
	char *unit;
}mtr_ref_table[] = {
	FILL_METER_TABLE(OMM_CURRENT_FROM_EV, 
			 "Current.Export", "A"),
	FILL_METER_TABLE(OMM_CURRENT_TO_EV,
			 "Current.Import", "A"),
	FILL_METER_TABLE(OMM_CURRENT_MAX_OFFERED_TO_EV,
			 "Current.OfferedMaximum", "A"),
	FILL_METER_TABLE(OMM_ACTIVE_ENERGY_FROM_EV,
			 "Energy.Active.Export.Register", "Wh"),
	FILL_METER_TABLE(OMM_ACTIVE_ENERGY_TO_EV,
			 "Energy.Active.Import.Register", "Wh"),
	FILL_METER_TABLE(OMM_REACTIVE_ENERGY_FROM_EV,
			 "Energy.Reactive.Export.Register", "varh"),
	FILL_METER_TABLE(OMM_REACTIVE_ENERGY_TO_EV,
			 "Energy.Reactive.Import.Register", "varh"),
	FILL_METER_TABLE(OMM_ACTIVE_POWER_FROM_EV,
			 "Power.Active.Export", "W"),
	FILL_METER_TABLE(OMM_ACTIVE_POWER_TO_EV,
			 "Power.Active.Import", "W"),
	FILL_METER_TABLE(OMM_REACTIVE_POWER_FROM_EV,
			 "Power.Reactive.Export", "var"),
	FILL_METER_TABLE(OMM_REACTIVE_POWER_TO_EV,
			 "Power.Reactive.Import", "var"),
	FILL_METER_TABLE(OMM_POWERLINE_FREQ,
			 "Frequency", "Hz"),
	FILL_METER_TABLE(OMM_POWER_FACTOR,
			 "Power.Factor", ""),
	FILL_METER_TABLE(OMM_POWER_MAX_OFFERED_TO_EV,
			 "Power.Offered", "W"),
	FILL_METER_TABLE(OMM_FAN_SPEED,
			 "RPM", "rpm"),
	FILL_METER_TABLE(OMM_CHARGING_PERCENT,
			 "SoCState", "Percent"),
	FILL_METER_TABLE(OMM_TEMPERATURE, 
			 "Temperature", "Celsius"),
	FILL_METER_TABLE(OMM_VOLTAGE_AC_RMS,
			 "Voltage", "V")
};

int ocpp_boot_notification(ocpp_session_handle_t hndl,
			   ocpp_cp_info_t *cpi)
{
	int ret;
	char buf[512];
	ocpp_session_t *sh = (ocpp_session_t *)hndl;
	ocpp_info_t *ctx = sh->ctx;
	ocpp_upstream_info_t *ui = ctx->ui;
	ocpp_msg_fp_t fn;
	ocpp_wamp_rpc_msg_t rmsg = {0};

	fn = ctx->cfn[BootNotification];
	sh->uid = fn(buf, sizeof(buf), sh, cpi);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;

	ret = ocpp_send_to_server(&rmsg, K_SECONDS(3));
	if (!ret && sh->resp_status != BOOT_ACCEPTED) {
		ret = -EAGAIN;
	}

	return ret;
}

int ocpp_get_configuration(ocpp_key_t key, ocpp_info_t *ctx, char *uid)
{
	int ret;
	char buf[512];
	char tmp[32];
	char *sval = NULL;
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_keyval_t* kval;
	ocpp_msg_fp_t fn;
	bool is_rw;
	ocpp_key_type_t ktype;

	if (key >= OCPP_CFG_END) {
		return -EINVAL;
	}

	ktype = ocpp_get_keyval_type(key);
	is_rw = ocpp_is_key_rw(key);
	kval = ocpp_get_key_val(key);

	if (ktype < KEY_TYPE_STR) {
		sval = tmp;
		snprintf(tmp, sizeof(tmp), "%d", kval->ival);
	} else {
		sval = kval->str;
	}

	fn = ctx->cfn[GetConfiguration];
	fn(buf, sizeof(buf), ocpp_get_key_literal(key), sval, is_rw, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;

	ret = ocpp_send_to_server(&rmsg, K_SECONDS(2));
	return ret;
}

int ocpp_change_configuration(char *skey, ocpp_info_t *ctx,
			      char *sval, char *uid)
{
	int ret;
	char buf[250];
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_keyval_t kval;
	ocpp_msg_fp_t fn;
	ocpp_key_t key;
	char *res = "Accepted";

	key = ocpp_key_to_cfg(skey);
	if (key < OCPP_CFG_END) {
		ocpp_key_type_t ktype;

		ktype = ocpp_get_keyval_type(key);
		if (ktype < KEY_TYPE_STR) {
			kval.ival = atoi(sval);
		} else {
			kval.str = sval;
		}
		ret = ocpp_update_cfg_val(key, &kval);
		if (ret) {
			res = "Rejected";
		}
	} else {
		res = "NotSupported";
	}

	fn = ctx->cfn[ChangeConfiguration];
	fn(buf, sizeof(buf), res, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;

	ret = ocpp_send_to_server(&rmsg, K_SECONDS(2));

	// notify user !
	return ret;
}

int ocpp_authorize(ocpp_session_handle_t hndl, char *idtag,
		   auth_status_t *status,
		   uint32_t timeout_ms)
{
	int ret;
	ocpp_session_t *sh = (ocpp_session_t *)hndl;
	ocpp_info_t *ctx;
	ocpp_upstream_info_t *ui;
	int uid;
	char buf[256]; // fix me stack
	bool is_found = false;
	ocpp_keyval_t *val;
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_msg_fp_t fn;
 
	if (!idtag || !status || 
	    !ocpp_session_is_valid(sh)) {
		return -EINVAL;
	}

	ctx = sh->ctx;
	if (ctx->state < CP_STATE_READY) {
		return -EAGAIN;
	}

	if (ctx->is_cs_offline) {
		val = ocpp_get_key_val(CFG_LOCAL_AUTH_OFFLINE);
		if (val && ! val->ival) {
			return -EAGAIN;
		}
	}

	// local auth list supported and enabed, check idtag available & valid

	// cache supported and enabled, check idtag available in cache & valid
	val = ocpp_get_key_val(CFG_AUTH_CACHE_ENABLED);
	if (val && val->ival) {

	}

	// send auth req to server, update cache on response from server
	strncpy(sh->idtag, idtag, sizeof(sh->idtag));
	ui = ctx->ui;
	fn = ctx->cfn[Authorize];
	sh->uid = fn(buf, sizeof(buf), idtag);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;
	ret = ocpp_send_to_server(&rmsg, K_MSEC(timeout_ms));
	if (ret < 0) {
		return ret;
	}

	*status = sh->resp_status;
	if (sh->resp_status == AUTH_ACCEPTED) {
		sh->is_active = true;
	}

	return 0;
}

int ocpp_heartbeat(ocpp_session_handle_t hndl)
{
	int ret;
	ocpp_session_t *sh = (ocpp_session_t *)hndl;
	ocpp_info_t *ctx = sh->ctx;
	ocpp_upstream_info_t *ui = ctx->ui;
	ocpp_msg_fp_t fn;
	ocpp_wamp_rpc_msg_t rmsg = {0};
	char buf[128];

	fn = ctx->cfn[Heartbeat];
	sh->uid = fn(buf, sizeof(buf), sh);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;

	ret = ocpp_send_to_server(&rmsg, K_SECONDS(1));
	if (!ret && sh->resp_status != BOOT_ACCEPTED) {
		ret = -EAGAIN;
	}

	return ret;
}

int ocpp_start_transaction(ocpp_session_handle_t hndl,
			   int Wh,
			   uint8_t conn_id,
			   uint32_t timeout_ms)
{
	ocpp_session_t *sh = (ocpp_session_t *)hndl;
	ocpp_info_t *ctx;
	ocpp_upstream_info_t *ui;
	int uid;
	char buf[350]; // fix me stack
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_msg_fp_t fn;
	int ret;
 
	if (!ocpp_session_is_valid(sh) ||
	    conn_id <= 0) {
		return -EINVAL;
	}
	ctx = sh->ctx;
	if (ctx->state < CP_STATE_READY) {
		return -EAGAIN;
	}

	// fill meter reading
	// server offline, accept start txn and save it in Q.
	if (ctx->is_cs_offline) {
	}

	fn = ctx->cfn[StartTransaction];
	sh->uid = fn(buf, sizeof(buf), sh, Wh, -1);

	// server online, send st txn msg.
	ui = ctx->ui;
	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;
	ret = ocpp_send_to_server(&rmsg, K_MSEC(timeout_ms));

	if (sh->resp_status != AUTH_ACCEPTED) {
		//notif user about unauthorised !!
		ret = -EACCES;
	}

	return ret;
}

int ocpp_stop_transaction(ocpp_session_handle_t hndl,
			  int Wh,
			  uint32_t timeout_ms)
{
	ocpp_session_t *sh = (ocpp_session_t *)hndl;
	ocpp_info_t *ctx;
	ocpp_upstream_info_t *ui;
	int uid;
	char buf[350]; // fix me stack
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_msg_fp_t fn;
	int ret;
 
	if (!ocpp_session_is_valid(sh)) {
		return -EINVAL;
	}

	ctx = sh->ctx;
	if (ctx->state < CP_STATE_READY) {
		return -EAGAIN;
	}

	sh->is_active = true;

	// server offline, accept stop txn and save it in Q.
	if (ctx->is_cs_offline) {
	}

	fn = ctx->cfn[StopTransaction];
	sh->uid = fn(buf, sizeof(buf), sh, Wh, NULL, "timestamp");

	// on response check idtag & update cache !!
	ui = ctx->ui;
	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;
	ret = ocpp_send_to_server(&rmsg, K_MSEC(timeout_ms));

	return ret;
}

int ocpp_remote_start_transaction(ocpp_info_t *ctx, internal_msg_t *msg,
				  char *uid)
{
	char buf[350]; // fix me stack
	char *resp = "Rejected";
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_msg_fp_t fn;
	int ret;
 
	ret = k_msgq_put(ctx->msgq, msg, K_MSEC(100));
	if (!ret) {
		resp = "Accepted";
	}

	fn = ctx->cfn[RemoteStartTransaction];
	fn(buf, sizeof(buf), resp, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	ret = ocpp_send_to_server(&rmsg, K_SECONDS(2));

	return ret;
}

int ocpp_remote_stop_transaction(ocpp_info_t *ctx, internal_msg_t *msg,
				 int idtxn, char *uid)
{
	char buf[350]; // fix me stack
	char *resp = "Rejected";
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_msg_fp_t fn;
	sys_snode_t *node;
	ocpp_session_t *sh;
	bool is_found = false;
	int ret;

	k_mutex_lock(&ctx->ilock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&ctx->slist, node) {
		sh = to_session(node);
		if (sh->is_active &&
		    sh->idtxn == idtxn) {
			is_found = true;
			break;
		}
	}
	k_mutex_unlock(&ctx->ilock);

	if (is_found) {
		msg->usr.stop_charge.id_con = sh->idcon;
		ret = k_msgq_put(ctx->msgq, msg, K_MSEC(100));
		if (!ret) {
			resp = "Accepted";
		}
	}

	fn = ctx->cfn[RemoteStartTransaction];
	fn(buf, sizeof(buf), resp, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	ret = ocpp_send_to_server(&rmsg, K_SECONDS(2));

	return ret;
}

int ocpp_unlock_connector(ocpp_info_t *ctx, internal_msg_t *msg, char *uid)
{
	char buf[350]; // fix me stack
	char *resp = "UnlockFailed";
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_msg_fp_t fn;
	sys_snode_t *node;
	bool is_found = false;
	int ret;

	ret = k_msgq_put(ctx->msgq, msg, K_MSEC(100));
	if (!ret) {
		resp = "Unlocked";
	}

	fn = ctx->cfn[UnlockConnector];
	fn(buf, sizeof(buf), resp, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	ret = ocpp_send_to_server(&rmsg, K_SECONDS(2));

	return ret;
}

int ocpp_meter_values(ocpp_session_handle_t hndl,
		      ocpp_meter_measurand_t mes,
		      char *sval)
{
	ocpp_session_t *sh = (ocpp_session_t *)hndl;
	ocpp_info_t *ctx = sh->ctx;
	ocpp_upstream_info_t *ui = ctx->ui;
	char buf[350]; // fix me stack
	ocpp_wamp_rpc_msg_t rmsg = {0};
	ocpp_msg_fp_t fn;
	int ret;

	//get time stamp
	// if msg not empty or offline add to q
	if (ctx->is_cs_offline) {
		return;
	}

	fn = ctx->cfn[MeterValues];
	sh->uid = fn(buf, sizeof(buf), sh, "timestamp", sval,
			mtr_ref_table[mes].smes, 
			mtr_ref_table[mes].unit);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf) + 1;
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;
	ret = ocpp_send_to_server(&rmsg, K_SECONDS(3));

	return ret;
}
