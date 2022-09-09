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

#if IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#endif

#define ENGINE_UPDATE_INTERVAL_MS 500

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
	uint32_t min_call_period; /* ms */
	uint64_t last_timestamp;  /* ms */
};

static struct service_node service_node_data[MAX_PERIODIC_SERVICE];
static sys_slist_t engine_service_list;

static K_KERNEL_STACK_DEFINE(engine_thread_stack, CONFIG_LWM2M_ENGINE_STACK_SIZE);
static struct k_thread engine_thread_data;

#define MAX_POLL_FD CONFIG_NET_SOCKETS_POLL_MAX

/* Resources */
static struct pollfd sock_fds[MAX_POLL_FD];

static struct lwm2m_ctx *sock_ctx[MAX_POLL_FD];
static int sock_nfds;

static struct lwm2m_block_context block1_contexts[NUM_BLOCK1_CONTEXT];
/* Resource wrappers */
struct lwm2m_ctx **lwm2m_sock_ctx(void) { return sock_ctx; }

int lwm2m_sock_nfds(void) { return sock_nfds; }

struct lwm2m_block_context *lwm2m_block1_context(void) { return block1_contexts; }

static int lwm2m_socket_update(struct lwm2m_ctx *ctx);

/* for debugging: to print IP addresses */
char *lwm2m_sprint_ip_addr(const struct sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

	if (addr->sa_family == AF_INET6) {
		return net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf, sizeof(buf));
	}

	if (addr->sa_family == AF_INET) {
		return net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr, buf, sizeof(buf));
	}

	LOG_ERR("Unknown IP address family:%d", addr->sa_family);
	strcpy(buf, "unk");
	return buf;
}

static uint8_t to_hex_digit(uint8_t digit)
{
	if (digit >= 10U) {
		return digit - 10U + 'a';
	}

	return digit + '0';
}

char *sprint_token(const uint8_t *token, uint8_t tkl)
{
	static char buf[32];
	char *ptr = buf;

	if (token && tkl != 0) {
		int i;

		tkl = MIN(tkl, sizeof(buf) / 2 - 1);

		for (i = 0; i < tkl; i++) {
			*ptr++ = to_hex_digit(token[i] >> 4);
			*ptr++ = to_hex_digit(token[i] & 0x0F);
		}

		*ptr = '\0';
	} else {
		strcpy(buf, "[no-token]");
	}

	return buf;
}

/* utility functions */

int lwm2m_open_socket(struct lwm2m_ctx *client_ctx)
{
	if (client_ctx->sock_fd < 0) {
		/* open socket */

		if (IS_ENABLED(CONFIG_LWM2M_DTLS_SUPPORT) && client_ctx->use_dtls) {
			client_ctx->sock_fd = socket(client_ctx->remote_addr.sa_family, SOCK_DGRAM,
						     IPPROTO_DTLS_1_2);
		} else {
			client_ctx->sock_fd =
				socket(client_ctx->remote_addr.sa_family, SOCK_DGRAM, IPPROTO_UDP);
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
	int ret = 0;

	if (client_ctx->sock_fd >= 0) {
		ret = close(client_ctx->sock_fd);
		if (ret) {
			LOG_ERR("Failed to close socket: %d", errno);
			ret = -errno;
			return ret;
		}

		client_ctx->sock_fd = -1;
		client_ctx->connection_suspended = true;
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
		/* Enable Queue mode buffer store */
		client_ctx->buffer_client_messages = true;
#endif
		lwm2m_socket_update(client_ctx);
	}

	return ret;
}


int lwm2m_socket_suspend(struct lwm2m_ctx *client_ctx)
{
	int ret = 0;

	if (client_ctx->sock_fd >= 0 && !client_ctx->connection_suspended) {
		int socket_temp_id = client_ctx->sock_fd;

		client_ctx->sock_fd = -1;
		client_ctx->connection_suspended = true;
#if defined(CONFIG_LWM2M_QUEUE_MODE_ENABLED)
		/* Enable Queue mode buffer store */
		client_ctx->buffer_client_messages = true;
#endif
		lwm2m_socket_update(client_ctx);
		client_ctx->sock_fd = socket_temp_id;
	}

	return ret;
}

int lwm2m_engine_connection_resume(struct lwm2m_ctx *client_ctx)
{
	int ret;

	if (client_ctx->connection_suspended) {
		lwm2m_close_socket(client_ctx);
		client_ctx->connection_suspended = false;
		ret = lwm2m_open_socket(client_ctx);
		if (ret) {
			return ret;
		}

		if (!client_ctx->use_dtls) {
			return 0;
		}

		LOG_DBG("Resume suspended connection");
		return lwm2m_socket_start(client_ctx);
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
	char pathstr[MAX_RESOURCE_LEN];
	bool bootstrap_server;
	int ret;

	if (obj_id == LWM2M_OBJECT_SECURITY_ID) {
		snprintk(pathstr, sizeof(pathstr), "%d/%d/1", LWM2M_OBJECT_SECURITY_ID,
			 obj_inst_id);
		ret = lwm2m_engine_get_bool(pathstr, &bootstrap_server);
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
/* returns ms until the next retransmission is due, or INT32_MAX
 * if no retransmissions are necessary
 */
static int32_t retransmit_request(struct lwm2m_ctx *client_ctx, const uint32_t timestamp)
{
	struct lwm2m_message *msg;
	struct coap_pending *p;
	int32_t remaining, next_retransmission = INT32_MAX;
	int i;

	for (i = 0, p = client_ctx->pendings; i < ARRAY_SIZE(client_ctx->pendings); i++, p++) {
		if (!p->timeout) {
			continue;
		}

		remaining = p->t0 + p->timeout - timestamp;
		if (remaining < 0) {
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
		if (remaining < next_retransmission) {
			next_retransmission = remaining;
		}
	}

	return next_retransmission;
}
static int32_t engine_next_service_timeout_ms(uint32_t max_timeout, const int64_t timestamp)
{
	struct service_node *srv;
	uint64_t time_left_ms;
	uint32_t timeout = max_timeout;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_service_list, srv, node) {
		time_left_ms = srv->last_timestamp + srv->min_call_period;

		/* service is due */
		if (time_left_ms < timestamp) {
			return 0;
		}

		/* service timeout is less than the current timeout */
		time_left_ms -= timestamp;
		if (time_left_ms < timeout) {
			timeout = time_left_ms;
		}
	}

	return timeout;
}

int lwm2m_engine_add_service(k_work_handler_t service, uint32_t period_ms)
{
	int i;

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
	service_node_data[i].min_call_period = period_ms;
	service_node_data[i].last_timestamp = 0U;

	sys_slist_append(&engine_service_list, &service_node_data[i].node);

	return 0;
}

int lwm2m_engine_update_service_period(k_work_handler_t service, uint32_t period_ms)
{
	int i = 0;

	for (i = 0; i < MAX_PERIODIC_SERVICE; i++) {
		if (service_node_data[i].service_work == service) {
			service_node_data[i].min_call_period = period_ms;
			return 0;
		}
	}

	return -ENOENT;
}

static int32_t lwm2m_engine_service(const int64_t timestamp)
{
	struct service_node *srv;
	int64_t service_due_timestamp;

	SYS_SLIST_FOR_EACH_CONTAINER(&engine_service_list, srv, node) {
		service_due_timestamp = srv->last_timestamp + srv->min_call_period;
		/* service is due */
		if (timestamp >= service_due_timestamp) {
			srv->last_timestamp = k_uptime_get();
			srv->service_work(NULL);
		}
	}

	/* calculate how long to sleep till the next service */
	return engine_next_service_timeout_ms(ENGINE_UPDATE_INTERVAL_MS, timestamp);
}

/* LwM2M Socket Integration */

int lwm2m_socket_add(struct lwm2m_ctx *ctx)
{
	if (sock_nfds >= MAX_POLL_FD) {
		return -ENOMEM;
	}

	sock_ctx[sock_nfds] = ctx;
	sock_fds[sock_nfds].fd = ctx->sock_fd;
	sock_fds[sock_nfds].events = POLLIN;
	sock_nfds++;

	return 0;
}

static int lwm2m_socket_update(struct lwm2m_ctx *ctx)
{
	for (int i = 0; i < sock_nfds; i++) {
		if (sock_ctx[i] != ctx) {
			continue;
		}
		sock_fds[i].fd = ctx->sock_fd;
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
}

static void check_notifications(struct lwm2m_ctx *ctx, const int64_t timestamp)
{
	struct observe_node *obs;
	int rc;

	lwm2m_registry_lock();
	SYS_SLIST_FOR_EACH_CONTAINER(&ctx->observer, obs, node) {
		if (!obs->event_timestamp || timestamp < obs->event_timestamp) {
			continue;
		}
		/* Check That There is not pending process*/
		if (obs->active_tx_operation) {
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
}

static int socket_recv_message(struct lwm2m_ctx *client_ctx)
{
	static uint8_t in_buf[NET_IPV6_MTU];
	socklen_t from_addr_len;
	ssize_t len;
	static struct sockaddr from_addr;

	from_addr_len = sizeof(from_addr);
	len = recvfrom(client_ctx->sock_fd, in_buf, sizeof(in_buf) - 1, 0, &from_addr,
		       &from_addr_len);

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
	lwm2m_udp_receive(client_ctx, in_buf, len, &from_addr, handle_request);

	return 0;
}

static int socket_send_message(struct lwm2m_ctx *client_ctx)
{
	sys_snode_t *msg_node = sys_slist_get(&client_ctx->pending_sends);
	struct lwm2m_message *msg;

	if (!msg_node) {
		return 0;
	}
	msg = SYS_SLIST_CONTAINER(msg_node, msg, node);
	return lwm2m_send_message(msg);
}

static void socket_reset_pollfd_events(void)
{
	for (int i = 0; i < sock_nfds; ++i) {
		sock_fds[i].events =
			POLLIN | (sys_slist_is_empty(&sock_ctx[i]->pending_sends) ? 0 : POLLOUT);
		sock_fds[i].revents = 0;
	}
}

/* LwM2M main work loop */
static void socket_loop(void)
{
	int i, rc;
	int64_t timestamp;
	int32_t timeout, next_retransmit;

	while (1) {
		/* Check is Thread Suspend Requested */
		if (suspend_engine_thread) {
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
			lwm2m_rd_client_pause();
#endif
			suspend_engine_thread = false;
			active_engine_thread = false;
			k_thread_suspend(engine_thread_id);
			active_engine_thread = true;
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT)
			lwm2m_rd_client_resume();
#endif
		}

		timestamp = k_uptime_get();
		timeout = lwm2m_engine_service(timestamp);

		/* wait for sockets */
		if (sock_nfds < 1) {
			k_msleep(timeout);
			continue;
		}

		for (i = 0; i < sock_nfds; ++i) {
			if (sys_slist_is_empty(&sock_ctx[i]->pending_sends)) {
				next_retransmit = retransmit_request(sock_ctx[i], timestamp);
				if (next_retransmit < timeout) {
					timeout = next_retransmit;
				}
			}
			if (sys_slist_is_empty(&sock_ctx[i]->pending_sends) &&
			    lwm2m_rd_client_is_registred(sock_ctx[i])) {
				check_notifications(sock_ctx[i], timestamp);
			}
		}

		socket_reset_pollfd_events();

		/*
		 * FIXME: Currently we timeout and restart poll in case fds
		 *        were modified.
		 */
		rc = poll(sock_fds, sock_nfds, timeout);
		if (rc < 0) {
			LOG_ERR("Error in poll:%d", errno);
			errno = 0;
			k_msleep(ENGINE_UPDATE_INTERVAL_MS);
			continue;
		}

		for (i = 0; i < sock_nfds; i++) {

			if (sock_ctx[i]->sock_fd < 0) {
				continue;
			}

			if ((sock_fds[i].revents & POLLERR) || (sock_fds[i].revents & POLLNVAL) ||
			    (sock_fds[i].revents & POLLHUP)) {
				LOG_ERR("Poll reported a socket error, %02x.", sock_fds[i].revents);
				if (sock_ctx[i] != NULL && sock_ctx[i]->fault_cb != NULL) {
					sock_ctx[i]->fault_cb(EIO);
				}
				continue;
			}

			if (sock_fds[i].revents & POLLIN) {
				while (sock_ctx[i]) {
					rc = socket_recv_message(sock_ctx[i]);
					if (rc) {
						break;
					}
				}
			}

			if (sock_fds[i].revents & POLLOUT) {
				socket_send_message(sock_ctx[i]);
			}
		}
	}
}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT) && defined(CONFIG_TLS_CREDENTIALS)
static int load_tls_credential(struct lwm2m_ctx *client_ctx, uint16_t res_id,
			       enum tls_credential_type type)
{
	int ret = 0;
	void *cred = NULL;
	uint16_t cred_len;
	uint8_t cred_flags;
	char pathstr[MAX_RESOURCE_LEN];

	/* ignore error value */
	tls_credential_delete(client_ctx->tls_tag, type);

	snprintk(pathstr, sizeof(pathstr), "0/%d/%u", client_ctx->sec_obj_inst, res_id);

	ret = lwm2m_engine_get_res_buf(pathstr, &cred, NULL, &cred_len, &cred_flags);
	if (ret < 0) {
		LOG_ERR("Unable to get resource data for '%s'", pathstr);
		return ret;
	}

	if (cred_len == 0) {
		LOG_ERR("Credential data is empty");
		return -EINVAL;
	}

	ret = tls_credential_add(client_ctx->tls_tag, type, cred, cred_len);
	if (ret < 0) {
		LOG_ERR("Error setting cred tag %d type %d: Error %d", client_ctx->tls_tag, type,
			ret);
	}

	return ret;
}
#endif /* CONFIG_LWM2M_DTLS_SUPPORT && CONFIG_TLS_CREDENTIALS*/

int lwm2m_socket_start(struct lwm2m_ctx *client_ctx)
{
	socklen_t addr_len;
	int flags;
	int ret;

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	uint8_t tmp;

	if (client_ctx->load_credentials) {
		ret = client_ctx->load_credentials(client_ctx);
		if (ret < 0) {
			return ret;
		}
	}
#if defined(CONFIG_TLS_CREDENTIALS)
	else {
		ret = load_tls_credential(client_ctx, 3, TLS_CREDENTIAL_PSK_ID);
		if (ret < 0) {
			return ret;
		}

		ret = load_tls_credential(client_ctx, 5, TLS_CREDENTIAL_PSK);
		if (ret < 0) {
			return ret;
		}
	}
#endif /* CONFIG_TLS_CREDENTIALS */
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */

	if (client_ctx->sock_fd < 0) {
		ret = lwm2m_open_socket(client_ctx);
		if (ret) {
			return ret;
		}
	}

#if defined(CONFIG_LWM2M_DTLS_SUPPORT)
	if (client_ctx->use_dtls) {
		sec_tag_t tls_tag_list[] = {
			client_ctx->tls_tag,
		};

		ret = setsockopt(client_ctx->sock_fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_tag_list,
				 sizeof(tls_tag_list));
		if (ret < 0) {
			ret = -errno;
			LOG_ERR("Failed to set TLS_SEC_TAG_LIST option: %d", ret);
			goto error;
		}

		if (IS_ENABLED(CONFIG_LWM2M_TLS_SESSION_CACHING)) {
			int session_cache = TLS_SESSION_CACHE_ENABLED;

			ret = setsockopt(client_ctx->sock_fd, SOL_TLS, TLS_SESSION_CACHE,
					 &session_cache, sizeof(session_cache));
			if (ret < 0) {
				ret = -errno;
				LOG_ERR("Failed to set TLS_SESSION_CACHE option: %d", errno);
				goto error;
			}
		}

		if (client_ctx->hostname_verify && (client_ctx->desthostname != NULL)) {
			/** store character at len position */
			tmp = client_ctx->desthostname[client_ctx->desthostnamelen];

			/** change it to '\0' to pass to socket*/
			client_ctx->desthostname[client_ctx->desthostnamelen] = '\0';

			/** mbedtls ignores length */
			ret = setsockopt(client_ctx->sock_fd, SOL_TLS, TLS_HOSTNAME,
					 client_ctx->desthostname, client_ctx->desthostnamelen);

			/** restore character */
			client_ctx->desthostname[client_ctx->desthostnamelen] = tmp;
			if (ret < 0) {
				ret = -errno;
				LOG_ERR("Failed to set TLS_HOSTNAME option: %d", ret);
				goto error;
			}
		}
	}
#endif /* CONFIG_LWM2M_DTLS_SUPPORT */
	if ((client_ctx->remote_addr).sa_family == AF_INET) {
		addr_len = sizeof(struct sockaddr_in);
	} else if ((client_ctx->remote_addr).sa_family == AF_INET6) {
		addr_len = sizeof(struct sockaddr_in6);
	} else {
		lwm2m_engine_stop(client_ctx);
		return -EPROTONOSUPPORT;
	}

	if (connect(client_ctx->sock_fd, &client_ctx->remote_addr, addr_len) < 0) {
		ret = -errno;
		LOG_ERR("Cannot connect UDP (%d)", ret);
		goto error;
	}

	flags = fcntl(client_ctx->sock_fd, F_GETFL, 0);
	if (flags == -1) {
		ret = -errno;
		LOG_ERR("fcntl(F_GETFL) failed (%d)", ret);
		goto error;
	}
	ret = fcntl(client_ctx->sock_fd, F_SETFL, flags | O_NONBLOCK);
	if (ret == -1) {
		ret = -errno;
		LOG_ERR("fcntl(F_SETFL) failed (%d)", ret);
		goto error;
	}

	LOG_INF("Connected, sock id %d", client_ctx->sock_fd);
	return 0;
error:
	lwm2m_engine_stop(client_ctx);
	return ret;
}

int lwm2m_socket_close(struct lwm2m_ctx *client_ctx)
{
	int sock_fd = client_ctx->sock_fd;

	lwm2m_socket_del(client_ctx);
	client_ctx->sock_fd = -1;
	if (sock_fd >= 0) {
		return close(sock_fd);
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
	char pathstr[MAX_RESOURCE_LEN];
	char *url;
	uint16_t url_len;
	uint8_t url_data_flags;
	int ret = 0U;

	/* get the server URL */
	snprintk(pathstr, sizeof(pathstr), "0/%d/0", client_ctx->sec_obj_inst);
	ret = lwm2m_engine_get_res_buf(pathstr, (void **)&url, NULL, &url_len, &url_data_flags);
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
	while (!active_engine_thread) {
		k_msleep(10);
	}
	LOG_INF("LWM2M engine thread resume");
	return 0;
}

static int lwm2m_engine_init(const struct device *dev)
{
	int i;

	for (i = 0; i < LWM2M_ENGINE_MAX_OBSERVER_PATH; i++) {
		sys_slist_append(lwm2m_obs_obj_path_list(), &observe_paths[i].node);
	}

	(void)memset(block1_contexts, 0, sizeof(block1_contexts));

	if (IS_ENABLED(CONFIG_LWM2M_RESOURCE_DATA_CACHE_SUPPORT)) {
		/* Init data cache */
		lwm2m_engine_data_cache_init();
	}

	/* start sock receive thread */
	engine_thread_id = k_thread_create(&engine_thread_data, &engine_thread_stack[0],
			K_KERNEL_STACK_SIZEOF(engine_thread_stack), (k_thread_entry_t)socket_loop,
			NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&engine_thread_data, "lwm2m-sock-recv");
	LOG_DBG("LWM2M engine socket receive thread started");
	active_engine_thread = true;

	return 0;
}

SYS_INIT(lwm2m_engine_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
