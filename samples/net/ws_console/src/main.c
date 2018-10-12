/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_ws_console_main
#define NET_LOG_LEVEL LOG_LEVEL_DBG

#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>

#include "common.h"

static struct k_sem quit_lock;

void quit(void)
{
	k_sem_give(&quit_lock);
}

static inline int init_app(void)
{
	k_sem_init(&quit_lock, 0, UINT_MAX);

	return 0;
}

void main(void)
{
	init_app();

	start_ws_console();

	k_sem_take(&quit_lock, K_FOREVER);

	stop_ws_console();
}
