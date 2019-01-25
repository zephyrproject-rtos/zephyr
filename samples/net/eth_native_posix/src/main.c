/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_native_posix_sample, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>

/* This application itself does nothing as there is net-shell that can be used
 * to monitor things.
 */
void main(void)
{
	LOG_INF("Start application");
}
