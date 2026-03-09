/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcp_json.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "mcp_common.h"

LOG_MODULE_REGISTER(mcp_json, CONFIG_MCP_LOG_LEVEL);

/*******************************************************************************
 * Envelope request descriptor (jsonrpc, method, id)
 ******************************************************************************/
struct mcp_json_envelope {
	struct json_obj_token jsonrpc;
	struct json_obj_token method;
	struct json_obj_token id_string;
	int64_t id_integer;
};

static const struct json_obj_descr mcp_envelope_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_envelope, jsonrpc, JSON_TOK_OPAQUE),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_envelope, method, JSON_TOK_OPAQUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct mcp_json_envelope, "id", id_string, JSON_TOK_OPAQUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct mcp_json_envelope, "id", id_integer, JSON_TOK_INT64),
};

/*******************************************************************************
 * Initialize request descriptor
 ******************************************************************************/
struct mcp_json_init_req {
	struct {
		const char *protocol_version;
	} params;
};

static const struct json_obj_descr mcp_init_params_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(__typeof__(((struct mcp_json_init_req *)0)->params),
				  "protocolVersion", protocol_version, JSON_TOK_STRING),
};

static const struct json_obj_descr mcp_init_req_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_init_req, params, mcp_init_params_descr),
};

/*******************************************************************************
 * Ping request descriptor
 ******************************************************************************/
struct mcp_json_ping_req {
	struct {
		const char *protocol_version;
	} params;
};

static const struct json_obj_descr mcp_ping_params_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(__typeof__(((struct mcp_json_ping_req *)0)->params),
				  "protocolVersion", protocol_version, JSON_TOK_STRING),
};

static const struct json_obj_descr mcp_ping_req_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_ping_req, params, mcp_ping_params_descr),
};

/*******************************************************************************
 * Tools list request descriptor
 ******************************************************************************/
struct mcp_json_tools_list_req {
	struct {
		const char *protocol_version;
	} params;
};

static const struct json_obj_descr mcp_tools_list_params_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(__typeof__(((struct mcp_json_tools_list_req *)0)->params),
				  "protocolVersion", protocol_version, JSON_TOK_STRING),
};

static const struct json_obj_descr mcp_tools_list_req_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_tools_list_req, params, mcp_tools_list_params_descr),
};

/*******************************************************************************
 * Tools call request descriptor
 ******************************************************************************/
struct mcp_json_tools_call_req {
	struct {
		const char *protocol_version;
		const char *name;
	} params;
};

static const struct json_obj_descr mcp_tools_call_params_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(__typeof__(((struct mcp_json_tools_call_req *)0)->params),
				  "protocolVersion", protocol_version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(__typeof__(((struct mcp_json_tools_call_req *)0)->params), name,
			    JSON_TOK_STRING),
};

static const struct json_obj_descr mcp_tools_call_req_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_tools_call_req, params, mcp_tools_call_params_descr),
};

/*******************************************************************************
 * Notification/cancelled request descriptor (client -> server)
 ******************************************************************************/
struct mcp_json_cancelled_notif_in {
	struct {
		struct json_obj_token request_id;
		const char *reason;
	} params;
};

static const struct json_obj_descr mcp_cancelled_in_params_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(__typeof__(((struct mcp_json_cancelled_notif_in *)0)->params),
				  "requestId", request_id, JSON_TOK_OPAQUE),
	JSON_OBJ_DESCR_PRIM(__typeof__(((struct mcp_json_cancelled_notif_in *)0)->params), reason,
			    JSON_TOK_STRING),
};

static const struct json_obj_descr mcp_cancelled_notif_in_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_cancelled_notif_in, params,
			      mcp_cancelled_in_params_descr),
};

/*******************************************************************************
 * Notification/cancelled request descriptor (server -> client)
 ******************************************************************************/
struct mcp_json_cancelled_notif_out {
	const char *jsonrpc;
	const char *method;
	struct {
		const char *request_id;
		const char *reason;
	} params;
};

static const struct json_obj_descr mcp_cancelled_out_params_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(__typeof__(((struct mcp_json_cancelled_notif_out *)0)->params),
				  "requestId", request_id, JSON_TOK_ENCODED_OBJ),
	JSON_OBJ_DESCR_PRIM(__typeof__(((struct mcp_json_cancelled_notif_out *)0)->params), reason,
			    JSON_TOK_STRING),
};

static const struct json_obj_descr mcp_json_cancelled_notif_out_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_cancelled_notif_out, jsonrpc, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_cancelled_notif_out, method, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_cancelled_notif_out, params,
			      mcp_cancelled_out_params_descr),
};

/*******************************************************************************
 * Initialize resposne descriptor
 ******************************************************************************/
struct mcp_json_server_info {
	const char *name;
	const char *version;
};

struct mcp_json_init_result_inner {
	const char *protocol_version;
	struct mcp_json_server_info server_info;
	const char *capabilities;
};

struct mcp_json_init_result {
	const char *jsonrpc;
	const char *id;
	struct mcp_json_init_result_inner result;
};

static const struct json_obj_descr mcp_json_server_info_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_server_info, name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_server_info, version, JSON_TOK_STRING),
};

static const struct json_obj_descr mcp_json_init_result_inner_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct mcp_json_init_result_inner, "protocolVersion",
				  protocol_version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct mcp_json_init_result_inner, "serverInfo", server_info,
				    mcp_json_server_info_descr),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_init_result_inner, capabilities, JSON_TOK_ENCODED_OBJ),
};

static const struct json_obj_descr mcp_json_init_result_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_init_result, jsonrpc, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_init_result, id, JSON_TOK_ENCODED_OBJ),
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_init_result, result,
			      mcp_json_init_result_inner_descr),
};

/*******************************************************************************
 * Ping resposne descriptor
 ******************************************************************************/
struct mcp_json_ping_result {
	const char *jsonrpc;
	const char *id;
	struct {
		bool dummy; /* Empty object */
	} result;
};

static const struct json_obj_descr mcp_json_ping_result_inner_descr[] = {
	/* Empty descriptor for empty object */
};

static const struct json_obj_descr mcp_json_ping_result_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_ping_result, jsonrpc, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_ping_result, id, JSON_TOK_ENCODED_OBJ),
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_ping_result, result,
			      mcp_json_ping_result_inner_descr),
};

/*******************************************************************************
 * Tools/list resposne descriptor
 ******************************************************************************/
struct mcp_json_tools_list_result {
	const char *jsonrpc;
	const char *id;
	struct {
		const char *tools; /* This should be the raw JSON array */
	} result;
};

static const struct json_obj_descr mcp_json_tools_list_result_inner_descr[] = {
	JSON_OBJ_DESCR_PRIM(__typeof__(((struct mcp_json_tools_list_result *)0)->result), tools,
			    JSON_TOK_ENCODED_OBJ),
};

static const struct json_obj_descr mcp_json_tools_list_result_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_tools_list_result, jsonrpc, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_tools_list_result, id, JSON_TOK_ENCODED_OBJ),
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_tools_list_result, result,
			      mcp_json_tools_list_result_inner_descr),
};

/*******************************************************************************
 * Tools/call resposne descriptor
 ******************************************************************************/
struct mcp_json_content_item {
	const char *type;
	const char *text;
};

struct mcp_json_tools_call_result {
	const char *jsonrpc;
	const char *id;
	struct {
		struct mcp_json_content_item content[MCP_MAX_CONTENT_ITEMS];
		size_t content_len;
		bool is_error;
	} result;
};

static const struct json_obj_descr mcp_json_content_item_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_content_item, type, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_content_item, text, JSON_TOK_STRING),
};

static const struct json_obj_descr mcp_json_tools_call_result_inner_descr[] = {
	JSON_OBJ_DESCR_OBJ_ARRAY(__typeof__(((struct mcp_json_tools_call_result *)0)->result),
				 content, MCP_MAX_CONTENT_ITEMS, content_len,
				 mcp_json_content_item_descr,
				 ARRAY_SIZE(mcp_json_content_item_descr)),
	JSON_OBJ_DESCR_PRIM_NAMED(__typeof__(((struct mcp_json_tools_call_result *)0)->result),
				  "isError", is_error, JSON_TOK_TRUE),
};

static const struct json_obj_descr mcp_json_tools_call_result_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_tools_call_result, jsonrpc, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_tools_call_result, id, JSON_TOK_ENCODED_OBJ),
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_tools_call_result, result,
			      mcp_json_tools_call_result_inner_descr),
};

/*******************************************************************************
 * Error resposne descriptor
 ******************************************************************************/
struct mcp_json_error {
	const char *jsonrpc;
	const char *id;
	struct {
		int32_t code;
		const char *message;
		const char *data;
	} error;
};

static const struct json_obj_descr mcp_json_error_inner_descr[] = {
	JSON_OBJ_DESCR_PRIM(__typeof__(((struct mcp_json_error *)0)->error), code, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(__typeof__(((struct mcp_json_error *)0)->error), message,
			    JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(__typeof__(((struct mcp_json_error *)0)->error), data,
			    JSON_TOK_ENCODED_OBJ),
};

static const struct json_obj_descr mcp_json_error_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct mcp_json_error, jsonrpc, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct mcp_json_error, id, JSON_TOK_ENCODED_OBJ),
	JSON_OBJ_DESCR_OBJECT(struct mcp_json_error, error, mcp_json_error_inner_descr),
};

/*******************************************************************************
 * Helpers
 ******************************************************************************/
/* Helper to extract string from json_obj_token */
static void extract_token_string(char *dst, size_t dst_sz, const struct json_obj_token *token)
{
	size_t len;

	if (!dst || dst_sz == 0 || !token || !token->start || token->length == 0) {
		if (dst && dst_sz > 0) {
			dst[0] = '\0';
		}
		return;
	}

	len = token->length;

	if (len >= dst_sz) {
		len = dst_sz - 1;
	}

	memcpy(dst, token->start, len);
	dst[len] = '\0';
}

static enum mcp_method mcp_method_from_string(char *m, size_t len)
{
	/* Create temporary null-terminated string for comparison */
	char method_buf[64];

	if (!m || len == 0) {
		return MCP_METHOD_UNKNOWN;
	}

	if (len >= sizeof(method_buf)) {
		return MCP_METHOD_UNKNOWN;
	}

	memcpy(method_buf, m, len);
	method_buf[len] = '\0';

	if (strcmp(method_buf, "initialize") == 0) {
		return MCP_METHOD_INITIALIZE;
	} else if (strcmp(method_buf, "ping") == 0) {
		return MCP_METHOD_PING;
	} else if (strcmp(method_buf, "tools/list") == 0) {
		return MCP_METHOD_TOOLS_LIST;
	} else if (strcmp(method_buf, "tools/call") == 0) {
		return MCP_METHOD_TOOLS_CALL;
	} else if (strcmp(method_buf, "notifications/initialized") == 0) {
		return MCP_METHOD_NOTIF_INITIALIZED;
	} else if (strcmp(method_buf, "notifications/cancelled") == 0) {
		return MCP_METHOD_NOTIF_CANCELLED;
	}

	LOG_DBG("Unsupported method: %s", method_buf);
	return MCP_METHOD_UNKNOWN;
}

static int parse_initialize_request(char *buf, size_t len, struct mcp_message *msg)
{
	struct mcp_json_init_req tmp = {0};
	struct mcp_params_initialize *p;
	int ret =
		json_obj_parse(buf, len, mcp_init_req_descr, ARRAY_SIZE(mcp_init_req_descr), &tmp);

	if (ret < 0) {
		LOG_DBG("Failed to parse initialize request: %d", ret);
		return -EINVAL;
	}

	p = &msg->req.initialize;

	memset(p, 0, sizeof(*p));

	if (tmp.params.protocol_version) {
		mcp_safe_strcpy(msg->protocol_version, sizeof(msg->protocol_version),
				tmp.params.protocol_version);
	}

	return 0;
}

/* ping request: we ignore params for now */
static int parse_ping_request(char *buf, size_t len, struct mcp_message *msg)
{
	struct mcp_json_ping_req tmp = {0};
	struct mcp_params_ping *p;
	int ret =
		json_obj_parse(buf, len, mcp_ping_req_descr, ARRAY_SIZE(mcp_ping_req_descr), &tmp);

	if (ret < 0) {
		LOG_DBG("Failed to parse ping request: %d", ret);
		return -EINVAL;
	}

	p = &msg->req.ping;

	memset(p, 0, sizeof(*p));

	if (tmp.params.protocol_version) {
		mcp_safe_strcpy(msg->protocol_version, sizeof(msg->protocol_version),
				tmp.params.protocol_version);
	}

	return 0;
}

/* tools/list request: no params for now */
static int parse_tools_list_request(char *buf, size_t len, struct mcp_message *msg)
{
	struct mcp_json_tools_list_req tmp = {0};
	struct mcp_params_tools_list *p;
	int ret = json_obj_parse(buf, len, mcp_tools_list_req_descr,
				 ARRAY_SIZE(mcp_tools_list_req_descr), &tmp);

	if (ret < 0) {
		LOG_DBG("Failed to parse tools_list request: %d", ret);
		return -EINVAL;
	}

	p = &msg->req.tools_list;

	memset(p, 0, sizeof(*p));

	if (tmp.params.protocol_version) {
		mcp_safe_strcpy(msg->protocol_version, sizeof(msg->protocol_version),
				tmp.params.protocol_version);
	}

	return 0;
}

static bool extract_arguments_json(char *buf, size_t len, char *dst, size_t dst_sz)
{
	const char *args_key;
	const char *args_start;
	const char *args_end;
	size_t args_len;
	int brace_count = 0;

	if (!buf || !dst || dst_sz == 0) {
		return false;
	}

	args_key = strstr(buf, "\"arguments\"");
	if (!args_key) {
		return false;
	}

	args_start = strchr(args_key, ':');
	if (!args_start) {
		return false;
	}
	args_start++;

	while (*args_start == ' ' || *args_start == '\t' || *args_start == '\n' ||
	       *args_start == '\r') {
		args_start++;
	}

	if (*args_start != '{') {
		return false;
	}

	args_end = args_start;
	brace_count = 0;

	do {
		if (*args_end == '{') {
			brace_count++;
		} else if (*args_end == '}') {
			brace_count--;
		} else {
			;
		}
		args_end++;

		if (args_end >= buf + len) {
			return false;
		}
	} while (brace_count > 0);

	args_len = args_end - args_start;
	if (args_len >= dst_sz) {
		return false;
	}

	memcpy(dst, args_start, args_len);
	dst[args_len] = '\0';

	return true;
}

static int parse_tools_call_request(char *buf, size_t len, struct mcp_message *msg)
{
	struct mcp_json_tools_call_req tmp = {0};
	struct mcp_params_tools_call *p = &msg->req.tools_call;
	int ret;

	memset(p, 0, sizeof(*p));

	/* Extract arguments BEFORE json_obj_parse modifies the buffer */
	if (extract_arguments_json(buf, len, p->arguments_json, sizeof(p->arguments_json))) {
		p->has_arguments = true;
	} else {
		p->has_arguments = false;
	}

	/* Now parse the rest (this will modify the buffer) */
	ret = json_obj_parse(buf, len, mcp_tools_call_req_descr,
			     ARRAY_SIZE(mcp_tools_call_req_descr), &tmp);
	if (ret < 0) {
		LOG_ERR("Failed to parse tools_call_req: %d", ret);
		return -EINVAL;
	}

	if (tmp.params.protocol_version) {
		mcp_safe_strcpy(msg->protocol_version, sizeof(msg->protocol_version),
				tmp.params.protocol_version);
	}

	if (tmp.params.name) {
		mcp_safe_strcpy(p->name, sizeof(p->name), tmp.params.name);
	}

	return 0;
}

static int parse_notif_cancelled(char *buf, size_t len, struct mcp_message *msg)
{
	struct mcp_json_cancelled_notif_in tmp = {0};
	struct mcp_params_notif_cancelled *p;

	int ret = json_obj_parse(buf, len, mcp_cancelled_notif_in_descr,
				 ARRAY_SIZE(mcp_cancelled_notif_in_descr), &tmp);

	if (ret < 0) {
		return -EINVAL;
	}

	p = &msg->notif.cancelled;

	memset(p, 0, sizeof(*p));

	/* Extract requestId as string (with quotes preserved if string) */
	extract_token_string(p->request_id.string, sizeof(p->request_id.string),
			     &tmp.params.request_id);

	if (tmp.params.reason) {
		mcp_safe_strcpy(p->reason, sizeof(p->reason), tmp.params.reason);
		p->has_reason = true;
	}

	return 0;
}

/*******************************************************************************
 * Public parser API
 ******************************************************************************/
int mcp_json_parse_message(char *buf, size_t len, struct mcp_message *out)
{
	struct mcp_json_envelope env = {0};
	char jsonrpc_buf[16];
	int ret;
	bool has_id_string;
	bool has_id_integer;
	bool has_method;

	if (!buf || !out || len == 0) {
		return -EINVAL;
	}

	memset(out, 0, sizeof(*out));
	out->kind = MCP_MSG_INVALID;
	out->method = MCP_METHOD_UNKNOWN;

	/* Step 1: parse the envelope (jsonrpc, method, id, protocolVersion) */
	ret = json_obj_parse(buf, len, mcp_envelope_descr, ARRAY_SIZE(mcp_envelope_descr), &env);

	if (ret < 0) {
		LOG_DBG("Failed to parse envelope: %d", ret);
		return -EINVAL;
	}

	/* Check jsonrpc version */
	extract_token_string(jsonrpc_buf, sizeof(jsonrpc_buf), &env.jsonrpc);
	if (strcmp(jsonrpc_buf, JSON_RPC_VERSION) != 0) {
		LOG_DBG("Invalid jsonrpc version: %s", jsonrpc_buf);
		return -EINVAL;
	}

	/* Determine presence and type of id based on bitmask */
	has_id_string = (ret & BIT(2)) != 0;
	has_id_integer = (ret & BIT(3)) != 0;

	LOG_DBG("Parse result: jsonrpc=%d method=%d id_string=%d id_integer=%d",
		(ret & BIT(0)) != 0, (ret & BIT(1)) != 0, has_id_string, has_id_integer);

	/* Store ID as string */
	if (has_id_integer) {
		/* Integer ID: store without quotes*/
		snprintk(out->id.string, sizeof(out->id.string), "%" PRId64, env.id_integer);
	} else if (has_id_string) {
		/* String ID: store with quotes */
		char temp[MCP_MAX_ID_LEN - 2];

		extract_token_string(temp, sizeof(temp), &env.id_string);
		snprintk(out->id.string, sizeof(out->id.string), "\"%s\"", temp);
	} else {
		/* No ID */
		out->id.string[0] = '\0';
	}

	/* Determine method */
	has_method = (ret & BIT(1)) != 0;

	if (has_method) {
		out->method = mcp_method_from_string(env.method.start, env.method.length);
	} else {
		out->method = MCP_METHOD_UNKNOWN;
	}

	/*
	 * - Request: method with id (params optional).
	 * - Notification: method without id (params optional).
	 */
	if (has_method && (has_id_integer || has_id_string)) {
		out->kind = MCP_MSG_REQUEST;
	} else if (has_method && !has_id_integer && !has_id_string) {
		out->kind = MCP_MSG_NOTIFICATION;
	} else {
		return -EINVAL;
	}

	if (out->kind == MCP_MSG_REQUEST) {
		switch (out->method) {
		case MCP_METHOD_INITIALIZE:
			ret = parse_initialize_request(buf, len, out);
			break;
		case MCP_METHOD_PING:
			ret = parse_ping_request(buf, len, out);
			break;
		case MCP_METHOD_TOOLS_LIST:
			ret = parse_tools_list_request(buf, len, out);
			break;
		case MCP_METHOD_TOOLS_CALL:
			ret = parse_tools_call_request(buf, len, out);
			break;
		default:
			/* Unknown method */
			ret = 0;
			break;
		}
	} else if (out->kind == MCP_MSG_NOTIFICATION) {
		switch (out->method) {
		case MCP_METHOD_NOTIF_INITIALIZED:
			/* Nothing to parse */
			ret = 0;
			break;
		case MCP_METHOD_NOTIF_CANCELLED:
			ret = parse_notif_cancelled(buf, len, out);
			break;
		default:
			/* Unknown method */
			ret = 0;
			break;
		}
	} else {
		ret = -EINVAL;
	}

	return ret;
}

/*******************************************************************************
 * Serializers
 ******************************************************************************/
int mcp_json_serialize_initialize_result(char *out, size_t out_len, const struct mcp_request_id *id,
					 const struct mcp_result_initialize *res)
{
	const char *id_str;
	int ret;
	struct mcp_json_init_result json_res;

	if (!out || !res || out_len == 0) {
		return -EINVAL;
	}

	id_str = (id && id->string[0] != '\0') ? id->string : "null";

	json_res = (struct mcp_json_init_result){
		.jsonrpc = JSON_RPC_VERSION,
		.id = id_str,
		.result = {
			.protocol_version = res->protocol_version,
			.server_info = {
				.name = res->server_name,
				.version = res->server_version,
			},
			.capabilities = (res->has_capabilities && res->capabilities_json[0] != '\0')
					? res->capabilities_json : "null",
		},
	};

	ret = json_obj_encode_buf(mcp_json_init_result_descr,
				  ARRAY_SIZE(mcp_json_init_result_descr), &json_res, out, out_len);

	return (ret == 0) ? strlen(out) : ret;
}

int mcp_json_serialize_ping_result(char *out, size_t out_len, const struct mcp_request_id *id,
				   const struct mcp_result_ping *res)
{
	const char *id_str;
	int ret;
	(void)res; /* currently unused; we return empty {} */
	struct mcp_json_ping_result json_res;

	if (!out || out_len == 0) {
		return -EINVAL;
	}

	id_str = (id && id->string[0] != '\0') ? id->string : "null";

	json_res = (struct mcp_json_ping_result){
		.jsonrpc = JSON_RPC_VERSION,
		.id = id_str,
		.result = {
			.dummy = false
		},
	};

	ret = json_obj_encode_buf(mcp_json_ping_result_descr,
				  ARRAY_SIZE(mcp_json_ping_result_descr), &json_res, out, out_len);

	return (ret == 0) ? strlen(out) : ret;
}

int mcp_json_serialize_tools_list_result(char *out, size_t out_len, const struct mcp_request_id *id,
					 const struct mcp_result_tools_list *res)
{
	const char *id_str;
	const char *tools_json;
	int ret;
	struct mcp_json_tools_list_result json_res;

	if (!out || !res || out_len == 0) {
		return -EINVAL;
	}

	id_str = (id && id->string[0] != '\0') ? id->string : "null";

	/* tools_json should already be a complete JSON array: [{"name":"foo",...},...] */
	tools_json = (res->tools_json[0] != '\0') ? res->tools_json : "[]";

	json_res = (struct mcp_json_tools_list_result){
		.jsonrpc = JSON_RPC_VERSION,
		.id = id_str,
		.result = {
				.tools = tools_json, /* Insert raw JSON array */
			},
	};

	ret = json_obj_encode_buf(mcp_json_tools_list_result_descr,
				  ARRAY_SIZE(mcp_json_tools_list_result_descr), &json_res, out,
				  out_len);

	return (ret == 0) ? strlen(out) : ret;
}

int mcp_json_serialize_tools_call_result(char *out, size_t out_len, const struct mcp_request_id *id,
					 const struct mcp_result_tools_call *res)
{
	const char *id_str;
	int ret;
	struct mcp_json_tools_call_result json_res;

	if (!out || !res || out_len == 0) {
		return -EINVAL;
	}

	id_str = (id && id->string[0] != '\0') ? id->string : "null";

	json_res = (struct mcp_json_tools_call_result){
		.jsonrpc = JSON_RPC_VERSION,
		.id = id_str,
		.result = {
				.content_len = res->content.count,
				.is_error = res->is_error,
			},
	};

	/* Copy content items */
	for (size_t i = 0; i < res->content.count && i < MCP_MAX_CONTENT_ITEMS; i++) {
		json_res.result.content[i].type = "text";
		json_res.result.content[i].text = res->content.items[i].text;
	}

	ret = json_obj_encode_buf(mcp_json_tools_call_result_descr,
				  ARRAY_SIZE(mcp_json_tools_call_result_descr), &json_res, out,
				  out_len);

	return (ret == 0) ? strlen(out) : ret;
}

int mcp_json_serialize_error(char *out, size_t out_len, const struct mcp_request_id *id,
			     const struct mcp_error *err)
{
	const char *id_str;
	struct mcp_json_error json_err;
	int ret;

	if (!out || !err || out_len == 0) {
		return -EINVAL;
	}

	id_str = (id && id->string[0] != '\0') ? id->string : "null";

	json_err = (struct mcp_json_error){
		.jsonrpc = JSON_RPC_VERSION,
		.id = id_str,
		.error = {
				.code = err->code,
				.message = err->message,
				.data = (err->has_data && err->data_json[0] != '\0')
					? err->data_json : "null",
			},
	};

	ret = json_obj_encode_buf(mcp_json_error_descr, ARRAY_SIZE(mcp_json_error_descr), &json_err,
				  out, out_len);

	return (ret == 0) ? strlen(out) : ret;
}

int mcp_json_serialize_cancel_notification(char *out, size_t out_len,
					   const struct mcp_params_notif_cancelled *params)
{
	const char *request_id_str;
	const char *reason_str;
	struct mcp_json_cancelled_notif_out json_notif;
	int ret;

	if (!out || !params || out_len == 0) {
		return -EINVAL;
	}

	request_id_str =
		(params->request_id.string[0] != '\0') ? params->request_id.string : "null";
	reason_str = (params->has_reason && params->reason[0] != '\0') ? params->reason : "null";

	json_notif = (struct mcp_json_cancelled_notif_out){
		.jsonrpc = JSON_RPC_VERSION,
		.method = "notifications/cancelled",
		.params = {
				.request_id = request_id_str,
				.reason = reason_str,
			},
	};

	ret = json_obj_encode_buf(mcp_json_cancelled_notif_out_descr,
				  ARRAY_SIZE(mcp_json_cancelled_notif_out_descr), &json_notif, out,
				  out_len);

	return (ret == 0) ? strlen(out) : ret;
}
