/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Uses some original concepts by:
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Joel Hoglund <joel@sics.se>
 */

#define LOG_MODULE_NAME net_lwm2m_message_handling
#define LOG_LEVEL	CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/net/http_parser_url.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include <fcntl.h>
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#include <zephyr/net/tls_credentials.h>
#endif
#if defined(CONFIG_DNS_RESOLVER)
#include <zephyr/net/dns_resolve.h>
#endif

#include "lwm2m_engine.h"
#include "lwm2m_object.h"
#include "lwm2m_obj_access_control.h"
#include "lwm2m_rw_link_format.h"
#include "lwm2m_rw_oma_tlv.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_util.h"
#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
#include "lwm2m_rw_senml_json.h"
#endif
#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
#include "lwm2m_rw_json.h"
#endif
#ifdef CONFIG_LWM2M_RW_CBOR_SUPPORT
#include "lwm2m_rw_cbor.h"
#endif
#ifdef CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT
#include "lwm2m_rw_senml_cbor.h"
#endif
#ifdef CONFIG_LWM2M_RD_CLIENT_SUPPORT
#include "lwm2m_rd_client.h"
#endif
/* TODO: figure out what's correct value */
#define TIMEOUT_BLOCKWISE_TRANSFER_MS (MSEC_PER_SEC * 30)

#define LWM2M_DP_CLIENT_URI "dp"
/* Resources */

/* Shared set of in-flight LwM2M messages */
static struct lwm2m_message messages[CONFIG_LWM2M_ENGINE_MAX_MESSAGES];

/* External resources */
sys_slist_t *lwm2m_engine_obj_list(void);

sys_slist_t *lwm2m_engine_obj_inst_list(void);

struct lwm2m_block_context *lwm2m_block1_context(void);
/* block-wise transfer functions */

enum coap_block_size lwm2m_default_block_size(void)
{
	switch (CONFIG_LWM2M_COAP_BLOCK_SIZE) {
	case 16:
		return COAP_BLOCK_16;
	case 32:
		return COAP_BLOCK_32;
	case 64:
		return COAP_BLOCK_64;
	case 128:
		return COAP_BLOCK_128;
	case 256:
		return COAP_BLOCK_256;
	case 512:
		return COAP_BLOCK_512;
	case 1024:
		return COAP_BLOCK_1024;
	}

	return COAP_BLOCK_256;
}
static int init_block_ctx(const uint8_t *token, uint8_t tkl, struct lwm2m_block_context **ctx)
{
	int i;
	int64_t timestamp;

	*ctx = NULL;
	timestamp = k_uptime_get();
	for (i = 0; i < NUM_BLOCK1_CONTEXT; i++) {
		if (lwm2m_block1_context()[i].tkl == 0U) {
			*ctx = &lwm2m_block1_context()[i];
			break;
		}

		if (timestamp - lwm2m_block1_context()[i].timestamp >
		    TIMEOUT_BLOCKWISE_TRANSFER_MS) {
			*ctx = &lwm2m_block1_context()[i];
			/* TODO: notify application for block
			 * transfer timeout
			 */
			break;
		}
	}

	if (*ctx == NULL) {
		LOG_ERR("Cannot find free block context");
		return -ENOMEM;
	}

	(*ctx)->tkl = tkl;
	memcpy((*ctx)->token, token, tkl);
	coap_block_transfer_init(&(*ctx)->ctx, lwm2m_default_block_size(), 0);
	(*ctx)->timestamp = timestamp;
	(*ctx)->expected = 0;
	(*ctx)->last_block = false;
	memset(&(*ctx)->opaque, 0, sizeof((*ctx)->opaque));

	return 0;
}

static int get_block_ctx(const uint8_t *token, uint8_t tkl, struct lwm2m_block_context **ctx)
{
	int i;

	*ctx = NULL;

	for (i = 0; i < NUM_BLOCK1_CONTEXT; i++) {
		if (lwm2m_block1_context()[i].tkl == tkl &&
		    memcmp(token, lwm2m_block1_context()[i].token, tkl) == 0) {
			*ctx = &lwm2m_block1_context()[i];
			/* refresh timestamp */
			(*ctx)->timestamp = k_uptime_get();
			break;
		}
	}

	if (*ctx == NULL) {
		return -ENOENT;
	}

	return 0;
}

static void free_block_ctx(struct lwm2m_block_context *ctx)
{
	if (ctx == NULL) {
		return;
	}

	ctx->tkl = 0U;
}

int lwm2m_engine_context_close(struct lwm2m_ctx *client_ctx)
{
	int sock_fd = client_ctx->sock_fd;
	struct lwm2m_message *msg;
	sys_snode_t *obs_node;
	struct observe_node *obs;
	size_t i;

	/* Remove observes for this context */
	while (!sys_slist_is_empty(&client_ctx->observer)) {
		obs_node = sys_slist_get_not_empty(&client_ctx->observer);
		obs = SYS_SLIST_CONTAINER(obs_node, obs, node);
		remove_observer_from_list(client_ctx, NULL, obs);
	}

	for (i = 0, msg = messages; i < ARRAY_SIZE(messages); i++, msg++) {
		if (msg->ctx == client_ctx) {
			lwm2m_reset_message(msg, true);
		}
	}

	coap_pendings_clear(client_ctx->pendings, ARRAY_SIZE(client_ctx->pendings));
	coap_replies_clear(client_ctx->replies, ARRAY_SIZE(client_ctx->replies));

#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
	client_ctx->connection_suspended = false;
	client_ctx->buffer_client_messages = true;
#endif
	lwm2m_socket_del(client_ctx);
	client_ctx->sock_fd = -1;
	if (sock_fd >= 0) {
		return close(sock_fd);
	} else {
		return 0;
	}
}

void lwm2m_engine_context_init(struct lwm2m_ctx *client_ctx)
{
	sys_slist_init(&client_ctx->pending_sends);
	sys_slist_init(&client_ctx->observer);
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
	client_ctx->buffer_client_messages = true;
	client_ctx->connection_suspended = false;
	sys_slist_init(&client_ctx->queued_messages);
#endif
}
/* utility functions */

static int coap_options_to_path(struct coap_option *opt, int options_count,
				struct lwm2m_obj_path *path)
{
	uint16_t len,
		*id[4] = {&path->obj_id, &path->obj_inst_id, &path->res_id, &path->res_inst_id};

	path->level = options_count;

	for (int i = 0; i < options_count; i++) {
		*id[i] = lwm2m_atou16(opt[i].value, opt[i].len, &len);
		if (len == 0U || opt[i].len != len) {
			path->level = i;
			break;
		}
	}

	return options_count == path->level ? 0 : -EINVAL;
}

struct lwm2m_message *find_msg(struct coap_pending *pending, struct coap_reply *reply)
{
	size_t i;
	struct lwm2m_message *msg;

	if (!pending && !reply) {
		return NULL;
	}

	msg = lwm2m_get_ongoing_rd_msg();
	if (msg) {
		if (pending != NULL && msg->pending == pending) {
			return msg;
		}

		if (reply != NULL && msg->reply == reply) {
			return msg;
		}
	}

	for (i = 0; i < CONFIG_LWM2M_ENGINE_MAX_MESSAGES; i++) {
		if (pending != NULL && messages[i].ctx && messages[i].pending == pending) {
			return &messages[i];
		}

		if (reply != NULL && messages[i].ctx && messages[i].reply == reply) {
			return &messages[i];
		}
	}

	return NULL;
}

struct lwm2m_message *lwm2m_get_message(struct lwm2m_ctx *client_ctx)
{
	size_t i;

	for (i = 0; i < CONFIG_LWM2M_ENGINE_MAX_MESSAGES; i++) {
		if (!messages[i].ctx) {
			messages[i].ctx = client_ctx;
			return &messages[i];
		}
	}

	return NULL;
}

void lm2m_message_clear_allocations(struct lwm2m_message *msg)
{
	if (msg->pending) {
		coap_pending_clear(msg->pending);
		msg->pending = NULL;
	}

	if (msg->reply) {
		/* make sure we want to clear the reply */
		coap_reply_clear(msg->reply);
		msg->reply = NULL;
	}
}

void lwm2m_reset_message(struct lwm2m_message *msg, bool release)
{
	if (!msg) {
		return;
	}

	lm2m_message_clear_allocations(msg);

	if (msg->ctx) {
		sys_slist_find_and_remove(&msg->ctx->pending_sends, &msg->node);
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
		sys_slist_find_and_remove(&msg->ctx->queued_messages, &msg->node);
#endif
	}

	if (release) {
		(void)memset(msg, 0, sizeof(*msg));
	} else {
		msg->message_timeout_cb = NULL;
		(void)memset(&msg->cpkt, 0, sizeof(msg->cpkt));
	}
}

int lwm2m_init_message(struct lwm2m_message *msg)
{
	uint8_t tokenlen = 0U;
	uint8_t *token = NULL;
	int r = 0;

	if (!msg || !msg->ctx) {
		LOG_ERR("LwM2M message is invalid.");
		return -EINVAL;
	}

	if (msg->tkl == LWM2M_MSG_TOKEN_GENERATE_NEW) {
		tokenlen = 8U;
		token = coap_next_token();
	} else if (msg->token && msg->tkl != 0) {
		tokenlen = msg->tkl;
		token = msg->token;
	}

	lm2m_message_clear_allocations(msg);

	r = coap_packet_init(&msg->cpkt, msg->msg_data, sizeof(msg->msg_data), COAP_VERSION_1,
			     msg->type, tokenlen, token, msg->code, msg->mid);
	if (r < 0) {
		LOG_ERR("coap packet init error (err:%d)", r);
		goto cleanup;
	}

	/* only TYPE_CON messages need pending tracking / reply handling */
	if (msg->type != COAP_TYPE_CON) {
		return 0;
	}

	msg->pending = coap_pending_next_unused(msg->ctx->pendings, ARRAY_SIZE(msg->ctx->pendings));
	if (!msg->pending) {
		LOG_ERR("Unable to find a free pending to track "
			"retransmissions.");
		r = -ENOMEM;
		goto cleanup;
	}

	r = coap_pending_init(msg->pending, &msg->cpkt, &msg->ctx->remote_addr,
			      CONFIG_COAP_MAX_RETRANSMIT);
	if (r < 0) {
		LOG_ERR("Unable to initialize a pending "
			"retransmission (err:%d).",
			r);
		goto cleanup;
	}

	if (msg->reply_cb) {
		msg->reply =
			coap_reply_next_unused(msg->ctx->replies, ARRAY_SIZE(msg->ctx->replies));
		if (!msg->reply) {
			LOG_ERR("No resources for waiting for replies.");
			r = -ENOMEM;
			goto cleanup;
		}

		coap_reply_clear(msg->reply);
		coap_reply_init(msg->reply, &msg->cpkt);
		msg->reply->reply = msg->reply_cb;
	}

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);

	return r;
}

int lwm2m_send_message_async(struct lwm2m_message *msg)
{
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED) && defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
	int ret;

	ret = lwm2m_rd_client_connection_resume(msg->ctx);
	if (ret) {
		lwm2m_reset_message(msg, true);
		return ret;
	}
#endif
	sys_slist_append(&msg->ctx->pending_sends, &msg->node);

	if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT) &&
	    IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_ENABLED)) {
		engine_update_tx_time();
	}
	return 0;
}

int lwm2m_information_interface_send(struct lwm2m_message *msg)
{
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED) && defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
	int ret;

	ret = lwm2m_rd_client_connection_resume(msg->ctx);
	if (ret) {
		lwm2m_reset_message(msg, true);
		return ret;
	}

	if (msg->ctx->buffer_client_messages) {
		sys_slist_append(&msg->ctx->queued_messages, &msg->node);
		return 0;
	}
#endif
	sys_slist_append(&msg->ctx->pending_sends, &msg->node);

	if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT) &&
	    IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_ENABLED)) {
		engine_update_tx_time();
	}
	return 0;
}

int lwm2m_send_message(struct lwm2m_message *msg)
{
	int rc;

	if (!msg || !msg->ctx) {
		LOG_ERR("LwM2M message is invalid.");
		return -EINVAL;
	}

	if (msg->type == COAP_TYPE_CON) {
		coap_pending_cycle(msg->pending);
	}

	rc = send(msg->ctx->sock_fd, msg->cpkt.data, msg->cpkt.offset, 0);

	if (rc < 0) {
		LOG_ERR("Failed to send packet, err %d", errno);
		if (msg->type != COAP_TYPE_CON) {
			lwm2m_reset_message(msg, true);
		}

		return -errno;
	}

	if (msg->type != COAP_TYPE_CON) {
		lwm2m_reset_message(msg, true);
	}

	return 0;
}
int lwm2m_send_empty_ack(struct lwm2m_ctx *client_ctx, uint16_t mid)
{
	struct lwm2m_message *msg;
	int ret;

	msg = lwm2m_get_message(client_ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = COAP_TYPE_ACK;
	msg->code = COAP_CODE_EMPTY;
	msg->mid = mid;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	lwm2m_send_message_async(msg);

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

void lwm2m_acknowledge(struct lwm2m_ctx *client_ctx)
{
	struct lwm2m_message *request;

	if (client_ctx == NULL || client_ctx->processed_req == NULL) {
		return;
	}

	request = (struct lwm2m_message *)client_ctx->processed_req;

	if (request->acknowledged) {
		return;
	}

	if (lwm2m_send_empty_ack(client_ctx, request->mid) < 0) {
		return;
	}

	request->acknowledged = true;
}

int lwm2m_register_payload_handler(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj *obj;
	struct lwm2m_engine_obj_inst *obj_inst;
	int ret;
	sys_slist_t *engine_obj_list = lwm2m_engine_obj_list();
	sys_slist_t *engine_obj_inst_list = lwm2m_engine_obj_inst_list();

	ret = engine_put_begin(&msg->out, NULL);
	if (ret < 0) {
		return ret;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(engine_obj_list, obj, node) {
		/* Security obj MUST NOT be part of registration message */
		if (obj->obj_id == LWM2M_OBJECT_SECURITY_ID) {
			continue;
		}

		/* Only report <OBJ_ID> when no instance available or it's
		 * needed to report object version.
		 */
		if (obj->instance_count == 0U || lwm2m_engine_shall_report_obj_version(obj)) {
			struct lwm2m_obj_path path = {
				.obj_id = obj->obj_id,
				.level = LWM2M_PATH_LEVEL_OBJECT,
			};

			ret = engine_put_corelink(&msg->out, &path);
			if (ret < 0) {
				return ret;
			}

			if (obj->instance_count == 0U) {
				continue;
			}
		}

		SYS_SLIST_FOR_EACH_CONTAINER(engine_obj_inst_list, obj_inst, node) {
			if (obj_inst->obj->obj_id == obj->obj_id) {
				struct lwm2m_obj_path path = {
					.obj_id = obj_inst->obj->obj_id,
					.obj_inst_id = obj_inst->obj_inst_id,
					.level = LWM2M_PATH_LEVEL_OBJECT_INST,
				};

				ret = engine_put_corelink(&msg->out, &path);
				if (ret < 0) {
					return ret;
				}
			}
		}
	}

	return 0;
}

static int select_writer(struct lwm2m_output_context *out, uint16_t accept)
{
	switch (accept) {

	case LWM2M_FORMAT_APP_LINK_FORMAT:
		out->writer = &link_format_writer;
		break;

	case LWM2M_FORMAT_PLAIN_TEXT:
	case LWM2M_FORMAT_OMA_PLAIN_TEXT:
		out->writer = &plain_text_writer;
		break;

#ifdef CONFIG_LWM2M_RW_OMA_TLV_SUPPORT
	case LWM2M_FORMAT_OMA_TLV:
	case LWM2M_FORMAT_OMA_OLD_TLV:
		out->writer = &oma_tlv_writer;
		break;
#endif

#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
	case LWM2M_FORMAT_OMA_JSON:
	case LWM2M_FORMAT_OMA_OLD_JSON:
		out->writer = &json_writer;
		break;
#endif

#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		out->writer = &senml_json_writer;
		break;
#endif

#ifdef CONFIG_LWM2M_RW_CBOR_SUPPORT
	case LWM2M_FORMAT_APP_CBOR:
		out->writer = &cbor_writer;
		break;
#endif

#ifdef CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT
	case LWM2M_FORMAT_APP_SENML_CBOR:
		out->writer = &senml_cbor_writer;
		break;
#endif

	default:
		LOG_WRN("Unknown content type %u", accept);
		return -ENOMSG;
	}

	return 0;
}

static int select_reader(struct lwm2m_input_context *in, uint16_t format)
{
	switch (format) {

	case LWM2M_FORMAT_APP_OCTET_STREAM:
	case LWM2M_FORMAT_PLAIN_TEXT:
	case LWM2M_FORMAT_OMA_PLAIN_TEXT:
		in->reader = &plain_text_reader;
		break;

#ifdef CONFIG_LWM2M_RW_OMA_TLV_SUPPORT
	case LWM2M_FORMAT_OMA_TLV:
	case LWM2M_FORMAT_OMA_OLD_TLV:
		in->reader = &oma_tlv_reader;
		break;
#endif

#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
	case LWM2M_FORMAT_OMA_JSON:
	case LWM2M_FORMAT_OMA_OLD_JSON:
		in->reader = &json_reader;
		break;
#endif

#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		in->reader = &senml_json_reader;
		break;
#endif

#ifdef CONFIG_LWM2M_RW_CBOR_SUPPORT
	case LWM2M_FORMAT_APP_CBOR:
		in->reader = &cbor_reader;
		break;
#endif

#ifdef CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT
	case LWM2M_FORMAT_APP_SENML_CBOR:
		in->reader = &senml_cbor_reader;
		break;
#endif
	default:
		LOG_WRN("Unknown content type %u", format);
		return -ENOMSG;
	}

	return 0;
}

/* generic data handlers */
static int lwm2m_write_handler_opaque(struct lwm2m_engine_obj_inst *obj_inst,
				      struct lwm2m_engine_res *res,
				      struct lwm2m_engine_res_inst *res_inst,
				      struct lwm2m_message *msg, void *data_ptr, size_t data_len)
{
	int len = 1;
	bool last_pkt_block = false;
	int ret = 0;
	bool last_block = true;
	struct lwm2m_opaque_context opaque_ctx = {0};
	void *write_buf;
	size_t write_buf_len;

	if (msg->in.block_ctx != NULL) {
		last_block = msg->in.block_ctx->last_block;

		/* Restore the opaque context from the block context, if used. */
		opaque_ctx = msg->in.block_ctx->opaque;
	}

#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	/* In case validation callback is present, write data to the temporary
	 * buffer first, for validation. Otherwise, write to the data buffer
	 * directly.
	 */
	if (res->validate_cb) {
		write_buf = msg->ctx->validate_buf;
		write_buf_len = sizeof(msg->ctx->validate_buf);
	} else
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */
	{
		write_buf = data_ptr;
		write_buf_len = data_len;
	}

	while (!last_pkt_block && len > 0) {
		len = engine_get_opaque(&msg->in, write_buf, MIN(data_len, write_buf_len),
					&opaque_ctx, &last_pkt_block);
		if (len <= 0) {
			return len;
		}

#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
		if (res->validate_cb) {
			ret = res->validate_cb(obj_inst->obj_inst_id, res->res_id,
					       res_inst->res_inst_id, write_buf, len,
					       last_pkt_block && last_block, opaque_ctx.len);
			if (ret < 0) {
				/* -EEXIST will generate Bad Request LWM2M response. */
				return -EEXIST;
			}

			memcpy(data_ptr, write_buf, len);
		}
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */

		if (res->post_write_cb) {
			ret = res->post_write_cb(obj_inst->obj_inst_id, res->res_id,
						 res_inst->res_inst_id, data_ptr, len,
						 last_pkt_block && last_block, opaque_ctx.len);
			if (ret < 0) {
				return ret;
			}
		}
	}

	if (msg->in.block_ctx != NULL) {
		msg->in.block_ctx->opaque = opaque_ctx;
	}

	return opaque_ctx.len;
}
/* This function is exposed for the content format writers */
int lwm2m_write_handler(struct lwm2m_engine_obj_inst *obj_inst, struct lwm2m_engine_res *res,
			struct lwm2m_engine_res_inst *res_inst,
			struct lwm2m_engine_obj_field *obj_field, struct lwm2m_message *msg)
{
	void *data_ptr = NULL;
	size_t data_len = 0;
	size_t len = 0;
	size_t total_size = 0;
	int64_t temp64 = 0;
	int32_t temp32 = 0;
	int ret = 0;
	bool last_block = true;
	void *write_buf;
	size_t write_buf_len;

	if (!obj_inst || !res || !res_inst || !obj_field || !msg) {
		return -EINVAL;
	}

	if (LWM2M_HAS_RES_FLAG(res_inst, LWM2M_RES_DATA_FLAG_RO)) {
		return -EACCES;
	}

	/* setup initial data elements */
	data_ptr = res_inst->data_ptr;
	data_len = res_inst->max_data_len;

	/* allow user to override data elements via callback */
	if (res->pre_write_cb) {
		data_ptr = res->pre_write_cb(obj_inst->obj_inst_id, res->res_id,
					     res_inst->res_inst_id, &data_len);
	}

	if (res->post_write_cb
#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	    || res->validate_cb
#endif
	) {
		if (msg->in.block_ctx != NULL) {
			/* Get block_ctx for total_size (might be zero) */
			total_size = msg->in.block_ctx->ctx.total_size;
			LOG_DBG("BLOCK1: total:%zu current:%zu"
				" last:%u",
				msg->in.block_ctx->ctx.total_size, msg->in.block_ctx->ctx.current,
				msg->in.block_ctx->last_block);
		}
	}

#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
	/* In case validation callback is present, write data to the temporary
	 * buffer first, for validation. Otherwise, write to the data buffer
	 * directly.
	 */
	if (res->validate_cb) {
		write_buf = msg->ctx->validate_buf;
		write_buf_len = sizeof(msg->ctx->validate_buf);
	} else
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */
	{
		write_buf = data_ptr;
		write_buf_len = data_len;
	}

	if (data_ptr && data_len > 0) {
		switch (obj_field->data_type) {

		case LWM2M_RES_TYPE_OPAQUE:
			ret = lwm2m_write_handler_opaque(obj_inst, res, res_inst, msg, data_ptr,
							 data_len);
			len = ret;
			break;

		case LWM2M_RES_TYPE_STRING:
			ret = engine_get_string(&msg->in, write_buf, write_buf_len);
			if (ret < 0) {
				break;
			}

			len = strlen((char *)write_buf);
			break;

		case LWM2M_RES_TYPE_TIME:
			ret = engine_get_time(&msg->in, &temp64);
			if (ret < 0) {
				break;
			}
			*(uint32_t *)write_buf = temp64;
			len = 4;
			break;

		case LWM2M_RES_TYPE_U32:
			ret = engine_get_s64(&msg->in, &temp64);
			if (ret < 0) {
				break;
			}

			*(uint32_t *)write_buf = temp64;
			len = 4;
			break;

		case LWM2M_RES_TYPE_U16:
			ret = engine_get_s32(&msg->in, &temp32);
			if (ret < 0) {
				break;
			}

			*(uint16_t *)write_buf = temp32;
			len = 2;
			break;

		case LWM2M_RES_TYPE_U8:
			ret = engine_get_s32(&msg->in, &temp32);
			if (ret < 0) {
				break;
			}

			*(uint8_t *)write_buf = temp32;
			len = 1;
			break;

		case LWM2M_RES_TYPE_S64:
			ret = engine_get_s64(&msg->in, (int64_t *)write_buf);
			len = 8;
			break;

		case LWM2M_RES_TYPE_S32:
			ret = engine_get_s32(&msg->in, (int32_t *)write_buf);
			len = 4;
			break;

		case LWM2M_RES_TYPE_S16:
			ret = engine_get_s32(&msg->in, &temp32);
			if (ret < 0) {
				break;
			}

			*(int16_t *)write_buf = temp32;
			len = 2;
			break;

		case LWM2M_RES_TYPE_S8:
			ret = engine_get_s32(&msg->in, &temp32);
			if (ret < 0) {
				break;
			}

			*(int8_t *)write_buf = temp32;
			len = 1;
			break;

		case LWM2M_RES_TYPE_BOOL:
			ret = engine_get_bool(&msg->in, (bool *)write_buf);
			len = 1;
			break;

		case LWM2M_RES_TYPE_FLOAT:
			ret = engine_get_float(&msg->in, (double *)write_buf);
			len = sizeof(double);
			break;

		case LWM2M_RES_TYPE_OBJLNK:
			ret = engine_get_objlnk(&msg->in, (struct lwm2m_objlnk *)write_buf);
			len = sizeof(struct lwm2m_objlnk);
			break;

		default:
			LOG_ERR("unknown obj data_type %d", obj_field->data_type);
			return -EINVAL;
		}

		if (ret < 0) {
			return ret;
		}
	} else {
		return -ENOENT;
	}

	if (obj_field->data_type != LWM2M_RES_TYPE_OPAQUE) {
#if CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0
		if (res->validate_cb) {
			ret = res->validate_cb(obj_inst->obj_inst_id, res->res_id,
					       res_inst->res_inst_id, write_buf, len, last_block,
					       total_size);
			if (ret < 0) {
				/* -EEXIST will generate Bad Request LWM2M response. */
				return -EEXIST;
			}

			if (len > data_len) {
				LOG_ERR("Received data won't fit into provided "
					"bufffer");
				return -ENOMEM;
			}

			if (obj_field->data_type == LWM2M_RES_TYPE_STRING) {
				strncpy(data_ptr, write_buf, data_len);
			} else {
				memcpy(data_ptr, write_buf, len);
			}
		}
#endif /* CONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE > 0 */

		if (res->post_write_cb) {
			ret = res->post_write_cb(obj_inst->obj_inst_id, res->res_id,
						 res_inst->res_inst_id, data_ptr, len, last_block,
						 total_size);
		}
	}

	res_inst->data_len = len;

	if (LWM2M_HAS_PERM(obj_field, LWM2M_PERM_R)) {
		lwm2m_notify_observer_path(&msg->path);
	}

	return ret;
}

static int lwm2m_read_handler(struct lwm2m_engine_obj_inst *obj_inst, struct lwm2m_engine_res *res,
			      struct lwm2m_engine_obj_field *obj_field, struct lwm2m_message *msg)
{
	int i, loop_max = 1, found_values = 0;
	uint16_t res_inst_id_tmp = 0U;
	void *data_ptr = NULL;
	size_t data_len = 0;
	int ret = 0;

	if (!obj_inst || !res || !obj_field || !msg) {
		return -EINVAL;
	}

	loop_max = res->res_inst_count;
	if (res->multi_res_inst) {
		/* search for valid resource instances */
		for (i = 0; i < loop_max; i++) {
			if (res->res_instances[i].res_inst_id != RES_INSTANCE_NOT_CREATED) {
				found_values = 1;
				break;
			}
		}

		if (!found_values) {
			return -ENOENT;
		}

		ret = engine_put_begin_ri(&msg->out, &msg->path);
		if (ret < 0) {
			return ret;
		}

		res_inst_id_tmp = msg->path.res_inst_id;
	}

	for (i = 0; i < loop_max; i++) {
		if (res->res_instances[i].res_inst_id == RES_INSTANCE_NOT_CREATED) {
			continue;
		}

		if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1) &&
		    msg->path.level == LWM2M_PATH_LEVEL_RESOURCE_INST &&
		    msg->path.res_inst_id != res->res_instances[i].res_inst_id) {
			continue;
		}

		if (res->res_inst_count > 1) {
			msg->path.res_inst_id = res->res_instances[i].res_inst_id;
		}

		/* setup initial data elements */
		data_ptr = res->res_instances[i].data_ptr;
		data_len = res->res_instances[i].data_len;

		/* allow user to override data elements via callback */
		if (res->read_cb) {
			data_ptr = res->read_cb(obj_inst->obj_inst_id, res->res_id,
						res->res_instances[i].res_inst_id, &data_len);
		}

		if (!data_ptr || data_len == 0) {
			return -ENOENT;
		}

		switch (obj_field->data_type) {

		case LWM2M_RES_TYPE_OPAQUE:
			ret = engine_put_opaque(&msg->out, &msg->path, (uint8_t *)data_ptr,
						data_len);
			break;

		case LWM2M_RES_TYPE_STRING:
			ret = engine_put_string(&msg->out, &msg->path, (uint8_t *)data_ptr,
						strlen((uint8_t *)data_ptr));
			break;

		case LWM2M_RES_TYPE_U32:
			ret = engine_put_s64(&msg->out, &msg->path,
					     (int64_t) *(uint32_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_U16:
			ret = engine_put_s32(&msg->out, &msg->path,
					     (int32_t) *(uint16_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_U8:
			ret = engine_put_s16(&msg->out, &msg->path,
					     (int16_t) *(uint8_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_S64:
			ret = engine_put_s64(&msg->out, &msg->path, *(int64_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_S32:
			ret = engine_put_s32(&msg->out, &msg->path, *(int32_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_S16:
			ret = engine_put_s16(&msg->out, &msg->path, *(int16_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_S8:
			ret = engine_put_s8(&msg->out, &msg->path, *(int8_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_TIME:
			ret = engine_put_time(&msg->out, &msg->path,
					      (int64_t) *(uint32_t *)data_ptr);
			break;

		case LWM2M_RES_TYPE_BOOL:
			ret = engine_put_bool(&msg->out, &msg->path, *(bool *)data_ptr);
			break;

		case LWM2M_RES_TYPE_FLOAT:
			ret = engine_put_float(&msg->out, &msg->path, (double *)data_ptr);
			break;

		case LWM2M_RES_TYPE_OBJLNK:
			ret = engine_put_objlnk(&msg->out, &msg->path,
						(struct lwm2m_objlnk *)data_ptr);
			break;

		default:
			LOG_ERR("unknown obj data_type %d", obj_field->data_type);
			return -EINVAL;
		}

		/* Validate that we really read some data */
		if (ret < 0) {
			LOG_ERR("Read operation fail");
			return -ENOMEM;
		}
	}

	if (res->multi_res_inst) {
		ret = engine_put_end_ri(&msg->out, &msg->path);
		if (ret < 0) {
			return ret;
		}

		msg->path.res_inst_id = res_inst_id_tmp;
	}

	return 0;
}

static int lwm2m_delete_handler(struct lwm2m_message *msg)
{
	int ret;

	if (!msg) {
		return -EINVAL;
	}

	/* Device management interface is not allowed to delete Security and
	 * Device objects instances.
	 */
	if (msg->path.obj_id == LWM2M_OBJECT_SECURITY_ID ||
	    msg->path.obj_id == LWM2M_OBJECT_DEVICE_ID) {
		return -EPERM;
	}

	ret = lwm2m_delete_obj_inst(msg->path.obj_id, msg->path.obj_inst_id);
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
	if (!msg->ctx->bootstrap_mode) {
		engine_trigger_update(true);
	}
#endif

	return 0;
}

static int do_read_op(struct lwm2m_message *msg, uint16_t content_format)
{
	switch (content_format) {

	case LWM2M_FORMAT_APP_OCTET_STREAM:
	case LWM2M_FORMAT_PLAIN_TEXT:
	case LWM2M_FORMAT_OMA_PLAIN_TEXT:
		return do_read_op_plain_text(msg, content_format);

#if defined(CONFIG_LWM2M_RW_OMA_TLV_SUPPORT)
	case LWM2M_FORMAT_OMA_TLV:
	case LWM2M_FORMAT_OMA_OLD_TLV:
		return do_read_op_tlv(msg, content_format);
#endif

#if defined(CONFIG_LWM2M_RW_JSON_SUPPORT)
	case LWM2M_FORMAT_OMA_JSON:
	case LWM2M_FORMAT_OMA_OLD_JSON:
		return do_read_op_json(msg, content_format);
#endif

#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		return do_read_op_senml_json(msg);
#endif

#if defined(CONFIG_LWM2M_RW_CBOR_SUPPORT)
	case LWM2M_FORMAT_APP_CBOR:
		return do_read_op_cbor(msg);
#endif

#if defined(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)
	case LWM2M_FORMAT_APP_SENML_CBOR:
		return do_read_op_senml_cbor(msg);
#endif

	default:
		LOG_ERR("Unsupported content-format: %u", content_format);
		return -ENOMSG;
	}
}

static int do_composite_read_op(struct lwm2m_message *msg, uint16_t content_format)
{
	switch (content_format) {

#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		return do_composite_read_op_senml_json(msg);
#endif

#if defined(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)
	case LWM2M_FORMAT_APP_SENML_CBOR:
		return do_composite_read_op_senml_cbor(msg);
#endif

	default:
		LOG_ERR("Unsupported content-format: %u", content_format);
		return -ENOMSG;
	}
}

static int lwm2m_perform_read_object_instance(struct lwm2m_message *msg,
					      struct lwm2m_engine_obj_inst *obj_inst,
					      uint8_t *num_read)
{
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_obj_field *obj_field;
	int ret = 0;

	while (obj_inst) {
		if (!obj_inst->resources || obj_inst->resource_count == 0U) {
			goto move_forward;
		}

		/* update the obj_inst_id as we move through the instances */
		msg->path.obj_inst_id = obj_inst->obj_inst_id;

		ret = engine_put_begin_oi(&msg->out, &msg->path);
		if (ret < 0) {
			return ret;
		}

		for (int index = 0; index < obj_inst->resource_count; index++) {
			if (msg->path.level > LWM2M_PATH_LEVEL_OBJECT_INST &&
			    msg->path.res_id != obj_inst->resources[index].res_id) {
				continue;
			}

			res = &obj_inst->resources[index];
			msg->path.res_id = res->res_id;
			obj_field = lwm2m_get_engine_obj_field(obj_inst->obj, res->res_id);
			if (!obj_field) {
				ret = -ENOENT;
			} else if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_R)) {
				ret = -EPERM;
			} else {
				/* start resource formatting */
				ret = engine_put_begin_r(&msg->out, &msg->path);
				if (ret < 0) {
					return ret;
				}

				/* perform read operation on this resource */
				ret = lwm2m_read_handler(obj_inst, res, obj_field, msg);
				if (ret == -ENOMEM) {
					/* No point continuing if there's no
					 * memory left in a message.
					 */
					return ret;
				} else if (ret < 0) {
					/* ignore errors unless single read */
					if (msg->path.level > LWM2M_PATH_LEVEL_OBJECT_INST &&
					    !LWM2M_HAS_PERM(obj_field, BIT(LWM2M_FLAG_OPTIONAL))) {
						LOG_ERR("READ OP: %d", ret);
					}
				} else {
					*num_read += 1U;
				}

				/* end resource formatting */
				ret = engine_put_end_r(&msg->out, &msg->path);
				if (ret < 0) {
					return ret;
				}
			}

			/* on single read break if errors */
			if (ret < 0 && msg->path.level > LWM2M_PATH_LEVEL_OBJECT_INST) {
				break;
			}

			/* when reading multiple resources ignore return code */
			ret = 0;
		}

move_forward:
		ret = engine_put_end_oi(&msg->out, &msg->path);
		if (ret < 0) {
			return ret;
		}

		if (msg->path.level <= LWM2M_PATH_LEVEL_OBJECT) {
			/* advance to the next object instance */
			obj_inst = next_engine_obj_inst(msg->path.obj_id, obj_inst->obj_inst_id);
		} else {
			obj_inst = NULL;
		}
	}

	return ret;
}

int lwm2m_perform_read_op(struct lwm2m_message *msg, uint16_t content_format)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_obj_path temp_path;
	int ret = 0;
	uint8_t num_read = 0U;

	if (msg->path.level >= LWM2M_PATH_LEVEL_OBJECT_INST) {
		obj_inst = get_engine_obj_inst(msg->path.obj_id, msg->path.obj_inst_id);
		if (!obj_inst) {
			/* When Object instace is indicated error have to be reported */
			return -ENOENT;
		}
	} else if (msg->path.level == LWM2M_PATH_LEVEL_OBJECT) {
		/* find first obj_inst with path's obj_id.
		 * Path level 1 can accept NULL. It define empty payload to response.
		 */
		obj_inst = next_engine_obj_inst(msg->path.obj_id, -1);
	}

	/* set output content-format */
	ret = coap_append_option_int(msg->out.out_cpkt, COAP_OPTION_CONTENT_FORMAT, content_format);
	if (ret < 0) {
		LOG_ERR("Error setting response content-format: %d", ret);
		return ret;
	}

	ret = coap_packet_append_payload_marker(msg->out.out_cpkt);
	if (ret < 0) {
		LOG_ERR("Error appending payload marker: %d", ret);
		return ret;
	}

	/* store original path values so we can change them during processing */
	memcpy(&temp_path, &msg->path, sizeof(temp_path));

	if (engine_put_begin(&msg->out, &msg->path) < 0) {
		return -ENOMEM;
	}

	ret = lwm2m_perform_read_object_instance(msg, obj_inst, &num_read);
	if (ret < 0) {
		return ret;
	}

	if (engine_put_end(&msg->out, &msg->path) < 0) {
		return -ENOMEM;
	}

	/* restore original path values */
	memcpy(&msg->path, &temp_path, sizeof(temp_path));

	/* did not read anything even if we should have - on single item */
	if (ret == 0 && num_read == 0U) {
		if (msg->path.level == LWM2M_PATH_LEVEL_RESOURCE) {
			return -ENOENT;
		}

		if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1) &&
		    msg->path.level == LWM2M_PATH_LEVEL_RESOURCE_INST) {
			return -ENOENT;
		}
	}

	return ret;
}

static int lwm2m_discover_add_res(struct lwm2m_message *msg, struct lwm2m_engine_obj_inst *obj_inst,
				  struct lwm2m_engine_res *res)
{
	int ret;
	struct lwm2m_obj_path path = {
		.obj_id = obj_inst->obj->obj_id,
		.obj_inst_id = obj_inst->obj_inst_id,
		.res_id = res->res_id,
		.level = LWM2M_PATH_LEVEL_RESOURCE,
	};

	ret = engine_put_corelink(&msg->out, &path);
	if (ret < 0) {
		return ret;
	}

	/* Report resource instances, if applicable. */
	if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1) && msg->path.level == LWM2M_PATH_LEVEL_RESOURCE &&
	    res->multi_res_inst) {
		for (int i = 0; i < res->res_inst_count; i++) {
			struct lwm2m_engine_res_inst *res_inst = &res->res_instances[i];

			if (res_inst->res_inst_id == RES_INSTANCE_NOT_CREATED) {
				continue;
			}

			path = (struct lwm2m_obj_path){
				.obj_id = obj_inst->obj->obj_id,
				.obj_inst_id = obj_inst->obj_inst_id,
				.res_id = res->res_id,
				.res_inst_id = res_inst->res_inst_id,
				.level = LWM2M_PATH_LEVEL_RESOURCE_INST,
			};

			ret = engine_put_corelink(&msg->out, &path);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

int lwm2m_discover_handler(struct lwm2m_message *msg, bool is_bootstrap)
{
	struct lwm2m_engine_obj *obj;
	struct lwm2m_engine_obj_inst *obj_inst;
	int ret;
	bool reported = false;
	sys_slist_t *engine_obj_list = lwm2m_engine_obj_list();
	sys_slist_t *engine_obj_inst_list = lwm2m_engine_obj_inst_list();

	/* Object ID is required in Device Management Discovery (5.4.2). */
	if (!is_bootstrap && (msg->path.level == LWM2M_PATH_LEVEL_NONE ||
			      msg->path.obj_id == LWM2M_OBJECT_SECURITY_ID)) {
		return -EPERM;
	}

	/* Bootstrap discovery allows to specify at most Object ID. */
	if (is_bootstrap && msg->path.level > LWM2M_PATH_LEVEL_OBJECT) {
		return -EPERM;
	}

	/* set output content-format */
	ret = coap_append_option_int(msg->out.out_cpkt, COAP_OPTION_CONTENT_FORMAT,
				     LWM2M_FORMAT_APP_LINK_FORMAT);
	if (ret < 0) {
		LOG_ERR("Error setting response content-format: %d", ret);
		return ret;
	}

	ret = coap_packet_append_payload_marker(msg->out.out_cpkt);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Add required prefix for bootstrap discovery (5.2.7.3).
	 * For device management discovery, `engine_put_begin()` adds nothing.
	 */
	ret = engine_put_begin(&msg->out, &msg->path);
	if (ret < 0) {
		return ret;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(engine_obj_list, obj, node) {
		/* Skip unrelated objects */
		if (msg->path.level > 0 && msg->path.obj_id != obj->obj_id) {
			continue;
		}

		/* For bootstrap discover, only report object ID when no
		 * instance is available or it's needed to report object
		 * version.
		 * For device management discovery, only report object ID with
		 * attributes if object ID (alone) was provided.
		 */
		if ((is_bootstrap &&
		     (obj->instance_count == 0U || lwm2m_engine_shall_report_obj_version(obj))) ||
		    (!is_bootstrap && msg->path.level == LWM2M_PATH_LEVEL_OBJECT)) {
			struct lwm2m_obj_path path = {
				.obj_id = obj->obj_id,
				.level = LWM2M_PATH_LEVEL_OBJECT,
			};

			ret = engine_put_corelink(&msg->out, &path);
			if (ret < 0) {
				return ret;
			}

			reported = true;

			if (obj->instance_count == 0U) {
				continue;
			}
		}

		SYS_SLIST_FOR_EACH_CONTAINER(engine_obj_inst_list, obj_inst, node) {
			if (obj_inst->obj->obj_id != obj->obj_id) {
				continue;
			}

			/* Skip unrelated object instance. */
			if (msg->path.level > LWM2M_PATH_LEVEL_OBJECT &&
			    msg->path.obj_inst_id != obj_inst->obj_inst_id) {
				continue;
			}

			/* Report object instances only if no Resource ID is
			 * provided.
			 */
			if (msg->path.level <= LWM2M_PATH_LEVEL_OBJECT_INST) {
				struct lwm2m_obj_path path = {
					.obj_id = obj_inst->obj->obj_id,
					.obj_inst_id = obj_inst->obj_inst_id,
					.level = LWM2M_PATH_LEVEL_OBJECT_INST,
				};

				ret = engine_put_corelink(&msg->out, &path);
				if (ret < 0) {
					return ret;
				}

				reported = true;
			}

			/* Do not report resources in bootstrap discovery. */
			if (is_bootstrap) {
				continue;
			}

			for (int i = 0; i < obj_inst->resource_count; i++) {
				/* Skip unrelated resources. */
				if (msg->path.level == LWM2M_PATH_LEVEL_RESOURCE &&
				    msg->path.res_id != obj_inst->resources[i].res_id) {
					continue;
				}

				ret = lwm2m_discover_add_res(msg, obj_inst,
							     &obj_inst->resources[i]);
				if (ret < 0) {
					return ret;
				}

				reported = true;
			}
		}
	}

	return reported ? 0 : -ENOENT;
}

static int do_discover_op(struct lwm2m_message *msg, uint16_t content_format)
{
	switch (content_format) {
	case LWM2M_FORMAT_APP_LINK_FORMAT:
		return do_discover_op_link_format(msg, msg->ctx->bootstrap_mode);

	default:
		LOG_ERR("Unsupported format: %u", content_format);
		return -ENOMSG;
	}
}

static int do_write_op(struct lwm2m_message *msg, uint16_t format)
{
	switch (format) {

	case LWM2M_FORMAT_APP_OCTET_STREAM:
	case LWM2M_FORMAT_PLAIN_TEXT:
	case LWM2M_FORMAT_OMA_PLAIN_TEXT:
		return do_write_op_plain_text(msg);

#ifdef CONFIG_LWM2M_RW_OMA_TLV_SUPPORT
	case LWM2M_FORMAT_OMA_TLV:
	case LWM2M_FORMAT_OMA_OLD_TLV:
		return do_write_op_tlv(msg);
#endif

#ifdef CONFIG_LWM2M_RW_JSON_SUPPORT
	case LWM2M_FORMAT_OMA_JSON:
	case LWM2M_FORMAT_OMA_OLD_JSON:
		return do_write_op_json(msg);
#endif

#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		return do_write_op_senml_json(msg);
#endif

#ifdef CONFIG_LWM2M_RW_CBOR_SUPPORT
	case LWM2M_FORMAT_APP_CBOR:
		return do_write_op_cbor(msg);
#endif

#ifdef CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT
	case LWM2M_FORMAT_APP_SENML_CBOR:
		return do_write_op_senml_cbor(msg);
#endif

	default:
		LOG_ERR("Unsupported format: %u", format);
		return -ENOMSG;
	}
}

static int do_composite_write_op(struct lwm2m_message *msg, uint16_t format)
{
	switch (format) {
#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		return do_write_op_senml_json(msg);
#endif

#if defined(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)
	case LWM2M_FORMAT_APP_SENML_CBOR:
		return do_write_op_senml_cbor(msg);
#endif

	default:
		LOG_ERR("Unsupported format: %u", format);
		return -ENOMSG;
	}
}

static bool lwm2m_engine_path_included(uint8_t code, bool bootstrap_mode)
{
	switch (code & COAP_REQUEST_MASK) {
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	case COAP_METHOD_DELETE:
	case COAP_METHOD_GET:
		if (bootstrap_mode) {
			return false;
		}
		break;
#endif
	case COAP_METHOD_FETCH:
	/* Composite Read operation */
	case COAP_METHOD_IPATCH:
		/* Composite write operation */
		return false;
	default:
		break;
	}
	return true;
}

static int lwm2m_engine_default_content_format(uint16_t *accept_format)
{
	if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1)) {
		/* Select content format use SenML CBOR when it possible */
		if (IS_ENABLED(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)) {
			LOG_DBG("No accept option given. Assume SenML CBOR.");
			*accept_format = LWM2M_FORMAT_APP_SENML_CBOR;
		} else if (IS_ENABLED(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)) {
			LOG_DBG("No accept option given. Assume SenML Json.");
			*accept_format = LWM2M_FORMAT_APP_SEML_JSON;
		} else if (IS_ENABLED(CONFIG_LWM2M_RW_CBOR_SUPPORT)) {
			LOG_DBG("No accept option given. Assume CBOR.");
			*accept_format = LWM2M_FORMAT_APP_CBOR;
		} else {
			LOG_ERR("CBOR, SenML CBOR or SenML JSON is not supported");
			return -ENOTSUP;
		}
	} else if (IS_ENABLED(CONFIG_LWM2M_RW_OMA_TLV_SUPPORT)) {
		LOG_DBG("No accept option given. Assume OMA TLV.");
		*accept_format = LWM2M_FORMAT_OMA_TLV;
	} else {
		LOG_ERR("No default content format is set");
		return -ENOTSUP;
	}

	return 0;
}

static int lwm2m_exec_handler(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst;
	struct lwm2m_engine_res *res = NULL;
	int ret;
	uint8_t *args;
	uint16_t args_len;

	if (!msg) {
		return -EINVAL;
	}

	ret = path_to_objs(&msg->path, &obj_inst, NULL, &res, NULL);
	if (ret < 0) {
		return ret;
	}

	args = (uint8_t *)coap_packet_get_payload(msg->in.in_cpkt, &args_len);

	if (res->execute_cb) {
		return res->execute_cb(obj_inst->obj_inst_id, args, args_len);
	}

	/* TODO: something else to handle for execute? */
	return -ENOENT;
}

int handle_request(struct coap_packet *request, struct lwm2m_message *msg)
{
	int r;
	uint8_t code;
	struct coap_option options[4];
	struct lwm2m_engine_obj *obj = NULL;
	uint8_t token[8];
	uint8_t tkl = 0U;
	uint16_t format = LWM2M_FORMAT_NONE, accept;
	int observe = -1; /* default to -1, 0 = ENABLE, 1 = DISABLE */
	int block_opt, block_num;
	struct lwm2m_block_context *block_ctx = NULL;
	enum coap_block_size block_size;
	uint16_t payload_len = 0U;
	bool last_block = false;
	bool ignore = false;
	const uint8_t *payload_start;

	/* set CoAP request / message */
	msg->in.in_cpkt = request;
	msg->out.out_cpkt = &msg->cpkt;

	/* set default reader/writer */
	msg->in.reader = &plain_text_reader;
	msg->out.writer = &plain_text_writer;

	code = coap_header_get_code(msg->in.in_cpkt);

	/* setup response token */
	tkl = coap_header_get_token(msg->in.in_cpkt, token);
	if (tkl) {
		msg->tkl = tkl;
		msg->token = token;
	}

	/* parse the URL path into components */
	r = coap_find_options(msg->in.in_cpkt, COAP_OPTION_URI_PATH, options, ARRAY_SIZE(options));
	if (r < 0) {
		goto error;
	}

	/* Treat empty URI path option as is there were no option - this will be
	 * represented as a level "zero" in the path structure.
	 */
	if (r == 1 && options[0].len == 0) {
		r = 0;
	}

	if (r == 0 && lwm2m_engine_path_included(code, msg->ctx->bootstrap_mode)) {
		/* No URI path or empty URI path option - allowed only during
		 * bootstrap or CoAP Fetch or iPATCH.
		 */

		r = -EPERM;
		goto error;
	}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	/* check for bootstrap-finish */
	if ((code & COAP_REQUEST_MASK) == COAP_METHOD_POST && r == 1 &&
	    strncmp(options[0].value, "bs", options[0].len) == 0) {
		engine_bootstrap_finish();

		msg->code = COAP_RESPONSE_CODE_CHANGED;

		r = lwm2m_init_message(msg);
		if (r < 0) {
			goto error;
		}

		return 0;
	}
#endif

	r = coap_options_to_path(options, r, &msg->path);
	if (r < 0) {
		r = -ENOENT;
		goto error;
	}

	/* read Content Format / setup in.reader */
	r = coap_find_options(msg->in.in_cpkt, COAP_OPTION_CONTENT_FORMAT, options, 1);
	if (r > 0) {
		format = coap_option_value_to_int(&options[0]);
		r = select_reader(&msg->in, format);
		if (r < 0) {
			goto error;
		}
	}

	/* read Accept / setup out.writer */
	r = coap_find_options(msg->in.in_cpkt, COAP_OPTION_ACCEPT, options, 1);
	if (r > 0) {
		accept = coap_option_value_to_int(&options[0]);
	} else {
		/* Select Default based LWM2M_VERSION */
		r = lwm2m_engine_default_content_format(&accept);
		if (r) {
			goto error;
		}
	}

	r = select_writer(&msg->out, accept);
	if (r < 0) {
		goto error;
	}

	/* Do Only Object find if path have been parsed */
	if (lwm2m_engine_path_included(code, msg->ctx->bootstrap_mode)) {
		if (!(msg->ctx->bootstrap_mode && msg->path.level == LWM2M_PATH_LEVEL_NONE)) {
			/* find registered obj */
			obj = get_engine_obj(msg->path.obj_id);
			if (!obj) {
				/* No matching object found - ignore request */
				r = -ENOENT;
				goto error;
			}
		}
	}

	/* set the operation */
	switch (code & COAP_REQUEST_MASK) {

	case COAP_METHOD_GET:
		/*
		 * LwM2M V1_0_1-20170704-A, table 25,
		 * Discover: CoAP GET + accept=LWM2M_FORMAT_APP_LINK_FORMAT
		 */
		if (accept == LWM2M_FORMAT_APP_LINK_FORMAT) {
			msg->operation = LWM2M_OP_DISCOVER;
			accept = LWM2M_FORMAT_APP_LINK_FORMAT;
		} else {
			msg->operation = LWM2M_OP_READ;
		}

		/* check for observe */
		observe = coap_get_option_int(msg->in.in_cpkt, COAP_OPTION_OBSERVE);
		msg->code = COAP_RESPONSE_CODE_CONTENT;
		break;

	case COAP_METHOD_FETCH:
		msg->operation = LWM2M_OP_READ;
		/* check for observe */
		observe = coap_get_option_int(msg->in.in_cpkt, COAP_OPTION_OBSERVE);
		msg->code = COAP_RESPONSE_CODE_CONTENT;
		break;

	case COAP_METHOD_IPATCH:
		msg->operation = LWM2M_OP_WRITE;
		msg->code = COAP_RESPONSE_CODE_CHANGED;
		break;

	case COAP_METHOD_POST:
		if (msg->path.level == 1U) {
			/* create an object instance */
			msg->operation = LWM2M_OP_CREATE;
			msg->code = COAP_RESPONSE_CODE_CREATED;
		} else if (msg->path.level == 2U) {
			/* write values to an object instance */
			msg->operation = LWM2M_OP_WRITE;
			msg->code = COAP_RESPONSE_CODE_CHANGED;
		} else {
			msg->operation = LWM2M_OP_EXECUTE;
			msg->code = COAP_RESPONSE_CODE_CHANGED;
		}

		break;

	case COAP_METHOD_PUT:
		/* write attributes if content-format is absent */
		if (format == LWM2M_FORMAT_NONE) {
			msg->operation = LWM2M_OP_WRITE_ATTR;
		} else {
			msg->operation = LWM2M_OP_WRITE;
		}

		msg->code = COAP_RESPONSE_CODE_CHANGED;
		break;

	case COAP_METHOD_DELETE:
		msg->operation = LWM2M_OP_DELETE;
		msg->code = COAP_RESPONSE_CODE_DELETED;
		break;

	default:
		break;
	}

	/* setup incoming data */
	payload_start = coap_packet_get_payload(msg->in.in_cpkt, &payload_len);
	if (payload_len > 0) {
		msg->in.offset = payload_start - msg->in.in_cpkt->data;
	} else {
		msg->in.offset = msg->in.in_cpkt->offset;
	}

	/* Check for block transfer */

	block_opt = coap_get_option_int(msg->in.in_cpkt, COAP_OPTION_BLOCK1);
	if (block_opt > 0) {
		last_block = !GET_MORE(block_opt);

		/* RFC7252: 4.6. Message Size */
		block_size = GET_BLOCK_SIZE(block_opt);
		if (!last_block && coap_block_size_to_bytes(block_size) > payload_len) {
			LOG_DBG("Trailing payload is discarded!");
			r = -EFBIG;
			goto error;
		}

		block_num = GET_BLOCK_NUM(block_opt);

		/* Try to retrieve existing block context. If one not exists,
		 * and we've received first block, allocate new context.
		 */
		r = get_block_ctx(token, tkl, &block_ctx);
		if (r < 0 && block_num == 0) {
			r = init_block_ctx(token, tkl, &block_ctx);
		}

		if (r < 0) {
			LOG_ERR("Cannot find block context");
			goto error;
		}

		msg->in.block_ctx = block_ctx;

		if (block_num < block_ctx->expected) {
			LOG_WRN("Block already handled %d, expected %d", block_num,
				block_ctx->expected);
			ignore = true;
		} else if (block_num > block_ctx->expected) {
			LOG_WRN("Block out of order %d, expected %d", block_num,
				block_ctx->expected);
			r = -EFAULT;
			goto error;
		} else {
			r = coap_update_from_block(msg->in.in_cpkt, &block_ctx->ctx);
			if (r < 0) {
				LOG_ERR("Error from block update: %d", r);
				goto error;
			}

			block_ctx->last_block = last_block;

			/* Initial block sent by the server might be larger than
			 * our block size therefore it is needed to take this
			 * into account when calculating next expected block
			 * number.
			 */
			block_ctx->expected +=
				GET_BLOCK_SIZE(block_opt) - block_ctx->ctx.block_size + 1;
		}

		/* Handle blockwise 1 (Part 1): Set response code */
		if (!last_block) {
			msg->code = COAP_RESPONSE_CODE_CONTINUE;
		}
	}

	/* render CoAP packet header */
	r = lwm2m_init_message(msg);
	if (r < 0) {
		goto error;
	}

	if (!ignore) {
#if defined(CONFIG_LWM2M_ACCESS_CONTROL_ENABLE)
		r = access_control_check_access(msg->path.obj_id, msg->path.obj_inst_id,
						msg->ctx->srv_obj_inst, msg->operation,
						msg->ctx->bootstrap_mode);
		if (r < 0) {
			LOG_ERR("Access denied - Server obj %u does not have proper access to "
				"resource",
				msg->ctx->srv_obj_inst);
			goto error;
		}
#endif
		switch (msg->operation) {

		case LWM2M_OP_READ:
			if (observe >= 0) {
				/* Validate That Token is valid for Observation */
				if (!msg->token) {
					LOG_ERR("OBSERVE request missing token");
					r = -EINVAL;
					goto error;
				}

				if ((code & COAP_REQUEST_MASK) == COAP_METHOD_GET) {
					/* Normal Observation Request or Cancel */
					r = lwm2m_engine_observation_handler(msg, observe, accept,
									     false);
					if (r < 0) {
						goto error;
					}

					r = do_read_op(msg, accept);
				} else {
					/* Composite Observation request & cancel handler */
					r = lwm2m_engine_observation_handler(msg, observe, accept,
									     true);
					if (r < 0) {
						goto error;
					}
				}
			} else {
				if ((code & COAP_REQUEST_MASK) == COAP_METHOD_GET) {
					r = do_read_op(msg, accept);
				} else {
					r = do_composite_read_op(msg, accept);
				}
			}
			break;

		case LWM2M_OP_DISCOVER:
			r = do_discover_op(msg, accept);
			break;

		case LWM2M_OP_WRITE:
		case LWM2M_OP_CREATE:
			if ((code & COAP_REQUEST_MASK) == COAP_METHOD_IPATCH) {
				/* iPATCH is for Composite purpose */
				r = do_composite_write_op(msg, format);
			} else {
				/* Single resource write Operation */
				r = do_write_op(msg, format);
			}
#if defined(CONFIG_LWM2M_ACCESS_CONTROL_ENABLE)
			if (msg->operation == LWM2M_OP_CREATE && r >= 0) {
				access_control_add(msg->path.obj_id, msg->path.obj_inst_id,
						   msg->ctx->srv_obj_inst);
			}
#endif
			break;

		case LWM2M_OP_WRITE_ATTR:
			r = lwm2m_write_attr_handler(obj, msg);
			break;

		case LWM2M_OP_EXECUTE:
			r = lwm2m_exec_handler(msg);
			break;

		case LWM2M_OP_DELETE:
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
			if (msg->ctx->bootstrap_mode) {
				r = bootstrap_delete(msg);
				break;
			}
#endif
			r = lwm2m_delete_handler(msg);
			break;

		default:
			LOG_ERR("Unknown operation: %u", msg->operation);
			r = -EINVAL;
		}

		if (r < 0) {
			goto error;
		}
	}

	/* Handle blockwise 1 (Part 2): Append BLOCK1 option / free context */
	if (block_ctx) {
		if (!last_block) {
			/* More to come, ack with correspond block # */
			r = coap_append_block1_option(msg->out.out_cpkt, &block_ctx->ctx);
			if (r < 0) {
				/* report as internal server error */
				LOG_ERR("Fail adding block1 option: %d", r);
				r = -EINVAL;
				goto error;
			}
		} else {
			/* Free context when finished */
			free_block_ctx(block_ctx);
		}
	}

	return 0;

error:
	lwm2m_reset_message(msg, false);
	if (r == -ENOENT) {
		msg->code = COAP_RESPONSE_CODE_NOT_FOUND;
	} else if (r == -EPERM) {
		msg->code = COAP_RESPONSE_CODE_NOT_ALLOWED;
	} else if (r == -EEXIST) {
		msg->code = COAP_RESPONSE_CODE_BAD_REQUEST;
	} else if (r == -EFAULT) {
		msg->code = COAP_RESPONSE_CODE_INCOMPLETE;
	} else if (r == -EFBIG) {
		msg->code = COAP_RESPONSE_CODE_REQUEST_TOO_LARGE;
	} else if (r == -ENOTSUP) {
		msg->code = COAP_RESPONSE_CODE_NOT_IMPLEMENTED;
	} else if (r == -ENOMSG) {
		msg->code = COAP_RESPONSE_CODE_UNSUPPORTED_CONTENT_FORMAT;
	} else if (r == -EACCES) {
		msg->code = COAP_RESPONSE_CODE_UNAUTHORIZED;
	} else {
		/* Failed to handle the request */
		msg->code = COAP_RESPONSE_CODE_INTERNAL_ERROR;
	}

	r = lwm2m_init_message(msg);
	if (r < 0) {
		LOG_ERR("Error recreating message: %d", r);
	}

	/* Free block context when error happened */
	free_block_ctx(block_ctx);

	return 0;
}

static int lwm2m_response_promote_to_con(struct lwm2m_message *msg)
{
	int ret;

	msg->type = COAP_TYPE_CON;
	msg->mid = coap_next_id();

	/* Since the response CoAP packet is already generated at this point,
	 * tweak the specific fields manually:
	 * - CoAP message type (byte 0, bits 2 and 3)
	 * - CoAP message id (bytes 2 and 3)
	 */
	msg->cpkt.data[0] &= ~(0x3 << 4);
	msg->cpkt.data[0] |= (msg->type & 0x3) << 4;
	msg->cpkt.data[2] = msg->mid >> 8;
	msg->cpkt.data[3] = (uint8_t)msg->mid;

	if (msg->pending) {
		coap_pending_clear(msg->pending);
	}

	/* Add the packet to the pending list. */
	msg->pending = coap_pending_next_unused(msg->ctx->pendings, ARRAY_SIZE(msg->ctx->pendings));
	if (!msg->pending) {
		LOG_ERR("Unable to find a free pending to track "
			"retransmissions.");
		return -ENOMEM;
	}

	ret = coap_pending_init(msg->pending, &msg->cpkt, &msg->ctx->remote_addr,
				CONFIG_COAP_MAX_RETRANSMIT);
	if (ret < 0) {
		LOG_ERR("Unable to initialize a pending "
			"retransmission (err:%d).",
			ret);
	}

	return ret;
}

void lwm2m_udp_receive(struct lwm2m_ctx *client_ctx, uint8_t *buf, uint16_t buf_len,
		       struct sockaddr *from_addr, udp_request_handler_cb_t udp_request_handler)
{
	struct lwm2m_message *msg = NULL;
	struct coap_pending *pending;
	struct coap_reply *reply;
	struct coap_packet response;
	int r;
	uint8_t token[8];

	r = coap_packet_parse(&response, buf, buf_len, NULL, 0);
	if (r < 0) {
		LOG_ERR("Invalid data received (err:%d)", r);
		return;
	}

	(void)coap_header_get_token(&response, token);
	pending = coap_pending_received(&response, client_ctx->pendings,
					ARRAY_SIZE(client_ctx->pendings));
	if (pending && coap_header_get_type(&response) == COAP_TYPE_ACK) {
		msg = find_msg(pending, NULL);
		if (msg == NULL) {
			LOG_DBG("Orphaned pending %p.", pending);
			coap_pending_clear(pending);
			return;
		}

		msg->acknowledged = true;

		if (msg->reply == NULL) {
			/* No response expected, release the message. */
			lwm2m_reset_message(msg, true);
			return;
		}

		/* If the original message was a request and an empty
		 * ACK was received, expect separate response later.
		 */
		if ((msg->code >= COAP_METHOD_GET) && (msg->code <= COAP_METHOD_DELETE) &&
		    (coap_header_get_code(&response) == COAP_CODE_EMPTY)) {
			LOG_DBG("Empty ACK, expect separate response.");
			return;
		}
	}

	LOG_DBG("checking for reply from [%s]", lwm2m_sprint_ip_addr(from_addr));
	reply = coap_response_received(&response, from_addr, client_ctx->replies,
				       ARRAY_SIZE(client_ctx->replies));
	if (reply) {
		msg = find_msg(NULL, reply);

		if (coap_header_get_type(&response) == COAP_TYPE_CON) {
			r = lwm2m_send_empty_ack(client_ctx, coap_header_get_id(&response));
			if (r < 0) {
				LOG_ERR("Error transmitting ACK");
			}
		}

		/* skip release if reply->user_data has error condition */
		if (reply && reply->user_data == (void *)COAP_REPLY_STATUS_ERROR) {
			/* reset reply->user_data for next time */
			reply->user_data = (void *)COAP_REPLY_STATUS_NONE;
			LOG_DBG("reply %p NOT removed", reply);
			return;
		}

		/* free up msg resources */
		if (msg) {
			lwm2m_reset_message(msg, true);
		}

		LOG_DBG("reply %p handled and removed", reply);
		return;
	}

	/*
	 * If no normal response handler is found, then this is
	 * a new request coming from the server.  Let's look
	 * at registered objects to find a handler.
	 */
	if (udp_request_handler && coap_header_get_type(&response) == COAP_TYPE_CON) {
		msg = lwm2m_get_message(client_ctx);
		if (!msg) {
			LOG_ERR("Unable to get a lwm2m message!");
			return;
		}

		/* Create a response message if we reach this point */
		msg->type = COAP_TYPE_ACK;
		msg->code = coap_header_get_code(&response);
		msg->mid = coap_header_get_id(&response);
		/* skip token generation by default */
		msg->tkl = 0;

		client_ctx->processed_req = msg;

		/* process the response to this request */
		r = udp_request_handler(&response, msg);
		if (r < 0) {
			return;
		}

		if (msg->acknowledged) {
			r = lwm2m_response_promote_to_con(msg);
			if (r < 0) {
				LOG_ERR("Failed to promote response to CON: %d", r);
				lwm2m_reset_message(msg, true);
				return;
			}
		}

		client_ctx->processed_req = NULL;
		lwm2m_send_message_async(msg);
	} else {
		LOG_DBG("No handler for response");
	}
}

static void notify_message_timeout_cb(struct lwm2m_message *msg)
{
	if (msg->ctx != NULL) {
		struct observe_node *obs;
		struct lwm2m_ctx *client_ctx = msg->ctx;
		sys_snode_t *prev_node = NULL;

		obs = engine_observe_node_discover(&client_ctx->observer, &prev_node, NULL,
						   msg->token, msg->tkl);

		if (obs) {
			obs->active_tx_operation = false;
			if (client_ctx->observe_cb) {
				client_ctx->observe_cb(LWM2M_OBSERVE_EVENT_NOTIFY_TIMEOUT,
						       &msg->path, msg->reply->user_data);
			}

			lwm2m_rd_client_timeout(client_ctx);
		}
	}

	LOG_ERR("Notify Message Timed Out : %p", msg);
}

static struct lwm2m_obj_path *lwm2m_read_first_path_ptr(sys_slist_t *lwm2m_path_list)
{
	struct lwm2m_obj_path_list *entry;

	entry = (struct lwm2m_obj_path_list *)sys_slist_peek_head(lwm2m_path_list);
	return &entry->path;
}

static int notify_message_reply_cb(const struct coap_packet *response, struct coap_reply *reply,
				   const struct sockaddr *from)
{
	int ret = 0;
	uint8_t type, code;
	struct lwm2m_message *msg;
	struct observe_node *obs;
	sys_snode_t *prev_node = NULL;

	type = coap_header_get_type(response);
	code = coap_header_get_code(response);

	LOG_DBG("NOTIFY ACK type:%u code:%d.%d reply_token:'%s'", type,
		COAP_RESPONSE_CODE_CLASS(code), COAP_RESPONSE_CODE_DETAIL(code),
		sprint_token(reply->token, reply->tkl));

	msg = find_msg(NULL, reply);

	/* remove observer on COAP_TYPE_RESET */
	if (type == COAP_TYPE_RESET) {
		if (reply->tkl > 0) {
			ret = engine_remove_observer_by_token(msg->ctx, reply->token, reply->tkl);
			if (ret) {
				LOG_ERR("remove observe error: %d", ret);
			}
		} else {
			LOG_ERR("notify reply missing token -- ignored.");
		}
	} else {
		obs = engine_observe_node_discover(&msg->ctx->observer, &prev_node, NULL,
						   reply->token, reply->tkl);

		if (obs) {
			obs->active_tx_operation = false;
			if (msg->ctx->observe_cb) {
				msg->ctx->observe_cb(LWM2M_OBSERVE_EVENT_NOTIFY_ACK,
						     lwm2m_read_first_path_ptr(&obs->path_list),
						     reply->user_data);
			}
		}
	}

	return 0;
}

static int do_send_op(struct lwm2m_message *msg, uint16_t content_format,
		      sys_slist_t *lwm2m_path_list)
{
	switch (content_format) {
#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		return do_send_op_senml_json(msg, lwm2m_path_list);
#endif

#if defined(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)
	case LWM2M_FORMAT_APP_SENML_CBOR:
		return do_send_op_senml_cbor(msg, lwm2m_path_list);
#endif

	default:
		LOG_ERR("Unsupported content-format for /dp: %u", content_format);
		return -ENOMSG;
	}
}

int generate_notify_message(struct lwm2m_ctx *ctx, struct observe_node *obs, void *user_data)
{
	struct lwm2m_message *msg;
	struct lwm2m_engine_obj_inst *obj_inst;
	struct lwm2m_obj_path *path;
	int ret = 0;

	msg = lwm2m_get_message(ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}
	if (!obs->composite) {
		path = lwm2m_read_first_path_ptr(&obs->path_list);
		if (!path) {
			LOG_ERR("Observation node not include path");
			ret = -EINVAL;
			goto cleanup;
		}
		/* copy path */
		memcpy(&msg->path, path, sizeof(struct lwm2m_obj_path));
		LOG_DBG("[%s] NOTIFY MSG START: %u/%u/%u(%u) token:'%s' [%s] %lld",
			obs->resource_update ? "MANUAL" : "AUTO", path->obj_id, path->obj_inst_id,
			path->res_id, path->level, sprint_token(obs->token, obs->tkl),
			lwm2m_sprint_ip_addr(&ctx->remote_addr), (long long)k_uptime_get());

		obj_inst = get_engine_obj_inst(path->obj_id, path->obj_inst_id);
		if (!obj_inst) {
			LOG_ERR("unable to get engine obj for %u/%u", path->obj_id,
				path->obj_inst_id);
			ret = -EINVAL;
			goto cleanup;
		}
	} else {
		LOG_DBG("[%s] NOTIFY MSG START: (Composite)) token:'%s' [%s] %lld",
			obs->resource_update ? "MANUAL" : "AUTO",
			sprint_token(obs->token, obs->tkl), lwm2m_sprint_ip_addr(&ctx->remote_addr),
			(long long)k_uptime_get());
	}

	msg->operation = LWM2M_OP_READ;
	msg->type = COAP_TYPE_CON;
	msg->code = COAP_RESPONSE_CODE_CONTENT;
	msg->mid = coap_next_id();
	msg->token = obs->token;
	msg->tkl = obs->tkl;
	msg->reply_cb = notify_message_reply_cb;
	msg->message_timeout_cb = notify_message_timeout_cb;
	msg->out.out_cpkt = &msg->cpkt;

	ret = lwm2m_init_message(msg);
	if (ret < 0) {
		LOG_ERR("Unable to init lwm2m message! (err: %d)", ret);
		goto cleanup;
	}

	/* lwm2m_init_message() cleans the coap reply fields, so we assign our data here */
	msg->reply->user_data = user_data;

	/* each notification should increment the obs counter */
	obs->counter++;
	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_OBSERVE, obs->counter);
	if (ret < 0) {
		LOG_ERR("OBSERVE option error: %d", ret);
		goto cleanup;
	}

	/* set the output writer */
	select_writer(&msg->out, obs->format);
	if (obs->composite) {
		/* Use do send which actually do Composite read operation */
		ret = do_send_op(msg, obs->format, &obs->path_list);
	} else {
		ret = do_read_op(msg, obs->format);
	}

	if (ret < 0) {
		LOG_ERR("error in multi-format read (err:%d)", ret);
		goto cleanup;
	}

	obs->active_tx_operation = true;
	obs->resource_update = false;
	lwm2m_information_interface_send(msg);

	LOG_DBG("NOTIFY MSG: SENT");
	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static int lwm2m_perform_composite_read_root(struct lwm2m_message *msg, uint8_t *num_read)
{
	int ret;
	struct lwm2m_engine_obj *obj;
	struct lwm2m_engine_obj_inst *obj_inst;
	sys_slist_t *engine_obj_list = lwm2m_engine_obj_list();

	SYS_SLIST_FOR_EACH_CONTAINER(engine_obj_list, obj, node) {
		/* Security obj MUST NOT be part of registration message */
		if (obj->obj_id == LWM2M_OBJECT_SECURITY_ID) {
			continue;
		}

		msg->path.level = 1;
		msg->path.obj_id = obj->obj_id;

		obj_inst = next_engine_obj_inst(msg->path.obj_id, -1);

		if (!obj_inst) {
			continue;
		}

		ret = lwm2m_perform_read_object_instance(msg, obj_inst, num_read);
		if (ret == -ENOMEM) {
			return ret;
		}
	}
	return 0;
}

int lwm2m_perform_composite_read_op(struct lwm2m_message *msg, uint16_t content_format,
				    sys_slist_t *lwm2m_path_list)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_obj_path_list *entry;
	int ret = 0;
	uint8_t num_read = 0U;

	/* set output content-format */
	ret = coap_append_option_int(msg->out.out_cpkt, COAP_OPTION_CONTENT_FORMAT, content_format);
	if (ret < 0) {
		LOG_ERR("Error setting response content-format: %d", ret);
		return ret;
	}

	ret = coap_packet_append_payload_marker(msg->out.out_cpkt);
	if (ret < 0) {
		LOG_ERR("Error appending payload marker: %d", ret);
		return ret;
	}

	/* Add object start mark */
	engine_put_begin(&msg->out, &msg->path);

	/* Read resource from path */
	SYS_SLIST_FOR_EACH_CONTAINER(lwm2m_path_list, entry, node) {
		/* Copy path to message path */
		memcpy(&msg->path, &entry->path, sizeof(struct lwm2m_obj_path));

		if (msg->path.level >= LWM2M_PATH_LEVEL_OBJECT_INST) {
			obj_inst = get_engine_obj_inst(msg->path.obj_id, msg->path.obj_inst_id);
		} else if (msg->path.level == LWM2M_PATH_LEVEL_OBJECT) {
			/* find first obj_inst with path's obj_id */
			obj_inst = next_engine_obj_inst(msg->path.obj_id, -1);
		} else {
			/* Read root Path */
			ret = lwm2m_perform_composite_read_root(msg, &num_read);
			if (ret == -ENOMEM) {
				LOG_ERR("Supported message size is too small for read root");
				return ret;
			}
			break;
		}

		if (!obj_inst) {
			continue;
		}

		ret = lwm2m_perform_read_object_instance(msg, obj_inst, &num_read);
		if (ret == -ENOMEM) {
			return ret;
		}
	}
	/* did not read anything even if we should have - on single item */
	if (num_read == 0U) {
		return -ENOENT;
	}

	/* Add object end mark */
	if (engine_put_end(&msg->out, &msg->path) < 0) {
		return -ENOMEM;
	}

	return 0;
}

int lwm2m_parse_peerinfo(char *url, struct lwm2m_ctx *client_ctx, bool is_firmware_uri)
{
	struct http_parser_url parser;
#if defined(CONFIG_LWM2M_DNS_SUPPORT)
	struct addrinfo *res, hints = {0};
#endif
	int ret;
	uint16_t off, len;
	uint8_t tmp;

	LOG_DBG("Parse url: %s", url);

	http_parser_url_init(&parser);
	ret = http_parser_parse_url(url, strlen(url), 0, &parser);
	if (ret < 0) {
		LOG_ERR("Invalid url: %s", url);
		return -ENOTSUP;
	}

	off = parser.field_data[UF_SCHEMA].off;
	len = parser.field_data[UF_SCHEMA].len;

	/* check for supported protocol */
	if (strncmp(url + off, "coaps", len) != 0) {
		return -EPROTONOSUPPORT;
	}

	/* check for DTLS requirement */
	client_ctx->use_dtls = false;
	if (len == 5U && strncmp(url + off, "coaps", len) == 0) {
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
		client_ctx->use_dtls = true;
#else
		return -EPROTONOSUPPORT;
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */
	}

	if (!(parser.field_set & (1 << UF_PORT))) {
		if (is_firmware_uri && client_ctx->use_dtls) {
			/* Set to default coaps firmware update port */
			parser.port = CONFIG_LWM2M_FIRMWARE_PORT_SECURE;
		} else if (is_firmware_uri) {
			/* Set to default coap firmware update port */
			parser.port = CONFIG_LWM2M_FIRMWARE_PORT_NONSECURE;
		} else {
			/* Set to default LwM2M server port */
			parser.port = CONFIG_LWM2M_PEER_PORT;
		}
	}

	off = parser.field_data[UF_HOST].off;
	len = parser.field_data[UF_HOST].len;

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	/** copy url pointer to be used in socket */
	client_ctx->desthostname = url + off;
	client_ctx->desthostnamelen = len;
#endif

	/* truncate host portion */
	tmp = url[off + len];
	url[off + len] = '\0';

	/* initialize remote_addr */
	(void)memset(&client_ctx->remote_addr, 0, sizeof(client_ctx->remote_addr));

	/* try and set IP address directly */
	client_ctx->remote_addr.sa_family = AF_INET6;
	ret = net_addr_pton(AF_INET6, url + off,
			    &((struct sockaddr_in6 *)&client_ctx->remote_addr)->sin6_addr);
	/* Try to parse again using AF_INET */
	if (ret < 0) {
		client_ctx->remote_addr.sa_family = AF_INET;
		ret = net_addr_pton(AF_INET, url + off,
				    &((struct sockaddr_in *)&client_ctx->remote_addr)->sin_addr);
	}

	if (ret < 0) {
#if defined(CONFIG_LWM2M_DNS_SUPPORT)
#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
		hints.ai_family = AF_UNSPEC;
#elif defined(CONFIG_NET_IPV6)
		hints.ai_family = AF_INET6;
#elif defined(CONFIG_NET_IPV4)
		hints.ai_family = AF_INET;
#else
		hints.ai_family = AF_UNSPEC;
#endif /* defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4) */
		hints.ai_socktype = SOCK_DGRAM;
		ret = getaddrinfo(url + off, NULL, &hints, &res);
		if (ret != 0) {
			LOG_ERR("Unable to resolve address");
			/* DNS error codes don't align with normal errors */
			ret = -ENOENT;
			goto cleanup;
		}

		memcpy(&client_ctx->remote_addr, res->ai_addr, sizeof(client_ctx->remote_addr));
		client_ctx->remote_addr.sa_family = res->ai_family;
		freeaddrinfo(res);
#else
		goto cleanup;
#endif /* CONFIG_LWM2M_DNS_SUPPORT */
	}

	/* set port */
	if (client_ctx->remote_addr.sa_family == AF_INET6) {
		net_sin6(&client_ctx->remote_addr)->sin6_port = htons(parser.port);
	} else if (client_ctx->remote_addr.sa_family == AF_INET) {
		net_sin(&client_ctx->remote_addr)->sin_port = htons(parser.port);
	} else {
		ret = -EPROTONOSUPPORT;
	}

cleanup:
	/* restore host separator */
	url[off + len] = tmp;
	return ret;
}

int do_composite_read_op_for_parsed_list(struct lwm2m_message *msg, uint16_t content_format,
					 sys_slist_t *path_list)
{
	switch (content_format) {

#if defined(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)
	case LWM2M_FORMAT_APP_SEML_JSON:
		return do_composite_read_op_for_parsed_list_senml_json(msg, path_list);
#endif

#if defined(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)
	case LWM2M_FORMAT_APP_SENML_CBOR:
		return do_composite_read_op_for_parsed_path_senml_cbor(msg, path_list);
#endif

	default:
		LOG_ERR("Unsupported content-format: %u", content_format);
		return -ENOMSG;
	}
}

#if defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)
static int do_send_reply_cb(const struct coap_packet *response, struct coap_reply *reply,
			    const struct sockaddr *from)
{
	uint8_t code;

	code = coap_header_get_code(response);
	LOG_DBG("Send callback (code:%u.%u)", COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	if (code == COAP_RESPONSE_CODE_CHANGED) {
		LOG_INF("Send done!");
		return 0;
	}

	LOG_ERR("Failed with code %u.%u. Not Retrying.", COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	return 0;
}

static void do_send_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("Send Timeout");
	lwm2m_rd_client_timeout(msg->ctx);
}
#endif

int lwm2m_engine_send(struct lwm2m_ctx *ctx, char const *path_list[], uint8_t path_list_size,
		      bool confirmation_request)
{
#if defined(CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1)
	struct lwm2m_message *msg;
	int ret;
	uint16_t content_format;

	/* Path list buffer */
	struct lwm2m_obj_path temp;
	struct lwm2m_obj_path_list lwm2m_path_list_buf[CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE];
	sys_slist_t lwm2m_path_list;
	sys_slist_t lwm2m_path_free_list;

	/* Validate Connection */
	if (!lwm2m_rd_client_is_registred(ctx)) {
		return -EPERM;
	}

	if (lwm2m_server_get_mute_send(ctx->srv_obj_inst)) {
		LOG_WRN("Send operation is muted by server");
		return -EPERM;
	}

	/* Init list */
	lwm2m_engine_path_list_init(&lwm2m_path_list, &lwm2m_path_free_list, lwm2m_path_list_buf,
				    CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE);

	if (path_list_size > CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE) {
		return -E2BIG;
	}

	if (IS_ENABLED(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)) {
		content_format = LWM2M_FORMAT_APP_SENML_CBOR;
	} else if (IS_ENABLED(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)) {
		content_format = LWM2M_FORMAT_APP_SEML_JSON;
	} else {
		LOG_WRN("SenML CBOR or JSON is not supported");
		return -ENOTSUP;
	}

	/* Parse Path to internal used object path format */
	for (int i = 0; i < path_list_size; i++) {
		ret = lwm2m_string_to_path(path_list[i], &temp, '/');
		if (ret < 0) {
			return ret;
		}
		/* Add to linked list */
		if (lwm2m_engine_add_path_to_list(&lwm2m_path_list, &lwm2m_path_free_list, &temp)) {
			return -1;
		}
	}
	/* Clear path which are part are part of recursive path /1 will include /1/0/1 */
	lwm2m_engine_clear_duplicate_path(&lwm2m_path_list, &lwm2m_path_free_list);

	/* Allocate Message buffer */
	msg = lwm2m_get_message(ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	if (confirmation_request) {
		msg->type = COAP_TYPE_CON;
		msg->reply_cb = do_send_reply_cb;
		msg->message_timeout_cb = do_send_timeout_cb;
	} else {
		msg->type = COAP_TYPE_NON_CON;
		msg->reply_cb = NULL;
		msg->message_timeout_cb = NULL;
	}
	msg->code = COAP_METHOD_POST;
	msg->mid = coap_next_id();
	msg->tkl = LWM2M_MSG_TOKEN_GENERATE_NEW;
	msg->out.out_cpkt = &msg->cpkt;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}
	ret = select_writer(&msg->out, content_format);
	if (ret) {
		goto cleanup;
	}

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH, LWM2M_DP_CLIENT_URI,
					strlen(LWM2M_DP_CLIENT_URI));
	if (ret < 0) {
		goto cleanup;
	}

	/* Write requested path data */
	ret = do_send_op(msg, content_format, &lwm2m_path_list);
	if (ret < 0) {
		LOG_ERR("Send (err:%d)", ret);
		goto cleanup;
	}
	LOG_INF("Send op to server (/dp)");
	lwm2m_information_interface_send(msg);

	return 0;
cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
#else
	LOG_WRN("LwM2M send is only supported for CONFIG_LWM2M_SERVER_OBJECT_VERSION_1_1");
	return -ENOTSUP;
#endif
}
