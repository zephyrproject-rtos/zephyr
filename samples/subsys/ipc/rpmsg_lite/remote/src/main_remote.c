/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpmsg_lite.h"
#include "rpmsg_queue.h"
#include "rpmsg_ns.h"

#define APP_TASK_STACK_SIZE (256)
#define LOCAL_EPT_ADDR (30)

#ifdef CPU_LPC54114J256BD64_cm0plus
#define RPMSG_LITE_LINK_ID (RL_PLATFORM_LPC5411x_M4_M0_LINK_ID)
#define RPMSG_LITE_SHMEM_BASE (0x20026800)
#define RPMSG_LITE_NS_USED (1)
#define RPMSG_LITE_NS_ANNOUNCE_STRING "rpmsg-openamp-demo-channel"
#else
#error Please define ERPC_TRANSPORT_RPMSG_LITE_LINK_ID, RPMSG_LITE_SHMEM_BASE, \
RPMSG_LITE_NS_USED and RPMSG_LITE_NS_ANNOUNCE_STRING values for the CPU used.
#endif

K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

struct the_message {
	u32_t DATA;
};

struct the_message volatile msg = {0};

void app_nameservice_isr_cb(unsigned int new_ept, const char *new_ept_name,
			    unsigned long flags, void *user_data)
{
}

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	volatile unsigned long remote_addr;
	struct rpmsg_lite_endpoint *volatile my_ept;
	volatile rpmsg_queue_handle my_queue;
	struct rpmsg_lite_instance *volatile my_rpmsg;
#ifdef RPMSG_LITE_NS_USED
	volatile rpmsg_ns_handle ns_handle;
#endif /*RPMSG_LITE_NS_USED*/

	my_rpmsg = rpmsg_lite_remote_init((void *)RPMSG_LITE_SHMEM_BASE,
					  RPMSG_LITE_LINK_ID, RL_NO_FLAGS);

	while (!rpmsg_lite_is_link_up(my_rpmsg)) {
	}

	my_queue = rpmsg_queue_create(my_rpmsg);
	my_ept = rpmsg_lite_create_ept(my_rpmsg, LOCAL_EPT_ADDR,
				       rpmsg_queue_rx_cb, my_queue);

#ifdef RPMSG_LITE_NS_USED
	ns_handle = rpmsg_ns_bind(my_rpmsg, app_nameservice_isr_cb, NULL);
	rpmsg_ns_announce(my_rpmsg, my_ept, RPMSG_LITE_NS_ANNOUNCE_STRING,
			  RL_NS_CREATE);
	printk("Nameservice announce sent.\r\n");
#endif /*RPMSG_LITE_NS_USED*/

	while (msg.DATA <= 100) {
		printk("Waiting for ping...\r\n");
		rpmsg_queue_recv(my_rpmsg, my_queue,
				 (unsigned long *)&remote_addr, (char *)&msg,
				 sizeof(struct the_message), NULL, RL_BLOCK);
		msg.DATA++;
		printk("Sending pong...\r\n");
		rpmsg_lite_send(my_rpmsg, my_ept, remote_addr, (char *)&msg,
				sizeof(struct the_message), RL_BLOCK);
	}

	rpmsg_lite_destroy_ept(my_rpmsg, my_ept);
	my_ept = NULL;
	rpmsg_queue_destroy(my_rpmsg, my_queue);
	my_queue = NULL;
#ifdef RPMSG_LITE_NS_USED
	rpmsg_ns_unbind(my_rpmsg, ns_handle);
#endif
	rpmsg_lite_deinit(my_rpmsg);
	msg.DATA = 0;

	/* End of example */
	while (1) {
	}
}

void main(void)
{
	printk("===== app started ========\n");

	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}
