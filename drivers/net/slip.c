/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * SLIP driver carrying IP packets on top of a UART device. This is meant for
 * network connectivity between host and qemu. The host will need to run
 * tunslip process.
 */

#define DT_DRV_COMPAT zephyr_slip

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(slip, CONFIG_SLIP_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_if.h>

#include "slip.h"

#define SLIP_MTU 576

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Exactly one enabled zephyr,slip node is needed");
BUILD_ASSERT(!(IS_ENABLED(CONFIG_ETH_SLIP_TAP) &&
	       DT_NODE_HAS_COMPAT(DT_DRV_INST(0), zephyr_slip_eth)),
	     "The SLIP node is also used by ETH_SLIP_TAP, disable one of the drivers");

static void slip_ip_iface_init(struct net_if *iface)
{
	int err;

	slip_iface_init(iface);

	err = net_if_set_name(iface, CONFIG_SLIP_DRV_NAME);
	if (err < 0) {
		LOG_ERR("Could not set the interface name: %d", err);
	}
}

static const struct dummy_api slip_if_api = {
	.iface_api.init = slip_ip_iface_init,

	.send = slip_send,
};

static struct slip_context slip_context_data;

static const struct slip_config slip_config = {
	.uart = DEVICE_DT_GET(DT_INST_BUS(0)),
};

NET_DEVICE_DT_INST_DEFINE(0, NULL, NULL, &slip_context_data, &slip_config,
			  CONFIG_SLIP_INIT_PRIORITY, &slip_if_api, DUMMY_L2,
			  NET_L2_GET_CTX_TYPE(DUMMY_L2), SLIP_MTU);
