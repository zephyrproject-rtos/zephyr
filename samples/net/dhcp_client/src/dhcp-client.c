/* dhcp-client.c - Get IPv4 address */

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

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <misc/sys_log.h>

#include <zephyr.h>
#include <sections.h>

#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_socket.h>
#include "contiki/ip/dhcpc.h"


static void dhcpc_configured_cb(void)
{
	SYS_LOG_INF("%s", __func__);
	SYS_LOG_INF("Got IP address %d.%d.%d.%d",
		    uip_ipaddr_to_quad(&uip_hostaddr));
}

static void dhcpc_unconfigured_cb(void)
{
	SYS_LOG_INF("%s", __func__);
}

void main(void)
{
	SYS_LOG_INF("run dhcp client");
	dhcpc_set_callbacks(dhcpc_configured_cb, dhcpc_unconfigured_cb);
	net_init();
}
