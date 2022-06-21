/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DW_ACE_V1X_H
#define ZEPHYR_INCLUDE_DRIVERS_DW_ACE_V1X_H

#include <zephyr/device.h>

typedef void (*irq_enable_t)(const struct device *dev, uint32_t irq);
typedef void (*irq_disable_t)(const struct device *dev, uint32_t irq);
typedef int (*irq_is_enabled_t)(const struct device *dev, unsigned int irq);
typedef int (*irq_connect_dynamic_t)(const struct device *dev,
				     unsigned int irq, unsigned int priority,
				     void (*routine)(const void *parameter),
				     const void *parameter, uint32_t flags);

struct dw_ace_v1_ictl_driver_api {
	irq_enable_t intr_enable;
	irq_disable_t intr_disable;
	irq_is_enabled_t intr_is_enabled;
#ifdef CONFIG_DYNAMIC_INTERRUPTS
	irq_connect_dynamic_t intr_connect_dynamic;
#endif
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_DW_ACE_V1X_H */
