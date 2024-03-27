/*
 * Copyright (c) 2024 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocpp_i.h"
#include <json.h>
#include <zephyr/random/random.h>

/* construct msg to server */
static int frame_rpc_call_req(char *rpcbuf, int len, int pdu, uint32_t ses,
			      struct json_object *pdumsg)
{
	const char *to_send;
	char uid[32] = {0};
	uint32_t rnd;
	struct json_object *rpc;

	rpc = json_object_new_array();
	if (!rpc) {
		return -ENOMEM;
	}

	json_object_array_put_idx(rpc, 0, json_object_new_int(2));

	rnd = sys_rand32_get();
	snprintk(uid, sizeof(uid), "%u-%u-%u", ses, pdu, rnd);
	json_object_array_put_idx(rpc, 1, json_object_new_string(uid));

	json_object_array_put_idx(rpc, 2,
			  json_object_new_string(ocpp_get_pdu_literal(pdu)));

	json_object_array_put_idx(rpc, 3, pdumsg);
	to_send = json_object_to_json_string_ext(rpc, JSON_C_TO_STRING_PLAIN);

	strncpy(rpcbuf, to_send, len);

	json_object_put(rpc);

	return 0;
}

static int frame_rpc_call_res(char *rpcbuf, int len, char *uid,
			      struct json_object *pdumsg)
{
	const char *to_send;
	struct json_object *rpc;

	rpc = json_object_new_array();
	if (!rpc) {
		return -ENOMEM;
	}

	json_object_array_put_idx(rpc, 0, json_object_new_int(3));
	json_object_array_put_idx(rpc, 1, json_object_new_string(uid));

	json_object_array_put_idx(rpc, 2, pdumsg);
	to_send = json_object_to_json_string_ext(rpc, JSON_C_TO_STRING_PLAIN);

	strncpy(rpcbuf, to_send, len);

	json_object_put(rpc);

	return 0;
}

static int frame_authorize_msg(char *buf, int len,
			       struct ocpp_session *ses)
{
	int ret;
	struct json_object *auth;
	struct json_object *tmp;

	auth = json_object_new_object();
	if (!auth) {
		return -ENOMEM;
	}

	tmp = json_object_new_string(ses->idtag);
	if (!tmp) {
		ret = -ENOMEM;
		goto out;
	}

	json_object_object_add(auth, "idTag", tmp);
	ret = frame_rpc_call_req(buf, len, PDU_AUTHORIZE, (uint32_t)ses,
				 auth);
	if (ret) {
		goto out;
	}

	return 0;

out:
	json_object_put(auth);
	return ret;
}

static int frame_heartbeat_msg(char *buf, int len,
			       struct ocpp_session *ses)
{
	int ret;
	struct json_object *hb;

	hb = json_object_new_object();
	if (!hb) {
		return -ENOMEM;
	}

	ret = frame_rpc_call_req(buf, len, PDU_HEARTBEAT,
				 (uint32_t)ses, hb);
	if (ret) {
		json_object_put(hb);
	}

	return ret;
}

static int frame_bootnotif_msg(char *buf, int len,
			       struct ocpp_session *ses,
			       struct ocpp_cp_info *cpi)
{
	int ret = -ENOMEM;
	struct json_object *txn;
	struct json_object *tmp;

	txn = json_object_new_object();
	if (!txn) {
		return -ENOMEM;
	}

	tmp = json_object_new_string(cpi->model);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "chargePointModel", tmp);
	tmp = json_object_new_string(cpi->vendor);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "chargePointVendor", tmp);
	if (cpi->box_sl_no) {
		tmp = json_object_new_string(cpi->box_sl_no);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "chargeBoxSerialNumber", tmp);
	}

	if (cpi->sl_no) {
		tmp = json_object_new_string(cpi->sl_no);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "chargePointSerialNumber", tmp);
	}

	if (cpi->fw_ver) {
		tmp = json_object_new_string(cpi->fw_ver);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "firmwareVersion", tmp);
	}

	if (cpi->iccid) {
		tmp = json_object_new_string(cpi->iccid);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "iccid", tmp);
	}

	if (cpi->imsi) {
		tmp = json_object_new_string(cpi->imsi);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "imsi", tmp);
	}

	if (cpi->meter_sl_no) {
		tmp = json_object_new_string(cpi->meter_sl_no);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "meterSerialNumber", tmp);
	}

	if (cpi->meter_type) {
		tmp = json_object_new_string(cpi->meter_type);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "meterType", tmp);
	}

	ret = frame_rpc_call_req(buf, len, PDU_BOOTNOTIFICATION,
				 (uint32_t)ses, txn);
	if (ret) {
		goto out;
	}

	return 0;

out:
	json_object_put(txn);
	return ret;
}

static int frame_meter_val_msg(char *buf, int len, struct ocpp_session *ses,
			       char *timestamp, char *val, char *measurand,
			       char *unit)
{
	int ret = -ENOMEM;
	struct json_object *mtr;
	struct json_object *tmp;
	struct json_object *persample;
	struct json_object *sampleval;
	struct json_object *smplarr;
	struct json_object *mtrarr;


	mtr = json_object_new_object();
	if (!mtr) {
		return -ENOMEM;
	}

	tmp = json_object_new_int(ses ? ses->idcon : 0);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(mtr, "connectorId", tmp);
	if (ses) {
		tmp = json_object_new_int(ses->idtxn);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(mtr, "transactionId", tmp);
	}

	persample = json_object_new_object();
	if (!persample) {
		goto out;
	}

	tmp = json_object_new_string(timestamp);
	if (!tmp) {
		goto out_persample;
	}

	json_object_object_add(persample, "timestamp", tmp);

	sampleval = json_object_new_object();
	if (!sampleval) {
		goto out_persample;
	}

	tmp = json_object_new_string(measurand);
	if (!tmp) {
		goto out_sampleval;
	}

	json_object_object_add(sampleval, "measurand", tmp);
	tmp = json_object_new_string(val);
	if (!tmp) {
		goto out_sampleval;
	}

	json_object_object_add(sampleval, "value", tmp);
	if (unit) {
		tmp = json_object_new_string(unit);
		if (!tmp) {
			goto out_sampleval;
		}

		json_object_object_add(sampleval, "unit", tmp);
	}
	smplarr = json_object_new_array();
	if (!smplarr) {
		goto out_sampleval;
	}

	mtrarr = json_object_new_array();
	if (!mtrarr) {
		goto out_smplarr;
	}

	json_object_array_put_idx(smplarr, 0, sampleval);
	json_object_object_add(persample, "sampledValue", smplarr);
	json_object_array_put_idx(mtrarr, 0, persample);
	json_object_object_add(mtr, "meterValue", mtrarr);

	ret = frame_rpc_call_req(buf, len, PDU_METER_VALUES,
				 (uint32_t)ses, mtr);
	if (ret) {
		goto out;
	}

	return 0;

out_smplarr:
	json_object_put(smplarr);

out_sampleval:
	json_object_put(sampleval);

out_persample:
	json_object_put(persample);

out:
	json_object_put(mtr);
	return ret;
}

static int frame_stop_txn_msg(char *buf, int len, struct ocpp_session *ses,
			      int Wh, char *reason, char *timestamp)
{
	int ret = -ENOMEM;
	struct json_object *txn;
	struct json_object *tmp;

	txn = json_object_new_object();
	if (!txn) {
		return -ENOMEM;
	}

	tmp = json_object_new_int(ses->idtxn);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "transactionId", tmp);
	tmp = json_object_new_int(Wh);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "meterStop", tmp);
	tmp = json_object_new_string(timestamp);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "timestamp", tmp);
	if (reason) {
		tmp = json_object_new_string(reason);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "reason", tmp);
	}

	if (!ses->idtag[0]) {
		tmp = json_object_new_string(ses->idtag);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "idTag", tmp);
	}


	ret = frame_rpc_call_req(buf, len, PDU_STOP_TRANSACTION,
				 (uint32_t)ses, txn);
	if (ret) {
		goto out;
	}

	return 0;

out:
	json_object_put(txn);
	return ret;
}

static int frame_start_txn_msg(char *buf, int len, struct ocpp_session *ses,
			       int Wh, int reserv_id, char *timestamp)
{
	int ret = -ENOMEM;
	struct json_object *txn;
	struct json_object *tmp;

	txn = json_object_new_object();
	if (!txn) {
		return -ENOMEM;
	}

	tmp = json_object_new_int(ses->idcon);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "connectorId", tmp);
	tmp = json_object_new_string(ses->idtag);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "idTag", tmp);
	tmp = json_object_new_int(Wh);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "meterStart", tmp);
	tmp = json_object_new_string(timestamp);
	if (!tmp) {
		goto out;
	}

	json_object_object_add(txn, "timestamp", tmp);
	if (reserv_id >= 0) {
		tmp = json_object_new_int(reserv_id);
		if (!tmp) {
			goto out;
		}

		json_object_object_add(txn, "reservationId", tmp);
	}

	ret = frame_rpc_call_req(buf, len, PDU_START_TRANSACTION, (uint32_t)ses,
				 txn);
	if (ret) {
		goto out;
	}

	return 0;

out:
	json_object_put(txn);
	return ret;
}

static int frame_getconfig_msg(char *buf, int len, char *key,
			       char *val, bool is_rw, char *uid)
{
	int ret = -ENOMEM;
	struct json_object *cfg;
	struct json_object *keyval;
	struct json_object *arr;
	struct json_object *tmp;

	cfg = json_object_new_object();
	if (!cfg) {
		return -ENOMEM;
	}

	tmp = json_object_new_string(key);
	if (!tmp) {
		goto out;
	}

	if (val) {
		keyval = json_object_new_object();
		if (!keyval) {
			goto out;
		}

		json_object_object_add(keyval, "key", tmp);
		tmp = json_object_new_int(!is_rw);
		if (!tmp) {
			goto out_keyval;
		}

		json_object_object_add(keyval, "readonly", tmp);
		tmp = json_object_new_string(val);
		if (!tmp) {
			goto out_keyval;
		}

		json_object_object_add(keyval, "value", tmp);

		arr = json_object_new_array();
		if (!arr) {
			goto out_keyval;
		}

		json_object_array_put_idx(arr, 0, keyval);
		json_object_object_add(cfg, "configurationKey", arr);
	} else {
		json_object_object_add(cfg, "unknownKey", tmp);
	}

	ret = frame_rpc_call_res(buf, len, uid, cfg);
	if (ret) {
		goto out;
	}

	return 0;

out_keyval:
	json_object_put(keyval);

out:
	json_object_put(cfg);
	return ret;
}

static int frame_status_resp_msg(char *buf, int len, char *res, char *uid)
{
	int ret;
	struct json_object *stat;
	struct json_object *tmp;

	stat = json_object_new_object();
	if (!stat) {
		return -ENOMEM;
	}

	tmp = json_object_new_string(res);
	if (!tmp) {
		ret = -ENOMEM;
		goto out;
	}

	json_object_object_add(stat, "status", tmp);
	ret = frame_rpc_call_res(buf, len, uid, stat);
	if (ret) {
		goto out;
	}

	return 0;

out:
	json_object_put(stat);
	return ret;
}

/* parse msg from server */
int parse_rpc_msg(char *msg, int msglen, char *uid, int uidlen,
		  int *pdu, bool *is_rsp)
{
	int ret = 0;
	int idx = 0, rpc_id;
	const char *str, *spdu, *payload;
	struct json_object *rpc;
	struct json_object *tmp;

	rpc = json_tokener_parse(msg);
	if (!rpc) {
		return -EINVAL;
	}

	tmp = json_object_array_get_idx(rpc, idx++);
	rpc_id = json_object_get_int(tmp);
	tmp = json_object_array_get_idx(rpc, idx++);
	str = json_object_get_string(tmp);
	strncpy(uid, str, uidlen);

	switch (rpc_id + '0') {
	case OCPP_WAMP_RPC_REQ:
		tmp = json_object_array_get_idx(rpc, idx++);
		spdu = json_object_get_string(tmp);
		*pdu = ocpp_find_pdu_from_literal(spdu);
		/* fall through */

	case OCPP_WAMP_RPC_RESP:
		*is_rsp = rpc_id - 2;

		tmp = json_object_array_get_idx(rpc, idx);
		payload = json_object_get_string(tmp);
		strncpy(msg, payload, msglen);
		break;

	case OCPP_WAMP_RPC_ERR:
		/* fall through */

	default:
		ret = -EINVAL;
	}

	json_object_put(rpc);

	return ret;
}

static int parse_idtag_info(struct json_object *root,
			    struct ocpp_idtag_info *idtag_info)
{
	struct json_object *idinfo;
	struct json_object *tmp;
	const char *str;

	if (!json_object_object_get_ex(root, "idTagInfo", &idinfo)) {
		return -EINVAL;
	}

	if (!json_object_object_get_ex(idinfo, "status", &tmp)) {
		return -EINVAL;
	}

	str = json_object_get_string(tmp);
	switch (*str) {
	case 'A':
		idtag_info->auth_status = OCPP_AUTH_ACCEPTED;
		break;

	case 'B':
		idtag_info->auth_status = OCPP_AUTH_BLOCKED;
		break;

	case 'E':
		idtag_info->auth_status = OCPP_AUTH_EXPIRED;
		break;

	case 'I':
		idtag_info->auth_status = OCPP_AUTH_INVALID;
		break;

	case 'C':
		idtag_info->auth_status = OCPP_AUTH_CONCURRENT_TX;
		break;

	default:
		return -EINVAL;
	}

	if (json_object_object_get_ex(idinfo, "parentIdTag", &tmp)) {
		strncpy(idtag_info->p_idtag, json_object_get_string(tmp),
			sizeof(idtag_info->p_idtag));
	}

	if (json_object_object_get_ex(idinfo, "expiryDate", &tmp)) {
		strncpy(idtag_info->exptime, json_object_get_string(tmp),
			sizeof(idtag_info->exptime));
	}

	return 0;
}

static int parse_heartbeat_msg(char *buf, struct timeval *date)
{
	struct json_object *root;
	struct json_object *tmp;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	ret = json_object_object_get_ex(root, "currentTime", &tmp);
	if (!ret) {
		/* todo: convert civil time to epoch and update local time */
		*date;
	}

	json_object_put(root);
	return ret;
}

static int parse_authorize_msg(char *buf, struct ocpp_idtag_info *idtag_info)
{
	struct json_object *root;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	ret = parse_idtag_info(root, idtag_info);
	json_object_put(root);

	return ret;
}

static int parse_bootnotification_msg(char *buf, struct boot_notif *binfo)
{
	struct json_object *root;
	struct json_object *tmp;
	const char *str;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	if (!json_object_object_get_ex(root, "status", &tmp)) {
		ret = -EINVAL;
		goto out;
	}

	str = json_object_get_string(tmp);
	switch (*str) {
	case 'A':	/* accepted */
		binfo->status = BOOT_ACCEPTED;
		break;

	case 'P':	/* pending */
		binfo->status = BOOT_PENDING;
		break;

	case 'R':	/* rejected */
		binfo->status = BOOT_REJECTED;
		break;

	default:
		ret = -EINVAL;
		goto out;
	}

	if (!json_object_object_get_ex(root, "interval", &tmp)) {
		ret = -EINVAL;
		goto out;
	}

	binfo->interval = json_object_get_int(tmp);
	if (!json_object_object_get_ex(root, "currentTime", &tmp)) {
		ret = -EINVAL;
		goto out;
	}

	/* todo: convert civil time to epoch and update local time */
	binfo->date;

out:
	json_object_put(root);
	return ret;
}

static int parse_start_txn_msg(char *buf,
			       int *idtxn,
			       struct ocpp_idtag_info *idtag_info)
{
	struct json_object *root;
	struct json_object *tmp;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	if (!json_object_object_get_ex(root, "transactionId", &tmp)) {
		ret = -EINVAL;
		goto out;
	}

	*idtxn = json_object_get_int(tmp);
	ret = parse_idtag_info(root, idtag_info);

out:
	json_object_put(root);

	return ret;
}

static int parse_getconfig_msg(char *buf, char *key)
{
	struct json_object *root;
	struct json_object *keys;
	struct json_object *cfg;
	const char *str;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	if (!json_object_object_get_ex(root, "key", &keys)) {
		return -EINVAL;
	}

	cfg = json_object_array_get_idx(keys, 0);
	if (!cfg) {
		goto out;
	}

	str = json_object_get_string(cfg);
	if (str) {
		strcpy(key, str);
	}

out:
	json_object_put(root);
	return ret;
}

static int parse_changeconfig_msg(char *buf, char *key, char *val)
{
	struct json_object *root;
	struct json_object *tmp;
	const char *str;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	if (!json_object_object_get_ex(root, "key", &tmp)) {
		return -EINVAL;
	}

	str = json_object_get_string(tmp);
	strncpy(key, str, CISTR50);

	if (!json_object_object_get_ex(root, "value", &tmp)) {
		return -EINVAL;
	}

	str = json_object_get_string(tmp);
	strncpy(val, str, CISTR500);

	json_object_put(root);
	return ret;
}

static int parse_remote_start_txn_msg(char *buf,
				      int *idcon,
				      char *idtag)
{
	struct json_object *root;
	struct json_object *tmp;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	if (!json_object_object_get_ex(root, "idTag", &tmp)) {
		ret = -EINVAL;
		goto out;
	}

	strncpy(idtag, json_object_get_string(tmp), CISTR50);
	if (json_object_object_get_ex(root, "connectorId", &tmp)) {
		*idcon = json_object_get_int(tmp);
	}

out:
	json_object_put(root);

	return ret;
}

static int parse_remote_stop_txn_msg(char *buf, int *idtxn)
{
	struct json_object *root;
	struct json_object *tmp;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	if (!json_object_object_get_ex(root, "transactionId", &tmp)) {
		ret = -EINVAL;
		goto out;
	}

	*idtxn = json_object_get_int(tmp);

out:
	json_object_put(root);

	return ret;
}

static int parse_unlock_connectormsg(char *buf,
				     int *idcon)
{
	struct json_object *root;
	struct json_object *tmp;
	int ret = 0;

	root = json_tokener_parse(buf);
	if (!root) {
		return -EINVAL;
	}

	if (!json_object_object_get_ex(root, "connectorId", &tmp)) {
		ret = -EINVAL;
		goto out;
	}

	*idcon = json_object_get_int(tmp);

out:
	json_object_put(root);

	return ret;
}

static ocpp_msg_fp_t ocpp_json_parser[PDU_MSG_END] = {
	[PDU_BOOTNOTIFICATION]	= (ocpp_msg_fp_t)parse_bootnotification_msg,
	[PDU_AUTHORIZE]		= (ocpp_msg_fp_t)parse_authorize_msg,
	[PDU_START_TRANSACTION]	= (ocpp_msg_fp_t)parse_start_txn_msg,
	[PDU_STOP_TRANSACTION]	= (ocpp_msg_fp_t)parse_authorize_msg,
	[PDU_METER_VALUES]	= NULL,
	[PDU_HEARTBEAT]		= (ocpp_msg_fp_t)parse_heartbeat_msg,
	[PDU_GET_CONFIGURATION]	= (ocpp_msg_fp_t)parse_getconfig_msg,
	[PDU_CHANGE_CONFIGURATION]	= (ocpp_msg_fp_t)parse_changeconfig_msg,
	[PDU_REMOTE_START_TRANSACTION]	= (ocpp_msg_fp_t)parse_remote_start_txn_msg,
	[PDU_REMOTE_STOP_TRANSACTION]	= (ocpp_msg_fp_t)parse_remote_stop_txn_msg,
	[PDU_UNLOCK_CONNECTOR]	= (ocpp_msg_fp_t)parse_unlock_connectormsg,
};

static ocpp_msg_fp_t ocpp_json_frame[PDU_MSG_END] = {
	[PDU_BOOTNOTIFICATION]	=  (ocpp_msg_fp_t)frame_bootnotif_msg,
	[PDU_AUTHORIZE]		=  (ocpp_msg_fp_t)frame_authorize_msg,
	[PDU_START_TRANSACTION]	=  (ocpp_msg_fp_t)frame_start_txn_msg,
	[PDU_STOP_TRANSACTION]	=  (ocpp_msg_fp_t)frame_stop_txn_msg,
	[PDU_METER_VALUES]	=  (ocpp_msg_fp_t)frame_meter_val_msg,
	[PDU_HEARTBEAT]		=  (ocpp_msg_fp_t)frame_heartbeat_msg,
	[PDU_GET_CONFIGURATION]	=  (ocpp_msg_fp_t)frame_getconfig_msg,
	[PDU_CHANGE_CONFIGURATION] (ocpp_msg_fp_t)frame_status_resp_msg,
	[PDU_REMOTE_START_TRANSACTION]	= (ocpp_msg_fp_t)frame_status_resp_msg,
	[PDU_REMOTE_STOP_TRANSACTION]	= (ocpp_msg_fp_t)frame_status_resp_msg,
	[PDU_UNLOCK_CONNECTOR]	= (ocpp_msg_fp_t)frame_status_resp_msg,
};

void ocpp_parser_init(ocpp_msg_fp_t **cfn, ocpp_msg_fp_t **pfn)
{
	*pfn = ocpp_json_parser;
	*cfn = ocpp_json_frame;
}
