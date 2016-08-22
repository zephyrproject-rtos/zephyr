/** @file
 * @brief Network context API
 *
 * An API for applications to define a network connection.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_CONTEXT)
#define SYS_LOG_DOMAIN "net/ctx"
#define NET_DEBUG 1
#endif

#include <nanokernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <net/nbuf.h>
#include <net/net_ip.h>
#include <net/net_context.h>

#include "connection.h"
#include "net_private.h"

#include "ipv6.h"
#include "ipv4.h"
#include "udp.h"

#define NET_MAX_CONTEXT CONFIG_NET_MAX_CONTEXTS

static struct net_context contexts[NET_MAX_CONTEXT];

/* We need to lock the contexts array as these APIs are typically called
 * from applications which are usually run in task context.
 */
static struct nano_sem contexts_lock;

int net_context_get(sa_family_t family,
		    enum net_sock_type type,
		    enum net_ip_protocol ip_proto,
		    struct net_context **context)
{
	int i, ret = -ENOENT;

#if defined(CONFIG_NET_CONTEXT_CHECK)

#if !defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		NET_ASSERT_INFO(family != AF_INET, "IPv4 disabled");
		return -EPFNOSUPPORT;
	}
#endif

#if !defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		NET_ASSERT_INFO(family != AF_INET6, "IPv6 disabled");
		return -EPFNOSUPPORT;
	}
#endif

#if !defined(CONFIG_NET_UDP)
	if (type == SOCK_DGRAM) {
		NET_ASSERT_INFO(type != SOCK_DGRAM,
				"Datagram context disabled");
		return -EPROTOTYPE;
	}

	if (ip_proto == IPPROTO_UDP) {
		NET_ASSERT_INFO(ip_proto != IPPROTO_UDP, "UDP disabled");
		return -EPROTONOSUPPORT;
	}
#endif

#if !defined(CONFIG_NET_TCP)
	if (type == SOCK_STREAM) {
		NET_ASSERT_INFO(type != SOCK_STREAM,
				"Stream context disabled");
		return -EPROTOTYPE;
	}

	if (ip_proto == IPPROTO_TCP) {
		NET_ASSERT_INFO(ip_proto != IPPROTO_TCP, "TCP disabled");
		return -EPROTONOSUPPORT;
	}
#endif

	if (family != AF_INET && family != AF_INET6) {
		NET_ASSERT_INFO(family == AF_INET || family == AF_INET6,
				"Unknown address family %d", family);
		return -EAFNOSUPPORT;
	}

	if (type != SOCK_DGRAM && type != SOCK_STREAM) {
		NET_ASSERT_INFO(type == SOCK_DGRAM || type == SOCK_STREAM,
				"Unknown context type");
		return -EPROTOTYPE;
	}

	if (ip_proto != IPPROTO_UDP && ip_proto != IPPROTO_TCP) {
		NET_ASSERT_INFO(ip_proto == IPPROTO_UDP ||
				ip_proto == IPPROTO_TCP,
				"Unknown IP protocol %d", ip_proto);
		return -EPROTONOSUPPORT;
	}

	if ((type == SOCK_STREAM && ip_proto == IPPROTO_UDP) ||
	    (type == SOCK_DGRAM && ip_proto == IPPROTO_TCP)) {
		NET_ASSERT_INFO(\
			(type != SOCK_STREAM || ip_proto != IPPROTO_UDP) &&
			(type != SOCK_DGRAM || ip_proto != IPPROTO_TCP),
			"Context type and protocol mismatch, type %d proto %d",
			type, ip_proto);
		return -EOPNOTSUPP;
	}

	if (!context) {
		NET_ASSERT_INFO(context, "Invalid context");
		return -EINVAL;
	}
#endif /* CONFIG_NET_CONTEXT_CHECK */

	nano_sem_take(&contexts_lock, TICKS_UNLIMITED);

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (net_context_is_used(&contexts[i])) {
			continue;
		}

		contexts[i].flags = 0;

		net_context_set_family(&contexts[i], family);
		net_context_set_type(&contexts[i], type);
		net_context_set_ip_proto(&contexts[i], ip_proto);

		contexts[i].flags |= NET_CONTEXT_IN_USE;
		contexts[i].iface = 0;

		memset(&contexts[i].remote, 0, sizeof(struct sockaddr));
		memset(&contexts[i].local, 0, sizeof(struct sockaddr_ptr));

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
		nano_sem_init(&contexts[i].recv_data_wait);
		nano_sem_give(&contexts[i].recv_data_wait);
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

		*context = &contexts[i];

		ret = 0;
		break;
	}

	nano_sem_give(&contexts_lock);

	return ret;
}

int net_context_put(struct net_context *context)
{
	NET_ASSERT(context);

	if (!context) {
		return -EINVAL;
	}

	nano_sem_take(&contexts_lock, TICKS_UNLIMITED);

	context->flags &= ~NET_CONTEXT_IN_USE;

	nano_sem_give(&contexts_lock);

	return 0;
}

static int check_used_port(enum net_ip_protocol ip_proto,
			   uint16_t local_port,
			   const struct sockaddr *local_addr)

{
	int i;

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (!net_context_is_used(&contexts[i])) {
			continue;
		}

		if (net_context_get_ip_proto(&contexts[i]) == ip_proto &&
		    net_sin((struct sockaddr *)&contexts[i].local)->sin_port ==
							    local_port) {
			if (local_addr->family == AF_INET6) {
				if (net_ipv6_addr_cmp(
					    ((struct sockaddr_in6_ptr *)
					      &contexts[i].local)->sin6_addr,
					    &((struct sockaddr_in6 *)
					      local_addr)->sin6_addr)) {
					return -EEXIST;
				}
			} else {
				if (net_ipv4_addr_cmp(
					    ((struct sockaddr_in_ptr *)
					     &contexts[i].local)->sin_addr,
					    &((struct sockaddr_in *)
					      local_addr)->sin_addr)) {
					return -EEXIST;
				}
			}
		}
	}

	return 0;
}

static uint16_t find_available_port(struct net_context *context,
				    const struct sockaddr *addr)
{
	if (net_sin(addr)->sin_port) {
		if (check_used_port(net_context_get_ip_proto(context),
				    net_sin(addr)->sin_port,
				    addr) < 0) {
			return 0;
		}
	} else {
		uint16_t local_port;

		do {
			local_port = sys_rand32_get() | 0x8000;
		} while (check_used_port(
				 net_context_get_ip_proto(context),
				 htons(local_port), addr) == -EEXIST);

		return htons(local_port);
	}

	return net_sin(addr)->sin_port;
}

int net_context_bind(struct net_context *context, const struct sockaddr *addr)
{
	NET_ASSERT(context && addr);
	NET_ASSERT(context >= &context[0] || \
		   context <= &context[NET_MAX_CONTEXT]);

#if defined(CONFIG_NET_IPV6)
	if (addr->family == AF_INET6) {
		struct net_if *iface;
		struct in6_addr *ptr;
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;

		if (net_is_ipv6_addr_mcast(&addr6->sin6_addr)) {
			struct net_if_mcast_addr *maddr;

			maddr = net_if_ipv6_maddr_lookup(&addr6->sin6_addr,
							 &iface);
			if (!maddr) {
				return -ENOENT;
			}

			ptr = &maddr->address.in6_addr;

		} else {
			struct net_if_addr *ifaddr;

			ifaddr = net_if_ipv6_addr_lookup(&addr6->sin6_addr,
							 &iface);
			if (!ifaddr) {
				return -ENOENT;
			}

			ptr = &ifaddr->address.in6_addr;
		}

		if (!iface) {
			NET_DBG("Cannot bind to %s",
				net_sprint_ipv6_addr(&addr6->sin6_addr));

			return -EADDRNOTAVAIL;
		}

		addr6->sin6_port = find_available_port(context,
						     (struct sockaddr *)addr6);
		if (!addr6->sin6_port) {
			return -EADDRINUSE;
		}

		net_context_set_iface(context, iface);

		((struct sockaddr_in6_ptr *)&context->local)->sin6_addr = ptr;
		((struct sockaddr_in6_ptr *)&context->local)->sin6_port =
							addr6->sin6_port;

		NET_DBG("Context %p binding to [%s]:%d iface %p", context,
			net_sprint_ipv6_addr(ptr), ntohs(addr6->sin6_port),
			iface);

		return 0;
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (addr->family == AF_INET) {
		struct net_if *iface;
		struct net_if_addr *ifaddr;
		struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;

		ifaddr = net_if_ipv4_addr_lookup(&addr4->sin_addr,
						 &iface);
		if (!ifaddr) {
			return -ENOENT;
		}

		if (!iface) {
			NET_DBG("Cannot bind to %s",
				net_sprint_ipv4_addr(&addr4->sin_addr));

			return -EADDRNOTAVAIL;
		}

		addr4->sin_port = find_available_port(context,
						     (struct sockaddr *)addr4);
		if (!addr4->sin_port) {
			return -EADDRINUSE;
		}

		net_context_set_iface(context, iface);

		((struct sockaddr_in_ptr *)&context->local)->sin_addr =
						&ifaddr->address.in_addr;
		((struct sockaddr_in_ptr *)&context->local)->sin_port =
							addr4->sin_port;

		NET_DBG("Context %p binding to %s:%d iface %p", context,
			net_sprint_ipv4_addr(&ifaddr->address.in_addr),
			ntohs(addr4->sin_port), iface);

		return 0;
	}
#endif

	return -EINVAL;
}

int net_context_listen(struct net_context *context, int backlog)
{
	ARG_UNUSED(backlog);

	NET_ASSERT(context);

#if defined(CONFIG_NET_UDP)
	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		return -EOPNOTSUPP;
	}
#endif

#if defined(CONFIG_NET_TCP)
	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

	contexts->flags |= NET_CONTEXT_LISTEN;

	/* FIXME - register a listener using net_conn_register() */
#endif

	return 0;
}

int net_context_connect(struct net_context *context,
			const struct sockaddr *addr,
			net_context_connect_cb_t cb,
			int32_t timeout,
			void *user_data)
{
	NET_ASSERT(context && addr);

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

	if (addr->family != net_context_get_family(context)) {
		NET_ASSERT_INFO(addr->family == \
				net_context_get_family(context),
				"Family mismatch %d should be %d",
				addr->family,
				net_context_get_family(context));
		return -EINVAL;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_context_get_family(context) == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)
							&context->remote;

		memcpy(&addr6->sin6_addr, &net_sin6(addr)->sin6_addr,
		       sizeof(struct in6_addr));

		addr6->sin6_port = net_sin6(addr)->sin6_port;
		addr6->sin6_family = AF_INET6;

		if (!net_is_ipv6_addr_unspecified(&addr6->sin6_addr)) {
			context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
		} else {
			context->flags &= ~NET_CONTEXT_REMOTE_ADDR_SET;
		}
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(context) == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)
							&context->remote;

		memcpy(&addr4->sin_addr, &net_sin(addr)->sin_addr,
		       sizeof(struct in_addr));

		addr4->sin_port = net_sin(addr)->sin_port;
		addr4->sin_family = AF_INET;

		if (addr4->sin_addr.s_addr[0]) {
			context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
		} else {
			context->flags &= ~NET_CONTEXT_REMOTE_ADDR_SET;
		}
	} else
#endif /* CONFIG_NET_IPV4 */

	{
		return -EINVAL; /* Not IPv4 or IPv6 */
	}

#if defined(CONFIG_NET_UDP)
	if (net_context_get_type(context) == SOCK_DGRAM) {
		if (cb) {
			cb(context, user_data);
		}

		return 0;
	}
#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)
	/* FIXME - Add here the TCP stuff */
	return -ENOTSUP;
#endif

	return 0;
}

int net_context_accept(struct net_context *context,
		       net_context_accept_cb_t cb,
		       int32_t timeout,
		       void *user_data)
{
	NET_ASSERT(context);

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

	if (!(context->flags & NET_CONTEXT_LISTEN) ||
	    (net_context_get_type(context) != SOCK_STREAM)) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_TCP)
	/* FIXME - Add here the TCP stuff */
	return -ENOTSUP;
#endif

	return 0;
}

static int send_data(struct net_context *context,
		     struct net_buf *buf,
		     net_context_send_cb_t cb,
		     int32_t timeout,
		     void *token,
		     void *user_data)
{
	int ret;

	context->send_cb = cb;
	context->user_data = user_data;
	net_nbuf_set_token(buf, token);

	ret =  net_send_data(buf);

	if (!timeout || net_context_get_ip_proto(context) == IPPROTO_UDP) {
		return ret;
	}

#if defined(CONFIG_NET_TCP)
	/* FIXME - Add TCP support here */
#endif

	return ret;
}

int net_context_send(struct net_buf *buf,
		     net_context_send_cb_t cb,
		     int32_t timeout,
		     void *token,
		     void *user_data)
{
	struct net_context *context = net_nbuf_context(buf);

	NET_ASSERT(context);

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

	if (!(context->flags & NET_CONTEXT_REMOTE_ADDR_SET) ||
	    !net_sin(&context->remote)->sin_port) {
		return -EDESTADDRREQ;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		buf = net_ipv6_create(context, buf,
			&((struct sockaddr_in6 *)&context->remote)->sin6_addr);
		buf = net_udp_append(context, buf,
				ntohs(net_sin6(&context->remote)->sin6_port));
		buf = net_ipv6_finalize(context, buf);
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET) {
		buf = net_ipv4_create(context, buf,
			&((struct sockaddr_in *)&context->remote)->sin_addr);
		buf = net_udp_append(context, buf,
				ntohs(net_sin(&context->remote)->sin_port));
		buf = net_ipv4_finalize(context, buf);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		NET_DBG("Invalid protocol family %d", net_nbuf_family(buf));
		return -EINVAL;
	}

	return send_data(context, buf, cb, timeout, token, user_data);
}

int net_context_sendto(struct net_buf *buf,
		       const struct sockaddr *dst_addr,
		       net_context_send_cb_t cb,
		       int32_t timeout,
		       void *token,
		       void *user_data)
{
	struct net_context *context = net_nbuf_context(buf);

	NET_ASSERT(context);

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

	if (!dst_addr) {
		return -EDESTADDRREQ;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)dst_addr;

		if (net_is_ipv6_addr_unspecified(&addr6->sin6_addr)) {
			return -EDESTADDRREQ;
		}

		buf = net_ipv6_create(context, buf, &addr6->sin6_addr);
		buf = net_udp_append(context, buf, ntohs(addr6->sin6_port));
		buf = net_ipv6_finalize(context, buf);

	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)dst_addr;

		if (!addr4->sin_addr.s_addr[0]) {
			return -EDESTADDRREQ;
		}

		buf = net_ipv4_create(context, buf, &addr4->sin_addr);
		buf = net_udp_append(context, buf, ntohs(addr4->sin_port));
		buf = net_ipv4_finalize(context, buf);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		NET_DBG("Invalid protocol family %d", net_nbuf_family(buf));
		return -EINVAL;
	}

	return send_data(context, buf, cb, timeout, token, user_data);
}

static inline struct net_context *find_context(void *conn_handler)
{
	int i;

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (!net_context_is_used(&contexts[i])) {
			continue;
		}

		if (contexts[i].conn_handler == conn_handler) {
			return &contexts[i];
		}
	}

	return NULL;
}

static void set_appdata_values(struct net_buf *buf,
			       enum net_ip_protocol proto,
			       size_t total_len)
{
	if (proto == IPPROTO_UDP) {
		net_nbuf_set_appdata(buf, net_nbuf_udp_data(buf) +
				     sizeof(struct net_udp_hdr));
		net_nbuf_set_appdatalen(buf, total_len -
					net_nbuf_ip_hdr_len(buf) -
					sizeof(struct net_udp_hdr));
	} else {
		net_nbuf_set_appdata(buf, net_nbuf_ip_data(buf) +
				     net_nbuf_ip_hdr_len(buf));
		net_nbuf_set_appdatalen(buf, total_len -
					net_nbuf_ip_hdr_len(buf));
	}

	NET_ASSERT_INFO(net_nbuf_appdatalen(buf) < total_len,
			"Wrong appdatalen %d, total %d",
			net_nbuf_appdatalen(buf), total_len);
}

enum net_verdict packet_received(struct net_conn *conn,
				 struct net_buf *buf,
				 void *user_data)
{
	struct net_context *context = find_context(conn);

	NET_ASSERT(context);

	net_context_set_iface(context, net_nbuf_iface(buf));

	net_nbuf_set_context(buf, context);

	NET_ASSERT(net_nbuf_iface(buf));

	if (context->recv_cb) {
		size_t total_len = net_buf_frags_len(buf);

		if (net_nbuf_family(buf) == AF_INET6) {
			set_appdata_values(buf, NET_IPV6_BUF(buf)->nexthdr,
					   total_len);
		} else {
			set_appdata_values(buf, NET_IPV4_BUF(buf)->proto,
					   total_len);
		}

		NET_DBG("Set appdata to %p len %d (total %d)",
			net_nbuf_appdata(buf), net_nbuf_appdatalen(buf),
			total_len);

		context->recv_cb(context, buf, 0, user_data);

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
		nano_sem_give(&context->recv_data_wait);
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

		return NET_OK;
	}

	/* If there is no callback registered, then we can only drop
	 * the packet.
	 */
	return NET_DROP;
}

int net_context_recv(struct net_context *context,
		     net_context_recv_cb_t cb,
		     int32_t timeout,
		     void *user_data)
{
	struct sockaddr local_addr = {
		.family = net_context_get_family(context),
	};
	struct sockaddr *laddr = NULL;
	uint16_t lport = 0;
	int ret;

	NET_ASSERT(context);

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

	if (context->conn_handler) {
		net_conn_unregister(context->conn_handler);
		context->conn_handler = NULL;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_context_get_family(context) == AF_INET6) {
		if (((struct sockaddr_in6_ptr *)&context->local)->sin6_addr) {
			net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr,
					((struct sockaddr_in6_ptr *)
					  &context->local)->sin6_addr);

			laddr = &local_addr;
		}

		net_sin6(&local_addr)->sin6_port =
			net_sin6((struct sockaddr *)&context->local)->sin6_port;
		lport = net_sin6((struct sockaddr *)&context->local)->sin6_port;
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(context) == AF_INET) {
		if (((struct sockaddr_in_ptr *)&context->local)->sin_addr) {
			net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
					((struct sockaddr_in_ptr *)
						&context->local)->sin_addr);

			laddr = &local_addr;
		}

		lport = net_sin((struct sockaddr *)&context->local)->sin_port;
	}
#endif /* CONFIG_NET_IPV4 */

	context->recv_cb = cb;

	ret = net_conn_register(net_context_get_ip_proto(context),
				context->flags & NET_CONTEXT_REMOTE_ADDR_SET ?
							&context->remote : NULL,
				laddr,
				ntohs(net_sin(&context->remote)->sin_port),
				ntohs(lport),
				packet_received,
				user_data,
				&context->conn_handler);
	if (ret < 0) {
		context->recv_cb = NULL;
		return ret;
	}

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
	if (timeout) {
		/* Make sure we have the lock, then the packet_received()
		 * callback will release the semaphore when data has been
		 * received.
		 */
		while (nano_sem_take(&context->recv_data_wait, 0)) {
			;
		}

		if (!nano_sem_take(&context->recv_data_wait, timeout)) {
			/* timeout */
			return -ETIMEDOUT;
		}
	}
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

	return 0;
}

void net_context_init(void)
{
	nano_sem_init(&contexts_lock);

	nano_sem_give(&contexts_lock);
}
