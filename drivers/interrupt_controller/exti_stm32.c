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
 * STM32F0/STM32L0/STM32L4: Lines 0 to 15. Lines > 15 are not mapped on an IRQ
 * STM32F2/STM32F4: Lines 0 to 15, 16, 17 18, 21 and 22. Others not supported
 * STM32F7: Lines 0 to 15, 16, 17 18, 21, 22 and 23. Others not supported
 *
 */
#include <device.h>
#include <soc.h>
#include <misc/__assert.h>
#include "exti_stm32.h"

#if defined(CONFIG_SOC_SERIES_STM32F0X)
#define EXTI_LINES 32
#elif defined(CONFIG_SOC_SERIES_STM32F1X)
#define EXTI_LINES 19
#elif defined(CONFIG_SOC_SERIES_STM32F2X)
#define EXTI_LINES 23
#elif defined(CONFIG_SOC_STM32F302X8)
#define EXTI_LINES 36
#elif defined(CONFIG_SOC_STM32F303XC)
#define EXTI_LINES 36
#elif defined(CONFIG_SOC_STM32F334X8)
#define EXTI_LINES 36
#elif defined(CONFIG_SOC_STM32F373XC)
#define EXTI_LINES 29
#elif defined(CONFIG_SOC_SERIES_STM32F4X)
#define EXTI_LINES 23
#elif defined(CONFIG_SOC_SERIES_STM32F7X)
#define EXTI_LINES 24
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
#define EXTI_LINES 30
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
#define EXTI_LINES 40
#endif

/* wrapper for user callback */
struct __exti_cb {
	stm32_exti_callback_t cb;
	void *data;
};

/* driver data */
struct stm32_exti_data {
	/* per-line callbacks */
	struct __exti_cb cb[EXTI_LINES];
};

int stm32_exti_enable(int line)
{
	int irqnum = 0;

	if (line < 32) {
		LL_EXTI_EnableIT_0_31(1 << line);
	} else {
#if EXTI_LINES > 32
		LL_EXTI_EnableIT_32_63(1 << (line - 32));
#else
		__ASSERT_NO_MSG(line);
#endif
	}

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
    defined(CONFIG_SOC_SERIES_STM32L0X)
	if (line >= 4 && line <= 15) {
		irqnum = EXTI4_15_IRQn;
	}	else if (line >= 2  && line <= 3) {
		irqnum = EXTI2_3_IRQn;
	}	else if (line >= 0  && line <= 1) {
		irqnum = EXTI0_1_IRQn;
	} else {
		/* > 15 are not mapped on an IRQ */
		/*
		 * On STM32F0X, this function also support enabling EXTI
		 * lines that are not connected to an IRQ. This might be used
		 * by other drivers or boards, to allow the device wakeup on
		 * some non-GPIO signals.
		 */
		return 0;
	}
#elif defined(CONFIG_SOC_SERIES_STM32F1X) || \
      defined(CONFIG_SOC_SERIES_STM32F2X) || \
      defined(CONFIG_SOC_SERIES_STM32F3X) || \
      defined(CONFIG_SOC_SERIES_STM32F4X) || \
      defined(CONFIG_SOC_SERIES_STM32F7X) || \
      defined(CONFIG_SOC_SERIES_STM32L4X)
	if (line >= 5 && line <= 9) {
		irqnum = EXTI9_5_IRQn;
	} else if (line >= 10 && line <= 15) {
		irqnum = EXTI15_10_IRQn;
	} else if (line >= 0 && line <= 4) {
		/* pins 0..4 are mapped to EXTI0.. EXTI4 */
		irqnum = EXTI0_IRQn + line;
	} else {
		switch (line) {
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
    defined(CONFIG_SOC_SERIES_STM32F4X) || \
    defined(CONFIG_SOC_SERIES_STM32F7X)
		case 16:
			irqnum = PVD_IRQn;
			break;
		case 18:
			irqnum = OTG_FS_WKUP_IRQn;
			break;
		case 21:
			irqnum = TAMP_STAMP_IRQn;
			break;
		case 22:
			irqnum = RTC_WKUP_IRQn;
			break;
#endif
#if defined(CONFIG_SOC_SERIES_STM32F7X)
		case 23:
			irqnum = LPTIM1_IRQn;
			break;
#endif
		default:
			/* No IRQ associated to this line */
#if defined(CONFIG_SOC_SERIES_STM32L4X)
			/* > 15 are not mapped on an IRQ */
			/*
			 * On STM32L4X, this function also support enabling EXTI
			 * lines that are not connected to an IRQ. This might be used
			 * by other drivers or boards, to allow the device wakeup on
			 * some non-GPIO signals.
			 */
			return 0;
#else
			return -ENOTSUP;
#endif /* CONFIG_SOC_SERIES_STM32L4X */
		}
	}
#else
	#error "Unknown STM32 SoC"
#endif

	irq_enable(irqnum);

	return 0;
}

void stm32_exti_disable(int line)
{
	if (line < 32) {
		LL_EXTI_DisableIT_0_31(1 << line);
	} else {
#if EXTI_LINES > 32
		LL_EXTI_DisableIT_32_63(1 << (line - 32));
#else
		__ASSERT_NO_MSG(line);
#endif
	}
}

/**
 * @brief check if interrupt is pending
 *
 * @param line line number
 */
static inline int stm32_exti_is_pending(int line)
{
	if (line < 32) {
		return LL_EXTI_IsActiveFlag_0_31(1 << line);
	} else {
#if EXTI_LINES > 32
		return LL_EXTI_IsActiveFlag_32_63(1 << (line - 32));
#else
		__ASSERT_NO_MSG(line);
		return 0;
#endif
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
		LL_EXTI_ClearFlag_0_31(1 << line);
	} else {
#if EXTI_LINES > 32
		LL_EXTI_ClearFlag_32_63(1 << (line - 32));
#else
		__ASSERT_NO_MSG(line);
#endif
	}
}

void stm32_exti_trigger(int line, int trigger)
{
	if (trigger & STM32_EXTI_TRIG_RISING) {
		if (line < 32) {
			LL_EXTI_EnableRisingTrig_0_31(1 << line);
		} else {
#if EXTI_LINES > 32
			LL_EXTI_EnableRisingTrig_32_63(1 << (line - 32));
#else
			__ASSERT_NO_MSG(line);
#endif
		}
	}

	if (trigger & STM32_EXTI_TRIG_FALLING) {
		if (line < 32) {
			LL_EXTI_EnableFallingTrig_0_31(1 << line);
		} else {
#if EXTI_LINES > 32
			LL_EXTI_EnableFallingTrig_32_63(1 << (line - 32));
#else
			__ASSERT_NO_MSG(line);
#endif
		}
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
static void __stm32_exti_isr(int min, int max, void *arg)
{
	struct device *dev = arg;
	struct stm32_exti_data *data = dev->driver_data;
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

#if defined(CONFIG_SOC_SERIES_STM32F0X) || defined(CONFIG_SOC_SERIES_STM32L0X)
static inline void __stm32_exti_isr_0_1(void *arg)
{
	__stm32_exti_isr(0, 2, arg);
}

static inline void __stm32_exti_isr_2_3(void *arg)
{
	__stm32_exti_isr(2, 4, arg);
}

static inline void __stm32_exti_isr_4_15(void *arg)
{
	__stm32_exti_isr(4, 16, arg);
}

#else
static inline void __stm32_exti_isr_0(void *arg)
{
	__stm32_exti_isr(0, 1, arg);
}

static inline void __stm32_exti_isr_1(void *arg)
{
	__stm32_exti_isr(1, 2, arg);
}

static inline void __stm32_exti_isr_2(void *arg)
{
	__stm32_exti_isr(2, 3, arg);
}

static inline void __stm32_exti_isr_3(void *arg)
{
	__stm32_exti_isr(3, 4, arg);
}

static inline void __stm32_exti_isr_4(void *arg)
{
	__stm32_exti_isr(4, 5, arg);
}

static inline void __stm32_exti_isr_9_5(void *arg)
{
	__stm32_exti_isr(5, 10, arg);
}

static inline void __stm32_exti_isr_15_10(void *arg)
{
	__stm32_exti_isr(10, 16, arg);
}

#if defined(CONFIG_SOC_SERIES_STM32F4X) || \
	defined(CONFIG_SOC_SERIES_STM32F7X) || \
	defined(CONFIG_SOC_SERIES_STM32F2X)
static inline void __stm32_exti_isr_16(void *arg)
{
	__stm32_exti_isr(16, 17, arg);
}

static inline void __stm32_exti_isr_18(void *arg)
{
	__stm32_exti_isr(18, 19, arg);
}

static inline void __stm32_exti_isr_21(void *arg)
{
	__stm32_exti_isr(21, 22, arg);
}

static inline void __stm32_exti_isr_22(void *arg)
{
	__stm32_exti_isr(22, 23, arg);
}
#endif
#ifdef CONFIG_SOC_SERIES_STM32F7X
static inline void __stm32_exti_isr_23(void *arg)
{
	__stm32_exti_isr(23, 24, arg);
}
#endif /* CONFIG_SOC_SERIES_STM32F7X */
#endif /* CONFIG_SOC_SERIES_STM32F0X */

static void __stm32_exti_connect_irqs(struct device *dev);

/**
 * @brief initialize EXTI device driver
 */
static int stm32_exti_init(struct device *dev)
{
	__stm32_exti_connect_irqs(dev);

	return 0;
}

static struct stm32_exti_data exti_data;
DEVICE_INIT(exti_stm32, STM32_EXTI_NAME, stm32_exti_init,
	    &exti_data, NULL,
	    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/**
 * @brief set & unset for the interrupt callbacks
 */
int stm32_exti_set_callback(int line, int port, stm32_exti_callback_t cb,
				void *arg)
{
	struct device *dev = DEVICE_GET(exti_stm32);
	struct stm32_exti_data *data = dev->driver_data;

	if (data->cb[line].cb) {
		return -EBUSY;
	}

	data->cb[line].cb = cb;
	data->cb[line].data = arg;

	return 0;
}

void stm32_exti_unset_callback(int line)
{
	struct device *dev = DEVICE_GET(exti_stm32);
	struct stm32_exti_data *data = dev->driver_data;

	data->cb[line].cb = NULL;
	data->cb[line].data = NULL;
}

/**
 * @brief connect all interrupts
 */
static void __stm32_exti_connect_irqs(struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_SOC_SERIES_STM32F0X) || \
    defined(CONFIG_SOC_SERIES_STM32L0X)
	IRQ_CONNECT(EXTI0_1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_0_IRQ_PRI,
		__stm32_exti_isr_0_1, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI2_3_IRQn,
		CONFIG_EXTI_STM32_EXTI3_2_IRQ_PRI,
		__stm32_exti_isr_2_3, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI4_15_IRQn,
		CONFIG_EXTI_STM32_EXTI15_4_IRQ_PRI,
		__stm32_exti_isr_4_15, DEVICE_GET(exti_stm32),
		0);
#elif defined(CONFIG_SOC_SERIES_STM32F1X) || \
      defined(CONFIG_SOC_SERIES_STM32F2X) || \
      defined(CONFIG_SOC_SERIES_STM32F3X) || \
      defined(CONFIG_SOC_SERIES_STM32F4X) || \
      defined(CONFIG_SOC_SERIES_STM32F7X) || \
      defined(CONFIG_SOC_SERIES_STM32L4X)
	IRQ_CONNECT(EXTI0_IRQn,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_GET(exti_stm32),
		0);
#ifdef CONFIG_SOC_SERIES_STM32F3X
	IRQ_CONNECT(EXTI2_TSC_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
#else
	IRQ_CONNECT(EXTI2_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
#endif /* CONFIG_SOC_SERIES_STM32F3X */
	IRQ_CONNECT(EXTI3_IRQn,
		CONFIG_EXTI_STM32_EXTI3_IRQ_PRI,
		__stm32_exti_isr_3, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI4_IRQn,
		CONFIG_EXTI_STM32_EXTI4_IRQ_PRI,
		__stm32_exti_isr_4, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI9_5_IRQn,
		CONFIG_EXTI_STM32_EXTI9_5_IRQ_PRI,
		__stm32_exti_isr_9_5, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI15_10_IRQn,
		CONFIG_EXTI_STM32_EXTI15_10_IRQ_PRI,
		__stm32_exti_isr_15_10, DEVICE_GET(exti_stm32),
		0);
#if defined(CONFIG_SOC_SERIES_STM32F2X) || \
      defined(CONFIG_SOC_SERIES_STM32F4X) || \
      defined(CONFIG_SOC_SERIES_STM32F7X)
	IRQ_CONNECT(PVD_IRQn,
		CONFIG_EXTI_STM32_PVD_IRQ_PRI,
		__stm32_exti_isr_16, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(OTG_FS_WKUP_IRQn,
		CONFIG_EXTI_STM32_OTG_FS_WKUP_IRQ_PRI,
		__stm32_exti_isr_18, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(TAMP_STAMP_IRQn,
		CONFIG_EXTI_STM32_TAMP_STAMP_IRQ_PRI,
		__stm32_exti_isr_21, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(RTC_WKUP_IRQn,
		CONFIG_EXTI_STM32_RTC_WKUP_IRQ_PRI,
		__stm32_exti_isr_22, DEVICE_GET(exti_stm32),
		0);
#endif
#if CONFIG_SOC_SERIES_STM32F7X
	IRQ_CONNECT(LPTIM1_IRQn,
		CONFIG_EXTI_STM32_LPTIM1_IRQ_PRI,
		__stm32_exti_isr_23, DEVICE_GET(exti_stm32),
		0);
#endif /* CONFIG_SOC_SERIES_STM32F7X */
#endif
}
