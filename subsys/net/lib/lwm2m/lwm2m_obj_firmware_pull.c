/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_obj_firmware_pull
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <net/http_parser.h>
#include <net/socket.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define URI_LEN		255

#define NETWORK_INIT_TIMEOUT	K_SECONDS(10)
#define NETWORK_CONNECT_TIMEOUT	K_SECONDS(10)
#define PACKET_TRANSFER_RETRY_MAX	3

static struct k_work firmware_work;
static char firmware_uri[URI_LEN];
static struct lwm2m_ctx firmware_ctx;
static int firmware_retry;
static struct coap_block_context firmware_block_ctx;

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
#define COAP2COAP_PROXY_URI_PATH	"coap2coap"
#define COAP2HTTP_PROXY_URI_PATH	"coap2http"

static char proxy_uri[URI_LEN];
#endif

static void do_transmit_timeout_cb(struct lwm2m_message *msg);

static void set_update_result_from_error(int error_code)
{
	if (error_code == -ENOMEM) {
		lwm2m_firmware_set_update_result(RESULT_OUT_OF_MEM);
	} else if (error_code == -ENOSPC) {
		lwm2m_firmware_set_update_result(RESULT_NO_STORAGE);
	} else if (error_code == -EFAULT) {
		lwm2m_firmware_set_update_result(RESULT_INTEGRITY_FAILED);
	} else if (error_code == -ENOMSG) {
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
	} else if (error_code == -ENOTSUP) {
		lwm2m_firmware_set_update_result(RESULT_INVALID_URI);
	} else if (error_code == -EPROTONOSUPPORT) {
		lwm2m_firmware_set_update_result(RESULT_UNSUP_PROTO);
	} else {
		lwm2m_firmware_set_update_result(RESULT_UPDATE_FAILED);
	}
}

static int transfer_request(struct coap_block_context *ctx,
			    u8_t *token, u8_t tkl,
			    coap_reply_t reply_cb)
{
	struct lwm2m_message *msg;
	int ret;
	char *cursor;
#if !defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	struct http_parser_url parser;
	u16_t off, len;
	char *next_slash;
#endif

	msg = lwm2m_get_message(&firmware_ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_GET;
	msg->mid = 0U;
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
	if (strncmp(firmware_uri, "http", 4) == 0) {
		cursor = COAP2HTTP_PROXY_URI_PATH;
	} else if (strncmp(firmware_uri, "coap", 4) == 0) {
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
	ret = http_parser_parse_url(firmware_uri, strlen(firmware_uri), 0,
				    &parser);
	if (ret < 0) {
		LOG_ERR("Invalid firmware url: %s", log_strdup(firmware_uri));
		ret = -ENOTSUP;
		goto cleanup;
	}

	/* if path is not available, off/len will be zero */
	off = parser.field_data[UF_PATH].off;
	len = parser.field_data[UF_PATH].len;
	cursor = firmware_uri + off;

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
					firmware_uri, strlen(firmware_uri));
	if (ret < 0) {
		LOG_ERR("Error adding PROXY_URI '%s'",
			log_strdup(firmware_uri));
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
	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		LOG_ERR("Error sending LWM2M packet (err:%d).", ret);
		goto cleanup;
	}

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static int transfer_empty_ack(u16_t mid)
{
	struct lwm2m_message *msg;
	int ret;

	msg = lwm2m_get_message(&firmware_ctx);
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

	ret = lwm2m_send_message(msg);
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
	u8_t token[8];
	u8_t tkl;
	u16_t payload_len, payload_offset, len;
	struct coap_packet *check_response = (struct coap_packet *)response;
	struct lwm2m_engine_res *res = NULL;
	lwm2m_engine_set_data_cb_t write_cb;
	size_t write_buflen;
	u8_t resp_code, *write_buf;
	struct coap_block_context received_block_ctx;

	/* token is used to determine a valid ACK vs a separated response */
	tkl = coap_header_get_token(check_response, token);

	/* If separated response (ACK) return and wait for response */
	if (!tkl && coap_header_get_type(response) == COAP_TYPE_ACK) {
		return 0;
	} else if (coap_header_get_type(response) == COAP_TYPE_CON) {
		/* Send back ACK so the server knows we received the pkt */
		ret = transfer_empty_ack(coap_header_get_id(check_response));
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
	memcpy(&received_block_ctx, &firmware_block_ctx,
	       sizeof(firmware_block_ctx));

	ret = coap_update_from_block(check_response, &firmware_block_ctx);
	if (ret < 0) {
		LOG_ERR("Error from block update: %d", ret);
		ret = -EFAULT;
		goto error;
	}

	/* test for duplicate transfer */
	if (firmware_block_ctx.current < received_block_ctx.current) {
		LOG_WRN("Duplicate packet ignored");

		/* restore main firmware block context */
		memcpy(&firmware_block_ctx, &received_block_ctx,
		       sizeof(firmware_block_ctx));

		/* set reply->user_data to error to avoid releasing */
		reply->user_data = (void *)COAP_REPLY_STATUS_ERROR;
		return 0;
	}

	/* Reach last block if ret equals to 0 */
	last_block = !coap_next_block(check_response, &firmware_block_ctx);

	/* Process incoming data */
	payload_offset = response->hdr_len + response->opt_len;
	coap_packet_get_payload(response, &payload_len);
	if (payload_len > 0) {
		LOG_DBG("total: %zd, current: %zd",
			firmware_block_ctx.total_size,
			firmware_block_ctx.current);

		/* look up firmware package resource */
		ret = lwm2m_engine_get_resource("5/0/0", &res);
		if (ret < 0) {
			goto error;
		}

		/* get buffer data */
		write_buf = res->res_instances->data_ptr;
		write_buflen = res->res_instances->data_len;

		/* check for user override to buffer */
		if (res->pre_write_cb) {
			write_buf = res->pre_write_cb(0, 0, 0, &write_buflen);
		}

		write_cb = lwm2m_firmware_get_write_cb();
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
					       firmware_block_ctx.total_size);
				if (ret < 0) {
					goto error;
				}
			}
		}
	}

	if (!last_block) {
		/* More block(s) to come, setup next transfer */
		ret = transfer_request(&firmware_block_ctx, token, tkl,
				       do_firmware_transfer_reply_cb);
		if (ret < 0) {
			goto error;
		}
	} else {
		/* Download finished */
		lwm2m_firmware_set_update_state(STATE_DOWNLOADED);
	}

	return 0;

error:
	set_update_result_from_error(ret);
	return ret;
}

static void do_transmit_timeout_cb(struct lwm2m_message *msg)
{
	int ret;

	if (firmware_retry < PACKET_TRANSFER_RETRY_MAX) {
		/* retry block */
		LOG_WRN("TIMEOUT - Sending a retry packet!");

		ret = transfer_request(&firmware_block_ctx,
				       msg->token, msg->tkl,
				       do_firmware_transfer_reply_cb);
		if (ret < 0) {
			/* abort retries / transfer */
			set_update_result_from_error(ret);
			firmware_retry = PACKET_TRANSFER_RETRY_MAX;
			return;
		}

		firmware_retry++;
	} else {
		LOG_ERR("TIMEOUT - Too many retry packet attempts! "
			"Aborting firmware download.");
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
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
	server_addr = firmware_uri;
#endif

	ret = lwm2m_parse_peerinfo(server_addr, &firmware_ctx.remote_addr,
				   &firmware_ctx.use_dtls);
	if (ret < 0) {
		goto error;
	}

	lwm2m_engine_context_init(&firmware_ctx);
	firmware_ctx.handle_separate_response = true;
	ret = lwm2m_socket_start(&firmware_ctx);
	if (ret < 0) {
		LOG_ERR("Cannot start a firmware-pull connection:%d", ret);
		goto error;
	}

	LOG_INF("Connecting to server %s", log_strdup(firmware_uri));

	/* reset block transfer context */
	coap_block_transfer_init(&firmware_block_ctx,
				 lwm2m_default_block_size(), 0);
	ret = transfer_request(&firmware_block_ctx, coap_next_token(), 8,
			       do_firmware_transfer_reply_cb);
	if (ret < 0) {
		goto error;
	}

	return;

error:
	set_update_result_from_error(ret);
}

/* TODO: */
int lwm2m_firmware_cancel_transfer(void)
{
	return 0;
}

int lwm2m_firmware_start_transfer(char *package_uri)
{
	/* close old socket */
	if (firmware_ctx.sock_fd > 0) {
		lwm2m_socket_del(&firmware_ctx);
		(void)close(firmware_ctx.sock_fd);
	}

	(void)memset(&firmware_ctx, 0, sizeof(struct lwm2m_ctx));
	firmware_retry = 0;
	k_work_init(&firmware_work, firmware_transfer);
	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);

	/* start file transfer work */
	strncpy(firmware_uri, package_uri, URI_LEN - 1);
	k_work_submit(&firmware_work);

	return 0;
}
