/*
 * Copyright (c) 2018, NXP
 * Copyright (c) 2018-2019, Linaro Limited
 * Copyright (c) 2018-2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/ipc/rpmsg_service.h>

#define APP_TASK_STACK_SIZE (1024)
K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

static volatile unsigned int received_data;

static K_SEM_DEFINE(data_sem, 0, 1);
static K_SEM_DEFINE(data_rx_sem, 0, 1);

int endpoint_cb(struct rpmsg_endpoint *ept, void *data,
		size_t len, uint32_t src, void *priv)
{
	received_data = *((unsigned int *) data);

	k_sem_give(&data_rx_sem);

	return RPMSG_SUCCESS;
}

static int ep_id;

static unsigned int receive_message(void)
{
	k_sem_take(&data_rx_sem, K_FOREVER);
	return received_data;
}

static int send_message(unsigned int message)
{
	return rpmsg_service_send(ep_id, &message, sizeof(message));
}

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int status = 0;
	unsigned int message = 0U;

	printk("\r\nRPMsg Service [remote] demo started\r\n");

	while (message < 99) {
		message = receive_message();
		printk("Remote core received a message: %d\n", message);

		message++;
		status = send_message(message);
		if (status < 0) {
			printk("send_message(%d) failed with status %d\n",
			       message, status);
			break;
		}
	}

	printk("RPMsg Service demo ended.\n");
}

int main(void)
{
	printk("Starting application thread!\n");
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	return 0;
}

/* Make sure we register endpoint before RPMsg Service is initialized. */
int register_endpoint(void)
{
	int status;

	status = rpmsg_service_register_endpoint("demo", endpoint_cb);

	if (status < 0) {
		printk("rpmsg_create_ept failed %d\n", status);
		return status;
	}

	ep_id = status;

	return 0;
}

SYS_INIT(register_endpoint, POST_KERNEL, CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);
