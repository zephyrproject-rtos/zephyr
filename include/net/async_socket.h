/**
 * @file
 * @brief Asynchronous sockets API definitions
 *
 * An API for adapting synchronous BSD socket APIs to applications
 * requiring asynchronous callbacks.
 *
 * Created to ease adaptation of asynchronous IP protocols from
 * net_app/net_context to sockets.
 */

/*
 * Copyright (c) 2018, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_ASYNC_SOCKET_H
#define __NET_ASYNC_SOCKET_H

#include <net/socket.h>
#include <net/zstream.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Callbacks, similar in semantics to those of net_context.h */
typedef void (*async_connect_cb_t)(int sock,
				   int status,
				   void *cb_data);

typedef void (*async_send_cb_t)(int sock,
				int bytes_sent,
				void *cb_data);

typedef void (*async_recv_cb_t)(int sock,
				void *data,
				size_t bytes_received,
				void *cb_data);

/*
 * Errors are the same as the corresponding POSIX socket functions: i.e.,
 * a return value of -1 implicitly sets errno.
 */

/* For now, same semantics as socket() call: */
static inline int async_socket(int family, int type, int proto)
{
	return socket(family, type, proto);
}

int async_close(int sock, struct zstream *stream);

int async_bind(int sock, const struct sockaddr *addr, socklen_t addrlen);

int async_connect(int sock, const struct sockaddr *addr, socklen_t addrlen,
		  async_connect_cb_t cb, void *cb_data);

ssize_t async_send(struct zstream *sock, const void *buf, size_t len,
		   async_send_cb_t cb, void *cb_data, int flags);

/* buf must be unique per sock */
ssize_t async_recv(int sock, struct zstream *stream, void *buf, size_t max_len,
		   async_recv_cb_t cb, void *cb_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __NET_ASYNC_SOCKET_H */
