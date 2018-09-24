/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_ENGINE_SOCKET_H
#define LWM2M_ENGINE_SOCKET_H

#include <net/lwm2m.h>

struct net_layer_socket {
	struct lwm2m_ctx *ctx;
	int sock_fd;
	struct k_delayed_work receive_work;
};

int  lwm2m_nl_socket_start(struct lwm2m_ctx *client_ctx,
			    char *peer_str, u16_t peer_port);
int  lwm2m_nl_socket_msg_send(struct lwm2m_message *msg);

#endif /* LWM2M_ENGINE_SOCKET_H */
