/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FAKE_DRIVER_H_
#define ZEPHYR_INCLUDE_DRIVERS_FAKE_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DT_DRV_COMPAT fakedriver

#if DT_INST_NODE_HAS_PROP(0, interrupts)
/* Platforms with specific needs can provide IRQn and priority via DT */
#define TEST_IRQ_NUM	DT_INST_IRQN(0)
#define TEST_IRQ_PRIO	DT_INST_IRQ(0, priority)
#elif defined(CONFIG_GIC)
/*
 * For the platforms that use the ARM GIC, use the SGI (software generated
 * interrupt) line 14 for testing.
 */
#define TEST_IRQ_NUM  14
#define TEST_IRQ_PRIO IRQ_DEFAULT_PRIORITY
#else
/* For all the other platforms, use the last available IRQ line for testing. */
#define TEST_IRQ_NUM  (CONFIG_NUM_IRQS - 1)
#define TEST_IRQ_PRIO 1
#endif

typedef void (*fake_driver_irq_callback_t)(const struct device *dev, void *user_data);

struct fake_driver_config {
	void (*irq_config_func)(void);
	uint16_t irq_num;
	uint8_t irq_priority;
};

struct fake_driver_data {
	fake_driver_irq_callback_t irq_callback;
	void *user_data;
};

__subsystem struct fake_driver_api {
	int (*configure)(const struct device *dev, int config);
	int (*register_irq_callback)(const struct device *dev, fake_driver_irq_callback_t cb,
				     void *user_data);
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FAKE_DRIVER_H_ */
