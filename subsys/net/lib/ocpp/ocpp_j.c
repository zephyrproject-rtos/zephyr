/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ocpp_i.h"
#include "ocpp_j.h"
#include <zephyr/data/json.h>
#include <zephyr/random/random.h>

static int extract_string_field(char *out_buf, int outlen, char *token)
{
	char *end;

	if (out_buf == NULL || token == NULL) {
		return -EINVAL;
	}

	strncpy(out_buf, token + 1, outlen - 1);
	end = strchr(out_buf, '"');
	if (end != NULL) {
		*end = '\0';
	}

	return 0;
}

static int extract_payload(char *msg, int msglen)
{
	size_t len;
	char *start = strchr(msg, '{');
	char *end = strrchr(msg, '}');

	if (start == NULL || end == NULL || end < start) {
		return -EINVAL;
	}

	len = end - start + 1;
	if (len >= msglen) {
		return -ENOMEM;
	}

	memmove(msg, start, len);
	msg[len] = '\0';

	return 0;
}

static int frame_rpc_call_req(char *rpcbuf, int len, int pdu,
			      uint32_t ses, char *pdumsg)
{
	int ret;
	char uid[JSON_MSG_BUF_128];
	const char *action;
	uint32_t rnd = sys_rand32_get();

	snprintk(uid, sizeof(uid), "%u-%d-%u", ses, pdu, rnd);

	action = ocpp_get_pdu_literal(pdu);
	if (action == NULL) {
		return -EINVAL;
	}

	/* Encode OCPP Call Request msg: [2,"<UID>","<Action>",<Payload>] */
	ret = snprintk(rpcbuf, len,
		       "[2,\"%s\",\"%s\",%s]",
		       uid, action, pdumsg);

	if (ret < 0 || ret >= len) {
		return -ENOMEM;
	}

	return 0;
}

static int frame_rpc_call_res(char *rpcbuf, int len,
			      char *uid, char *pdumsg)
{
	int ret;

	/* Encode OCPP Call Result msg: [3,"<UID>",<Payload>] */
	ret = snprintk(rpcbuf, len, "[3,\"%s\",%s]", uid, pdumsg);

	if (ret < 0 || ret >= len) {
		return -ENOMEM;
	}

	return 0;
}

static int frame_authorize_msg(char *buf, int len,
			       struct ocpp_session *ses)
{
	int ret;
	char auth_obj[JSON_MSG_BUF_128];

	struct json_obj_descr authorize_descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field_str, "idTag",
					  val1, JSON_TOK_STRING),
	};

	struct json_common_payload_field_str payload = {
		.val1 = ses->idtag,
	};

	ret = json_obj_encode_buf(authorize_descr,
				  ARRAY_SIZE(authorize_descr),
				  &payload,
				  auth_obj,
				  sizeof(auth_obj));
	if (ret < 0) {
		return ret;
	}

	ret = frame_rpc_call_req(buf, len, PDU_AUTHORIZE,
				 (uintptr_t)ses, auth_obj);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int frame_heartbeat_msg(char *buf, int len, struct ocpp_session *ses)
{
	int ret;
	char tmp_buf[8] = "{}";

	ret = frame_rpc_call_req(buf, len, PDU_HEARTBEAT,
				 (uintptr_t)ses, tmp_buf);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int frame_bootnotif_msg(char *buf, int len,
			       struct ocpp_session *ses,
			       struct ocpp_cp_info *cpi)
{
	int ret;
	uint8_t descr_count = BOOTNOTIF_MIN_FIELDS;
	char tmp_buf[JSON_MSG_BUF_512];

	struct json_ocpp_bootnotif_msg msg = {
		.charge_point_model = cpi->model,
		.charge_point_vendor = cpi->vendor,
		.charge_box_serial_number = cpi->box_sl_no ? cpi->box_sl_no : NULL,
		.charge_point_serial_number = cpi->sl_no ? cpi->sl_no : NULL,
		.firmware_version = cpi->fw_ver ? cpi->fw_ver : NULL,
		.iccid = cpi->iccid ? cpi->iccid : NULL,
		.imsi = cpi->imsi ? cpi->imsi : NULL,
		.meter_serial_number = cpi->meter_sl_no ? cpi->meter_sl_no : NULL,
		.meter_type = cpi->meter_type ? cpi->meter_type : NULL,
	};

	struct json_obj_descr bootnotif_descr[BOOTNOTIF_MAX_FIELDS] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "chargePointModel",
					  charge_point_model, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "chargePointVendor",
					  charge_point_vendor, JSON_TOK_STRING),
		};

	if (msg.charge_box_serial_number != NULL) {
		bootnotif_descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "chargeBoxSerialNumber",
					  charge_box_serial_number, JSON_TOK_STRING);
	}

	if (msg.charge_point_serial_number != NULL) {
		bootnotif_descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "chargePointSerialNumber",
					  charge_point_serial_number, JSON_TOK_STRING);
	}

	if (msg.firmware_version != NULL) {
		bootnotif_descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "firmwareVersion",
					  firmware_version, JSON_TOK_STRING);
	}

	if (msg.iccid != NULL) {
		bootnotif_descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "iccid",
					  iccid, JSON_TOK_STRING);
	}

	if (msg.imsi != NULL) {
		bootnotif_descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "imsi",
					  imsi, JSON_TOK_STRING);
	}

	if (msg.meter_serial_number != NULL) {
		bootnotif_descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "meterSerialNumber",
					  meter_serial_number, JSON_TOK_STRING);
	}

	if (msg.meter_type != NULL) {
		bootnotif_descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_bootnotif_msg, "meterType",
					  meter_type, JSON_TOK_STRING);
	}

	ret = json_obj_encode_buf(bootnotif_descr, descr_count,
				  &msg, tmp_buf, sizeof(tmp_buf));
	if (ret < 0) {
		return ret;
	}

	ret = frame_rpc_call_req(buf, len, PDU_BOOTNOTIFICATION,
				 (uintptr_t)ses, tmp_buf);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int frame_meter_val_msg(char *buf, int len, struct ocpp_session *ses, char *timestamp,
			       char *val, char *measurand, char *unit)
{
	int ret;
	char tmp_buf[JSON_MSG_BUF_512];
	uint8_t descr_count = SAMPLED_VALUE_MIN_FIELDS;

	struct json_ocpp_meter_val_msg msg = {
		.connector_id = ses ? ses->idcon : 0,
		.transaction_id = ses ? ses->idtxn : 0,
		.meter_value = {{
			.timestamp = timestamp,
			.sampled_value = {{
				.measurand = measurand,
				.value = val,
				.unit = unit ? unit : NULL,
			}},
			.sampled_value_len = 1,
		}},
		.meter_value_len = 1,
	};

	struct json_obj_descr sampled_value_descr[SAMPLED_VALUE_MAX_FIELDS] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_sample_val, "measurand",
					  measurand, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_sample_val, "value",
					  value, JSON_TOK_STRING),
	};

	if (msg.meter_value[0].sampled_value[0].unit != NULL) {
		sampled_value_descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_sample_val, "unit",
					  unit, JSON_TOK_STRING);
	}

	struct json_obj_descr meter_value_descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_meter_val, "timestamp",
					  timestamp, JSON_TOK_STRING),
		JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct json_ocpp_meter_val, "sampledValue",
					       sampled_value, 1, sampled_value_len,
					       sampled_value_descr,
					       descr_count),
	};

	struct json_obj_descr meter_val_msg_descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_meter_val_msg, "connectorId",
					  connector_id, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_meter_val_msg, "transactionId",
					  transaction_id, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct json_ocpp_meter_val_msg, "meterValue",
					       meter_value, 1, meter_value_len,
					       meter_value_descr,
					       ARRAY_SIZE(meter_value_descr)),
	};

	ret = json_obj_encode_buf(meter_val_msg_descr,
				  ARRAY_SIZE(meter_val_msg_descr),
				  &msg, tmp_buf, sizeof(tmp_buf));
	if (ret < 0) {
		return ret;
	}

	ret = frame_rpc_call_req(buf, len, PDU_METER_VALUES,
				 (uintptr_t)ses, tmp_buf);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int frame_stop_txn_msg(char *buf, int len, struct ocpp_session *ses,
			      int Wh, char *reason, char *timestamp)
{
	int ret;
	char tmp_buf[JSON_MSG_BUF_256];
	uint8_t descr_count = STOP_TXN_MIN_FIELDS;

	struct json_ocpp_stop_txn_msg msg = {
		.transaction_id = ses->idtxn,
		.meter_stop = Wh,
		.timestamp = timestamp,
		.reason = reason ? reason : NULL,
		.id_tag = ses->idtag[0] ? ses->idtag : NULL,
	};

	struct json_obj_descr descr[STOP_TXN_MAX_FIELDS] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_stop_txn_msg, "transactionId",
					  transaction_id, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_stop_txn_msg, "meterStop",
					  meter_stop, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_stop_txn_msg, "timestamp",
					  timestamp, JSON_TOK_STRING),
	};

	if (msg.reason != NULL) {
		descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_stop_txn_msg, "reason",
					  reason, JSON_TOK_STRING);
	}

	if (msg.id_tag != NULL) {
		descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_stop_txn_msg, "idTag",
					  id_tag, JSON_TOK_STRING);
	}

	ret = json_obj_encode_buf(descr, descr_count, &msg,
				  tmp_buf, sizeof(tmp_buf));
	if (ret < 0) {
		return ret;
	}

	ret = frame_rpc_call_req(buf, len, PDU_STOP_TRANSACTION,
				 (uintptr_t)ses, tmp_buf);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int frame_start_txn_msg(char *buf, int len, struct ocpp_session *ses,
			       int Wh, int reserv_id, char *timestamp)
{
	int ret;
	char tmp_buf[JSON_MSG_BUF_256];
	uint8_t descr_count = START_TXN_MIN_FIELDS;

	struct json_ocpp_start_txn_msg msg = {
		.connector_id = ses->idcon,
		.id_tag = ses->idtag,
		.meter_start = Wh,
		.timestamp = timestamp,
		.reservation_id = (reserv_id >= 0) ? reserv_id : -1,
	};

	struct json_obj_descr descr[START_TXN_MAX_FIELDS] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_start_txn_msg, "connectorId",
					  connector_id, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_start_txn_msg, "idTag",
					  id_tag, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_start_txn_msg, "meterStart",
					  meter_start, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_start_txn_msg, "timestamp",
					  timestamp, JSON_TOK_STRING),
	};

	if (msg.reservation_id != -1) {
		descr[descr_count++] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_start_txn_msg, "reservationId",
					  reservation_id, JSON_TOK_NUMBER);
	}

	ret = json_obj_encode_buf(descr, descr_count, &msg,
				  tmp_buf, sizeof(tmp_buf));
	if (ret < 0) {
		return ret;
	}

	ret = frame_rpc_call_req(buf, len, PDU_START_TRANSACTION,
				 (uintptr_t)ses, tmp_buf);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int frame_getconfig_msg(char *buf, int len, char *key, char *val,
			       bool is_rw, char *uid)
{
	int ret;
	char tmp_buf[JSON_MSG_BUF_128];

	struct json_ocpp_getconfig_msg msg = { 0 };

	if (val != NULL) {
		msg.configuration_key[0].key = key;
		msg.configuration_key[0].readonly = !is_rw;
		msg.configuration_key[0].value = val;
		msg.configuration_key_len = 1;
	} else {
		msg.unknown_key = key;
	}

	struct json_obj_descr keyval_descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_key_val, "key",
					  key, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_key_val, "readonly",
					  readonly, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_key_val, "value",
					  value, JSON_TOK_STRING),
	};

	struct json_obj_descr config_descr[GET_CFG_MAX_FIELDS] = {
		JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct json_ocpp_getconfig_msg, "configurationKey",
					       configuration_key, 1, configuration_key_len,
					       keyval_descr, ARRAY_SIZE(keyval_descr))
	};

	if (val == NULL) {
		config_descr[0] = (struct json_obj_descr)
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_ocpp_getconfig_msg, "unknownKey",
					  unknown_key, JSON_TOK_STRING);
	}

	ret = json_obj_encode_buf(config_descr, GET_CFG_MAX_FIELDS, &msg,
				  tmp_buf, sizeof(tmp_buf));
	if (ret < 0) {
		return ret;
	}

	ret = frame_rpc_call_res(buf, len, uid, tmp_buf);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int frame_status_resp_msg(char *buf, int len, char *res, char *uid)
{
	int ret;
	char tmp_buf[JSON_MSG_BUF_128];

	struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field_str, "status",
					  val1, JSON_TOK_STRING),
	};

	struct json_common_payload_field_str msg = {
		.val1 = res,
	};

	ret = json_obj_encode_buf(descr, ARRAY_SIZE(descr), &msg,
				  tmp_buf, sizeof(tmp_buf));
	if (ret < 0) {
		return ret;
	}

	ret = frame_rpc_call_res(buf, len, uid, tmp_buf);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/* parse msg from server */
int parse_rpc_msg(char *msg, int msglen, char *uid, int uidlen,
		  int *pdu, bool *is_rsp)
{
	int ret;
	char local_buf[JSON_MSG_BUF_512];
	char action[JSON_MSG_BUF_128];
	char *token;
	char *saveptr = NULL;
	int rpc_id = -1;

	if (msg == NULL || uid == NULL || pdu == NULL || is_rsp == NULL) {
		return -EINVAL;
	}

	memcpy(local_buf, msg + 1, sizeof(local_buf) - 1);
	local_buf[sizeof(local_buf) - 1] = '\0';

	token = strtok_r(local_buf, ",", &saveptr);
	if (token == NULL) {
		return -EINVAL;
	}

	rpc_id = *token - '0';

	token = strtok_r(NULL, ",", &saveptr);
	if (token == NULL) {
		return -EINVAL;
	}

	ret = extract_string_field(uid, uidlen, token);
	if (ret < 0) {
		return ret;
	}

	switch (rpc_id + '0') {
	case OCPP_WAMP_RPC_REQ:
		token = strtok_r(NULL, ",", &saveptr);
		if (token == NULL) {
			return -EINVAL;
		}

		ret = extract_string_field(action, sizeof(action), token);
		if (ret < 0) {
			return ret;
		}
		*pdu = ocpp_find_pdu_from_literal(action);
		__fallthrough;

	case OCPP_WAMP_RPC_RESP:
		*is_rsp = rpc_id - 2;
		ret = extract_payload(msg, msglen);
		if (ret < 0) {
			return ret;
		}
		break;

	case OCPP_WAMP_RPC_ERR:
		__fallthrough;

	default:
		return -EINVAL;
	}

	return 0;
}

static int parse_idtag_info(char *json, struct ocpp_idtag_info *idtag_info)
{
	int ret;
	char *status;

	struct json_obj_descr inner_descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_idtag_info_root, "status",
					  json_id_tag_info.status, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_idtag_info_root, "parentIdTag",
					  json_id_tag_info.parent_id_tag, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_idtag_info_root, "expiryDate",
					  json_id_tag_info.expiry_date, JSON_TOK_STRING),
	};

	struct json_obj_descr root_descr[] = {
		JSON_OBJ_DESCR_OBJECT_NAMED(struct json_idtag_info_root, "idTagInfo",
					    json_id_tag_info, inner_descr),
	};

	struct json_idtag_info_root parsed = { 0 };

	ret = json_obj_parse(json, strlen(json), root_descr,
			     ARRAY_SIZE(root_descr), &parsed);
	if (ret < 0) {
		return ret;
	}

	status = parsed.json_id_tag_info.status;
	if (status == 0) {
		return -EINVAL;
	}

	switch (*status) {
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

	if (parsed.json_id_tag_info.parent_id_tag != NULL) {
		strncpy(idtag_info->p_idtag, parsed.json_id_tag_info.parent_id_tag,
			sizeof(idtag_info->p_idtag));
	}

	if (parsed.json_id_tag_info.expiry_date != NULL) {
		strncpy(idtag_info->exptime, parsed.json_id_tag_info.expiry_date,
			sizeof(idtag_info->exptime));
	}

	return 0;
}

static int parse_heartbeat_msg(char *json, struct timeval *date)
{
	int ret;

	struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field_str, "currentTime",
					  val1, JSON_TOK_STRING),
	};

	struct json_common_payload_field_str heartbeat = {0};

	ret = json_obj_parse(json, strlen(json), descr,
			     ARRAY_SIZE(descr), &heartbeat);

	/* todo: convert civil time to epoch and update local time */

	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int parse_authorize_msg(char *json, struct ocpp_idtag_info *idtag_info)
{
	int ret;

	ret = parse_idtag_info(json, idtag_info);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int parse_bootnotification_msg(char *json, struct boot_notif *binfo)
{
	int ret;

	struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_bootnotif_payload, "status",
					  status, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_bootnotif_payload, "interval",
					  interval, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_bootnotif_payload, "currentTime",
					  current_time, JSON_TOK_STRING),
	};

	struct json_bootnotif_payload msg = { 0 };

	ret = json_obj_parse(json, strlen(json), descr, ARRAY_SIZE(descr), &msg);
	if (ret < 0) {
		return ret;
	}

	if (msg.status == NULL) {
		return -EINVAL;
	}

	switch (*msg.status) {
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
		return -EINVAL;
	}

	if (msg.interval == 0) {
		return -EINVAL;
	}

	binfo->interval = msg.interval;

	if (msg.current_time == NULL) {
		return -EINVAL;
	}

	/* todo: convert civil time to epoch and update local time */
	(void)binfo->date;

	return 0;
}

static int parse_start_txn_msg(char *json,
			       int *idtxn,
			       struct ocpp_idtag_info *idtag_info)
{
	int ret;

	struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field, "transactionId",
					  val1, JSON_TOK_NUMBER),
	};

	struct json_common_payload_field payload = { 0 };

	ret = json_obj_parse(json, strlen(json), descr, ARRAY_SIZE(descr), &payload);
	if (ret < 0) {
		return ret;
	}

	*idtxn = payload.val1;

	ret = parse_idtag_info(json, idtag_info);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int parse_getconfig_msg(char *json, char *key)
{
	int ret;

	struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_ARRAY_NAMED(struct json_getconfig_payload, "key",
					   key, 1, key_len, JSON_TOK_STRING),
	};

	struct json_getconfig_payload payload = { 0 };

	ret = json_obj_parse(json, strlen(json), descr,
			     ARRAY_SIZE(descr), &payload);
	if (ret < 0) {
		return ret;
	}

	/* key is optional so return success*/
	if (payload.key[0] != NULL) {
		strcpy(key, payload.key[0]);
	}

	return 0;
}

static int parse_changeconfig_msg(char *json, char *key, char *val)
{
	int ret;

	struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field_str, "key",
					  val1, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field_str, "value",
					  val2, JSON_TOK_STRING),
	};

	struct json_common_payload_field_str payload = { 0 };

	ret = json_obj_parse(json, strlen(json), descr, ARRAY_SIZE(descr), &payload);
	if (ret < 0) {
		return ret;
	}

	if (payload.val1 == NULL || payload.val2 == NULL) {
		return -EINVAL;
	}

	strncpy(key, payload.val1, CISTR50);
	strncpy(val, payload.val2, CISTR500);

	return 0;
}

static int parse_remote_start_txn_msg(char *json,
				      int *idcon,
				      char *idtag)
{
	int ret;

	struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field, "connectorId",
					  val1, JSON_TOK_NUMBER),
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field, "idTag",
					  val2, JSON_TOK_STRING),
	};

	struct json_common_payload_field payload = { 0 };

	ret = json_obj_parse(json, strlen(json), descr, ARRAY_SIZE(descr), &payload);
	if (ret < 0) {
		return ret;
	}

	if (payload.val2 == NULL) {
		return -EINVAL;
	}

	strncpy(idtag, payload.val2, CISTR50);
	*idcon = payload.val1;

	return 0;
}

static int parse_remote_stop_txn_msg(char *json, int *idtxn)
{
	int ret;

	struct json_obj_descr descr[] = {
		JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field, "transactionId",
					  val1, JSON_TOK_NUMBER),
	};

	struct json_common_payload_field payload = { 0 };

	ret = json_obj_parse(json, strlen(json), descr, ARRAY_SIZE(descr), &payload);
	if (ret < 0) {
		return ret;
	}

	*idtxn = payload.val1;

	return 0;
}

static int parse_unlock_connectormsg(char *json, int *idcon)
{
	int ret;

	struct json_obj_descr descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_common_payload_field, "connectorId",
				  val1, JSON_TOK_NUMBER),
	};

	struct json_common_payload_field payload = { 0 };

	ret = json_obj_parse(json, strlen(json), descr, ARRAY_SIZE(descr), &payload);
	if (ret < 0) {
		return ret;
	}

	if (payload.val1 == 0) {
		return -EINVAL;
	}

	*idcon = payload.val1;

	return 0;
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
