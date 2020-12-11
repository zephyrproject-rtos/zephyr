/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * A common driver for STM32 pinmux. Each SoC must implement a SoC
 * specific part of the driver.
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>
#include <drivers/pinmux.h>
#include <gpio/gpio_stm32.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/stm32/pinmux_stm32.h>

#ifdef CONFIG_SOC_SERIES_STM32MP1X
#define GPIO_REG_SIZE         0x1000
/* 0x1000 between each port, 0x400 gpio registry 0xC00 reserved */
#else
#define GPIO_REG_SIZE         0x400
#endif /* CONFIG_SOC_SERIES_STM32MP1X */
/* base address for where GPIO registers start */
#define GPIO_PORTS_BASE       (GPIOA_BASE)

static const uint32_t ports_enable[STM32_PORTS_MAX] = {
	STM32_PERIPH_GPIOA,
	STM32_PERIPH_GPIOB,
	STM32_PERIPH_GPIOC,
#ifdef GPIOD_BASE
	STM32_PERIPH_GPIOD,
#else
	STM32_PORT_NOT_AVAILABLE,
#endif
#ifdef GPIOE_BASE
	STM32_PERIPH_GPIOE,
#else
	STM32_PORT_NOT_AVAILABLE,
#endif
#ifdef GPIOF_BASE
	STM32_PERIPH_GPIOF,
#else
	STM32_PORT_NOT_AVAILABLE,
#endif
#ifdef GPIOG_BASE
	STM32_PERIPH_GPIOG,
#else
	STM32_PORT_NOT_AVAILABLE,
#endif
#ifdef GPIOH_BASE
	STM32_PERIPH_GPIOH,
#else
	STM32_PORT_NOT_AVAILABLE,
#endif
#ifdef GPIOI_BASE
	STM32_PERIPH_GPIOI,
#else
	STM32_PORT_NOT_AVAILABLE,
#endif
#ifdef GPIOJ_BASE
	STM32_PERIPH_GPIOJ,
#else
	STM32_PORT_NOT_AVAILABLE,
#endif
#ifdef GPIOK_BASE
	STM32_PERIPH_GPIOK,
#else
	STM32_PORT_NOT_AVAILABLE,
#endif
};

/**
 * @brief enable IO port clock
 *
 * @param port I/O port ID
 * @param clk  optional clock device
 *
 * @return 0 on success, error otherwise
 */
static int enable_port(uint32_t port, const struct device *clk)
{
	/* enable port clock */
	if (!clk) {
		clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	}

	struct stm32_pclken pclken;

	pclken.bus = STM32_CLOCK_BUS_GPIO;
	pclken.enr = ports_enable[port];

	if (pclken.enr == STM32_PORT_NOT_AVAILABLE) {
		return -EIO;
	}

	return clock_control_on(clk, (clock_control_subsys_t *) &pclken);
}

static int stm32_pin_configure(uint32_t pin, uint32_t func, uint32_t altf)
{
	/* determine IO port registers location */
	uint32_t offset = STM32_PORT(pin) * GPIO_REG_SIZE;
	uint8_t *port_base = (uint8_t *)(GPIO_PORTS_BASE + offset);

	/* not much here, on STM32F10x the alternate function is
	 * controller by setting up GPIO pins in specific mode.
	 */
	return gpio_stm32_configure((uint32_t *)port_base,
				    STM32_PIN(pin), func, altf);
}

/**
 * @brief helper for converting dt stm32 pinctrl format to existing pin config
 *        format
 *
 * @param *pinctrl pointer to soc_gpio_pinctrl list
 * @param list_size list size
 * @param base device base register value
 *
 * @return 0 on success, -EINVAL otherwise
 */
int stm32_dt_pinctrl_configure(const struct soc_gpio_pinctrl *pinctrl,
			       size_t list_size, uint32_t base)
{
	const struct device *clk;
	uint32_t pin, mux;
	uint32_t func = 0;

	if (!list_size) {
		/* Empty pinctrl. Exit */
		return 0;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
	if (stm32_dt_pinctrl_remap(pinctrl, list_size, base)) {
		/* Wrong remap config. Exit */
		return -EINVAL;
	}
#else
	ARG_UNUSED(base);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

	/* make sure to enable port clock first */
	clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	for (int i = 0; i < list_size; i++) {
		mux = pinctrl[i].pinmux;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
		uint32_t pupd;

		if (STM32_DT_PINMUX_FUNC(mux) == ALTERNATE) {
			func = pinctrl[i].pincfg | STM32_MODE_OUTPUT |
							STM32_CNF_ALT_FUNC;
		} else if (STM32_DT_PINMUX_FUNC(mux) == ANALOG) {
			func = pinctrl[i].pincfg | STM32_MODE_INPUT |
							STM32_CNF_IN_ANALOG;
		} else if (STM32_DT_PINMUX_FUNC(mux) == GPIO_IN) {
			func = pinctrl[i].pincfg | STM32_MODE_INPUT;
			pupd = func & (STM32_PUPD_MASK << STM32_PUPD_SHIFT);
			if (pupd == STM32_PUPD_NO_PULL) {
				func = func | STM32_CNF_IN_FLOAT;
			} else {
				func = func | STM32_CNF_IN_PUPD;
			}
		} else {
			/* Not supported */
			__ASSERT_NO_MSG(STM32_DT_PINMUX_FUNC(mux));
		}
#else
		if (STM32_DT_PINMUX_FUNC(mux) < ANALOG) {
			func = pinctrl[i].pincfg | STM32_MODER_ALT_MODE;
		} else if (STM32_DT_PINMUX_FUNC(mux) == ANALOG) {
			func = STM32_MODER_ANALOG_MODE;
		} else {
			/* Not supported */
			__ASSERT_NO_MSG(STM32_DT_PINMUX_FUNC(mux));
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

		pin = STM32PIN(STM32_DT_PINMUX_PORT(mux),
			       STM32_DT_PINMUX_LINE(mux));

		enable_port(STM32_PORT(pin), clk);

		stm32_pin_configure(pin, func, STM32_DT_PINMUX_FUNC(mux));
	}

	return 0;
}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
/**
 * @brief Helper function to check and apply provided pinctrl remap
 *        configuration
 *
 * Check operation verifies that pin remapping configuration is the same on all
 * pins. If configuration is valid AFIO clock is enabled and remap is applied
 *
 * @param *pinctrl pointer to soc_gpio_pinctrl list
 * @param list_size list size
 * @param base device base register value
 *
 * @return 0 on success, -EINVAL otherwise
 */
int stm32_dt_pinctrl_remap(const struct soc_gpio_pinctrl *pinctrl,
			   size_t list_size, uint32_t base)
{
	int remap;
	uint32_t mux;

	remap = STM32_DT_PINMUX_REMAP(pinctrl[0].pinmux);

	for (int i = 1; i < list_size; i++) {
		mux = pinctrl[i].pinmux;
		remap = STM32_DT_PINMUX_REMAP(mux);

		if (STM32_DT_PINMUX_REMAP(mux) != remap) {
			return -EINVAL;
		}
	}

	/* A valid remapping configuration is available */
	/* Apply remapping before proceeding with pin configuration */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);

	switch (base) {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(can1), okay)
	case DT_REG_ADDR(DT_NODELABEL(can1)):
		if (remap == REMAP_1) {
			/* PB8/PB9 */
			LL_GPIO_AF_RemapPartial2_CAN1();
		} else if (remap == REMAP_2) {
			/* PD0/PD1 */
			LL_GPIO_AF_RemapPartial3_CAN1();
		} else {
			/* NO_REMAP: PA11/PA12 */
			LL_GPIO_AF_RemapPartial1_CAN1();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(can2), okay)
	case DT_REG_ADDR(DT_NODELABEL(can2)):
		if (remap == REMAP_1) {
			/* PB5/PB6 */
			LL_GPIO_AF_EnableRemap_CAN2();
		} else {
			/* PB12/PB13 */
			LL_GPIO_AF_DisableRemap_CAN2();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
	case DT_REG_ADDR(DT_NODELABEL(i2c1)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_I2C1();
		} else {
			LL_GPIO_AF_DisableRemap_I2C1();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers1), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers1)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_RemapPartial_TIM1();
		} else if (remap == REMAP_2) {
			LL_GPIO_AF_EnableRemap_TIM1();
		} else {
			LL_GPIO_AF_DisableRemap_TIM1();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers2), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers2)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_RemapPartial1_TIM2();
		} else if (remap == REMAP_2) {
			LL_GPIO_AF_RemapPartial2_TIM2();
		} else if (remap == REMAP_FULL) {
			LL_GPIO_AF_EnableRemap_TIM2();
		} else {
			LL_GPIO_AF_DisableRemap_TIM2();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers3), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers3)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_RemapPartial_TIM3();
		} else if (remap == REMAP_2) {
			LL_GPIO_AF_EnableRemap_TIM3();
		} else {
			LL_GPIO_AF_DisableRemap_TIM3();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers4), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers4)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM4();
		} else {
			LL_GPIO_AF_DisableRemap_TIM4();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers9), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers9)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM9();
		} else {
			LL_GPIO_AF_DisableRemap_TIM9();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers10), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers10)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM10();
		} else {
			LL_GPIO_AF_DisableRemap_TIM10();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers11), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers11)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM11();
		} else {
			LL_GPIO_AF_DisableRemap_TIM11();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers12), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers12)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM12();
		} else {
			LL_GPIO_AF_DisableRemap_TIM12();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers13), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers13)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM13();
		} else {
			LL_GPIO_AF_DisableRemap_TIM13();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers14), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers14)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM14();
		} else {
			LL_GPIO_AF_DisableRemap_TIM14();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers15), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers15)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM15();
		} else {
			LL_GPIO_AF_DisableRemap_TIM15();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers16), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers16)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM16();
		} else {
			LL_GPIO_AF_DisableRemap_TIM16();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers17), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers17)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_TIM17();
		} else {
			LL_GPIO_AF_DisableRemap_TIM17();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart1), okay)
	case DT_REG_ADDR(DT_NODELABEL(usart1)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_USART1();
		} else {
			LL_GPIO_AF_DisableRemap_USART1();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart2), okay)
	case DT_REG_ADDR(DT_NODELABEL(usart2)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_USART2();
		} else {
			LL_GPIO_AF_DisableRemap_USART2();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart3), okay)
	case DT_REG_ADDR(DT_NODELABEL(usart3)):
		if (remap == REMAP_2) {
			LL_GPIO_AF_EnableRemap_USART3();
		} else if (remap == REMAP_1) {
			LL_GPIO_AF_RemapPartial_USART3();
		} else {
			LL_GPIO_AF_DisableRemap_USART3();
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
	case DT_REG_ADDR(DT_NODELABEL(spi1)):
		if (remap == REMAP_1) {
			LL_GPIO_AF_EnableRemap_SPI1();
		} else {
			LL_GPIO_AF_DisableRemap_SPI1();
		}
		break;
#endif
	}

	return 0;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */


/**
 * @brief pin setup
 *
 * @param pin  STM32PIN() encoded pin ID
 * @param func SoC specific function assignment
 * @param clk  optional clock device
 *
 * @return 0 on success, error otherwise
 */
int z_pinmux_stm32_set(uint32_t pin, uint32_t func,
				const struct device *clk)
{
	/* make sure to enable port clock first */
	if (enable_port(STM32_PORT(pin), clk)) {
		return -EIO;
	}

	return stm32_pin_configure(pin, func, func & STM32_AFR_MASK);
}

/**
 * @brief setup pins according to their assignments
 *
 * @param pinconf  board pin configuration array
 * @param pins     array size
 */
void stm32_setup_pins(const struct pin_config *pinconf,
		      size_t pins)
{
	const struct device *clk;
	int i;

	clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	for (i = 0; i < pins; i++) {
		z_pinmux_stm32_set(pinconf[i].pin_num,
				  pinconf[i].mode,
				  clk);
	}
}
