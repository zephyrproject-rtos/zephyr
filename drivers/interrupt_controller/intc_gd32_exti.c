/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_exti

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gd32_exti.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>

#include <gd32_exti.h>

/** Unsupported line indicator */
#define EXTI_NOTSUP 0xFFU

/** Number of EXTI lines. */
#define NUM_EXTI_LINES DT_INST_PROP(0, num_lines)

/** @brief EXTI line ranges hold by a single ISR */
struct gd32_exti_range {
	/** Start of the range */
	uint8_t min;
	/** End of the range */
	uint8_t max;
};

/** @brief EXTI line interrupt callback. */
struct gd32_cb_data {
	/** Callback function */
	gd32_exti_cb_t cb;
	/** User data. */
	void *user;
};

/** EXTI driver data. */
struct gd32_exti_data {
	/** Array of callbacks. */
	struct gd32_cb_data cbs[NUM_EXTI_LINES];
};

#ifdef CONFIG_GPIO_GD32
static const struct gd32_exti_range line0_range = {0U, 0U};
static const struct gd32_exti_range line1_range = {1U, 1U};
static const struct gd32_exti_range line2_range = {2U, 2U};
static const struct gd32_exti_range line3_range = {3U, 3U};
static const struct gd32_exti_range line4_range = {4U, 4U};
static const struct gd32_exti_range line5_9_range = {5U, 9U};
static const struct gd32_exti_range line10_15_range = {10U, 15U};
#endif /* CONFIG_GPIO_GD32 */

/** @brief Obtain line IRQ number if enabled. */
#define EXTI_LINE_IRQ_COND(enabled, line) \
	COND_CODE_1(enabled, (DT_INST_IRQ_BY_NAME(0, line, irq)), (EXTI_NOTSUP))

static const uint8_t line2irq[NUM_EXTI_LINES] = {
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line0),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line1),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line2),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line3),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line4),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line5_9),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line5_9),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line5_9),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line5_9),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line5_9),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line10_15),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line10_15),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line10_15),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line10_15),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line10_15),
	EXTI_LINE_IRQ_COND(CONFIG_GPIO_GD32, line10_15),
	EXTI_NOTSUP,
	EXTI_NOTSUP,
	EXTI_NOTSUP,
#ifdef CONFIG_SOC_SERIES_GD32F4XX
	EXTI_NOTSUP,
	EXTI_NOTSUP,
	EXTI_NOTSUP,
	EXTI_NOTSUP,
#endif /* CONFIG_SOC_SERIES_GD32F4XX */
};

__unused static void gd32_exti_isr(const void *isr_data)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct gd32_exti_data *data = dev->data;
	const struct gd32_exti_range *range = isr_data;

	for (uint8_t i = range->min; i <= range->max; i++) {
		if ((EXTI_PD & BIT(i)) != 0U) {
			EXTI_PD = BIT(i);

			if (data->cbs[i].cb != NULL) {
				data->cbs[i].cb(i, data->cbs[i].user);
			}
		}
	}
}

void gd32_exti_enable(uint8_t line)
{
	__ASSERT_NO_MSG(line < NUM_EXTI_LINES);
	__ASSERT_NO_MSG(line2irq[line] != EXTI_NOTSUP);

	EXTI_INTEN |= BIT(line);

	irq_enable(line2irq[line]);
}

void gd32_exti_disable(uint8_t line)
{
	__ASSERT_NO_MSG(line < NUM_EXTI_LINES);
	__ASSERT_NO_MSG(line2irq[line] != EXTI_NOTSUP);

	EXTI_INTEN &= ~BIT(line);
}

void gd32_exti_trigger(uint8_t line, uint8_t trigger)
{
	__ASSERT_NO_MSG(line < NUM_EXTI_LINES);
	__ASSERT_NO_MSG(line2irq[line] != EXTI_NOTSUP);

	if ((trigger & GD32_EXTI_TRIG_RISING) != 0U) {
		EXTI_RTEN |= BIT(line);
	} else {
		EXTI_RTEN &= ~BIT(line);
	}

	if ((trigger & GD32_EXTI_TRIG_FALLING) != 0U) {
		EXTI_FTEN |= BIT(line);
	} else {
		EXTI_FTEN &= ~BIT(line);
	}
}

int gd32_exti_configure(uint8_t line, gd32_exti_cb_t cb, void *user)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct gd32_exti_data *data = dev->data;

	__ASSERT_NO_MSG(line < NUM_EXTI_LINES);
	__ASSERT_NO_MSG(line2irq[line] != EXTI_NOTSUP);

	if ((data->cbs[line].cb != NULL) && (cb != NULL)) {
		return -EALREADY;
	}

	data->cbs[line].cb = cb;
	data->cbs[line].user = user;

	return 0;
}

static int gd32_exti_init(const struct device *dev)
{
#ifdef CONFIG_GPIO_GD32
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, line0, irq),
		    DT_INST_IRQ_BY_NAME(0, line0, priority),
		    gd32_exti_isr, &line0_range, 0);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, line1, irq),
		    DT_INST_IRQ_BY_NAME(0, line1, priority),
		    gd32_exti_isr, &line1_range, 0);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, line2, irq),
		    DT_INST_IRQ_BY_NAME(0, line2, priority),
		    gd32_exti_isr, &line2_range, 0);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, line3, irq),
		    DT_INST_IRQ_BY_NAME(0, line3, priority),
		    gd32_exti_isr, &line3_range, 0);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, line4, irq),
		    DT_INST_IRQ_BY_NAME(0, line4, priority),
		    gd32_exti_isr, &line4_range, 0);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, line5_9, irq),
		    DT_INST_IRQ_BY_NAME(0, line5_9, priority),
		    gd32_exti_isr, &line5_9_range, 0);

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, line10_15, irq),
		    DT_INST_IRQ_BY_NAME(0, line10_15, priority),
		    gd32_exti_isr, &line10_15_range, 0);
#endif /* CONFIG_GPIO_GD32 */

	return 0;
}

static struct gd32_exti_data data;

DEVICE_DT_INST_DEFINE(0, gd32_exti_init, NULL, &data, NULL, PRE_KERNEL_1,
		      CONFIG_INTC_INIT_PRIORITY, NULL);
