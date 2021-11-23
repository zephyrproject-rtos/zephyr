/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_pull_context
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <net/http_parser.h>
#include <net/socket.h>

#include "lwm2m_pull_context.h"
#include "lwm2m_engine.h"

#define NETWORK_INIT_TIMEOUT        K_SECONDS(10)
#define NETWORK_CONNECT_TIMEOUT     K_SECONDS(10)
#define PACKET_TRANSFER_RETRY_MAX   3

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
#define COAP2COAP_PROXY_URI_PATH	"coap2coap"
#define COAP2HTTP_PROXY_URI_PATH	"coap2http"

static char proxy_uri[URI_LEN];
#endif

static struct firmware_pull_context *context = NULL;

static void do_transmit_timeout_cb(struct lwm2m_message *msg);

static int transfer_request(struct coap_block_context *ctx,
			    uint8_t *token, uint8_t tkl,
			    coap_reply_t reply_cb)
{
	struct lwm2m_message *msg;
	int ret;
	char *cursor;
#if !defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	struct http_parser_url parser;
	uint16_t off, len;
	char *next_slash;
#endif

	msg = lwm2m_get_message(&context->firmware_ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_GET;
	msg->mid = coap_next_id();
	msg->token = token;
	msg->tkl = tkl;
	msg->reply_cb = reply_cb;
	msg->message_timeout_cb = do_transmit_timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret < 0) {
		LOG_ERR("Error setting up lwm2m message");
		goto cleanup;
	}

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	/* TODO: shift to lower case */
	if (strncmp(context->uri, "http", 4) == 0) {
		cursor = COAP2HTTP_PROXY_URI_PATH;
	} else if (strncmp(context->uri, "coap", 4) == 0) {
		cursor = COAP2COAP_PROXY_URI_PATH;
	} else {
		ret = -EPROTONOSUPPORT;
		LOG_ERR("Unsupported schema");
		goto cleanup;
	}

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					cursor, strlen(cursor));
	if (ret < 0) {
		LOG_ERR("Error adding URI_PATH '%s'", log_strdup(cursor));
		goto cleanup;
	}
#else
	http_parser_url_init(&parser);
	ret = http_parser_parse_url(context->uri, strlen(context->uri), 0,
				    &parser);
	if (ret < 0) {
		LOG_ERR("Invalid firmware url: %s", log_strdup(context->uri));
		ret = -ENOTSUP;
		goto cleanup;
	}

	/* if path is not available, off/len will be zero */
	off = parser.field_data[UF_PATH].off;
	len = parser.field_data[UF_PATH].len;
	cursor = context->uri + off;

	/* add path portions (separated by slashes) */
	while (len > 0 && (next_slash = strchr(cursor, '/')) != NULL) {
		if (next_slash != cursor) {
			ret = coap_packet_append_option(&msg->cpkt,
							COAP_OPTION_URI_PATH,
							cursor,
							next_slash - cursor);
			if (ret < 0) {
				LOG_ERR("Error adding URI_PATH");
				goto cleanup;
			}
		}

		/* skip slash */
		len -= (next_slash - cursor) + 1;
		cursor = next_slash + 1;
	}

	if (len > 0) {
		/* flush the rest */
		ret = coap_packet_append_option(&msg->cpkt,
						COAP_OPTION_URI_PATH,
						cursor, len);
		if (ret < 0) {
			LOG_ERR("Error adding URI_PATH");
			goto cleanup;
		}
	}
#endif

	ret = coap_append_block2_option(&msg->cpkt, ctx);
	if (ret < 0) {
		LOG_ERR("Unable to add block2 option.");
		goto cleanup;
	}

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_PROXY_URI,
					context->uri, strlen(context->uri));
	if (ret < 0) {
		LOG_ERR("Error adding PROXY_URI '%s'",
			log_strdup(context->uri));
		goto cleanup;
	}
#else
	/* Ask the server to provide a size estimate */
	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_SIZE2, 0);
	if (ret < 0) {
		LOG_ERR("Unable to add size2 option.");
		goto cleanup;
	}
#endif

	/* send request */
	ret = lwm2m_send_message_async(msg);
	if (ret < 0) {
		LOG_ERR("Error sending LWM2M packet (err:%d).", ret);
		goto cleanup;
	}

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static int
do_firmware_transfer_reply_cb(const struct coap_packet *response,
			      struct coap_reply *reply,
			      const struct sockaddr *from)
{
	int ret;
	bool last_block;
	uint8_t token[8];
	uint8_t tkl;
	uint16_t payload_len, payload_offset, len;
	struct coap_packet *check_response = (struct coap_packet *)response;
	struct lwm2m_engine_res *res = NULL;
	lwm2m_engine_set_data_cb_t write_cb;
	size_t write_buflen;
	uint8_t resp_code, *write_buf;
	struct coap_block_context received_block_ctx;

	/* token is used to determine a valid ACK vs a separated response */
	tkl = coap_header_get_token(check_response, token);

	/* If separated response (ACK) return and wait for response */
	if (!tkl && coap_header_get_type(response) == COAP_TYPE_ACK) {
		return 0;
	} else if (coap_header_get_type(response) == COAP_TYPE_CON) {
		/* Send back ACK so the server knows we received the pkt */
		ret = lwm2m_send_empty_ack(&context->firmware_ctx,
					   coap_header_get_id(check_response));
		if (ret < 0) {
			LOG_ERR("Error transmitting ACK");
			goto error;
		}
	}

	/* Check response code from server. Expecting (2.05) */
	resp_code = coap_header_get_code(check_response);
	if (resp_code != COAP_RESPONSE_CODE_CONTENT) {
		LOG_ERR("Unexpected response from server: %d.%d",
			COAP_RESPONSE_CODE_CLASS(resp_code),
			COAP_RESPONSE_CODE_DETAIL(resp_code));
		ret = -ENOMSG;
		goto error;
	}

	/* save main firmware block context */
	memcpy(&received_block_ctx, &context->block_ctx,
	       sizeof(context->block_ctx));

	ret = coap_update_from_block(check_response, &context->block_ctx);
	if (ret < 0) {
		LOG_ERR("Error from block update: %d", ret);
		ret = -EFAULT;
		goto error;
	}

	/* test for duplicate transfer */
	if (context->block_ctx.current < received_block_ctx.current) {
		LOG_WRN("Duplicate packet ignored");

		/* restore main firmware block context */
		memcpy(&context->block_ctx, &received_block_ctx,
		       sizeof(context->block_ctx));

		/* set reply->user_data to error to avoid releasing */
		reply->user_data = (void *)COAP_REPLY_STATUS_ERROR;
		return 0;
	}

	/* Reach last block if ret equals to 0 */
	last_block = !coap_next_block(check_response, &context->block_ctx);

	/* Process incoming data */
	payload_offset = response->hdr_len + response->opt_len;
	coap_packet_get_payload(response, &payload_len);
	if (payload_len > 0) {
		LOG_DBG("total: %zd, current: %zd",
			context->block_ctx.total_size,
			context->block_ctx.current);

		/* look up firmware package resource */
		ret = lwm2m_engine_get_resource("5/0/0", &res);
		if (ret < 0) {
			goto error;
		}

		/* get buffer data */
		write_buf = res->res_instances->data_ptr;
		write_buflen = res->res_instances->max_data_len;

		/* check for user override to buffer */
		if (res->pre_write_cb) {
			write_buf = res->pre_write_cb(0, 0, 0, &write_buflen);
		}

		write_cb = context->write_cb;
		if (write_cb) {
			/* flush incoming data to write_cb */
			while (payload_len > 0) {
				len = (payload_len > write_buflen) ?
				       write_buflen : payload_len;
				payload_len -= len;
				/* check for end of packet */
				if (buf_read(write_buf, len,
					     CPKT_BUF_READ(response),
					     &payload_offset) < 0) {
					/* malformed packet */
					ret = -EFAULT;
					goto error;
				}

				ret = write_cb(0, 0, 0,
					       write_buf, len,
					       last_block &&
							(payload_len == 0U),
					       context->block_ctx.total_size);
				if (ret < 0) {
					goto error;
				}
			}
		}
	}

	if (!last_block) {
		/* More block(s) to come, setup next transfer */
		ret = transfer_request(&context->block_ctx, token, tkl,
				       do_firmware_transfer_reply_cb);
		if (ret < 0) {
			goto error;
		}
	} else {
		/* Download finished */
		context->result_cb(0);
		lwm2m_engine_context_close(&context->firmware_ctx);
	}

	return 0;

error:
	context->result_cb(ret);
	lwm2m_engine_context_close(&context->firmware_ctx);
	return ret;
}

static void do_transmit_timeout_cb(struct lwm2m_message *msg)
{
	int ret;

	if (context->retry < PACKET_TRANSFER_RETRY_MAX) {
		/* retry block */
		LOG_WRN("TIMEOUT - Sending a retry packet!");

		ret = transfer_request(&context->block_ctx,
				       msg->token, msg->tkl,
				       do_firmware_transfer_reply_cb);
		if (ret < 0) {
			/* abort retries / transfer */
			context->result_cb(ret);
			context->retry = PACKET_TRANSFER_RETRY_MAX;
			lwm2m_engine_context_close(&context->firmware_ctx);
			return;
		}

		context->retry++;
	} else {
		LOG_ERR("TIMEOUT - Too many retry packet attempts! "
			"Aborting firmware download.");
		context->result_cb(-ENOMSG);
		lwm2m_engine_context_close(&context->firmware_ctx);
	}
}

static void firmware_transfer(struct k_work *work)
{
	int ret;
	char *server_addr;

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	server_addr = CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_ADDR;
	if (strlen(server_addr) >= URI_LEN) {
		LOG_ERR("Invalid Proxy URI: %s", log_strdup(server_addr));
		ret = -ENOTSUP;
		goto error;
	}

	/* Copy required as it gets modified when port is available */
	strcpy(proxy_uri, server_addr);
	server_addr = proxy_uri;
#else
	server_addr = context->uri;
#endif

	ret = lwm2m_parse_peerinfo(server_addr, &context->firmware_ctx.remote_addr,
				   &context->firmware_ctx.use_dtls);
	if (ret < 0) {
		LOG_ERR("Failed to parse server URI.");
		goto error;
	}

	lwm2m_engine_context_init(&context->firmware_ctx);
	ret = lwm2m_socket_start(&context->firmware_ctx);
	if (ret < 0) {
		LOG_ERR("Cannot start a firmware-pull connection:%d", ret);
		goto error;
	}

	LOG_INF("Connecting to server %s", log_strdup(context->uri));

	/* reset block transfer context */
	coap_block_transfer_init(&context->block_ctx,
				 lwm2m_default_block_size(), 0);
	ret = transfer_request(&context->block_ctx, coap_next_token(), 8,
			       do_firmware_transfer_reply_cb);
	if (ret < 0) {
		goto error;
	}

	return;

error:
	context->result_cb(ret);
	lwm2m_engine_context_close(&context->firmware_ctx);
}

int lwm2m_pull_context_start_transfer(struct firmware_pull_context *ctx)
{
	if (!ctx || !ctx->write_cb || !ctx->result_cb) {
		LOG_DBG("Context failed sanity check. Verify initialization!");
		return -EINVAL;
	}

	context = ctx;

	/* close old socket */
	if (ctx->firmware_ctx.sock_fd > -1) {
		lwm2m_engine_context_close(&ctx->firmware_ctx);
	}

	(void)memset(&ctx->firmware_ctx, 0, sizeof(struct lwm2m_ctx));
	ctx->firmware_ctx.sock_fd = -1;
	ctx->retry = 0;

	k_work_init(&ctx->firmware_work, firmware_transfer);
	k_work_submit(&ctx->firmware_work);

	return 0;
}
