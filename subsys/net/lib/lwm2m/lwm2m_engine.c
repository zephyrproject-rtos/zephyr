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

#define LOG_MODULE_NAME net_lwm2m_engine
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
#include <zephyr/net/http/parser_url.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/posix/fcntl.h>

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#include <zephyr/net/tls_credentials.h>
#include <mbedtls/ssl_ciphersuites.h>
#endif
#if defined(CONFIG_DNS_RESOLVER)
#include <zephyr/net/dns_resolve.h>
#endif

#include "lwm2m_engine.h"
#include "lwm2m_object.h"
#include "lwm2m_rw_link_format.h"
#include "lwm2m_rw_oma_tlv.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_util.h"
#include "lwm2m_rd_client.h"
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

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#endif

#define ENGINE_SLEEP_MS 500

#ifdef CONFIG_LWM2M_VERSION_1_1
#define LWM2M_ENGINE_MAX_OBSERVER_PATH CONFIG_LWM2M_ENGINE_MAX_OBSERVER * 3
#else
#define LWM2M_ENGINE_MAX_OBSERVER_PATH CONFIG_LWM2M_ENGINE_MAX_OBSERVER
#endif
static struct lwm2m_obj_path_list observe_paths[LWM2M_ENGINE_MAX_OBSERVER_PATH];
#define MAX_PERIODIC_SERVICE 10

static k_tid_t engine_thread_id;
static bool suspend_engine_thread;
static bool active_engine_thread;

struct service_node {
	sys_snode_t node;
	k_work_handler_t service_work;
	uint32_t call_period; /* ms */
	int64_t next_timestamp;  /* ms */
};

static struct service_node service_node_data[MAX_PERIODIC_SERVICE];
static sys_slist_t engine_service_list;

static K_KERNEL_STACK_DEFINE(engine_thread_stack, CONFIG_LWM2M_ENGINE_STACK_SIZE);
static struct k_thread engine_thread_data;

#define MAX_POLL_FD CONFIG_NET_SOCKETS_POLL_MAX

/* Resources */
static struct zsock_pollfd sock_fds[MAX_POLL_FD];

static struct lwm2m_ctx *sock_ctx[MAX_POLL_FD];
static int sock_nfds;
static int control_sock;

/* Resource wrappers */
#if defined(CONFIG_LWM2M_COAP_BLOCK_TRANSFER)
static struct coap_block_context output_block_contexts[NUM_OUTPUT_BLOCK_CONTEXT];
#endif

/* Resource wrappers */
struct lwm2m_ctx **lwm2m_sock_ctx(void) { return sock_ctx; }

int lwm2m_sock_nfds(void) { return sock_nfds; }

#if defined(CONFIG_LWM2M_COAP_BLOCK_TRANSFER)
struct coap_block_context *lwm2m_output_block_context(void) { return output_block_contexts; }
#endif

static int lwm2m_socket_update(struct lwm2m_ctx *ctx);

/* utility functions */

void lwm2m_engine_wake_up(void)
{
	if (IS_ENABLED(CONFIG_LWM2M_TICKLESS)) {
		zsock_send(control_sock, &(char){0}, 1, 0);
	}
}

int lwm2m_open_socket(struct lwm2m_ctx *client_ctx)
{
	if (client_ctx->sock_fd < 0) {
		/* open socket */

		if (IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) && client_ctx->use_dtls) {
			client_ctx->sock_fd = zsock_socket(client_ctx->remote_addr.sa_family,
							   SOCK_DGRAM, IPPROTO_DTLS_1_2);
		} else {
			client_ctx->sock_fd =
				zsock_socket(client_ctx->remote_addr.sa_family, SOCK_DGRAM,
					     IPPROTO_UDP);
		}

		if (client_ctx->sock_fd < 0) {
			LOG_ERR("Failed to create socket: %d", errno);
			return -errno;
		}

		if (lwm2m_socket_update(client_ctx)) {
			return lwm2m_socket_add(client_ctx);
		}
	}

	return 0;
}

int lwm2m_close_socket(struct lwm2m_ctx *client_ctx)
{
	if (client_ctx->sock_fd >= 0) {
		int ret = zsock_close(client_ctx->sock_fd);

		if (ret) {
			LOG_ERR("Failed to close socket: %d", errno);
			ret = -errno;
			return ret;
		}
	}

	client_ctx->sock_fd = -1;
	client_ctx->connection_suspended = true;
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
	/* Enable Queue mode buffer store */
	client_ctx->buffer_client_messages = true;
#endif
	lwm2m_socket_update(client_ctx);

	return 0;
}

int lwm2m_socket_suspend(struct lwm2m_ctx *client_ctx)
{
	int ret = 0;

	if (client_ctx->sock_fd >= 0 && !client_ctx->connection_suspended) {
		int socket_temp_id = client_ctx->sock_fd;

		/* Prevent closing */
		client_ctx->sock_fd = -1;
		/* Just mark as suspended */
		lwm2m_close_socket(client_ctx);
		/* store back the socket handle */
		client_ctx->sock_fd = socket_temp_id;

		if (client_ctx->set_socket_state) {
			client_ctx->set_socket_state(client_ctx->sock_fd,
						     LWM2M_SOCKET_STATE_NO_DATA);
		}
	}

	return ret;
}

int lwm2m_engine_connection_resume(struct lwm2m_ctx *client_ctx)
{
	int ret;

	if (client_ctx->connection_suspended) {
		if (IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_STOP_POLLING_AT_IDLE) ||
		    IS_ENABLED(CONFIG_LWM2M_RD_CLIENT_LISTEN_AT_IDLE)) {
			LOG_DBG("Resume suspended connection");
			lwm2m_socket_update(client_ctx);
			client_ctx->connection_suspended = false;
		} else {
			LOG_DBG("Close and resume a new connection");
			lwm2m_close_socket(client_ctx);
			ret = lwm2m_open_socket(client_ctx);
			if (ret) {
				return ret;
			}
			client_ctx->connection_suspended = false;
			return lwm2m_socket_start(client_ctx);
		}
	}

	return 0;
}

int lwm2m_push_queued_buffers(struct lwm2m_ctx *client_ctx)
{
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
	client_ctx->buffer_client_messages = false;
	while (!sys_slist_is_empty(&client_ctx->queued_messages)) {
		sys_snode_t *msg_node = sys_slist_get(&client_ctx->queued_messages);
		struct lwm2m_message *msg;

		if (!msg_node) {
			break;
		}
		msg = SYS_SLIST_CONTAINER(msg_node, msg, node);
		msg->pending->t0 = k_uptime_get();
		sys_slist_append(&msg->ctx->pending_sends, &msg->node);
	}
#endif
	return 0;
}

bool lwm2m_engine_bootstrap_override(struct lwm2m_ctx *client_ctx, struct lwm2m_obj_path *path)
{
	if (!client_ctx->bootstrap_mode) {
		/* Bootstrap is not active override is not possible then */
		return false;
	}

	if (path->obj_id == LWM2M_OBJECT_SECURITY_ID || path->obj_id == LWM2M_OBJECT_SERVER_ID) {
		/* Bootstrap server have a access to Security and Server object */
		return true;
	}

	return false;
}

int lwm2m_engine_validate_write_access(struct lwm2m_message *msg,
				       struct lwm2m_engine_obj_inst *obj_inst,
				       struct lwm2m_engine_obj_field **obj_field)
{
	struct lwm2m_engine_obj_field *o_f;

	o_f = lwm2m_get_engine_obj_field(obj_inst->obj, msg->path.res_id);
	if (!o_f) {
		return -ENOENT;
	}

	*obj_field = o_f;

	if (!LWM2M_HAS_PERM(o_f, LWM2M_PERM_W) &&
	    !lwm2m_engine_bootstrap_override(msg->ctx, &msg->path)) {
		return -EPERM;
	}

	if (!obj_inst->resources || obj_inst->resource_count == 0U) {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
static bool bootstrap_delete_allowed(int obj_id, int obj_inst_id)
{
	bool bootstrap_server;
	int ret;

	if (obj_id == LWM2M_OBJECT_SECURITY_ID) {
		ret = lwm2m_get_bool(&LWM2M_OBJ(LWM2M_OBJECT_SECURITY_ID, obj_inst_id, 1),
					    &bootstrap_server);
		if (ret < 0) {
			return false;
		}

		if (bootstrap_server) {
			return false;
		}
	}

	if (obj_id == LWM2M_OBJECT_DEVICE_ID) {
		return false;
	}

	return true;
}

int bootstrap_delete(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst, *tmp;
	int ret = 0;
	sys_slist_t *engine_obj_inst_list = lwm2m_engine_obj_inst_list();

	if (msg->path.level > 2) {
		return -EPERM;
	}

	if (msg->path.level == 2) {
		if (!bootstrap_delete_allowed(msg->path.obj_id, msg->path.obj_inst_id)) {
			return -EPERM;
		}

		return lwm2m_delete_obj_inst(msg->path.obj_id, msg->path.obj_inst_id);
	}

	/* DELETE all instances of a specific object or all object instances if
	 * not specified, excluding the following exceptions (according to the
	 * LwM2M specification v1.0.2, ch 5.2.7.5):
	 * - LwM2M Bootstrap-Server Account (Bootstrap Security object, ID 0)
	 * - Device object (ID 3)
	 */
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(engine_obj_inst_list, obj_inst, tmp, node) {
		if (msg->path.level == 1 && obj_inst->obj->obj_id != msg->path.obj_id) {
			continue;
		}

		if (!bootstrap_delete_allowed(obj_inst->obj->obj_id, obj_inst->obj_inst_id)) {
			continue;
		}

		ret = lwm2m_delete_obj_inst(obj_inst->obj->obj_id, obj_inst->obj_inst_id);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}
#endif

/* returns timestamp when next retransmission is due, or INT64_MAX
 * if no retransmissions are necessary
 */
static int64_t retransmit_request(struct lwm2m_ctx *client_ctx, const int64_t timestamp)
{
	struct lwm2m_message *msg;
	struct coap_pending *p;
	int64_t remaining, next = INT64_MAX;
	int i;

	for (i = 0, p = client_ctx->pendings; i < ARRAY_SIZE(client_ctx->pendings); i++, p++) {
		if (!p->timeout) {
			continue;
		}

		remaining = p->t0 + p->timeout;
		if (remaining < timestamp) {
			msg = find_msg(p, NULL);
			if (!msg) {
				LOG_ERR("pending has no valid LwM2M message!");
				coap_pending_clear(p);
				continue;
			}
			if (!p->retries) {
				/* pending request has expired */
				if (msg->message_timeout_cb) {
					msg->message_timeout_cb(msg);
				}
				lwm2m_reset_message(msg, true);
				continue;
			}
			if (msg->acknowledged) {
				/* No need to retransmit, just keep the timer running to
				 * timeout in case no response arrives.
				 */
				coap_pending_cycle(p);
				continue;
			}

			lwm2m_send_message_async(msg);
			break;
		}
		if (remaining < next) {
			next = remaining;
		}
	}

	return next;
}
static int64_t engine_next_service_timestamp(void)
{
	struct service_node *srv;
	int64_t next = INT64_MAX;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_service_list, srv, node) {
		if (srv->next_timestamp < next) {
			next = srv->next_timestamp;
		}
	}

	return next;
}

static int engine_add_srv(k_work_handler_t service, uint32_t period_ms, int64_t next)
{
	int i;

	if (!service) {
		return -EINVAL;
	}

	/* First, try if the service is already registered, and modify it*/
	if (lwm2m_engine_update_service_period(service, period_ms) == 0) {
		return 0;
	}

	/* find an unused service index node */
	for (i = 0; i < MAX_PERIODIC_SERVICE; i++) {
		if (!service_node_data[i].service_work) {
			break;
		}
	}

	if (i == MAX_PERIODIC_SERVICE) {
		return -ENOMEM;
	}

	service_node_data[i].service_work = service;
	service_node_data[i].call_period = period_ms;
	service_node_data[i].next_timestamp = next;

	sys_slist_append(&engine_service_list, &service_node_data[i].node);

	lwm2m_engine_wake_up();

	return 0;
}

int lwm2m_engine_add_service(k_work_handler_t service, uint32_t period_ms)
{
	return engine_add_srv(service, period_ms, k_uptime_get() + period_ms);
}

int lwm2m_engine_call_at(k_work_handler_t service, int64_t timestamp)
{
	return engine_add_srv(service, 0, timestamp);
}

int lwm2m_engine_call_now(k_work_handler_t service)
{
	return engine_add_srv(service, 0, k_uptime_get());
}

int lwm2m_engine_update_service_period(k_work_handler_t service, uint32_t period_ms)
{
	int i = 0;

	for (i = 0; i < MAX_PERIODIC_SERVICE; i++) {
		if (service_node_data[i].service_work == service) {
			if (period_ms) {
				service_node_data[i].call_period = period_ms;
				service_node_data[i].next_timestamp = k_uptime_get() + period_ms;
				lwm2m_engine_wake_up();
				return 0;
			}
			sys_slist_find_and_remove(&engine_service_list, &service_node_data[i].node);
			service_node_data[i].service_work = NULL;
			return 1;
		}
	}

	return -ENOENT;
}

static int64_t lwm2m_engine_service(const int64_t timestamp)
{
	struct service_node *srv, *tmp;
	bool restart;

	do {
		restart = false;
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&engine_service_list, srv, tmp, node) {
			/* service is due */
			if (timestamp >= srv->next_timestamp) {
				k_work_handler_t work = srv->service_work;

				if (srv->call_period) {
					srv->next_timestamp = k_uptime_get() + srv->call_period;
				} else {
					sys_slist_find_and_remove(&engine_service_list, &srv->node);
					srv->service_work = NULL;
				}
				if (work) {
					work(NULL);
				}
				/* List might have been modified by the callback */
				restart = true;
				break;
			}
		}
	} while (restart);

	/* calculate how long to sleep till the next service */
	return engine_next_service_timestamp();
}

/* LwM2M Socket Integration */

int lwm2m_socket_add(struct lwm2m_ctx *ctx)
{
	if (IS_ENABLED(CONFIG_LWM2M_TICKLESS)) {
		/* Last poll-handle is reserved for control socket */
		if (sock_nfds >= (MAX_POLL_FD - 1)) {
			return -ENOMEM;
		}
	} else {
		if (sock_nfds >= MAX_POLL_FD) {
			return -ENOMEM;
		}
	}

	sock_ctx[sock_nfds] = ctx;
	sock_fds[sock_nfds].fd = ctx->sock_fd;
	sock_fds[sock_nfds].events = ZSOCK_POLLIN;
	sock_nfds++;

	lwm2m_engine_wake_up();

	return 0;
}

static int lwm2m_socket_update(struct lwm2m_ctx *ctx)
{
	for (int i = 0; i < sock_nfds; i++) {
		if (sock_ctx[i] != ctx) {
			continue;
		}
		sock_fds[i].fd = ctx->sock_fd;
		lwm2m_engine_wake_up();
		return 0;
	}
	return -1;
}

void lwm2m_socket_del(struct lwm2m_ctx *ctx)
{
	for (int i = 0; i < sock_nfds; i++) {
		if (sock_ctx[i] != ctx) {
			continue;
		}

		sock_nfds--;

		/* If not last, overwrite the entry with the last one. */
		if (i < sock_nfds) {
			sock_ctx[i] = sock_ctx[sock_nfds];
			sock_fds[i].fd = sock_fds[sock_nfds].fd;
			sock_fds[i].events = sock_fds[sock_nfds].events;
		}

		/* Remove the last entry. */
		sock_ctx[sock_nfds] = NULL;
		sock_fds[sock_nfds].fd = -1;
		break;
	}
	lwm2m_engine_wake_up();
}

/* Generate notify messages. Return timestamp of next Notify event */
static int64_t check_notifications(struct lwm2m_ctx *ctx, const int64_t timestamp)
{
	struct observe_node *obs;
	int rc;
	int64_t next = INT64_MAX;

	lwm2m_registry_lock();
	SYS_SLIST_FOR_EACH_CONTAINER(&ctx->observer, obs, node) {
		if (!obs->event_timestamp) {
			continue;
		}

		if (obs->event_timestamp < next) {
			next = obs->event_timestamp;
		}

		if (timestamp < obs->event_timestamp) {
			continue;
		}
		/* Check That There is not pending process*/
		if (obs->active_notify != NULL) {
			continue;
		}

		rc = generate_notify_message(ctx, obs, NULL);
		if (rc == -ENOMEM) {
			/* no memory/messages available, retry later */
			goto cleanup;
		}
		obs->event_timestamp =
			engine_observe_shedule_next_event(obs, ctx->srv_obj_inst, timestamp);
		obs->last_timestamp = timestamp;

		if (!rc) {
			/* create at most one notification */
			goto cleanup;
		}
	}
cleanup:
	lwm2m_registry_unlock();
	return next;
}

/**
 * @brief Check TX queue states as well as number or pending CoAP transmissions.
 *
 * If all queues are empty and there is no packet we are currently transmitting and no
 * CoAP responses (pendings) we are waiting, inform the application by a callback
 * that socket is in state LWM2M_SOCKET_STATE_NO_DATA.
 * Otherwise, before sending a packet, depending on the state of the queues, inform with
 * one of the ONGOING, ONE_RESPONSE or LAST indicators.
 *
 * @param ctx Client context.
 * @param ongoing_tx Current packet to be transmitted or NULL.
 */
static void hint_socket_state(struct lwm2m_ctx *ctx, struct lwm2m_message *ongoing_tx)
{
	if (!ctx || !ctx->set_socket_state) {
		return;
	}

#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
	bool empty = sys_slist_is_empty(&ctx->pending_sends) &&
		     sys_slist_is_empty(&ctx->queued_messages);
#else
	bool empty = sys_slist_is_empty(&ctx->pending_sends);
#endif
	size_t pendings = coap_pendings_count(ctx->pendings, ARRAY_SIZE(ctx->pendings));

	if (ongoing_tx) {
		/* Check if more than current TX is in pendings list*/
		if (pendings > 1) {
			empty = false;
		}

		bool ongoing_block_tx = coap_block_has_more(&ongoing_tx->cpkt);

		if (!empty || ongoing_block_tx) {
			ctx->set_socket_state(ctx->sock_fd, LWM2M_SOCKET_STATE_ONGOING);
		} else if (ongoing_tx->type == COAP_TYPE_CON) {
			ctx->set_socket_state(ctx->sock_fd, LWM2M_SOCKET_STATE_ONE_RESPONSE);
		} else {
			ctx->set_socket_state(ctx->sock_fd, LWM2M_SOCKET_STATE_LAST);
		}
	} else if (empty && pendings == 0) {
		ctx->set_socket_state(ctx->sock_fd, LWM2M_SOCKET_STATE_NO_DATA);
	}
}

static int socket_recv_message(struct lwm2m_ctx *client_ctx)
{
	static uint8_t in_buf[NET_IPV6_MTU];
	socklen_t from_addr_len;
	ssize_t len;
	static struct sockaddr from_addr;

	from_addr_len = sizeof(from_addr);
	len = zsock_recvfrom(client_ctx->sock_fd, in_buf, sizeof(in_buf) - 1, ZSOCK_MSG_DONTWAIT,
			     &from_addr, &from_addr_len);

	if (len < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return -errno;
		}

		LOG_ERR("Error reading response: %d", errno);
		if (client_ctx->fault_cb != NULL) {
			client_ctx->fault_cb(errno);
		}
		return -errno;
	}

	if (len == 0) {
		LOG_ERR("Zero length recv");
		return 0;
	}

	in_buf[len] = 0U;
	lwm2m_udp_receive(client_ctx, in_buf, len, &from_addr);

	return 0;
}

static int socket_send_message(struct lwm2m_ctx *ctx)
{
	int rc;
	sys_snode_t *msg_node = sys_slist_get(&ctx->pending_sends);
	struct lwm2m_message *msg;

	if (!msg_node) {
		return 0;
	}

	msg = SYS_SLIST_CONTAINER(msg_node, msg, node);
	if (!msg || !msg->ctx) {
		LOG_ERR("LwM2M message is invalid.");
		return -EINVAL;
	}

	if (msg->type == COAP_TYPE_CON) {
		coap_pending_cycle(msg->pending);
	}

	hint_socket_state(ctx, msg);

	rc = zsock_send(msg->ctx->sock_fd, msg->cpkt.data, msg->cpkt.offset, 0);

	if (rc < 0) {
		LOG_ERR("Failed to send packet, err %d", errno);
		rc = -errno;
	} else {
		engine_update_tx_time();
	}

	if (msg->type != COAP_TYPE_CON) {
		if (!lwm2m_outgoing_is_part_of_blockwise(msg)) {
			lwm2m_reset_message(msg, true);
		}
	}

	return rc;
}

static void socket_reset_pollfd_events(void)
{
	for (int i = 0; i < MAX_POLL_FD; ++i) {
		sock_fds[i].events =
			ZSOCK_POLLIN |
			(!sock_ctx[i] || sys_slist_is_empty(&sock_ctx[i]->pending_sends)
				 ? 0
				 : ZSOCK_POLLOUT);
		sock_fds[i].revents = 0;
	}
}

/* LwM2M main work loop */
static void socket_loop(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int i, rc;
	int64_t now, next;
	int64_t timeout, next_tx;
	bool rd_client_paused;

	while (1) {
		rd_client_paused = false;
		/* Check is Thread Suspend Requested */
		if (suspend_engine_thread) {
			rc = lwm2m_rd_client_pause();
			if (rc == 0) {
				rd_client_paused = true;
			} else {
				LOG_ERR("Could not pause RD client");
			}

			suspend_engine_thread = false;
			active_engine_thread = false;
			k_thread_suspend(engine_thread_id);
			active_engine_thread = true;

			if (rd_client_paused) {
				rc = lwm2m_rd_client_resume();
				if (rc < 0) {
					LOG_ERR("Could not resume RD client");
				}
			}
		}

		now = k_uptime_get();
		next = lwm2m_engine_service(now);

		for (i = 0; i < sock_nfds; ++i) {
			if (sock_ctx[i] == NULL) {
				continue;
			}
			if (!sys_slist_is_empty(&sock_ctx[i]->pending_sends)) {
				continue;
			}
			next_tx = retransmit_request(sock_ctx[i], now);
			if (next_tx < next) {
				next = next_tx;
			}
			if (lwm2m_rd_client_is_registred(sock_ctx[i])) {
				next_tx = check_notifications(sock_ctx[i], now);
				if (next_tx < next) {
					next = next_tx;
				}
			}
		}

		socket_reset_pollfd_events();

		timeout = next > now ? next - now : 0;
		if (IS_ENABLED(CONFIG_LWM2M_TICKLESS)) {
			/* prevent roll-over */
			timeout = timeout > INT32_MAX ? INT32_MAX : timeout;
		} else {
			timeout = timeout > ENGINE_SLEEP_MS ? ENGINE_SLEEP_MS : timeout;
		}

		rc = zsock_poll(sock_fds, MAX_POLL_FD, timeout);
		if (rc < 0) {
			LOG_ERR("Error in poll:%d", errno);
			errno = 0;
			k_msleep(ENGINE_SLEEP_MS);
			continue;
		}

		for (i = 0; i < MAX_POLL_FD; i++) {

			if (sock_fds[i].revents & ZSOCK_POLLIN && sock_fds[i].fd != -1 &&
			    sock_ctx[i] == NULL) {
				/* This is the control socket, just read and ignore the data */
				char tmp;

				zsock_recv(sock_fds[i].fd, &tmp, 1, 0);
				continue;
			}
			if (sock_ctx[i] != NULL && sock_ctx[i]->sock_fd < 0) {
				continue;
			}

			if ((sock_fds[i].revents & ZSOCK_POLLERR) ||
			    (sock_fds[i].revents & ZSOCK_POLLNVAL) ||
			    (sock_fds[i].revents & ZSOCK_POLLHUP)) {
				LOG_ERR("Poll reported a socket error, %02x.", sock_fds[i].revents);
				if (sock_ctx[i] != NULL && sock_ctx[i]->fault_cb != NULL) {
					sock_ctx[i]->fault_cb(EIO);
				}
				continue;
			}

			if (sock_fds[i].revents & ZSOCK_POLLIN) {
				while (sock_ctx[i]) {
					rc = socket_recv_message(sock_ctx[i]);
					if (rc) {
						break;
					}
				}

				hint_socket_state(sock_ctx[i], NULL);
			}

			if (sock_fds[i].revents & ZSOCK_POLLOUT) {
				rc = socket_send_message(sock_ctx[i]);
				/* Drop packets that cannot be send, CoAP layer handles retry */
				/* Other fatal errors should trigger a recovery */
				if (rc < 0 && rc != -EAGAIN) {
					LOG_ERR("send() reported a socket error, %d", -rc);
					if (sock_ctx[i] != NULL && sock_ctx[i]->fault_cb != NULL) {
						sock_ctx[i]->fault_cb(-rc);
					}
				}
			}
		}
	}
}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
#if defined(CONFIG_TLS_CREDENTIALS)
static void delete_tls_credentials(sec_tag_t tag)
{
	tls_credential_delete(tag, TLS_CREDENTIAL_PSK_ID);
	tls_credential_delete(tag, TLS_CREDENTIAL_PSK);
	tls_credential_delete(tag, TLS_CREDENTIAL_SERVER_CERTIFICATE);
	tls_credential_delete(tag, TLS_CREDENTIAL_PRIVATE_KEY);
	tls_credential_delete(tag, TLS_CREDENTIAL_CA_CERTIFICATE);
}

static bool is_pem(const void *buf, size_t len)
{
	static const char pem_start[] = "-----BEGIN";

	if (len < sizeof(pem_start)) {
		return false;
	}
	if (strncmp(pem_start, (const char *) buf, sizeof(pem_start) - 1) == 0) {
		return true;
	}
	return false;
}

static int load_tls_type(struct lwm2m_ctx *client_ctx, uint16_t res_id,
			       enum tls_credential_type type)
{
	int ret = 0;
	void *cred = NULL;
	uint16_t cred_len;
	uint16_t max_len;

	ret = lwm2m_get_res_buf(&LWM2M_OBJ(0, client_ctx->sec_obj_inst, res_id), &cred, &max_len,
				&cred_len, NULL);
	if (ret < 0) {
		LOG_ERR("Unable to get resource data for %d/%d/%d", 0,  client_ctx->sec_obj_inst,
			res_id);
		return ret;
	}

	if (cred_len == 0) {
		LOG_ERR("Credential data is empty");
		return -EINVAL;
	}

	/* LwM2M registry stores strings without NULL-terminator, so we need to ensure that
	 * string based PEM credentials are terminated properly.
	 */
	if (is_pem(cred, cred_len)) {
		if (cred_len >= max_len) {
			LOG_ERR("No space for string terminator, cannot handle PEM");
			return -EINVAL;
		}
		((uint8_t *) cred)[cred_len] = 0;
		cred_len += 1;
	}

	ret = tls_credential_add(client_ctx->tls_tag, type, cred, cred_len);
	if (ret < 0) {
		LOG_ERR("Error setting cred tag %d type %d: Error %d", client_ctx->tls_tag, type,
			ret);
	}

	return ret;
}

static int lwm2m_load_psk_credentials(struct lwm2m_ctx *ctx)
{
	int ret;

	delete_tls_credentials(ctx->tls_tag);

	ret = load_tls_type(ctx, 3, TLS_CREDENTIAL_PSK_ID);
	if (ret < 0) {
		return ret;
	}

	ret = load_tls_type(ctx, 5, TLS_CREDENTIAL_PSK);
	return ret;
}

static int lwm2m_load_x509_credentials(struct lwm2m_ctx *ctx)
{
	int ret;

	delete_tls_credentials(ctx->tls_tag);

	ret = load_tls_type(ctx, 3, TLS_CREDENTIAL_SERVER_CERTIFICATE);
	if (ret < 0) {
		return ret;
	}
	ret = load_tls_type(ctx, 5, TLS_CREDENTIAL_PRIVATE_KEY);
	if (ret < 0) {
		return ret;
	}

	ret = load_tls_type(ctx, 4, TLS_CREDENTIAL_CA_CERTIFICATE);
	if (ret < 0) {
		return ret;
	}
	return ret;
}
#else

int lwm2m_load_psk_credentials(struct lwm2m_ctx *ctx)
{
	return -EOPNOTSUPP;
}

int lwm2m_load_x509_credentials(struct lwm2m_ctx *ctx)
{
	return -EOPNOTSUPP;
}
#endif /* CONFIG_TLS_CREDENTIALS*/

static int lwm2m_load_tls_credentials(struct lwm2m_ctx *ctx)
{
	switch (lwm2m_security_mode(ctx)) {
	case LWM2M_SECURITY_NOSEC:
		if (ctx->use_dtls) {
			return -EINVAL;
		}
		return 0;
	case LWM2M_SECURITY_PSK:
		return lwm2m_load_psk_credentials(ctx);
	case LWM2M_SECURITY_CERT:
		return lwm2m_load_x509_credentials(ctx);
	default:
		return -EOPNOTSUPP;
	}
}

static const int cipher_list_psk[] = {
	MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8,
};

static const int cipher_list_cert[] = {
	MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
};

#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

int lwm2m_set_default_sockopt(struct lwm2m_ctx *ctx)
{
#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	if (ctx->use_dtls) {
		int ret;
		uint8_t tmp;
		sec_tag_t tls_tag_list[] = {
			ctx->tls_tag,
		};

		ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_tag_list,
				       sizeof(tls_tag_list));
		if (ret < 0) {
			ret = -errno;
			LOG_ERR("Failed to set TLS_SEC_TAG_LIST option: %d", ret);
			return ret;
		}

		if (IS_ENABLED(CONFIG_LWM2M_TLS_SESSION_CACHING)) {
			int session_cache = TLS_SESSION_CACHE_ENABLED;

			ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_SESSION_CACHE,
					       &session_cache, sizeof(session_cache));
			if (ret < 0) {
				ret = -errno;
				LOG_ERR("Failed to set TLS_SESSION_CACHE option: %d", errno);
				return ret;
			}
		}
		if (IS_ENABLED(CONFIG_LWM2M_DTLS_CID)) {
			/* Enable CID */
			int cid = TLS_DTLS_CID_SUPPORTED;

			ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_DTLS_CID, &cid,
					       sizeof(cid));
			if (ret) {
				ret = -errno;
				LOG_ERR("Failed to enable TLS_DTLS_CID: %d", ret);
				/* Not fatal, continue. */
			}
		}

		if (ctx->hostname_verify && (ctx->desthostname != NULL)) {
			/** store character at len position */
			tmp = ctx->desthostname[ctx->desthostnamelen];

			/** change it to '\0' to pass to socket*/
			ctx->desthostname[ctx->desthostnamelen] = '\0';

			/** mbedtls ignores length */
			ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_HOSTNAME,
					       ctx->desthostname, ctx->desthostnamelen);

			/** restore character */
			ctx->desthostname[ctx->desthostnamelen] = tmp;
			if (ret < 0) {
				ret = -errno;
				LOG_ERR("Failed to set TLS_HOSTNAME option: %d", ret);
				return ret;
			}

			int verify = TLS_PEER_VERIFY_REQUIRED;

			ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_PEER_VERIFY, &verify,
					       sizeof(verify));
			if (ret) {
				LOG_ERR("Failed to set TLS_PEER_VERIFY");
			}

		} else {
			/* By default, Mbed TLS tries to verify peer hostname, disable it */
			int verify = TLS_PEER_VERIFY_NONE;

			ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_PEER_VERIFY, &verify,
					       sizeof(verify));
			if (ret) {
				LOG_ERR("Failed to set TLS_PEER_VERIFY");
			}
		}

		switch (lwm2m_security_mode(ctx)) {
		case LWM2M_SECURITY_PSK:
			ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_CIPHERSUITE_LIST,
					       cipher_list_psk, sizeof(cipher_list_psk));
			if (ret) {
				LOG_ERR("Failed to set TLS_CIPHERSUITE_LIST");
			}
			break;
		case LWM2M_SECURITY_CERT:
			ret = zsock_setsockopt(ctx->sock_fd, SOL_TLS, TLS_CIPHERSUITE_LIST,
					       cipher_list_cert, sizeof(cipher_list_cert));
			if (ret) {
				LOG_ERR("Failed to set TLS_CIPHERSUITE_LIST (rc %d, errno %d)", ret,
					errno);
			}
			break;
		default:
			return -EOPNOTSUPP;
		}
	}
#else
	if (!IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) && ctx->use_dtls) {
		return -EOPNOTSUPP;
	}
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */
	return 0;
}

int lwm2m_socket_start(struct lwm2m_ctx *client_ctx)
{
	socklen_t addr_len;
	int flags;
	int ret;

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	if (client_ctx->use_dtls) {
		if (client_ctx->load_credentials) {
			ret = client_ctx->load_credentials(client_ctx);
		} else {
			ret = lwm2m_load_tls_credentials(client_ctx);
		}
		if (ret < 0) {
			return ret;
		}
	}
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

	if (client_ctx->sock_fd < 0) {
		ret = lwm2m_open_socket(client_ctx);
		if (ret) {
			return ret;
		}
	}

	if (client_ctx->set_socketoptions) {
		ret = client_ctx->set_socketoptions(client_ctx);
	} else {
		ret = lwm2m_set_default_sockopt(client_ctx);
	}
	if (ret) {
		goto error;
	}

	if ((client_ctx->remote_addr).sa_family == AF_INET) {
		addr_len = sizeof(struct sockaddr_in);
	} else if ((client_ctx->remote_addr).sa_family == AF_INET6) {
		addr_len = sizeof(struct sockaddr_in6);
	} else {
		ret = -EPROTONOSUPPORT;
		goto error;
	}

	if (zsock_connect(client_ctx->sock_fd, &client_ctx->remote_addr, addr_len) < 0) {
		ret = -errno;
		LOG_ERR("Cannot connect UDP (%d)", ret);
		goto error;
	}

	flags = zsock_fcntl(client_ctx->sock_fd, F_GETFL, 0);
	if (flags == -1) {
		ret = -errno;
		LOG_ERR("zsock_fcntl(F_GETFL) failed (%d)", ret);
		goto error;
	}
	ret = zsock_fcntl(client_ctx->sock_fd, F_SETFL, flags | O_NONBLOCK);
	if (ret == -1) {
		ret = -errno;
		LOG_ERR("zsock_fcntl(F_SETFL) failed (%d)", ret);
		goto error;
	}

	LOG_INF("Connected, sock id %d", client_ctx->sock_fd);
	return 0;
error:
	lwm2m_socket_close(client_ctx);
	return ret;
}

int lwm2m_socket_close(struct lwm2m_ctx *client_ctx)
{
	int sock_fd = client_ctx->sock_fd;

	lwm2m_socket_del(client_ctx);
	client_ctx->sock_fd = -1;
	if (sock_fd >= 0) {
		return zsock_close(sock_fd);
	}

	return 0;
}

int lwm2m_engine_stop(struct lwm2m_ctx *client_ctx)
{
	lwm2m_engine_context_close(client_ctx);

	return lwm2m_socket_close(client_ctx);
}

int lwm2m_engine_start(struct lwm2m_ctx *client_ctx)
{
	char *url;
	uint16_t url_len;
	uint8_t url_data_flags;
	int ret = 0U;

	/* get the server URL */
	ret = lwm2m_get_res_buf(&LWM2M_OBJ(0, client_ctx->sec_obj_inst, 0), (void **)&url, NULL,
				&url_len, &url_data_flags);
	if (ret < 0) {
		return ret;
	}

	url[url_len] = '\0';
	ret = lwm2m_parse_peerinfo(url, client_ctx, false);
	if (ret < 0) {
		return ret;
	}

	return lwm2m_socket_start(client_ctx);
}

int lwm2m_engine_pause(void)
{
	if (suspend_engine_thread || !active_engine_thread) {
		LOG_WRN("Engine thread already suspended");
		return 0;
	}

	suspend_engine_thread = true;
	lwm2m_engine_wake_up();

	/* Check if pause requested within a engine thread, a callback for example. */
	if (engine_thread_id == k_current_get()) {
		LOG_DBG("Pause requested");
		return 0;
	}

	while (active_engine_thread) {
		k_msleep(10);
	}
	LOG_INF("LWM2M engine thread paused");
	return 0;
}

int lwm2m_engine_resume(void)
{
	if (suspend_engine_thread || active_engine_thread) {
		LOG_WRN("LWM2M engine thread state not ok for resume");
		return -EPERM;
	}

	k_thread_resume(engine_thread_id);
	lwm2m_engine_wake_up();

	return 0;
}

static int lwm2m_engine_init(void)
{
	for (int i = 0; i < LWM2M_ENGINE_MAX_OBSERVER_PATH; i++) {
		sys_slist_append(lwm2m_obs_obj_path_list(), &observe_paths[i].node);
	}

	/* Reset all socket handles to -1 so unused ones are ignored by zsock_poll() */
	for (int i = 0; i < MAX_POLL_FD; ++i) {
		sock_fds[i].fd = -1;
	}

	if (IS_ENABLED(CONFIG_LWM2M_TICKLESS)) {
		/* Create socketpair that is used to wake zsock_poll() in the main loop */
		int s[2];
		int ret = zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, s);

		if (ret) {
			LOG_ERR("Error; socketpair() returned %d", ret);
			return ret;
		}
		/* Last poll-handle is reserved for control socket */
		sock_fds[MAX_POLL_FD - 1].fd = s[0];
		control_sock = s[1];
		ret = zsock_fcntl(s[0], F_SETFL, O_NONBLOCK);
		if (ret) {
			LOG_ERR("zsock_fcntl() %d", ret);
			zsock_close(s[0]);
			zsock_close(s[1]);
			return ret;
		}
		ret = zsock_fcntl(s[1], F_SETFL, O_NONBLOCK);
		if (ret) {
			LOG_ERR("zsock_fcntl() %d", ret);
			zsock_close(s[0]);
			zsock_close(s[1]);
			return ret;
		}
	}

	lwm2m_clear_block_contexts();
#if defined(CONFIG_LWM2M_COAP_BLOCK_TRANSFER)
	(void)memset(output_block_contexts, 0, sizeof(output_block_contexts));
#endif

	STRUCT_SECTION_FOREACH(lwm2m_init_func, init) {
		int ret = init->f();

		if (ret) {
			LOG_ERR("Init function %p returned %d", init, ret);
		}
	}

	/* start sock receive thread */
	engine_thread_id = k_thread_create(&engine_thread_data, &engine_thread_stack[0],
			K_KERNEL_STACK_SIZEOF(engine_thread_stack), socket_loop,
			NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&engine_thread_data, "lwm2m-sock-recv");
	LOG_DBG("LWM2M engine socket receive thread started");
	active_engine_thread = true;

	return 0;
}

SYS_INIT(lwm2m_engine_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
