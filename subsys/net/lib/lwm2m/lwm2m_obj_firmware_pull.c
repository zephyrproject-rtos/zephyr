/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lwm2m_obj_firmware_pull"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <net/http_parser.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define URI_LEN				255

#define NETWORK_INIT_TIMEOUT		K_SECONDS(10)
#define NETWORK_CONNECT_TIMEOUT		K_SECONDS(10)
#define PACKET_TRANSFER_RETRY_MAX	3

static struct k_work firmware_work;
static struct http_parser_url parsed_uri;
static struct lwm2m_ctx firmware_ctx;
static struct coap_block_context firmware_block_ctx;

static char firmware_uri[URI_LEN];
static int firmware_retry;

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
#define COAP2COAP_PROXY_URI_PATH	"coap2coap"
#define COAP2HTTP_PROXY_URI_PATH	"coap2http"

static char proxy_uri[URI_LEN];
#endif /* CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT */

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
	u16_t off;
	u16_t len;
	char *cursor;
#if !defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	int i;
	int path_len;
#else
	char *uri_path;
#endif

	msg = lwm2m_get_message(&firmware_ctx);
	if (!msg) {
		SYS_LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_GET;
	msg->mid = 0;
	msg->token = token;
	msg->tkl = tkl;
	msg->reply_cb = reply_cb;
	msg->message_timeout_cb = do_transmit_timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret < 0) {
		SYS_LOG_ERR("Error setting up lwm2m message");
		goto cleanup;
	}

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	/* if path is not available, off/len will be zero */
	off = parsed_uri.field_data[UF_SCHEMA].off;
	len = parsed_uri.field_data[UF_SCHEMA].len;
	cursor = firmware_uri + off;

	/* TODO: convert to lower case */
	if (len < 4 || len > 5) {
		ret = -EPROTONOSUPPORT;
		SYS_LOG_ERR("Unsupported schema");
		goto cleanup;
	}

	if (strncmp(cursor, (len == 4 ? "http" : "https"), len) == 0) {
		uri_path = COAP2HTTP_PROXY_URI_PATH;
	} else if (strncmp(cursor, (len == 4 ? "coap" : "coaps"), len) == 0) {
		uri_path = COAP2COAP_PROXY_URI_PATH;
	} else {
		ret = -EPROTONOSUPPORT;
		SYS_LOG_ERR("Unsupported schema");
		goto cleanup;
	}

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					uri_path, strlen(uri_path));
	if (ret < 0) {
		SYS_LOG_ERR("Error adding URI_PATH '%s'", uri_path);
		goto cleanup;
	}
#else
	/* if path is not available, off/len will be zero */
	off = parsed_uri.field_data[UF_PATH].off;
	len = parsed_uri.field_data[UF_PATH].len;
	cursor = firmware_uri + off;
	path_len = 0;

	for (i = 0; i < len; i++) {
		if (firmware_uri[off + i] == '/') {
			if (path_len > 0) {
				ret = coap_packet_append_option(&msg->cpkt,
						      COAP_OPTION_URI_PATH,
						      cursor, path_len);
				if (ret < 0) {
					SYS_LOG_ERR("Error adding URI_PATH");
					goto cleanup;
				}

				cursor += path_len + 1;
				path_len = 0;
			} else {
				/* skip current slash */
				cursor += 1;
			}
			continue;
		}

		if (i == len - 1) {
			/* flush the rest */
			ret = coap_packet_append_option(&msg->cpkt,
							COAP_OPTION_URI_PATH,
							cursor, path_len + 1);
			if (ret < 0) {
				SYS_LOG_ERR("Error adding URI_PATH");
				goto cleanup;
			}
			break;
		}
		path_len += 1;
	}
#endif

	ret = coap_append_block2_option(&msg->cpkt, ctx);
	if (ret < 0) {
		SYS_LOG_ERR("Unable to add block2 option.");
		goto cleanup;
	}

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_PROXY_URI,
					firmware_uri, strlen(firmware_uri));
	if (ret < 0) {
		SYS_LOG_ERR("Error adding PROXY_URI '%s'", firmware_uri);
		goto cleanup;
	}
#else
	/* Ask the server to provide a size estimate */
	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_SIZE2, 0);
	if (ret < 0) {
		SYS_LOG_ERR("Unable to add size2 option.");
		goto cleanup;
	}
#endif

	/* send request */
	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		SYS_LOG_ERR("Error sending LWM2M packet (err:%d).", ret);
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
		SYS_LOG_ERR("Unable to get a lwm2m message!");
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
		SYS_LOG_ERR("Error sending LWM2M packet (err:%d).",
			    ret);
		goto cleanup;
	}

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static int
do_firmware_transfer_reply_cb(struct coap_packet *response,
			      struct coap_reply *reply,
			      const struct sockaddr *from)
{
	int ret;
	bool last_block;
	u8_t token[8];
	u8_t tkl;
	u16_t payload_len, payload_offset, len;
	struct coap_packet *check_response = (struct coap_packet *)response;
	struct lwm2m_engine_res_inst *res = NULL;
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
			SYS_LOG_ERR("Error transmitting ACK");
			goto error;
		}
	}

	/* Check response code from server. Expecting (2.05) */
	resp_code = coap_header_get_code(check_response);
	if (resp_code != COAP_RESPONSE_CODE_CONTENT) {
		SYS_LOG_ERR("Unexpected response from server: %d.%d",
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
		SYS_LOG_ERR("Error from block update: %d", ret);
		ret = -EFAULT;
		goto error;
	}

	/* test for duplicate transfer */
	if (firmware_block_ctx.current < received_block_ctx.current) {
		SYS_LOG_WRN("Duplicate packet ignored");

		/* restore main firmware block context */
		memcpy(&firmware_block_ctx, &received_block_ctx,
		       sizeof(firmware_block_ctx));

		/* set reply->user_data to error to avoid releasing */
		reply->user_data = (void *)COAP_REPLY_STATUS_ERROR;
		return 0;
	}

	/* Reach last block if ret equals to 0 */
	last_block = !coap_next_block(check_response, &firmware_block_ctx);

	/* setup incoming data */
	payload_offset = check_response->hdr_len + check_response->opt_len;
	coap_packet_get_payload(check_response, &payload_len);
	if (payload_len > 0) {
		SYS_LOG_DBG("total: %zd, current: %zd",
			    firmware_block_ctx.total_size,
			    firmware_block_ctx.current);

		/* look up firmware package resource */
		ret = lwm2m_engine_get_resource("5/0/0", &res);
		if (ret < 0) {
			goto error;
		}

		/* get buffer data */
		write_buf = res->data_ptr;
		write_buflen = res->data_len;

		/* check for user override to buffer */
		if (res->pre_write_cb) {
			write_buf = res->pre_write_cb(0, &write_buflen);
		}

		write_cb = lwm2m_firmware_get_write_cb();
		if (write_cb) {
			/* flush incoming data to write_cb */
			while (payload_len > 0) {
				len = (payload_len > write_buflen) ?
				       write_buflen : payload_len;
				payload_len -= len;
				/* check for end of packet */
				if (fbuf_read(&check_response->fbuf,
					      &payload_offset,
					      write_buf, len) < 0) {
					/* malformed packet */
					ret = -EFAULT;
					goto error;
				}

				ret = write_cb(0, write_buf, len, last_block,
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
	u8_t token[8];
	u8_t tkl;

	if (firmware_retry < PACKET_TRANSFER_RETRY_MAX) {
		/* retry block */
		SYS_LOG_WRN("TIMEOUT - Sending a retry packet!");
		tkl = coap_header_get_token(&msg->cpkt, token);

		ret = transfer_request(&firmware_block_ctx, token, tkl,
				       do_firmware_transfer_reply_cb);
		if (ret < 0) {
			/* abort retries / transfer */
			set_update_result_from_error(ret);
			firmware_retry = PACKET_TRANSFER_RETRY_MAX;
			return;
		}

		firmware_retry++;
	} else {
		SYS_LOG_ERR("TIMEOUT - Too many retry packet attempts! "
			    "Aborting firmware download.");
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
	}
}

static void firmware_transfer(struct k_work *work)
{
	char *server_addr;
	int ret, family;
	u16_t off, len;

	/* Server Peer IP information */
	family = AF_INET;

#if defined(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_SUPPORT)
	server_addr = CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_COAP_PROXY_ADDR;
	if (strlen(server_addr) >= URI_LEN) {
		SYS_LOG_ERR("Invalid Proxy URI: %s", server_addr);
		ret = -ENOTSUP;
		goto error;
	}

	/* Copy required as it gets modified when port is available */
	strcpy(proxy_uri, server_addr);
	server_addr = proxy_uri;
#else
	server_addr = firmware_uri;
#endif

	http_parser_url_init(&parsed_uri);
	ret = http_parser_parse_url(server_addr,
				    strlen(server_addr),
				    0,
				    &parsed_uri);
	if (ret != 0) {
		SYS_LOG_ERR("Invalid firmware URI: %s", server_addr);
		ret = -ENOTSUP;
		goto error;
	}

	/* Check schema and only support coap for now */
	if (!(parsed_uri.field_set & (1 << UF_SCHEMA))) {
		SYS_LOG_ERR("No schema in package uri");
		ret = -ENOTSUP;
		goto error;
	}

	/* TODO: enable coaps when DTLS is ready */
	off = parsed_uri.field_data[UF_SCHEMA].off;
	len = parsed_uri.field_data[UF_SCHEMA].len;
	if (len != 4 || memcmp(server_addr + off, "coap", 4)) {
		SYS_LOG_ERR("Unsupported schema");
		ret = -EPROTONOSUPPORT;
		goto error;
	}

	if (!(parsed_uri.field_set & (1 << UF_PORT))) {
		/* Set to default port of CoAP */
		parsed_uri.port = 5683;
	}

	off = parsed_uri.field_data[UF_HOST].off;
	len = parsed_uri.field_data[UF_HOST].len;

	/* truncate host portion */
	server_addr[off + len] = '\0';

	lwm2m_engine_context_init(&firmware_ctx);

	/* setup the local firmware download client port */
	(void)memset(&firmware_ctx.local_addr, 0,
		     sizeof(firmware_ctx.local_addr));
#if defined(CONFIG_NET_IPV6)
	firmware_ctx.local_addr.sa_family = AF_INET6;
	net_sin6(&firmware_ctx.local_addr)->sin6_port =
		htons(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_LOCAL_PORT);
#elif defined(CONFIG_NET_IPV4)
	firmware_ctx.local_addr.sa_family = AF_INET;
	net_sin(&firmware_ctx.local_addr)->sin_port =
		htons(CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_LOCAL_PORT);
#endif

	ret = lwm2m_nl_start(&firmware_ctx, &server_addr[off], parsed_uri.port);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot start a firmware-pull connection:%d", ret);
		goto error;
	}

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
	/* TODO check for existing transfer */

	(void)memset(&firmware_ctx, 0, sizeof(struct lwm2m_ctx));
	firmware_retry = 0;
	firmware_ctx.net_init_timeout = NETWORK_INIT_TIMEOUT;
	firmware_ctx.net_timeout = NETWORK_CONNECT_TIMEOUT;
	k_work_init(&firmware_work, firmware_transfer);
	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);

#if defined(CONFIG_LWM2M_NET_LAYER_NET_APP)
	firmware_ctx.net_layer_api = lwm2m_firmware_pull_nl_net_app_api();
#elif defined(CONFIG_LWM2M_NET_LAYER_SOCKET)
	firmware_ctx.net_layer_api = lwm2m_firmware_pull_nl_socket_api();
#endif

	/* start file transfer work */
	strncpy(firmware_uri, package_uri, URI_LEN - 1);
	k_work_submit(&firmware_work);

	return 0;
}
