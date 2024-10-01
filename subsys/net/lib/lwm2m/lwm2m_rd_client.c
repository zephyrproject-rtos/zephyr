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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/socket.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_rd_client.h"
#include "lwm2m_rw_link_format.h"
#include "lwm2m_util.h"
#include "lwm2m_obj_server.h"

#define LWM2M_RD_CLIENT_URI "rd"
#define CLIENT_EP_LEN		CONFIG_LWM2M_RD_CLIENT_ENDPOINT_NAME_MAX_LENGTH
#define CLIENT_BINDING_LEN sizeof("UQ")
#define CLIENT_QUEUE_LEN sizeof("Q")
#define DELAY_BEFORE_CLOSING	(1 * MSEC_PER_SEC)
#define DELAY_FOR_ACK		100U
#define EXCHANGE_LIFETIME	247U
#define MINIMUM_PERIOD		15
#define DISABLE_TIMEOUT		(K_SECONDS(CONFIG_LWM2M_RD_CLIENT_MAX_RETRIES * EXCHANGE_LIFETIME))

static void sm_handle_registration_update_failure(void);
static int sm_send_registration_msg(void);
static bool sm_is_suspended(void);
static void lwm2m_rd_client_service(struct k_work *work);
static int64_t calc_next_event(void);
static void set_sm_state_delayed(uint8_t sm_state, int64_t delay_ms);
static void set_sm_state(uint8_t sm_state);
/** Try to fallback to bootstrap. Return true if we did. */
static bool fallback_to_bootstrap(void);

/* The states for the RD client state machine */
/*
 * When node is unregistered it ends up in UNREGISTERED
 * and this is going to be there until use X or Y kicks it
 * back into INIT again
 */
enum sm_engine_state {
	ENGINE_IDLE,
	ENGINE_INIT,
	ENGINE_DO_BOOTSTRAP_REG,
	ENGINE_BOOTSTRAP_REG_SENT,
	ENGINE_BOOTSTRAP_REG_DONE,
	ENGINE_BOOTSTRAP_TRANS_DONE,
	ENGINE_DO_REGISTRATION,
	ENGINE_SEND_REGISTRATION,
	ENGINE_REGISTRATION_SENT,
	ENGINE_REGISTRATION_DONE,
	ENGINE_REGISTRATION_DONE_RX_OFF,
	ENGINE_UPDATE_REGISTRATION,
	ENGINE_UPDATE_SENT,
	ENGINE_SERVER_DISABLED,
	ENGINE_SUSPENDED,
	ENGINE_DEREGISTER,
	ENGINE_DEREGISTER_SENT,
	ENGINE_DEREGISTERED,
	ENGINE_NETWORK_ERROR,
};

struct lwm2m_rd_client_info {
	struct k_mutex mutex;
	struct lwm2m_message rd_message;
	struct lwm2m_ctx *ctx;
	uint32_t lifetime;
	uint8_t engine_state;
	uint8_t retries;
	uint8_t retry_delay;

	int64_t last_update;
	int64_t last_tx;
	int64_t next_event;
	int64_t last_state_change;

	char ep_name[CLIENT_EP_LEN];
	char server_ep[CLIENT_EP_LEN];

	bool use_bootstrap  : 1;
	bool trigger_update : 1;
	bool update_objects : 1;
	bool close_socket   : 1;
	bool server_disabled: 1;
} client;

/* Allocate some data for queries and updates. Make sure it's large enough to
 * hold the largest query string, which in most cases will be the endpoint
 * string. In other case, 32 bytes are enough to encode any other query string
 * documented in the LwM2M specification.
 */
static char query_buffer[MAX(32, sizeof("ep=") + CLIENT_EP_LEN)];
static enum sm_engine_state suspended_client_state;

static struct lwm2m_message *rd_get_message(void)
{
	if (client.rd_message.ctx) {
		/* Free old message */
		lwm2m_reset_message(&client.rd_message, true);
	}

	client.rd_message.ctx = client.ctx;
	return &client.rd_message;

}

static void rd_client_message_free(void)
{
	lwm2m_reset_message(lwm2m_get_ongoing_rd_msg(), true);
}


struct lwm2m_message *lwm2m_get_ongoing_rd_msg(void)
{
	if (!client.ctx || !client.rd_message.ctx) {
		return NULL;
	}
	return &client.rd_message;
}

void engine_update_tx_time(void)
{
	client.last_tx = k_uptime_get();
}

static void next_event_at(int64_t timestamp)
{
	client.next_event = timestamp;
	(void)lwm2m_engine_call_at(lwm2m_rd_client_service, timestamp);
}

static void set_sm_state_delayed(uint8_t sm_state, int64_t delay_ms)
{
	k_mutex_lock(&client.mutex, K_FOREVER);
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_NONE;

	/* Determine if a callback to the app is needed */
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	if (sm_state == ENGINE_BOOTSTRAP_REG_DONE) {
		event = LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_COMPLETE;
	} else if (client.engine_state == ENGINE_BOOTSTRAP_TRANS_DONE &&
		   sm_state == ENGINE_DO_REGISTRATION) {
		event = LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_TRANSFER_COMPLETE;
	} else
#endif
	if (client.engine_state == ENGINE_UPDATE_SENT &&
	    (sm_state == ENGINE_REGISTRATION_DONE ||
	     sm_state == ENGINE_REGISTRATION_DONE_RX_OFF)) {
		lwm2m_push_queued_buffers(client.ctx);
		event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE_COMPLETE;
	} else if (sm_state == ENGINE_REGISTRATION_DONE) {
		lwm2m_push_queued_buffers(client.ctx);
		event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_COMPLETE;
	} else if (sm_state == ENGINE_REGISTRATION_DONE_RX_OFF) {
		event = LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF;
	} else if (sm_state == ENGINE_DEREGISTERED && !client.server_disabled) {
		event = LWM2M_RD_CLIENT_EVENT_DISCONNECT;
	} else if (sm_state == ENGINE_UPDATE_REGISTRATION) {
		event = LWM2M_RD_CLIENT_EVENT_REG_UPDATE;
	} else if (sm_state == ENGINE_DEREGISTER) {
		if (client.server_disabled) {
			event = LWM2M_RD_CLIENT_EVENT_SERVER_DISABLED;
		} else {
			event = LWM2M_RD_CLIENT_EVENT_DEREGISTER;
		}
	}

	if (sm_is_suspended()) {
		/* Just change the state where we are going to resume next */
		suspended_client_state = sm_state;
	} else {
		client.engine_state = sm_state;
	}

	if (event > LWM2M_RD_CLIENT_EVENT_NONE && client.ctx->event_cb) {
		client.ctx->event_cb(client.ctx, event);
	}

	/* Suspend socket after Event callback */
	if (event == LWM2M_RD_CLIENT_EVENT_QUEUE_MODE_RX_OFF) {
		if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUSPEND_SOCKET_AT_IDLE) ||
		    IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_STOP_POLLING_AT_IDLE)) {
			lwm2m_socket_suspend(client.ctx);
		} else if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_CLOSE_SOCKET_AT_IDLE)) {
			lwm2m_close_socket(client.ctx);
		}
	}
	client.last_state_change = k_uptime_get();
	next_event_at(k_uptime_get() + delay_ms);
	k_mutex_unlock(&client.mutex);
}

static void set_sm_state(uint8_t sm_state)
{
	set_sm_state_delayed(sm_state, 0);
}

static bool sm_is_bootstrap(void)
{
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	k_mutex_lock(&client.mutex, K_FOREVER);
	bool is_bootstrap = (client.engine_state >= ENGINE_DO_BOOTSTRAP_REG &&
		client.engine_state <= ENGINE_BOOTSTRAP_TRANS_DONE);
	k_mutex_unlock(&client.mutex);
	return is_bootstrap;
#else
	return false;
#endif
}

static bool sm_is_registered(void)
{
	k_mutex_lock(&client.mutex, K_FOREVER);
	bool registered = (client.engine_state >= ENGINE_REGISTRATION_DONE &&
			   client.engine_state <= ENGINE_DEREGISTER_SENT);

	k_mutex_unlock(&client.mutex);
	return registered;
}

static bool sm_is_suspended(void)
{
	k_mutex_lock(&client.mutex, K_FOREVER);
	bool suspended = (client.engine_state == ENGINE_SUSPENDED);

	k_mutex_unlock(&client.mutex);
	return suspended;
}


static uint8_t get_sm_state(void)
{
	k_mutex_lock(&client.mutex, K_FOREVER);
	uint8_t state = client.engine_state;

	k_mutex_unlock(&client.mutex);
	return state;
}

/** Handle state transition when we have lost the connection. */
static void sm_handle_timeout_state(enum sm_engine_state sm_state)
{
	k_mutex_lock(&client.mutex, K_FOREVER);
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_NONE;

	switch (client.engine_state) {
	case ENGINE_DO_BOOTSTRAP_REG:
	case ENGINE_BOOTSTRAP_REG_SENT:
	case ENGINE_BOOTSTRAP_REG_DONE:
	case ENGINE_BOOTSTRAP_TRANS_DONE:
		/* Don't send BOOTSTRAP_REG_FAILURE event, that is only emitted from
		 * do_network_error() once we are out of retries.
		 */
		break;

	case ENGINE_SEND_REGISTRATION:
	case ENGINE_REGISTRATION_SENT:
	case ENGINE_REGISTRATION_DONE:
	case ENGINE_REGISTRATION_DONE_RX_OFF:
	case ENGINE_UPDATE_REGISTRATION:
	case ENGINE_UPDATE_SENT:
		event = LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT;
		break;

	case ENGINE_DEREGISTER:
	case ENGINE_DEREGISTER_SENT:
		event = LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE;
		break;
	default:
		/* No default events for socket errors */
		break;
	}

	set_sm_state(sm_state);

	if (event > LWM2M_RD_CLIENT_EVENT_NONE && client.ctx->event_cb) {
		client.ctx->event_cb(client.ctx, event);
	}
	k_mutex_unlock(&client.mutex);
}

/** Handle state transition where server have rejected the connection. */
static void sm_handle_failure_state(enum sm_engine_state sm_state)
{
	k_mutex_lock(&client.mutex, K_FOREVER);
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_NONE;

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	if (client.engine_state == ENGINE_BOOTSTRAP_REG_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE;
	} else
#endif
	if (client.engine_state == ENGINE_REGISTRATION_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_REGISTRATION_FAILURE;
	} else if (client.engine_state == ENGINE_UPDATE_SENT) {
		sm_handle_registration_update_failure();
		k_mutex_unlock(&client.mutex);
		return;
	} else if (client.engine_state == ENGINE_DEREGISTER_SENT) {
		event = LWM2M_RD_CLIENT_EVENT_DEREGISTER_FAILURE;
	}

	lwm2m_engine_stop(client.ctx);
	set_sm_state(sm_state);

	if (event > LWM2M_RD_CLIENT_EVENT_NONE && client.ctx->event_cb) {
		client.ctx->event_cb(client.ctx, event);
	}
	k_mutex_unlock(&client.mutex);
}

/* force state machine restart */
static void socket_fault_cb(int error)
{
	LOG_ERR("RD Client socket error: %d", error);
	lwm2m_socket_close(client.ctx);

	if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) && sm_is_bootstrap()) {
		client.ctx->sec_obj_inst = -1;
		/* force full registration */
		client.last_update = 0;

		if (get_sm_state() == ENGINE_BOOTSTRAP_TRANS_DONE) {
			/* Ignore the error, some servers close the connection immediately
			 * after receiving Ack to Bootstrap-Finish command.
			 */
			return;
		}
	}

	/* Network error state causes engine to re-register,
	 * so only trigger that state if we are not stopping the
	 * engine.
	 * Also when engine is going to be disabled, for a while, we might get spurious
	 * socket errors when closing, so ignore them.
	 */
	if (client.engine_state > ENGINE_IDLE &&
		client.engine_state < ENGINE_SERVER_DISABLED) {
		sm_handle_timeout_state(ENGINE_NETWORK_ERROR);
	} else if (client.engine_state != ENGINE_SUSPENDED &&
		   !client.server_disabled) {
		sm_handle_timeout_state(ENGINE_IDLE);
	}
}

/* force re-update with remote peer */
void engine_trigger_update(bool update_objects)
{
	k_mutex_lock(&client.mutex, K_FOREVER);
	if (client.engine_state < ENGINE_REGISTRATION_SENT ||
	    client.engine_state > ENGINE_UPDATE_SENT) {
		k_mutex_unlock(&client.mutex);
		return;
	}

	client.trigger_update = true;
	/* short delay for Ack, then trigger an update */
	next_event_at(k_uptime_get() + DELAY_FOR_ACK);

	if (update_objects) {
		client.update_objects = true;
	}
	k_mutex_unlock(&client.mutex);
}

static inline const char *code2str(uint8_t code)
{
	switch (code) {
	case COAP_RESPONSE_CODE_BAD_REQUEST:
		return "Bad Request";
	case COAP_RESPONSE_CODE_FORBIDDEN:
		return "Forbidden";
	case COAP_RESPONSE_CODE_NOT_FOUND:
		return "Not Found";
	case COAP_RESPONSE_CODE_PRECONDITION_FAILED:
		return "Precondition Failed";
	default:
		break;
	}

	return "Unknown";
}

/* state machine reply callbacks */

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
static int do_bootstrap_reply_cb(const struct coap_packet *response,
				 struct coap_reply *reply,
				 const struct sockaddr *from)
{
	uint8_t code;

	code = coap_header_get_code(response);
	LOG_DBG("Bootstrap callback (code:%u.%u)",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	if (code == COAP_RESPONSE_CODE_CHANGED) {
		LOG_INF("Bootstrap registration done!");
		set_sm_state(ENGINE_BOOTSTRAP_REG_DONE);
		return 0;
	}

	LOG_ERR("Failed with code %u.%u (%s). Not Retrying.",
		COAP_RESPONSE_CODE_CLASS(code), COAP_RESPONSE_CODE_DETAIL(code),
		code2str(code));

	sm_handle_failure_state(ENGINE_IDLE);

	return 0;
}

static void do_bootstrap_reg_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("Bootstrap Timeout");
	sm_handle_timeout_state(ENGINE_NETWORK_ERROR);
}
#endif

int engine_trigger_bootstrap(void)
{
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	k_mutex_lock(&client.mutex, K_FOREVER);

	if (client.use_bootstrap) {
		/* Bootstrap is not possible to trig */
		LOG_WRN("Bootstrap process ongoing");
		k_mutex_unlock(&client.mutex);
		return -EPERM;
	}
	LOG_INF("Server Initiated Bootstrap");
	/* Free ongoing possible message */
	rd_client_message_free();
	client.use_bootstrap = true;
	client.trigger_update = false;
	set_sm_state_delayed(ENGINE_INIT, DELAY_BEFORE_CLOSING);
	k_mutex_unlock(&client.mutex);
	return 0;
#else
	return -EPERM;
#endif
}
static int do_registration_reply_cb(const struct coap_packet *response,
				    struct coap_reply *reply,
				    const struct sockaddr *from)
{
	struct coap_option options[2];
	uint8_t code;
	int ret = -EINVAL;

	code = coap_header_get_code(response);
	LOG_DBG("Registration callback (code:%u.%u)",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	/* check state and possibly set registration to done */
	if (code == COAP_RESPONSE_CODE_CREATED) {
		ret = coap_find_options(response, COAP_OPTION_LOCATION_PATH,
					options, 2);
		if (ret < 2) {
			LOG_ERR("Unexpected endpoint data returned. ret = %d", ret);
			ret = -EINVAL;
			goto fail;
		}

		/* option[0] should be "rd" */

		if (options[1].len + 1 > sizeof(client.server_ep)) {
			LOG_ERR("Unexpected length of query: "
				    "%u (expected %zu)\n",
				    options[1].len,
				    sizeof(client.server_ep));
			ret = -EINVAL;
			goto fail;
		}

		/* remember the last reg time */
		client.last_update = k_uptime_get();
		client.server_disabled = false;
		client.retries = 0;

		memcpy(client.server_ep, options[1].value,
		       options[1].len);
		client.server_ep[options[1].len] = '\0';
		set_sm_state(ENGINE_REGISTRATION_DONE);
		LOG_INF("Registration Done (EP='%s')",
			client.server_ep);

		return 0;
	}

	LOG_ERR("Failed with code %u.%u (%s).",
		COAP_RESPONSE_CODE_CLASS(code), COAP_RESPONSE_CODE_DETAIL(code),
		code2str(code));
fail:
	lwm2m_server_disable(client.ctx->srv_obj_inst, DISABLE_TIMEOUT);
	sm_handle_failure_state(ENGINE_NETWORK_ERROR);

	return ret;
}

static void do_registration_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("Registration Timeout");

	sm_handle_timeout_state(ENGINE_NETWORK_ERROR);
}

static int do_update_reply_cb(const struct coap_packet *response,
			      struct coap_reply *reply,
			      const struct sockaddr *from)
{
	uint8_t code;

	code = coap_header_get_code(response);
	LOG_INF("Update callback (code:%u.%u)",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	/* If NOT_FOUND just continue on */
	if ((code == COAP_RESPONSE_CODE_CHANGED) ||
	    (code == COAP_RESPONSE_CODE_CREATED)) {
		/* remember the last reg time */
		client.last_update = k_uptime_get();
		client.server_disabled = false;
		client.retries = 0;
		set_sm_state(ENGINE_REGISTRATION_DONE);
		LOG_INF("Update Done");
		return 0;
	}

	LOG_ERR("Failed with code %u.%u (%s). Retrying registration.",
		COAP_RESPONSE_CODE_CLASS(code), COAP_RESPONSE_CODE_DETAIL(code),
		code2str(code));

	sm_handle_failure_state(ENGINE_DO_REGISTRATION);

	return 0;
}

static void do_update_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("Registration Update Timeout");

	if (client.ctx->sock_fd > -1) {
		client.close_socket = true;
	}
	/* Re-do registration */
	sm_handle_timeout_state(ENGINE_DO_REGISTRATION);
}

static int do_deregister_reply_cb(const struct coap_packet *response,
				  struct coap_reply *reply,
				  const struct sockaddr *from)
{
	uint8_t code;

	code = coap_header_get_code(response);
	LOG_DBG("Deregister callback (code:%u.%u)",
		COAP_RESPONSE_CODE_CLASS(code),
		COAP_RESPONSE_CODE_DETAIL(code));

	if (code == COAP_RESPONSE_CODE_DELETED) {
		LOG_INF("Deregistration success");
		set_sm_state(ENGINE_DEREGISTERED);
		return 0;
	}

	LOG_ERR("Failed with code %u.%u (%s). Not Retrying",
		COAP_RESPONSE_CODE_CLASS(code), COAP_RESPONSE_CODE_DETAIL(code),
		code2str(code));

	sm_handle_failure_state(ENGINE_DEREGISTERED);

	return 0;
}

static void do_deregister_timeout_cb(struct lwm2m_message *msg)
{
	LOG_WRN("De-Registration Timeout");

	sm_handle_timeout_state(ENGINE_DEREGISTERED);
}

static bool is_bootsrap_server(int sec_obj_inst)
{
	bool bootstrap;
	int ret;

	ret = lwm2m_get_bool(&LWM2M_OBJ(0, sec_obj_inst, 1), &bootstrap);
	if (ret < 0) {
		LOG_WRN("Failed to check bootstrap, err %d", ret);
		return false;
	}
	return bootstrap;
}

static bool sm_update_lifetime(int srv_obj_inst, uint32_t *lifetime)
{
	uint32_t new_lifetime;

	if (lwm2m_get_u32(&LWM2M_OBJ(1, srv_obj_inst, 1), &new_lifetime) < 0) {
		new_lifetime = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
		LOG_INF("Using default lifetime: %u", new_lifetime);
	}

	if (new_lifetime < CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME) {
		new_lifetime = CONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME;
		lwm2m_set_u32(&LWM2M_OBJ(1, srv_obj_inst, 1), new_lifetime);
		LOG_INF("Overwrite a server lifetime with default");
	}

	if (new_lifetime != *lifetime) {
		*lifetime = new_lifetime;
		return true;
	}

	return false;
}

/**
 * @brief Find the next security instance for bootstrapping.
 *
 * Search for the next security instance that has the bootstrap flag set and
 * is not the same as current security instance.
 *
 * @param sec_obj_inst current security instance or -1.
 * @return zero on success, negative on error.
 */
static int sm_next_bootstrap_inst(int *sec_obj_inst)
{
	int i, obj_inst_id = -1;

	if (*sec_obj_inst >= 0 && !is_bootsrap_server(*sec_obj_inst)) {
		*sec_obj_inst = -1;
	}

	/* Iterate over all instances to find the correct one. */
	for (i = 0; i < CONFIG_LWM2M_SECURITY_INSTANCE_COUNT; i++) {
		obj_inst_id = lwm2m_security_index_to_inst_id(i);
		if (obj_inst_id < 0) {
			continue;
		}
		if (obj_inst_id == *sec_obj_inst) {
			continue;
		}

		if (is_bootsrap_server(obj_inst_id)) {
			*sec_obj_inst = obj_inst_id;
			return 0;
		}
	}

	LOG_WRN("No Bootstrap servers found.");

	return -ENOENT;
}

/* state machine step functions */

static int sm_do_init(void)
{
	lwm2m_engine_stop(client.ctx);
	client.trigger_update = false;
	client.lifetime = 0U;
	client.last_update = 0U;
	client.close_socket = false;

	/* Do bootstrap or registration */
	if (client.use_bootstrap && IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)) {
		set_sm_state(ENGINE_DO_BOOTSTRAP_REG);
	} else {
		set_sm_state(ENGINE_DO_REGISTRATION);
	}
	return 0;
}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
static int sm_send_bootstrap_registration(void)
{
	struct lwm2m_message *msg;
	int ret;

	msg = rd_get_message();
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_POST;
	msg->mid = coap_next_id();
	msg->tkl = LWM2M_MSG_TOKEN_GENERATE_NEW;
	msg->reply_cb = do_bootstrap_reply_cb;
	msg->message_timeout_cb = do_bootstrap_reg_timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					"bs", strlen("bs"));
	if (ret < 0) {
		goto cleanup;
	}

	snprintk(query_buffer, sizeof(query_buffer) - 1, "ep=%s",
		 client.ep_name);

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
					query_buffer, strlen(query_buffer));
	if (ret < 0) {
		goto cleanup;
	}

	if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1)) {
		int pct = LWM2M_FORMAT_OMA_TLV;

		if (IS_ENABLED(CONFIG_LWM2M_RW_SENML_CBOR_SUPPORT)) {
			pct = LWM2M_FORMAT_APP_SENML_CBOR;
		} else if (IS_ENABLED(CONFIG_LWM2M_RW_SENML_JSON_SUPPORT)) {
			pct = LWM2M_FORMAT_APP_SEML_JSON;
		}

		snprintk(query_buffer, sizeof(query_buffer) - 1, "pct=%d", pct);

		coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY,
				  query_buffer, strlen(query_buffer));
	}

	/* log the bootstrap attempt */
	LOG_DBG("Register ID with bootstrap server as '%s'",
		query_buffer);

	lwm2m_send_message_async(msg);

	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
	return ret;
}

static void sm_do_bootstrap_reg(void)
{
	int ret;

	/* clear out existing connection data */
	if (client.ctx->sock_fd > -1) {
		lwm2m_engine_stop(client.ctx);
	}

	client.ctx->bootstrap_mode = true;
	ret = sm_next_bootstrap_inst(&client.ctx->sec_obj_inst);
	if (ret < 0) {
		set_sm_state(ENGINE_NETWORK_ERROR);
		return;
	}

	LOG_INF("Bootstrap started with endpoint '%s' using security object %d",
		client.ep_name, client.ctx->sec_obj_inst);

	ret = lwm2m_engine_start(client.ctx);
	if (ret < 0) {
		LOG_ERR("Cannot init LWM2M engine (%d)", ret);
		set_sm_state(ENGINE_NETWORK_ERROR);
		return;
	}

	ret = sm_send_bootstrap_registration();
	if (!ret) {
		set_sm_state(ENGINE_BOOTSTRAP_REG_SENT);
	} else {
		LOG_ERR("Bootstrap registration err: %d", ret);
		set_sm_state(ENGINE_NETWORK_ERROR);
	}

	return;
}

void engine_bootstrap_finish(void)
{
	LOG_INF("Bootstrap data transfer done!");
	/* Delay the state transition, so engine have some time to send ACK
	 * before we close the socket
	 */
	set_sm_state_delayed(ENGINE_BOOTSTRAP_TRANS_DONE, DELAY_BEFORE_CLOSING);
}

static int sm_bootstrap_trans_done(void)
{
	/* close down context resources */
	lwm2m_engine_stop(client.ctx);

	/* reset security object instance */
	client.ctx->sec_obj_inst = -1;
	client.use_bootstrap = false;

	set_sm_state(ENGINE_DO_REGISTRATION);

	return 0;
}
#endif

static int sm_send_registration(bool send_obj_support_data,
				coap_reply_t reply_cb,
				lwm2m_message_timeout_cb_t timeout_cb)
{
	struct lwm2m_message *msg;
	int ret;
	char binding[CLIENT_BINDING_LEN];
	char queue[CLIENT_QUEUE_LEN];

	msg = rd_get_message();
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		return -ENOMEM;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_POST;
	msg->mid = coap_next_id();
	msg->tkl = LWM2M_MSG_TOKEN_GENERATE_NEW;
	msg->reply_cb = reply_cb;
	msg->message_timeout_cb = timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					LWM2M_RD_CLIENT_URI,
					strlen(LWM2M_RD_CLIENT_URI));
	if (ret < 0) {
		goto cleanup;
	}

	if (sm_is_registered()) {
		ret = coap_packet_append_option(
			&msg->cpkt, COAP_OPTION_URI_PATH,
			client.server_ep, strlen(client.server_ep));
		if (ret < 0) {
			goto cleanup;
		}
	}

	if (send_obj_support_data) {
		ret = coap_append_option_int(
			&msg->cpkt, COAP_OPTION_CONTENT_FORMAT,
			LWM2M_FORMAT_APP_LINK_FORMAT);
		if (ret < 0) {
			goto cleanup;
		}
	}

	if (!sm_is_registered()) {
		snprintk(query_buffer, sizeof(query_buffer) - 1,
			"lwm2m=%s", LWM2M_PROTOCOL_VERSION_STRING);
		ret = coap_packet_append_option(
			&msg->cpkt, COAP_OPTION_URI_QUERY,
			query_buffer, strlen(query_buffer));
		if (ret < 0) {
			goto cleanup;
		}

		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "ep=%s", client.ep_name);
		ret = coap_packet_append_option(
			&msg->cpkt, COAP_OPTION_URI_QUERY,
			query_buffer, strlen(query_buffer));
		if (ret < 0) {
			goto cleanup;
		}
	}

	/* Send lifetime only if changed or on initial registration.*/
	if (sm_update_lifetime(client.ctx->srv_obj_inst, &client.lifetime) ||
	    !sm_is_registered()) {
		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "lt=%d", client.lifetime);
		ret = coap_packet_append_option(
			&msg->cpkt, COAP_OPTION_URI_QUERY,
			query_buffer, strlen(query_buffer));
		if (ret < 0) {
			goto cleanup;
		}
	}

	lwm2m_engine_get_binding(binding);
	lwm2m_engine_get_queue_mode(queue);
	/* UDP is a default binding, no need to add option if UDP without queue is used. */
	if ((!sm_is_registered() && (strcmp(binding, "U") != 0 || strcmp(queue, "Q") == 0))) {
		snprintk(query_buffer, sizeof(query_buffer) - 1,
			 "b=%s", binding);

		ret = coap_packet_append_option(
			&msg->cpkt, COAP_OPTION_URI_QUERY,
			query_buffer, strlen(query_buffer));
		if (ret < 0) {
			goto cleanup;
		}

#if CONFIG_LWM2M_VERSION_1_1
		/* In LwM2M 1.1, queue mode is a separate parameter */
		uint16_t len = strlen(queue);

		if (len) {
			ret = coap_packet_append_option(
				&msg->cpkt, COAP_OPTION_URI_QUERY,
				queue, len);
			if (ret < 0) {
				goto cleanup;
			}
		}
#endif
	}

	if (send_obj_support_data) {
		ret = coap_packet_append_payload_marker(&msg->cpkt);
		if (ret < 0) {
			goto cleanup;
		}

		msg->out.out_cpkt = &msg->cpkt;
		msg->out.writer = &link_format_writer;

		ret = do_register_op_link_format(msg);
		if (ret < 0) {
			goto cleanup;
		}
	}

	lwm2m_send_message_async(msg);

	/* log the registration attempt */
	LOG_DBG("registration sent [%s]",
		lwm2m_sprint_ip_addr(&client.ctx->remote_addr));

	return 0;

cleanup:
	LOG_ERR("error %d when sending registration message", ret);
	lwm2m_reset_message(msg, true);
	return ret;
}

static void sm_handle_registration_update_failure(void)
{
	k_mutex_lock(&client.mutex, K_FOREVER);
	LOG_WRN("Registration Update fail -> trigger full registration");
	lwm2m_engine_context_close(client.ctx);
	set_sm_state(ENGINE_SEND_REGISTRATION);
	k_mutex_unlock(&client.mutex);
}

static int sm_send_registration_msg(void)
{
	int ret;

	ret = sm_send_registration(true,
				   do_registration_reply_cb,
				   do_registration_timeout_cb);
	if (!ret) {
		set_sm_state(ENGINE_REGISTRATION_SENT);
	} else {
		LOG_ERR("Registration err: %d", ret);
		set_sm_state(ENGINE_NETWORK_ERROR);
	}

	return ret;
}

static void sm_do_registration(void)
{
	uint16_t ssid;
	int ret = 0;

	if (client.ctx->connection_suspended) {
		if (lwm2m_engine_connection_resume(client.ctx)) {
			lwm2m_engine_context_close(client.ctx);
			/* perform full registration */
			set_sm_state(ENGINE_DO_REGISTRATION);
			return;
		}

	} else {
		bool select_srv = true;
		uint16_t srv = (uint16_t) client.ctx->srv_obj_inst;

		client.last_update = 0;
		client.ctx->bootstrap_mode = false;

		/* clear out existing connection data */
		if (client.ctx->sock_fd > -1) {
			if (client.close_socket) {
				/* Clear old socket connection */
				client.close_socket = false;
				lwm2m_engine_stop(client.ctx);
			} else {
				lwm2m_engine_context_close(client.ctx);
				/* Keep current connection, retry registration with same server */
				select_srv = false;
			}
		}

		if (select_srv) {
			/* Select next one from the list, or fail */
			if (!lwm2m_server_select(&srv)) {
				LOG_ERR("Unable to find a valid server instance.");
				goto bootstrap_or_retry;
			}

			client.ctx->srv_obj_inst = srv;
			sm_update_lifetime(srv, &client.lifetime);

			ret = lwm2m_get_u16(&LWM2M_OBJ(1, client.ctx->srv_obj_inst, 0), &ssid);
			if (ret < 0) {
				LOG_ERR("Failed to read SSID");
				lwm2m_server_disable(srv, K_FOREVER);
				goto bootstrap_or_retry;
			}

			ret = lwm2m_security_short_id_to_inst(ssid);
			if (ret < 0) {
				LOG_ERR("Unable to find a valid security instance.");
				lwm2m_server_disable(srv, K_FOREVER);
				goto bootstrap_or_retry;
			}
			client.ctx->sec_obj_inst = (uint16_t) ret;
		}

		LOG_INF("RD Client started with endpoint '%s' with client lifetime %d using server "
			"object %d",
			client.ep_name, client.lifetime, client.ctx->srv_obj_inst);

		ret = lwm2m_engine_start(client.ctx);
		if (ret < 0) {
			LOG_ERR("Cannot init LWM2M engine (%d)", ret);
			goto retry;
		}
	}

	sm_send_registration_msg();
	return;

bootstrap_or_retry:
	if (!client.server_disabled && fallback_to_bootstrap()) {
		lwm2m_engine_stop(client.ctx);
		return;
	}
retry:
	lwm2m_engine_stop(client.ctx);
	set_sm_state(ENGINE_NETWORK_ERROR);
}

static int64_t next_update(void)
{
	int64_t next;
	int64_t period = CONFIG_LWM2M_UPDATE_PERIOD;
	int64_t early = CONFIG_LWM2M_SECONDS_TO_UPDATE_EARLY;

	if (period == 0) {
		period = client.lifetime;
	}
	if (early > client.lifetime) {
		early = client.lifetime;
	}

	next = MIN(period, client.lifetime - early);
	next = MAX(next, MINIMUM_PERIOD);

	return client.last_update + next * MSEC_PER_SEC;
}

static int64_t next_rx_off(void)
{
	if (IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_ENABLED)) {
		return client.last_tx + CONFIG_LWM2M_QUEUE_MODE_UPTIME * MSEC_PER_SEC;
	} else {
		return next_update();
	}
}

/** Return timestamp to next even whether it is RX_OFF or update event */
static int64_t calc_next_event(void)
{
	return Z_MIN(next_update(), next_rx_off());
}

static void sm_registration_done(void)
{
	k_mutex_lock(&client.mutex, K_FOREVER);

	int64_t now = k_uptime_get();

	if (sm_is_registered() &&
	    (client.trigger_update ||
	     now >= next_update())) {
		set_sm_state_delayed(ENGINE_UPDATE_REGISTRATION, DELAY_FOR_ACK);
	} else if (IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_ENABLED) &&
	    (client.engine_state != ENGINE_REGISTRATION_DONE_RX_OFF) &&
	    (now >= next_rx_off())) {
		set_sm_state(ENGINE_REGISTRATION_DONE_RX_OFF);
		next_event_at(next_update());
	} else {
		next_event_at(calc_next_event());
	}
	k_mutex_unlock(&client.mutex);
}

static int update_registration(void)
{
	int ret;
	bool update_objects;

	update_objects = client.update_objects;
	client.trigger_update = false;
	client.update_objects = false;

	ret = lwm2m_engine_connection_resume(client.ctx);
	if (ret) {
		return ret;
	}

	ret = sm_send_registration(update_objects,
				   do_update_reply_cb,
				   do_update_timeout_cb);
	if (ret) {
		LOG_ERR("Registration update err: %d", ret);
		return ret;
	}

	return 0;
}

static int sm_update_registration(void)
{
	int ret;

	ret = update_registration();
	if (ret) {
		LOG_ERR("Failed to update registration. Falling back to full registration");

		lwm2m_engine_stop(client.ctx);
		/* perform full registration */
		set_sm_state(ENGINE_DO_REGISTRATION);
		return ret;
	}

	set_sm_state(ENGINE_UPDATE_SENT);

	return 0;
}

static int sm_do_deregister(void)
{
	struct lwm2m_message *msg;
	int ret;

	if (lwm2m_engine_connection_resume(client.ctx)) {
		lwm2m_engine_context_close(client.ctx);
		/* Connection failed, enter directly to deregistered state */
		set_sm_state(ENGINE_DEREGISTERED);
		return 0;
	}

	msg = rd_get_message();
	if (!msg) {
		LOG_ERR("Unable to get a lwm2m message!");
		ret = -ENOMEM;
		goto close_ctx;
	}

	msg->type = COAP_TYPE_CON;
	msg->code = COAP_METHOD_DELETE;
	msg->mid = coap_next_id();
	msg->tkl = LWM2M_MSG_TOKEN_GENERATE_NEW;
	msg->reply_cb = do_deregister_reply_cb;
	msg->message_timeout_cb = do_deregister_timeout_cb;

	ret = lwm2m_init_message(msg);
	if (ret) {
		goto cleanup;
	}

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					LWM2M_RD_CLIENT_URI,
					strlen(LWM2M_RD_CLIENT_URI));
	if (ret < 0) {
		LOG_ERR("Failed to encode URI path option (err:%d).", ret);
		goto cleanup;
	}

	/* include server endpoint in URI PATH */
	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_PATH,
					client.server_ep,
					strlen(client.server_ep));
	if (ret < 0) {
		LOG_ERR("Failed to encode URI path option (err:%d).", ret);
		goto cleanup;
	}

	LOG_INF("Deregister from '%s'", client.server_ep);

	lwm2m_send_message_async(msg);

	set_sm_state(ENGINE_DEREGISTER_SENT);
	return 0;

cleanup:
	lwm2m_reset_message(msg, true);
close_ctx:
	lwm2m_engine_stop(client.ctx);
	set_sm_state(ENGINE_DEREGISTERED);
	return ret;
}

static bool fallback_to_bootstrap(void)
{
	if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)) {
		bool fallback = true;

		(void)lwm2m_get_bool(&LWM2M_OBJ(LWM2M_OBJECT_SERVER_ID, client.ctx->srv_obj_inst,
						SERVER_BOOTSTRAP_ON_REGISTRATION_FAILURE_ID),
				     &fallback);
		if (fallback) {
			client.use_bootstrap = true;
			set_sm_state(ENGINE_INIT);
			return true;
		}
	}
	return false;
}

static void sm_do_network_error(void)
{
	int err;

	LOG_ERR("sm_do_network_error, retries %d", client.retries);

	lwm2m_socket_close(client.ctx);

	if (client.retry_delay) {
		next_event_at(k_uptime_get() + client.retry_delay * MSEC_PER_SEC);
		client.retry_delay = 0;
		return;
	}
	client.retry_delay = 1 << client.retries;
	client.retries++;

	/* Stop retrying and try fallback */
	if (client.retries > CONFIG_LWM2M_RD_CLIENT_MAX_RETRIES) {
		LOG_ERR("Network error, max retries reached (%d)", client.retries);

		/* Disable current server for a period so lwm2m_server_select() does not pick it */
		if (client.ctx->srv_obj_inst > -1) {
			lwm2m_server_disable(client.ctx->srv_obj_inst, DISABLE_TIMEOUT);
		}

		/* Are we in bootstrap? Try if we can fallback to some other BS server */
		if (client.ctx->bootstrap_mode &&
		    IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)) {
			LOG_DBG("In bootstrap, try fallback srv");
			/* Do we have any other bootstrap server to back off to? */
			if (sm_next_bootstrap_inst(&client.ctx->sec_obj_inst) < 0) {
				/* No, we are out of options, stop engine */
				goto stop_engine;
			}
			set_sm_state(ENGINE_INIT);
			return;
		}

		/* Try if there are other server to fall back to,
		 * Only allow fallback to higher priority server (lower value, or lower id)
		 * if we have successfully registered before.
		 * This should block us from looping the same list again.
		 * Instead we should fallback to bootstrap.
		 */
		uint16_t srv;

		if (lwm2m_server_select(&srv)) {
			uint8_t p1, p2;

			p1 = lwm2m_server_get_prio(client.ctx->srv_obj_inst);
			p2 = lwm2m_server_get_prio(srv);
			if (p1 < p2 || client.last_update != 0) {
				set_sm_state(ENGINE_INIT);
				return;
			}
		}

		/* If we have been disabled by some server, don't fall back to bootstrap */
		if (client.server_disabled) {
			set_sm_state(ENGINE_SERVER_DISABLED);
			return;
		}

		if (fallback_to_bootstrap()) {
			return;
		}
		goto stop_engine;
	}

	/* Retry bootstrap */
	if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)) {
		if (client.ctx->bootstrap_mode) {
			lwm2m_engine_context_close(client.ctx);
			/* If we don't have fallback BS server, retry with current one */
			if (sm_next_bootstrap_inst(&client.ctx->sec_obj_inst) < 0) {
				client.ctx->sec_obj_inst = -1;
			}
			set_sm_state(ENGINE_DO_BOOTSTRAP_REG);
			return;
		}
	}

	if (!client.last_update ||
	    (k_uptime_get() - client.last_update) / MSEC_PER_SEC > client.lifetime) {
		/* do full registration as there is no active registration or lifetime exceeded */
		/* Keep the same server until out of retry */
		set_sm_state(ENGINE_DO_REGISTRATION);
		return;
	}

	/* Try if we can recover the DTLS session and try Update.
	 * This might fallback into full registration on sm_handle_registration_update_failure().
	 */
	err = lwm2m_socket_start(client.ctx);
	if (err) {
		LOG_ERR("Failed to start socket %d", err);
		/*
		 * keep this state until lifetime/retry count exceeds. Renew
		 * sm state to set retry_delay etc ...
		 */
		set_sm_state(ENGINE_NETWORK_ERROR);
		return;
	}
	set_sm_state(ENGINE_UPDATE_REGISTRATION);
	return;

stop_engine:

	/* We are out of options, stop engine */
	if (client.ctx->event_cb) {
		if (client.ctx->bootstrap_mode) {
			client.ctx->event_cb(client.ctx,
					     LWM2M_RD_CLIENT_EVENT_BOOTSTRAP_REG_FAILURE);
		} else {
			client.ctx->event_cb(client.ctx, LWM2M_RD_CLIENT_EVENT_NETWORK_ERROR);
		}
	}
	set_sm_state(ENGINE_IDLE);
}

static void lwm2m_rd_client_service(struct k_work *work)
{
	k_mutex_lock(&client.mutex, K_FOREVER);

	int64_t timeout = 0;

	if (client.ctx) {
		LOG_DBG("State: %d", get_sm_state());
		client.next_event = INT64_MAX;
		switch (get_sm_state()) {
		case ENGINE_IDLE:
			if (client.ctx->sock_fd > -1) {
				lwm2m_engine_stop(client.ctx);
			}
			rd_client_message_free();
			break;

		case ENGINE_INIT:
			sm_do_init();
			break;

		case ENGINE_SUSPENDED:
			break;

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
		case ENGINE_DO_BOOTSTRAP_REG:
			sm_do_bootstrap_reg();
			break;

		case ENGINE_BOOTSTRAP_REG_SENT:
			/* wait for bootstrap registration done */
			timeout = EXCHANGE_LIFETIME;
			break;

		case ENGINE_BOOTSTRAP_REG_DONE:
			/* wait for transfer done */
			timeout = EXCHANGE_LIFETIME;
			break;

		case ENGINE_BOOTSTRAP_TRANS_DONE:
			sm_bootstrap_trans_done();
			break;
#endif

		case ENGINE_DO_REGISTRATION:
			sm_do_registration();
			break;

		case ENGINE_SEND_REGISTRATION:
			sm_send_registration_msg();
			break;

		case ENGINE_REGISTRATION_SENT:
			/* wait registration to be done or timeout */
			timeout = EXCHANGE_LIFETIME;
			break;

		case ENGINE_REGISTRATION_DONE:
		case ENGINE_REGISTRATION_DONE_RX_OFF:
			sm_registration_done();
			break;

		case ENGINE_UPDATE_REGISTRATION:
			sm_update_registration();
			break;

		case ENGINE_UPDATE_SENT:
			/* wait update to be done or abort */
			timeout = EXCHANGE_LIFETIME;
			break;

		case ENGINE_SERVER_DISABLED:
			if (lwm2m_server_select(NULL)) {
				set_sm_state(ENGINE_INIT);
			} else {
				/* wait for server to be enabled. */
				/*
				 * TODO: Once engine is converted to use timepoint_t
				 * this should calculate the next event from the previous server.
				 */
				next_event_at(k_uptime_get() + SEC_PER_MIN * MSEC_PER_SEC);
			}
			break;

		case ENGINE_DEREGISTER:
			sm_do_deregister();
			break;

		case ENGINE_DEREGISTER_SENT:
			/* wait for deregister to be done or reset */
			timeout = EXCHANGE_LIFETIME;
			break;

		case ENGINE_DEREGISTERED:
			lwm2m_engine_stop(client.ctx);
			if (client.server_disabled) {
				set_sm_state(ENGINE_SERVER_DISABLED);
			} else {
				set_sm_state(ENGINE_IDLE);
			}
			break;

		case ENGINE_NETWORK_ERROR:
			sm_do_network_error();
			break;

		default:
			LOG_ERR("Unhandled state: %d", get_sm_state());

		}

		if (timeout) {
			int64_t end = client.last_state_change + timeout * MSEC_PER_SEC;

			if (end < k_uptime_get()) {
				LOG_DBG("State machine have timed out");
				sm_handle_timeout_state(ENGINE_INIT);
			} else if (client.next_event > end) {
				next_event_at(end);
			}
		}
	}

	k_mutex_unlock(&client.mutex);
}

int lwm2m_rd_client_start(struct lwm2m_ctx *client_ctx, const char *ep_name,
			   uint32_t flags, lwm2m_ctx_event_cb_t event_cb,
			   lwm2m_observe_cb_t observe_cb)
{
	k_mutex_lock(&client.mutex, K_FOREVER);

	if (!IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP) &&
	    (flags & LWM2M_RD_CLIENT_FLAG_BOOTSTRAP)) {
		LOG_ERR("Bootstrap support is disabled. Please enable "
			"CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP.");

		k_mutex_unlock(&client.mutex);
		return -ENOTSUP;
	}

	/* Check client idle state or socket is still active */

	if (client.ctx && (client.engine_state != ENGINE_IDLE || client.ctx->sock_fd != -1)) {
		LOG_WRN("Client is already running. state %d ", client.engine_state);
		k_mutex_unlock(&client.mutex);
		return -EINPROGRESS;
	}

	/* Init Context */
	lwm2m_server_reset_timestamps();
	lwm2m_engine_context_init(client_ctx);

	client.ctx = client_ctx;
	client.ctx->sock_fd = -1;
	client.ctx->fault_cb = socket_fault_cb;
	client.ctx->observe_cb = observe_cb;
	client.ctx->event_cb = event_cb;
	client.use_bootstrap = flags & LWM2M_RD_CLIENT_FLAG_BOOTSTRAP;
	client.ctx->srv_obj_inst = -1;
	client.ctx->sec_obj_inst = -1;
	client.retries = 0;

	strncpy(client.ep_name, ep_name, CLIENT_EP_LEN - 1);
	client.ep_name[CLIENT_EP_LEN - 1] = '\0';
	LOG_INF("Start LWM2M Client: %s", client.ep_name);

	set_sm_state(ENGINE_INIT);

	k_mutex_unlock(&client.mutex);

	return 0;
}

int lwm2m_rd_client_stop(struct lwm2m_ctx *client_ctx,
			   lwm2m_ctx_event_cb_t event_cb, bool deregister)
{
	k_mutex_lock(&client.mutex, K_FOREVER);

	if (client.ctx != client_ctx) {
		k_mutex_unlock(&client.mutex);
		LOG_WRN("Cannot stop. Wrong context");
		return -EPERM;
	}

	client.ctx->event_cb = event_cb;
	rd_client_message_free();

	if (sm_is_registered() && deregister && !client.server_disabled) {
		set_sm_state(ENGINE_DEREGISTER);
	} else {
		client.server_disabled = false;
		set_sm_state(ENGINE_DEREGISTERED);
	}

	LOG_INF("Stop LWM2M Client: %s", client.ep_name);

	k_mutex_unlock(&client.mutex);


	return 0;
}

int lwm2m_rd_client_pause(void)
{
	enum lwm2m_rd_client_event event = LWM2M_RD_CLIENT_EVENT_ENGINE_SUSPENDED;
	LOG_DBG("lwm2m_rd_client_pause()");

	k_mutex_lock(&client.mutex, K_FOREVER);

	if (!client.ctx) {
		k_mutex_unlock(&client.mutex);
		LOG_ERR("Cannot pause. No context");
		return -EPERM;
	} else if (sm_is_suspended()) {
		k_mutex_unlock(&client.mutex);
		LOG_ERR("LwM2M client already suspended");
		return 0;
	}

	LOG_INF("Suspend client");
	if (client.ctx->event_cb) {
		client.ctx->event_cb(client.ctx, event);
	}

	/* Suspend or close the socket */
	if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_CLOSE_SOCKET_AT_IDLE)) {
		lwm2m_close_socket(client.ctx);
	} else {
		lwm2m_socket_suspend(client.ctx);
	}

	suspended_client_state = get_sm_state();
	set_sm_state(ENGINE_SUSPENDED);

	k_mutex_unlock(&client.mutex);

	return 0;
}

int lwm2m_rd_client_resume(void)
{
	k_mutex_lock(&client.mutex, K_FOREVER);

	if (!client.ctx || !lwm2m_rd_client_is_suspended(client.ctx)) {
		k_mutex_unlock(&client.mutex);
		LOG_WRN("Cannot resume, state is not suspended");
		return -EPERM;
	}

	LOG_INF("Resume Client state");

	if (suspended_client_state == ENGINE_UPDATE_SENT) {
		/* Set back to Registration done and trigger an update */
		suspended_client_state = ENGINE_REGISTRATION_DONE;
	}
	/* Clear Possible pending RD Client message */
	rd_client_message_free();

	client.engine_state = suspended_client_state;

	/* Do we need to resume the bootstrap? */
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
	if (sm_is_bootstrap()) {
		client.engine_state = ENGINE_DO_BOOTSTRAP_REG;
	}
#endif
	/* Or do we resume into registration state */
	if (client.engine_state >= ENGINE_DO_REGISTRATION &&
		client.engine_state <= ENGINE_SERVER_DISABLED) {
		if (!client.last_update ||
			(client.lifetime <= (k_uptime_get() - client.last_update) / MSEC_PER_SEC)) {
			/* No lifetime left, register again */
			client.engine_state = ENGINE_DO_REGISTRATION;
		} else {
			/* Resume similarly like from QUEUE mode */
			client.engine_state = ENGINE_REGISTRATION_DONE_RX_OFF;
			lwm2m_rd_client_connection_resume(client.ctx);
		}
	}

	next_event_at(0);
	k_mutex_unlock(&client.mutex);

	return 0;
}

int lwm2m_rd_client_server_disabled(uint16_t inst_id)
{
	if (client.ctx->srv_obj_inst != inst_id) {
		return -EPERM;
	}

	k_mutex_lock(&client.mutex, K_FOREVER);

	client.server_disabled = true;

	if (sm_is_registered()) {
		LOG_INF("Server disabled, deregister");
		set_sm_state_delayed(ENGINE_DEREGISTER, DELAY_BEFORE_CLOSING);
	} else {
		LOG_INF("Server disabled");
		set_sm_state(ENGINE_DEREGISTERED);
	}

	k_mutex_unlock(&client.mutex);

	return 0;
}

void lwm2m_rd_client_update(void)
{
	engine_trigger_update(false);
}

struct lwm2m_ctx *lwm2m_rd_client_ctx(void)
{
	return client.ctx;
}

void lwm2m_rd_client_set_ctx(struct lwm2m_ctx *ctx)
{
	client.ctx = ctx;
}

int lwm2m_rd_client_connection_resume(struct lwm2m_ctx *client_ctx)
{
	if (client.ctx != client_ctx) {
		return -EPERM;
	}

	if (client.engine_state == ENGINE_REGISTRATION_DONE_RX_OFF) {
		/*
		 * Switch state to triggering a proper registration message
		 * If the socket stays open (Connection ID or no-sec), or we have TLS session cache,
		 * we can trigger the update, otherwise fall back to full registration.
		 */
		if ((IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_SUSPEND_SOCKET_AT_IDLE) &&
		     IS_ENABLED(CONFIG_LWM2M_TLS_SESSION_CACHING)) ||
		    (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_STOP_POLLING_AT_IDLE) ||
		     IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_LISTEN_AT_IDLE)) ||
		    !IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT)) {
			client.engine_state = ENGINE_REGISTRATION_DONE;
			if (IS_ENABLED(CONFIG_LWM2M_QUEUE_MODE_NO_MSG_BUFFERING)) {
				/* Force online for a short period */
				engine_update_tx_time();
			} else {
				client.trigger_update = true;
			}
		} else {
			client.engine_state = ENGINE_DO_REGISTRATION;
		}
		next_event_at(0);
	}

	return 0;
}

int lwm2m_rd_client_timeout(struct lwm2m_ctx *client_ctx)
{
	if (client.ctx != client_ctx) {
		return -EPERM;
	}

	if (!sm_is_registered()) {
		return 0;
	}
	k_mutex_lock(&client.mutex, K_FOREVER);
	LOG_WRN("Confirmable Timeout -> Re-connect and register");
	set_sm_state(ENGINE_DO_REGISTRATION);
	next_event_at(0);
	k_mutex_unlock(&client.mutex);
	return 0;
}

bool lwm2m_rd_client_is_registred(struct lwm2m_ctx *client_ctx)
{
	if (client.ctx != client_ctx || !sm_is_registered()) {
		return false;
	}

	return true;
}
bool lwm2m_rd_client_is_suspended(struct lwm2m_ctx *client_ctx)
{
	if (client.ctx != client_ctx || !sm_is_suspended()) {
		return false;
	}

	return true;
}


int lwm2m_rd_client_init(void)
{
	client.ctx = NULL;
	client.rd_message.ctx = NULL;
	client.engine_state = ENGINE_IDLE;
	k_mutex_init(&client.mutex);

	return 0;
}

static int sys_lwm2m_rd_client_init(void)
{
	return lwm2m_rd_client_init();
}

LWM2M_ENGINE_INIT(sys_lwm2m_rd_client_init);
