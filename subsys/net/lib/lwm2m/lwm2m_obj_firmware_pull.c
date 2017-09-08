/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "lwm2m_obj_firmware_pull"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <net/zoap.h>
#include <net/net_app.h>
#include <net/net_core.h>
#include <net/http_parser.h>
#include <net/net_pkt.h>
#include <net/udp.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define PACKAGE_URI_LEN		255

#define BUF_ALLOC_TIMEOUT	K_SECONDS(1)
#define NETWORK_INIT_TIMEOUT	K_SECONDS(10)
#define NETWORK_CONNECT_TIMEOUT	K_SECONDS(10)

static struct k_work firmware_work;
static char firmware_uri[PACKAGE_URI_LEN];
static struct http_parser_url parsed_uri;
static struct lwm2m_ctx firmware_ctx;
static struct zoap_block_context firmware_block_ctx;

static void
firmware_udp_receive(struct net_app_ctx *app_ctx, struct net_pkt *pkt,
		     int status, void *user_data)
{
	lwm2m_udp_receive(&firmware_ctx, pkt, true, NULL);
}

static void do_transmit_timeout_cb(struct lwm2m_message *msg)
{
	lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
}

static int transfer_request(struct zoap_block_context *ctx,
			    const u8_t *token, u8_t tkl,
			    zoap_reply_t reply_cb)
{
	struct lwm2m_message *msg;
	int ret;
	int i;
	int path_len;
	char *cursor;
	u16_t off;
	u16_t len;

	msg = lwm2m_get_message(&firmware_ctx);
	if (!msg) {
		SYS_LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = ZOAP_TYPE_CON;
	msg->code = ZOAP_METHOD_GET;
	msg->mid = 0;
	msg->token = token;
	msg->tkl = tkl;
	msg->reply_cb = reply_cb;
	msg->message_timeout_cb = do_transmit_timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	/* if path is not available, off/len will be zero */
	off = parsed_uri.field_data[UF_PATH].off;
	len = parsed_uri.field_data[UF_PATH].len;
	cursor = firmware_uri + off;
	path_len = 0;

	for (i = 0; i < len; i++) {
		if (firmware_uri[off + i] == '/') {
			if (path_len > 0) {
				ret = zoap_add_option(&msg->zpkt,
						      ZOAP_OPTION_URI_PATH,
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
			ret = zoap_add_option(&msg->zpkt, ZOAP_OPTION_URI_PATH,
					      cursor, path_len + 1);
			if (ret < 0) {
				SYS_LOG_ERR("Error adding URI_PATH");
				goto cleanup;
			}
			break;
		}
		path_len += 1;
	}

	ret = zoap_add_block2_option(&msg->zpkt, ctx);
	if (ret) {
		SYS_LOG_ERR("Unable to add block2 option.");
		goto cleanup;
	}

	/* Ask the server to provide a size estimate */
	ret = zoap_add_option_int(&msg->zpkt, ZOAP_OPTION_SIZE2, 0);
	if (ret) {
		SYS_LOG_ERR("Unable to add size2 option.");
		goto cleanup;
	}

	/* send request */
	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		SYS_LOG_ERR("Error sending LWM2M packet (err:%d).", ret);
		goto cleanup;
	}

	return 0;

cleanup:
	lwm2m_release_message(msg);

	if (ret == -ENOMEM) {
		lwm2m_firmware_set_update_result(RESULT_OUT_OF_MEM);
	} else {
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
	}

	return ret;
}

static int
do_firmware_transfer_reply_cb(const struct zoap_packet *response,
			      struct zoap_reply *reply,
			      const struct sockaddr *from)
{
	int ret;
	size_t transfer_offset = 0;
	const u8_t *token;
	u8_t tkl;
	u16_t payload_len;
	u8_t *payload;
	struct zoap_packet *check_response = (struct zoap_packet *)response;
	lwm2m_engine_set_data_cb_t callback;
	u8_t resp_code;

	/* Check response code from server. Expecting (2.05) */
	resp_code = zoap_header_get_code(check_response);
	if (resp_code != ZOAP_RESPONSE_CODE_CONTENT) {
		SYS_LOG_ERR("Unexpected response from server: %d.%d",
			    ZOAP_RESPONSE_CODE_CLASS(resp_code),
			    ZOAP_RESPONSE_CODE_DETAIL(resp_code));
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		return -ENOENT;
	}

	ret = zoap_update_from_block(check_response, &firmware_block_ctx);
	if (ret < 0) {
		SYS_LOG_ERR("Error from block update: %d", ret);
		lwm2m_firmware_set_update_result(RESULT_INTEGRITY_FAILED);
		return ret;
	}

	/* Reach last block if transfer_offset equals to 0 */
	transfer_offset = zoap_next_block(check_response, &firmware_block_ctx);

	/* Process incoming data */
	payload = zoap_packet_get_payload(check_response, &payload_len);
	if (payload_len > 0) {
		SYS_LOG_DBG("total: %zd, current: %zd",
			    firmware_block_ctx.total_size,
			    firmware_block_ctx.current);

		callback = lwm2m_firmware_get_write_cb();
		if (callback) {
			ret = callback(0, payload, payload_len,
				       transfer_offset == 0,
				       firmware_block_ctx.total_size);
			if (ret == -ENOMEM) {
				lwm2m_firmware_set_update_result(
						RESULT_OUT_OF_MEM);
				return ret;
			} else if (ret == -ENOSPC) {
				lwm2m_firmware_set_update_result(
						RESULT_NO_STORAGE);
				return ret;
			} else if (ret < 0) {
				lwm2m_firmware_set_update_result(
						RESULT_INTEGRITY_FAILED);
				return ret;
			}
		}
	}

	if (transfer_offset > 0) {
		/* More block(s) to come, setup next transfer */
		token = zoap_header_get_token(check_response, &tkl);
		ret = transfer_request(&firmware_block_ctx, token, tkl,
				       do_firmware_transfer_reply_cb);
	} else {
		/* Download finished */
		lwm2m_firmware_set_update_state(STATE_DOWNLOADED);
	}

	return ret;
}

static void firmware_transfer(struct k_work *work)
{
	int ret, family;
	u16_t off;
	u16_t len;
	char tmp;

	/* Server Peer IP information */
	family = AF_INET;

	http_parser_url_init(&parsed_uri);
	ret = http_parser_parse_url(firmware_uri,
				    strlen(firmware_uri),
				    0,
				    &parsed_uri);
	if (ret != 0) {
		SYS_LOG_ERR("Invalid firmware URI: %s", firmware_uri);
		lwm2m_firmware_set_update_result(RESULT_INVALID_URI);
		return;
	}

	/* Check schema and only support coap for now */
	if (!(parsed_uri.field_set & (1 << UF_SCHEMA))) {
		SYS_LOG_ERR("No schema in package uri");
		lwm2m_firmware_set_update_result(RESULT_INVALID_URI);
		return;
	}

	/* TODO: enable coaps when DTLS is ready */
	off = parsed_uri.field_data[UF_SCHEMA].off;
	len = parsed_uri.field_data[UF_SCHEMA].len;
	if (len != 4 || memcmp(firmware_uri + off, "coap", 4)) {
		SYS_LOG_ERR("Unsupported schema");
		lwm2m_firmware_set_update_result(RESULT_UNSUP_PROTO);
		return;
	}

	if (!(parsed_uri.field_set & (1 << UF_PORT))) {
		/* Set to default port of CoAP */
		parsed_uri.port = 5683;
	}

	off = parsed_uri.field_data[UF_HOST].off;
	len = parsed_uri.field_data[UF_HOST].len;

	/* truncate host portion */
	tmp = firmware_uri[off + len];
	firmware_uri[off + len] = '\0';

	ret = net_app_init_udp_client(&firmware_ctx.net_app_ctx, NULL, NULL,
				      &firmware_uri[off], parsed_uri.port,
				      firmware_ctx.net_init_timeout, NULL);
	firmware_uri[off + len] = tmp;
	if (ret) {
		SYS_LOG_ERR("Could not get an UDP context (err:%d)", ret);
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		return;
	}

	SYS_LOG_DBG("Attached to port: %d", parsed_uri.port);

	lwm2m_engine_context_init(&firmware_ctx);

	/* set net_app callbacks */
	ret = net_app_set_cb(&firmware_ctx.net_app_ctx, NULL,
			     firmware_udp_receive, NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("Could not set receive callback (err:%d)", ret);
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		goto cleanup;
	}

	ret = net_app_connect(&firmware_ctx.net_app_ctx,
			      firmware_ctx.net_timeout);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot connect UDP (%d)", ret);
		goto cleanup;
	}

	/* reset block transfer context */
	zoap_block_transfer_init(&firmware_block_ctx,
				 lwm2m_default_block_size(), 0);
	transfer_request(&firmware_block_ctx, NULL, 0,
			 do_firmware_transfer_reply_cb);
	return;

cleanup:
	net_app_close(&firmware_ctx.net_app_ctx);
	net_app_release(&firmware_ctx.net_app_ctx);
}

/* TODO: */
int lwm2m_firmware_cancel_transfer(void)
{
	return 0;
}

int lwm2m_firmware_start_transfer(char *package_uri)
{
	/* free up old context */
	if (firmware_ctx.net_app_ctx.is_init) {
		net_app_close(&firmware_ctx.net_app_ctx);
		net_app_release(&firmware_ctx.net_app_ctx);
	}

	memset(&firmware_ctx, 0, sizeof(struct lwm2m_ctx));
	firmware_ctx.net_init_timeout = NETWORK_INIT_TIMEOUT;
	firmware_ctx.net_timeout = NETWORK_CONNECT_TIMEOUT;
	k_work_init(&firmware_work, firmware_transfer);
	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);

	/* start file transfer work */
	strncpy(firmware_uri, package_uri, PACKAGE_URI_LEN - 1);
	k_work_submit(&firmware_work);

	return 0;
}
