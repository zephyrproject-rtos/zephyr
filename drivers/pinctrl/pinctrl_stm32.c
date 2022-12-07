/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <gpio/gpio_stm32.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_system.h>

/**
 * @brief Array containing pointers to each GPIO port.
 *
 * Entries will be NULL if the GPIO port is not enabled.
 */
static const struct device *const gpio_ports[] = {
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

/** Number of GPIO ports. */
static const size_t gpio_ports_cnt = ARRAY_SIZE(gpio_ports);

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
	 CONFIG_PINCTRL_STM32_REMAP_INIT_PRIORITY);

#endif /* REMAP_PA11 || REMAP_PA12 || REMAP_PA11_PA12 */

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
 * configuration.
 *
 * Check operation verifies that pin remapping configuration is the same on all
 * pins. If configuration is valid AFIO clock is enabled and remap is applied
 *
 * @param pins List of pins to be configured.
 * @param pin_cnt Number of pins.
 *
 * @retval 0 If successful
 * @retval -EINVAL If pins have an incompatible set of remaps.
 */
static int stm32_pins_remap(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt)
{
	uint32_t reg_val;
	uint16_t remap;

	remap = (uint16_t)STM32_DT_PINMUX_REMAP(pins[0].pinmux);

	/* not remappable */
	if (remap == NO_REMAP) {
		return 0;
	}

	for (size_t i = 1U; i < pin_cnt; i++) {
		if (STM32_DT_PINMUX_REMAP(pins[i].pinmux) != remap) {
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

static int stm32_pin_configure(uint32_t pin, uint32_t pin_cgf, uint32_t pin_func)
{
	const struct device *port_device;

	if (STM32_PORT(pin) >= gpio_ports_cnt) {
		return -EINVAL;
	}

	port_device = gpio_ports[STM32_PORT(pin)];

	if ((port_device == NULL) || (!device_is_ready(port_device))) {
		return -ENODEV;
	}

	return gpio_stm32_configure(port_device, STM32_PIN(pin), pin_cgf, pin_func);
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint32_t pin, mux;
	uint32_t pin_cgf = 0;
	int ret = 0;

	ARG_UNUSED(reg);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
	ret = stm32_pins_remap(pins, pin_cnt);
	if (ret < 0) {
		return ret;
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		mux = pins[i].pinmux;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
		uint32_t pupd;

		if (STM32_DT_PINMUX_FUNC(mux) == ALTERNATE) {
			pin_cgf = pins[i].pincfg | STM32_MODE_OUTPUT | STM32_CNF_ALT_FUNC;
		} else if (STM32_DT_PINMUX_FUNC(mux) == ANALOG) {
			pin_cgf = pins[i].pincfg | STM32_MODE_INPUT | STM32_CNF_IN_ANALOG;
		} else if (STM32_DT_PINMUX_FUNC(mux) == GPIO_IN) {
			pin_cgf = pins[i].pincfg | STM32_MODE_INPUT;
			pupd = pin_cgf & (STM32_PUPD_MASK << STM32_PUPD_SHIFT);
			if (pupd == STM32_PUPD_NO_PULL) {
				pin_cgf = pin_cgf | STM32_CNF_IN_FLOAT;
			} else {
				pin_cgf = pin_cgf | STM32_CNF_IN_PUPD;
			}
		} else if (STM32_DT_PINMUX_FUNC(mux) == GPIO_OUT) {
			pin_cgf = pins[i].pincfg | STM32_MODE_OUTPUT | STM32_CNF_GP_OUTPUT;
		} else {
			/* Not supported */
			__ASSERT_NO_MSG(STM32_DT_PINMUX_FUNC(mux));
		}
#else
		if (STM32_DT_PINMUX_FUNC(mux) < STM32_ANALOG) {
			pin_cgf = pins[i].pincfg | STM32_MODER_ALT_MODE;
		} else if (STM32_DT_PINMUX_FUNC(mux) == STM32_ANALOG) {
			pin_cgf = STM32_MODER_ANALOG_MODE;
		} else if (STM32_DT_PINMUX_FUNC(mux) == STM32_GPIO) {
			uint32_t gpio_out = pins[i].pincfg &
						(STM32_ODR_MASK << STM32_ODR_SHIFT);
			if (gpio_out != 0) {
				pin_cgf = pins[i].pincfg | STM32_MODER_OUTPUT_MODE;
			} else {
				pin_cgf = pins[i].pincfg | STM32_MODER_INPUT_MODE;
			}
		} else {
			/* Not supported */
			__ASSERT_NO_MSG(STM32_DT_PINMUX_FUNC(mux));
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */

		pin = STM32PIN(STM32_DT_PINMUX_PORT(mux),
			       STM32_DT_PINMUX_LINE(mux));

		ret = stm32_pin_configure(pin, pin_cgf, STM32_DT_PINMUX_FUNC(mux));
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
