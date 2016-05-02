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

#include <init.h>
#include <net/net_if.h>

/* net_if dedicated section limiters */
extern struct net_if __net_if_start[];
extern struct net_if __net_if_end[];

static int net_if_init(struct device *unused)
{
	struct net_if_api *api;
	struct net_if *iface;

	ARG_UNUSED(unused);

	for (iface = __net_if_start; iface != __net_if_end; iface++) {
		api = (struct net_if_api *) iface->dev->driver_api;

		if (api && api->init) {
			api->init(iface);
		}
	}

	return 0;
}

SYS_INIT(net_if_init, NANOKERNEL, CONFIG_NET_YAIP_INIT_PRIO);
