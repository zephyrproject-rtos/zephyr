/*
 * Copyright (c) 2022 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include <errno.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/net/mqtt_sn.h>
#include <zephyr/net/net_conn_mgr.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt_sn_publisher_sample, LOG_LEVEL_INF);

#if defined(CONFIG_USERSPACE)
#include <zephyr/app_memory/app_memdomain.h>
K_APPMEM_PARTITION_DEFINE(app_partition);
struct k_mem_domain app_domain;
#endif

#define APP_BANNER "MQTT-SN sample"

K_SEM_DEFINE(run_app, 0, 1);

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)

static void net_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
			      struct net_if *iface)
{
	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");

		k_sem_give(&run_app);

		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network disconnected");

		k_sem_reset(&run_app);

		return;
	}
}

static void init_app(void)
{
	static struct net_mgmt_event_callback mgmt_cb;

	LOG_INF(APP_BANNER);

#if defined(CONFIG_USERSPACE)
	int err;
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&app_partition
	};

	err = k_mem_domain_init(&app_domain, ARRAY_SIZE(parts), parts);
	__ASSERT(err == 0, "k_mem_domain_init() failed %d", err);
	ARG_UNUSED(err);

#endif

	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, EVENT_MASK);
		net_mgmt_add_event_callback(&mgmt_cb);

		net_conn_mgr_resend_status();
	}
}

static int start_client(void)
{
	/* Wait for connection */
	k_sem_take(&run_app, K_FOREVER);

	return start_thread();
}

void main(void)
{
	init_app();

	if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		/* If the config library has not been configured to start the
		 * app only after we have a connection, then we can start
		 * it right away.
		 */
		k_sem_give(&run_app);
	}

#if defined(CONFIG_USERSPACE)
	k_thread_access_grant(k_current_get(), &run_app);
	k_mem_domain_add_thread(&app_domain, k_current_get());

	k_thread_user_mode_enter((k_thread_entry_t)start_client, NULL, NULL, NULL);
#else
	exit(start_client());
#endif
}
