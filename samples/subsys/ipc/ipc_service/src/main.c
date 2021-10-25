/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/ipm.h>
#include <sys/printk.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <init.h>

#include <ipc/ipc_service.h>

#define MI_BACKEND_DRIVER_NAME "MI_BACKEND"

#define APP_TASK_STACK_SIZE  1024

K_THREAD_STACK_DEFINE(thread_stack_1, APP_TASK_STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_stack_2, APP_TASK_STACK_SIZE);

static struct k_thread thread_data_1;
static struct k_thread thread_data_2;

static volatile uint8_t received_data_1;
static volatile uint8_t received_data_2;

static K_SEM_DEFINE(bound_ept1_sem, 0, 1);
static K_SEM_DEFINE(bound_ept2_sem, 0, 1);

static K_SEM_DEFINE(data_rx1_sem, 0, 1);
static K_SEM_DEFINE(data_rx2_sem, 0, 1);

static struct ipc_ept ept_1;
static struct ipc_ept ept_2;

static void ept_bound_1(void *priv)
{
	k_sem_give(&bound_ept1_sem);
}

static void ept_recv_1(const void *data, size_t len, void *priv)
{
	received_data_1 = *((uint8_t *) data);
	k_sem_give(&data_rx1_sem);
}

static void ept_error_1(const char *message, void *priv)
{
	printk("Endpoint [1] error: %s", message);
}

static void ept_bound_2(void *priv)
{
	k_sem_give(&bound_ept2_sem);
}

static void ept_recv_2(const void *data, size_t len, void *priv)
{
	received_data_2 = *((uint8_t *) data);
	k_sem_give(&data_rx2_sem);
}

static void ept_error_2(const char *message, void *priv)
{
	printk("Endpoint [2] error: %s", message);
}

void app_task_1(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	const struct device *ipc_instance;
	int status = 0;
	uint8_t message = 0U;

	printk("\r\nIPC Service [master 1] demo started\r\n");

	ipc_instance = device_get_binding(MI_BACKEND_DRIVER_NAME);

	static struct ipc_ept_cfg ept_cfg = {
		.name = "ep_1",
		.prio = 0,
		.priv = NULL,
		.cb = {
			.bound    = ept_bound_1,
			.received = ept_recv_1,
			.error    = ept_error_1,
		},
	};

	status = ipc_service_register_endpoint(ipc_instance, &ept_1, &ept_cfg);
	if (status < 0) {
		printk("ipc_service_register_endpoint failed %d\n", status);
		return;
	}

	k_sem_take(&bound_ept1_sem, K_FOREVER);

	while (message < 100) {
		status = ipc_service_send(&ept_1, &message, sizeof(message));
		if (status < 0) {
			printk("send_message(%d) failed with status %d\n",
			       message, status);
			break;
		}

		k_sem_take(&data_rx1_sem, K_FOREVER);
		message = received_data_1;

		printk("Master [1] received a message: %d\n", message);
		message++;
	}

	printk("IPC Service [master 1] demo ended.\n");
}

void app_task_2(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	const struct device *ipc_instance;
	int status = 0;
	uint8_t message = 0U;

	printk("\r\nIPC Service [master 2] demo started\r\n");

	ipc_instance = device_get_binding(MI_BACKEND_DRIVER_NAME);

	static struct ipc_ept_cfg ept_cfg = {
		.name = "ep_2",
		.prio = 0,
		.priv = NULL,
		.cb = {
			.bound    = ept_bound_2,
			.received = ept_recv_2,
			.error    = ept_error_2,
		},
	};

	status = ipc_service_register_endpoint(ipc_instance, &ept_2, &ept_cfg);

	if (status < 0) {
		printk("ipc_service_register_endpoint failed %d\n", status);
		return;
	}

	k_sem_take(&bound_ept2_sem, K_FOREVER);

	while (message < 100) {
		status = ipc_service_send(&ept_2, &message, sizeof(message));
		if (status < 0) {
			printk("send_message(%d) failed with status %d\n",
			       message, status);
			break;
		}

		k_sem_take(&data_rx2_sem, K_FOREVER);
		message = received_data_2;

		printk("Master [2] received a message: %d\n", message);
		message++;
	}

	printk("IPC Service [master 2] demo ended.\n");
}

void main(void)
{
	k_thread_create(&thread_data_1, thread_stack_1, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task_1,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	k_thread_create(&thread_data_2, thread_stack_2, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task_2,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
}
