/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in STM32 MCUs
 *
 * Based on reference manuals:
 *   RM0008 Reference Manual: STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx
 *   and STM32F107xx advanced ARM-based 32-bit MCUs
 * and
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM-based 32-bit MCUs
 *
 * Chapter 10.2: External interrupt/event controller (EXTI)
 *
 */
#include <device.h>
#include <soc.h>
#include <misc/__assert.h>
#include "exti_stm32.h"

#ifdef CONFIG_SOC_SERIES_STM32F0X
#define EXTI_LINES 32
#elif CONFIG_SOC_SERIES_STM32F1X
#define EXTI_LINES 19
#elif CONFIG_SOC_SERIES_STM32F2X
#define EXTI_LINES 23
#elif CONFIG_SOC_STM32F303XC
#define EXTI_LINES 36
#elif CONFIG_SOC_STM32F334X8
#define EXTI_LINES 36
#elif CONFIG_SOC_STM32F373XC
#define EXTI_LINES 29
#elif CONFIG_SOC_SERIES_STM32F4X
#define EXTI_LINES 23
#elif CONFIG_SOC_SERIES_STM32F7X
#define EXTI_LINES 24
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
#define EXTI_LINES 30
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
#define EXTI_LINES 40
#endif

/* 10.3.7 EXTI register map */
struct stm32_exti {
	/* EXTI_IMR */
	u32_t imr;
	/* EXTI_EMR */
	u32_t emr;
	/* EXTI_RTSR */
	u32_t rtsr;
	/* EXTI_FTSR */
	u32_t ftsr;
	/* EXTI_SWIER */
	u32_t swier;
	/* EXTI_PR */
	u32_t pr;
};

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

/*
 * return the proper base addr based on the line number, also we adjust
 * the line number to be relative to the base, we do this here to save
 * a bit of code size
 */
static inline struct stm32_exti *get_exti_addr_adjust_line(int *line)
{
	struct stm32_exti *base = (struct stm32_exti *)EXTI_BASE;

#if EXTI_LINES > 32
	if (*line > 31) {
		*line -= 32;
		return base + 1;
	}
#endif
	return base;
}

void stm32_exti_enable(int line)
{
	volatile struct stm32_exti *exti = get_exti_addr_adjust_line(&line);
	int irqnum = 0;

	exti->imr |= 1 << line;

#ifdef CONFIG_SOC_SERIES_STM32F0X
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
		return;
	}
#elif CONFIG_SOC_SERIES_STM32F1X
	if (line >= 5 && line <= 9) {
		irqnum = EXTI9_5_IRQn;
	} else if (line >= 10 && line <= 15) {
		irqnum = EXTI15_10_IRQn;
	} else {
		/* pins 0..4 are mapped to EXTI0.. EXTI4 */
		irqnum = EXTI0_IRQn + line;
	}
#elif CONFIG_SOC_SERIES_STM32F2X
	if (line >= 5 && line <= 9) {
		irqnum = EXTI9_5_IRQn;
	} else if (line >= 10 && line <= 15) {
		irqnum = EXTI15_10_IRQn;
	} else if (line >= 0 && line <= 4) {
		/* pins 0..4 are mapped to EXTI0.. EXTI4 */
		irqnum = EXTI0_IRQn + line;
	} else {
		switch (line) {
		case 16:
			irqnum = PVD_IRQn;
			break;
		case 17:
			irqnum = RTC_Alarm_IRQn;
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
		}
	}
#elif CONFIG_SOC_SERIES_STM32F3X
	if (line >= 5 && line <= 9) {
		irqnum = EXTI9_5_IRQn;
	} else if (line >= 10 && line <= 15) {
		irqnum = EXTI15_10_IRQn;
	} else {
		/* pins 0..4 are mapped to EXTI0.. EXTI4 */
		irqnum = EXTI0_IRQn + line;
	}
#elif CONFIG_SOC_SERIES_STM32F4X
	if (line >= 5 && line <= 9) {
		irqnum = EXTI9_5_IRQn;
	} else if (line >= 10 && line <= 15) {
		irqnum = EXTI15_10_IRQn;
	} else if (line >= 0 && line <= 4) {
		/* pins 0..4 are mapped to EXTI0.. EXTI4 */
		irqnum = EXTI0_IRQn + line;
	} else {
		switch (line) {
		case 16:
			irqnum = PVD_IRQn;
			break;
		case 17:
			irqnum = RTC_Alarm_IRQn;
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
		}
	}
#elif CONFIG_SOC_SERIES_STM32F7X
	if (line >= 5 && line <= 9) {
		irqnum = EXTI9_5_IRQn;
	} else if (line >= 10 && line <= 15) {
		irqnum = EXTI15_10_IRQn;
	} else if (line >= 0 && line <= 4) {
		/* pins 0..4 are mapped to EXTI0.. EXTI4 */
		irqnum = EXTI0_IRQn + line;
	} else {
		switch (line) {
		case 16:
			irqnum = PVD_IRQn;
			break;
		case 17:
			irqnum = RTC_Alarm_IRQn;
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
		case 23:
			irqnum = LPTIM1_IRQn;
			break;
		}
	}
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
	if (line >= 4 && line <= 15) {
		irqnum = EXTI4_15_IRQn;
	}	else if (line >= 2  && line <= 3) {
		irqnum = EXTI2_3_IRQn;
	}	else if (line >= 0  && line <= 1) {
		irqnum = EXTI0_1_IRQn;
	} else {
		/* > 15 are not mapped on an IRQ */
		/*
		 * On STM32L0X, this function also support enabling EXTI
		 * lines that are not connected to an IRQ. This might be used
		 * by other drivers or boards, to allow the device wakeup on
		 * some non-GPIO signals.
		 */
		return;
	}
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	if (line >= 5 && line <= 9) {
		irqnum = EXTI9_5_IRQn;
	} else if (line >= 10 && line <= 15) {
		irqnum = EXTI15_10_IRQn;
	} else if (line < 5) {
		/* pins 0..4 are mapped to EXTI0.. EXTI4 */
		irqnum = EXTI0_IRQn + line;
	} else {/* > 15 are not mapped on an IRQ */
		/*
		 * On STM32L4X, this function also support enabling EXTI
		 * lines that are not connected to an IRQ. This might be used
		 * by other drivers or boards, to allow the device wakeup on
		 * some non-GPIO signals.
		 */
		return;
	}
#else
	#error "Unknown STM32 SoC"
#endif

	irq_enable(irqnum);
}

void stm32_exti_disable(int line)
{
	volatile struct stm32_exti *exti = get_exti_addr_adjust_line(&line);

	exti->imr &= ~(1 << line);
}

/**
 * @brief check if interrupt is pending
 *
 * @param line line number
 */
static inline int stm32_exti_is_pending(int line)
{
	volatile struct stm32_exti *exti = get_exti_addr_adjust_line(&line);

	return (exti->pr & (1 << line)) ? 1 : 0;
}

/**
 * @brief clear pending interrupt bit
 *
 * @param line line number
 */
static inline void stm32_exti_clear_pending(int line)
{
	volatile struct stm32_exti *exti = get_exti_addr_adjust_line(&line);

	exti->pr = 1 << line;
}

void stm32_exti_trigger(int line, int trigger)
{
	volatile struct stm32_exti *exti = get_exti_addr_adjust_line(&line);

	if (trigger & STM32_EXTI_TRIG_RISING) {
		exti->rtsr |= 1 << line;
	}

	if (trigger & STM32_EXTI_TRIG_FALLING) {
		exti->ftsr |= 1 << line;
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

static inline void __stm32_exti_isr_17(void *arg)
{
	__stm32_exti_isr(17, 18, arg);
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
#endif /* CONFIG_SOC_SERIES_STM32{F4X F7X, F2X} */
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
void stm32_exti_set_callback(int line, stm32_exti_callback_t cb, void *arg)
{
	struct device *dev = DEVICE_GET(exti_stm32);
	struct stm32_exti_data *data = dev->driver_data;

	__ASSERT(data->cb[line].cb == NULL,
		"EXTI %d callback already registered", line);

	data->cb[line].cb = cb;
	data->cb[line].data = arg;
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

#ifdef CONFIG_SOC_SERIES_STM32F0X
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
#elif CONFIG_SOC_SERIES_STM32F1X
	IRQ_CONNECT(EXTI0_IRQn,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI2_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
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
#elif CONFIG_SOC_SERIES_STM32F2X
	IRQ_CONNECT(EXTI0_IRQn,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI2_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
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
	IRQ_CONNECT(PVD_IRQn,
		CONFIG_EXTI_STM32_PVD_IRQ_PRI,
		__stm32_exti_isr_16, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(RTC_Alarm_IRQn,
		CONFIG_EXTI_STM32_RTC_ALARM_IRQ_PRI,
		__stm32_exti_isr_17, DEVICE_GET(exti_stm32),
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
#elif CONFIG_SOC_SERIES_STM32F3X
	IRQ_CONNECT(EXTI0_IRQn,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI2_TSC_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
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
#elif CONFIG_SOC_SERIES_STM32F4X
	IRQ_CONNECT(EXTI0_IRQn,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI2_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
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
	IRQ_CONNECT(PVD_IRQn,
		CONFIG_EXTI_STM32_PVD_IRQ_PRI,
		__stm32_exti_isr_16, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(RTC_Alarm_IRQn,
		CONFIG_EXTI_STM32_RTC_ALARM_IRQ_PRI,
		__stm32_exti_isr_17, DEVICE_GET(exti_stm32),
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
#elif CONFIG_SOC_SERIES_STM32F7X
	IRQ_CONNECT(EXTI0_IRQn,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI2_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
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
	IRQ_CONNECT(PVD_IRQn,
		CONFIG_EXTI_STM32_PVD_IRQ_PRI,
		__stm32_exti_isr_16, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(RTC_Alarm_IRQn,
		CONFIG_EXTI_STM32_RTC_ALARM_IRQ_PRI,
		__stm32_exti_isr_17, DEVICE_GET(exti_stm32),
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
	IRQ_CONNECT(LPTIM1_IRQn,
		CONFIG_EXTI_STM32_LPTIM1_IRQ_PRI,
		__stm32_exti_isr_23, DEVICE_GET(exti_stm32),
		0);
#elif defined(CONFIG_SOC_SERIES_STM32L0X)
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
#elif defined(CONFIG_SOC_SERIES_STM32L4X)
	IRQ_CONNECT(EXTI0_IRQn,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI1_IRQn,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(EXTI2_IRQn,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
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
#endif
}
