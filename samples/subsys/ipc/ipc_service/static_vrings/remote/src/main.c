/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>

#include <zephyr/ipc/ipc_service.h>

#define STACKSIZE	(1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define PRIORITY	K_PRIO_PREEMPT(2)

K_THREAD_STACK_DEFINE(ipc0A_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(ipc0B_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(ipc1_stack, STACKSIZE);

static volatile uint8_t ipc0A_received_data;
static volatile uint8_t ipc0B_received_data;
static volatile uint8_t ipc1_received_data;

static K_SEM_DEFINE(ipc0A_bound_sem, 0, 1);
static K_SEM_DEFINE(ipc0B_bound_sem, 0, 1);
static K_SEM_DEFINE(ipc1_bound_sem, 0, 1);

static K_SEM_DEFINE(ipc0A_data_sem, 0, 1);
static K_SEM_DEFINE(ipc0B_data_sem, 0, 1);
static K_SEM_DEFINE(ipc1_data_sem, 0, 1);

/*
 * ==> THREAD 0A (IPC instance 0 - endpoint A) <==
 */

static void ipc0A_ept_bound(void *priv)
{
	k_sem_give(&ipc0A_bound_sem);
}

static void ipc0A_ept_recv(const void *data, size_t len, void *priv)
{
	ipc0A_received_data = *((uint8_t *) data);

	k_sem_give(&ipc0A_data_sem);
}

static struct ipc_ept_cfg ipc0A_ept_cfg = {
	.name = "ipc0A",
	.cb = {
		.bound    = ipc0A_ept_bound,
		.received = ipc0A_ept_recv,
	},
};

static void ipc0A_entry(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	const struct device *ipc0_instance;
	unsigned char message = 0;
	struct ipc_ept ipc0A_ept;
	int ret;

	printk("IPC-service REMOTE [INST 0 - ENDP A] demo started\n");

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("ipc_service_open_instance() failure\n");
		return;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ipc0A_ept, &ipc0A_ept_cfg);
	if (ret < 0) {
		printf("ipc_service_register_endpoint() failure\n");
		return;
	}

	k_sem_take(&ipc0A_bound_sem, K_FOREVER);

	while (message < 99) {
		k_sem_take(&ipc0A_data_sem, K_FOREVER);
		message = ipc0A_received_data;

		printk("REMOTE [0A]: %d\n", message);

		message++;

		ret = ipc_service_send(&ipc0A_ept, &message, sizeof(message));
		if (ret < 0) {
			printk("send_message(%d) failed with ret %d\n", message, ret);
			break;
		}
	}

	printk("IPC-service REMOTE [INST 0 - ENDP A] demo ended.\n");
}
K_THREAD_DEFINE(ipc0A_thread_id, STACKSIZE, ipc0A_entry, NULL, NULL, NULL, PRIORITY, 0, 0);

/*
 * ==> THREAD 0B (IPC instance 0 - endpoint B) <==
 */

static void ipc0B_ept_bound(void *priv)
{
	k_sem_give(&ipc0B_bound_sem);
}

static void ipc0B_ept_recv(const void *data, size_t len, void *priv)
{
	ipc0B_received_data = *((uint8_t *) data);

	k_sem_give(&ipc0B_data_sem);
}

static struct ipc_ept_cfg ipc0B_ept_cfg = {
	.name = "ipc0B",
	.cb = {
		.bound    = ipc0B_ept_bound,
		.received = ipc0B_ept_recv,
	},
};

static void ipc0B_entry(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	const struct device *ipc0_instance;
	unsigned char message = 0;
	struct ipc_ept ipc0B_ept;
	int ret;

	printk("IPC-service REMOTE [INST 0 - ENDP B] demo started\n");

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("ipc_service_open_instance() failure\n");
		return;
	}

	/*
	 * Wait 2 sec to give the opportunity to the PRIMARY core to register
	 * the endpoint first
	 */

	k_sleep(K_MSEC(2000));

	ret = ipc_service_register_endpoint(ipc0_instance, &ipc0B_ept, &ipc0B_ept_cfg);
	if (ret < 0) {
		printf("ipc_service_register_endpoint() failure\n");
		return;
	}

	k_sem_take(&ipc0B_bound_sem, K_FOREVER);

	while (message < 99) {
		k_sem_take(&ipc0B_data_sem, K_FOREVER);
		message = ipc0B_received_data;

		printk("REMOTE [0B]: %d\n", message);

		message++;

		ret = ipc_service_send(&ipc0B_ept, &message, sizeof(message));
		if (ret < 0) {
			printk("send_message(%d) failed with ret %d\n", message, ret);
			break;
		}
	}

	printk("IPC-service REMOTE [INST 0 - ENDP B] demo ended.\n");
}
K_THREAD_DEFINE(ipc0B_thread_id, STACKSIZE, ipc0B_entry, NULL, NULL, NULL, PRIORITY, 0, 0);

/*
 * ==> THREAD 1 (IPC instance 1) <==
 *
 * NOTE: This instance is using the NOCOPY copability of the backend.
 */

static struct ipc_ept ipc1_ept;
static void *recv_data;

static void ipc1_ept_bound(void *priv)
{
	k_sem_give(&ipc1_bound_sem);
}

static void ipc1_ept_recv(const void *data, size_t len, void *priv)
{
	int ret;

	ret = ipc_service_hold_rx_buffer(&ipc1_ept, (void *) data);
	if (ret < 0) {
		printk("ipc_service_hold_rx_buffer failed with ret %d\n", ret);
	}

	/*
	 * This will only support a synchronous request-answer mechanism. For
	 * asynchronous cases a chain list should be implemented.
	 */
	recv_data = (void *) data;

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
	int ret;

	printk("IPC-service REMOTE [INST 1] demo started\n");

	ipc1_instance = DEVICE_DT_GET(DT_NODELABEL(ipc1));

	ret = ipc_service_open_instance(ipc1_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("ipc_service_open_instance() failure\n");
		return;
	}

	ret = ipc_service_register_endpoint(ipc1_instance, &ipc1_ept, &ipc1_ept_cfg);
	if (ret < 0) {
		printf("ipc_service_register_endpoint() failure\n");
		return;
	}

	k_sem_take(&ipc1_bound_sem, K_FOREVER);

	while (message < 50) {
		uint32_t len = sizeof(message);
		void *data;

		k_sem_take(&ipc1_data_sem, K_FOREVER);

		printk("REMOTE [1]: %d\n", *((unsigned char *) recv_data));

		ret = ipc_service_get_tx_buffer(&ipc1_ept, &data, &len, K_FOREVER);
		if (ret < 0) {
			printk("ipc_service_get_tx_buffer failed with ret %d\n", ret);
			break;
		}

		*((unsigned char *) data) = *((unsigned char *) recv_data) + 1;

		ret = ipc_service_release_rx_buffer(&ipc1_ept, recv_data);
		if (ret < 0) {
			printk("ipc_service_release_rx_buffer failed with ret %d\n", ret);
			break;
		}

		ret = ipc_service_send_nocopy(&ipc1_ept, data, sizeof(unsigned char));
		if (ret < 0) {
			printk("send_message(%d) failed with ret %d\n", message, ret);
			break;
		}

		message++;
	}

	printk("IPC-service REMOTE [INST 1] demo ended.\n");
}
K_THREAD_DEFINE(ipc1_thread_id, STACKSIZE, ipc1_entry, NULL, NULL, NULL, PRIORITY, 0, 0);
