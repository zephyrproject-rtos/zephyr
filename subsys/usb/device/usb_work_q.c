/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <usb_work_q.h>

K_KERNEL_STACK_DEFINE(z_usb_work_q_stack, CONFIG_USB_WORKQUEUE_STACK_SIZE);

struct k_work_q z_usb_work_q;

static int z_usb_work_q_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_queue_start(&z_usb_work_q,
			   z_usb_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(z_usb_work_q_stack),
			   CONFIG_USB_WORKQUEUE_PRIORITY, NULL);
	k_thread_name_set(&z_usb_work_q.thread, "usbworkq");

	return 0;
}

SYS_INIT(z_usb_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
