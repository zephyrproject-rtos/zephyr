/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in STM32 MCUs
 *
 * Driver is currently implemented to support following EXTI lines
 * STM32F1/STM32F3: Lines 0 to 15. Lines > 15 not supported
 * STM32F0/STM32L0/STM32L4/STM32L5/STM32G0/STM32G4: Lines 0 to 15.
 * Lines > 15 are not mapped on an IRQ
 * STM32F2/STM32F4: Lines 0 to 15, 16, 17 18, 21 and 22. Others not supported
 * STM32F7: Lines 0 to 15, 16, 17 18, 21, 22 and 23. Others not supported
 *
 */

#define EXTI_NODE DT_INST(0, st_stm32_exti)

#include <zephyr/device.h>
#include <soc.h>
#include <stm32_ll_exti.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/interrupt_controller/exti_stm32.h>

#include "stm32_hsem.h"

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
    defined(CONFIG_SOC_SERIES_STM32L0X) || \
    defined(CONFIG_SOC_SERIES_STM32G0X)
const IRQn_Type exti_irq_table[] = {
	EXTI0_1_IRQn, EXTI0_1_IRQn, EXTI2_3_IRQn, EXTI2_3_IRQn,
	EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn,
	EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn,
	EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn, EXTI4_15_IRQn
};
#elif defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32L1X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
const IRQn_Type exti_irq_table[] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
	EXTI4_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn
};
#elif defined(CONFIG_SOC_SERIES_STM32F3X)
const IRQn_Type exti_irq_table[] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_TSC_IRQn, EXTI3_IRQn,
	EXTI4_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn
};
#elif defined(CONFIG_SOC_STM32F410RX) /* STM32F410RX has no OTG_FS_WKUP_IRQn */
const IRQn_Type exti_irq_table[] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
	EXTI4_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	PVD_IRQn, 0xFF, 0xFF, 0xFF,
	0xFF, TAMP_STAMP_IRQn, RTC_WKUP_IRQn
};
#elif defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X)
const IRQn_Type exti_irq_table[] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
	EXTI4_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	PVD_IRQn, 0xFF, OTG_FS_WKUP_IRQn, 0xFF,
	0xFF, TAMP_STAMP_IRQn, RTC_WKUP_IRQn
};
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
const IRQn_Type exti_irq_table[] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
	EXTI4_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
	EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
	PVD_IRQn, 0xFF, OTG_FS_WKUP_IRQn, 0xFF,
	0xFF, TAMP_STAMP_IRQn, RTC_WKUP_IRQn, LPTIM1_IRQn
};
#elif defined(CONFIG_SOC_SERIES_STM32MP1X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
const IRQn_Type exti_irq_table[] = {
	EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn,
	EXTI4_IRQn, EXTI5_IRQn, EXTI6_IRQn, EXTI7_IRQn,
	EXTI8_IRQn, EXTI9_IRQn, EXTI10_IRQn, EXTI11_IRQn,
	EXTI12_IRQn, EXTI13_IRQn, EXTI14_IRQn, EXTI15_IRQn
};
#endif

/* wrapper for user callback */
struct __exti_cb {
	stm32_exti_callback_t cb;
	void *data;
};

/* driver data */
struct stm32_exti_data {
	/* per-line callbacks */
	struct __exti_cb cb[ARRAY_SIZE(exti_irq_table)];
};

void stm32_exti_enable(int line)
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
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
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
#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
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
 * Check EXTI lines in range @min @max for pending interrupts
 *
 * @param arg isr argument
 * @param min low end of EXTI# range
 * @param max low end of EXTI# range
 */
static void __stm32_exti_isr(int min, int max, const struct device *dev)
{
	struct stm32_exti_data *data = dev->data;
	int line;

	/* see which bits are set */
	for (line = min; line < max; line++) {
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

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X)
static inline void __stm32_exti_isr_0_1(const struct device *dev)
{
	__stm32_exti_isr(0, 2, dev);
}

static inline void __stm32_exti_isr_2_3(const struct device *dev)
{
	__stm32_exti_isr(2, 4, dev);
}

static inline void __stm32_exti_isr_4_15(const struct device *dev)
{
	__stm32_exti_isr(4, 16, dev);
}

#else
static inline void __stm32_exti_isr_0(const struct device *dev)
{
	__stm32_exti_isr(0, 1, dev);
}

static inline void __stm32_exti_isr_1(const struct device *dev)
{
	__stm32_exti_isr(1, 2, dev);
}

static inline void __stm32_exti_isr_2(const struct device *dev)
{
	__stm32_exti_isr(2, 3, dev);
}

static inline void __stm32_exti_isr_3(const struct device *dev)
{
	__stm32_exti_isr(3, 4, dev);
}

static inline void __stm32_exti_isr_4(const struct device *dev)
{
	__stm32_exti_isr(4, 5, dev);
}

#if defined(CONFIG_SOC_SERIES_STM32MP1X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X)
static inline void __stm32_exti_isr_5(const struct device *dev)
{
	__stm32_exti_isr(5, 6, dev);
}

static inline void __stm32_exti_isr_6(const struct device *dev)
{
	__stm32_exti_isr(6, 7, dev);
}

static inline void __stm32_exti_isr_7(const struct device *dev)
{
	__stm32_exti_isr(7, 8, dev);
}

static inline void __stm32_exti_isr_8(const struct device *dev)
{
	__stm32_exti_isr(8, 9, dev);
}

static inline void __stm32_exti_isr_9(const struct device *dev)
{
	__stm32_exti_isr(9, 10, dev);
}

static inline void __stm32_exti_isr_10(const struct device *dev)
{
	__stm32_exti_isr(10, 11, dev);
}

static inline void __stm32_exti_isr_11(const struct device *dev)
{
	__stm32_exti_isr(11, 12, dev);
}

static inline void __stm32_exti_isr_12(const struct device *dev)
{
	__stm32_exti_isr(12, 13, dev);
}

static inline void __stm32_exti_isr_13(const struct device *dev)
{
	__stm32_exti_isr(13, 14, dev);
}

static inline void __stm32_exti_isr_14(const struct device *dev)
{
	__stm32_exti_isr(14, 15, dev);
}

static inline void __stm32_exti_isr_15(const struct device *dev)
{
	__stm32_exti_isr(15, 16, dev);
}
#endif

static inline void __stm32_exti_isr_9_5(const struct device *dev)
{
	__stm32_exti_isr(5, 10, dev);
}

static inline void __stm32_exti_isr_15_10(const struct device *dev)
{
	__stm32_exti_isr(10, 16, dev);
}

#if defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32MP1X)
static inline void __stm32_exti_isr_16(const struct device *dev)
{
	__stm32_exti_isr(16, 17, dev);
}

static inline void __stm32_exti_isr_18(const struct device *dev)
{
	__stm32_exti_isr(18, 19, dev);
}

static inline void __stm32_exti_isr_21(const struct device *dev)
{
	__stm32_exti_isr(21, 22, dev);
}

static inline void __stm32_exti_isr_22(const struct device *dev)
{
	__stm32_exti_isr(22, 23, dev);
}
#endif
#if defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32MP1X)
static inline void __stm32_exti_isr_23(const struct device *dev)
{
	__stm32_exti_isr(23, 24, dev);
}
#endif
#endif /* CONFIG_SOC_SERIES_STM32F0X */

static void __stm32_exti_connect_irqs(const struct device *dev);

/**
 * @brief initialize EXTI device driver
 */
static int stm32_exti_init(const struct device *dev)
{
	__stm32_exti_connect_irqs(dev);

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

/**
 * @brief connect all interrupts
 */
static void __stm32_exti_connect_irqs(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
	defined(CONFIG_SOC_SERIES_STM32L0X) || \
	defined(CONFIG_SOC_SERIES_STM32G0X)
	IRQ_CONNECT(EXTI0_1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_0_IRQ_PRI,
		__stm32_exti_isr_0_1, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI2_3_IRQn,
		CONFIG_EXTI_STM32_EXTI3_2_IRQ_PRI,
		__stm32_exti_isr_2_3, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI4_15_IRQn,
		CONFIG_EXTI_STM32_EXTI15_4_IRQ_PRI,
		__stm32_exti_isr_4_15, DEVICE_DT_GET(EXTI_NODE),
		0);
#elif defined(CONFIG_SOC_SERIES_STM32F1X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F3X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32H7X) || \
	defined(CONFIG_SOC_SERIES_STM32L1X) || \
	defined(CONFIG_SOC_SERIES_STM32L4X) || \
	defined(CONFIG_SOC_SERIES_STM32L5X) || \
	defined(CONFIG_SOC_SERIES_STM32MP1X) || \
	defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBX) || \
	defined(CONFIG_SOC_SERIES_STM32G4X) || \
	defined(CONFIG_SOC_SERIES_STM32WLX)
	IRQ_CONNECT(EXTI0_IRQn,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_DT_GET(EXTI_NODE),
		0);
#ifdef CONFIG_SOC_SERIES_STM32F3X
	IRQ_CONNECT(EXTI2_TSC_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_DT_GET(EXTI_NODE),
		0);
#else
	IRQ_CONNECT(EXTI2_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_DT_GET(EXTI_NODE),
		0);
#endif /* CONFIG_SOC_SERIES_STM32F3X */
	IRQ_CONNECT(EXTI3_IRQn,
		CONFIG_EXTI_STM32_EXTI3_IRQ_PRI,
		__stm32_exti_isr_3, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI4_IRQn,
		CONFIG_EXTI_STM32_EXTI4_IRQ_PRI,
		__stm32_exti_isr_4, DEVICE_DT_GET(EXTI_NODE),
		0);
#if !defined(CONFIG_SOC_SERIES_STM32MP1X) && \
	!defined(CONFIG_SOC_SERIES_STM32L5X) && \
	!defined(CONFIG_SOC_SERIES_STM32U5X)
	IRQ_CONNECT(EXTI9_5_IRQn,
		CONFIG_EXTI_STM32_EXTI9_5_IRQ_PRI,
		__stm32_exti_isr_9_5, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI15_10_IRQn,
		CONFIG_EXTI_STM32_EXTI15_10_IRQ_PRI,
		__stm32_exti_isr_15_10, DEVICE_DT_GET(EXTI_NODE),
		0);
#else
	IRQ_CONNECT(EXTI5_IRQn,
		CONFIG_EXTI_STM32_EXTI5_IRQ_PRI,
		__stm32_exti_isr_5, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI6_IRQn,
		CONFIG_EXTI_STM32_EXTI6_IRQ_PRI,
		__stm32_exti_isr_6, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI7_IRQn,
		CONFIG_EXTI_STM32_EXTI7_IRQ_PRI,
		__stm32_exti_isr_7, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI8_IRQn,
		CONFIG_EXTI_STM32_EXTI8_IRQ_PRI,
		__stm32_exti_isr_8, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI9_IRQn,
		CONFIG_EXTI_STM32_EXTI9_IRQ_PRI,
		__stm32_exti_isr_9, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI10_IRQn,
		CONFIG_EXTI_STM32_EXTI10_IRQ_PRI,
		__stm32_exti_isr_10, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI11_IRQn,
		CONFIG_EXTI_STM32_EXTI11_IRQ_PRI,
		__stm32_exti_isr_11, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI12_IRQn,
		CONFIG_EXTI_STM32_EXTI12_IRQ_PRI,
		__stm32_exti_isr_12, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI13_IRQn,
		CONFIG_EXTI_STM32_EXTI13_IRQ_PRI,
		__stm32_exti_isr_13, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI14_IRQn,
		CONFIG_EXTI_STM32_EXTI14_IRQ_PRI,
		__stm32_exti_isr_14, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(EXTI15_IRQn,
		CONFIG_EXTI_STM32_EXTI15_IRQ_PRI,
		__stm32_exti_isr_15, DEVICE_DT_GET(EXTI_NODE),
		0);
#endif /* CONFIG_SOC_SERIES_STM32MP1X || CONFIG_SOC_SERIES_STM32L5X */

#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
	defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X)
	IRQ_CONNECT(PVD_IRQn,
		CONFIG_EXTI_STM32_PVD_IRQ_PRI,
		__stm32_exti_isr_16, DEVICE_DT_GET(EXTI_NODE),
		0);
#if !defined(CONFIG_SOC_STM32F410RX)
	IRQ_CONNECT(OTG_FS_WKUP_IRQn,
		CONFIG_EXTI_STM32_OTG_FS_WKUP_IRQ_PRI,
		__stm32_exti_isr_18, DEVICE_DT_GET(EXTI_NODE),
		0);
#endif
	IRQ_CONNECT(TAMP_STAMP_IRQn,
		CONFIG_EXTI_STM32_TAMP_STAMP_IRQ_PRI,
		__stm32_exti_isr_21, DEVICE_DT_GET(EXTI_NODE),
		0);
	IRQ_CONNECT(RTC_WKUP_IRQn,
		CONFIG_EXTI_STM32_RTC_WKUP_IRQ_PRI,
		__stm32_exti_isr_22, DEVICE_DT_GET(EXTI_NODE),
		0);
#endif
#if CONFIG_SOC_SERIES_STM32F7X
	IRQ_CONNECT(LPTIM1_IRQn,
		CONFIG_EXTI_STM32_LPTIM1_IRQ_PRI,
		__stm32_exti_isr_23, DEVICE_DT_GET(EXTI_NODE),
		0);
#endif /* CONFIG_SOC_SERIES_STM32F7X */
#endif
}
