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
int zsock_poll_internal(struct zsock_pollfd *fds, int nfds, k_timeout_t timeout);

int zsock_wait_data(struct net_context *ctx, k_timeout_t *timeout);

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

#define sock_is_eof(ctx) sock_get_flag(ctx, SOCK_EOF)
#define sock_set_eof(ctx) sock_set_flag(ctx, SOCK_EOF, SOCK_EOF)
#define sock_is_nonblock(ctx) sock_get_flag(ctx, SOCK_NONBLOCK)
#define sock_is_error(ctx) sock_get_flag(ctx, SOCK_ERROR)
#define sock_set_error(ctx) sock_set_flag(ctx, SOCK_ERROR, SOCK_ERROR)

size_t msghdr_non_empty_iov_count(const struct msghdr *msg);

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
