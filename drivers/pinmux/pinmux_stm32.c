/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * A common driver for STM32 pinmux.
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_system.h>
#include <drivers/pinmux.h>
#include <gpio/gpio_stm32.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <pinmux/pinmux_stm32.h>
#include <pm/device_runtime.h>

const struct device * const gpio_ports[STM32_PORTS_MAX] = {
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioa)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiob)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioc)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiod)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioe)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiof)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiog)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioh)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioi)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioj)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiok)),
};

#if DT_NODE_HAS_PROP(DT_NODELABEL(pinctrl), remap_pa11)
#define REMAP_PA11	DT_PROP(DT_NODELABEL(pinctrl), remap_pa11)
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(pinctrl), remap_pa12)
#define REMAP_PA12	DT_PROP(DT_NODELABEL(pinctrl), remap_pa12)
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(pinctrl), remap_pa11_pa12)
#define REMAP_PA11_PA12	DT_PROP(DT_NODELABEL(pinctrl), remap_pa11_pa12)
#endif

#if REMAP_PA11 || REMAP_PA12 || REMAP_PA11_PA12

int stm32_pinmux_init_remap(const struct device *dev)
{
	ARG_UNUSED(dev);

#if REMAP_PA11 || REMAP_PA12

#if !defined(CONFIG_SOC_SERIES_STM32G0X)
#error "Pin remap property available only on STM32G0 SoC series"
#endif

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
#if REMAP_PA11
	LL_SYSCFG_EnablePinRemap(LL_SYSCFG_PIN_RMP_PA11);
#endif
#if REMAP_PA12
	LL_SYSCFG_EnablePinRemap(LL_SYSCFG_PIN_RMP_PA12);
#endif

#elif REMAP_PA11_PA12

#if !defined(SYSCFG_CFGR1_PA11_PA12_RMP)
#error "Pin remap property available only on STM32F070x SoC series"
#endif

	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
	LL_SYSCFG_EnablePinRemap();

#endif /* (REMAP_PA11 || REMAP_PA12) || REMAP_PA11_PA12 */

	return 0;
}

SYS_INIT(stm32_pinmux_init_remap, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_REMAP_INIT_PRIORITY);

#endif /* REMAP_PA11 || REMAP_PA12 || REMAP_PA11_PA12 */


static int stm32_pin_configure(uint32_t pin, uint32_t func, uint32_t altf)
{
	const struct device *port_device;
	int ret = 0;

	if (STM32_PORT(pin) >= STM32_PORTS_MAX) {
		return -EINVAL;
	}

	port_device = gpio_ports[STM32_PORT(pin)];

	if ((port_device == NULL) || (!device_is_ready(port_device))) {
		return -ENODEV;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = pm_device_runtime_get(port_device);
	if (ret != 0) {
		return ret;
	}
#endif

	gpio_stm32_configure(port_device, STM32_PIN(pin), func, altf);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = pm_device_runtime_put(port_device);
#endif

	return ret;
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
	uint32_t pin, mux;
	uint32_t func = 0;
	int ret = 0;

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
		if (STM32_DT_PINMUX_FUNC(mux) < STM32_ANALOG) {
			func = pinctrl[i].pincfg | STM32_MODER_ALT_MODE;
		} else if (STM32_DT_PINMUX_FUNC(mux) == STM32_ANALOG) {
			func = STM32_MODER_ANALOG_MODE;
		} else {
			/* Not supported */
			__ASSERT_NO_MSG(STM32_DT_PINMUX_FUNC(mux));
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

		pin = STM32PIN(STM32_DT_PINMUX_PORT(mux),
			       STM32_DT_PINMUX_LINE(mux));

		ret = stm32_pin_configure(pin, func, STM32_DT_PINMUX_FUNC(mux));
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
/* Z_AFIO_REMAP is keeping the value of AFIO_MAPR_SWJ_CFG_x */
#if defined(CONFIG_GPIO_STM32_SWJ_ENABLE)
/* reset state */
#define Z_AFIO_REMAP AFIO_MAPR_SWJ_CFG_RESET
#elif defined(CONFIG_GPIO_STM32_SWJ_NONJTRST)
/* released PB4 */
#define Z_AFIO_REMAP AFIO_MAPR_SWJ_CFG_NOJNTRST
#elif defined(CONFIG_GPIO_STM32_SWJ_NOJTAG)
/* released PB4 PB3 PA15 */
#define Z_AFIO_REMAP AFIO_MAPR_SWJ_CFG_JTAGDISABLE
#elif defined(CONFIG_GPIO_STM32_SWJ_DISABLE)
/* released PB4 PB3 PA13 PA14 PA15 */
#define Z_AFIO_REMAP AFIO_MAPR_SWJ_CFG_DISABLE
#endif

/* enable remap : modify MAPR and keep the AFIO_MAPR_SWJ_CFG_x */
#define enable_remap(REMAP_PIN) MODIFY_REG(AFIO->MAPR,\
					   (REMAP_PIN | AFIO_MAPR_SWJ_CFG), \
					   (REMAP_PIN | Z_AFIO_REMAP))

/* enable partial remap : modify MAPR and keep the AFIO_MAPR_SWJ_CFG_x */
#define enable_partial_remap(REMAP_PIN, PARTIAL_REMAP) \
				MODIFY_REG(AFIO->MAPR, \
					   (REMAP_PIN | AFIO_MAPR_SWJ_CFG), \
					   (PARTIAL_REMAP | Z_AFIO_REMAP))

/* disable remap : modify MAPR and keep the AFIO_MAPR_SWJ_CFG_x */
#define disable_remap(REMAP_PIN) MODIFY_REG(AFIO->MAPR,\
					    (REMAP_PIN | AFIO_MAPR_SWJ_CFG), \
					    Z_AFIO_REMAP)

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
	uint8_t remap;

	remap = (uint8_t)STM32_DT_PINMUX_REMAP(pinctrl[0].pinmux);

	for (size_t i = 1U; i < list_size; i++) {
		if (STM32_DT_PINMUX_REMAP(pinctrl[i].pinmux) != remap) {
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
			/* PB8/PB9 (CAN_REMAP = 0b10) */
			enable_partial_remap(AFIO_MAPR_CAN_REMAP,
				AFIO_MAPR_CAN_REMAP_REMAP2);
		} else if (remap == REMAP_2) {
			/* PD0/PD1  (CAN_REMAP = 0b11) */
			enable_partial_remap(AFIO_MAPR_CAN_REMAP,
				AFIO_MAPR_CAN_REMAP_REMAP3);
		} else {
			/* NO_REMAP: PA11/PA12 (CAN_REMAP = 0b00) */
			enable_partial_remap(AFIO_MAPR_CAN_REMAP,
				AFIO_MAPR_CAN_REMAP_REMAP1);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(can2), okay)
	case DT_REG_ADDR(DT_NODELABEL(can2)):
		if (remap == REMAP_1) {
			/* PB5/PB6 */
			enable_remap(AFIO_MAPR_CAN2_REMAP);
		} else {
			/* PB12/PB13 */
			disable_remap(AFIO_MAPR_CAN2_REMAP);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
	case DT_REG_ADDR(DT_NODELABEL(i2c1)):
		if (remap == REMAP_1) {
			enable_remap(AFIO_MAPR_I2C1_REMAP);
		} else {
			disable_remap(AFIO_MAPR_I2C1_REMAP);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers1), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers1)):
		if (remap == REMAP_1) {
			enable_partial_remap(AFIO_MAPR_TIM1_REMAP,
				AFIO_MAPR_TIM1_REMAP_PARTIALREMAP);
		} else if (remap == REMAP_2) {
			enable_partial_remap(AFIO_MAPR_TIM1_REMAP,
				AFIO_MAPR_TIM1_REMAP_FULLREMAP);
		} else {
			enable_partial_remap(AFIO_MAPR_TIM1_REMAP,
				AFIO_MAPR_TIM1_REMAP_NOREMAP);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers2), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers2)):
		if (remap == REMAP_1) {
			enable_partial_remap(AFIO_MAPR_TIM2_REMAP,
				AFIO_MAPR_TIM2_REMAP_PARTIALREMAP1);
		} else if (remap == REMAP_2) {
			enable_partial_remap(AFIO_MAPR_TIM2_REMAP,
				AFIO_MAPR_TIM2_REMAP_PARTIALREMAP2);
		} else if (remap == REMAP_FULL) {
			enable_partial_remap(AFIO_MAPR_TIM2_REMAP,
				AFIO_MAPR_TIM2_REMAP_FULLREMAP);
		} else {
			enable_partial_remap(AFIO_MAPR_TIM2_REMAP,
				AFIO_MAPR_TIM2_REMAP_NOREMAP);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers3), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers3)):
		if (remap == REMAP_1) {
			enable_partial_remap(AFIO_MAPR_TIM3_REMAP,
				AFIO_MAPR_TIM3_REMAP_PARTIALREMAP);
		} else if (remap == REMAP_2) {
			enable_partial_remap(AFIO_MAPR_TIM3_REMAP,
				AFIO_MAPR_TIM3_REMAP_FULLREMAP);
		} else {
			enable_partial_remap(AFIO_MAPR_TIM3_REMAP,
				AFIO_MAPR_TIM3_REMAP_NOREMAP);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(timers4), okay)
	case DT_REG_ADDR(DT_NODELABEL(timers4)):
		if (remap == REMAP_1) {
			enable_remap(AFIO_MAPR_TIM4_REMAP);
		} else {
			disable_remap(AFIO_MAPR_TIM4_REMAP);
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
			enable_remap(AFIO_MAPR_USART1_REMAP);
		} else {
			disable_remap(AFIO_MAPR_USART1_REMAP);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart2), okay)
	case DT_REG_ADDR(DT_NODELABEL(usart2)):
		if (remap == REMAP_1) {
			enable_remap(AFIO_MAPR_USART2_REMAP);
		} else {
			disable_remap(AFIO_MAPR_USART2_REMAP);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart3), okay)
	case DT_REG_ADDR(DT_NODELABEL(usart3)):
		if (remap == REMAP_2) {
			enable_partial_remap(AFIO_MAPR_USART3_REMAP,
				AFIO_MAPR_USART3_REMAP_FULLREMAP);
		} else if (remap == REMAP_1) {
			enable_partial_remap(AFIO_MAPR_USART3_REMAP,
				AFIO_MAPR_USART3_REMAP_PARTIALREMAP);
		} else {
			enable_partial_remap(AFIO_MAPR_USART3_REMAP,
				AFIO_MAPR_USART3_REMAP_NOREMAP);
		}
		break;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay)
	case DT_REG_ADDR(DT_NODELABEL(spi1)):
		if (remap == REMAP_1) {
			enable_remap(AFIO_MAPR_SPI1_REMAP);
		} else {
			disable_remap(AFIO_MAPR_SPI1_REMAP);
		}
		break;
#endif
	}

	return 0;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */
