/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <ipc/ipc_based_driver.h>

#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(heartbeat, CONFIG_SOC_LOG_LEVEL);

#if CONFIG_CORE_HEARTBEAT_TELINK_W91_TIMEOUT_MS <= CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS
#error CORE_HEARTBEAT_TELINK_W91_TIMEOUT_MS should be greater than \
TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS
#endif

static struct telink_w91_heartbeat_data {
	struct ipc_based_driver     ipc;
	struct k_work_delayable     work;
} telink_w91_heartbeat;

enum {
	IPC_DISPATCHER_HEARTBEAT_CHECK = IPC_DISPATCHER_HEARTBEAT,
};

static void telink_w91_heartbeat_worker(struct k_work *item)
{
	struct telink_w91_heartbeat_data *heartbeat =
		CONTAINER_OF(k_work_delayable_from_work(item), struct telink_w91_heartbeat_data, work);
	const uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_HEARTBEAT_CHECK, 0);

	if (ipc_based_driver_send(&heartbeat->ipc, &id, sizeof(id), NULL,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS)) {
		LOG_ERR("N22 core response failed");
		abort();
	}

	(void) k_work_schedule(&heartbeat->work,
		K_MSEC(CONFIG_CORE_HEARTBEAT_TELINK_W91_TIMEOUT_MS));
}

static int telink_w91_heartbeat_init(void)
{
	ipc_based_driver_init(&telink_w91_heartbeat.ipc);
	k_work_init_delayable(&telink_w91_heartbeat.work, telink_w91_heartbeat_worker);
	(void) k_work_schedule(&telink_w91_heartbeat.work,
		K_MSEC(CONFIG_CORE_HEARTBEAT_TELINK_W91_TIMEOUT_MS));

	return 0;
}

SYS_INIT(telink_w91_heartbeat_init, POST_KERNEL, CONFIG_TELINK_W91_IPC_PRE_DRIVERS_INIT_PRIORITY);
