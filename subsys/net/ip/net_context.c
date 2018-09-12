/** @file
 * @brief Network context API
 *
 * An API for applications to define a network connection.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_CONTEXT)
#define SYS_LOG_DOMAIN "net/ctx"
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/net_context.h>
#include <net/net_offload.h>
#include <net/tcp.h>

#include "connection.h"
#include "net_private.h"

#include "ipv6.h"
#include "ipv4.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "net_stats.h"

#ifndef EPFNOSUPPORT
/* Some old versions of newlib haven't got this defined in errno.h,
 * Just use EPROTONOSUPPORT in this case
 */
#define EPFNOSUPPORT EPROTONOSUPPORT
#endif

#define NET_MAX_CONTEXT CONFIG_NET_MAX_CONTEXTS

static struct net_context contexts[NET_MAX_CONTEXT];

/* We need to lock the contexts array as these APIs are typically called
 * from applications which are usually run in task context.
 */
static struct k_sem contexts_lock;

static int check_used_port(enum net_ip_protocol ip_proto,
			   u16_t local_port,
			   const struct sockaddr *local_addr)

{
	int i;

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (!net_context_is_used(&contexts[i])) {
			continue;
		}

		if (!(net_context_get_ip_proto(&contexts[i]) == ip_proto &&
		      net_sin((struct sockaddr *)&
			      contexts[i].local)->sin_port == local_port)) {
			continue;
		}

		if (local_addr->sa_family == AF_INET6) {
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

	return 0;
}

static u16_t find_available_port(struct net_context *context,
				    const struct sockaddr *addr)
{
	u16_t local_port;

	do {
		local_port = sys_rand32_get() | 0x8000;
		if (local_port <= 1023) {
			/* 0 - 1023 ports are reserved */
			continue;
		}
	} while (check_used_port(
				net_context_get_ip_proto(context),
				htons(local_port), addr) == -EEXIST);

	return htons(local_port);
}

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

	k_sem_take(&contexts_lock, K_FOREVER);

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (net_context_is_used(&contexts[i])) {
			continue;
		}

		if (ip_proto == IPPROTO_TCP) {
			if (net_tcp_get(&contexts[i]) < 0) {
				break;
			}
		}

		contexts[i].iface = 0;
		contexts[i].flags = 0;
		atomic_set(&contexts[i].refcount, 1);

		net_context_set_family(&contexts[i], family);
		net_context_set_type(&contexts[i], type);
		net_context_set_ip_proto(&contexts[i], ip_proto);

		(void)memset(&contexts[i].remote, 0, sizeof(struct sockaddr));
		(void)memset(&contexts[i].local, 0,
			     sizeof(struct sockaddr_ptr));

#if defined(CONFIG_NET_IPV6)
		if (family == AF_INET6) {
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6
						      *)&contexts[i].local;
			addr6->sin6_port = find_available_port(&contexts[i],
						    (struct sockaddr *)addr6);

			if (!addr6->sin6_port) {
				ret = -EADDRINUSE;
				break;
			}
		}
#endif

#if defined(CONFIG_NET_IPV4)
		if (family == AF_INET) {
			struct sockaddr_in *addr = (struct sockaddr_in
						      *)&contexts[i].local;
			addr->sin_port = find_available_port(&contexts[i],
						    (struct sockaddr *)addr);

			if (!addr->sin_port) {
				ret = -EADDRINUSE;
				break;
			}
		}
#endif

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
		k_sem_init(&contexts[i].recv_data_wait, 1, UINT_MAX);
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

		contexts[i].flags |= NET_CONTEXT_IN_USE;
		*context = &contexts[i];

		ret = 0;
		break;
	}

	k_sem_give(&contexts_lock);

#if defined(CONFIG_NET_OFFLOAD)
	/* FIXME - Figure out a way to get the correct network interface
	 * as it is not known at this point yet.
	 */
	if (!ret && net_if_is_ip_offloaded(net_if_get_default())) {
		ret = net_offload_get(net_if_get_default(),
				      family,
				      type,
				      ip_proto,
				      context);
		if (ret < 0) {
			(*context)->flags &= ~NET_CONTEXT_IN_USE;
			*context = NULL;
		}

		return ret;
	}
#endif /* CONFIG_NET_OFFLOAD */

	return ret;
}

int net_context_ref(struct net_context *context)
{
	int old_rc = atomic_inc(&context->refcount);

	return old_rc + 1;
}

int net_context_unref(struct net_context *context)
{
	int old_rc = atomic_dec(&context->refcount);

	if (old_rc != 1) {
		return old_rc - 1;
	}

	k_sem_take(&contexts_lock, K_FOREVER);

	net_tcp_unref(context);

	if (context->conn_handler) {
		net_conn_unregister(context->conn_handler);
		context->conn_handler = NULL;
	}

	net_context_set_state(context, NET_CONTEXT_UNCONNECTED);

	context->flags &= ~NET_CONTEXT_IN_USE;

	NET_DBG("Context %p released", context);

	k_sem_give(&contexts_lock);

	return 0;
}

int net_context_put(struct net_context *context)
{
	NET_ASSERT(context);

	if (!PART_OF_ARRAY(contexts, context)) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		k_sem_take(&contexts_lock, K_FOREVER);
		context->flags &= ~NET_CONTEXT_IN_USE;
		k_sem_give(&contexts_lock);
		return net_offload_put(
			net_context_get_iface(context), context);
	}
#endif /* CONFIG_NET_OFFLOAD */

	context->connect_cb = NULL;
	context->recv_cb = NULL;
	context->send_cb = NULL;

	if (net_tcp_put(context) >= 0) {
		return 0;
	}

	net_context_unref(context);
	return 0;
}

/* If local address is not bound, bind it to INADDR_ANY and random port. */
static int bind_default(struct net_context *context)
{
	sa_family_t family = net_context_get_family(context);

#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		struct sockaddr_in6 addr6;

		if (net_sin6_ptr(&context->local)->sin6_addr) {
			return 0;
		}

		addr6.sin6_family = AF_INET6;
		memcpy(&addr6.sin6_addr, net_ipv6_unspecified_address(),
		       sizeof(addr6.sin6_addr));
		addr6.sin6_port =
			find_available_port(context,
					    (struct sockaddr *)&addr6);

		return net_context_bind(context, (struct sockaddr *)&addr6,
					sizeof(addr6));
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		struct sockaddr_in addr4;

		if (net_sin_ptr(&context->local)->sin_addr) {
			return 0;
		}

		addr4.sin_family = AF_INET;
		addr4.sin_addr.s_addr = INADDR_ANY;
		addr4.sin_port =
			find_available_port(context,
					    (struct sockaddr *)&addr4);

		return net_context_bind(context, (struct sockaddr *)&addr4,
					sizeof(addr4));
	}
#endif

	return -EINVAL;
}

int net_context_bind(struct net_context *context, const struct sockaddr *addr,
		     socklen_t addrlen)
{
	NET_ASSERT(addr);
	NET_ASSERT(PART_OF_ARRAY(contexts, context));

	/* If we already have connection handler, then it effectively
	 * means that it's already bound to an interface/port, and we
	 * don't support rebinding connection to new address/port in
	 * the code below.
	 * TODO: Support rebinding.
	 */
	if (context->conn_handler) {
		return -EISCONN;
	}

#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		struct net_if *iface = NULL;
		struct in6_addr *ptr;
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
		int ret;

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

		} else if (net_is_ipv6_addr_unspecified(&addr6->sin6_addr)) {
			iface = net_if_get_default();

			ptr = (struct in6_addr *)net_ipv6_unspecified_address();
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
			NET_ERR("Cannot bind to %s",
				net_sprint_ipv6_addr(&addr6->sin6_addr));

			return -EADDRNOTAVAIL;
		}

#if defined(CONFIG_NET_OFFLOAD)
		if (net_if_is_ip_offloaded(iface)) {
			net_context_set_iface(context, iface);

			return net_offload_bind(iface,
						context,
						addr,
						addrlen);
		}
#endif /* CONFIG_NET_OFFLOAD */

		net_context_set_iface(context, iface);

		net_sin6_ptr(&context->local)->sin6_family = AF_INET6;
		net_sin6_ptr(&context->local)->sin6_addr = ptr;
		if (addr6->sin6_port) {
			ret = check_used_port(AF_INET6, addr6->sin6_port,
					      addr);
			if (!ret) {
				net_sin6_ptr(&context->local)->sin6_port =
					addr6->sin6_port;
			} else {
				NET_ERR("Port %d is in use!",
					ntohs(addr6->sin6_port));
				return ret;
			}
		} else {
			addr6->sin6_port =
				net_sin6_ptr(&context->local)->sin6_port;
		}

		NET_DBG("Context %p binding to %s [%s]:%d iface %p",
			context,
			net_proto2str(net_context_get_ip_proto(context)),
			net_sprint_ipv6_addr(ptr), ntohs(addr6->sin6_port),
			iface);

		return 0;
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (addr->sa_family == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
		struct net_if *iface = NULL;
		struct net_if_addr *ifaddr;
		struct in_addr *ptr;
		int ret;

		if (addrlen < sizeof(struct sockaddr_in)) {
			return -EINVAL;
		}

		if (net_is_ipv4_addr_mcast(&addr4->sin_addr)) {
			struct net_if_mcast_addr *maddr;

			maddr = net_if_ipv4_maddr_lookup(&addr4->sin_addr,
							 &iface);
			if (!maddr) {
				return -ENOENT;
			}

			ptr = &maddr->address.in_addr;

		} else if (addr4->sin_addr.s_addr == INADDR_ANY) {
			iface = net_if_get_default();

			ptr = (struct in_addr *)net_ipv4_unspecified_address();
		} else {
			ifaddr = net_if_ipv4_addr_lookup(&addr4->sin_addr,
							 &iface);
			if (!ifaddr) {
				return -ENOENT;
			}

			ptr = &ifaddr->address.in_addr;
		}

		if (!iface) {
			NET_ERR("Cannot bind to %s",
				net_sprint_ipv4_addr(&addr4->sin_addr));

			return -EADDRNOTAVAIL;
		}

#if defined(CONFIG_NET_OFFLOAD)
		if (net_if_is_ip_offloaded(iface)) {
			net_context_set_iface(context, iface);

			return net_offload_bind(iface,
						context,
						addr,
						addrlen);
		}
#endif /* CONFIG_NET_OFFLOAD */

		net_context_set_iface(context, iface);

		net_sin_ptr(&context->local)->sin_family = AF_INET;
		net_sin_ptr(&context->local)->sin_addr = ptr;
		if (addr4->sin_port) {
			ret = check_used_port(AF_INET, addr4->sin_port,
					      addr);
			if (!ret) {
				net_sin_ptr(&context->local)->sin_port =
					addr4->sin_port;
			} else {
				NET_ERR("Port %d is in use!",
					ntohs(addr4->sin_port));
				return ret;
			}
		} else {
			addr4->sin_port =
				net_sin_ptr(&context->local)->sin_port;
		}

		NET_DBG("Context %p binding to %s %s:%d iface %p",
			context,
			net_proto2str(net_context_get_ip_proto(context)),
			net_sprint_ipv4_addr(ptr),
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

	NET_ASSERT(PART_OF_ARRAY(contexts, context));

	if (!net_context_is_used(context)) {
		return -EBADF;
	}

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		return net_offload_listen(
			net_context_get_iface(context), context, backlog);
	}
#endif /* CONFIG_NET_OFFLOAD */

	if (net_tcp_listen(context) >= 0) {
		return 0;
	}

	return -EOPNOTSUPP;
}

#if defined(CONFIG_NET_IPV4)
struct net_pkt *net_context_create_ipv4(struct net_context *context,
					struct net_pkt *pkt,
					const struct in_addr *src,
					const struct in_addr *dst)
{
	NET_ASSERT(((struct sockaddr_in_ptr *)&context->local)->sin_addr);

	if (!src) {
		src = ((struct sockaddr_in_ptr *)&context->local)->sin_addr;
	}

	if (net_is_ipv4_addr_unspecified(src)
	    || net_is_ipv4_addr_mcast(src)) {
		src = net_if_ipv4_select_src_addr(net_pkt_iface(pkt),
						  (struct in_addr *)dst);
	}

	return net_ipv4_create(pkt,
			       src,
			       dst,
			       net_context_get_iface(context),
			       net_context_get_ip_proto(context));
}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6)
struct net_pkt *net_context_create_ipv6(struct net_context *context,
					struct net_pkt *pkt,
					const struct in6_addr *src,
					const struct in6_addr *dst)
{
	NET_ASSERT(((struct sockaddr_in6_ptr *)&context->local)->sin6_addr);

	if (!src) {
		src = ((struct sockaddr_in6_ptr *)&context->local)->sin6_addr;
	}

	if (net_is_ipv6_addr_unspecified(src)
	    || net_is_ipv6_addr_mcast(src)) {
		src = net_if_ipv6_select_src_addr(net_pkt_iface(pkt),
						  (struct in6_addr *)dst);
	}

	return net_ipv6_create(pkt,
			       src,
			       dst,
			       net_context_get_iface(context),
			       net_context_get_ip_proto(context));
}
#endif /* CONFIG_NET_IPV6 */

int net_context_connect(struct net_context *context,
			const struct sockaddr *addr,
			socklen_t addrlen,
			net_context_connect_cb_t cb,
			s32_t timeout,
			void *user_data)
{
	struct sockaddr *laddr = NULL;
	struct sockaddr local_addr;
	u16_t lport, rport;
	int ret;

	NET_ASSERT(addr);
	NET_ASSERT(PART_OF_ARRAY(contexts, context));

	if (!net_context_is_used(context)) {
		return -EBADF;
	}

	ret = bind_default(context);
	if (ret) {
		return ret;
	}

	if (addr->sa_family != net_context_get_family(context)) {
		NET_ASSERT_INFO(addr->sa_family == \
				net_context_get_family(context),
				"Family mismatch %d should be %d",
				addr->sa_family,
				net_context_get_family(context));
		return -EINVAL;
	}

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		return net_offload_connect(
			net_context_get_iface(context),
			context,
			addr,
			addrlen,
			cb,
			timeout,
			user_data);
	}
#endif /* CONFIG_NET_OFFLOAD */

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

		if (net_context_get_ip_proto(context) == IPPROTO_TCP &&
		    net_is_ipv6_addr_mcast(&addr6->sin6_addr)) {
			return -EADDRNOTAVAIL;
		}

		memcpy(&addr6->sin6_addr, &net_sin6(addr)->sin6_addr,
		       sizeof(struct in6_addr));

		addr6->sin6_port = net_sin6(addr)->sin6_port;
		addr6->sin6_family = AF_INET6;

		if (!net_is_ipv6_addr_unspecified(&addr6->sin6_addr)) {
			context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
		} else {
			context->flags &= ~NET_CONTEXT_REMOTE_ADDR_SET;
		}

		rport = addr6->sin6_port;

		net_sin6_ptr(&context->local)->sin6_family = AF_INET6;
		net_sin6(&local_addr)->sin6_family = AF_INET6;
		net_sin6(&local_addr)->sin6_port = lport =
			net_sin6((struct sockaddr *)&context->local)->sin6_port;

		if (net_sin6_ptr(&context->local)->sin6_addr) {
			net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr,
				     net_sin6_ptr(&context->local)->sin6_addr);

			laddr = &local_addr;
		}
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(context) == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)
							&context->remote;

		if (addrlen < sizeof(struct sockaddr_in)) {
			return -EINVAL;
		}

		/* FIXME - Add multicast and broadcast address check */

		addr4 = (struct sockaddr_in *)&context->remote;

		memcpy(&addr4->sin_addr, &net_sin(addr)->sin_addr,
		       sizeof(struct in_addr));

		addr4->sin_port = net_sin(addr)->sin_port;
		addr4->sin_family = AF_INET;

		if (addr4->sin_addr.s_addr) {
			context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
		} else {
			context->flags &= ~NET_CONTEXT_REMOTE_ADDR_SET;
		}

		rport = addr4->sin_port;

		net_sin_ptr(&context->local)->sin_family = AF_INET;
		net_sin(&local_addr)->sin_family = AF_INET;
		net_sin(&local_addr)->sin_port = lport =
			net_sin((struct sockaddr *)&context->local)->sin_port;

		if (net_sin_ptr(&context->local)->sin_addr) {
			net_ipaddr_copy(&net_sin(&local_addr)->sin_addr,
				       net_sin_ptr(&context->local)->sin_addr);

			laddr = &local_addr;
		}
	} else
#endif /* CONFIG_NET_IPV4 */

	{
		return -EINVAL; /* Not IPv4 or IPv6 */
	}

	switch (net_context_get_type(context)) {
#if defined(CONFIG_NET_UDP)
	case SOCK_DGRAM:
		if (cb) {
			cb(context, 0, user_data);
		}

		return 0;

#endif /* CONFIG_NET_UDP */

	case SOCK_STREAM:
		return net_tcp_connect(context, addr, laddr, rport, lport,
				       timeout, cb, user_data);

	default:
		return -ENOTSUP;
	}

	return 0;
}

int net_context_accept(struct net_context *context,
		       net_tcp_accept_cb_t cb,
		       s32_t timeout,
		       void *user_data)
{
	NET_ASSERT(PART_OF_ARRAY(contexts, context));

	if (!net_context_is_used(context)) {
		return -EBADF;
	}

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		return net_offload_accept(
			net_context_get_iface(context),
			context,
			cb,
			timeout,
			user_data);
	}
#endif /* CONFIG_NET_OFFLOAD */

	if ((net_context_get_state(context) != NET_CONTEXT_LISTENING) &&
	    (net_context_get_type(context) != SOCK_STREAM)) {
		NET_DBG("Invalid socket, state %d type %d",
			net_context_get_state(context),
			net_context_get_type(context));
		return -EINVAL;
	}

	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		return net_tcp_accept(context, cb, user_data);
	}

	return 0;
}

#if defined(CONFIG_NET_UDP)
static int create_udp_packet(struct net_context *context,
			     struct net_pkt *pkt,
			     const struct sockaddr *dst_addr,
			     struct net_pkt **out_pkt)
{
	int r = 0;
	struct net_pkt *tmp;

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)dst_addr;

		if (!net_context_create_ipv6(context, pkt,
					     NULL, &addr6->sin6_addr)) {
			return -ENOMEM;
		}

		tmp = net_udp_insert(pkt,
				     net_pkt_ip_hdr_len(pkt) +
				     net_pkt_ipv6_ext_len(pkt),
				     net_sin((struct sockaddr *)
					     &context->local)->sin_port,
				     addr6->sin6_port);
		if (!tmp) {
			return -ENOMEM;
		}

		pkt = tmp;

		r = net_ipv6_finalize(pkt, net_context_get_ip_proto(context));
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)dst_addr;

		if (!net_context_create_ipv4(context, pkt,
					     NULL, &addr4->sin_addr)) {
			return -ENOMEM;
		}

		tmp = net_udp_insert(pkt, net_pkt_ip_hdr_len(pkt),
				     net_sin((struct sockaddr *)
					     &context->local)->sin_port,
				     addr4->sin_port);
		if (!tmp) {
			return -ENOMEM;
		}

		pkt = tmp;

		net_ipv4_finalize(pkt, net_context_get_ip_proto(context));
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		return -EPROTONOSUPPORT;
	}

	*out_pkt = pkt;

	return r;
}
#endif /* CONFIG_NET_UDP */

static int sendto(struct net_pkt *pkt,
		  const struct sockaddr *dst_addr,
		  socklen_t addrlen,
		  net_context_send_cb_t cb,
		  s32_t timeout,
		  void *token,
		  void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);
	int ret = 0;

	if (!net_context_is_used(context)) {
		return -EBADF;
	}

	if (!dst_addr) {
		return -EDESTADDRREQ;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)dst_addr;

		if (addrlen < sizeof(struct sockaddr_in6)) {
			return -EINVAL;
		}

		if (net_is_ipv6_addr_unspecified(&addr6->sin6_addr)) {
			return -EDESTADDRREQ;
		}
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)dst_addr;

		if (addrlen < sizeof(struct sockaddr_in)) {
			return -EINVAL;
		}

		if (!addr4->sin_addr.s_addr) {
			return -EDESTADDRREQ;
		}
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		NET_DBG("Invalid protocol family %d", net_pkt_family(pkt));
		return -EINVAL;
	}

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(net_pkt_iface(pkt))) {
		return net_offload_sendto(
			net_pkt_iface(pkt),
			pkt, dst_addr, addrlen,
			cb, timeout, token, user_data);
	}
#endif /* CONFIG_NET_OFFLOAD */

	switch (net_context_get_ip_proto(context)) {
	case IPPROTO_UDP:
#if defined(CONFIG_NET_UDP)
		/* Bind default address and port only if UDP */
		ret = bind_default(context);
		if (ret) {
			return ret;
		}

		ret = create_udp_packet(context, pkt, dst_addr, &pkt);
#endif /* CONFIG_NET_UDP */
		break;

	case IPPROTO_TCP:
		ret = net_tcp_queue_data(context, pkt);
		break;

	default:
		ret = -EPROTONOSUPPORT;
	}

	if (ret < 0) {
		if (ret == -EPROTONOSUPPORT) {
			NET_DBG("Unknown protocol while sending packet: %d",
				net_context_get_ip_proto(context));
		} else {
			NET_DBG("Could not create network packet to send (%d)",
				ret);
		}

		return ret;
	}

	context->send_cb = cb;
	context->user_data = user_data;
	net_pkt_set_token(pkt, token);

	switch (net_context_get_ip_proto(context)) {
	case IPPROTO_UDP:
		return net_send_data(pkt);

	case IPPROTO_TCP:
		return net_tcp_send_data(context, cb, token, user_data);

	default:
		return -EPROTONOSUPPORT;
	}
}

int net_context_send(struct net_pkt *pkt,
		     net_context_send_cb_t cb,
		     s32_t timeout,
		     void *token,
		     void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);
	socklen_t addrlen;

	NET_ASSERT(PART_OF_ARRAY(contexts, context));

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(net_pkt_iface(pkt))) {
		return net_offload_send(
			net_pkt_iface(pkt),
			pkt, cb, timeout,
			token, user_data);
	}
#endif /* CONFIG_NET_OFFLOAD */

	if (!(context->flags & NET_CONTEXT_REMOTE_ADDR_SET) ||
	    !net_sin(&context->remote)->sin_port) {
		return -EDESTADDRREQ;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		addrlen = sizeof(struct sockaddr_in6);
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		addrlen = sizeof(struct sockaddr_in);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		addrlen = 0;
	}

	return sendto(pkt, &context->remote, addrlen, cb, timeout, token,
		      user_data);
}

int net_context_sendto(struct net_pkt *pkt,
		       const struct sockaddr *dst_addr,
		       socklen_t addrlen,
		       net_context_send_cb_t cb,
		       s32_t timeout,
		       void *token,
		       void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);

	NET_ASSERT(PART_OF_ARRAY(contexts, context));

	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		/* Match POSIX behavior and ignore dst_address and addrlen */
		return net_context_send(pkt, cb, timeout, token, user_data);
	}

	return sendto(pkt, dst_addr, addrlen, cb, timeout, token, user_data);
}

enum net_verdict net_context_packet_received(struct net_conn *conn,
					     struct net_pkt *pkt,
					     void *user_data)
{
	struct net_context *context = find_context(conn);

	NET_ASSERT(context);
	NET_ASSERT(net_pkt_iface(pkt));

	net_context_set_iface(context, net_pkt_iface(pkt));
	net_pkt_set_context(pkt, context);

	/* If there is no callback registered, then we can only drop
	 * the packet.
	 */

	if (!context->recv_cb) {
		return NET_DROP;
	}

	if (net_context_get_ip_proto(context) != IPPROTO_TCP) {
		/* TCP packets get appdata earlier in tcp_established(). */
		net_pkt_set_appdata_values(pkt, IPPROTO_UDP);
	} else {
		net_stats_update_tcp_recv(net_pkt_iface(pkt),
					  net_pkt_appdatalen(pkt));
	}

	NET_DBG("Set appdata %p to len %u (total %zu)",
		net_pkt_appdata(pkt), net_pkt_appdatalen(pkt),
		net_pkt_get_len(pkt));

	context->recv_cb(context, pkt, 0, user_data);

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
	k_sem_give(&context->recv_data_wait);
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

	return NET_OK;
}

#if defined(CONFIG_NET_UDP)
static int recv_udp(struct net_context *context,
		    net_context_recv_cb_t cb,
		    s32_t timeout,
		    void *user_data)
{
	struct sockaddr local_addr = {
		.sa_family = net_context_get_family(context),
	};
	struct sockaddr *laddr = NULL;
	u16_t lport = 0;
	int ret;

	ARG_UNUSED(timeout);

	if (context->conn_handler) {
		net_conn_unregister(context->conn_handler);
		context->conn_handler = NULL;
	}

	ret = bind_default(context);
	if (ret) {
		return ret;
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
				net_context_packet_received,
				user_data,
				&context->conn_handler);

	return ret;
}
#endif /* CONFIG_NET_UDP */

int net_context_recv(struct net_context *context,
		     net_context_recv_cb_t cb,
		     s32_t timeout,
		     void *user_data)
{
	int ret;
	NET_ASSERT(context);

	if (!net_context_is_used(context)) {
		return -EBADF;
	}

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		return net_offload_recv(
			net_context_get_iface(context),
			context, cb, timeout, user_data);
	}
#endif /* CONFIG_NET_OFFLOAD */

	switch (net_context_get_ip_proto(context)) {
#if defined(CONFIG_NET_UDP)
	case IPPROTO_UDP:
		ret = recv_udp(context, cb, timeout, user_data);
		break;
#endif /* CONFIG_NET_UDP */

	case IPPROTO_TCP:
		ret = net_tcp_recv(context, cb, user_data);
		break;

	default:
		ret = -EPROTOTYPE;
		break;
	}

	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
	if (timeout) {
		int ret;

		/* Make sure we have the lock, then the
		 * net_context_packet_received() callback will release the
		 * semaphore when data has been received.
		 */
		k_sem_reset(&context->recv_data_wait);

		ret = k_sem_take(&context->recv_data_wait, timeout);
		if (ret == -EAGAIN) {
			return -ETIMEDOUT;
		}
	}
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

	return 0;
}

int net_context_update_recv_wnd(struct net_context *context,
				s32_t delta) {
	return net_tcp_update_recv_wnd(context, delta);
}

static int set_context_priority(struct net_context *context,
				const void *value, size_t len)
{
#if defined(CONFIG_NET_CONTEXT_PRIORITY)
	if (len > sizeof(u8_t)) {
		return -EINVAL;
	}

	context->options.priority = *((u8_t *)value);

	return 0;
#else
	return -ENOTSUP;
#endif
}

static int get_context_priority(struct net_context *context,
				void *value, size_t *len)
{
#if defined(CONFIG_NET_CONTEXT_PRIORITY)
	*((u8_t *)value) = context->options.priority;

	if (len) {
		*len = sizeof(u8_t);
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}

int net_context_set_option(struct net_context *context,
			   enum net_context_option option,
			   const void *value, size_t len)
{
	int ret = 0;

	NET_ASSERT(context);

	if (!PART_OF_ARRAY(contexts, context)) {
		return -EINVAL;
	}

	switch (option) {
	case NET_OPT_PRIORITY:
		ret = set_context_priority(context, value, len);
		break;
	}

	return ret;
}

int net_context_get_option(struct net_context *context,
			    enum net_context_option option,
			    void *value, size_t *len)
{
	int ret = 0;

	NET_ASSERT(context);

	if (!PART_OF_ARRAY(contexts, context)) {
		return -EINVAL;
	}

	switch (option) {
	case NET_OPT_PRIORITY:
		ret = get_context_priority(context, value, len);
		break;
	}

	return ret;
}

void net_context_foreach(net_context_cb_t cb, void *user_data)
{
	int i;

	k_sem_take(&contexts_lock, K_FOREVER);

	for (i = 0; i < NET_MAX_CONTEXT; i++) {
		if (!net_context_is_used(&contexts[i])) {
			continue;
		}

		cb(&contexts[i], user_data);
	}

	k_sem_give(&contexts_lock);
}

void net_context_init(void)
{
	k_sem_init(&contexts_lock, 1, UINT_MAX);
}
