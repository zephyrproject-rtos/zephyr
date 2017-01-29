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

#include <net/nbuf.h>
#include <net/net_ip.h>
#include <net/net_context.h>
#include <net/offload_ip.h>

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
static struct k_sem contexts_lock;

enum net_verdict packet_received(struct net_conn *conn,
				 struct net_buf *buf,
				 void *user_data);

static void set_appdata_values(struct net_buf *buf,
			       enum net_ip_protocol proto,
			       size_t total_len);

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

	k_sem_take(&contexts_lock, K_FOREVER);

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
		k_sem_init(&contexts[i].recv_data_wait, 0, UINT_MAX);
		k_sem_give(&contexts[i].recv_data_wait);
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

		*context = &contexts[i];

		ret = 0;
		break;
	}

	k_sem_give(&contexts_lock);

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	/* FIXME - Figure out a way to get the correct network interface
	 * as it is not known at this point yet.
	 */
	if (!ret && net_if_is_ip_offloaded(net_if_get_default())) {
		ret = net_l2_offload_ip_get(net_if_get_default(),
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
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

	return ret;
}

#if defined(CONFIG_NET_TCP)
static inline int send_fin(struct net_context *context,
			   struct sockaddr *remote);
static enum net_verdict tcp_active_close(struct net_conn *conn,
					 struct net_buf *buf,
					 void *user_data);

static bool send_fin_if_active_close(struct net_context *context)
{
	NET_ASSERT(context->tcp);

	switch (net_tcp_get_state(context->tcp)) {
	case NET_TCP_SYN_RCVD:
	case NET_TCP_ESTABLISHED:
		/* Sending a packet with the FIN flag automatically
		 * transitions to FIN_WAIT_1
		 */
		send_fin(context, &context->remote);
		net_conn_change_callback(context->conn_handler,
					 tcp_active_close, context);
		return true;
	default:
		return false;
	}
}
#endif /* CONFIG_NET_TCP */

int net_context_put(struct net_context *context)
{
	NET_ASSERT(context);

	if (!PART_OF_ARRAY(contexts, context)) {
		return -EINVAL;
	}

	k_sem_take(&contexts_lock, K_FOREVER);

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		context->flags &= ~NET_CONTEXT_IN_USE;
		return net_l2_offload_ip_put(
			net_context_get_iface(context), context);
	}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		if (send_fin_if_active_close(context)) {
			NET_DBG("TCP connection in active close, not "
				"disposing yet");
			goto still_in_use;
		}

		if (context->tcp) {
			net_tcp_release(context->tcp);
		}
	}
#endif /* CONFIG_NET_TCP */

	if (context->conn_handler) {
		net_conn_unregister(context->conn_handler);
	}

	context->flags &= ~NET_CONTEXT_IN_USE;

#if defined(CONFIG_NET_TCP)
still_in_use:
#endif /* CONFIG_NET_TCP */

	k_sem_give(&contexts_lock);

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
	if (!net_sin(addr)->sin_port) {
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
	NET_ASSERT(addr);
	NET_ASSERT(PART_OF_ARRAY(contexts, context));

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
			NET_DBG("Cannot bind to %s",
				net_sprint_ipv6_addr(&addr6->sin6_addr));

			return -EADDRNOTAVAIL;
		}

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
		if (net_if_is_ip_offloaded(iface)) {
			net_context_set_iface(context, iface);

			return net_l2_offload_ip_bind(iface,
						      context,
						      addr,
						      addrlen);
		}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

		addr6->sin6_port = find_available_port(context,
						     (struct sockaddr *)addr6);
		if (!addr6->sin6_port) {
			return -EADDRINUSE;
		}

		net_context_set_iface(context, iface);

		net_sin6_ptr(&context->local)->sin6_family = AF_INET6;
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
		struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
		struct net_if_addr *ifaddr;
		struct net_if *iface;
		struct in_addr *ptr;

		if (addrlen < sizeof(struct sockaddr_in)) {
			return -EINVAL;
		}

		if (addr4->sin_addr.s_addr[0] == INADDR_ANY) {
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
			NET_DBG("Cannot bind to %s",
				net_sprint_ipv4_addr(&addr4->sin_addr));

			return -EADDRNOTAVAIL;
		}

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
		if (net_if_is_ip_offloaded(iface)) {
			net_context_set_iface(context, iface);

			return net_l2_offload_ip_bind(iface,
						      context,
						      addr,
						      addrlen);
		}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

		addr4->sin_port = find_available_port(context,
						     (struct sockaddr *)addr4);
		if (!addr4->sin_port) {
			return -EADDRINUSE;
		}

		net_context_set_iface(context, iface);

		net_sin_ptr(&context->local)->sin_family = AF_INET;
		net_sin_ptr(&context->local)->sin_addr = ptr;
		net_sin_ptr(&context->local)->sin_port = addr4->sin_port;

		NET_DBG("Context %p binding to %s:%d iface %p", context,
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
		return -ENOENT;
	}

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		return net_l2_offload_ip_listen(
			net_context_get_iface(context), context, backlog);
	}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		net_tcp_change_state(context->tcp, NET_TCP_LISTEN);
		net_context_set_state(context, NET_CONTEXT_LISTENING);

		return 0;
	}
#endif

	return -EOPNOTSUPP;
}

#if defined(CONFIG_NET_TCP)
#if defined(CONFIG_NET_DEBUG_CONTEXT)
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
#define net_tcp_print_recv_info(...)
#define net_tcp_print_send_info(...)
#endif /* CONFIG_NET_DEBUG_CONTEXT */

static inline int send_control_segment(struct net_context *context,
				       const struct sockaddr_ptr *local,
				       const struct sockaddr *remote,
				       int flags, const char *msg)
{
	struct net_buf *buf = NULL;
	int ret;

	ret = net_tcp_prepare_segment(context->tcp, flags, NULL, 0,
				      local, remote, &buf);
	if (ret) {
		return ret;
	}

	ret = net_send_data(buf);
	if (ret < 0) {
		net_nbuf_unref(buf);
	}

	net_tcp_print_send_info(msg, buf, NET_TCP_BUF(buf)->dst_port);

	return ret;
}

static inline int send_syn(struct net_context *context,
			   const struct sockaddr *remote)
{
	net_tcp_change_state(context->tcp, NET_TCP_SYN_SENT);

	return send_control_segment(context, NULL, remote, NET_TCP_SYN, "SYN");
}

static inline int send_syn_ack(struct net_context *context,
			       struct sockaddr_ptr *local,
			       struct sockaddr *remote)
{
	return send_control_segment(context, local, remote,
				    NET_TCP_SYN | NET_TCP_ACK,
				    "SYN_ACK");
}

static inline int send_fin(struct net_context *context,
			   struct sockaddr *remote)
{
	return send_control_segment(context, NULL, remote, NET_TCP_FIN, "FIN");
}

static inline int send_fin_ack(struct net_context *context,
			       struct sockaddr *remote)
{
	return send_control_segment(context, NULL, remote,
				    NET_TCP_FIN | NET_TCP_ACK, "FIN_ACK");
}

static inline int send_ack(struct net_context *context,
			   struct sockaddr *remote)
{
	struct net_buf *buf = NULL;
	int ret;

	/* Something (e.g. a data transmission under the user
	 * callback) already sent the ACK, no need
	 */
	if (context->tcp->send_ack == context->tcp->sent_ack) {
		return 0;
	}

	ret = net_tcp_prepare_ack(context->tcp, remote, &buf);
	if (ret) {
		return ret;
	}

	net_tcp_print_send_info("ACK", buf, NET_TCP_BUF(buf)->dst_port);

	ret = net_tcp_send_buf(buf);
	if (ret < 0) {
		net_nbuf_unref(buf);
	}

	return ret;
}

static int send_reset(struct net_context *context,
		      struct sockaddr *remote)
{
	struct net_buf *buf = NULL;
	int ret;

	ret = net_tcp_prepare_reset(context->tcp, remote, &buf);
	if (ret) {
		return ret;
	}

	net_tcp_print_send_info("RST", buf, NET_TCP_BUF(buf)->dst_port);

	ret = net_send_data(buf);
	if (ret < 0) {
		net_nbuf_unref(buf);
	}

	return ret;
}

static int tcp_hdr_len(struct net_buf *buf)
{
	/* "Offset": 4-bit field in high nibble, units of dwords */
	struct net_tcp_hdr *hdr = (void *)net_nbuf_tcp_data(buf);

	return 4 * (hdr->offset >> 4);
}

static enum net_verdict tcp_passive_close(struct net_conn *conn,
					  struct net_buf *buf,
					  void *user_data)
{
	struct net_context *context = (struct net_context *)user_data;

	NET_ASSERT(context && context->tcp);

	switch (net_tcp_get_state(context->tcp)) {
	case NET_TCP_CLOSE_WAIT:
	case NET_TCP_LAST_ACK:
		break;
	default:
		NET_DBG("Context %p in wrong state %d",
			context, net_tcp_get_state(context->tcp));
		return NET_DROP;
	}

	net_tcp_print_recv_info("PASSCLOSE", buf, NET_TCP_BUF(buf)->src_port);

	if (net_tcp_get_state(context->tcp) == NET_TCP_LAST_ACK &&
	    NET_TCP_FLAGS(buf) & NET_TCP_ACK) {
		NET_DBG("ACK received in LAST_ACK, disposing of connection");
		net_context_put(context);
	}

	return NET_DROP;
}

/* This is called when we receive data after the connection has been
 * established. The core TCP logic is located here.
 */
static enum net_verdict tcp_established(struct net_conn *conn,
					struct net_buf *buf,
					void *user_data)
{
	struct net_context *context = (struct net_context *)user_data;
	enum net_verdict ret;
	uint8_t tcp_flags;

	NET_ASSERT(context && context->tcp);

	if (net_tcp_get_state(context->tcp) != NET_TCP_ESTABLISHED) {
		NET_DBG("Context %p in wrong state %d",
			context, net_tcp_get_state(context->tcp));
		return NET_DROP;
	}

	net_tcp_print_recv_info("DATA", buf, NET_TCP_BUF(buf)->src_port);

	tcp_flags = NET_TCP_FLAGS(buf);
	if (tcp_flags & NET_TCP_ACK) {
		net_tcp_ack_received(context,
				     sys_get_be32(NET_TCP_BUF(buf)->ack));
	}

	if (sys_get_be32(NET_TCP_BUF(buf)->seq) - context->tcp->send_ack) {
		/* Don't try to reorder packets.  If it doesn't
		 * match the next segment exactly, drop and wait for
		 * retransmit
		 */
		return NET_DROP;
	}

	set_appdata_values(buf, IPPROTO_TCP, net_buf_frags_len(buf));
	context->tcp->send_ack += net_nbuf_appdatalen(buf);

	ret = packet_received(conn, buf, context->tcp->recv_user_data);

	if (tcp_flags & NET_TCP_FIN) {
		/* Sending an ACK in the CLOSE_WAIT state will transition to
		 * LAST_ACK state
		 */
		net_tcp_change_state(context->tcp, NET_TCP_CLOSE_WAIT);
		net_conn_change_callback(context->conn_handler,
					 tcp_passive_close, context);

		context->tcp->send_ack += 1;

		if (context->recv_cb) {
			context->recv_cb(context, NULL, 0,
					 context->tcp->recv_user_data);
		}
	}

	send_ack(context, &conn->remote_addr);

	return ret;
}

static enum net_verdict tcp_active_close(struct net_conn *conn,
					 struct net_buf *buf,
					 void *user_data)
{
	struct net_context *context = (struct net_context *)user_data;
	struct net_tcp *tcp;

	NET_ASSERT(context && context->tcp);

	tcp = context->tcp;

	if (NET_TCP_FLAGS(buf) == NET_TCP_FIN) {
		if (net_tcp_get_state(tcp) == NET_TCP_FIN_WAIT_1 ||
		    net_tcp_get_state(tcp) == NET_TCP_FIN_WAIT_2) {
			/* Sending an ACK in FIN_WAIT_1 will transition
			 * to CLOSING, and to TIME_WAIT if on FIN_WAIT_2
			 */
			send_ack(context, &context->remote);
			return NET_DROP;
		}
	} else if (NET_TCP_FLAGS(buf) == NET_TCP_ACK) {
		if (net_tcp_get_state(tcp) == NET_TCP_FIN_WAIT_1) {
			net_tcp_change_state(tcp, NET_TCP_FIN_WAIT_2);
			return NET_DROP;
		}

		if (net_tcp_get_state(tcp) == NET_TCP_CLOSING) {
			net_tcp_change_state(tcp, NET_TCP_TIME_WAIT);
			return NET_DROP;
		}
	} else if (NET_TCP_FLAGS(buf) == (NET_TCP_FIN | NET_TCP_ACK)) {
		if (net_tcp_get_state(tcp) == NET_TCP_FIN_WAIT_1) {
			send_fin_ack(context, &context->remote);
			return NET_DROP;
		}
	}

	NET_DBG("Context %p in wrong state %d", context, tcp->state);
	return NET_DROP;
}

static enum net_verdict tcp_synack_received(struct net_conn *conn,
					    struct net_buf *buf,
					    void *user_data)
{
	struct net_context *context = (struct net_context *)user_data;
	int ret;

	NET_ASSERT(context && context->tcp);

	switch (net_tcp_get_state(context->tcp)) {
	case NET_TCP_SYN_SENT:
		net_context_set_iface(context, net_nbuf_iface(buf));
		break;
	default:
		NET_DBG("Context %p in wrong state %d",
			context, net_tcp_get_state(context->tcp));
		return NET_DROP;
	}

	net_nbuf_set_context(buf, context);

	NET_ASSERT(net_nbuf_iface(buf));

	if (NET_TCP_FLAGS(buf) & NET_TCP_RST) {
		if (context->connect_cb) {
			context->connect_cb(context, -ECONNREFUSED,
					    context->user_data);
		}

		return NET_DROP;
	}

	if (NET_TCP_FLAGS(buf) & NET_TCP_SYN) {
		context->tcp->send_ack =
			sys_get_be32(NET_TCP_BUF(buf)->seq) + 1;
		context->tcp->recv_max_ack = context->tcp->send_seq + 1;
	}
	/*
	 * If we receive SYN, we send SYN-ACK and go to SYN_RCVD state.
	 */
	if (NET_TCP_FLAGS(buf) == (NET_TCP_SYN | NET_TCP_ACK)) {
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

			r6addr.sin6_family = AF_INET6;
			r6addr.sin6_port = NET_TCP_BUF(buf)->src_port;
			net_ipaddr_copy(&r6addr.sin6_addr,
					&NET_IPV6_BUF(buf)->src);

			l6addr.sin6_family = AF_INET6;
			l6addr.sin6_port = NET_TCP_BUF(buf)->dst_port;
			net_ipaddr_copy(&l6addr.sin6_addr,
					&NET_IPV6_BUF(buf)->dst);
		} else
#endif
#if defined(CONFIG_NET_IPV4)
		if (net_nbuf_family(buf) == AF_INET) {
			laddr = (struct sockaddr *)&l4addr;
			raddr = (struct sockaddr *)&r4addr;

			r4addr.sin_family = AF_INET;
			r4addr.sin_port = NET_TCP_BUF(buf)->src_port;
			net_ipaddr_copy(&r4addr.sin_addr,
					&NET_IPV4_BUF(buf)->src);

			l4addr.sin_family = AF_INET;
			l4addr.sin_port = NET_TCP_BUF(buf)->dst_port;
			net_ipaddr_copy(&l4addr.sin_addr,
					&NET_IPV4_BUF(buf)->dst);
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
		net_context_set_state(context, NET_CONTEXT_CONNECTED);

		if (context->connect_cb) {
			context->connect_cb(context, 0, context->user_data);
		}

		send_ack(context, raddr);

		k_sem_give(&context->tcp->connect_wait);
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

	NET_ASSERT(addr);
	NET_ASSERT(PART_OF_ARRAY(contexts, context));
#if defined(CONFIG_NET_TCP)
	NET_ASSERT(context->tcp);
#endif

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

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		return net_l2_offload_ip_connect(
			net_context_get_iface(context),
			context,
			addr,
			addrlen,
			cb,
			timeout,
			user_data);
	}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

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
		net_sin6(&local_addr)->sin6_family = AF_INET6;
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
		net_sin(&local_addr)->sin_family = AF_INET;
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
			cb(context, 0, user_data);
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

	send_syn(context, addr);

	context->connect_cb = cb;
	context->user_data = user_data;

	/* in tcp_synack_received() we give back this semaphore */
	if (timeout > 0 && k_sem_take(&context->tcp->connect_wait, timeout)) {
		return -ETIMEDOUT;
	}
#endif

	return 0;
}

#if defined(CONFIG_NET_TCP)

#define ACK_TIMEOUT MSEC_PER_SEC

static void ack_timeout(struct k_work *work)
{
	/* This means that we did not receive ACK response in time. */
	struct net_tcp *tcp = CONTAINER_OF(work, struct net_tcp, ack_timer);

	NET_DBG("Did not receive ACK in %dms", ACK_TIMEOUT);

	send_reset(tcp->context, &tcp->context->remote);

	net_tcp_change_state(tcp, NET_TCP_LISTEN);
}

static void buf_get_sockaddr(sa_family_t family, struct net_buf *buf,
			     struct sockaddr_ptr *addr)
{
	memset(addr, 0, sizeof(*addr));

#if defined(CONFIG_NET_IPV4)
	if (family == AF_INET) {
		struct sockaddr_in_ptr *addr4 = net_sin_ptr(addr);

		addr4->sin_family = AF_INET;
		addr4->sin_port = NET_TCP_BUF(buf)->dst_port;
		addr4->sin_addr = &NET_IPV4_BUF(buf)->dst;
	}
#endif

#if defined(CONFIG_NET_IPV6)
	if (family == AF_INET6) {
		struct sockaddr_in6_ptr *addr6 = net_sin6_ptr(addr);

		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = NET_TCP_BUF(buf)->dst_port;
		addr6->sin6_addr = &NET_IPV6_BUF(buf)->dst;
	}
#endif
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
	struct net_tcp *tcp;
	struct sockaddr_ptr buf_src_addr;

	NET_ASSERT(context && context->tcp);

	tcp = context->tcp;

	switch (net_tcp_get_state(tcp)) {
	case NET_TCP_LISTEN:
		net_context_set_iface(context, net_nbuf_iface(buf));
		break;
	case NET_TCP_SYN_RCVD:
		if (net_nbuf_iface(buf) != net_context_get_iface(context)) {
			return NET_DROP;
		}
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
	if (NET_TCP_FLAGS(buf) == NET_TCP_SYN) {
		struct sockaddr peer, *remote;

		net_tcp_print_recv_info("SYN", buf, NET_TCP_BUF(buf)->src_port);

		net_tcp_change_state(tcp, NET_TCP_SYN_RCVD);

		remote = create_sockaddr(buf, &peer);

		/* FIXME: Is this the correct place to set tcp->send_ack? */
		context->tcp->send_ack =
			sys_get_be32(NET_TCP_BUF(buf)->seq) + 1;
		context->tcp->recv_max_ack = context->tcp->send_seq + 1;

		buf_get_sockaddr(net_context_get_family(context),
				 buf, &buf_src_addr);
		send_syn_ack(context, &buf_src_addr, remote);

		/* We might be entering this section multiple times
		 * if the SYN is sent more than once. So we need to cancel
		 * any pending timers.
		 */
		k_delayed_work_cancel(&tcp->ack_timer);
		k_delayed_work_init(&tcp->ack_timer, ack_timeout);
		k_delayed_work_submit(&tcp->ack_timer, ACK_TIMEOUT);

		return NET_DROP;
	}

	/*
	 * If we receive RST, we go back to LISTEN state.
	 */
	if (NET_TCP_FLAGS(buf) == NET_TCP_RST) {
		k_delayed_work_cancel(&tcp->ack_timer);

		net_tcp_print_recv_info("RST", buf, NET_TCP_BUF(buf)->src_port);

		net_tcp_change_state(tcp, NET_TCP_LISTEN);

		return NET_DROP;
	}

	/*
	 * If we receive ACK, we go to ESTABLISHED state.
	 */
	if (NET_TCP_FLAGS(buf) == NET_TCP_ACK) {
		struct net_context *new_context;
		struct sockaddr local_addr;
		struct sockaddr remote_addr;
		struct net_tcp *tmp_tcp;
		socklen_t addrlen;
		int ret;

		/* We can only receive ACK if we have already received SYN.
		 * So if we are not in SYN_RCVD state, then it is an error.
		 */
		if (net_tcp_get_state(tcp) != NET_TCP_SYN_RCVD) {
			k_delayed_work_cancel(&tcp->ack_timer);
			NET_DBG("Not in SYN_RCVD state, sending RST");
			goto reset;
		}

		net_tcp_print_recv_info("ACK", buf, NET_TCP_BUF(buf)->src_port);

		if (!context->tcp->accept_cb) {
			NET_DBG("No accept callback, connection reset.");
			goto reset;
		}

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

		new_context->tcp->recv_max_ack = context->tcp->recv_max_ack;
		new_context->tcp->send_seq = context->tcp->send_seq;
		new_context->tcp->send_ack = context->tcp->send_ack;

#if defined(CONFIG_NET_IPV6)
		if (net_context_get_family(context) == AF_INET6) {
			struct sockaddr_in6 *local_addr6 =
				net_sin6(&local_addr);
			struct sockaddr_in6 *remote_addr6 =
				net_sin6(&remote_addr);

			remote_addr6->sin6_family = AF_INET6;
			local_addr6->sin6_family = AF_INET6;

			local_addr6->sin6_port = NET_TCP_BUF(buf)->dst_port;
			remote_addr6->sin6_port = NET_TCP_BUF(buf)->src_port;

			net_ipaddr_copy(&local_addr6->sin6_addr,
					&NET_IPV6_BUF(buf)->dst);
			net_ipaddr_copy(&remote_addr6->sin6_addr,
					&NET_IPV6_BUF(buf)->src);
			addrlen = sizeof(struct sockaddr_in6);
		} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
		if (net_context_get_family(context) == AF_INET) {
			struct sockaddr_in *local_addr4 =
				net_sin(&local_addr);
			struct sockaddr_in *remote_addr4 =
				net_sin(&remote_addr);

			remote_addr4->sin_family = AF_INET;
			local_addr4->sin_family = AF_INET;

			local_addr4->sin_port = NET_TCP_BUF(buf)->dst_port;
			remote_addr4->sin_port = NET_TCP_BUF(buf)->src_port;

			net_ipaddr_copy(&local_addr4->sin_addr,
					&NET_IPV4_BUF(buf)->dst);
			net_ipaddr_copy(&remote_addr4->sin_addr,
					&NET_IPV4_BUF(buf)->src);
			addrlen = sizeof(struct sockaddr_in);
		} else
#endif /* CONFIG_NET_IPV4 */
		{
			NET_ASSERT_INFO(false, "Invalid protocol family %d",
					net_context_get_family(context));
			net_context_put(new_context);
			return NET_DROP;
		}

		ret = net_context_bind(new_context, &local_addr,
				       sizeof(local_addr));
		if (ret < 0) {
			NET_DBG("Cannot bind accepted context, "
				"connection reset");
			net_context_put(new_context);
			goto reset;
		}

		new_context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;

		memcpy(&new_context->remote, &remote_addr,
		       sizeof(remote_addr));

		ret = net_tcp_register(&new_context->remote,
			       &local_addr,
			       ntohs(net_sin(&new_context->remote)->sin_port),
			       ntohs(net_sin(&local_addr)->sin_port),
			       tcp_established,
			       new_context,
			       &new_context->conn_handler);
		if (ret < 0) {
			NET_DBG("Cannot register accepted TCP handler (%d)",
				ret);
			net_context_put(new_context);
			goto reset;
		}

		/* Swap the newly-created TCP states with the one that
		 * was used to establish this connection. The new TCP
		 * must be listening to accept other connections.
		 */
		tmp_tcp = new_context->tcp;
		tmp_tcp->accept_cb = tcp->accept_cb;
		tcp->accept_cb = NULL;
		new_context->tcp = tcp;
		context->tcp = tmp_tcp;

		tcp->context = new_context;
		tmp_tcp->context = context;

		net_tcp_change_state(tmp_tcp, NET_TCP_LISTEN);

		net_tcp_change_state(new_context->tcp, NET_TCP_ESTABLISHED);
		net_context_set_state(new_context, NET_CONTEXT_CONNECTED);

		k_delayed_work_cancel(&tcp->ack_timer);

		context->tcp->accept_cb(new_context,
					&new_context->remote,
					addrlen,
					0,
					context->user_data);
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
		       net_tcp_accept_cb_t cb,
		       int32_t timeout,
		       void *user_data)
{
#if defined(CONFIG_NET_TCP)
	struct sockaddr local_addr;
	struct sockaddr *laddr = NULL;
	uint16_t lport = 0;
	int ret;
#endif /* CONFIG_NET_TCP */

	NET_ASSERT(PART_OF_ARRAY(contexts, context));

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		return net_l2_offload_ip_accept(
			net_context_get_iface(context),
			context,
			cb,
			timeout,
			user_data);
	}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

	if ((net_context_get_state(context) != NET_CONTEXT_LISTENING) &&
	    (net_context_get_type(context) != SOCK_STREAM)) {
		NET_DBG("Invalid socket, state %d type %d",
			net_context_get_state(context),
			net_context_get_type(context));
		return -EINVAL;
	}

#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		NET_ASSERT(context->tcp);

		if (net_tcp_get_state(context->tcp) != NET_TCP_LISTEN) {
			NET_DBG("Context %p in wrong state %d, should be %d",
				context, context->tcp->state, NET_TCP_LISTEN);
			return -EINVAL;
		}
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

	/* accept callback is only valid for TCP contexts */
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		context->tcp->accept_cb = cb;
	}
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
	context->send_cb = cb;
	context->user_data = user_data;
	net_nbuf_set_token(buf, token);

	if (!timeout || net_context_get_ip_proto(context) == IPPROTO_UDP) {
		return net_send_data(buf);
	}

#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		int ret = net_tcp_send_data(context);

		/* Just make the callback synchronously even if it didn't
		 * go over the wire.  In theory it would be nice to track
		 * specific ACK locations in the stream and make the
		 * callback at that time, but there's nowhere to store the
		 * potentially-separate token/user_data values right now.
		 */
		if (cb) {
			cb(context, ret, token, user_data);
		}

		return ret;
	}
#endif

	return -EPROTONOSUPPORT;
}

/* If the reserve has changed, we need to adjust it accordingly in the
 * fragment chain. This can only happen in IEEE 802.15.4 where the link
 * layer header size can change if the destination address changes.
 * Thus we need to check it here. Note that this cannot happen for IPv4
 * as 802.15.4 supports IPv6 only.
 */
static inline struct net_buf *update_ll_reserve(struct net_buf *buf,
						struct in6_addr *addr)
{
	/* We need to go through all the fragments and adjust the
	 * fragment data size.
	 */
	uint16_t reserve, room_len, copy_len, pos;
	struct net_buf *orig_frag, *frag;

	reserve = net_if_get_ll_reserve(net_nbuf_iface(buf), addr);
	if (reserve == net_nbuf_ll_reserve(buf)) {
		return buf;
	}

	NET_DBG("Adjust reserve old %d new %d",
		net_nbuf_ll_reserve(buf), reserve);

	net_nbuf_set_ll_reserve(buf, reserve);

	orig_frag = buf->frags;
	copy_len = orig_frag->len;
	pos = 0;

	buf->frags = NULL;
	room_len = 0;
	frag = NULL;

	while (orig_frag) {
		if (!room_len) {
			frag = net_nbuf_get_reserve_data(reserve);

			net_buf_frag_add(buf, frag);

			room_len = net_buf_tailroom(frag);
		}

		if (room_len >= copy_len) {
			memcpy(net_buf_add(frag, copy_len),
			       orig_frag->data + pos, copy_len);

			room_len -= copy_len;
			copy_len = 0;
		} else {
			memcpy(net_buf_add(frag, room_len),
			       orig_frag->data + pos, room_len);

			copy_len -= room_len;
			pos += room_len;
			room_len = 0;
		}

		if (!copy_len) {
			orig_frag = net_buf_frag_del(NULL, orig_frag);
			if (!orig_frag) {
				break;
			}

			copy_len = orig_frag->len;
			pos = 0;
		}
	}

	return buf;
}

#if defined(CONFIG_NET_UDP)
static int create_udp_packet(struct net_context *context,
			     struct net_buf *buf,
			     const struct sockaddr *dst_addr,
			     struct net_buf **out_buf)
{
#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)dst_addr;

		buf = net_ipv6_create(context, buf, NULL, &addr6->sin6_addr);
		buf = net_udp_append(context, buf, ntohs(addr6->sin6_port));
		buf = net_ipv6_finalize(context, buf);
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)dst_addr;

		buf = net_ipv4_create(context, buf, NULL, &addr4->sin_addr);
		buf = net_udp_append(context, buf, ntohs(addr4->sin_port));
		buf = net_ipv4_finalize(context, buf);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		return -EPROTONOSUPPORT;
	}

	*out_buf = buf;

	return 0;
}
#endif /* CONFIG_NET_UDP */

static int sendto(struct net_buf *buf,
		  const struct sockaddr *dst_addr,
		  socklen_t addrlen,
		  net_context_send_cb_t cb,
		  int32_t timeout,
		  void *token,
		  void *user_data)
{
	struct net_context *context = net_nbuf_context(buf);
	int ret;

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		if (net_context_get_state(context) != NET_CONTEXT_CONNECTED) {
			return -ENOTCONN;
		}

		NET_ASSERT(context->tcp);
		if (context->tcp->flags & NET_TCP_IS_SHUTDOWN) {
			return -ESHUTDOWN;
		}
	}
#endif /* CONFIG_NET_TCP */

	if (!dst_addr) {
		return -EDESTADDRREQ;
	}

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	if (net_if_is_ip_offloaded(net_nbuf_iface(buf))) {
		return net_l2_offload_ip_sendto(
			net_nbuf_iface(buf),
			buf, dst_addr, addrlen,
			cb, timeout, token, user_data);
	}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)dst_addr;

		if (addrlen < sizeof(struct sockaddr_in6)) {
			return -EINVAL;
		}

		if (net_is_ipv6_addr_unspecified(&addr6->sin6_addr)) {
			return -EDESTADDRREQ;
		}

		buf = update_ll_reserve(buf, &addr6->sin6_addr);
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
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		NET_DBG("Invalid protocol family %d", net_nbuf_family(buf));
		return -EINVAL;
	}

#if defined(CONFIG_NET_UDP)
	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		ret = create_udp_packet(context, buf, dst_addr, &buf);
	} else
#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		ret = net_tcp_queue_data(context, buf);
	} else
#endif /* CONFIG_NET_TCP */
	{
		NET_DBG("Unknown protocol while sending packet: %d",
			net_context_get_ip_proto(context));
		return -EPROTONOSUPPORT;
	}

	if (ret < 0) {
		NET_DBG("Could not create network packet to send");
		return ret;
	}

	return send_data(context, buf, cb, timeout, token, user_data);
}

int net_context_send(struct net_buf *buf,
		     net_context_send_cb_t cb,
		     int32_t timeout,
		     void *token,
		     void *user_data)
{
	struct net_context *context = net_nbuf_context(buf);
	socklen_t addrlen;

	NET_ASSERT(PART_OF_ARRAY(contexts, context));

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	if (net_if_is_ip_offloaded(net_nbuf_iface(buf))) {
		return net_l2_offload_ip_send(
			net_nbuf_iface(buf),
			buf, cb, timeout,
			token, user_data);
	}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

	if (!(context->flags & NET_CONTEXT_REMOTE_ADDR_SET) ||
	    !net_sin(&context->remote)->sin_port) {
		return -EDESTADDRREQ;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		addrlen = sizeof(struct sockaddr_in6);
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET) {
		addrlen = sizeof(struct sockaddr_in);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		addrlen = 0;
	}

	return sendto(buf, &context->remote, addrlen, cb, timeout, token,
		      user_data);
}

int net_context_sendto(struct net_buf *buf,
		       const struct sockaddr *dst_addr,
		       socklen_t addrlen,
		       net_context_send_cb_t cb,
		       int32_t timeout,
		       void *token,
		       void *user_data)
{
#if defined(CONFIG_NET_TCP)
	struct net_context *context = net_nbuf_context(buf);

	NET_ASSERT(PART_OF_ARRAY(contexts, context));

	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		/* Match POSIX behavior and ignore dst_address and addrlen */
		return net_context_send(buf, cb, timeout, token, user_data);
	}
#endif /* CONFIG_NET_TCP */

	return sendto(buf, dst_addr, addrlen, cb, timeout, token, user_data);
}

static void set_appdata_values(struct net_buf *buf,
			       enum net_ip_protocol proto,
			       size_t total_len)
{
#if defined(CONFIG_NET_UDP)
	if (proto == IPPROTO_UDP) {
		net_nbuf_set_appdata(buf, net_nbuf_udp_data(buf) +
				     sizeof(struct net_udp_hdr));
		net_nbuf_set_appdatalen(buf, total_len -
					net_nbuf_ip_hdr_len(buf) -
					sizeof(struct net_udp_hdr));
	} else
#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)
	if (proto == IPPROTO_TCP) {
		net_nbuf_set_appdata(buf, net_nbuf_udp_data(buf) +
				     tcp_hdr_len(buf));
		net_nbuf_set_appdatalen(buf, total_len -
					net_nbuf_ip_hdr_len(buf) -
					tcp_hdr_len(buf));
	} else
#endif /* CONFIG_NET_TCP */
	{
		net_nbuf_set_appdata(buf, net_nbuf_ip_data(buf) +
				     net_nbuf_ip_hdr_len(buf));
		net_nbuf_set_appdatalen(buf, total_len -
					net_nbuf_ip_hdr_len(buf));
	}

	NET_ASSERT_INFO(net_nbuf_appdatalen(buf) < total_len,
			"Wrong appdatalen %u, total %zu",
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

		/* TCP packets get appdata earlier in tcp_established() */
		if (net_context_get_ip_proto(context) != IPPROTO_TCP) {
			if (net_nbuf_family(buf) == AF_INET6) {
				set_appdata_values(buf,
						   NET_IPV6_BUF(buf)->nexthdr,
						   total_len);
			} else {
				set_appdata_values(buf,
						   NET_IPV4_BUF(buf)->proto,
						   total_len);
			}
		}

		NET_DBG("Set appdata %p to len %u (total %zu)",
			net_nbuf_appdata(buf), net_nbuf_appdatalen(buf),
			total_len);

		context->recv_cb(context, buf, 0, user_data);

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
		k_sem_give(&context->recv_data_wait);
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

		return NET_OK;
	}

	/* If there is no callback registered, then we can only drop
	 * the packet.
	 */
	return NET_DROP;
}

#if defined(CONFIG_NET_UDP)
static int recv_udp(struct net_context *context,
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

	ARG_UNUSED(timeout);

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

	return ret;
}
#endif /* CONFIG_NET_UDP */

int net_context_recv(struct net_context *context,
		     net_context_recv_cb_t cb,
		     int32_t timeout,
		     void *user_data)
{
	NET_ASSERT(context);

	if (!net_context_is_used(context)) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_L2_OFFLOAD_IP)
	if (net_if_is_ip_offloaded(net_context_get_iface(context))) {
		return net_l2_offload_ip_recv(
			net_context_get_iface(context),
			context, cb, timeout, user_data);
	}
#endif /* CONFIG_NET_L2_OFFLOAD_IP */

#if defined(CONFIG_NET_UDP)
	if (net_context_get_ip_proto(context) == IPPROTO_UDP) {
		int ret = recv_udp(context, cb, timeout, user_data);
		if (ret < 0) {
			return ret;
		}
	} else
#endif /* CONFIG_NET_UDP */

#if defined(CONFIG_NET_TCP)
	if (net_context_get_ip_proto(context) == IPPROTO_TCP) {
		NET_ASSERT(context->tcp);

		if (context->tcp->flags & NET_TCP_IS_SHUTDOWN) {
			return -ESHUTDOWN;
		} else if (net_context_get_state(context)
			   != NET_CONTEXT_CONNECTED) {
			return -ENOTCONN;
		}

		context->recv_cb = cb;
		context->tcp->recv_user_data = user_data;
	} else
#endif /* CONFIG_NET_TCP */
	{
		return -EPROTOTYPE;
	}

#if defined(CONFIG_NET_CONTEXT_SYNC_RECV)
	if (timeout) {
		/* Make sure we have the lock, then the packet_received()
		 * callback will release the semaphore when data has been
		 * received.
		 */
		while (k_sem_take(&context->recv_data_wait, K_NO_WAIT)) {
			;
		}

		if (!k_sem_take(&context->recv_data_wait, timeout)) {
			/* timeout */
			return -ETIMEDOUT;
		}
	}
#endif /* CONFIG_NET_CONTEXT_SYNC_RECV */

	return 0;
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
	k_sem_init(&contexts_lock, 0, UINT_MAX);

	k_sem_give(&contexts_lock);
}
