/* Networking DHCPv4 client */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if 1
#define SYS_LOG_DOMAIN "dhcpv4"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_DEBUG 1
#endif

#include <zephyr.h>
#include <sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>

static struct nano_sem quit_lock;

static inline void quit(void)
{
	nano_sem_give(&quit_lock);
}

static inline void init_app(void)
{
	struct net_if *iface;

	NET_INFO("Run dhcpv4 client");

	iface = net_if_get_default();

	net_dhcpv4_start(iface);

	nano_sem_init(&quit_lock);
}

void main_fiber(void)
{
	nano_sem_take(&quit_lock, TICKS_UNLIMITED);
}

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char __noinit __stack fiberStack[STACKSIZE];
#endif

void main(void)
{
	NET_INFO("In main");

	init_app();

#if defined(CONFIG_MICROKERNEL)
	main_fiber();
#else
	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)main_fiber, 0, 0, 7, 0);
#endif
}
