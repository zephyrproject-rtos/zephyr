/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017 Open Source Foundries Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2015, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Original authors:
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Joel Hoglund <joel@sics.se>
 */

#define SYS_LOG_DOMAIN "lib/lwm2m_rd_client"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <init.h>
#include <misc/printk.h>
#include <net/net_pkt.h>
#include <net/coap.h>
#include <net/lwm2m.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define LWM2M_RD_CLIENT_URI "rd"

#define SECONDS_TO_UPDATE_EARLY	2
#define STATE_MACHINE_UPDATE_INTERVAL K_MSEC(500)

#define LWM2M_PEER_PORT		CONFIG_LWM2M_PEER_PORT
#define LWM2M_BOOTSTRAP_PORT	CONFIG_LWM2M_BOOTSTRAP_PORT

/* Leave room for 32 hexadeciaml digits (UUID) + NULL */
#define CLIENT_EP_LEN		33

/* The states for the RD client state machine */
/*
 * When node is unregistered it ends up in UNREGISTERED
 * and this is going to be there until use X or Y kicks it
 * back into INIT again
 */
enum sm_engine_state {
	ENGINE_INIT,
	ENGINE_DO_BOOTSTRAP,
	ENGINE_BOOTSTRAP_SENT,
	ENGINE_BOOTSTRAP_DONE,
	ENGINE_DO_REGISTRATION,
	ENGINE_REGISTRATION_SENT,
	ENGINE_REGISTRATION_DONE,
	ENGINE_UPDATE_SENT,
	ENGINE_DEREGISTER,
	ENGINE_DEREGISTER_SENT,
	ENGINE_DEREGISTER_FAILED,
	ENGINE_DEREGISTERED
};

struct lwm2m_rd_client_info {
	u16_t lifetime;
	struct lwm2m_ctx *ctx;
	u8_t engine_state;
	u8_t use_bootstrap;
	u8_t has_bs_server_info;
	u8_t use_registration;
	u8_t has_registration_info;
	u8_t bootstrapped; /* bootstrap done */
	u8_t trigger_update;

	s64_t last_update;

	char ep_name[CLIENT_EP_LEN];
	char server_ep[CLIENT_EP_LEN];

	lwm2m_ctx_event_cb_t event_cb;
};

/* buffers */
static char query_buffer[64]; /* allocate some data for queries and updates */
static u8_t client_data[256]; /* allocate some data for the RD */

#define CLIENT_INSTANCE_COUNT	CONFIG_LWM2M_RD_CLIENT_INSTANCE_COUNT
static struct lwm2m_rd_client_info clients[CLIENT_INSTANCE_COUNT];
static int client_count;

static void set_sm_state(int index, u8_t sm_state)
{
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_NONE;

	/* Determine if a callback to the app is needed */
	if (sm_state == ENGINE_BOOTSTRAP_DONE) {
		event = LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_COMPLETE;
	} else if (clients[index].engine_state == ENGINE_UPDATE_SENT &&
		   sm_state == ENGINE_REGISTRATION_DONE) {
		event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE;
	} else if (sm_state == ENGINE_REGISTRATION_DONE) {
		event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE;
	} else if ((sm_state == ENGINE_INIT ||
		    sm_state == ENGINE_DEREGISTERED) &&
		   (clients[index].engine_state > ENGINE_BOOTSTRAP_DONE &&
		    clients[index].engine_state < ENGINE_DEREGISTER)) {
		event = LWM2M_RD_CLIENT_EVENT_DISCONNECT;
	}

	/* TODO: add locking? */
	clients[index].engine_state = sm_state;

	if (event > LWM2M_RD_CLIENT_EVENT_NONE && clients[index].event_cb) {
		clients[index].event_cb(clients[index].ctx, event);
	}
}

static bool sm_is_registered(int index)
{
	return (clients[index].engine_state >= ENGINE_REGISTRATION_DONE &&
		clients[index].engine_state <= ENGINE_DEREGISTER_FAILED);
}

static u8_t get_sm_state(int index)
{
	/* TODO: add locking? */
	return clients[index].engine_state;
}

static int find_clients_index(const struct sockaddr *addr)
{
	int index = -1, i;
	struct sockaddr *remote = NULL;
	struct net_app_ctx *net_app_ctx;

	for (i = 0; i < client_count; i++) {
		if (!clients[i].ctx) {
			continue;
		}

		net_app_ctx = &clients[i].ctx->net_app_ctx;

#if defined(CONFIG_NET_APP_DTLS)
		if (net_app_ctx->dtls.ctx) {
			remote = &net_app_ctx->dtls.ctx->remote;
		}
#endif

		if (!remote) {
			remote = &net_app_ctx->default_ctx->remote;
		}

		if (remote->sa_family != addr->sa_family) {
			continue;
		}

#if defined(CONFIG_NET_IPV6)
		if (remote->sa_family == AF_INET6 &&
		    net_ipv6_addr_cmp(&net_sin6(remote)->sin6_addr,
				      &net_sin6(addr)->sin6_addr) &&
		    net_sin6(remote)->sin6_port ==
				net_sin6(addr)->sin6_port) {
			index = i;
			break;
		}
#endif

#if defined(CONFIG_NET_IPV4)
		if (remote->sa_family == AF_INET &&
		    net_ipv4_addr_cmp(&net_sin(remote)->sin_addr,
				      &net_sin(addr)->sin_addr) &&
		    net_sin(remote)->sin_port ==
				net_sin(addr)->sin_port) {
			index = i;
			break;
		}
#endif
	}

	return index;
}

static int find_rd_client_from_msg(struct lwm2m_message *msg,
				   struct lwm2m_rd_client_info *rd_clients,
				   size_t len)
{
	size_t i;

	if (!msg || !rd_clients) {
		return -1;
	}

	for (i = 0; i < len; i++) {
		if (rd_clients[i].ctx && rd_clients[i].ctx == msg->ctx) {
			return i;
		}
	}

	return -1;
}

static void sm_handle_timeout_state(struct lwm2m_message *msg,
				    enum sm_engine_state sm_state)
{
	int index;
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_NONE;

	index = find_rd_client_from_msg(msg, clients, CLIENT_INSTANCE_COUNT);
	if (index < 0) {
		SYS_LOG_ERR("Can't find RD client from msg: %p!", msg);
		return;
	}

	if (clients[index].engine_state == ENGINE_BOOTSTRAP_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_FAILURE;
	} else if (clients[index].engine_state == ENGINE_REGISTRATION_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE;
	} else if (clients[index].engine_state == ENGINE_UPDATE_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE;
	} else if (clients[index].engine_state == ENGINE_DEREGISTER_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE;
	} else {
		/* TODO: unknown timeout state */
	}

	set_sm_state(index, sm_state);

	if (event > LWM2M_RD_CLIENT_EVENT_NONE && clients[index].event_cb) {
		clients[index].event_cb(clients[index].ctx, event);
	}
}

/* force re-update with remote peer(s) */
void engine_trigger_update(void)
{
	int index;

	for (index = 0; index < client_count; index++) {
		/* TODO: add locking? */
		clients[index].trigger_update = 1;
	}
}

/* state machine reply callbacks */

static int do_bootstrap_reply_cb(const struct coap_packet *response,
				 struct coap_reply *reply,
				 const struct sockaddr *from)
{
	u8_t code;
	int index;

	code = coap_header_get_code(response);
	SYS_LOG_DBG("Bootstrap callback (code:%u.%u)",
		    COAP_RESPONSE_CODE_CLASS(code),
		    COAP_RESPONSE_CODE_DETAIL(code));

	index = find_clients_index(from);
	if (index < 0) {
		SYS_LOG_ERR("Bootstrap client index not found.");
		return 0;
	}

	if (code == COAP_RESPONSE_CODE_CHANGED) {
		SYS_LOG_DBG("Considered done!");
		set_sm_state(index, ENGINE_BOOTSTRAP_DONE);
	} else if (code == COAP_RESPONSE_CODE_NOT_FOUND) {
		SYS_LOG_ERR("Failed: NOT_FOUND.  Not Retrying.");
		set_sm_state(index, ENGINE_DO_REGISTRATION);
	} else if (code == COAP_RESPONSE_CODE_FORBIDDEN) {
		SYS_LOG_ERR("Failed: 4.03 - Forbidden.  Not Retrying.");
		set_sm_state(index, ENGINE_DO_REGISTRATION);
	} else {
		/* TODO: Read payload for error message? */
		SYS_LOG_ERR("Failed with code %u.%u. Retrying ...",
			    COAP_RESPONSE_CODE_CLASS(code),
			    COAP_RESPONSE_CODE_DETAIL(code));
		set_sm_state(index, ENGINE_INIT);
	}

	return 0;
}

static void do_bootstrap_timeout_cb(struct lwm2m_message *msg)
{
	SYS_LOG_WRN("Bootstrap Timeout");

	/* Restart from scratch */
	sm_handle_timeout_state(msg, ENGINE_INIT);
}

static int do_registration_reply_cb(const struct coap_packet *response,
				    struct coap_reply *reply,
				    const struct sockaddr *from)
{
	struct coap_option options[2];
	u8_t code;
	int ret, index;

	code = coap_header_get_code(response);
	SYS_LOG_DBG("Registration callback (code:%u.%u)",
		    COAP_RESPONSE_CODE_CLASS(code),
		    COAP_RESPONSE_CODE_DETAIL(code));

	index = find_clients_index(from);
	if (index < 0) {
		SYS_LOG_ERR("Registration client index not found.");
		return 0;
	}

	/* check state and possibly set registration to done */
	if (code == COAP_RESPONSE_CODE_CREATED) {
		ret = coap_find_options(response, COAP_OPTION_LOCATION_PATH,
					options, 2);
		if (ret < 0) {
			return ret;
		}

		if (ret < 2) {
			SYS_LOG_ERR("Unexpected endpoint data returned.");
			return -EINVAL;
		}

		/* option[0] should be "rd" */

		if (options[1].len + 1 > sizeof(clients[index].server_ep)) {
			SYS_LOG_ERR("Unexpected length of query: "
				    "%u (expected %zu)\n",
				    options[1].len,
				    sizeof(clients[index].server_ep));
			return -EINVAL;
		}

		memcpy(clients[index].server_ep, options[1].value,
		       options[1].len);
		clients[index].server_ep[options[1].len] = '\0';
		set_sm_state(index, ENGINE_REGISTRATION_DONE);
		SYS_LOG_INF("Registration Done (EP='%s')",
			    clients[index].server_ep);

		return 0;
	} else if (code == COAP_RESPONSE_CODE_NOT_FOUND) {
		SYS_LOG_ERR("Failed: NOT_FOUND.  Not Retrying.");
		set_sm_state(index, ENGINE_REGISTRATION_DONE);
		return 0;
	} else if (code == COAP_RESPONSE_CODE_FORBIDDEN) {
		SYS_LOG_ERR("Failed: 4.03 - Forbidden.  Not Retrying.");
		set_sm_state(index, ENGINE_REGISTRATION_DONE);
		return 0;
	}

	/* TODO: Read payload for error message? */
	/* Possible error response codes: 4.00 Bad request */
	SYS_LOG_ERR("failed with code %u.%u. Re-init network",
		    COAP_RESPONSE_CODE_CLASS(code),
		    COAP_RESPONSE_CODE_DETAIL(code));
	set_sm_state(index, ENGINE_INIT);
	return 0;
}

static void do_registration_timeout_cb(struct lwm2m_message *msg)
{
	SYS_LOG_WRN("Registration Timeout");

	/* Restart from scratch */
	sm_handle_timeout_state(msg, ENGINE_INIT);
}

static int do_update_reply_cb(const struct coap_packet *response,
			      struct coap_reply *reply,
			      const struct sockaddr *from)
{
	u8_t code;
	int index;

	code = coap_header_get_code(response);
	SYS_LOG_INF("Update callback (code:%u.%u)",
		    COAP_RESPONSE_CODE_CLASS(code),
		    COAP_RESPONSE_CODE_DETAIL(code));

	index = find_clients_index(from);
	if (index < 0) {
		SYS_LOG_ERR("Registration client index not found.");
		return 0;
	}

	/* If NOT_FOUND just continue on */
	if ((code == COAP_RESPONSE_CODE_CHANGED) ||
	    (code == COAP_RESPONSE_CODE_CREATED)) {
		set_sm_state(index, ENGINE_REGISTRATION_DONE);
		SYS_LOG_INF("Update Done");
		return 0;
	}

	/* TODO: Read payload for error message? */
	/* Possible error response codes: 4.00 Bad request & 4.04 Not Found */
	SYS_LOG_ERR("Failed with code %u.%u. Retrying registration",
		    COAP_RESPONSE_CODE_CLASS(code),
		    COAP_RESPONSE_CODE_DETAIL(code));
	set_sm_state(index, ENGINE_DO_REGISTRATION);

	return 0;
}

static void do_update_timeout_cb(struct lwm2m_message *msg)
{
	SYS_LOG_WRN("Registration Update Timeout");

	/* Re-do registration */
	sm_handle_timeout_state(msg, ENGINE_DO_REGISTRATION);
}

static int do_deregister_reply_cb(const struct coap_packet *response,
				  struct coap_reply *reply,
				  const struct sockaddr *from)
{
	u8_t code;
	int index;

	code = coap_header_get_code(response);
	SYS_LOG_DBG("Deregister callback (code:%u.%u)",
		    COAP_RESPONSE_CODE_CLASS(code),
		    COAP_RESPONSE_CODE_DETAIL(code));

	index = find_clients_index(from);
	if (index < 0) {
		SYS_LOG_ERR("Registration clients index not found.");
		return 0;
	}

	if (code == COAP_RESPONSE_CODE_DELETED) {
		SYS_LOG_DBG("Deregistration success");
		set_sm_state(index, ENGINE_DEREGISTERED);
	} else {
		SYS_LOG_ERR("failed with code %u.%u",
			    COAP_RESPONSE_CODE_CLASS(code),
			    COAP_RESPONSE_CODE_DETAIL(code));
		if (get_sm_state(index) == ENGINE_DEREGISTER_SENT) {
			set_sm_state(index, ENGINE_DEREGISTER_FAILED);
		}
	}

	return 0;
}

static void do_deregister_timeout_cb(struct lwm2m_message *msg)
{
	SYS_LOG_WRN("De-Registration Timeout");

	/* Abort de-registration and start from scratch */
	sm_handle_timeout_state(msg, ENGINE_INIT);
}

/* state machine step functions */

static int sm_do_init(int index)
{
	SYS_LOG_INF("RD Client started with endpoint "
		    "'%s' and client lifetime %d",
		    clients[index].ep_name,
		    clients[index].lifetime);
	/* Zephyr has joined network already */
	clients[index].has_registration_info = 1;
	clients[index].bootstrapped = 0;
	clients[index].trigger_update = 0;
#if defined(CONFIG_LWM2M_BOOTSTRAP_SERVER)
	clients[index].use_bootstrap = 1;
#else
	clients[index].use_registration = 1;
#endif
	if (clients[index].lifetime == 0) {
		clients[index].lifetime = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
	}
	/* Do bootstrap or registration */
	if (clients[index].use_bootstrap) {
		set_sm_state(index, ENGINE_DO_BOOTSTRAP);
	} else {
		set_sm_state(index, ENGINE_DO_REGISTRATION);
	}

	return 0;
}

static int sm_do_bootstrap(int index)
{
	struct lwm2m_message *msg;
	struct net_app_ctx *app_ctx = NULL;
	int ret;
	struct sockaddr *remote = NULL;

	if (clients[index].use_bootstrap &&
	    clients[index].bootstrapped == 0 &&
	    clients[index].has_bs_server_info) {
		app_ctx = &clients[index].ctx->net_app_ctx;
		msg = lwm2m_get_message(clients[index].ctx);
		if (!msg) {
			SYS_LOG_ERR("Unable to get a lwm2m message!");
			return -ENOMEM;
		}

		msg->type = COAP_TYPE_CON;
		msg->code = COAP_METHOD_POST;
		msg->mid = 0;
		msg->reply_cb = do_bootstrap_reply_cb;
		msg->message_timeout_cb = do_bootstrap_timeout_cb;

		ret = lwm2m_init_message(msg);
		if (ret) {
			goto cleanup;
		}

		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					  "bs", strlen("bs"));

		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "ep=%s", clients[index].ep_name);
		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
					  query_buffer, strlen(query_buffer));

		/* log the bootstrap attempt */
#if defined(CONFIG_NET_APP_DTLS)
		if (app_ctx->dtls.ctx) {
			remote = &app_ctx->dtls.ctx->remote;
		}
#endif

		if (!remote) {
			remote = &app_ctx->default_ctx->remote;
		}

		SYS_LOG_DBG("Register ID with bootstrap server [%s] as '%s'",
			    lwm2m_sprint_ip_addr(remote),
			    query_buffer);

		ret = lwm2m_send_message(msg);
		if (ret < 0) {
			SYS_LOG_ERR("Error sending LWM2M packet (err:%d).",
				    ret);
			goto cleanup;
		}

		set_sm_state(index, ENGINE_BOOTSTRAP_SENT);
	}
	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static int sm_bootstrap_done(int index)
{
	/* TODO: Fix this */
	/* check that we should still use bootstrap */
	if (clients[index].use_bootstrap) {
#ifdef CONFIG_LWM2M_SECURITY_OBJ_SUPPORT
		int i;

		SYS_LOG_DBG("*** Bootstrap - checking for server info ...");

		/* get the server URI */
		if (sec_data->server_uri_len > 0) {
			/* TODO: Write endpoint parsing function */
#if 0
			if (!parse_endpoint(sec_data->server_uri,
					    sec_data->server_uri_len,
					    &clients[index].reg_server)) {
#else
			if (true) {
#endif
				SYS_LOG_ERR("Failed to parse URI!");
			} else {
				clients[index].has_registration_info = 1;
				clients[index].bootstrapped++;
			}
		} else {
			SYS_LOG_ERR("** failed to parse URI");
		}

		/* if we did not register above - then fail this and restart */
		if (clients[index].bootstrapped == 0) {
			/* Not ready - Retry with the bootstrap server again */
			set_sm_state(index, ENGINE_DO_BOOTSTRAP);
		} else {
			set_sm_state(index, ENGINE_DO_REGISTRATION);
		}
	} else {
#endif
		set_sm_state(index, ENGINE_DO_REGISTRATION);
	}

	return 0;
}

static int sm_send_registration(int index, bool send_obj_support_data,
				coap_reply_t reply_cb,
				lwm2m_message_timeout_cb_t timeout_cb)
{
	struct net_app_ctx *app_ctx = NULL;
	struct lwm2m_message *msg;
	u16_t client_data_len;
	int ret;
	struct sockaddr *remote = NULL;

	app_ctx = &clients[index].ctx->net_app_ctx;
	msg = lwm2m_get_message(clients[index].ctx);
	if (!msg) {
		SYS_LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	/* remember the last reg time */
	clients[index].last_update = k_uptime_get();

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_POST;
	msg->mid = 0;
	msg->reply_cb = reply_cb;
	msg->message_timeout_cb = timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	/* TODO: handle return error */
	coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
				  LWM2M_RD_CLIENT_URI,
				  strlen(LWM2M_RD_CLIENT_URI));

	if (!sm_is_registered(index)) {
		/* include client endpoint in URI QUERY on 1st registration */
		coap_append_option_int(&msg->cpkt, COAP_OPTION_CONTENT_FORMAT,
				       LWM2M_FORMAT_APP_LINK_FORMAT);
		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "lwm2m=%s", LWM2M_PROTOCOL_VERSION);
		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
					  query_buffer, strlen(query_buffer));
		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "ep=%s", clients[index].ep_name);
		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
					  query_buffer, strlen(query_buffer));
	} else {
		/* include server endpoint in URI PATH otherwise */
		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					  clients[index].server_ep,
					  strlen(clients[index].server_ep));
	}

	snprintk(query_buffer, sizeof(query_buffer) - 1,
		 "lt=%d", clients[index].lifetime);
	/* TODO: handle return error */
	coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
				  query_buffer, strlen(query_buffer));

	/* TODO: add supported binding query string */

	if (send_obj_support_data) {
		ret = coap_packet_append_payload_marker(&msg->cpkt);
		if (ret < 0) {
			goto cleanup;
		}

		/* generate the rd data */
		client_data_len = lwm2m_get_rd_data(client_data,
						    sizeof(client_data));

		if (!net_pkt_append_all(msg->cpkt.pkt, client_data_len,
					client_data, BUF_ALLOC_TIMEOUT)) {
			ret = -ENOMEM;
			goto cleanup;
		}
	}

	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		SYS_LOG_ERR("Error sending LWM2M packet (err:%d).",
			    ret);
		goto cleanup;
	}

	/* log the registration attempt */
#if defined(CONFIG_NET_APP_DTLS)
	if (app_ctx->dtls.ctx) {
		remote = &app_ctx->dtls.ctx->remote;
	}
#endif

	if (!remote) {
		remote = &app_ctx->default_ctx->remote;
	}

	SYS_LOG_DBG("registration sent [%s]",
		    lwm2m_sprint_ip_addr(remote));

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static int sm_do_registration(int index)
{
	int ret = 0;

	if (clients[index].use_registration &&
	    !sm_is_registered(index) &&
	    clients[index].has_registration_info) {
		ret = sm_send_registration(index, true,
					   do_registration_reply_cb,
					   do_registration_timeout_cb);
		if (!ret) {
			set_sm_state(index, ENGINE_REGISTRATION_SENT);
		} else {
			SYS_LOG_ERR("Registration err: %d", ret);
		}
	}

	return ret;
}

static int sm_registration_done(int index)
{
	int ret = 0;
	bool forced_update;

	/* check for lifetime seconds - 1 so that we can update early */
	if (sm_is_registered(index) &&
	    (clients[index].trigger_update ||
	     ((clients[index].lifetime - SECONDS_TO_UPDATE_EARLY) <=
	      (k_uptime_get() - clients[index].last_update) / 1000))) {
		forced_update = clients[index].trigger_update;
		clients[index].trigger_update = 0;
		ret = sm_send_registration(index, forced_update,
					   do_update_reply_cb,
					   do_update_timeout_cb);
		if (!ret) {
			set_sm_state(index, ENGINE_UPDATE_SENT);
		} else {
			SYS_LOG_ERR("Registration update err: %d", ret);
		}
	}

	return ret;
}

static int sm_do_deregister(int index)
{
	struct net_app_ctx *app_ctx = NULL;
	struct lwm2m_message *msg;
	int ret;

	app_ctx = &clients[index].ctx->net_app_ctx;
	msg = lwm2m_get_message(clients[index].ctx);
	if (!msg) {
		SYS_LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_DELETE;
	msg->mid = 0;
	msg->reply_cb = do_deregister_reply_cb;
	msg->message_timeout_cb = do_deregister_timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	/* TODO: handle return error */
	coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
				  clients[index].server_ep,
				  strlen(clients[index].server_ep));

	SYS_LOG_INF("Deregister from '%s'", clients[index].server_ep);

	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		SYS_LOG_ERR("Error sending LWM2M packet (err:%d).",
			    ret);
		goto cleanup;
	}

	set_sm_state(index, ENGINE_DEREGISTER_SENT);
	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static void lwm2m_rd_client_service(void)
{
	int index;

	for (index = 0; index < client_count; index++) {
		switch (get_sm_state(index)) {

		case ENGINE_INIT:
			sm_do_init(index);
			break;

		case ENGINE_DO_BOOTSTRAP:
			sm_do_bootstrap(index);
			break;

		case ENGINE_BOOTSTRAP_SENT:
			/* wait for bootstrap to be done or timeout */
			break;

		case ENGINE_BOOTSTRAP_DONE:
			sm_bootstrap_done(index);
			break;

		case ENGINE_DO_REGISTRATION:
			sm_do_registration(index);
			break;

		case ENGINE_REGISTRATION_SENT:
			/* wait registration to be done or timeout */
			break;

		case ENGINE_REGISTRATION_DONE:
			sm_registration_done(index);
			break;

		case ENGINE_UPDATE_SENT:
			/* wait update to be done or abort */
			break;

		case ENGINE_DEREGISTER:
			sm_do_deregister(index);
			break;

		case ENGINE_DEREGISTER_SENT:
			/* wait for deregister to be done or reset */
			break;

		case ENGINE_DEREGISTER_FAILED:
			break;

		case ENGINE_DEREGISTERED:
			break;

		default:
			SYS_LOG_ERR("Unhandled state: %d",
				    get_sm_state(index));

		}
	}
}

int lwm2m_rd_client_start(struct lwm2m_ctx *client_ctx,
			  char *peer_str, u16_t peer_port,
			  const char *ep_name,
			  lwm2m_ctx_event_cb_t event_cb)
{
	int index, ret = 0;

	if (client_count + 1 > CLIENT_INSTANCE_COUNT) {
		return -ENOMEM;
	}

	ret = lwm2m_engine_start(client_ctx, peer_str, peer_port);
	if (ret < 0) {
		SYS_LOG_ERR("Cannot init LWM2M engine (%d)", ret);
		goto cleanup;
	}

	if (!client_ctx->net_app_ctx.default_ctx) {
		SYS_LOG_ERR("Default net_app_ctx not selected!");
		return -EINVAL;
	}

	/* TODO: use server URI data from security */
	index = client_count;
	client_count++;
	clients[index].ctx = client_ctx;
	clients[index].event_cb = event_cb;
	set_sm_state(index, ENGINE_INIT);
	strncpy(clients[index].ep_name, ep_name, CLIENT_EP_LEN - 1);
	SYS_LOG_INF("LWM2M Client: %s", clients[index].ep_name);

	return 0;

cleanup:
	net_app_close(&client_ctx->net_app_ctx);
	net_app_release(&client_ctx->net_app_ctx);
	return ret;
}

static int lwm2m_rd_client_init(struct device *dev)
{
	return lwm2m_engine_add_service(lwm2m_rd_client_service,
					STATE_MACHINE_UPDATE_INTERVAL);
}

SYS_INIT(lwm2m_rd_client_init, APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
