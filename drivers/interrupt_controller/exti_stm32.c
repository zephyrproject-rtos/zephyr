/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Driver for External interrupt/event controller in STM32 MCUs
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 10.2: External interrupt/event controller (EXTI)
 *
 */
#include <device.h>
#include <soc.h>
#include <misc/__assert.h>
#include "exti_stm32.h"


/* 10.3.7 EXTI register map */
struct stm32_exti {
	/* EXTI_IMR */
	uint32_t imr;
	/* EXTI_EMR */
	uint32_t emr;
	/* EXTI_RTSR */
	uint32_t rtsr;
	/* EXTI_FTSR */
	uint32_t ftsr;
	/* EXTI_SWIER */
	uint32_t swier;
	/* EXTI_PR */
	uint32_t pr;
};

/* wrapper for user callback */
struct __exti_cb {
	stm32_exti_callback_t cb;
	void *data;
};

#ifdef CONFIG_SOC_SERIES_STM32F1X
#define EXTI_LINES 19
#endif

/* driver data */
struct stm32_exti_data {
	/* per-line callbacks */
	struct __exti_cb cb[EXTI_LINES];
};


#define AS_EXTI(__base_addr)			\
	((struct stm32_exti *)(__base_addr))

void stm32_exti_enable(int line)
{
	volatile struct stm32_exti *exti = AS_EXTI(EXTI_BASE);
	int irqnum;

	exti->imr |= 1 << line;

#ifdef CONFIG_SOC_SERIES_STM32F1X
	if (line >= 5 && line <= 9) {
		irqnum = STM32F1_IRQ_EXTI9_5;
	} else if (line >= 10 && line <= 15) {
		irqnum = STM32F1_IRQ_EXTI15_10;
	} else {
		/* pins 0..4 are mapped to EXTI0.. EXTI4 */
		irqnum = STM32F1_IRQ_EXTI0 + line;
	}
#endif

	irq_enable(irqnum);
}

void stm32_exti_disable(int line)
{
	volatile struct stm32_exti *exti = AS_EXTI(EXTI_BASE);

	exti->imr &= ~(1 << line);
}

/**
 * @brief check if interrupt is pending
 *
 * @param line line number
 */
static inline int stm32_exti_is_pending(int line)
{
	volatile struct stm32_exti *exti = AS_EXTI(EXTI_BASE);

	return (exti->pr & (1 << line)) ? 1 : 0;
}

/**
 * @brief clear pending interrupt bit
 *
 * @param line line number
 */
static inline void stm32_exti_clear_pending(int line)
{
	volatile struct stm32_exti *exti = AS_EXTI(EXTI_BASE);

	exti->pr = 1 << line;
}

void stm32_exti_trigger(int line, int trigger)
{
	volatile struct stm32_exti *exti = AS_EXTI(EXTI_BASE);

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
 * @parram min low end of EXTI# range
 * @parram max low end of EXTI# range
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
	    PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

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
#ifdef CONFIG_SOC_SERIES_STM32F1X
	IRQ_CONNECT(STM32F1_IRQ_EXTI0,
		CONFIG_EXTI_STM32_EXTI0_IRQ_PRI,
		__stm32_exti_isr_0, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(STM32F1_IRQ_EXTI1,
		CONFIG_EXTI_STM32_EXTI1_IRQ_PRI,
		__stm32_exti_isr_1, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(STM32F1_IRQ_EXTI2,
		CONFIG_EXTI_STM32_EXTI2_IRQ_PRI,
		__stm32_exti_isr_2, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(STM32F1_IRQ_EXTI3,
		CONFIG_EXTI_STM32_EXTI3_IRQ_PRI,
		__stm32_exti_isr_3, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(STM32F1_IRQ_EXTI4,
		CONFIG_EXTI_STM32_EXTI4_IRQ_PRI,
		__stm32_exti_isr_4, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(STM32F1_IRQ_EXTI9_5,
		CONFIG_EXTI_STM32_EXTI9_5_IRQ_PRI,
		__stm32_exti_isr_9_5, DEVICE_GET(exti_stm32),
		0);
	IRQ_CONNECT(STM32F1_IRQ_EXTI15_10,
		CONFIG_EXTI_STM32_EXTI15_10_IRQ_PRI,
		__stm32_exti_isr_15_10, DEVICE_GET(exti_stm32),
		0);
#endif
}


