/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <ipc/ipc_based_driver.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(heartbeat);

static struct ipc_based_driver ipc_data;
static struct k_thread heartbeat_thread_data;

K_THREAD_STACK_DEFINE(heartbeat_thread_stack, CONFIG_TELINK_W91_CORE_HEARTBEAT_THREAD_STACK_SIZE);

enum {
	IPC_DISPATCHER_HEARTBEAT_INIT = IPC_DISPATCHER_HEARTBEAT,
	IPC_DISPATCHER_HEARTBEAT_CHECK,
};

IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(heartbeat_w91_check, IPC_DISPATCHER_HEARTBEAT_CHECK);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(heartbeat_w91_check);

static void heartbeat_w91_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		bool heartbeat_response_status = 0;

		IPC_DISPATCHER_HOST_SEND_DATA(
			&ipc_data, 0, heartbeat_w91_check, NULL, &heartbeat_response_status,
			CONFIG_CORE_HEARTBEAT_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);

		if (!heartbeat_response_status) {
#ifdef CONFIG_LOG
			LOG_ERR("No response from core N22, rebooting...\n");
			k_msleep(100);
#else
			printk("[E] No response from core N22, rebooting...\n");
			k_msleep(100);
#endif
			/* board reboots */
			sys_reboot(0);
		}

		k_msleep(CONFIG_CORE_HEARTBEAT_TELINK_W91_TIMEOUT_MS);
	}
}

IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(heartbeat_w91_start, IPC_DISPATCHER_HEARTBEAT_INIT);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(heartbeat_w91_start);

static int heartbeat_w91_init(void)
{
	int err = 0;

	ipc_based_driver_init(&ipc_data);

	IPC_DISPATCHER_HOST_SEND_DATA(
		&ipc_data, 0, heartbeat_w91_start, NULL, &err,
		CONFIG_CORE_HEARTBEAT_TELINK_W91_IPC_RESPONSE_TIMEOUT_MS);

	if (err) {
		return err;
	}

	k_thread_create(&heartbeat_thread_data, heartbeat_thread_stack,
			K_THREAD_STACK_SIZEOF(heartbeat_thread_stack), heartbeat_w91_thread, NULL,
			NULL, NULL, CONFIG_TELINK_W91_CORE_HEARTBEAT_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(heartbeat_w91_init, POST_KERNEL, CONFIG_TELINK_W91_IPC_PRE_DRIVERS_INIT_PRIORITY);
