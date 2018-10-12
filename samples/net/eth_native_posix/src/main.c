/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_native_posix
#define NET_LOG_LEVEL LOG_LEVEL_DBG

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>

/* This application itself does nothing as there is net-shell that can be used
 * to monitor things.
 */
void main(void)
{
	NET_INFO("Start application");
}
