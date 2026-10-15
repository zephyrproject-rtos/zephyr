/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_socket_offload, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/net/socket.h>

#include "sockets_internal.h"

static const struct socket_dns_offload *dns_offload;
static bool dns_offload_enabled;

void socket_offload_dns_register(const struct socket_dns_offload *ops)
{
	__ASSERT_NO_MSG(ops);
	__ASSERT_NO_MSG(dns_offload == NULL);

	dns_offload = ops;

	socket_offload_dns_enable(true);
}

int socket_offload_dns_deregister(const struct socket_dns_offload *ops)
{
	__ASSERT_NO_MSG(ops != NULL);

	if (dns_offload != ops) {
		return -EINVAL;
	}

	dns_offload = NULL;

	socket_offload_dns_enable(false);

	return 0;
}

void socket_offload_dns_enable(bool enable)
{
	dns_offload_enabled = enable;
}

bool socket_offload_dns_is_enabled(void)
{
	return (dns_offload != NULL) && dns_offload_enabled;
}

int socket_offload_getaddrinfo(const char *node, const char *service,
			       const struct zsock_addrinfo *hints,
			       struct zsock_addrinfo **res)
{
	__ASSERT_NO_MSG(dns_offload);
	__ASSERT_NO_MSG(dns_offload->getaddrinfo);

	return dns_offload->getaddrinfo(node, service, hints, res);
}

void socket_offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	__ASSERT_NO_MSG(dns_offload);
	__ASSERT_NO_MSG(dns_offload->freeaddrinfo);

	dns_offload->freeaddrinfo(res);
}

#if defined(CONFIG_NET_SOCKETS_OFFLOAD_PORT_TRACKING)
/* Offloaded sockets bind their local port in the offload engine, outside the
 * net_context list, so net_context_port_in_use() cannot see them. Record the
 * local binding of each such bound socket here, keyed by file descriptor, and
 * expose the lookup net_socket_port_in_use() consults.
 */
struct offloaded_port {
	union {
		struct net_in_addr in_addr;
		struct net_in6_addr in6_addr;
	} addr;
	int fd;         /* owning descriptor while family != NET_AF_UNSPEC */
	uint16_t port;  /* host byte order */
	uint16_t proto; /* NET_IPPROTO_TCP or NET_IPPROTO_UDP */
	uint8_t family; /* NET_AF_UNSPEC marks a free slot */
};

static struct offloaded_port offloaded_ports[CONFIG_NET_SOCKETS_OFFLOAD_PORT_TRACKING_COUNT];
static K_MUTEX_DEFINE(offloaded_ports_lock);

/* Resolve the socket's IP protocol through the socket API, so the same path
 * works for native and offloaded stacks. Returns 0 when the stack reports
 * neither SO_PROTOCOL nor a known SO_TYPE, in which case the socket is left
 * untracked.
 */
static uint16_t offloaded_port_proto(int sock)
{
	int optval;

	if (zsock_getsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_PROTOCOL, &optval,
			     &(net_socklen_t){sizeof(optval)}) == 0 &&
	    (optval == NET_IPPROTO_TCP || optval == NET_IPPROTO_UDP)) {
		return (uint16_t)optval;
	}

	if (zsock_getsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_TYPE, &optval,
			     &(net_socklen_t){sizeof(optval)}) == 0) {
		if (optval == NET_SOCK_DGRAM) {
			return NET_IPPROTO_UDP;
		}
		if (optval == NET_SOCK_STREAM) {
			return NET_IPPROTO_TCP;
		}
	}

	return 0U;
}

static struct offloaded_port *offloaded_port_find(int fd)
{
	ARRAY_FOR_EACH(offloaded_ports, i) {
		if (offloaded_ports[i].family != NET_AF_UNSPEC && offloaded_ports[i].fd == fd) {
			return &offloaded_ports[i];
		}
	}

	return NULL;
}

static uint16_t sockaddr_port(const struct net_sockaddr *addr)
{
	if (addr->sa_family == NET_AF_INET) {
		return net_ntohs(net_sin(addr)->sin_port);
	}

	return net_ntohs(net_sin6(addr)->sin6_port);
}

/* Decide whether fd's local binding is already accounted for elsewhere and so
 * must not be tracked in the offloaded-port table:
 *
 *  - A native IP socket is backed by a net_context whose binding
 *    net_context_port_in_use() already reports. It is identified by the single
 *    native socket vtable.
 *  - A native (mbedTLS) TLS socket forwards bind() to a separate transport fd
 *    through the public zsock_bind(), so that transport fd (native or
 *    offloaded) is tracked on its own and the TLS wrapper fd must be skipped.
 */
static bool offloaded_port_skip(int fd)
{
	const struct fd_op_vtable *vtable;
	void *obj = zvfs_get_fd_obj_and_vtable(fd, &vtable, NULL);

#if defined(CONFIG_NET_NATIVE)
	if (vtable == (const struct fd_op_vtable *)&sock_fd_op_vtable) {
		return true;
	}
#endif

	return net_socket_is_tls(obj);
}

void net_socket_offloaded_port_bind(int fd, const struct net_sockaddr *addr)
{
	struct net_sockaddr_storage bound = {0};
	struct offloaded_port *slot;
	uint16_t proto;
	uint16_t port;

	if (fd < 0 || addr == NULL) {
		return;
	}

	if (addr->sa_family != NET_AF_INET && addr->sa_family != NET_AF_INET6) {
		return;
	}

	if (offloaded_port_skip(fd)) {
		return;
	}

	proto = offloaded_port_proto(fd);
	if (proto == 0U) {
		return;
	}

	port = sockaddr_port(addr);

	/* Record the port the offload engine actually bound if ephemeral (0) */
	if (port == 0U &&
	    zsock_getsockname(fd, (struct net_sockaddr *)&bound, &(net_socklen_t){sizeof(bound)}) ==
		    0 &&
	    bound.ss_family == addr->sa_family) {
		addr = (const struct net_sockaddr *)&bound;
		port = sockaddr_port(addr);
	}

	/* An unresolved ephemeral port (0) matches no later query. Don't track it. */
	if (port == 0U) {
		return;
	}

	k_mutex_lock(&offloaded_ports_lock, K_FOREVER);

	/* A rebind reuses the socket's existing slot. */
	slot = offloaded_port_find(fd);
	if (slot == NULL) {
		ARRAY_FOR_EACH(offloaded_ports, i) {
			if (offloaded_ports[i].family == NET_AF_UNSPEC) {
				slot = &offloaded_ports[i];
				break;
			}
		}
	}

	if (slot == NULL) {
		k_mutex_unlock(&offloaded_ports_lock);
		NET_WARN("No free offloaded port slot for fd %d, port-in-use tracking "
			 "incomplete (raise CONFIG_NET_SOCKETS_OFFLOAD_PORT_TRACKING_COUNT)",
			 fd);
		return;
	}

	slot->fd = fd;
	slot->proto = proto;
	slot->port = port;

	if (addr->sa_family == NET_AF_INET) {
		const struct net_sockaddr_in *in = (const struct net_sockaddr_in *)addr;

		slot->addr.in_addr = in->sin_addr;
		slot->family = NET_AF_INET;
	} else {
		const struct net_sockaddr_in6 *in6 = (const struct net_sockaddr_in6 *)addr;

		slot->addr.in6_addr = in6->sin6_addr;
		slot->family = NET_AF_INET6;
	}

	k_mutex_unlock(&offloaded_ports_lock);
}

void net_socket_offloaded_port_untrack(int fd)
{
	struct offloaded_port *slot;

	if (fd < 0) {
		return;
	}

	k_mutex_lock(&offloaded_ports_lock, K_FOREVER);

	slot = offloaded_port_find(fd);
	if (slot != NULL) {
		slot->proto = 0U;
		slot->family = NET_AF_UNSPEC;
		slot->port = 0U;
	}

	k_mutex_unlock(&offloaded_ports_lock);
}

bool net_socket_offloaded_port_in_use(enum net_ip_protocol proto, uint16_t local_port,
				      const struct net_sockaddr *local_addr)
{
	bool in_use = false;

	if (local_addr == NULL) {
		return false;
	}

	k_mutex_lock(&offloaded_ports_lock, K_FOREVER);

	ARRAY_FOR_EACH(offloaded_ports, i) {
		const struct offloaded_port *op = &offloaded_ports[i];

		if (op->family == NET_AF_UNSPEC || op->port != local_port ||
		    op->proto != (uint16_t)proto || op->family != local_addr->sa_family) {
			continue;
		}

		if (op->family == NET_AF_INET) {
			const struct net_sockaddr_in *in =
				(const struct net_sockaddr_in *)local_addr;

			if (net_ipv4_is_addr_unspecified(&op->addr.in_addr) ||
			    net_ipv4_is_addr_unspecified(&in->sin_addr) ||
			    net_ipv4_addr_cmp(&op->addr.in_addr, &in->sin_addr)) {
				in_use = true;
				break;
			}
		} else {
			const struct net_sockaddr_in6 *in6 =
				(const struct net_sockaddr_in6 *)local_addr;

			if (net_ipv6_is_addr_unspecified(&op->addr.in6_addr) ||
			    net_ipv6_is_addr_unspecified(&in6->sin6_addr) ||
			    net_ipv6_addr_cmp(&op->addr.in6_addr, &in6->sin6_addr)) {
				in_use = true;
				break;
			}
		}
	}

	k_mutex_unlock(&offloaded_ports_lock);

	return in_use;
}
#endif /* CONFIG_NET_SOCKETS_OFFLOAD_PORT_TRACKING */
