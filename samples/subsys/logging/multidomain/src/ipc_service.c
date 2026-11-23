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

K_THREAD_STACK_DEFINE(sample_stack, STACKSIZE);
static volatile uint8_t sample_received_data;
static K_SEM_DEFINE(bound_sem, 0, 1);
static K_SEM_DEFINE(data_sem, 0, 1);

/*
 * ==> THREAD 1 (IPC instance 1) <==
 */

static void ept_bound(void *priv)
{
	k_sem_give(&bound_sem);
}

static void ept_recv(const void *data, size_t len, void *priv)
{
	sample_received_data = *((uint8_t *) data);

	k_sem_give(&data_sem);
}

static struct ipc_ept_cfg ept_cfg = {
	.name = "sample",
	.cb = {
		.bound    = ept_bound,
		.received = ept_recv,
	},
};

static void sample_entry(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	const struct device *ipc_instance;
	unsigned char message = 0;
	struct ipc_ept ept;
	int ret;

	LOG_INF("Multidomain logging HOST demo started");

	k_sleep(K_MSEC(1000));

	ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("ipc_service_open_instance() failure");
		return;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &ept, &ept_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return;
	}

	k_sem_take(&bound_sem, K_FOREVER);

	while (message < 100) {
		ret = ipc_service_send(&ept, &message, sizeof(message));
		if (ret < 0) {
			LOG_ERR("send_message(%d) failed with ret %d", message, ret);
			break;
		}

		k_sem_take(&data_sem, K_FOREVER);
		message = sample_received_data;

		LOG_INF("HOST [1]: %d", message);
		message++;
	}

	LOG_INF("Multidomain logging HOST demo ended.");
}

K_THREAD_DEFINE(sample_thread_id, STACKSIZE, sample_entry, NULL, NULL, NULL, PRIORITY, 0, 0);
