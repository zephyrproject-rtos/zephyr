/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/ipc/ipc_service.h>
LOG_MODULE_REGISTER(app);

static volatile uint8_t sample_received_data;
static volatile bool ipc_ready;
static volatile bool ipc_bound;

static void ready_notify(volatile bool *ready)
{
	*ready = true;
}

static void ready_wait(volatile bool *ready)
{
	while (!*ready) {
		k_msleep(1);
	}
	*ready = false;
}
/*
 * ==> THREAD 1 (IPC instance 1) <==
 */

static void ept_bound(void *priv)
{
	ready_notify(&ipc_bound);
}

static void ept_recv(const void *data, size_t len, void *priv)
{
	sample_received_data = *((uint8_t *) data);

	ready_notify(&ipc_ready);
}

static const struct ipc_ept_cfg ept_cfg = {
	.name = "sample",
	.cb = {
		.bound    = ept_bound,
		.received = ept_recv,
	},
};

static void ipc_run(void)
{
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

	ready_wait(&ipc_bound);

	while (message < 20) {
		ret = ipc_service_send(&ept, &message, sizeof(message));
		if (ret < 0) {
			LOG_ERR("send_message(%d) failed with ret %d", message, ret);
			break;
		}

		ready_wait(&ipc_ready);
		message = sample_received_data;

		LOG_INF("HOST [1]: %d", message);
		message++;

		k_msleep(10);
	}

	LOG_INF("Multidomain logging HOST demo ended.");
}

int main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

	ipc_run();

	return 0;
}
