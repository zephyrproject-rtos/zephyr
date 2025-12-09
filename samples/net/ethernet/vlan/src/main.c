/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_vlan_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>

#include "net_sample_common.h"

int main(void)
{
	init_vlan();
	return 0;
}
