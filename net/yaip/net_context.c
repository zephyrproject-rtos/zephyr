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

#if defined(CONFIG_NET_DEBUG_CONTEXT)
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
#include "tcp.h"

#define NET_MAX_CONTEXT CONFIG_NET_MAX_CONTEXTS

static struct net_context contexts[NET_MAX_CONTEXT];

/* We need to lock the contexts array as these APIs are typically called
 * from applications which are usually run in task context.
 */
static struct nano_sem contexts_lock;

#if defined(CONFIG_NET_TCP)
static struct sockaddr *create_sockaddr(struct net_buf *buf,
					struct sockaddr *addr)
{
#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		net_ipaddr_copy(&net_sin6(addr)->sin6_addr,
				&NET_IPV6_BUF(buf)->src);
		net_sin6(addr)->sin6_port = NET_TCP_BUF(buf)->src_port;
		net_sin6(addr)->sin6_family = AF_INET6;
	} else
#endif

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET) {
		net_ipaddr_copy(&net_sin(addr)->sin_addr,
				&NET_IPV4_BUF(buf)->src);
		net_sin(addr)->sin_port = NET_TCP_BUF(buf)->src_port;
		net_sin(addr)->sin_family = AF_INET;
	} else
#endif
	{
		return NULL;
	}

	return addr;
}
#endif

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

#if defined(CONFIG_NET_TCP)
		if (ip_proto == IPPROTO_TCP) {
			contexts[i].tcp = net_tcp_alloc(&contexts[i]);
			if (!contexts[i].tcp) {
				NET_ASSERT_INFO(contexts[i].tcp,
						"Cannot allocate TCP context");
				return -ENOBUFS;
			}
		}
#endif /* CONFIG_NET_TCP */

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

#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		net_tcp_release(context->tcp);
	}
#endif /* CONFIG_NET_TCP */

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
					    net_sin6_ptr(&contexts[i].local)->
								     sin6_addr,
					    &((struct sockaddr_in6 *)
					      local_addr)->sin6_addr)) {
					return -EEXIST;
				}
			} else {
				if (net_ipv4_addr_cmp(
					    net_sin_ptr(&contexts[i].local)->
								      sin_addr,
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

int net_context_bind(struct net_context *context, const struct sockaddr *addr,
		     socklen_t addrlen)
{
	NET_ASSERT(context && addr);
	NET_ASSERT(context >= &context[0] || \
		   context <= &context[NET_MAX_CONTEXT]);

#if defined(CONFIG_NET_IPV6)
	if (addr->family == AF_INET6) {
		struct net_if *iface;
		struct in6_addr *ptr;
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;

		if (addrlen < sizeof(struct sockaddr_in6)) {
			return -EINVAL;
		}

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

		net_sin6_ptr(&context->local)->sin6_addr = ptr;
		net_sin6_ptr(&context->local)->sin6_port = addr6->sin6_port;

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

		if (addrlen < sizeof(struct sockaddr_in)) {
			return -EINVAL;
		}

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

		net_sin_ptr(&context->local)->sin_addr =
						&ifaddr->address.in_addr;
		net_sin_ptr(&context->local)->sin_port = addr4->sin_port;

		NET_DBG("Context %p binding to %s:%d iface %p", context,
			net_sprint_ipv4_addr(&ifaddr->address.in_addr),
			ntohs(addr4->sin_port), iface);

		return 0;
	}
#endif

	return -EINVAL;
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

int net_context_listen(struct net_context *context, int backlog)
{
	ARG_UNUSED(backlog);

	NET_ASSERT(context);

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		return -EOPNOTSUPP;
	}

#if defined(CONFIG_NET_TCP)
	net_tcp_change_state(context->tcp, NET_TCP_LISTEN);
	net_context_set_state(context, NET_CONTEXT_LISTENING);
#endif

	return 0;
}

#if defined(CONFIG_NET_TCP)
#if NET_DEBUG > 0
#define net_tcp_print_recv_info(str, buf, port)		\
	do {								\
		if (net_context_get_family(context) == AF_INET6) {	\
			NET_DBG("%s received from %s port %d", str,	\
				net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src),\
				ntohs(port));				\
		} else if (net_context_get_family(context) == AF_INET) {\
			NET_DBG("%s received from %s port %d", str,	\
				net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->src),\
				ntohs(port));				\
		}							\
	} while (0)
#else
#define net_tcp_print_recv_info(...)
#endif

#if NET_DEBUG > 0
#define net_tcp_print_send_info(str, buf, port)		\
	do {								\
		if (net_context_get_family(context) == AF_INET6) {	\
			NET_DBG("%s sent to %s port %d", str,		\
				net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst),\
				ntohs(port));				\
		} else if (net_context_get_family(context) == AF_INET) {\
			NET_DBG("%s sent to %s port %d", str,		\
				net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->dst),\
				ntohs(port));				\
		}							\
	} while (0)
#else
#define net_tcp_print_send_info(...)
#endif

static inline int send_syn(struct net_context *context,
		    const struct sockaddr *remote)
{
	struct net_buf *buf;
	int ret;

	net_tcp_change_state(context->tcp, NET_TCP_SYN_SENT);

	ret = net_tcp_prepare_segment(context->tcp, NET_TCP_SYN,
				      NULL, 0, remote, &buf);
	if (ret) {
		return ret;
	}

	net_tcp_print_send_info("SYN", buf, NET_TCP_BUF(buf)->dst_port);

	ret =  net_send_data(buf);
	if (!ret) {
		net_nbuf_unref(buf);
	}

	return 0;
}

static inline int send_syn_ack(struct net_context *context,
			       struct sockaddr *remote)
{
	struct net_buf *buf;
	int ret;

	ret = net_tcp_prepare_segment(context->tcp,
				      NET_TCP_SYN | NET_TCP_ACK,
				      NULL, 0, remote, &buf);
	if (ret) {
		return ret;
	}

	net_tcp_print_send_info("SYN_ACK", buf, NET_TCP_BUF(buf)->dst_port);

	ret =  net_send_data(buf);
	if (!ret) {
		net_nbuf_unref(buf);
	}

	return 0;
}

static inline int send_ack(struct net_context *context,
			   struct sockaddr *remote)
{
	struct net_buf *buf;
	int ret;

	ret = net_tcp_prepare_ack(context->tcp, remote, &buf);
	if (ret) {
		return ret;
	}

	net_tcp_print_send_info("ACK", buf, NET_TCP_BUF(buf)->dst_port);

	ret =  net_send_data(buf);
	if (!ret) {
		net_nbuf_unref(buf);
	}

	return 0;
}

static int send_reset(struct net_context *context,
		      struct sockaddr *remote)
{
	struct net_buf *buf;
	int ret;

	ret = net_tcp_prepare_reset(context->tcp, remote, &buf);
	if (ret) {
		return ret;
	}

	net_tcp_print_send_info("RST", buf, NET_TCP_BUF(buf)->dst_port);

	ret =  net_send_data(buf);
	if (!ret) {
		net_nbuf_unref(buf);
	}

	return 0;
}

/* This is called when we receive data after the connection has been
 * established. The core TCP logic is located here.
 */
static enum net_verdict tcp_established(struct net_conn *conn,
					  struct net_buf *buf,
					  void *user_data)
{
	struct net_context *context = (struct net_context *)user_data;

	NET_ASSERT(context && context->tcp);

	net_tcp_print_recv_info("DATA", buf, NET_TCP_BUF(buf)->src_port);

	return NET_DROP;
}

static enum net_verdict tcp_synack_received(struct net_conn *conn,
					    struct net_buf *buf,
					    void *user_data)
{
	struct net_context *context = (struct net_context *)user_data;
	int ret;

	NET_ASSERT(context && context->tcp);

	switch (context->tcp->state) {
	case NET_TCP_SYN_SENT:
		net_context_set_iface(context, net_nbuf_iface(buf));
		break;
	default:
		NET_DBG("Context %p in wrong state %d",
			context, context->tcp->state);
		return NET_DROP;
	}

	net_nbuf_set_context(buf, context);

	NET_ASSERT(net_nbuf_iface(buf));

	/*
	 * If we receive SYN, we send SYN-ACK and go to SYN_RCVD state.
	 */
	if ((NET_TCP_BUF(buf)->flags & NET_TCP_CTL) ==
	    (NET_TCP_SYN | NET_TCP_ACK)) {
		struct sockaddr *laddr;
		struct sockaddr *raddr;

#if defined(CONFIG_NET_IPV6)
		struct sockaddr_in6 r6addr;
		struct sockaddr_in6 l6addr;
#endif
#if defined(CONFIG_NET_IPV4)
		struct sockaddr_in r4addr;
		struct sockaddr_in l4addr;
#endif

#if defined(CONFIG_NET_IPV6)
		if (net_nbuf_family(buf) == AF_INET6) {
			laddr = (struct sockaddr *)&l6addr;
			raddr = (struct sockaddr *)&r6addr;
		} else
#endif
#if defined(CONFIG_NET_IPV4)
		if (net_nbuf_family(buf) == AF_INET) {
			laddr = (struct sockaddr *)&l4addr;
			raddr = (struct sockaddr *)&r4addr;
		} else
#endif
		{
			NET_DBG("Invalid family (%d)", net_nbuf_family(buf));
			return NET_DROP;
		}

		/* Remove the temporary connection handler and register
		 * a proper now as we have an established connection.
		 */
		net_tcp_unregister(context->conn_handler);

		ret = net_tcp_register(raddr,
				       laddr,
				       ntohs(NET_TCP_BUF(buf)->src_port),
				       ntohs(NET_TCP_BUF(buf)->dst_port),
				       tcp_established,
				       context,
				       &context->conn_handler);
		if (ret < 0) {
			NET_DBG("Cannot register TCP handler (%d)", ret);
			send_reset(context, raddr);
			return NET_DROP;
		}

		net_tcp_change_state(context->tcp, NET_TCP_ESTABLISHED);

		send_ack(context, raddr);

		return NET_OK;
	}

	return NET_DROP;
}
#endif /* CONFIG_NET_TCP */

int net_context_connect(struct net_context *context,
			const struct sockaddr *addr,
			socklen_t addrlen,
			net_context_connect_cb_t cb,
			int32_t timeout,
			void *user_data)
{
#if defined(CONFIG_NET_TCP)
	struct sockaddr *laddr = NULL;
	struct sockaddr local_addr;
	uint16_t lport, rport;
	int ret;
#endif

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

	if (net_context_get_state(context) == NET_CONTEXT_LISTENING) {
		return -EOPNOTSUPP;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_context_get_family(context) == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)
							&context->remote;

		if (addrlen < sizeof(struct sockaddr_in6)) {
			return -EINVAL;
		}

#if defined(CONFIG_NET_TCP)
		if (net_is_ipv6_addr_mcast(&addr6->sin6_addr)) {
			return -EADDRNOTAVAIL;
		}
#endif /* CONFIG_NET_TCP */

		memcpy(&addr6->sin6_addr, &net_sin6(addr)->sin6_addr,
		       sizeof(struct in6_addr));

		addr6->sin6_port = net_sin6(addr)->sin6_port;
		addr6->sin6_family = AF_INET6;

		if (!net_is_ipv6_addr_unspecified(&addr6->sin6_addr)) {
			context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
		} else {
			context->flags &= ~NET_CONTEXT_REMOTE_ADDR_SET;
		}

#if defined(CONFIG_NET_TCP)
		rport = addr6->sin6_port;

		net_sin6_ptr(&context->local)->sin6_family = AF_INET6;
		net_sin6(&local_addr)->sin6_port = lport =
			net_sin6((struct sockaddr *)&context->local)->sin6_port;

		if (net_sin6_ptr(&context->local)->sin6_addr) {
			net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr,
				     net_sin6_ptr(&context->local)->sin6_addr);

			laddr = &local_addr;
		}
#endif
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(context) == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)
							&context->remote;

		if (addrlen < sizeof(struct sockaddr_in)) {
			return -EINVAL;
		}

#if defined(CONFIG_NET_TCP)
		/* FIXME - Add multicast and broadcast address check */
#endif /* CONFIG_NET_TCP */

		memcpy(&addr4->sin_addr, &net_sin(addr)->sin_addr,
		       sizeof(struct in_addr));

		addr4->sin_port = net_sin(addr)->sin_port;
		addr4->sin_family = AF_INET;

		if (addr4->sin_addr.s_addr[0]) {
			context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
		} else {
			context->flags &= ~NET_CONTEXT_REMOTE_ADDR_SET;
		}

#if defined(CONFIG_NET_TCP)
		rport = addr4->sin_port;

		net_sin_ptr(&context->local)->sin_family = AF_INET;
		net_sin(&local_addr)->sin_port = lport =
			net_sin((struct sockaddr *)&context->local)->sin_port;

		if (net_sin_ptr(&context->local)->sin_addr) {
			net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
				       net_sin_ptr(&context->local)->sin_addr);

			laddr = &local_addr;
		}
#endif
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
	if (net_context_get_type(context) != SOCK_STREAM) {
		return -ENOTSUP;
	}

	/* We need to register a handler, otherwise the SYN-ACK
	 * packet would not be received.
	 */
	ret = net_tcp_register(addr,
			       laddr,
			       ntohs(rport),
			       ntohs(lport),
			       tcp_synack_received,
			       context,
			       &context->conn_handler);
	if (ret < 0) {
		return ret;
	}

	net_context_set_state(context, NET_CONTEXT_CONNECTING);

	/* FIXME - set timer to wait for SYN-ACK */
	send_syn(context, addr);

	if (cb) {
		cb(context, user_data);
	}

	return 0;
#endif

	return 0;
}

#if defined(CONFIG_NET_TCP)

#define ACK_TIMEOUT (sys_clock_ticks_per_sec)

static void ack_timeout(struct nano_work *work)
{
	/* This means that we did not receive ACK response in time. */
	struct net_tcp *tcp = CONTAINER_OF(work, struct net_tcp, ack_timer);

	NET_DBG("Did not receive ACK");

	send_reset(tcp->context, &tcp->context->remote);

	net_tcp_change_state(tcp, NET_TCP_CLOSED);
}

/* This callback is called when we are waiting connections and we receive
 * a packet. We need to check if we are receiving proper msg (SYN) here.
 * The ACK could also be received, in which case we have an established
 * connection.
 */
static enum net_verdict tcp_syn_rcvd(struct net_conn *conn,
				     struct net_buf *buf,
				     void *user_data)
{
	struct net_context *context = (struct net_context *)user_data;
	struct net_tcp *tcp = context->tcp;

	NET_ASSERT(context && tcp);

	switch (tcp->state) {
	case NET_TCP_LISTEN:
		net_context_set_iface(context, net_nbuf_iface(buf));
		break;
	case NET_TCP_SYN_RCVD:
		break;
	default:
		NET_DBG("Context %p in wrong state %d",
			context, tcp->state);
		return NET_DROP;
	}

	net_nbuf_set_context(buf, context);

	NET_ASSERT(net_nbuf_iface(buf));

	/*
	 * If we receive SYN, we send SYN-ACK and go to SYN_RCVD state.
	 */
	if ((NET_TCP_BUF(buf)->flags & NET_TCP_CTL) == NET_TCP_SYN) {
		struct sockaddr peer, *remote;

		net_tcp_print_recv_info("SYN", buf, NET_TCP_BUF(buf)->src_port);

		net_tcp_change_state(tcp, NET_TCP_SYN_RCVD);

		remote = create_sockaddr(buf, &peer);

		send_syn_ack(context, remote);

		/* We might be entering this section multiple times
		 * if the SYN is sent more than once. So we need to cancel
		 * any pending timers.
		 */
		nano_delayed_work_cancel(&tcp->ack_timer);
		nano_delayed_work_init(&tcp->ack_timer, ack_timeout);
		nano_delayed_work_submit(&tcp->ack_timer, ACK_TIMEOUT);

		return NET_DROP;
	}

	/*
	 * If we receive RST, we go back to LISTEN state.
	 */
	if ((NET_TCP_BUF(buf)->flags & NET_TCP_CTL) == NET_TCP_RST) {
		nano_delayed_work_cancel(&tcp->ack_timer);

		net_tcp_print_recv_info("RST", buf, NET_TCP_BUF(buf)->src_port);

		net_tcp_change_state(tcp, NET_TCP_LISTEN);

		return NET_DROP;
	}

	/*
	 * If we receive ACK, we go to ESTABLISHED state.
	 */
	if ((NET_TCP_BUF(buf)->flags & NET_TCP_CTL) == NET_TCP_ACK) {
		struct net_context *new_context;
		struct sockaddr local_addr;
		struct net_tcp *tmp_tcp;
		socklen_t addrlen;
		int ret;

		/* We can only receive ACK if we have already received SYN.
		 * So if we are not in SYN_RCVD state, then it is an error.
		 */
		if (tcp->state != NET_TCP_SYN_RCVD) {
			nano_delayed_work_cancel(&tcp->ack_timer);
			goto reset;
		}

		net_tcp_print_recv_info("ACK", buf, NET_TCP_BUF(buf)->src_port);

		net_tcp_change_state(tcp, NET_TCP_ESTABLISHED);

		/* We create a new context that starts to wait data.
		 */
		ret = net_context_get(net_nbuf_family(buf),
				      SOCK_STREAM, IPPROTO_TCP,
				      &new_context);
		if (ret < 0) {
			NET_DBG("Cannot get accepted context, "
				"connection reset");
			goto reset;
		}

#if defined(CONFIG_NET_IPV6)
		if (net_context_get_family(context) == AF_INET6) {
			struct sockaddr_in6 *addr6 = net_sin6(&local_addr);

			addr6->sin6_family = AF_INET6;
			addr6->sin6_port = 0;
			net_ipaddr_copy(&addr6->sin6_addr,
					&NET_IPV6_BUF(buf)->dst);
			addrlen = sizeof(struct sockaddr_in6);
		} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
		if (net_context_get_family(context) == AF_INET) {
			struct sockaddr_in *addr4 = net_sin(&local_addr);

			addr4->sin_family = AF_INET;
			addr4->sin_port = 0; /* bind will allocate free port */
			net_ipaddr_copy(&addr4->sin_addr,
					&NET_IPV4_BUF(buf)->dst);
			addrlen = sizeof(struct sockaddr_in);
		} else
#endif /* CONFIG_NET_IPV6 */
		{
			NET_ASSERT_INFO(false, "Invalid protocol family %d",
					net_context_get_family(context));
			return NET_DROP;
		}

		ret = net_context_bind(new_context, &local_addr,
				       sizeof(local_addr));
		if (ret < 0) {
			NET_DBG("Cannot bind accepted context, "
				"connection reset");
			goto reset;
		}

		memcpy(&new_context->remote, &context->remote,
		       sizeof(context->remote));

		ret = net_tcp_register(&new_context->remote,
			       &local_addr,
			       ntohs(net_sin(&new_context->remote)->sin_port),
			       ntohs(net_sin(&local_addr)->sin_port),
			       tcp_established,
			       new_context,
			       new_context->conn_handler);
		if (ret < 0) {
			NET_DBG("Cannot register accepted TCP handler (%d)",
				ret);
			goto reset;
		}

		if (!context->accept_cb) {
			NET_DBG("No accept callback, connection reset.");
			goto reset;
		}

		/* The original context is left to serve another
		 * incoming request.
		 */
		tmp_tcp = new_context->tcp;
		new_context->tcp = tcp;
		context->tcp = tmp_tcp;

		net_tcp_change_state(tcp, NET_TCP_LISTEN);

		new_context->user_data = context->user_data;
		context->user_data = NULL;

		context->accept_cb(new_context,
				   &new_context->remote,
				   addrlen,
				   0,
				   new_context->user_data);
	}

	return NET_DROP;

reset:
	{
		struct sockaddr peer;

		send_reset(tcp->context, create_sockaddr(buf, &peer));
	}

	return NET_DROP;
}
#endif /* CONFIG_NET_TCP */

int net_context_accept(struct net_context *context,
		       net_context_accept_cb_t cb,
		       int32_t timeout,
		       void *user_data)
{
#if defined(CONFIG_NET_TCP)
	struct sockaddr local_addr;
	struct sockaddr *laddr = NULL;
	uint16_t lport = 0;
	int ret;
#endif /* CONFIG_NET_TCP */

	NET_ASSERT(context);

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

	if ((net_context_get_state(context) != NET_CONTEXT_LISTENING) &&
	    (net_context_get_type(context) != SOCK_STREAM)) {
		NET_DBG("Invalid socket, state %d type %d",
			net_context_get_state(context),
			net_context_get_type(context));
		return -EINVAL;
	}

#if defined(CONFIG_NET_TCP)
	if (context->tcp->state != NET_TCP_LISTEN) {
		NET_DBG("Context %p in wrong state %d, should be %d",
			context, context->tcp->state, NET_TCP_LISTEN);
		return -EINVAL;
	}

	local_addr.family = net_context_get_family(context);

#if defined(CONFIG_NET_IPV6)
	if (net_context_get_family(context) == AF_INET6) {
		if (net_sin6_ptr(&context->local)->sin6_addr) {
			net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr,
				     net_sin6_ptr(&context->local)->sin6_addr);

			laddr = &local_addr;
		}

		net_sin6(&local_addr)->sin6_port =
			net_sin6((struct sockaddr *)&context->local)->sin6_port;
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(context) == AF_INET) {
		if (net_sin_ptr(&context->local)->sin_addr) {
			net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
				      net_sin_ptr(&context->local)->sin_addr);

			laddr = &local_addr;
		}

		lport = net_sin((struct sockaddr *)&context->local)->sin_port;
	}
#endif /* CONFIG_NET_IPV4 */

	ret = net_tcp_register(context->flags & NET_CONTEXT_REMOTE_ADDR_SET ?
			       &context->remote : NULL,
			       laddr,
			       ntohs(net_sin(&context->remote)->sin_port),
			       ntohs(lport),
			       tcp_syn_rcvd,
			       context,
			       &context->conn_handler);
	if (ret < 0) {
		return ret;
	}

	context->user_data = user_data;
	context->accept_cb = cb;
#endif /* CONFIG_NET_TCP */

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
		       socklen_t addrlen,
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

		if (addrlen < sizeof(struct sockaddr_in6)) {
			return -EINVAL;
		}

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

		if (addrlen < sizeof(struct sockaddr_in)) {
			return -EINVAL;
		}

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
		if (net_sin6_ptr(&context->local)->sin6_addr) {
			net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr,
				     net_sin6_ptr(&context->local)->sin6_addr);

			laddr = &local_addr;
		}

		net_sin6(&local_addr)->sin6_port =
			net_sin6((struct sockaddr *)&context->local)->sin6_port;
		lport = net_sin6((struct sockaddr *)&context->local)->sin6_port;
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(context) == AF_INET) {
		if (net_sin_ptr(&context->local)->sin_addr) {
			net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
				      net_sin_ptr(&context->local)->sin_addr);

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
