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

static void timeout(struct k_timer *timer)
{
	static int cnt;

	LOG_INF("loop: %d", cnt++);
	if (cnt < 10) {
		k_timer_start(timer, K_MSEC(500), K_NO_WAIT);
	}
}
K_TIMER_DEFINE(timeout_timer, timeout, NULL);

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

	printk("IPC-service REMOTE [INST 1] demo started\n");

	ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	/*k_sleep(K_FOREVER);*/
	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("ipc_service_open_instance() failure\n");
		return;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &ept, &ept_cfg);
	if (ret < 0) {
		printk("ipc_service_register_endpoint() failure\n");
		return;
	}

	LOG_INF("wait for bound");
	ready_wait(&ipc_bound);
	LOG_INF("bounded");

	while (message < 19) {
		ready_wait(&ipc_ready);
		message = sample_received_data;

		LOG_INF("REMOTE [1]: %d", message);

		message++;

		ret = ipc_service_send(&ept, &message, sizeof(message));
		if (ret < 0) {
			printk("send_message(%d) failed with ret %d\n", message, ret);
			break;
		}

		k_msleep(10);
	}

	printk("IPC-service REMOTE [INST 1] demo ended.\n");
}

int main(void)
{
	k_timer_start(&timeout_timer, K_MSEC(500), K_NO_WAIT);

	ipc_run();

	return 0;
}
