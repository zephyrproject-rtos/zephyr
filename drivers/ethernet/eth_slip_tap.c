/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME eth_slip_tap
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/net/ethernet.h>
#include "../net/slip.h"

static struct slip_context slip_context_data;

static enum ethernet_hw_caps eth_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_HW_VLAN
#if defined(CONFIG_NET_LLDP)
		| ETHERNET_LLDP
#endif
		;
}

static const struct ethernet_api slip_if_api = {
	.iface_api.init = slip_iface_init,

	.get_capabilities = eth_capabilities,
	.send = slip_send,
};

#define _SLIP_L2_LAYER ETHERNET_L2
#define _SLIP_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)

ETH_NET_DEVICE_INIT(slip, CONFIG_SLIP_DRV_NAME,
		    slip_init, NULL,
		    &slip_context_data, NULL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &slip_if_api, _SLIP_MTU);
