/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2019 Linaro Limited
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in GD32 MCUs
 *
 * Driver is currently implemented to support following EXTI lines
 * Lines 0 to 15. Lines > 15 not supported
 *
 */
#include <device.h>
#include <soc.h>
#include <sys/__assert.h>
#include <drivers/interrupt_controller/exti_gd32.h>

#include "gd32vf103_exti.h"

const IRQn_Type exti_irq_table[] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
	EXTI4_IRQn, EXTI5_9_IRQn, EXTI5_9_IRQn, EXTI5_9_IRQn,
	EXTI5_9_IRQn, EXTI5_9_IRQn, EXTI10_15_IRQn, EXTI10_15_IRQn,
	EXTI10_15_IRQn, EXTI10_15_IRQn, EXTI10_15_IRQn, EXTI10_15_IRQn
};

/* wrapper for user callback */
struct __exti_cb {
	gd32_exti_callback_t cb;
	void *data;
};

/* driver data */
struct gd32_exti_data {
	/* per-line callbacks */
	struct __exti_cb cb[ARRAY_SIZE(exti_irq_table)];
};

static void exti_enable_interrupt(exti_line_enum linex)
{
	/* set the EXTI mode and enable the interrupts or events from EXTI line x */
	EXTI_INTEN |= (uint32_t) linex;
}

static void exti_set_rising_trigger(exti_line_enum linex)
{
	EXTI_RTEN |= (uint32_t) linex;
}

static void exti_reset_rising_trigger(exti_line_enum linex)
{
	EXTI_RTEN &= ~(uint32_t) linex;
}

static void exti_set_falling_trigger(exti_line_enum linex)
{
	EXTI_FTEN |= (uint32_t) linex;
}

static void exti_reset_falling_trigger(exti_line_enum linex)
{
	EXTI_FTEN &= ~(uint32_t) linex;
}


void gd32_exti_enable(int line)
{
	int irqnum = 0;

	if (line >= ARRAY_SIZE(exti_irq_table)) {
		__ASSERT_NO_MSG(line);
	}

	/* Get matching exti irq provided line thanks to irq_table */
	irqnum = exti_irq_table[line];
	if (irqnum == 0xFF) {
		__ASSERT_NO_MSG(line);
	}

	/* Enable requested line interrupt */
	exti_enable_interrupt(1 << line);

	/* Enable exti irq interrupt */
	irq_enable(irqnum);
}

void gd32_exti_disable(int line)
{
	if (line < 32) {
		exti_interrupt_disable(1 << line);
	} else {
		__ASSERT_NO_MSG(line);
	}
}

/**
 * @brief check if interrupt is pending
 *
 * @param line line number
 */
static inline int gd32_exti_is_pending(int line)
{
	int ret = 0;

	if (line < 32) {
		ret = exti_interrupt_flag_get(1 << line);
	} else {
		__ASSERT_NO_MSG(line);
	}
	return ret;
}

/**
 * @brief clear pending interrupt bit
 *
 * @param line line number
 */
static inline void gd32_exti_clear_pending(int line)
{
	if (line < 32) {
		exti_interrupt_flag_clear(1 << line);
	} else {
		__ASSERT_NO_MSG(line);
	}
}

void gd32_exti_trigger(int line, int trigger)
{

	if (line >= 32) {
		__ASSERT_NO_MSG(line);
	}

	switch (trigger) {
	case EXTI_TRIG_NONE:
		exti_reset_rising_trigger(1 << line);
		exti_reset_falling_trigger(1 << line);
		break;
	case EXTI_TRIG_RISING:
		exti_set_rising_trigger(1 << line);
		exti_reset_falling_trigger(1 << line);
		break;
	case EXTI_TRIG_FALLING:
		exti_reset_rising_trigger(1 << line);
		exti_set_falling_trigger(1 << line);
		break;
	case EXTI_TRIG_BOTH:
		exti_set_rising_trigger(1 << line);
		exti_set_falling_trigger(1 << line);
		break;
	default:
		__ASSERT_NO_MSG(trigger);
	}
}

/**
 * @brief EXTI ISR handler
 *
 * Check EXTI lines in range @min @max for pending interrupts
 *
 * @param arg isr argument
 * @param min low end of EXTI# range
 * @param max low end of EXTI# range
 */
static void __gd32_exti_isr(int min, int max, const struct device *dev)
{
	struct gd32_exti_data *data = dev->data;
	int line;

	/* see which bits are set */
	for (line = min; line < max; line++) {
		/* check if interrupt is pending */
		if (gd32_exti_is_pending(line)) {
			/* clear pending interrupt */
			gd32_exti_clear_pending(line);

			/* run callback only if one is registered */
			if (!data->cb[line].cb) {
				continue;
			}

			data->cb[line].cb(line, data->cb[line].data);
		}
	}
}

static inline void __gd32_exti_isr_0(const void *arg)
{
	__gd32_exti_isr(0, 1, arg);
}

static inline void __gd32_exti_isr_1(const void *arg)
{
	__gd32_exti_isr(1, 2, arg);
}

static inline void __gd32_exti_isr_2(const void *arg)
{
	__gd32_exti_isr(2, 3, arg);
}

static inline void __gd32_exti_isr_3(const void *arg)
{
	__gd32_exti_isr(3, 4, arg);
}

static inline void __gd32_exti_isr_4(const void *arg)
{
	__gd32_exti_isr(4, 5, arg);
}

static inline void __gd32_exti_isr_5_9(const void *arg)
{
	__gd32_exti_isr(5, 10, arg);
}

static inline void __gd32_exti_isr_10_15(const void *arg)
{
	__gd32_exti_isr(10, 16, arg);
}

static void __gd32_exti_connect_irqs(const struct device *dev);

/**
 * @brief initialize EXTI device driver
 */
static int gd32_exti_init(const struct device *dev)
{
	__gd32_exti_connect_irqs(dev);

	return 0;
}

static struct gd32_exti_data exti_data;
DEVICE_DEFINE(exti_gd32, GD32_EXTI_NAME, gd32_exti_init,
	      NULL, &exti_data, NULL,
	      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      NULL);

/**
 * @brief set & unset for the interrupt callbacks
 */
int gd32_exti_set_callback(int line, gd32_exti_callback_t cb, void *arg)
{
	const struct device *dev = DEVICE_GET(exti_gd32);
	struct gd32_exti_data *data = dev->data;

	if (data->cb[line].cb) {
		return -EBUSY;
	}

	data->cb[line].cb = cb;
	data->cb[line].data = arg;

	return 0;
}

void gd32_exti_unset_callback(int line)
{
	const struct device *dev = DEVICE_GET(exti_gd32);
	struct gd32_exti_data *data = dev->data;

	data->cb[line].cb = NULL;
	data->cb[line].data = NULL;
}

/**
 * @brief connect all interrupts
 */
static void __gd32_exti_connect_irqs(const struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(EXTI0_IRQn,
		    CONFIG_EXTI_GD32_EXTI0_IRQ_PRI,
		    __gd32_exti_isr_0, DEVICE_GET(exti_gd32),
		    0);
	IRQ_CONNECT(EXTI1_IRQn,
		    CONFIG_EXTI_GD32_EXTI1_IRQ_PRI,
		    __gd32_exti_isr_1, DEVICE_GET(exti_gd32),
		    0);
	IRQ_CONNECT(EXTI2_IRQn,
		    CONFIG_EXTI_GD32_EXTI2_IRQ_PRI,
		    __gd32_exti_isr_2, DEVICE_GET(exti_gd32),
		    0);
	IRQ_CONNECT(EXTI3_IRQn,
		    CONFIG_EXTI_GD32_EXTI3_IRQ_PRI,
		    __gd32_exti_isr_3, DEVICE_GET(exti_gd32),
		    0);
	IRQ_CONNECT(EXTI4_IRQn,
		    CONFIG_EXTI_GD32_EXTI4_IRQ_PRI,
		    __gd32_exti_isr_4, DEVICE_GET(exti_gd32),
		    0);
	IRQ_CONNECT(EXTI5_9_IRQn,
		    CONFIG_EXTI_GD32_EXTI5_9_IRQ_PRI,
		    __gd32_exti_isr_5_9, DEVICE_GET(exti_gd32),
		    0);
	IRQ_CONNECT(EXTI10_15_IRQn,
		    CONFIG_EXTI_GD32_EXTI10_15_IRQ_PRI,
		    __gd32_exti_isr_10_15, DEVICE_GET(exti_gd32),
		    0);

}
