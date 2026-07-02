/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/ipc/ipc_service.h>

#define STACKSIZE    CONFIG_SAMPLE_IPC_RPMSG_LITE_STACK_SIZE
#define PRIORITY     K_PRIO_PREEMPT(2)
#define NUM_MESSAGES 20

/* Start-gate semaphores: ipc1 starts after ipc0 ends, ipc2 after ipc1 ends */
static K_SEM_DEFINE(ipc1_start_sem, 0, 1);
static K_SEM_DEFINE(ipc2_start_sem, 0, 1);

/* Bound / data semaphores */
static K_SEM_DEFINE(ipc0_bound_sem, 0, 1);
static K_SEM_DEFINE(ipc1_bound_sem, 0, 1);
static K_SEM_DEFINE(ipc2_bound_sem, 0, 1);

static K_SEM_DEFINE(ipc0_data_sem, 0, 1);
static K_SEM_DEFINE(ipc1_data_sem, 0, 1);
static K_SEM_DEFINE(ipc2_data_sem, 0, 1);

static volatile uint8_t ipc0_received_data;
static volatile uint8_t ipc2_received_data;

/* NOCOPY: ipc1 stores the received buffer pointer */
static void *ipc1_recv_data;
static bool ipc1_recv_data_is_copy;
static uint8_t ipc1_recv_data_copy;

static struct ipc_ept ipc1_ept;

/* ------------------------------------------------------------------ */
/*  IPC instance 0 callbacks                                           */
/* ------------------------------------------------------------------ */

static void ipc0_ept_bound(void *priv)
{
	k_sem_give(&ipc0_bound_sem);
}

static void ipc0_ept_recv(const void *data, size_t len, void *priv)
{
	ipc0_received_data = *((uint8_t *)data);
	k_sem_give(&ipc0_data_sem);
}

static struct ipc_ept_cfg ipc0_ept_cfg = {
	.name = "ipc0",
	.cb = {
		.bound    = ipc0_ept_bound,
		.received = ipc0_ept_recv,
	},
};

/* ------------------------------------------------------------------ */
/*  IPC instance 1 callbacks (NOCOPY)                                  */
/* ------------------------------------------------------------------ */

static void ipc1_ept_bound(void *priv)
{
	k_sem_give(&ipc1_bound_sem);
}

static void ipc1_ept_recv(const void *data, size_t len, void *priv)
{
	int ret;

	if (len == 0U) {
		return;
	}

	/*
	 * The buffer belongs to the backend. Hold it before returning from the
	 * callback, otherwise ipc1_recv_data becomes a dangling pointer.
	 */
	ret = ipc_service_hold_rx_buffer(&ipc1_ept, (void *)data);
	if (ret == 0) {
		ipc1_recv_data = (void *)data;
		ipc1_recv_data_is_copy = false;
	} else {
		/* Fall back to copying the first byte so the sample keeps running. */
		ipc1_recv_data_copy = *((const uint8_t *)data);
		ipc1_recv_data = &ipc1_recv_data_copy;
		ipc1_recv_data_is_copy = true;
		printk("ipc_service_hold_rx_buffer failed with ret %d\n", ret);
	}

	k_sem_give(&ipc1_data_sem);
}

static struct ipc_ept_cfg ipc1_ept_cfg = {
	.name = "ipc1",
	.cb = {
		.bound    = ipc1_ept_bound,
		.received = ipc1_ept_recv,
	},
};

/* ------------------------------------------------------------------ */
/*  IPC instance 2 callbacks                                           */
/* ------------------------------------------------------------------ */

static void ipc2_ept_bound(void *priv)
{
	k_sem_give(&ipc2_bound_sem);
}

static void ipc2_ept_recv(const void *data, size_t len, void *priv)
{
	ipc2_received_data = *((uint8_t *)data);
	k_sem_give(&ipc2_data_sem);
}

static struct ipc_ept_cfg ipc2_ept_cfg = {
	.name = "ipc2",
	.cb = {
		.bound    = ipc2_ept_bound,
		.received = ipc2_ept_recv,
	},
};

/* ------------------------------------------------------------------ */
/*  THREAD 0 (IPC instance 0)                                          */
/* ------------------------------------------------------------------ */

static void ipc0_entry(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	const struct device *ipc0_instance;
	unsigned char message = 0;
	struct ipc_ept ipc0_ept;
	int ret;

	printk("IPC-service REMOTE [INST 0] demo started\n");

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("ipc_service_open_instance() failure\n");
		goto out;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ipc0_ept, &ipc0_ept_cfg);
	if (ret < 0) {
		printk("ipc_service_register_endpoint() failure\n");
		goto out;
	}

	k_sem_take(&ipc0_bound_sem, K_FOREVER);

	while (message < NUM_MESSAGES - 1) {
		k_sem_take(&ipc0_data_sem, K_FOREVER);
		message = ipc0_received_data;
		printk("REMOTE [0]: %d\n", message);
		message++;

		ret = ipc_service_send(&ipc0_ept, &message, sizeof(message));
		if (ret < 0) {
			printk("send_message(%d) failed with ret %d\n", message, ret);
			break;
		}
	}

	printk("IPC-service REMOTE [INST 0] demo ended.\n");

out:
	k_sem_give(&ipc1_start_sem);
}

K_THREAD_DEFINE(ipc0_thread_id, STACKSIZE, ipc0_entry, NULL, NULL, NULL, PRIORITY, 0, 0);

/* ------------------------------------------------------------------ */
/*  THREAD 1 (IPC instance 1) — NOCOPY                                 */
/* ------------------------------------------------------------------ */

static void ipc1_entry(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	const struct device *ipc1_instance;
	unsigned char message = 0;
	int ret;

	/* Wait for instance 0 to finish */
	k_sem_take(&ipc1_start_sem, K_FOREVER);

	printk("IPC-service REMOTE [INST 1] demo started\n");

	ipc1_instance = DEVICE_DT_GET(DT_NODELABEL(ipc1));

	ret = ipc_service_open_instance(ipc1_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("ipc_service_open_instance() failure\n");
		goto out;
	}

	/*
	 * Give the host thread time to open and register its ipc1 endpoint
	 * after its own ipc0 loop finishes.
	 */
	k_sleep(K_MSEC(100));

	ret = ipc_service_register_endpoint(ipc1_instance, &ipc1_ept, &ipc1_ept_cfg);
	if (ret < 0) {
		printk("ipc_service_register_endpoint() failure\n");
		goto out;
	}

	k_sem_take(&ipc1_bound_sem, K_FOREVER);

	while (message < NUM_MESSAGES) {
		uint32_t len = sizeof(message);
		void *data;

		k_sem_take(&ipc1_data_sem, K_FOREVER);
		printk("REMOTE [1]: %d\n", *((unsigned char *)ipc1_recv_data));

		ret = ipc_service_get_tx_buffer(&ipc1_ept, &data, &len, K_FOREVER);
		if (ret < 0) {
			printk("ipc_service_get_tx_buffer failed with ret %d\n", ret);
			break;
		}

		*((unsigned char *)data) = *((unsigned char *)ipc1_recv_data) + 1;

		if (!ipc1_recv_data_is_copy) {
			ret = ipc_service_release_rx_buffer(&ipc1_ept, ipc1_recv_data);
			if (ret < 0) {
				printk("ipc_service_release_rx_buffer failed with ret %d\n", ret);
				break;
			}
		}

		ret = ipc_service_send_nocopy(&ipc1_ept, data, sizeof(unsigned char));
		if (ret < 0) {
			printk("send_message(%d) failed with ret %d\n", message, ret);
			break;
		}

		message++;
	}

	printk("IPC-service REMOTE [INST 1] demo ended.\n");

out:
	k_sem_give(&ipc2_start_sem);
}

K_THREAD_DEFINE(ipc1_thread_id, STACKSIZE, ipc1_entry, NULL, NULL, NULL, PRIORITY, 0, 0);

/* ------------------------------------------------------------------ */
/*  THREAD 2 (IPC instance 2)                                          */
/* ------------------------------------------------------------------ */

static void ipc2_entry(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	const struct device *ipc2_instance;
	unsigned char message = 0;
	struct ipc_ept ipc2_ept;
	int ret;

	/* Wait for instance 1 to finish */
	k_sem_take(&ipc2_start_sem, K_FOREVER);

	printk("IPC-service REMOTE [INST 2] demo started\n");

	ipc2_instance = DEVICE_DT_GET(DT_NODELABEL(ipc2));

	ret = ipc_service_open_instance(ipc2_instance);
	if (ret < 0 && ret != -EALREADY) {
		printk("ipc_service_open_instance() failure\n");
		return;
	}

	ret = ipc_service_register_endpoint(ipc2_instance, &ipc2_ept, &ipc2_ept_cfg);
	if (ret < 0) {
		printk("ipc_service_register_endpoint() failure\n");
		return;
	}

	k_sem_take(&ipc2_bound_sem, K_FOREVER);

	while (message < NUM_MESSAGES - 1) {
		k_sem_take(&ipc2_data_sem, K_FOREVER);
		message = ipc2_received_data;
		printk("REMOTE [2]: %d\n", message);
		message++;

		ret = ipc_service_send(&ipc2_ept, &message, sizeof(message));
		if (ret < 0) {
			printk("send_message(%d) failed with ret %d\n", message, ret);
			break;
		}
	}

	printk("IPC-service REMOTE [INST 2] demo ended.\n");
}

K_THREAD_DEFINE(ipc2_thread_id, STACKSIZE, ipc2_entry, NULL, NULL, NULL, PRIORITY, 0, 0);
