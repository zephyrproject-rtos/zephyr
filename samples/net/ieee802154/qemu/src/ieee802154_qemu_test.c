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

#include <zephyr.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_core.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

struct in6_addr addr6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0,
			      0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct net_if *init_device(void)
{
	struct net_if *iface;
	struct device *dev;

	dev = device_get_binding(CONFIG_UPIPE_15_4_DRV_NAME);
	if (!dev) {
		PRINT("Cannot get UPIPE device\n");
		return NULL;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		PRINT("Cannot get UPIPE network interface\n");
		return NULL;
	}

	net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0);

	PRINT("802.15.4 device up and running\n");

	return iface;
}


void main(void)
{
	struct net_if *iface;

	iface = init_device();

	return;
}
