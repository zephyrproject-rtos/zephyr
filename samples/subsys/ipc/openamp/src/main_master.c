/*
 * Copyright (c) 2018, NXP
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This implements the master side of an OpenAMP system.
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openamp/open_amp.h>

#include "platform.h"
#include "resource_table.h"

#define APP_TASK_STACK_SIZE (512)
K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

static struct rpmsg_channel *rp_channel;
static struct rpmsg_endpoint *rp_endpoint;

static K_SEM_DEFINE(channel_created, 0, 1);

static K_SEM_DEFINE(message_received, 0, 1);
static volatile unsigned int received_data;

static struct rsc_table_info rsc_info;
static struct hil_proc *proc;

static void rpmsg_recv_callback(struct rpmsg_channel *channel, void *data,
				int data_length, void *private,
				unsigned long src)
{
	received_data = *((unsigned int *) data);
	k_sem_give(&message_received);
}

static void rpmsg_channel_created(struct rpmsg_channel *channel)
{
	rp_channel = channel;
	rp_endpoint = rpmsg_create_ept(rp_channel, rpmsg_recv_callback,
				       RPMSG_NULL, RPMSG_ADDR_ANY);
	k_sem_give(&channel_created);
}

static void rpmsg_channel_deleted(struct rpmsg_channel *channel)
{
	rpmsg_destroy_ept(rp_endpoint);
}

static unsigned int receive_message(void)
{
	while (k_sem_take(&message_received, K_NO_WAIT) != 0)
		hil_poll(proc, 0);
	return received_data;
}

static int send_message(unsigned int message)
{
	return rpmsg_send(rp_channel, &message, sizeof(message));
}

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	printk("\r\nOpenAMP demo started\r\n");

	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	metal_init(&metal_params);

	proc = platform_init(RPMSG_MASTER);
	if (proc == NULL) {
		printk("platform_init() failed\n");
		goto _cleanup;
	}

	resource_table_init((void **) &rsc_info.rsc_tab, &rsc_info.size);

	struct remote_proc *rproc_ptr = NULL;
	int status = remoteproc_resource_init(&rsc_info, proc,
					      rpmsg_channel_created,
					      rpmsg_channel_deleted,
					      rpmsg_recv_callback,
					      &rproc_ptr, RPMSG_MASTER);
	if (status != 0) {
		printk("remoteproc_resource_init() failed with status %d\n",
		       status);
		goto _cleanup;
	}

	while (k_sem_take(&channel_created, K_NO_WAIT) != 0)
		hil_poll(proc, 0);

	unsigned int message = 0;

	status = send_message(message);
	if (status < 0) {
		printk("send_message(%d) failed with status %d\n",
		       message, status);
		goto _cleanup;
	}

	while (message <= 100) {
		message = receive_message();
		printk("Primary core received a message: %d\n", message);

		message++;
		status = send_message(message);
		if (status < 0) {
			printk("send_message(%d) failed with status %d\n",
			       message, status);
			goto _cleanup;
		}
	}

_cleanup:
	if (rproc_ptr) {
		remoteproc_resource_deinit(rproc_ptr);
	}
	metal_finish();

	printk("OpenAMP demo ended.\n");
}

void main(void)
{
	printk("Starting application thread!\n");
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
