/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_engine_socket.h"

static struct net_layer_socket nl_socket_data;

static struct lwm2m_net_layer_api nl_socket_api = {
	.nl_start     = lwm2m_nl_socket_start,
	.nl_msg_send  = lwm2m_nl_socket_msg_send,
	.nl_user_data = &nl_socket_data,
};

struct lwm2m_net_layer_api *lwm2m_firmware_pull_nl_socket_api(void)
{
	return &nl_socket_api;
}
