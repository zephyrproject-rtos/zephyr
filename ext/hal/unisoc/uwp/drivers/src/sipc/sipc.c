/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal_log.h>
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <uwp_hal.h>
#include "sipc.h"
#include "sipc_priv.h"

extern int smsg_init(u32_t dst, u32_t smsg_base);
static int sipc_init(struct device *arg)
{
	int ret;

	LOG_INF("sipc init start.");

	ret = smsg_init(IPC_DST, IPC_RING_ADDR);
	if (ret) {
		LOG_ERR("sipc init failed.");
		return ret;
	}

	LOG_INF("sipc init success.");

	return 0;
}

SYS_INIT(sipc_init, POST_KERNEL, 1);
