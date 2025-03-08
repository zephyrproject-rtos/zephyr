/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for STM32WB0 GPIO interrupt controller
 *
 * In this file, "EXTI" should be understood as "GPIO interrupt controller".
 * There is no "External interrupt/event controller (EXTI)" in STM32WB0 MCUs.
 */

#define DT_DRV_COMPAT st_stm32wb0_gpio_intc

#include <errno.h>

#include <soc.h>
#include <stm32_ll_system.h>

#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>
#include <zephyr/dt-bindings/pinctrl/stm32-pinctrl-common.h>	/* For PORTA/PORTB defines */

#include "stm32_hsem.h"
#include "intc_exti_stm32_priv.h"

#define INTC_NODE DT_DRV_INST(0)

#define NUM_GPIO_PORTS		(2)
#define NUM_PINS_PER_GPIO_PORT	(16)

#define GPIO_PORT_TABLE_INDEX(port)	\
	DT_PROP_BY_IDX(INTC_NODE, line_ranges, UTIL_X2(port))

/* For good measure only */
#define _NUM_GPIOS_ON_PORT_X(x)	\
	DT_PROP_BY_IDX(INTC_NODE, line_ranges, UTIL_INC(UTIL_X2(x)))
BUILD_ASSERT(DT_PROP_LEN(INTC_NODE, line_ranges) == (2 * NUM_GPIO_PORTS));
BUILD_ASSERT(_NUM_GPIOS_ON_PORT_X(STM32_PORTA) == NUM_PINS_PER_GPIO_PORT);
BUILD_ASSERT(_NUM_GPIOS_ON_PORT_X(STM32_PORTB) == NUM_PINS_PER_GPIO_PORT);
BUILD_ASSERT(GPIO_PORT_TABLE_INDEX(STM32_PORTB) == NUM_PINS_PER_GPIO_PORT);
#undef _NUM_GPIOS_ON_PORT_X

/* wrapper for user callback */
struct gpio_irq_cb_wrp {
	stm32_exti_irq_cb_t fn;
	void *data;
};

/* wrapper for ISR argument block */
struct wb0_gpio_isr_argblock {
	/* LL define for first line on GPIO port
	 * (= least significant bit of the port's defines)
	 */
	uint32_t port_first_line;
	/* Pointer to first element of irq_callbacks_table
	 * array that corresponds to this GPIO line
	 */
	struct gpio_irq_cb_wrp *cb_table;
};

/* driver data */
struct stm32wb0_gpio_intc_data {
	/* per-port user callbacks */
	struct gpio_irq_cb_wrp irq_cb_table[
		NUM_GPIO_PORTS * NUM_PINS_PER_GPIO_PORT];
};

/**
 * @returns the LL_EXTI_LINE_n define for EXTI line number @p linenum
 */
static inline uint32_t linenum_to_ll_exti_line(uint32_t linenum)
{
	return BIT(linenum % 32U);
}

/**
 * @returns EXTI line number for LL_EXTI_LINE_n define
 */
static inline uint32_t ll_exti_line_to_linenum(uint32_t ll_exti_line)
{
	return ll_exti_line % 32U;
}

/**
 * @returns the LL_EXTI_LINE_Pxy define for pin @p pin on GPIO port @p port
 */
static inline uint32_t portpin_to_ll_exti_line(uint32_t port, uint32_t pin)
{
	uint32_t line = (1U << pin);

	if (port == STM32_PORTB) {
		line <<= SYSCFG_IO_DTR_PB0_DT_Pos;
	} else if (port == STM32_PORTA) {
		line <<= SYSCFG_IO_DTR_PA0_DT_Pos;
	} else {
		__ASSERT_NO_MSG(0);
	}

	return line;
}

/**
 * @returns a 32-bit value contaning:
 *	- <5:5> port number (0 = PORTA, 1 = PORTB)
 *	- <4:0> pin number (0~15)
 *
 * The returned value is always between 0~31.
 */
static inline uint32_t ll_exti_line_to_portpin(uint32_t line)
{
	return LOG2(line);
}

/**
 * @brief Retrieves the user callback block for a given line
 */
static struct gpio_irq_cb_wrp *irq_cb_wrp_for_line(uint32_t line)
{
	const struct device *const dev = DEVICE_DT_GET(INTC_NODE);
	struct stm32wb0_gpio_intc_data *const data = dev->data;
	const uint32_t index = ll_exti_line_to_portpin(line);

	return data->irq_cb_table + index;
}

/* Interrupt subroutines */
static void stm32wb0_gpio_isr(const void *userdata)
{
	const struct wb0_gpio_isr_argblock *arg = userdata;
	const struct gpio_irq_cb_wrp *cb_table = arg->cb_table;

	uint32_t line = arg->port_first_line;

	for (uint32_t i = 0; i < NUM_PINS_PER_GPIO_PORT; i++, line <<= 1) {
		if (LL_EXTI_IsActiveFlag(line) != 0) {
			/* clear pending interrupt */
			LL_EXTI_ClearFlag(line);

			/* execute user callback if registered */
			if (cb_table[i].fn != NULL) {
				const uint32_t pin = (1U << i);

				cb_table[i].fn(pin, cb_table[i].data);
			}
		}
	}
}

/**
 * Define the driver data early so that the macro that follows can
 * refer to it directly instead of indirecting through drv->data.
 */
static struct stm32wb0_gpio_intc_data gpio_intc_data;

 /**
  * This macro creates the ISR argument block for the @p pidx GPIO port,
  * connects the ISR to the interrupt line and enable IRQ at NVIC level.
  *
  * @param node	GPIO INTC device tree node
  * @param pidx	GPIO port index
  * @param plin	LL define of first line on GPIO port
  */
#define INIT_INTC_PORT_INNER(node, pidx, plin)	\
	static const struct wb0_gpio_isr_argblock			\
		port ##pidx ##_argblock = {				\
			.port_first_line = plin,			\
			.cb_table = gpio_intc_data.irq_cb_table +	\
				GPIO_PORT_TABLE_INDEX(pidx)		\
		};							\
									\
	IRQ_CONNECT(DT_IRQN_BY_IDX(node, pidx),				\
		DT_IRQ_BY_IDX(node, pidx, priority),			\
		stm32wb0_gpio_isr, &port ##pidx ##_argblock, 0);	\
									\
	irq_enable(DT_IRQN_BY_IDX(node, pidx))

#define STM32WB0_INIT_INTC_FOR_PORT(_PORT)				\
	INIT_INTC_PORT_INNER(INTC_NODE,					\
		STM32_PORT ##_PORT, LL_EXTI_LINE_P ##_PORT ## 0)

/**
 * @brief Initializes the GPIO interrupt controller driver
 */
static int stm32wb0_gpio_intc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	STM32WB0_INIT_INTC_FOR_PORT(A);

	STM32WB0_INIT_INTC_FOR_PORT(B);

	return 0;
}

DEVICE_DT_DEFINE(INTC_NODE, &stm32wb0_gpio_intc_init,
		 NULL, &gpio_intc_data, NULL, PRE_KERNEL_1,
		 CONFIG_INTC_INIT_PRIORITY, NULL);

/**
 * @brief STM32 GPIO interrupt controller API implementation
 */

int stm32_exti_enable_irq(uint32_t line)
{
	/* Enable line interrupt at INTC level */
	LL_EXTI_EnableIT(line);

	/**
	 * Nothing else to do; INTC interrupt line
	 * is enabled at NVIC level during init.
	 */
	return 0;
}

int stm32_exti_disable_irq(uint32_t line)
{
	/* Disable line interrupt at INTC level */
	LL_EXTI_DisableIT(line);

	return 0;
}

void stm32_exti_set_trigger_type(uint32_t line, uint32_t trg)
{
	switch (trg) {
	case STM32_EXTI_TRIG_NONE:
		/**
		 * There is no NONE trigger on STM32WB0.
		 * We could disable the line interrupts here, but it isn't
		 * really necessary: the GPIO driver already does it by
		 * calling @ref stm32_exti_disable_irq before calling
		 * us with @p trigger = STM32_EXTI_TRIG_NONE.
		 */
		break;
	case STM32_EXTI_TRIG_RISING:
		LL_EXTI_EnableEdgeDetection(line);
		LL_EXTI_DisableBothEdgeTrig(line);
		LL_EXTI_EnableRisingTrig(line);
		break;
	case STM32_EXTI_TRIG_FALLING:
		LL_EXTI_EnableEdgeDetection(line);
		LL_EXTI_DisableBothEdgeTrig(line);
		LL_EXTI_DisableRisingTrig(line);
		break;
	case STM32_EXTI_TRIG_BOTH:
		LL_EXTI_EnableEdgeDetection(line);
		LL_EXTI_EnableBothEdgeTrig(line);
		break;
	case STM32_EXTI_TRIG_HIGH_LEVEL:
		LL_EXTI_DisableEdgeDetection(line);
		LL_EXTI_EnableRisingTrig(line);
		break;
	case STM32_EXTI_TRIG_LOW_LEVEL:
		LL_EXTI_DisableEdgeDetection(line);
		LL_EXTI_DisableRisingTrig(line);
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}

	/* Since it is not possible to disable triggers on STM32WB0,
	 * unlike in other STM32 series, activity on GPIO pin may have
	 * set the "event occurred" bit spuriously.
	 *
	 * Clear the bit now after reconfiguration to make sure that no
	 * spurious interrupt is delivered. (This works because interrupts
	 * are enabled *after* trigger selection by the GPIO driver, which
	 * is the only sensical order to do things in)
	 */
	LL_EXTI_ClearFlag(line);
}

int stm32_exti_set_irq_callback(uint32_t line,
								stm32_exti_irq_cb_t cb, void *user)
{
	struct gpio_irq_cb_wrp *cb_wrp = irq_cb_wrp_for_line(line);

	if ((cb_wrp->fn == cb) && (cb_wrp->data == user)) {
		return 0;
	}

	/* If line already has a callback, return EBUSY */
	if (cb_wrp->fn != NULL) {
		return -EBUSY;
	}

	cb_wrp->fn = cb;
	cb_wrp->data = user;

	return 0;
}

void stm32_exti_remove_irq_callback(uint32_t line)
{
	struct gpio_irq_cb_wrp *cb_wrp = irq_cb_wrp_for_line(line);

	cb_wrp->fn = cb_wrp->data = NULL;
}

int stm32_exti_enable_event(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);

	LL_EXTI_EnableBothEdgeTrig(ll_exti_line);

	return 0;
}

int stm32_exti_disable_event(uint32_t linenum)
{
	const uint32_t ll_exti_line = linenum_to_ll_exti_line(linenum);

	LL_EXTI_DisableBothEdgeTrig(ll_exti_line);

	return 0;
}

/**
 * @brief Enables external interrupt/event for specified EXTI line
 *
 * @param linenum EXTI line number
 * @param mode EXTI mode
 */
void stm32_exti_set_mode(uint32_t linenum, stm32_exti_mode mode)
{
	switch (mode) {
	case STM32_EXTI_MODE_NONE:
		stm32_exti_disable_event(linenum);
		stm32_exti_disable_irq(linenum);
		break;
	case STM32_EXTI_MODE_IT:
		stm32_exti_disable_event(linenum);
		stm32_exti_enable_irq(linenum);
		break;
	case STM32_EXTI_MODE_EVENT:
		stm32_exti_disable_irq(linenum);
		stm32_exti_enable_event(linenum);
		break;
	case STM32_EXTI_MODE_BOTH:
		stm32_exti_enable_irq(linenum);
		stm32_exti_enable_event(linenum);
		break;
	default:
		__ASSERT_NO_MSG(0);
		break;
	}
}

int stm32_exti_enable(uint32_t linenum, stm32_exti_trigger_type trigger,
					  stm32_exti_mode mode, stm32_exti_irq_cb_t cb, void *user)
{
	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	int ret = 0;

	ret = stm32_exti_set_irq_callback(linenum, cb, user);
	if (ret != 0) {
		return ret;
	}

	stm32_exti_set_trigger_type(linenum, trigger);
	stm32_exti_set_mode(linenum, mode);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return ret;
}

int stm32_exti_disable(uint32_t linenum)
{
	z_stm32_hsem_lock(CFG_HW_EXTI_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	stm32_exti_set_trigger_type(linenum, STM32_EXTI_TRIG_NONE);
	stm32_exti_set_mode(linenum, STM32_EXTI_MODE_NONE);
	stm32_exti_remove_irq_callback(linenum);

	z_stm32_hsem_unlock(CFG_HW_EXTI_SEMID);

	return 0;
}
