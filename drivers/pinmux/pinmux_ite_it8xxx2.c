/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief PINMUX driver for the IT8xxx2
 */

#include <errno.h>
#include <device.h>
#include <drivers/pinmux.h>
#include <soc.h>
#include <dt-bindings/pinctrl/it8xxx2-pinctrl.h>

#define DT_DRV_COMPAT ite_it8xxx2_pinmux

#include <logging/log.h>
LOG_MODULE_REGISTER(pinmux_ite_it8xxx2, LOG_LEVEL_ERR);

struct pinmux_it8xxx2_config {
	/* gpio port control register (byte mapping to pin) */
	uintptr_t reg_gpcr;
	/* function 3 general control register */
	uintptr_t func3_gcr[8];
	/* function 4 general control register */
	uintptr_t func4_gcr[8];
	/* function 3 enable mask */
	uint8_t func3_en_mask[8];
	/* function 4 enable mask */
	uint8_t func4_en_mask[8];
};

static int pinmux_it8xxx2_set(const struct device *dev,
			      uint32_t pin, uint32_t func)
{
	const struct pinmux_it8xxx2_config *pinmux_config = dev->config;

	volatile uint8_t *reg_gpcr =
		(uint8_t *)(pinmux_config->reg_gpcr + pin);
	volatile uint8_t *reg_func3_gcr =
		(uint8_t *)(pinmux_config->func3_gcr[pin]);
	volatile uint8_t *reg_func4_gcr =
		(uint8_t *)(pinmux_config->func4_gcr[pin]);

	if (pin >= IT8XXX2_PINMUX_PINS) {
		return -EINVAL;
	}

	/* Common settings for alternate function. */
	*reg_gpcr &= ~(GPCR_PORT_PIN_MODE_INPUT |
		GPCR_PORT_PIN_MODE_OUTPUT);

	switch (func) {
	case IT8XXX2_PINMUX_FUNC_1:
		/* Func1: Alternate function has been set above. */
		break;
	case IT8XXX2_PINMUX_FUNC_2:
		/* Func2: WUI function: turn the pin into an input */
		*reg_gpcr |= GPCR_PORT_PIN_MODE_INPUT;
		break;
	case IT8XXX2_PINMUX_FUNC_3:
		/*
		 * Func3: In addition to the alternate setting above,
		 *        Func3 also need to set the general control.
		 */
		*reg_func3_gcr |= pinmux_config->func3_en_mask[pin];
		break;
	case IT8XXX2_PINMUX_FUNC_4:
		/*
		 * Func4: In addition to the alternate setting above,
		 *        Func4 also need to set the general control.
		 */
		*reg_func4_gcr |= pinmux_config->func4_en_mask[pin];
		break;
	default:
		LOG_ERR("This function is not supported");
		return -EINVAL;
	}

	return 0;
}

static int pinmux_it8xxx2_get(const struct device *dev,
			      uint32_t pin, uint32_t *func)
{
	const struct pinmux_it8xxx2_config *pinmux_config = dev->config;

	volatile uint8_t *reg_gpcr =
		(uint8_t *)(pinmux_config->reg_gpcr + pin);

	if (pin >= IT8XXX2_PINMUX_PINS || func == NULL) {
		return -EINVAL;
	}

	*func = (*reg_gpcr & (GPCR_PORT_PIN_MODE_INPUT |
		GPCR_PORT_PIN_MODE_OUTPUT)) == GPCR_PORT_PIN_MODE_INPUT ?
		IT8XXX2_PINMUX_FUNC_2 : IT8XXX2_PINMUX_FUNC_1;

	/* TODO: IT8XXX2_PINMUX_FUNC_3 & IT8XXX2_PINMUX_FUNC_4 */

	return 0;
}

static int pinmux_it8xxx2_pullup(const struct device *dev,
				 uint32_t pin, uint8_t func)
{
	const struct pinmux_it8xxx2_config *pinmux_config = dev->config;

	volatile uint8_t *reg_gpcr =
		(uint8_t *)(pinmux_config->reg_gpcr + pin);

	if (func == PINMUX_PULLUP_ENABLE) {
		*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_PULLUP) &
			~GPCR_PORT_PIN_MODE_PULLDOWN;
	} else if (func == PINMUX_PULLUP_DISABLE) {
		*reg_gpcr &= ~(GPCR_PORT_PIN_MODE_PULLUP |
			GPCR_PORT_PIN_MODE_PULLDOWN);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int pinmux_it8xxx2_input(const struct device *dev,
				uint32_t pin, uint8_t func)
{
	const struct pinmux_it8xxx2_config *pinmux_config = dev->config;

	volatile uint8_t *reg_gpcr =
		(uint8_t *)(pinmux_config->reg_gpcr + pin);

	*reg_gpcr &= ~(GPCR_PORT_PIN_MODE_INPUT |
		GPCR_PORT_PIN_MODE_OUTPUT);

	if (func == PINMUX_INPUT_ENABLED) {
		*reg_gpcr |= GPCR_PORT_PIN_MODE_INPUT;
	} else if (func == PINMUX_OUTPUT_ENABLED) {
		*reg_gpcr |= GPCR_PORT_PIN_MODE_OUTPUT;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int pinmux_it8xxx2_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * The default value of LPCRSTEN is bit2:1 = 10b(GPD2) in GCR.
	 * If LPC reset is enabled on GPB7, we have to clear bit2:1
	 * to 00b.
	 */
	IT8XXX2_GPIO_GCR &= ~(BIT(1) | BIT(2));

	/*
	 * If SMBUS3 swaps from H group to F group, we have to
	 * set SMB3PSEL = 1 in PMER3 register.
	 */
	if (DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(i2c3), gpio_dev)) ==
	    DEVICE_DT_GET(DT_NODELABEL(gpiof))) {

		struct gctrl_it8xxx2_regs *const gctrl_base =
			(struct gctrl_it8xxx2_regs *)
				DT_REG_ADDR(DT_NODELABEL(gctrl));

			gctrl_base->GCTRL_PMER3 |= IT8XXX2_GCTRL_SMB3PSEL;
	}
	/*
	 * TODO: If UART2 swaps from bit2:1 to bit6:5 in H group, we
	 * have to set UART1PSEL = 1 in UART1PMR register.
	 */

	return 0;
}

static const struct pinmux_driver_api pinmux_it8xxx2_driver_api = {
	.set = pinmux_it8xxx2_set,
	.get = pinmux_it8xxx2_get,
	.pullup = pinmux_it8xxx2_pullup,
	.input = pinmux_it8xxx2_input,
};

#define PINMUX_ITE_INIT(inst)							\
	static const struct pinmux_it8xxx2_config pinmux_it8xxx2_cfg_##inst = {	\
		.reg_gpcr = DT_INST_REG_ADDR(inst),				\
		.func3_gcr = DT_INST_PROP(inst, func3_gcr),			\
		.func3_en_mask = DT_INST_PROP(inst, func3_en_mask),		\
		.func4_gcr = DT_INST_PROP(inst, func4_gcr),			\
		.func4_en_mask = DT_INST_PROP(inst, func4_en_mask),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			    &pinmux_it8xxx2_init,				\
			    NULL, NULL, &pinmux_it8xxx2_cfg_##inst,		\
			    PRE_KERNEL_1,					\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
			    &pinmux_it8xxx2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PINMUX_ITE_INIT)
