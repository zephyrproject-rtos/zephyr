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
#include <net/net_core.h>
#include <net/http_parser.h>
#include <net/net_pkt.h>
#include <net/udp.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define PACKAGE_URI_LEN				255

#define BUF_ALLOC_TIMEOUT K_SECONDS(1)

static struct k_work firmware_work;
static char firmware_uri[PACKAGE_URI_LEN];
static struct sockaddr firmware_addr;
static struct http_parser_url parsed_uri;
static struct net_context *firmware_net_ctx;
static struct k_delayed_work retransmit_work;

#define NUM_PENDINGS	CONFIG_LWM2M_ENGINE_MAX_PENDING
#define NUM_REPLIES	CONFIG_LWM2M_ENGINE_MAX_REPLIES
static struct zoap_pending pendings[NUM_PENDINGS];
static struct zoap_reply replies[NUM_REPLIES];
static struct zoap_block_context firmware_block_ctx;

static void
firmware_udp_receive(struct net_context *ctx, struct net_pkt *pkt, int status,
		     void *user_data)
{
	lwm2m_udp_receive(ctx, pkt, pendings, NUM_PENDINGS,
			  replies, NUM_REPLIES, true, NULL);
}

static void retransmit_request(struct k_work *work)
{
	struct zoap_pending *pending;
	int r;

	pending = zoap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (!pending) {
		return;
	}

	r = lwm2m_udp_sendto(pending->pkt, &pending->addr);
	if (r < 0) {
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		return;
	}

	if (!zoap_pending_cycle(pending)) {
		zoap_pending_clear(pending);
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		return;
	}

	k_delayed_work_submit(&retransmit_work, pending->timeout);
}

static int transfer_request(struct zoap_block_context *ctx,
			    const u8_t *token, u8_t tkl,
			    zoap_reply_t reply_cb)
{
	struct zoap_packet request;
	struct net_pkt *pkt = NULL;
	struct zoap_pending *pending = NULL;
	struct zoap_reply *reply = NULL;
	int ret;
	int i;
	int path_len;
	char *cursor;
	u16_t off;
	u16_t len;

	ret = lwm2m_init_message(firmware_net_ctx, &request, &pkt,
				 ZOAP_TYPE_CON, ZOAP_METHOD_GET,
				 0, token, tkl);
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
				ret = zoap_add_option(&request,
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
			ret = zoap_add_option(&request, ZOAP_OPTION_URI_PATH,
					      cursor, path_len + 1);
			if (ret < 0) {
				SYS_LOG_ERR("Error adding URI_PATH");
				goto cleanup;
			}
			break;
		}

		path_len += 1;
	}

	ret = zoap_add_block2_option(&request, ctx);
	if (ret) {
		SYS_LOG_ERR("Unable to add block2 option.");
		goto cleanup;
	}

	/* Ask the server to provide a size estimate */
	ret = zoap_add_option_int(&request, ZOAP_OPTION_SIZE2, 0);
	if (ret) {
		SYS_LOG_ERR("Unable to add size2 option.");
		goto cleanup;
	}

	pending = lwm2m_init_message_pending(&request, &firmware_addr,
					     pendings, NUM_PENDINGS);
	if (!pending) {
		ret = -ENOMEM;
		goto cleanup;
	}

	/* set the reply handler */
	if (reply_cb) {
		reply = zoap_reply_next_unused(replies, NUM_REPLIES);
		if (!reply) {
			SYS_LOG_ERR("No resources for waiting for replies.");
			ret = -ENOMEM;
			goto cleanup;
		}

		zoap_reply_init(reply, &request);
		reply->reply = reply_cb;
	}

	/* send request */
	ret = lwm2m_udp_sendto(pkt, &firmware_addr);
	if (ret < 0) {
		SYS_LOG_ERR("Error sending LWM2M packet (err:%d).",
			    ret);
		goto cleanup;
	}

	zoap_pending_cycle(pending);
	k_delayed_work_submit(&retransmit_work, pending->timeout);
	return 0;

cleanup:
	lwm2m_init_message_cleanup(pkt, pending, reply);

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
#if defined(CONFIG_NET_IPV6)
	static struct sockaddr_in6 any_addr6 = { .sin6_addr = IN6ADDR_ANY_INIT,
						.sin6_family = AF_INET6 };
#endif
#if defined(CONFIG_NET_IPV4)
	static struct sockaddr_in any_addr4 = { .sin_addr = INADDR_ANY_INIT,
					       .sin_family = AF_INET };
#endif
	struct net_if *iface;
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

	/* IPv6 is wrapped by brackets */
	if (off > 0 && firmware_uri[off - 1] == '[') {
		if (!IS_ENABLED(CONFIG_NET_IPV6)) {
			SYS_LOG_ERR("Doesn't support IPv6");
			lwm2m_firmware_set_update_result(RESULT_UNSUP_PROTO);
			return;
		}

		family = AF_INET6;
	} else {
		/* Distinguish IPv4 or DNS */
		for (int i = off; i < off + len; i++) {
			if (!isdigit(firmware_uri[i]) &&
			    firmware_uri[i] != '.') {
				SYS_LOG_ERR("Doesn't support DNS lookup");
				lwm2m_firmware_set_update_result(
						RESULT_UNSUP_PROTO);
				return;
			}
		}

		if (!IS_ENABLED(CONFIG_NET_IPV4)) {
			SYS_LOG_ERR("Doesn't support IPv4");
			lwm2m_firmware_set_update_result(RESULT_UNSUP_PROTO);
			return;
		}

		family = AF_INET;
	}

	tmp = firmware_uri[off + len];
	firmware_uri[off + len] = '\0';

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		firmware_addr.sa_family = family;
		net_addr_pton(firmware_addr.sa_family, firmware_uri + off,
			      &net_sin6(&firmware_addr)->sin6_addr);
		net_sin6(&firmware_addr)->sin6_port = htons(parsed_uri.port);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		firmware_addr.sa_family = family;
		net_addr_pton(firmware_addr.sa_family, firmware_uri + off,
			      &net_sin(&firmware_addr)->sin_addr);
		net_sin(&firmware_addr)->sin_port = htons(parsed_uri.port);
	}

	/* restore firmware_uri */
	firmware_uri[off + len] = tmp;

	ret = net_context_get(firmware_addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &firmware_net_ctx);
	if (ret) {
		SYS_LOG_ERR("Could not get an UDP context (err:%d)", ret);
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		return;
	}

	iface = net_if_get_default();
	if (!iface) {
		SYS_LOG_ERR("Could not find default interface");
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		return;
	}

#if defined(CONFIG_NET_IPV6)
	if (firmware_addr.sa_family == AF_INET6) {
		ret = net_context_bind(firmware_net_ctx,
				       (struct sockaddr *)&any_addr6,
				       sizeof(any_addr6));
	}
#endif
#if defined(CONFIG_NET_IPV4)
	if (firmware_addr.sa_family == AF_INET) {
		ret = net_context_bind(firmware_net_ctx,
				       (struct sockaddr *)&any_addr4,
				       sizeof(any_addr4));
	}
#endif

	if (ret) {
		NET_ERR("Could not bind the UDP context (err:%d)", ret);
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		goto cleanup;
	}

	SYS_LOG_DBG("Attached to port: %d", parsed_uri.port);
	ret = net_context_recv(firmware_net_ctx, firmware_udp_receive, 0, NULL);
	if (ret) {
		SYS_LOG_ERR("Could not set receive for net context (err:%d)",
			    ret);
		lwm2m_firmware_set_update_result(RESULT_CONNECTION_LOST);
		goto cleanup;
	}

	/* reset block transfer context */
	zoap_block_transfer_init(&firmware_block_ctx,
				 lwm2m_default_block_size(), 0);
	transfer_request(&firmware_block_ctx, NULL, 0,
			 do_firmware_transfer_reply_cb);
	return;

cleanup:
	if (firmware_net_ctx) {
		net_context_put(firmware_net_ctx);
	}
}

/* TODO: */
int lwm2m_firmware_cancel_transfer(void)
{
	return 0;
}

int lwm2m_firmware_start_transfer(char *package_uri)
{
	/* free up old context */
	if (firmware_net_ctx) {
		net_context_put(firmware_net_ctx);
	}

	k_work_init(&firmware_work, firmware_transfer);
	k_delayed_work_init(&retransmit_work, retransmit_request);

	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);

	/* start file transfer work */
	strncpy(firmware_uri, package_uri, PACKAGE_URI_LEN - 1);
	k_work_submit(&firmware_work);

	return 0;
}
