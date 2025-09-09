/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocpp_i.h"

#define OCPP_PDU_TIMEOUT 2
#define OCPP_USER_REQ_PDU_BUF 350
#define FILL_METER_TABLE(_mes, _smes, _unit)[_mes] = {.mes = _mes,	\
						 .smes = _smes, .unit = _unit}

static struct {
	enum ocpp_meter_measurand	mes;
	const char * const smes;
	const char * const unit;
} mtr_ref_table[] = {
	FILL_METER_TABLE(OCPP_OMM_CURRENT_FROM_EV,
			 "Current.Export", "A"),
	FILL_METER_TABLE(OCPP_OMM_CURRENT_TO_EV,
			 "Current.Import", "A"),
	FILL_METER_TABLE(OCPP_OMM_CURRENT_MAX_OFFERED_TO_EV,
			 "Current.OfferedMaximum", "A"),
	FILL_METER_TABLE(OCPP_OMM_ACTIVE_ENERGY_FROM_EV,
			 "Energy.Active.Export.Register", "Wh"),
	FILL_METER_TABLE(OCPP_OMM_ACTIVE_ENERGY_TO_EV,
			 "Energy.Active.Import.Register", "Wh"),
	FILL_METER_TABLE(OCPP_OMM_REACTIVE_ENERGY_FROM_EV,
			 "Energy.Reactive.Export.Register", "varh"),
	FILL_METER_TABLE(OCPP_OMM_REACTIVE_ENERGY_TO_EV,
			 "Energy.Reactive.Import.Register", "varh"),
	FILL_METER_TABLE(OCPP_OMM_ACTIVE_POWER_FROM_EV,
			 "Power.Active.Export", "W"),
	FILL_METER_TABLE(OCPP_OMM_ACTIVE_POWER_TO_EV,
			 "Power.Active.Import", "W"),
	FILL_METER_TABLE(OCPP_OMM_REACTIVE_POWER_FROM_EV,
			 "Power.Reactive.Export", "var"),
	FILL_METER_TABLE(OCPP_OMM_REACTIVE_POWER_TO_EV,
			 "Power.Reactive.Import", "var"),
	FILL_METER_TABLE(OCPP_OMM_POWERLINE_FREQ,
			 "Frequency", NULL),
	FILL_METER_TABLE(OCPP_OMM_POWER_FACTOR,
			 "Power.Factor", NULL),
	FILL_METER_TABLE(OCPP_OMM_POWER_MAX_OFFERED_TO_EV,
			 "Power.Offered", NULL),
	FILL_METER_TABLE(OCPP_OMM_FAN_SPEED,
			 "RPM", "rpm"),
	FILL_METER_TABLE(OCPP_OMM_CHARGING_PERCENT,
			 "SoCState", "Percent"),
	FILL_METER_TABLE(OCPP_OMM_TEMPERATURE,
			 "Temperature", "Celsius"),
	FILL_METER_TABLE(OCPP_OMM_VOLTAGE_AC_RMS,
			 "Voltage", "V")
};

int ocpp_boot_notification(ocpp_session_handle_t hndl,
			   struct ocpp_cp_info *cpi)
{
	int ret;
	struct ocpp_session *sh = (struct ocpp_session *)hndl;
	struct ocpp_info *ctx = sh->ctx;
	struct ocpp_upstream_info *ui = &ctx->ui;
	char *buf = ctx->pdu_buf;
	ocpp_msg_fp_t fn;
	struct ocpp_wamp_rpc_msg rmsg = {0};

	fn = ctx->cfn[PDU_BOOTNOTIFICATION];
	sh->uid = fn(buf, sizeof(ctx->pdu_buf), sh, cpi);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;

	ret = ocpp_send_to_server(&rmsg, K_SECONDS(OCPP_PDU_TIMEOUT));
	if (ret == 0 && sh->resp_status != BOOT_ACCEPTED) {
		ret = -EAGAIN;
	}

	return ret;
}

int ocpp_get_configuration(enum ocpp_key key, struct ocpp_info *ctx, char *uid)
{
	int ret;
	char tmp[32];
	char *sval = NULL;
	char *buf = ctx->pdu_buf;
	struct ocpp_wamp_rpc_msg rmsg = {0};
	union ocpp_keyval *kval;
	ocpp_msg_fp_t fn;
	bool is_rw;
	enum ocpp_key_type ktype;

	if (key >= OCPP_CFG_END) {
		return -EINVAL;
	}

	ktype = ocpp_get_keyval_type(key);
	is_rw = ocpp_is_key_rw(key);
	kval = ocpp_get_key_val(key);

	if (ktype < KEY_TYPE_STR) {
		sval = tmp;
		snprintk(tmp, sizeof(tmp), "%d", kval->ival);
	} else {
		sval = kval->str;
	}

	fn = ctx->cfn[PDU_GET_CONFIGURATION];
	fn(buf, sizeof(ctx->pdu_buf), ocpp_get_key_literal(key), sval, is_rw, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);

	ret = ocpp_send_to_server(&rmsg, K_SECONDS(OCPP_PDU_TIMEOUT));
	return ret;
}

int ocpp_change_configuration(char *skey, struct ocpp_info *ctx,
			      char *sval, char *uid)
{
	int ret = -EINVAL;
	char *buf = ctx->pdu_buf;
	struct ocpp_wamp_rpc_msg rmsg = {0};
	union ocpp_keyval kval;
	ocpp_msg_fp_t fn;
	enum ocpp_key key;
	const char *res = "Accepted";

	key = ocpp_key_to_cfg(skey);
	if (key < OCPP_CFG_END) {
		enum ocpp_key_type ktype;

		ktype = ocpp_get_keyval_type(key);

		if (ktype < KEY_TYPE_STR) {
			kval.ival = atoi(sval);
		} else {
			kval.str = sval;
		}

		ret = ocpp_update_cfg_val(key, &kval);
		if (ret < 0) {
			res = "Rejected";
		}
	} else {
		res = "NotSupported";
	}

	if (ret == 0) {
		switch (key) {
		case CFG_MTR_VAL_SAMPLE_INTERVAL:
			if (atomic_get(&ctx->mtr_timer_ref_cnt) <= 0) {
				break;
			}

			k_timer_start(&ctx->mtr_timer,
				      K_SECONDS(kval.ival),
				      K_SECONDS(kval.ival));
			break;

		default:
			break;
		}
	}

	fn = ctx->cfn[PDU_CHANGE_CONFIGURATION];
	fn(buf, sizeof(ctx->pdu_buf), res, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);

	ret = ocpp_send_to_server(&rmsg, K_SECONDS(OCPP_PDU_TIMEOUT));

	return ret;
}

int ocpp_authorize(ocpp_session_handle_t hndl, char *idtag,
		   enum ocpp_auth_status *status,
		   uint32_t timeout_ms)
{
	int ret;
	struct ocpp_session *sh = (struct ocpp_session *)hndl;
	struct ocpp_info *ctx;
	struct ocpp_upstream_info *ui;
	char buf[OCPP_USER_REQ_PDU_BUF];
	union ocpp_keyval *val;
	struct ocpp_wamp_rpc_msg rmsg = {0};
	ocpp_msg_fp_t fn;

	if (idtag == NULL || status == NULL || !ocpp_session_is_valid(sh)) {
		return -EINVAL;
	}

	ctx = sh->ctx;
	if (ctx->state < CP_STATE_READY) {
		return -EAGAIN;
	}

	if (ctx->is_cs_offline) {
		val = ocpp_get_key_val(CFG_LOCAL_AUTH_OFFLINE);
		if (val != NULL && val->ival == 0) {
			return -EAGAIN;
		}
	}

	strncpy(sh->idtag, idtag, sizeof(sh->idtag));
	ui = &ctx->ui;
	fn = ctx->cfn[PDU_AUTHORIZE];
	sh->uid = fn(buf, sizeof(buf), sh);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;
	ret = ocpp_send_to_server(&rmsg, K_MSEC(timeout_ms));
	if (ret < 0) {
		return ret;
	}

	*status = sh->resp_status;
	if (sh->resp_status == OCPP_AUTH_ACCEPTED) {
		sh->is_active = true;
	}

	return 0;
}

int ocpp_heartbeat(ocpp_session_handle_t hndl)
{
	int ret;
	struct ocpp_session *sh = (struct ocpp_session *)hndl;
	struct ocpp_info *ctx = sh->ctx;
	struct ocpp_upstream_info *ui = &ctx->ui;
	ocpp_msg_fp_t fn;
	struct ocpp_wamp_rpc_msg rmsg = {0};
	char *buf = ctx->pdu_buf;

	fn = ctx->cfn[PDU_HEARTBEAT];
	sh->uid = fn(buf, sizeof(ctx->pdu_buf), sh);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;

	ret = ocpp_send_to_server(&rmsg, K_SECONDS(OCPP_PDU_TIMEOUT));
	if (ret == 0 && sh->resp_status != BOOT_ACCEPTED) {
		ret = -EAGAIN;
	}

	return ret;
}

int ocpp_start_transaction(ocpp_session_handle_t hndl,
			   int meter_val,
			   uint8_t conn_id,
			   uint32_t timeout_ms)
{
	struct ocpp_session *sh = (struct ocpp_session *)hndl;
	struct ocpp_info *ctx;
	struct ocpp_upstream_info *ui;
	char buf[OCPP_USER_REQ_PDU_BUF];
	struct ocpp_wamp_rpc_msg rmsg = {0};
	ocpp_msg_fp_t fn;
	char utc[CISTR25] = {0};
	atomic_val_t ref;
	int ret;

	if (!ocpp_session_is_valid(sh) ||
	    conn_id <= 0) {
		return -EINVAL;
	}
	ctx = sh->ctx;
	sh->idcon = conn_id;
	if (ctx->state < CP_STATE_READY) {
		return -EAGAIN;
	}

	if (ctx->is_cs_offline) {
		/* todo: fill meter reading
		 * server offline, accept start txn and save it in Q.
		 */
		return 0;
	}

	fn = ctx->cfn[PDU_START_TRANSACTION];
	ocpp_get_utc_now(utc);
	sh->uid = fn(buf, sizeof(buf), sh, meter_val, -1, utc);

	ui = &ctx->ui;
	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;
	ret = ocpp_send_to_server(&rmsg, K_MSEC(timeout_ms));
	if (ret < 0) {
		return ret;
	}

	if (sh->resp_status != OCPP_AUTH_ACCEPTED) {
		sh->is_active = false;
		ret = -EACCES;
	} else {
		union ocpp_keyval *keyval;

		keyval = ocpp_get_key_val(CFG_MTR_VAL_SAMPLE_INTERVAL);
		do {
			ref = atomic_get(&ctx->mtr_timer_ref_cnt);
			if (ref == 0) {
				k_timer_start(&ctx->mtr_timer,
					      K_SECONDS(keyval->ival),
					      K_SECONDS(keyval->ival));
			}
		} while (!atomic_cas(&ctx->mtr_timer_ref_cnt, ref, ref + 1));
	}

	return ret;
}

int ocpp_stop_transaction(ocpp_session_handle_t hndl,
			  int meter_val,
			  uint32_t timeout_ms)
{
	struct ocpp_session *sh = (struct ocpp_session *)hndl;
	struct ocpp_info *ctx;
	struct ocpp_upstream_info *ui;
	char buf[OCPP_USER_REQ_PDU_BUF];
	char utc[CISTR25] = {0};
	struct ocpp_wamp_rpc_msg rmsg = {0};
	ocpp_msg_fp_t fn;
	atomic_val_t ref;
	int ret;

	if (!ocpp_session_is_valid(sh)) {
		return -EINVAL;
	}

	ctx = sh->ctx;
	if (ctx->state < CP_STATE_READY) {
		return -EAGAIN;
	}

	sh->is_active = false;
	do {
		ref = atomic_get(&ctx->mtr_timer_ref_cnt);
		if (ref == 0) {
			k_timer_stop(&ctx->mtr_timer);
			break;
		}
	} while (!atomic_cas(&ctx->mtr_timer_ref_cnt, ref, ref - 1));

	if (ctx->is_cs_offline) {
		/* todo: fill meter reading
		 * server offline, accept start txn and save it in Q.
		 */
		return 0;
	}

	fn = ctx->cfn[PDU_STOP_TRANSACTION];
	ocpp_get_utc_now(utc);
	sh->uid = fn(buf, sizeof(buf), sh, meter_val, NULL, utc);

	ui = &ctx->ui;
	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;
	ret = ocpp_send_to_server(&rmsg, K_MSEC(timeout_ms));

	return ret;
}

int ocpp_remote_start_transaction(struct ocpp_info *ctx,
				  struct internal_msg *msg,
				  char *uid)
{
	char *buf = ctx->pdu_buf;
	const char *resp = "Rejected";
	struct ocpp_wamp_rpc_msg rmsg = {0};
	ocpp_msg_fp_t fn;
	int ret;

	ret = k_msgq_put(ctx->msgq, msg, K_MSEC(100));
	if (ret == 0) {
		resp = "Accepted";
	}

	fn = ctx->cfn[PDU_REMOTE_START_TRANSACTION];
	fn(buf, sizeof(ctx->pdu_buf), resp, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	ret = ocpp_send_to_server(&rmsg, K_SECONDS(OCPP_PDU_TIMEOUT));

	return ret;
}

int ocpp_remote_stop_transaction(struct ocpp_info *ctx,
				 struct internal_msg *msg,
				 int idtxn, char *uid)
{
	char *buf = ctx->pdu_buf;
	const char *resp = "Rejected";
	struct ocpp_wamp_rpc_msg rmsg = {0};
	ocpp_msg_fp_t fn;
	sys_snode_t *node;
	struct ocpp_session *sh;
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
		if (ret == 0) {
			resp = "Accepted";
		}
	}

	fn = ctx->cfn[PDU_REMOTE_STOP_TRANSACTION];
	fn(buf, sizeof(ctx->pdu_buf), resp, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	ret = ocpp_send_to_server(&rmsg, K_SECONDS(OCPP_PDU_TIMEOUT));

	return ret;
}

int ocpp_unlock_connector(struct ocpp_info *ctx,
			  struct internal_msg *msg,
			  char *uid)
{
	char *buf = ctx->pdu_buf;
	char *resp = "UnlockFailed";
	struct ocpp_wamp_rpc_msg rmsg = {0};
	ocpp_msg_fp_t fn;
	int ret;

	ret = k_msgq_put(ctx->msgq, msg, K_MSEC(100));
	if (ret == 0) {
		resp = "Unlocked";
	}

	fn = ctx->cfn[PDU_UNLOCK_CONNECTOR];
	fn(buf, sizeof(ctx->pdu_buf), resp, uid);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	ret = ocpp_send_to_server(&rmsg, K_SECONDS(OCPP_PDU_TIMEOUT));

	return ret;
}

int ocpp_meter_values(ocpp_session_handle_t hndl,
		      enum ocpp_meter_measurand mes,
		      char *sval)
{
	struct ocpp_session *sh = (struct ocpp_session *)hndl;
	struct ocpp_info *ctx = sh->ctx;
	struct ocpp_upstream_info *ui = &ctx->ui;
	char *buf = ctx->pdu_buf;
	char utc[CISTR25] = {0};
	struct ocpp_wamp_rpc_msg rmsg = {0};
	ocpp_msg_fp_t fn;
	int ret;

	if (ctx->is_cs_offline) {
		return -EAGAIN;
	}

	fn = ctx->cfn[PDU_METER_VALUES];
	ocpp_get_utc_now(utc);
	sh->uid = fn(buf, sizeof(ctx->pdu_buf), sh, utc, sval,
			mtr_ref_table[mes].smes,
			mtr_ref_table[mes].unit);

	rmsg.ctx = ctx;
	rmsg.msg = buf;
	rmsg.msg_len = strlen(buf);
	rmsg.sndlock = &ui->ws_sndlock;
	rmsg.rspsig = &ui->ws_rspsig;
	ret = ocpp_send_to_server(&rmsg, K_SECONDS(OCPP_PDU_TIMEOUT));

	return ret;
}
