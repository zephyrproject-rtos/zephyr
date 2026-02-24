/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <gpio/gpio_stm32.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_system.h>

#include <stm32_gpio_shared.h>

/** Helper to extract IO port number from STM32_PINMUX() encoded value */
#define STM32_DT_PINMUX_PORT(__pin) \
	(((__pin) >> STM32_PORT_SHIFT) & STM32_PORT_MASK)

/** Helper to extract IO pin number from STM32_PINMUX() encoded value */
#define STM32_DT_PINMUX_LINE(__pin) \
	(((__pin) >> STM32_LINE_SHIFT) & STM32_LINE_MASK)

/** Helper to extract IO pin func from STM32_PINMUX() encoded value */
#define STM32_DT_PINMUX_FUNC(__pin) \
	(((__pin) >> STM32_MODE_SHIFT) & STM32_MODE_MASK)

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl)
/** Helper to extract IO pin remap from STM32_PINMUX() encoded value */
#define STM32_DT_PINMUX_REMAP(__pin) \
	(((__pin) >> STM32_REMAP_SHIFT) & STM32_REMAP_MASK)
#endif

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

int stm32_pinmux_init_remap(void)
{

#if REMAP_PA11 || REMAP_PA12

#if !defined(CONFIG_SOC_SERIES_STM32G0X) && !defined(CONFIG_SOC_SERIES_STM32C0X)
#error "Pin remap property available only on STM32G0 and STM32C0 SoC series"
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

	remap = NO_REMAP;

	for (size_t i = 0U; i < pin_cnt; i++) {
		if (remap == NO_REMAP) {
			remap = STM32_DT_PINMUX_REMAP(pins[i].pinmux);
		} else if (STM32_DT_PINMUX_REMAP(pins[i].pinmux) == NO_REMAP) {
			continue;
		} else if (STM32_DT_PINMUX_REMAP(pins[i].pinmux) != remap) {
			return -EINVAL;
		}
	}

	/* not remappable */
	if (remap == NO_REMAP) {
		return 0;
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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_pinctrl)
static int apply_iosync_configuration(uint32_t port, uint32_t pin, uint32_t pincfg)
{
	const struct device *port_device;
	const struct gpio_stm32_config *gpio_cfg;
	uint32_t piocfgr, delayr, pinbit;
	GPIO_TypeDef *gpio_reg;
	int ret;

	port_device = stm32_gpioport_get(port);
	if (port_device == NULL) {
		return -ENODEV;
	}

	/**
	 * For lack of better way, obtain the GPIO base address from the
	 * device's configuration directly. This *can* be made cleaner
	 * but would require reworking the GPIO & PINCTRL entirely...
	 */
	gpio_cfg = port_device->config;
	gpio_reg = (GPIO_TypeDef *)gpio_cfg->base;

	/* Make sure GPIO clock is enabled */
	ret = pm_device_runtime_get(port_device);
	if (ret < 0) {
		return ret;
	}

	piocfgr = (pincfg >> STM32_IORETIME_ADVCFGR_SHIFT) & STM32_IORETIME_ADVCFGR_MASK;
	delayr = (pincfg >> STM32_IODELAY_LENGTH_SHIFT) & STM32_IODELAY_LENGTH_MASK;
	pinbit = BIT(pin);

	/**
	 * Thanks to clever encoding, we don't have to check whether the I/O retiming
	 * is to be enabled or not; all we need to do is write to the registers where
	 * everything will fall in place nicely. This can obviously be updated for
	 * new hardware, if required...
	 */
	if (pin <= 7) {
		LL_GPIO_SetDelayPin_0_7(gpio_reg, pinbit, delayr);
		LL_GPIO_SetPIOControlPin_0_7(gpio_reg, pinbit, piocfgr);
	} else {
		LL_GPIO_SetDelayPin_8_15(gpio_reg, pinbit, delayr);
		LL_GPIO_SetPIOControlPin_8_15(gpio_reg, pinbit, piocfgr);
	}

	/* Release GPIO device since we are done */
	return pm_device_runtime_put(port_device);
}
#endif

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	const struct device *port;
	uint32_t pin_cgf = 0;
	uint32_t mux;
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
			pin_cgf = pins[i].pincfg;
		} else {
			/* Not supported */
			__ASSERT_NO_MSG(STM32_DT_PINMUX_FUNC(mux));
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32f1_pinctrl) */
		port = stm32_gpioport_get(STM32_DT_PINMUX_PORT(mux));
		if (port == NULL) {
			return -ENODEV;
		}

		ret = gpio_stm32_configure(port, STM32_DT_PINMUX_LINE(mux),
					   pin_cgf, STM32_DT_PINMUX_FUNC(mux));
		if (ret < 0) {
			return ret;
		}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_pinctrl)
		ret = apply_iosync_configuration(
			STM32_DT_PINMUX_PORT(mux),
			STM32_DT_PINMUX_LINE(mux),
			pins[i].pincfg);
		if (ret < 0) {
			return ret;
		}
#endif
	}

	return 0;
}
