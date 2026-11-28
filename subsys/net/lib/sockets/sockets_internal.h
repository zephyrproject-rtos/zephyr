/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOCKETS_INTERNAL_H_
#define _SOCKETS_INTERNAL_H_

#include <zephyr/sys/fdtable.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/socket.h>

#define SOCK_EOF 1
#define SOCK_NONBLOCK 2
#define SOCK_ERROR 4

int zsock_close_ctx(struct net_context *ctx, int sock);

#if defined(CONFIG_TAILSCALE_DNS_RESOLVER) && defined(CONFIG_NET_SOCKETS_OFFLOAD)
int zsock_offload_getaddrinfo_copy(const char *host, const char *service,
				  const struct zsock_addrinfo *hints,
				  struct zsock_addrinfo **res);
#endif

/* Synchronous notification before the native socket context is torn down. */
void zsock_native_pre_close(struct net_context *ctx);

int zsock_poll_internal(struct zsock_pollfd *fds, int nfds, k_timeout_t timeout);

int zsock_wait_data(struct net_context *ctx, k_timeout_t *timeout);

#if defined(CONFIG_NET_NATIVE)
/* Native IP sockets all share this vtable; used to tell them apart from
 * offloaded sockets by the vtable installed on an fd.
 */
extern const struct socket_op_vtable sock_fd_op_vtable;
#endif

static inline void sock_set_flag(struct net_context *ctx, uintptr_t mask,
				 uintptr_t flag)
{
	uintptr_t val = POINTER_TO_UINT(ctx->socket_data);

	val = (val & ~mask) | flag;
	(ctx)->socket_data = UINT_TO_POINTER(val);
}

static inline uintptr_t sock_get_flag(struct net_context *ctx, uintptr_t mask)
{
	return POINTER_TO_UINT(ctx->socket_data) & mask;
}

void net_socket_update_tc_rx_time(struct net_pkt *pkt, uint32_t end_tick);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
bool net_socket_is_tls(void *obj);
#else
static inline bool net_socket_is_tls(void *obj)
{
	ARG_UNUSED(obj);

	return false;
}
#endif

/* Offloaded sockets bind their local port in the offload engine, outside the
 * net_context list. socket_offload.c tracks those bindings; the socket layer
 * records one here after every successful bind() so net_socket_port_in_use()
 * covers native and offloaded sockets alike.
 */
#if defined(CONFIG_NET_SOCKETS_OFFLOAD_PORT_TRACKING)
void net_socket_offloaded_port_bind(int fd, const struct net_sockaddr *addr);
void net_socket_offloaded_port_untrack(int fd);
#else
static inline void net_socket_offloaded_port_bind(int fd, const struct net_sockaddr *addr)
{
	ARG_UNUSED(fd);
	ARG_UNUSED(addr);
}

static inline void net_socket_offloaded_port_untrack(int fd)
{
	ARG_UNUSED(fd);
}
#endif /* CONFIG_NET_SOCKETS_OFFLOAD_PORT_TRACKING */

static inline bool sock_is_eof(struct net_context *ctx)
{
	return sock_get_flag(ctx, SOCK_EOF) != 0U;
}

#define sock_set_eof(ctx) sock_set_flag(ctx, SOCK_EOF, SOCK_EOF)

static inline bool sock_is_nonblock(struct net_context *ctx)
{
	return sock_get_flag(ctx, SOCK_NONBLOCK) != 0U;
}

static inline bool sock_is_error(struct net_context *ctx)
{
	return sock_get_flag(ctx, SOCK_ERROR) != 0U;
}

/* Record a pending socket error (positive errno) and flag the context.
 * The value is stored in a dedicated net_context field rather than being
 * stashed in user_data, which the stack also uses to carry the parent
 * context pointer for accept callbacks.
 */
static inline void sock_set_error(struct net_context *ctx, int err)
{
	ctx->sock_error = err;
	sock_set_flag(ctx, SOCK_ERROR, SOCK_ERROR);
}

/* Retrieve the pending socket error (positive errno) previously recorded
 * by sock_set_error(). Only meaningful while sock_is_error() is true.
 */
static inline int sock_get_error(struct net_context *ctx)
{
	return ctx->sock_error;
}

size_t msghdr_non_empty_iov_count(const struct net_msghdr *msg);

#if defined(CONFIG_NET_SOCKETS_OBJ_CORE)
int sock_obj_core_alloc(int sock, struct net_socket_register *reg,
			int family, int type, int proto);
int sock_obj_core_alloc_find(int sock, int new_sock, int type);
int sock_obj_core_dealloc(int sock);
void sock_obj_core_update_send_stats(int sock, int bytes);
void sock_obj_core_update_recv_stats(int sock, int bytes);
#else
static inline int sock_obj_core_alloc(int sock,
				      struct net_socket_register *reg,
				      int family, int type, int proto)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(reg);
	ARG_UNUSED(family);
	ARG_UNUSED(type);
	ARG_UNUSED(proto);

	return -ENOTSUP;
}

static inline int sock_obj_core_alloc_find(int sock, int new_sock, int type)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(new_sock);
	ARG_UNUSED(type);

	return -ENOTSUP;
}

static inline int sock_obj_core_dealloc(int sock)
{
	ARG_UNUSED(sock);

	return -ENOTSUP;
}

static inline void sock_obj_core_update_send_stats(int sock, int bytes)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(bytes);
}

static inline void sock_obj_core_update_recv_stats(int sock, int bytes)
{
	ARG_UNUSED(sock);
	ARG_UNUSED(bytes);
}
#endif /* CONFIG_NET_SOCKETS_OBJ_CORE */

#endif /* _SOCKETS_INTERNAL_H_ */
