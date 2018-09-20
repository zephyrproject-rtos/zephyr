/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_ENGINE_NET_APP_H
#define LWM2M_ENGINE_NET_APP_H

#include <net/net_app.h>
#include <net/net_pkt.h>

#include <net/lwm2m.h>

struct net_layer_net_app {
	struct lwm2m_ctx *ctx;
	struct net_app_ctx net_app_ctx;
};

void lwm2m_engine_udp_receive(struct net_app_ctx *app_ctx, struct net_pkt *pkt,
			      int status, void *user_data);

int  lwm2m_nl_net_app_start(struct lwm2m_ctx *client_ctx,
			    char *peer_str, u16_t peer_port);
int  lwm2m_nl_net_app_msg_send(struct lwm2m_message *msg);

#endif /* LWM2M_ENGINE_NET_APP_H */
