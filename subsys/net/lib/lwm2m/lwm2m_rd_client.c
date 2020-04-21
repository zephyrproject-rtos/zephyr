/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2017-2019 Foundries.io
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

#define LOG_MODULE_NAME net_lwm2m_rd_client
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <init.h>
#include <sys/printk.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

#define LWM2M_RD_CLIENT_URI "rd"

#define SECONDS_TO_UPDATE_EARLY	6
#define STATE_MACHINE_UPDATE_INTERVAL K_MSEC(500)

/* Leave room for 32 hexadeciaml digits (UUID) + NULL */
#define CLIENT_EP_LEN		33

/* Up to 3 characters + NULL */
#define CLIENT_BINDING_LEN sizeof("UQS")

/* The states for the RD client state machine */
/*
 * When node is unregistered it ends up in UNREGISTERED
 * and this is going to be there until use X or Y kicks it
 * back into INIT again
 */
enum sm_engine_state {
	ENGINE_INIT,
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	ENGINE_DO_BOOTSTRAP_REG,
	ENGINE_BOOTSTRAP_REG_SENT,
	ENGINE_BOOTSTRAP_REG_DONE,
	ENGINE_BOOTSTRAP_TRANS_DONE,
#endif
	ENGINE_DO_REGISTRATION,
	ENGINE_REGISTRATION_SENT,
	ENGINE_REGISTRATION_DONE,
	ENGINE_REGISTRATION_DONE_RX_OFF,
	ENGINE_UPDATE_SENT,
	ENGINE_DEREGISTER,
	ENGINE_DEREGISTER_SENT,
	ENGINE_DEREGISTER_FAILED,
	ENGINE_DEREGISTERED
};

struct lwm2m_rd_client_info {
	u32_t lifetime;
	struct lwm2m_ctx *ctx;
	u8_t engine_state;
	u8_t use_bootstrap;
	u8_t trigger_update;

	s64_t last_update;
	s64_t last_tx;

	char ep_name[CLIENT_EP_LEN];
	char server_ep[CLIENT_EP_LEN];

	lwm2m_ctx_event_cb_t event_cb;
} client;

/* buffers */
static char query_buffer[64]; /* allocate some data for queries and updates */
static u8_t client_data[256]; /* allocate some data for the RD */

void engine_update_tx_time(void)
{
	client.last_tx = k_uptime_get();
}

static void set_sm_state(u8_t sm_state)
{
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_NONE;

	/* Determine if a callback to the app is needed */
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	if (sm_state == ENGINE_BOOTSTRAP_REG_DONE) {
		event = LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE;
	} else if (sm_state == ENGINE_BOOTSTRAP_TRANS_DONE) {
		event = LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE;
	} else
#endif
	if (client.engine_state == ENGINE_UPDATE_SENT &&
	    sm_state == ENGINE_REGISTRATION_DONE) {
		event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE;
	} else if (sm_state == ENGINE_REGISTRATION_DONE) {
		event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE;
	} else if (sm_state == ENGINE_REGISTRATION_DONE_RX_OFF) {
		event = LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF;
	} else if ((sm_state == ENGINE_INIT ||
		    sm_state == ENGINE_DEREGISTERED) &&
		   (client.engine_state >= ENGINE_DO_REGISTRATION &&
		    client.engine_state <= ENGINE_DEREGISTER_SENT)) {
		event = LWM2M_RD_CLIENT_EVENT_DISCONNECT;
	}

	/* TODO: add locking? */
	client.engine_state = sm_state;

	if (event > LWM2M_RD_CLIENT_EVENT_NONE && client.event_cb) {
		client.event_cb(client.ctx, event);
	}
}

static bool sm_is_registered(void)
{
	return (client.engine_state >= ENGINE_REGISTRATION_DONE &&
		client.engine_state <= ENGINE_DEREGISTER_FAILED);
}

static u8_t get_sm_state(void)
{
	/* TODO: add locking? */
	return client.engine_state;
}

static void sm_handle_timeout_state(struct lwm2m_message *msg,
				    enum sm_engine_state sm_state)
{
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_NONE;

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	if (client.engine_state == ENGINE_BOOTSTRAP_REG_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE;
	} else
#endif
	if (client.engine_state == ENGINE_REGISTRATION_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE;
	} else if (client.engine_state == ENGINE_UPDATE_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE;
	} else if (client.engine_state == ENGINE_DEREGISTER_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE;
	} else {
		/* TODO: unknown timeout state */
	}

	set_sm_state(sm_state);

	if (event > LWM2M_RD_CLIENT_EVENT_NONE && client.event_cb) {
		client.event_cb(client.ctx, event);
	}
}

/* force re-update with remote peer */
void engine_trigger_update(void)
{
	/* TODO: add locking? */
	client.trigger_update = 1U;
}

/* state machine reply callbacks */

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
static int do_bootstrap_reply_cb(const struct coap_packet *response,
				 struct coap_reply *reply,
				 const struct sockaddr *from)
{
	u8_t code;

	code = coap_header_get_code(response);
	LOG_DBG("Bootstrap callback (code:%u.%u)",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	if (code == COAP_RESPONSE_CODE_CHANGED) {
		LOG_INF("Bootstrap registration done!");
		set_sm_state(ENGINE_BOOTSTRAP_REG_DONE);
	} else if (code == COAP_RESPONSE_CODE_NOT_FOUND) {
		/* TODO: try and find another bootstrap server entry? */
		LOG_ERR("Failed: NOT_FOUND.  Not Retrying.");
		set_sm_state(ENGINE_DO_REGISTRATION);
	} else if (code == COAP_RESPONSE_CODE_FORBIDDEN) {
		/* TODO: try and find another bootstrap server entry? */
		LOG_ERR("Failed: 4.03 - Forbidden.  Not Retrying.");
		set_sm_state(ENGINE_DO_REGISTRATION);
	} else {
		/* TODO: Read payload for error message? */
		LOG_ERR("Failed with code %u.%u. Retrying ...",
			COAP_RESPONSE_CODE_CLASS(code),
			COAP_RESPONSE_CODE_DETAIL(code));
		set_sm_state(ENGINE_INIT);
	}

	return 0;
}

static void do_bootstrap_reg_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("Bootstrap Timeout");

	/* TODO:
	 * Look for the "next" BOOTSTRAP server entry in our security info
	 */

	/* Restart from scratch */
	sm_handle_timeout_state(msg, ENGINE_INIT);
}
#endif

static int do_registration_reply_cb(const struct coap_packet *response,
				    struct coap_reply *reply,
				    const struct sockaddr *from)
{
	struct coap_option options[2];
	u8_t code;
	int ret;

	code = coap_header_get_code(response);
	LOG_DBG("Registration callback (code:%u.%u)",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	/* check state and possibly set registration to done */
	if (code == COAP_RESPONSE_CODE_CREATED) {
		ret = coap_find_options(response, COAP_OPTION_LOCATION_PATH,
					options, 2);
		if (ret < 2) {
			LOG_ERR("Unexpected endpoint data returned.");
			return -EINVAL;
		}

		/* option[0] should be "rd" */

		if (options[1].len + 1 > sizeof(client.server_ep)) {
			LOG_ERR("Unexpected length of query: "
				    "%u (expected %zu)\n",
				    options[1].len,
				    sizeof(client.server_ep));
			return -EINVAL;
		}

		memcpy(client.server_ep, options[1].value,
		       options[1].len);
		client.server_ep[options[1].len] = '\0';
		set_sm_state(ENGINE_REGISTRATION_DONE);
		LOG_INF("Registration Done (EP='%s')",
			log_strdup(client.server_ep));

		return 0;
	} else if (code == COAP_RESPONSE_CODE_NOT_FOUND) {
		LOG_ERR("Failed: NOT_FOUND.  Not Retrying.");
		set_sm_state(ENGINE_REGISTRATION_DONE);
		return 0;
	} else if (code == COAP_RESPONSE_CODE_FORBIDDEN) {
		LOG_ERR("Failed: 4.03 - Forbidden.  Not Retrying.");
		set_sm_state(ENGINE_REGISTRATION_DONE);
		return 0;
	}

	/* TODO: Read payload for error message? */
	/* Possible error response codes: 4.00 Bad request */
	LOG_ERR("failed with code %u.%u. Re-init network",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	set_sm_state(ENGINE_INIT);
	return 0;
}

static void do_registration_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("Registration Timeout");

	/* Restart from scratch */
	sm_handle_timeout_state(msg, ENGINE_INIT);
}

static int do_update_reply_cb(const struct coap_packet *response,
			      struct coap_reply *reply,
			      const struct sockaddr *from)
{
	u8_t code;

	code = coap_header_get_code(response);
	LOG_INF("Update callback (code:%u.%u)",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	/* If NOT_FOUND just continue on */
	if ((code == COAP_RESPONSE_CODE_CHANGED) ||
	    (code == COAP_RESPONSE_CODE_CREATED)) {
		set_sm_state(ENGINE_REGISTRATION_DONE);
		LOG_INF("Update Done");
		return 0;
	}

	/* TODO: Read payload for error message? */
	/* Possible error response codes: 4.00 Bad request & 4.04 Not Found */
	LOG_ERR("Failed with code %u.%u. Retrying registration",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));
	set_sm_state(ENGINE_DO_REGISTRATION);

	return 0;
}

static void do_update_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("Registration Update Timeout");

	/* Re-do registration */
	sm_handle_timeout_state(msg, ENGINE_DO_REGISTRATION);
}

static int do_deregister_reply_cb(const struct coap_packet *response,
				  struct coap_reply *reply,
				  const struct sockaddr *from)
{
	u8_t code;

	code = coap_header_get_code(response);
	LOG_DBG("Deregister callback (code:%u.%u)",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	if (code == COAP_RESPONSE_CODE_DELETED) {
		LOG_INF("Deregistration success");
		lwm2m_engine_context_close(client.ctx);
		set_sm_state(ENGINE_DEREGISTERED);
	} else {
		LOG_ERR("failed with code %u.%u",
			COAP_RESPONSE_CODE_CLASS(code),
			COAP_RESPONSE_CODE_DETAIL(code));
		if (get_sm_state() == ENGINE_DEREGISTER_SENT) {
			set_sm_state(ENGINE_DEREGISTER_FAILED);
		}
	}

	return 0;
}

static void do_deregister_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("De-Registration Timeout");

	/* Abort de-registration and start from scratch */
	sm_handle_timeout_state(msg, ENGINE_INIT);
}

static int sm_select_next_sec_inst(bool bootstrap_server,
				   int *sec_obj_inst, u32_t *lifetime)
{
	char pathstr[MAX_RESOURCE_LEN];
	int ret, end, i, obj_inst_id, found = -1;
	bool temp;

	/* lookup existing index */
	i = lwm2m_security_inst_id_to_index(*sec_obj_inst);
	if (i < 0) {
		*sec_obj_inst = -1;
		i = -1;
	}

	/* store end marker, due to looping */
	end = (i == -1 ? CONFIG_LWM2M_SECURITY_INSTANCE_COUNT : i);

	/* loop through servers starting from the index after the current one */
	for (i++; i < end; i++) {
		if (i >= CONFIG_LWM2M_SECURITY_INSTANCE_COUNT) {
			i = 0;
		}

		obj_inst_id = lwm2m_security_index_to_inst_id(i);
		if (obj_inst_id < 0) {
			continue;
		}

		/* Query for bootstrap support */
		snprintk(pathstr, sizeof(pathstr), "0/%d/1",
			 obj_inst_id);
		ret = lwm2m_engine_get_bool(pathstr, &temp);
		if (ret < 0) {
			continue;
		}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
		if (temp == bootstrap_server) {
#else
		if (temp == false) {
#endif
			found = obj_inst_id;
			break;
		}
	}

	if (found > -1) {
		*sec_obj_inst = found;

		/* query the lifetime */
		/* TODO: use Short Server ID to link to server info */
		snprintk(pathstr, sizeof(pathstr), "1/%d/1",
			 obj_inst_id);
		if (lwm2m_engine_get_u32(pathstr, lifetime) < 0) {
			*lifetime = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
			LOG_DBG("Using default lifetime: %u", *lifetime);
		}
	}

	if (*sec_obj_inst < 0) {
		/* no servers found */
		LOG_WRN("sec_obj_inst: No matching servers found.");
		return -ENOENT;
	}

	return 0;
}

/* state machine step functions */

static int sm_do_init(void)
{
	client.ctx->sec_obj_inst = -1;
	client.trigger_update = 0U;
	client.lifetime = 0U;

	/* Do bootstrap or registration */
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	set_sm_state(ENGINE_DO_BOOTSTRAP_REG);
#else
	set_sm_state(ENGINE_DO_REGISTRATION);
#endif
	return 0;
}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
static int sm_do_bootstrap_reg(void)
{
	struct lwm2m_message *msg;
	int ret;

	/* clear out existing connection data */
	if (client.ctx->sock_fd > -1) {
		lwm2m_engine_context_close(client.ctx);
	}

	client.ctx->bootstrap_mode = true;
	ret = sm_select_next_sec_inst(client.ctx->bootstrap_mode,
				      &client.ctx->sec_obj_inst,
				      &client.lifetime);
	if (ret < 0) {
		/* no bootstrap server found, let's move to registration */
		LOG_WRN("Bootstrap server not found! Try normal registration.");
		set_sm_state(ENGINE_DO_REGISTRATION);
		return ret;
	}

	if (client.lifetime == 0U) {
		client.lifetime = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
	}

	LOG_INF("Bootstrap started with endpoint '%s' with client lifetime %d",
		log_strdup(client.ep_name), client.lifetime);

	ret = lwm2m_engine_start(client.ctx);
	if (ret < 0) {
		LOG_ERR("Cannot init LWM2M engine (%d)", ret);
		return ret;
	}

	msg = lwm2m_get_message(client.ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		ret = -ENOMEM;
		goto cleanup_engine;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_POST;
	msg->mid = 0U;
	msg->reply_cb = do_bootstrap_reply_cb;
	msg->message_timeout_cb = do_bootstrap_reg_timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	/* TODO: handle return error */
	coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
				  "bs", strlen("bs"));

	snprintk(query_buffer, sizeof(query_buffer) - 1, "ep=%s",
		 client.ep_name);
	/* TODO: handle return error */
	coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
				  query_buffer, strlen(query_buffer));

	/* log the bootstrap attempt */
	LOG_DBG("Register ID with bootstrap server as '%s'",
		log_strdup(query_buffer));

	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		LOG_ERR("Error sending LWM2M packet (err:%d).",
			    ret);
		goto cleanup;
	}

	set_sm_state(ENGINE_BOOTSTRAP_REG_SENT);
	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
cleanup_engine:
	lwm2m_engine_context_close(client.ctx);
	return ret;
}

static int sm_bootstrap_reg_done(void)
{
	LOG_INF("Bootstrap registration done.");
	return 0;
}

void engine_bootstrap_finish(void)
{
	LOG_INF("Bootstrap data transfer done!");
	set_sm_state(ENGINE_BOOTSTRAP_TRANS_DONE);
}

static int sm_bootstrap_trans_done(void)
{
	/* close down context resources */
	lwm2m_engine_context_close(client.ctx);

	/* reset security object instance */
	client.ctx->sec_obj_inst = -1;

	set_sm_state(ENGINE_DO_REGISTRATION);

	return 0;
}
#endif

static int sm_send_registration(bool send_obj_support_data,
				coap_reply_t reply_cb,
				lwm2m_message_timeout_cb_t timeout_cb)
{
	struct lwm2m_message *msg;
	u16_t client_data_len;
	int ret;
	char binding[CLIENT_BINDING_LEN];

	msg = lwm2m_get_message(client.ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	/* remember the last reg time */
	client.last_update = k_uptime_get();

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_POST;
	msg->mid = 0U;
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

	if (!sm_is_registered()) {
		/* include client endpoint in URI QUERY on 1st registration */
		coap_append_option_int(&msg->cpkt, COAP_OPTION_CONTENT_FORMAT,
				       LWM2M_FORMAT_APP_LINK_FORMAT);
		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "lwm2m=%s", LWM2M_PROTOCOL_VERSION);
		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
					  query_buffer, strlen(query_buffer));
		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "ep=%s", client.ep_name);
		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
					  query_buffer, strlen(query_buffer));
	} else {
		/* include server endpoint in URI PATH otherwise */
		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					  client.server_ep,
					  strlen(client.server_ep));
	}

	snprintk(query_buffer, sizeof(query_buffer) - 1,
		 "lt=%d", client.lifetime);
	/* TODO: handle return error */
	coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
				  query_buffer, strlen(query_buffer));

	lwm2m_engine_get_binding(binding);
	/* UDP is a default binding, no need to add option if UDP is used. */
	if (strcmp(binding, "U") != 0) {
		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "b=%s", binding);
		/* TODO: handle return error */
		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
					  query_buffer, strlen(query_buffer));
	}

	if (send_obj_support_data) {
		ret = coap_packet_append_payload_marker(&msg->cpkt);
		if (ret < 0) {
			goto cleanup;
		}

		/* generate the rd data */
		client_data_len = lwm2m_get_rd_data(client_data,
						    sizeof(client_data));
		ret = buf_append(CPKT_BUF_WRITE(&msg->cpkt), client_data,
				 client_data_len);
		if (ret < 0) {
			goto cleanup;
		}
	}

	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		LOG_ERR("Error sending LWM2M packet (err:%d).", ret);
		goto cleanup;
	}

	/* log the registration attempt */
	LOG_DBG("registration sent [%s]",
		log_strdup(lwm2m_sprint_ip_addr(&client.ctx->remote_addr)));

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static int sm_do_registration(void)
{
	int ret = 0;

	/* clear out existing connection data */
	if (client.ctx->sock_fd > -1) {
		lwm2m_engine_context_close(client.ctx);
	}

	client.ctx->bootstrap_mode = false;
	ret = sm_select_next_sec_inst(client.ctx->bootstrap_mode,
				      &client.ctx->sec_obj_inst,
				      &client.lifetime);
	if (ret < 0) {
		LOG_ERR("Unable to find a valid security instance.");
		set_sm_state(ENGINE_INIT);
		return -EINVAL;
	}

	if (client.lifetime == 0U) {
		client.lifetime = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
	}

	LOG_INF("RD Client started with endpoint '%s' with client lifetime %d",
		log_strdup(client.ep_name), client.lifetime);

	ret = lwm2m_engine_start(client.ctx);
	if (ret < 0) {
		LOG_ERR("Cannot init LWM2M engine (%d)", ret);
		return ret;
	}

	ret = sm_send_registration(true,
				   do_registration_reply_cb,
				   do_registration_timeout_cb);
	if (!ret) {
		set_sm_state(ENGINE_REGISTRATION_SENT);
	} else {
		LOG_ERR("Registration err: %d", ret);
		lwm2m_engine_context_close(client.ctx);
		/* exit and try again */
	}

	return ret;
}

static int sm_registration_done(void)
{
	int ret = 0;
	bool forced_update;

	/*
	 * check for lifetime seconds - SECONDS_TO_UPDATE_EARLY
	 * so that we can update early and avoid lifetime timeout
	 */
	if (sm_is_registered() &&
	    (client.trigger_update ||
	     ((client.lifetime - SECONDS_TO_UPDATE_EARLY) <=
	      (k_uptime_get() - client.last_update) / 1000))) {
		forced_update = client.trigger_update;
		client.trigger_update = 0U;
		ret = sm_send_registration(forced_update,
					   do_update_reply_cb,
					   do_update_timeout_cb);
		if (!ret) {
			set_sm_state(ENGINE_UPDATE_SENT);
		} else {
			LOG_ERR("Registration update err: %d", ret);
			lwm2m_engine_context_close(client.ctx);
			/* perform full registration */
			set_sm_state(ENGINE_DO_REGISTRATION);
		}
	}

	if (IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_ENABLED) &&
	    (client.engine_state != ENGINE_REGISTRATION_DONE_RX_OFF) &&
	    (((k_uptime_get() - client.last_tx) / 1000) >=
	     CONFIG_LWM2M_QUEUE_MODE_UPTIME)) {
		set_sm_state(ENGINE_REGISTRATION_DONE_RX_OFF);
	}

	return ret;
}

static int sm_do_deregister(void)
{
	struct lwm2m_message *msg;
	int ret;

	msg = lwm2m_get_message(client.ctx);
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_DELETE;
	msg->mid = 0U;
	msg->reply_cb = do_deregister_reply_cb;
	msg->message_timeout_cb = do_deregister_timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	/* TODO: handle return error */
	coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
				  LWM2M_RD_CLIENT_URI,
				  strlen(LWM2M_RD_CLIENT_URI));
	/* include server endpoint in URI PATH */
	coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
				  client.server_ep,
				  strlen(client.server_ep));

	LOG_INF("Deregister from '%s'", log_strdup(client.server_ep));

	ret = lwm2m_send_message(msg);
	if (ret < 0) {
		LOG_ERR("Error sending LWM2M packet (err:%d).", ret);
		goto cleanup;
	}

	set_sm_state(ENGINE_DEREGISTER_SENT);
	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	lwm2m_engine_context_close(client.ctx);
	return ret;
}

static void lwm2m_rd_client_service(struct k_work *work)
{
	if (client.ctx) {
		switch (get_sm_state()) {

		case ENGINE_INIT:
			sm_do_init();
			break;

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
		case ENGINE_DO_BOOTSTRAP_REG:
			sm_do_bootstrap_reg();
			break;

		case ENGINE_BOOTSTRAP_REG_SENT:
			/* wait for bootstrap registration done */
			break;

		case ENGINE_BOOTSTRAP_REG_DONE:
			sm_bootstrap_reg_done();
			/* wait for transfer done */
			break;

		case ENGINE_BOOTSTRAP_TRANS_DONE:
			sm_bootstrap_trans_done();
			break;
#endif

		case ENGINE_DO_REGISTRATION:
			sm_do_registration();
			break;

		case ENGINE_REGISTRATION_SENT:
			/* wait registration to be done or timeout */
			break;

		case ENGINE_REGISTRATION_DONE:
		case ENGINE_REGISTRATION_DONE_RX_OFF:
			sm_registration_done();
			break;

		case ENGINE_UPDATE_SENT:
			/* wait update to be done or abort */
			break;

		case ENGINE_DEREGISTER:
			sm_do_deregister();
			break;

		case ENGINE_DEREGISTER_SENT:
			/* wait for deregister to be done or reset */
			break;

		case ENGINE_DEREGISTER_FAILED:
			break;

		case ENGINE_DEREGISTERED:
			break;

		default:
			LOG_ERR("Unhandled state: %d", get_sm_state());

		}
	}
}

void lwm2m_rd_client_start(struct lwm2m_ctx *client_ctx, const char *ep_name,
			   lwm2m_ctx_event_cb_t event_cb)
{
	client.ctx = client_ctx;
	client.ctx->sock_fd = -1;
	client.event_cb = event_cb;

	set_sm_state(ENGINE_INIT);
	strncpy(client.ep_name, ep_name, CLIENT_EP_LEN - 1);
	LOG_INF("Start LWM2M Client: %s", log_strdup(client.ep_name));
}

void lwm2m_rd_client_stop(struct lwm2m_ctx *client_ctx,
			   lwm2m_ctx_event_cb_t event_cb)
{
	client.ctx = client_ctx;
	client.event_cb = event_cb;

	set_sm_state(ENGINE_DEREGISTER);
	LOG_INF("Stop LWM2M Client: %s", log_strdup(client.ep_name));
}

static int lwm2m_rd_client_init(struct device *dev)
{
	return lwm2m_engine_add_service(lwm2m_rd_client_service,
					STATE_MACHINE_UPDATE_INTERVAL);
}

SYS_INIT(lwm2m_rd_client_init, APPLICATION,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
