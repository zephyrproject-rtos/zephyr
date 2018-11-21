/*
 * Copyright (c) 2018 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKS_H_
#define ZEPHYR_INCLUDE_NET_SOCKS_H_

#include <net/net_app.h>

enum socks_connection_state {
	SOCKS5_STATE_IDLE,
	SOCKS5_STATE_NEGOTIATING,
	SOCKS5_STATE_NEGOTIATED,
	SOCKS5_STATE_CONNECTED,
	SOCKS5_STATE_ERROR,
};

struct socks_client {
	struct net_app_ctx app_ctx;
	struct net_app_ctx *socks_app;

	struct sockaddr_in incoming;
	struct sockaddr_in outgoing;

	struct net_context *in_ctx;

	enum socks_connection_state state;
};

struct socks_ctx {
	struct net_app_ctx app_ctx;
	struct sockaddr local;

	struct socks_client clients[3] ; //CONFIG_NET_APP_SERVER_NUM_CONN];

	/** Is this context setup or not */
	u8_t is_init : 1;
};

int socks_init(struct socks_ctx *ctx, u16_t port);

/* XXX */
#define SOCKS5_STACK_SIZE	2048
#define SOCKS5_MAX_CONNECTIONS	1

#define SOCKS5_BSIZE		2048

struct socks5_connection {
	int fd;
	//int port;
	int len;

	struct sockaddr_in addr;
	struct k_thread thread;
	K_THREAD_STACK_MEMBER(stack, SOCKS5_STACK_SIZE);
};

struct socks5_proxy {
	int id;
	struct socks5_connection client;
	struct socks5_connection destination;

	enum socks_connection_state state;

	struct k_sem exit_sem;
	struct k_sem *busy_sem;

	char buffer_rcv[SOCKS5_BSIZE];
	char buffer_snd[SOCKS5_BSIZE];
	char buffer_dst[SOCKS5_BSIZE];
};

struct socks5_ctx {
	int fd;

	struct sockaddr_in addr;
	struct k_sem busy_sem;

	struct socks5_proxy proxy[SOCKS5_MAX_CONNECTIONS];
};
/* XXX */

#endif /* ZEPHYR_INCLUDE_NET_SOCKS_H_ */
