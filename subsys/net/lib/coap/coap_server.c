/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap, CONFIG_COAP_LOG_LEVEL);

#include <zephyr/net/socket.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_link_format.h>
#include <zephyr/net/coap_mgmt.h>
#include <zephyr/net/coap_service.h>
#ifdef CONFIG_ARCH_POSIX
#include <fcntl.h>
#else
#include <zephyr/posix/fcntl.h>
#endif

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#endif

#define ADDRLEN(sock) \
	(((struct sockaddr *)sock)->sa_family == AF_INET ? \
		sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))

/* Shortened defines */
#define MAX_OPTIONS    CONFIG_COAP_SERVER_MESSAGE_OPTIONS
#define MAX_PENDINGS   CONFIG_COAP_SERVICE_PENDING_MESSAGES
#define MAX_OBSERVERS  CONFIG_COAP_SERVICE_OBSERVERS
#define MAX_POLL_FD    CONFIG_NET_SOCKETS_POLL_MAX

BUILD_ASSERT(CONFIG_NET_SOCKETS_POLL_MAX > 0, "CONFIG_NET_SOCKETS_POLL_MAX can't be 0");

static K_MUTEX_DEFINE(lock);
static int control_socks[2];

#if defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_STATIC)
K_MEM_SLAB_DEFINE_STATIC(pending_data, CONFIG_COAP_SERVER_MESSAGE_SIZE,
			 CONFIG_COAP_SERVER_PENDING_ALLOCATOR_STATIC_BLOCKS, 4);
#endif

static inline void *coap_server_alloc(size_t len)
{
#if defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_STATIC)
	void *ptr;
	int ret;

	if (len > CONFIG_COAP_SERVER_MESSAGE_SIZE) {
		return NULL;
	}

	ret = k_mem_slab_alloc(&pending_data, &ptr, K_NO_WAIT);
	if (ret < 0) {
		return NULL;
	}

	return ptr;
#elif defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_SYSTEM_HEAP)
	return k_malloc(len);
#else
	ARG_UNUSED(len);

	return NULL;
#endif
}

static inline void coap_server_free(void *ptr)
{
#if defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_STATIC)
	k_mem_slab_free(&pending_data, ptr);
#elif defined(CONFIG_COAP_SERVER_PENDING_ALLOCATOR_SYSTEM_HEAP)
	k_free(ptr);
#else
	ARG_UNUSED(ptr);
#endif
}

static int coap_service_remove_observer(const struct coap_service *service,
					struct coap_resource *resource,
					const struct sockaddr *addr,
					const uint8_t *token, uint8_t tkl)
{
	struct coap_observer *obs;

	if (tkl > 0 && addr != NULL) {
		/* Prefer addr+token to find the observer */
		obs = coap_find_observer(service->data->observers, MAX_OBSERVERS, addr, token, tkl);
	} else if (tkl > 0) {
		/* Then try to to find the observer by token */
		obs = coap_find_observer_by_token(service->data->observers, MAX_OBSERVERS, token,
						  tkl);
	} else if (addr != NULL) {
		obs = coap_find_observer_by_addr(service->data->observers, MAX_OBSERVERS, addr);
	} else {
		/* Either a token or an address is required */
		return -EINVAL;
	}

	if (obs == NULL) {
		return 0;
	}

	if (resource == NULL) {
		COAP_SERVICE_FOREACH_RESOURCE(service, it) {
			if (coap_remove_observer(it, obs)) {
				memset(obs, 0, sizeof(*obs));
				return 1;
			}
		}
	} else if (coap_remove_observer(resource, obs)) {
		memset(obs, 0, sizeof(*obs));
		return 1;
	}

	return 0;
}

static int coap_server_process(int sock_fd)
{
	static uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];

	struct sockaddr client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	struct coap_service *service = NULL;
	struct coap_packet request;
	struct coap_pending *pending;
	struct coap_option options[MAX_OPTIONS] = { 0 };
	uint8_t opt_num = MAX_OPTIONS;
	uint8_t type;
	ssize_t received;
	int ret;

	received = zsock_recvfrom(sock_fd, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT, &client_addr,
				  &client_addr_len);
	__ASSERT_NO_MSG(received <= sizeof(buf));

	if (received < 0) {
		if (errno == EWOULDBLOCK) {
			return 0;
		}

		LOG_ERR("Failed to process client request (%d)", -errno);
		return -errno;
	}

	ret = coap_packet_parse(&request, buf, received, options, opt_num);
	if (ret < 0) {
		LOG_ERR("Failed To parse coap message (%d)", ret);
		return ret;
	}

	(void)k_mutex_lock(&lock, K_FOREVER);
	/* Find the active service */
	COAP_SERVICE_FOREACH(svc) {
		if (svc->data->sock_fd == sock_fd) {
			service = svc;
			break;
		}
	}
	if (service == NULL) {
		ret = -ENOENT;
		goto unlock;
	}

	type = coap_header_get_type(&request);

	pending = coap_pending_received(&request, service->data->pending, MAX_PENDINGS);
	if (pending) {
		uint8_t token[COAP_TOKEN_MAX_LEN];
		uint8_t tkl;

		switch (type) {
		case COAP_TYPE_RESET:
			tkl = coap_header_get_token(&request, token);
			coap_service_remove_observer(service, NULL, &client_addr, token, tkl);
			__fallthrough;
		case COAP_TYPE_ACK:
			coap_server_free(pending->data);
			coap_pending_clear(pending);
			break;
		default:
			LOG_WRN("Unexpected pending type %d", type);
			ret = -EINVAL;
			goto unlock;
		}

		goto unlock;
	} else if (type == COAP_TYPE_ACK || type == COAP_TYPE_RESET) {
		LOG_WRN("Unexpected type %d without pending packet", type);
		ret = -EINVAL;
		goto unlock;
	}

	if (IS_ENABLED(CONFIG_COAP_SERVER_WELL_KNOWN_CORE) &&
	    coap_header_get_code(&request) == COAP_METHOD_GET &&
	    coap_uri_path_match(COAP_WELL_KNOWN_CORE_PATH, options, opt_num)) {
		uint8_t well_known_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
		struct coap_packet response;

		ret = coap_well_known_core_get_len(service->res_begin,
						   COAP_SERVICE_RESOURCE_COUNT(service),
						   &request, &response,
						   well_known_buf, sizeof(well_known_buf));
		if (ret < 0) {
			LOG_ERR("Failed to build well known core for %s (%d)", service->name, ret);
			goto unlock;
		}

		ret = coap_service_send(service, &response, &client_addr, client_addr_len, NULL);
	} else {
		ret = coap_handle_request_len(&request, service->res_begin,
					      COAP_SERVICE_RESOURCE_COUNT(service),
					      options, opt_num, &client_addr, client_addr_len);

		/* Translate errors to response codes */
		switch (ret) {
		case -ENOENT:
			ret = COAP_RESPONSE_CODE_NOT_FOUND;
			break;
		case -ENOTSUP:
			ret = COAP_RESPONSE_CODE_BAD_REQUEST;
			break;
		case -EPERM:
			ret = COAP_RESPONSE_CODE_NOT_ALLOWED;
			break;
		}

		/* Shortcut for replying a code without a body */
		if (ret > 0 && type == COAP_TYPE_CON) {
			/* Minimal sized ack buffer */
			uint8_t ack_buf[COAP_TOKEN_MAX_LEN + 4U];
			struct coap_packet ack;

			ret = coap_ack_init(&ack, &request, ack_buf, sizeof(ack_buf), (uint8_t)ret);
			if (ret < 0) {
				LOG_ERR("Failed to init ACK (%d)", ret);
				goto unlock;
			}

			ret = coap_service_send(service, &ack, &client_addr, client_addr_len, NULL);
		}
	}

unlock:
	(void)k_mutex_unlock(&lock);

	return ret;
}

static void coap_server_retransmit(void)
{
	struct coap_pending *pending;
	int64_t remaining;
	int64_t now = k_uptime_get();
	int ret;

	(void)k_mutex_lock(&lock, K_FOREVER);

	COAP_SERVICE_FOREACH(service) {
		if (service->data->sock_fd < 0) {
			continue;
		}

		pending = coap_pending_next_to_expire(service->data->pending, MAX_PENDINGS);
		if (pending == NULL) {
			/* No work to be done */
			continue;
		}

		/* Check if the pending request has expired */
		remaining = pending->t0 + pending->timeout - now;
		if (remaining > 0) {
			continue;
		}

		if (coap_pending_cycle(pending)) {
			ret = zsock_sendto(service->data->sock_fd, pending->data, pending->len, 0,
					   &pending->addr, ADDRLEN(&pending->addr));
			if (ret < 0) {
				LOG_ERR("Failed to send pending retransmission for %s (%d)",
					service->name, ret);
			}
			__ASSERT_NO_MSG(ret == pending->len);
		} else {
			LOG_WRN("Packet retransmission failed for %s", service->name);

			coap_service_remove_observer(service, NULL, &pending->addr, NULL, 0U);
			coap_server_free(pending->data);
			coap_pending_clear(pending);
		}
	}

	(void)k_mutex_unlock(&lock);
}

static int coap_server_poll_timeout(void)
{
	struct coap_pending *pending;
	int64_t result = INT64_MAX;
	int64_t remaining;
	int64_t now = k_uptime_get();

	COAP_SERVICE_FOREACH(svc) {
		if (svc->data->sock_fd < -1) {
			continue;
		}

		pending = coap_pending_next_to_expire(svc->data->pending, MAX_PENDINGS);
		if (pending == NULL) {
			continue;
		}

		remaining = pending->t0 + pending->timeout - now;
		if (result > remaining) {
			result = remaining;
		}
	}

	if (result == INT64_MAX) {
		return -1;
	}

	return MAX(result, 0);
}

static void coap_server_update_services(void)
{
	zsock_send(control_socks[1], &(char){0}, 1, 0);
}

static inline bool coap_service_in_section(const struct coap_service *service)
{
	STRUCT_SECTION_START_EXTERN(coap_service);
	STRUCT_SECTION_END_EXTERN(coap_service);

	return STRUCT_SECTION_START(coap_service) <= service &&
	       STRUCT_SECTION_END(coap_service) > service;
}

static inline void coap_service_raise_event(const struct coap_service *service, uint32_t mgmt_event)
{
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	const struct net_event_coap_service net_event = {
		.service = service,
	};

	net_mgmt_event_notify_with_info(mgmt_event, NULL, (void *)&net_event, sizeof(net_event));
#else
	ARG_UNUSED(service);

	net_mgmt_event_notify(mgmt_event, NULL);
#endif
}

int coap_service_start(const struct coap_service *service)
{
	int ret;

	uint8_t af;
	socklen_t len;
	struct sockaddr_storage addr_storage;
	union {
		struct sockaddr *addr;
		struct sockaddr_in *addr4;
		struct sockaddr_in6 *addr6;
	} addr_ptrs = {
		.addr = (struct sockaddr *)&addr_storage,
	};

	if (!coap_service_in_section(service)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (service->data->sock_fd >= 0) {
		ret = -EALREADY;
		goto end;
	}

	/* set the default address (in6addr_any / INADDR_ANY are all 0) */
	addr_storage = (struct sockaddr_storage){0};
	if (IS_ENABLED(CONFIG_NET_IPV6) && service->host != NULL &&
	    zsock_inet_pton(AF_INET6, service->host, &addr_ptrs.addr6->sin6_addr) == 1) {
		/* if a literal IPv6 address is provided as the host, use IPv6 */
		af = AF_INET6;
		len = sizeof(struct sockaddr_in6);

		addr_ptrs.addr6->sin6_family = AF_INET6;
		addr_ptrs.addr6->sin6_port = htons(*service->port);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && service->host != NULL &&
		   zsock_inet_pton(AF_INET, service->host, &addr_ptrs.addr4->sin_addr) == 1) {
		/* if a literal IPv4 address is provided as the host, use IPv4 */
		af = AF_INET;
		len = sizeof(struct sockaddr_in);

		addr_ptrs.addr4->sin_family = AF_INET;
		addr_ptrs.addr4->sin_port = htons(*service->port);
	} else if (IS_ENABLED(CONFIG_NET_IPV6)) {
		/* prefer IPv6 if both IPv6 and IPv4 are supported */
		af = AF_INET6;
		len = sizeof(struct sockaddr_in6);

		addr_ptrs.addr6->sin6_family = AF_INET6;
		addr_ptrs.addr6->sin6_port = htons(*service->port);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		af = AF_INET;
		len = sizeof(struct sockaddr_in);

		addr_ptrs.addr4->sin_family = AF_INET;
		addr_ptrs.addr4->sin_port = htons(*service->port);
	} else {
		ret = -ENOTSUP;
		goto end;
	}

	service->data->sock_fd = zsock_socket(af, SOCK_DGRAM, IPPROTO_UDP);
	if (service->data->sock_fd < 0) {
		ret = -errno;
		goto end;
	}

	ret = zsock_fcntl(service->data->sock_fd, F_SETFL, O_NONBLOCK);
	if (ret < 0) {
		ret = -errno;
		goto close;
	}

	ret = zsock_bind(service->data->sock_fd, addr_ptrs.addr, len);
	if (ret < 0) {
		ret = -errno;
		goto close;
	}

	if (*service->port == 0) {
		/* ephemeral port - read back the port number */
		len = sizeof(addr_storage);
		ret = zsock_getsockname(service->data->sock_fd, addr_ptrs.addr, &len);
		if (ret < 0) {
			goto close;
		}

		if (af == AF_INET6) {
			*service->port = addr_ptrs.addr6->sin6_port;
		} else {
			*service->port = addr_ptrs.addr4->sin_port;
		}
	}

end:
	k_mutex_unlock(&lock);

	coap_server_update_services();

	coap_service_raise_event(service, NET_EVENT_COAP_SERVICE_STARTED);

	return ret;

close:
	(void)zsock_close(service->data->sock_fd);
	service->data->sock_fd = -1;

	k_mutex_unlock(&lock);

	return ret;
}

int coap_service_stop(const struct coap_service *service)
{
	int ret;

	if (!coap_service_in_section(service)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (service->data->sock_fd < 0) {
		k_mutex_unlock(&lock);
		return -EALREADY;
	}

	/* Closing a socket will trigger a poll event */
	ret = zsock_close(service->data->sock_fd);
	service->data->sock_fd = -1;

	k_mutex_unlock(&lock);

	coap_service_raise_event(service, NET_EVENT_COAP_SERVICE_STOPPED);

	return ret;
}

int coap_service_is_running(const struct coap_service *service)
{
	int ret;

	if (!coap_service_in_section(service)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	ret = (service->data->sock_fd < 0) ? 0 : 1;

	k_mutex_unlock(&lock);

	return ret;
}

int coap_service_send(const struct coap_service *service, const struct coap_packet *cpkt,
		      const struct sockaddr *addr, socklen_t addr_len,
		      const struct coap_transmission_parameters *params)
{
	int ret;

	if (!coap_service_in_section(service)) {
		__ASSERT_NO_MSG(false);
		return -EINVAL;
	}

	(void)k_mutex_lock(&lock, K_FOREVER);

	if (service->data->sock_fd < 0) {
		(void)k_mutex_unlock(&lock);
		return -EBADF;
	}

	/*
	 * Check if we should start with retransmits, if creating a pending message fails we still
	 * try to send.
	 */
	if (coap_header_get_type(cpkt) == COAP_TYPE_CON) {
		struct coap_pending *pending = coap_pending_next_unused(service->data->pending,
									MAX_PENDINGS);

		if (pending == NULL) {
			LOG_WRN("No pending message available for %s", service->name);
			goto send;
		}

		ret = coap_pending_init(pending, cpkt, addr, params);
		if (ret < 0) {
			LOG_WRN("Failed to init pending message for %s (%d)", service->name, ret);
			goto send;
		}

		/* Replace tracked data with our allocated copy */
		pending->data = coap_server_alloc(pending->len);
		if (pending->data == NULL) {
			LOG_WRN("Failed to allocate pending message data for %s", service->name);
			coap_pending_clear(pending);
			goto send;
		}
		memcpy(pending->data, cpkt->data, pending->len);

		coap_pending_cycle(pending);

		/* Trigger event in receive loop to schedule retransmit */
		coap_server_update_services();
	}

send:
	(void)k_mutex_unlock(&lock);

	ret = zsock_sendto(service->data->sock_fd, cpkt->data, cpkt->offset, 0, addr, addr_len);
	if (ret < 0) {
		LOG_ERR("Failed to send CoAP message (%d)", ret);
		return ret;
	}
	__ASSERT_NO_MSG(ret == cpkt->offset);

	return 0;
}

int coap_resource_send(const struct coap_resource *resource, const struct coap_packet *cpkt,
		       const struct sockaddr *addr, socklen_t addr_len,
		       const struct coap_transmission_parameters *params)
{
	/* Find owning service */
	COAP_SERVICE_FOREACH(svc) {
		if (COAP_SERVICE_HAS_RESOURCE(svc, resource)) {
			return coap_service_send(svc, cpkt, addr, addr_len, params);
		}
	}

	return -ENOENT;
}

int coap_resource_parse_observe(struct coap_resource *resource, const struct coap_packet *request,
				const struct sockaddr *addr)
{
	const struct coap_service *service = NULL;
	int ret;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;

	if (!coap_packet_is_request(request)) {
		return -EINVAL;
	}

	ret = coap_get_option_int(request, COAP_OPTION_OBSERVE);
	if (ret < 0) {
		return ret;
	}

	/* Find owning service */
	COAP_SERVICE_FOREACH(svc) {
		if (COAP_SERVICE_HAS_RESOURCE(svc, resource)) {
			service = svc;
			break;
		}
	}

	if (service == NULL) {
		return -ENOENT;
	}

	tkl = coap_header_get_token(request, token);
	if (tkl == 0) {
		return -EINVAL;
	}

	(void)k_mutex_lock(&lock, K_FOREVER);

	if (ret == 0) {
		struct coap_observer *observer;

		/* RFC7641 section 4.1 - Check if the current observer already exists */
		observer = coap_find_observer(service->data->observers, MAX_OBSERVERS, addr, token,
					      tkl);
		if (observer != NULL) {
			/* Client refresh */
			goto unlock;
		}

		/* New client */
		observer = coap_observer_next_unused(service->data->observers, MAX_OBSERVERS);
		if (observer == NULL) {
			ret = -ENOMEM;
			goto unlock;
		}

		coap_observer_init(observer, request, addr);
		coap_register_observer(resource, observer);
	} else if (ret == 1) {
		ret = coap_service_remove_observer(service, resource, addr, token, tkl);
		if (ret < 0) {
			LOG_WRN("Failed to remove observer (%d)", ret);
		}
	}

unlock:
	(void)k_mutex_unlock(&lock);

	return ret;
}

static int coap_resource_remove_observer(struct coap_resource *resource,
					 const struct sockaddr *addr,
					 const uint8_t *token, uint8_t token_len)
{
	const struct coap_service *service = NULL;
	int ret;

	/* Find owning service */
	COAP_SERVICE_FOREACH(svc) {
		if (COAP_SERVICE_HAS_RESOURCE(svc, resource)) {
			service = svc;
			break;
		}
	}

	if (service == NULL) {
		return -ENOENT;
	}

	(void)k_mutex_lock(&lock, K_FOREVER);
	ret = coap_service_remove_observer(service, resource, addr, token, token_len);
	(void)k_mutex_unlock(&lock);

	if (ret == 1) {
		/* An observer was found and removed */
		return 0;
	} else if (ret == 0) {
		/* No matching observer found */
		return -ENOENT;
	}

	/* An error occurred */
	return ret;
}

int coap_resource_remove_observer_by_addr(struct coap_resource *resource,
					  const struct sockaddr *addr)
{
	return coap_resource_remove_observer(resource, addr, NULL, 0);
}

int coap_resource_remove_observer_by_token(struct coap_resource *resource,
					   const uint8_t *token, uint8_t token_len)
{
	return coap_resource_remove_observer(resource, NULL, token, token_len);
}

static void coap_server_thread(void *p1, void *p2, void *p3)
{
	struct zsock_pollfd sock_fds[MAX_POLL_FD];
	int sock_nfds;
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Create a socket pair to wake zsock_poll */
	ret = zsock_socketpair(AF_UNIX, SOCK_STREAM, 0, control_socks);
	if (ret < 0) {
		LOG_ERR("Failed to create socket pair (%d)", ret);
		return;
	}

	for (int i = 0; i < 2; ++i) {
		ret = zsock_fcntl(control_socks[i], F_SETFL, O_NONBLOCK);

		if (ret < 0) {
			zsock_close(control_socks[0]);
			zsock_close(control_socks[1]);

			LOG_ERR("Failed to set socket pair [%d] non-blocking (%d)", i, ret);
			return;
		}
	}

	COAP_SERVICE_FOREACH(svc) {
		if (svc->flags & COAP_SERVICE_AUTOSTART) {
			ret = coap_service_start(svc);
			if (ret < 0) {
				LOG_ERR("Failed to autostart service %s (%d)", svc->name, ret);
			}
		}
	}

	while (true) {
		sock_nfds = 0;
		COAP_SERVICE_FOREACH(svc) {
			if (svc->data->sock_fd < 0) {
				continue;
			}
			if (sock_nfds >= MAX_POLL_FD) {
				LOG_ERR("Maximum active CoAP services reached (%d), "
					"increase CONFIG_NET_SOCKETS_POLL_MAX to support more.",
					MAX_POLL_FD);
				break;
			}

			sock_fds[sock_nfds].fd = svc->data->sock_fd;
			sock_fds[sock_nfds].events = ZSOCK_POLLIN;
			sock_fds[sock_nfds].revents = 0;
			sock_nfds++;
		}

		/* Add socket pair FD to allow wake up */
		if (sock_nfds < MAX_POLL_FD) {
			sock_fds[sock_nfds].fd = control_socks[0];
			sock_fds[sock_nfds].events = ZSOCK_POLLIN;
			sock_fds[sock_nfds].revents = 0;
			sock_nfds++;
		}

		__ASSERT_NO_MSG(sock_nfds > 0);

		ret = zsock_poll(sock_fds, sock_nfds, coap_server_poll_timeout());
		if (ret < 0) {
			LOG_ERR("Poll error (%d)", -errno);
			k_msleep(10);
		}

		for (int i = 0; i < sock_nfds; ++i) {
			/* Check the wake up event */
			if (sock_fds[i].fd == control_socks[0] &&
			    sock_fds[i].revents & ZSOCK_POLLIN) {
				char tmp;

				zsock_recv(sock_fds[i].fd, &tmp, 1, 0);
				continue;
			}

			/* Check if socket can receive/was closed first */
			if (sock_fds[i].revents & ZSOCK_POLLIN) {
				coap_server_process(sock_fds[i].fd);
				continue;
			}

			if (sock_fds[i].revents & ZSOCK_POLLERR) {
				LOG_ERR("Poll error on %d", sock_fds[i].fd);
			}
			if (sock_fds[i].revents & ZSOCK_POLLHUP) {
				LOG_ERR("Poll hup on %d", sock_fds[i].fd);
			}
			if (sock_fds[i].revents & ZSOCK_POLLNVAL) {
				LOG_ERR("Poll invalid on %d", sock_fds[i].fd);
			}
		}

		/* Process retransmits */
		coap_server_retransmit();
	}
}

K_THREAD_DEFINE(coap_server_id, CONFIG_COAP_SERVER_STACK_SIZE,
		coap_server_thread, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, 0);
