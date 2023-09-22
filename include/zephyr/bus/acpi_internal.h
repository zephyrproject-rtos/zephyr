/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ACPI_INTF_H_
#define ZEPHYR_INCLUDE_ACPI_INTF_H_

#include <zephyr/dt-bindings/acpi/acpi.h>
#include <zephyr/acpi/acpi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Platform ACPI init function (Internal use only).
 *
 */
int platform_acpi_init(const struct device *dev, platform_isr_t isr, int irq, int priority,
		       uint32_t flags, char *hid, char *uid);

/**
 * @brief ACPI specific init function (Internal use only).
 *
 */
#define Z_BUS_DEV_ACPI_INIT(n, init, isr)                                                     \
	static int __acpi_init_##n(const struct device *dev)                                       \
	{                                                                                          \
		return platform_acpi_init(dev, isr, DT_IRQN(n), DT_IRQ(n, priority),               \
					  Z_IRQ_FLAGS_GET(n), ACPI_DT_HID(n), ACPI_DT_UID(n));     \
	}                                                                                          \
												   \
	static int Z_BUS_DEV_INIT_NAME(n)(const struct device *dev)				   \
	{                                                                                          \
		int status;                                                                        \
												   \
		status = __acpi_init_##n(dev);                                                     \
		if (!status) {                                                                     \
			status = init(dev);                                                        \
		}						                                   \
		return status;                                                                     \
	}

#endif /* ZEPHYR_INCLUDE_ACPI_INTF_H_ */
