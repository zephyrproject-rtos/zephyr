/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2019-23 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in STM32 MCUs
 */

#define EXTI_NODE DT_INST(0, st_stm32_exti)

#include <zephyr/device.h>
#include <soc.h>
#include <stm32_ll_exti.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/interrupt_controller/exti_stm32.h>
#include <zephyr/irq.h>

#include "stm32_hsem.h"

/** @brief EXTI line ranges hold by a single ISR */
struct stm32_exti_range {
	/** Start of the range */
	uint8_t start;
	/** Range length */
	uint8_t len;
};

#define NUM_EXTI_LINES DT_PROP(DT_NODELABEL(exti), num_lines)

static IRQn_Type exti_irq_table[NUM_EXTI_LINES] = {[0 ... NUM_EXTI_LINES - 1] = 0xFF};

/* wrapper for user callback */
struct __exti_cb {
	stm32_exti_callback_t cb;
	void *data;
};

/* driver data */
struct stm32_exti_data {
	/* per-line callbacks */
	struct __exti_cb cb[NUM_EXTI_LINES];
};

void stm32_exti_enable(int line)
{
	int irqnum = 0;

	if (line >= NUM_EXTI_LINES) {
		__ASSERT_NO_MSG(line);
	}

	/* Get matching exti irq provided line thanks to irq_table */
	irqnum = exti_irq_table[line];
	if (irqnum == 0xFF) {
		__ASSERT_NO_MSG(line);
	}

	/* Enable requested line interrupt */
#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_EnableIT_0_31(1 << line);
#else
	LL_EXTI_EnableIT_0_31(1 << line);
#endif

	/* Enable exti irq interrupt */
	irq_enable(irqnum);
}

void stm32_exti_disable(int line)
{
	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	if (line < 32) {
#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
		LL_C2_EXTI_DisableIT_0_31(1 << line);
#else
		LL_EXTI_DisableIT_0_31(1 << line);
#endif
	} else {
		__ASSERT_NO_MSG(line);
	}
	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);
}

/**
 * @brief check if interrupt is pending
 *
 * @param line line number
 */
static inline int stm32_exti_is_pending(int line)
{
	if (line < 32) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
		return (LL_EXTI_IsActiveRisingFlag_0_31(1 << line) ||
			LL_EXTI_IsActiveFallingFlag_0_31(1 << line));
#elif defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
		return LL_C2_EXTI_IsActiveFlag_0_31(1 << line);
#else
		return LL_EXTI_IsActiveFlag_0_31(1 << line);
#endif
	} else {
		__ASSERT_NO_MSG(line);
		return 0;
	}
}

/**
 * @brief clear pending interrupt bit
 *
 * @param line line number
 */
static inline void stm32_exti_clear_pending(int line)
{
	if (line < 32) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32g0_exti)
		LL_EXTI_ClearRisingFlag_0_31(1 << line);
		LL_EXTI_ClearFallingFlag_0_31(1 << line);
#elif defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
		LL_C2_EXTI_ClearFlag_0_31(1 << line);
#else
		LL_EXTI_ClearFlag_0_31(1 << line);
#endif
	} else {
		__ASSERT_NO_MSG(line);
	}
}

void stm32_exti_trigger(int line, int trigger)
{

	if (line >= 32) {
		__ASSERT_NO_MSG(line);
	}

	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	switch (trigger) {
	case STM32_EXTI_TRIG_NONE:
		LL_EXTI_DisableRisingTrig_0_31(1 << line);
		LL_EXTI_DisableFallingTrig_0_31(1 << line);
		break;
	case STM32_EXTI_TRIG_RISING:
		LL_EXTI_EnableRisingTrig_0_31(1 << line);
		LL_EXTI_DisableFallingTrig_0_31(1 << line);
		break;
	case STM32_EXTI_TRIG_FALLING:
		LL_EXTI_EnableFallingTrig_0_31(1 << line);
		LL_EXTI_DisableRisingTrig_0_31(1 << line);
		break;
	case STM32_EXTI_TRIG_BOTH:
		LL_EXTI_EnableRisingTrig_0_31(1 << line);
		LL_EXTI_EnableFallingTrig_0_31(1 << line);
		break;
	default:
		__ASSERT_NO_MSG(trigger);
	}
	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);
}

/**
 * @brief EXTI ISR handler
 *
 * Check EXTI lines in exti_range for pending interrupts
 *
 * @param *exti_range Pointer to a exti_range structure
 */
static void stm32_exti_isr(const void *exti_range)
{
	const struct device *dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;
	const struct stm32_exti_range *range = exti_range;
	int line;

	/* see which bits are set */
	for (int i = 0; i <= range->len; i++) {
		line = range->start + i;
		/* check if interrupt is pending */
		if (stm32_exti_is_pending(line)) {
			/* clear pending interrupt */
			stm32_exti_clear_pending(line);

			/* run callback only if one is registered */
			if (!data->cb[line].cb) {
				continue;
			}

			data->cb[line].cb(line, data->cb[line].data);
		}
	}
}

static void stm32_fill_irq_table(int8_t start, int8_t len, int32_t irqn)
{
	for (int i = 0; i < len; i++) {
		exti_irq_table[start + i] = irqn;
	}
}

/* This macro:
 * - populates line_range_x from line_range dt property
 * - fill exti_irq_table through stm32_fill_irq_table()
 * - calls IRQ_CONNECT for each irq & matching line_range
 */
#define STM32_EXTI_INIT(node_id, interrupts, idx)			\
	static const struct stm32_exti_range line_range_##idx = {	\
		range[2 * idx],						\
		range[2 * idx + 1]					\
	};								\
	stm32_fill_irq_table(line_range_##idx.start,			\
			     line_range_##idx.len,			\
			     DT_IRQ_BY_IDX(node_id, idx, irq));		\
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),			\
		DT_IRQ_BY_IDX(node_id, idx, priority),			\
		stm32_exti_isr, &line_range_##idx,			\
		0);

/**
 * @brief initialize EXTI device driver
 */
static int stm32_exti_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	const uint8_t range[2*NUM_EXTI_LINES] =
					DT_PROP(DT_NODELABEL(exti), line_ranges);
	DT_FOREACH_PROP_ELEM(DT_NODELABEL(exti),
			     interrupt_names,
			     STM32_EXTI_INIT);

	return 0;
}

static struct stm32_exti_data exti_data;
DEVICE_DT_DEFINE(EXTI_NODE, &stm32_exti_init,
		 NULL,
		 &exti_data, NULL,
		 PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,
		 NULL);

/**
 * @brief set & unset for the interrupt callbacks
 */
int stm32_exti_set_callback(int line, stm32_exti_callback_t cb, void *arg)
{
	const struct device *const dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;

	if (data->cb[line].cb == cb && data->cb[line].data == arg) {
		return 0;
	}

	if (data->cb[line].cb) {
		return -EBUSY;
	}

	data->cb[line].cb = cb;
	data->cb[line].data = arg;

	return 0;
}

void stm32_exti_unset_callback(int line)
{
	const struct device *const dev = DEVICE_DT_GET(EXTI_NODE);
	struct stm32_exti_data *data = dev->data;

	data->cb[line].cb = NULL;
	data->cb[line].data = NULL;
}
