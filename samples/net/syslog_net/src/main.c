/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "sys-log-net"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>

#include <shell/shell.h>
#include <net/net_core.h>
#include <net/net_pkt.h>

/* Try to make sure that we have the peer MAC address resolved properly.
 * This is only needed in this sample application.
 */
static void init_app(void)
{
	struct sockaddr server_addr;
	char cmd[64];
	int ret;

	ret = net_ipaddr_parse(CONFIG_SYS_LOG_BACKEND_NET_SERVER,
			       sizeof(CONFIG_SYS_LOG_BACKEND_NET_SERVER) - 1,
			       &server_addr);
	if (!ret) {
		printk("Invalid syslog server address (%s)\n",
		       CONFIG_SYS_LOG_BACKEND_NET_SERVER);
		return;
	}

#if defined(CONFIG_NET_IPV6)
	if (server_addr.sa_family == AF_INET6) {
		snprintk(cmd, sizeof(cmd), "net ping %s",
			 CONFIG_NET_CONFIG_PEER_IPV6_ADDR);
		shell_exec(cmd);
		return;
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (server_addr.sa_family == AF_INET) {
		snprintk(cmd, sizeof(cmd), "net ping %s",
			 CONFIG_NET_CONFIG_PEER_IPV4_ADDR);
		shell_exec(cmd);
		return;
	}
#endif
}

void main(void)
{
	init_app();

	NET_DBG("Starting");
	NET_ERR("Error message");
	NET_WARN("Warning message");
	NET_INFO("Info message");
	NET_DBG("Debug message");
	NET_DBG("Stopping");
}
