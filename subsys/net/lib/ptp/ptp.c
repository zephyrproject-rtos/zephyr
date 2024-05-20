/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp, CONFIG_PTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ptp.h>

K_KERNEL_STACK_DEFINE(ptp_stack, CONFIG_PTP_STACK_SIZE);

static struct k_thread ptp_thread_data;

static void ptp_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		k_msleep(1);
	}
}

static int ptp_init(void)
{
	k_tid_t tid;
	const struct ptp_clock *domain = ptp_clock_init();

	if (!domain) {
		return -ENODEV;
	}

	net_if_foreach(ptp_port_init, NULL);

	tid = k_thread_create(&ptp_thread_data, ptp_stack, K_KERNEL_STACK_SIZEOF(ptp_stack),
			      ptp_thread, NULL, NULL, NULL,
			      K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_thread_name_set(&ptp_thread_data, "PTP");

	return 0;
}

SYS_INIT(ptp_init, APPLICATION, CONFIG_PTP_INIT_PRIO);
