/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_cbor
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <zephyr/net/lwm2m.h>
#include "lwm2m_object.h"
#include "lwm2m_rw_cbor.h"
#include "lwm2m_engine.h"
#include "lwm2m_util.h"

LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define CPKT_CBOR_W_SZ(pos, cpkt) ((size_t)(pos) - (size_t)(cpkt)->data - (size_t)(cpkt)->offset)

#define ICTX_CBOR_R_SZ(pos, ictx) ((size_t)pos - (size_t)(ictx)->in_cpkt->data - (ictx)->offset)

static int put_time(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, time_t value)
{
	/* CBOR time output format is unspecified but SenML CBOR uses string format.
	 * Let's stick into the same format with plain CBOR
	 */
	struct tm dt;
	char time_str[sizeof("1970-01-01T00:00:00-00:00")] = { 0 };
	int len;
	int str_sz;
	int tag_sz;
	bool ret;

	if (gmtime_r(&value, &dt) != &dt) {
		LOG_ERR("unable to convert from secs since Epoch to a date/time construct");
		return -EINVAL;
	}

	/* Time construct to a string. Time in UTC, offset to local time not known */
	len = snprintk(time_str, sizeof(time_str),
			"%04d-%02d-%02dT%02d:%02d:%02d-00:00",
			dt.tm_year+1900,
			dt.tm_mon+1,
			dt.tm_mday,
			dt.tm_hour,
			dt.tm_min,
			dt.tm_sec);

	if (len < 0 || len > sizeof(time_str) - 1) {
		LOG_ERR("unable to form a date/time string");
		return -EINVAL;
	}

	ZCBOR_STATE_E(states, 0, CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),  1);

	/* Are tags required? V1.1 leaves this unspecified but some servers require tags */
	ret = zcbor_tag_encode(states, ZCBOR_TAG_TIME_TSTR);

	if (!ret) {
		LOG_ERR("unable to encode date/time string tag");
		return -ENOMEM;
	}

	tag_sz = CPKT_CBOR_W_SZ(states[0].payload, out->out_cpkt);

	out->out_cpkt->offset += tag_sz;

	ret = zcbor_tstr_put_term(states, time_str);
	if (!ret) {
		LOG_ERR("unable to encode date/time string");
		return -ENOMEM;
	}

	str_sz = CPKT_CBOR_W_SZ(states[0].payload, out->out_cpkt);

	out->out_cpkt->offset += str_sz;

	return tag_sz + str_sz;
}

static int put_s64(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, int64_t value)
{
	int payload_len;
	bool ret;

	ZCBOR_STATE_E(states, 0, CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),  1);

	ret = zcbor_int64_encode(states, &value);

	if (ret) {
		payload_len = CPKT_CBOR_W_SZ(states[0].payload, out->out_cpkt);
		out->out_cpkt->offset += payload_len;
	} else {
		LOG_ERR("unable to encode a long long integer value");
		payload_len = -ENOMEM;
	}

	return payload_len;
}

static int put_s32(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, int32_t value)
{
	int payload_len;
	bool ret;

	ZCBOR_STATE_E(states, 0, CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),  1);

	ret = zcbor_int32_encode(states, &value);

	if (ret) {
		payload_len = CPKT_CBOR_W_SZ(states[0].payload, out->out_cpkt);
		out->out_cpkt->offset += payload_len;
	} else {
		LOG_ERR("unable to encode an integer value");
		payload_len = -ENOMEM;
	}

	return payload_len;
}

static int put_s16(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, int16_t value)
{
	return put_s32(out, path, value);
}

static int put_s8(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, int8_t value)
{
	return put_s32(out, path, value);
}

static int put_float(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, double *value)
{
	int payload_len;
	bool ret;

	ZCBOR_STATE_E(states, 0, CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),  1);

	ret = zcbor_float64_encode(states, value);

	if (ret) {
		payload_len = CPKT_CBOR_W_SZ(states[0].payload, out->out_cpkt);
		out->out_cpkt->offset += payload_len;
	} else {
		payload_len = -ENOMEM;
	}

	return payload_len;
}

static int put_string(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
					  char *buf, size_t buflen)
{
	int payload_len;
	bool ret;

	ZCBOR_STATE_E(states, 0, CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),  1);

	ret = zcbor_tstr_encode_ptr(states, buf, buflen);

	if (ret) {
		payload_len = CPKT_CBOR_W_SZ(states[0].payload, out->out_cpkt);
		out->out_cpkt->offset += payload_len;
	} else {
		payload_len = -ENOMEM;
	}

	return payload_len;
}

static int put_opaque(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
					  char *buf, size_t buflen)
{
	int payload_len;
	bool ret;

	ZCBOR_STATE_E(states, 0, CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),  1);

	ret = zcbor_bstr_encode_ptr(states, buf, buflen);

	if (ret) {
		payload_len = CPKT_CBOR_W_SZ(states[0].payload, out->out_cpkt);
		out->out_cpkt->offset += payload_len;
	} else {
		payload_len = -ENOMEM;
	}

	return payload_len;
}

static int put_bool(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, bool value)
{
	int payload_len;
	bool ret;

	ZCBOR_STATE_E(states, 0, CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),  1);

	ret = zcbor_bool_encode(states, &value);

	if (ret) {
		payload_len = CPKT_CBOR_W_SZ(states[0].payload, out->out_cpkt);
		out->out_cpkt->offset += payload_len;
	} else {
		payload_len = -ENOMEM;
	}

	return payload_len;
}

static int put_objlnk(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
			 struct lwm2m_objlnk *value)
{
	char objlnk[sizeof("65535:65535")] = { 0 };

	snprintk(objlnk, sizeof(objlnk), "%" PRIu16 ":%" PRIu16 "", value->obj_id, value->obj_inst);

	return put_string(out, path, objlnk, strlen(objlnk) + 1);
}

static int get_s64(struct lwm2m_input_context *in, int64_t *value)
{
	ZCBOR_STATE_D(states, 0, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	if (!zcbor_int64_decode(states, value)) {
		LOG_WRN("unable to decode a 64-bit integer value");
		return -EBADMSG;
	}

	int len = ICTX_CBOR_R_SZ(states[0].payload, in);

	in->offset += len;

	return len;
}

static int get_s32(struct lwm2m_input_context *in, int32_t *value)
{
	ZCBOR_STATE_D(states, 0, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	if (!zcbor_int32_decode(states, value)) {
		LOG_WRN("unable to decode a 32-bit integer value, err: %d",
				states->constant_state->error);
		return -EBADMSG;
	}

	int len = ICTX_CBOR_R_SZ(states[0].payload, in);

	in->offset += len;

	return len;
}

static int get_float(struct lwm2m_input_context *in, double *value)
{
	ZCBOR_STATE_D(states, 0, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	if (!zcbor_float_decode(states, value)) {
		LOG_ERR("unable to decode a floating-point value");
		return -EBADMSG;
	}

	int len = ICTX_CBOR_R_SZ(states[0].payload, in);

	in->offset += len;

	return len;
}

static int get_string(struct lwm2m_input_context *in, uint8_t *value, size_t buflen)
{
	struct zcbor_string hndl;
	int len;

	ZCBOR_STATE_D(states, 0, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	if (!zcbor_tstr_decode(states, &hndl)) {
		LOG_WRN("unable to decode a string");
		return -EBADMSG;
	}

	len = MIN(buflen-1, hndl.len);

	memcpy(value, hndl.value, len);

	value[len] = '\0';

	len = ICTX_CBOR_R_SZ(states[0].payload, in);

	in->offset += len;

	return len;
}

/* Gets time decoded as a numeric string.
 *
 * return 0 on success, -EBADMSG if decoding fails or -EFAIL if value is invalid
 */
static int get_time_string(struct lwm2m_input_context *in, int64_t *value)
{
	char time_str[sizeof("4294967295")] = { 0 };
	struct zcbor_string hndl = { .value = time_str, .len = sizeof(time_str) - 1 };

	ZCBOR_STATE_D(states, 0, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	if (!zcbor_tstr_decode(states, &hndl)) {
		return -EBADMSG;
	}

	/* TODO: decode date/time string */
	LOG_DBG("decoding a date/time string not supported");

	return -ENOTSUP;
}

/* Gets time decoded as a numerical.
 *
 * return 0 on success, -EBADMSG if decoding fails
 */
static int get_time_numerical(struct lwm2m_input_context *in, int64_t *value)
{
	ZCBOR_STATE_D(states, 0, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	if (!zcbor_int64_decode(states, value)) {
		LOG_WRN("unable to decode seconds since Epoch");
		return -EBADMSG;
	}

	return 0;
}

static int get_time(struct lwm2m_input_context *in, time_t *value)
{
	uint32_t tag;
	int tag_sz = 0;
	int data_len;
	int ret;
	bool success;
	int64_t temp64;

	ZCBOR_STATE_D(states, 0, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	success = zcbor_tag_decode(states, &tag);

	if (success) {
		tag_sz = ICTX_CBOR_R_SZ(states[0].payload, in);
		in->offset += tag_sz;

		switch (tag) {
		case ZCBOR_TAG_TIME_NUM:
			ret = get_time_numerical(in, &temp64);
			if (ret < 0) {
				goto error;
			}
			break;
		case ZCBOR_TAG_TIME_TSTR:
			ret = get_time_string(in, &temp64);
			if (ret < 0) {
				goto error;
			}
			break;
		default:
			LOG_WRN("expected tagged date/time, got tag %" PRIu32 "", tag);
			return -EBADMSG;
		}
	} else { /* Assumption is that tags are optional */

		/* Let's assume numeric string but in case that fails falls go back to numerical */
		ret = get_time_string(in, &temp64);
		if (ret == -EBADMSG) {
			ret = get_time_numerical(in, &temp64);
		}

		if (ret < 0) {
			goto error;
		}
	}

	data_len = ICTX_CBOR_R_SZ(states[0].payload, in);
	in->offset += data_len;
	*value = (time_t)temp64;

	return tag_sz + data_len;

error:
	return ret;
}

static int get_bool(struct lwm2m_input_context *in, bool *value)
{
	ZCBOR_STATE_D(states, 0, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	if (!zcbor_bool_decode(states, value)) {
		LOG_WRN("unable to decode a boolean value");
		return -EBADMSG;
	}

	int len = ICTX_CBOR_R_SZ(states[0].payload, in);

	in->offset += len;

	return len;
}

static int get_opaque(struct lwm2m_input_context *in, uint8_t *value, size_t buflen,
			 struct lwm2m_opaque_context *opaque, bool *last_block)
{
	struct zcbor_string_fragment hndl = { 0 };
	int ret;

	ZCBOR_STATE_D(states, 1, ICTX_BUF_R_PTR(in), ICTX_BUF_R_LEFT_SZ(in),  1);

	/* Get the CBOR header only on first read. */
	if (opaque->remaining == 0) {
		ret = zcbor_bstr_start_decode_fragment(states, &hndl);

		if (!ret) {
			LOG_WRN("unable to decode opaque data header");
			return -EBADMSG;
		}

		opaque->len = hndl.total_len;
		opaque->remaining = hndl.total_len;

		int len = ICTX_CBOR_R_SZ(states[0].payload, in);

		in->offset += len;
	}

	return lwm2m_engine_get_opaque_more(in, value, buflen, opaque, last_block);
}

static int get_objlnk(struct lwm2m_input_context *in, struct lwm2m_objlnk *value)
{
	char objlnk[sizeof("65535:65535")] = { 0 };
	char *end;
	char *idp = objlnk;

	value->obj_id = LWM2M_OBJLNK_MAX_ID;
	value->obj_inst = LWM2M_OBJLNK_MAX_ID;

	int len = get_string(in, objlnk, sizeof(objlnk));


	for (int idx = 0; idx < 2; idx++) {
		errno = 0;

		unsigned long id = strtoul(idp, &end, 10);

		idp = end + 1;

		if ((!id && errno == ERANGE) || id > LWM2M_OBJLNK_MAX_ID) {
			LOG_WRN("decoded id %lu out of range[0..65535]", id);
			return -EBADMSG;
		}

		switch (idx) {
		case 0:
			value->obj_id = id;
			continue;
		case 1:
			value->obj_inst = id;
			continue;
		}
	}

	if (value->obj_inst != LWM2M_OBJLNK_MAX_ID && (value->obj_inst == LWM2M_OBJLNK_MAX_ID)) {
		LOG_WRN("decoded obj inst id without obj id");
		return -EBADMSG;
	}

	return len;
}

const struct lwm2m_writer cbor_writer = {
	.put_s8 = put_s8,
	.put_s16 = put_s16,
	.put_s32 = put_s32,
	.put_s64 = put_s64,
	.put_string = put_string,
	.put_float = put_float,
	.put_time = put_time,
	.put_bool = put_bool,
	.put_opaque = put_opaque,
	.put_objlnk = put_objlnk,
};

const struct lwm2m_reader cbor_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_time = get_time,
	.get_string = get_string,
	.get_float = get_float,
	.get_bool = get_bool,
	.get_opaque = get_opaque,
	.get_objlnk = get_objlnk,
};

int do_read_op_cbor(struct lwm2m_message *msg)
{
	/* Can only return single resource */
	if (msg->path.level < LWM2M_PATH_LEVEL_RESOURCE) {
		return -EPERM;
	} else if (msg->path.level > LWM2M_PATH_LEVEL_RESOURCE_INST) {
		return -ENOENT;
	}

	return lwm2m_perform_read_op(msg, LWM2M_FORMAT_APP_CBOR);
}

int do_write_op_cbor(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret;
	uint8_t created = 0U;

	ret = lwm2m_get_or_create_engine_obj(msg, &obj_inst, &created);
	if (ret < 0) {
		return ret;
	}

	ret = lwm2m_engine_validate_write_access(msg, obj_inst, &obj_field);
	if (ret < 0) {
		return ret;
	}

	ret = lwm2m_engine_get_create_res_inst(&msg->path, &res, &res_inst);
	if (ret < 0) {
		return -ENOENT;
	}

	if (msg->path.level < LWM2M_PATH_LEVEL_RESOURCE) {
		msg->path.level = LWM2M_PATH_LEVEL_RESOURCE;
	}

	return lwm2m_write_handler(obj_inst, res, res_inst, obj_field, msg);
}
