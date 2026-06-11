/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_TC3X_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_TC3X_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/sys_io.h>

#define TC3X_OUT_OFFSET  0x0
#define TC3X_OMR_OFFSET  0x4
#define TC3X_IOCR_OFFSET 0x10
#define TC3X_IN_OFFSET   0x24

#define TC3X_IOCR_OUTPUT     0x10
#define TC3X_IOCR_OPEN_DRAIN 0x08
#define TC3X_IOCR_PULL_DOWN  0x01
#define TC3X_IOCR_PULL_UP    0x02

#define TC3X_GPIO_MODE_INPUT_TRISTATE    0
#define TC3X_GPIO_MODE_INPUT_PULL_UP     TC3X_IOCR_PULL_UP
#define TC3X_GPIO_MODE_INPUT_PULL_DOWN   TC3X_IOCR_PULL_DOWN
#define TC3X_GPIO_MODE_OUTPUT_PUSH_PULL  (TC3X_IOCR_OUTPUT)
#define TC3X_GPIO_MODE_OUTPUT_OPEN_DRAIN (TC3X_IOCR_OUTPUT | TC3X_IOCR_OPEN_DRAIN)

enum gpio_tc3x_irq_type {
	TC3X_IRQ_TYPE_GTM = 1,
	TC3X_IRQ_TYPE_ERU,
};

/* irq-source DT cell layout:
 *   bits  3:0   mux  (ERU EXIS, or GTM TIMINSEL mux value)
 *   bits  7:4   ch   (ERU EICR channel 0..7, or GTM TIM channel)
 *   bits 11:8   cls  (ERU OGU/INP 0..3, or GTM TIM unit 0..7)
 *   bits 13:12  type (TC3X_IRQ_TYPE_GTM=1, TC3X_IRQ_TYPE_ERU=2)
 *   bits 31:28  pin  (port pin 0..15)
 */
struct gpio_tc3x_irq_source {
	uint32_t mux: 4;
	uint32_t ch: 4;
	uint32_t cls: 4;
	uint32_t type: 2;
	uint32_t pin: 4;
};

struct gpio_tc3x_config {
	struct gpio_driver_config common;
	mm_reg_t base;
	void (*config_func)(const struct device *dev);
	const struct gpio_tc3x_irq_source *irq_sources;
	uint8_t irq_source_count;
};

struct gpio_tc3x_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	/* Bitmap of pins configured as level-triggered. Level semantics
	 * are emulated in the ISR by retrying gpio_fire_callbacks while
	 * the pin level matches the configured trigger.
	 */
	uint16_t level_pins;
};

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_TC3X_H_ */
