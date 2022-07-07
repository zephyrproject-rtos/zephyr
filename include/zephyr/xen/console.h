/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_CONSOLE_H__
#define __XEN_CONSOLE_H__

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/device_mmio.h>

struct hvc_xen_data {
	DEVICE_MMIO_RAM;	/* should be first */
	const struct device *dev;
	struct xencons_interface *intf;
	uint64_t evtchn;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

int xen_console_init(const struct device *dev);

#endif /* __XEN_CONSOLE_H__ */
