/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app);

#define STACKSIZE	4096
#define PRIORITY	K_PRIO_PREEMPT(2)

K_THREAD_STACK_DEFINE(ipc1_stack, STACKSIZE);
static volatile uint8_t ipc1_received_data;
static K_SEM_DEFINE(ipc1_bound_sem, 0, 1);
static K_SEM_DEFINE(ipc1_data_sem, 0, 1);

/*
 * ==> THREAD 1 (IPC instance 1) <==
 */

static void ipc1_ept_bound(void *priv)
{
	k_sem_give(&ipc1_bound_sem);
}

static void ipc1_ept_recv(const void *data, size_t len, void *priv)
{
	ipc1_received_data = *((uint8_t *) data);

	k_sem_give(&ipc1_data_sem);
}

static struct ipc_ept_cfg ipc1_ept_cfg = {
	.name = "ipc1",
	.cb = {
		.bound    = ipc1_ept_bound,
		.received = ipc1_ept_recv,
	},
};

static void ipc1_entry(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	const struct device *ipc1_instance;
	unsigned char message = 0;
	struct ipc_ept ipc1_ept;
	int ret;

	LOG_INF("IPC-service HOST [INST 1] demo started");

	k_sleep(K_MSEC(1000));

	ipc1_instance = DEVICE_DT_GET(DT_NODELABEL(ipc1));

	ret = ipc_service_open_instance(ipc1_instance);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("ipc_service_open_instance() failure");
		return;
	}

	LOG_INF("ipc open %d", ret);
	ret = ipc_service_register_endpoint(ipc1_instance, &ipc1_ept, &ipc1_ept_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return;
	}

	k_sem_take(&ipc1_bound_sem, K_FOREVER);

	while (message < 100) {
		ret = ipc_service_send(&ipc1_ept, &message, sizeof(message));
		if (ret < 0) {
			LOG_ERR("send_message(%d) failed with ret %d", message, ret);
			break;
		}

		k_sem_take(&ipc1_data_sem, K_FOREVER);
		message = ipc1_received_data;

		LOG_INF("HOST [1]: %d", message);
		message++;
	}

	LOG_INF("IPC-service HOST [INST 1] demo ended.");
}

K_THREAD_DEFINE(ipc1_thread_id, STACKSIZE, ipc1_entry, NULL, NULL, NULL, PRIORITY, 0, 0);
