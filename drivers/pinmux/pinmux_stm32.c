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

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_system.h>
#include <zephyr/drivers/pinmux.h>
#include <gpio/gpio_stm32.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <pinmux/pinmux_stm32.h>

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

	if (STM32_PORT(pin) >= STM32_PORTS_MAX) {
		return -EINVAL;
	}

	port_device = gpio_ports[STM32_PORT(pin)];

	if ((port_device == NULL) || (!device_is_ready(port_device))) {
		return -ENODEV;
	}

	return gpio_stm32_configure(port_device, STM32_PIN(pin), func, altf);
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
	if (stm32_dt_pinctrl_remap(pinctrl, list_size)) {
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

/* ignore swj-cfg reset state (default value) */
#if ((DT_NODE_HAS_PROP(DT_NODELABEL(pinctrl), swj_cfg)) && \
	(DT_ENUM_IDX(DT_NODELABEL(pinctrl), swj_cfg) != 0))

static int stm32f1_swj_cfg_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);

	/* reset state is '000' (Full SWJ, (JTAG-DP + SW-DP)) */
	/* only one of the 3 bits can be set */
#if (DT_ENUM_IDX(DT_NODELABEL(pinctrl), swj_cfg) == 1)
	/* 001: Full SWJ (JTAG-DP + SW-DP) but without NJTRST */
	/* releases: PB4 */
	LL_GPIO_AF_Remap_SWJ_NONJTRST();
#elif (DT_ENUM_IDX(DT_NODELABEL(pinctrl), swj_cfg) == 2)
	/* 010: JTAG-DP Disabled and SW-DP Enabled */
	/* releases: PB4 PB3 PA15 */
	LL_GPIO_AF_Remap_SWJ_NOJTAG();
#elif (DT_ENUM_IDX(DT_NODELABEL(pinctrl), swj_cfg) == 3)
	/* 100: JTAG-DP Disabled and SW-DP Disabled */
	/* releases: PB4 PB3 PA13 PA14 PA15 */
	LL_GPIO_AF_DisableRemap_SWJ();
#endif

	return 0;
}

SYS_INIT(stm32f1_swj_cfg_init, PRE_KERNEL_1, 0);

#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(pinctrl), swj_cfg) */

/**
 * @brief Helper function to check and apply provided pinctrl remap
 *        configuration
 *
 * Check operation verifies that pin remapping configuration is the same on all
 * pins. If configuration is valid AFIO clock is enabled and remap is applied
 *
 * @param *pinctrl pointer to soc_gpio_pinctrl list
 * @param list_size list size
 *
 * @return 0 on success, -EINVAL otherwise
 */
int stm32_dt_pinctrl_remap(const struct soc_gpio_pinctrl *pinctrl,
			   size_t list_size)
{
	uint32_t reg_val;
	uint16_t remap;

	remap = (uint16_t)STM32_DT_PINMUX_REMAP(pinctrl[0].pinmux);

	/* not remappable */
	if (remap == NO_REMAP) {
		return 0;
	}

	for (size_t i = 1U; i < list_size; i++) {
		if (STM32_DT_PINMUX_REMAP(pinctrl[i].pinmux) != remap) {
			return -EINVAL;
		}
	}

	/* A valid remapping configuration is available */
	/* Apply remapping before proceeding with pin configuration */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);

	if (STM32_REMAP_REG_GET(remap) == 0U) {
		/* read initial value, ignore write-only SWJ_CFG */
		reg_val = AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG;
		reg_val |= STM32_REMAP_VAL_GET(remap) << STM32_REMAP_SHIFT_GET(remap);
		/* apply undocumented '111' (AFIO_MAPR_SWJ_CFG) to affirm SWJ_CFG */
		/* the pins are not remapped without that (when SWJ_CFG is not default) */
		AFIO->MAPR = reg_val | AFIO_MAPR_SWJ_CFG;
	} else {
		reg_val = AFIO->MAPR2;
		reg_val |= STM32_REMAP_VAL_GET(remap) << STM32_REMAP_SHIFT_GET(remap);
		AFIO->MAPR2 = reg_val;
	}

	return 0;
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */
