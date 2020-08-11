/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOCKETS_INTERNAL_H_
#define _SOCKETS_INTERNAL_H_

#include <sys/fdtable.h>

#define SOCK_EOF 1
#define SOCK_NONBLOCK 2

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

#define sock_is_eof(ctx) sock_get_flag(ctx, SOCK_EOF)
#define sock_set_eof(ctx) sock_set_flag(ctx, SOCK_EOF, SOCK_EOF)
#define sock_is_nonblock(ctx) sock_get_flag(ctx, SOCK_NONBLOCK)

struct socket_op_vtable {
	struct fd_op_vtable fd_vtable;
	int (*bind)(void *obj, const struct sockaddr *addr, socklen_t addrlen);
	int (*connect)(void *obj, const struct sockaddr *addr,
		       socklen_t addrlen);
	int (*listen)(void *obj, int backlog);
	int (*accept)(void *obj, struct sockaddr *addr, socklen_t *addrlen);
	ssize_t (*sendto)(void *obj, const void *buf, size_t len, int flags,
			  const struct sockaddr *dest_addr, socklen_t addrlen);
	ssize_t (*recvfrom)(void *obj, void *buf, size_t max_len, int flags,
			    struct sockaddr *src_addr, socklen_t *addrlen);
	int (*getsockopt)(void *obj, int level, int optname,
			  void *optval, socklen_t *optlen);
	int (*setsockopt)(void *obj, int level, int optname,
			  const void *optval, socklen_t optlen);
	ssize_t (*sendmsg)(void *obj, const struct msghdr *msg, int flags);
	int (*getsockname)(void *obj, struct sockaddr *addr,
			   socklen_t *addrlen);
};

#endif /* _SOCKETS_INTERNAL_H_ */
