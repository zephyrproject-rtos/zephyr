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
#define REMOTE_EPT_ADDR (30)
#define LOCAL_EPT_ADDR  (40)

K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

#ifdef CPU_LPC54114J256BD64_cm4
#define RPMSG_LITE_LINK_ID (RL_PLATFORM_LPC5411x_M4_M0_LINK_ID)
#define SH_MEM_TOTAL_SIZE (6144)
#else
#error Please define RPMSG_LITE_LINK_ID and SH_MEM_TOTAL_SIZE for the CPU used.
#endif

#if defined(__ICCARM__) /* IAR Workbench */
#pragma location = "rpmsg_sh_mem_section"
char rpmsg_lite_base[SH_MEM_TOTAL_SIZE];
#elif defined(__GNUC__) /* LPCXpresso */
char rpmsg_lite_base[SH_MEM_TOTAL_SIZE] __attribute__((
						section(".noinit.$rpmsg_sh_mem")
					));
#elif defined(__CC_ARM) /* Keil MDK */
char rpmsg_lite_base[SH_MEM_TOTAL_SIZE] __attribute__((
						section("rpmsg_sh_mem_section")
					));
#else
#error "RPMsg: Please provide your definition of rpmsg_lite_base[]!"
#endif

struct the_message {
	u32_t DATA;
};

struct the_message volatile msg = {0};

void app_nameservice_isr_cb(unsigned int new_ept, const char *new_ept_name,
			    unsigned long flags, void *user_data)
{
	unsigned long *data = (unsigned long *)user_data;

	*data = new_ept;
}


void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	volatile unsigned long remote_addr = 0;
	struct rpmsg_lite_endpoint *my_ept;
	rpmsg_queue_handle my_queue;
	struct rpmsg_lite_instance *my_rpmsg;
	rpmsg_ns_handle ns_handle;
	int len;

	/* Print the initial banner */
	printk("\r\nRPMsg demo starts\r\n");

	my_rpmsg = rpmsg_lite_master_init(rpmsg_lite_base, SH_MEM_TOTAL_SIZE,
					  RPMSG_LITE_LINK_ID, RL_NO_FLAGS);

	my_queue = rpmsg_queue_create(my_rpmsg);
	my_ept = rpmsg_lite_create_ept(my_rpmsg, LOCAL_EPT_ADDR,
				       rpmsg_queue_rx_cb, my_queue);
	ns_handle = rpmsg_ns_bind(my_rpmsg, app_nameservice_isr_cb,
				  (void *)&remote_addr);

	/* Wait until the secondary core application issues the nameservice isr
	 * and the remote endpoint address is known.
	 */
	while (remote_addr == 0) {
	}

	/* Send the first message to the remoteproc */
	msg.DATA = 0;
	rpmsg_lite_send(my_rpmsg, my_ept, remote_addr, (char *)&msg,
			sizeof(struct the_message), RL_DONT_BLOCK);

	while (msg.DATA <= 100) {
		rpmsg_queue_recv(my_rpmsg, my_queue,
				 (unsigned long *)&remote_addr, (char *)&msg,
				 sizeof(struct the_message), &len, RL_BLOCK);
		printk("Primary core received a msg\r\n");
		printk("Message: Size=%x, DATA = %i\r\n", len, msg.DATA);
		msg.DATA++;

		rpmsg_lite_send(my_rpmsg, my_ept, remote_addr, (char *)&msg,
				sizeof(struct the_message), RL_BLOCK);
	}

	rpmsg_lite_destroy_ept(my_rpmsg, my_ept);
	my_ept = NULL;
	rpmsg_queue_destroy(my_rpmsg, my_queue);
	my_queue = NULL;
	rpmsg_ns_unbind(my_rpmsg, ns_handle);
	rpmsg_lite_deinit(my_rpmsg);

	/* Print the ending banner */
	printk("\r\nRPMsg demo ends\r\n");
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
