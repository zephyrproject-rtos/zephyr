/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_engine_net_app.h"

static struct net_layer_net_app nl_net_app_data;

static struct lwm2m_net_layer_api nl_net_app_api = {
	.nl_start     = lwm2m_nl_net_app_start,
	.nl_msg_send  = lwm2m_nl_net_app_msg_send,
	.nl_user_data = &nl_net_app_data,
};

struct lwm2m_net_layer_api *lwm2m_firmware_pull_nl_net_app_api(void)
{
	return &nl_net_app_api;
}
