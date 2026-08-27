/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal Xen console driver data.
 * @ingroup xen_console_driver
 */

#ifndef __XEN_CONSOLE_H__
#define __XEN_CONSOLE_H__

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/device_mmio.h>

/**
 * @defgroup xen_console_driver Xen console driver
 * @ingroup xen_internal
 * @brief Keep the Xen UART console implementation synchronized with its device data layout.
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/**
 * @brief Runtime data used by the Xen console UART driver.
 */
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
/** @endcond */

/**
 * @brief Initialize the Xen console UART instance.
 *
 * @param dev UART device instance to initialize.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_console_init(const struct device *dev);

/** @} */

#endif /* __XEN_CONSOLE_H__ */
