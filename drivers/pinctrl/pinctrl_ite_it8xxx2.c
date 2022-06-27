/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_pinctrl_func

#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pinctrl_ite_it8xxx2, LOG_LEVEL_ERR);

#define GPIO_IT8XXX2_REG_BASE \
	((struct gpio_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(gpiogcr)))
#define GPIO_GROUP_MEMBERS  8

struct pinctrl_it8xxx2_config {
	/* gpio port control register (byte mapping to pin) */
	uint8_t *reg_gpcr;
	/* function 3 general control register */
	uintptr_t func3_gcr[GPIO_GROUP_MEMBERS];
	/* function 3 enable mask */
	uint8_t func3_en_mask[GPIO_GROUP_MEMBERS];
	/* function 4 general control register */
	uintptr_t func4_gcr[GPIO_GROUP_MEMBERS];
	/* function 4 enable mask */
	uint8_t func4_en_mask[GPIO_GROUP_MEMBERS];
	/* Input voltage selection */
	uintptr_t volt_sel[GPIO_GROUP_MEMBERS];
	/* Input voltage selection mask */
	uint8_t volt_sel_mask[GPIO_GROUP_MEMBERS];
};

static int pinctrl_it8xxx2_set(const pinctrl_soc_pin_t *pins)
{
	const struct pinctrl_it8xxx2_config *pinctrl_config = pins->pinctrls->config;
	uint32_t pincfg = pins->pincfg;
	uint8_t pin = pins->pin;
	volatile uint8_t *reg_gpcr = (uint8_t *)pinctrl_config->reg_gpcr + pin;
	volatile uint8_t *reg_volt_sel = (uint8_t *)(pinctrl_config->volt_sel[pin]);

	/* Setting pull-up or pull-down. */
	switch (IT8XXX2_DT_PINCFG_PUPDR(pincfg)) {
	case IT8XXX2_PULL_PIN_DEFAULT:
		/* No pull-up or pull-down */
		*reg_gpcr &= ~(GPCR_PORT_PIN_MODE_PULLUP |
			       GPCR_PORT_PIN_MODE_PULLDOWN);
		break;
	case IT8XXX2_PULL_UP:
		*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_PULLUP) &
			     ~GPCR_PORT_PIN_MODE_PULLDOWN;
		break;
	case IT8XXX2_PULL_DOWN:
		*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_PULLDOWN) &
			     ~GPCR_PORT_PIN_MODE_PULLUP;
		break;
	default:
		LOG_ERR("This pull level is not supported.");
		return -EINVAL;
	}

	/* Setting voltage 3.3V or 1.8V. */
	switch (IT8XXX2_DT_PINCFG_VOLTAGE(pincfg)) {
	case IT8XXX2_VOLTAGE_3V3:
		/* Input voltage selection 3.3V. */
		*reg_volt_sel &= ~pinctrl_config->volt_sel_mask[pin];
		break;
	case IT8XXX2_VOLTAGE_1V8:
		__ASSERT(!(IT8XXX2_DT_PINCFG_PUPDR(pincfg)
			   == IT8XXX2_PULL_UP),
		"Don't enable internal pullup if 1.8V voltage is used");
		/* Input voltage selection 1.8V. */
		*reg_volt_sel |= pinctrl_config->volt_sel_mask[pin];
		break;
	default:
		LOG_ERR("The voltage selection is not supported");
		return -EINVAL;
	}

	/* Setting tri-state mode. */
	if (IT8XXX2_DT_PINCFG_IMPEDANCE(pincfg)) {
		*reg_gpcr |= (GPCR_PORT_PIN_MODE_PULLUP |
			      GPCR_PORT_PIN_MODE_PULLDOWN);
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);

	const struct pinctrl_it8xxx2_config *pinctrl_config;
	volatile uint8_t *reg_gpcr;
	volatile uint8_t *reg_func3_gcr;
	volatile uint8_t *reg_func4_gcr;
	uint8_t pin;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_config = pins[i].pinctrls->config;
		pin = pins[i].pin;
		reg_gpcr = (uint8_t *)pinctrl_config->reg_gpcr + pin;
		reg_func3_gcr = (uint8_t *)(pinctrl_config->func3_gcr[pin]);
		reg_func4_gcr = (uint8_t *)(pinctrl_config->func4_gcr[pin]);

		/* Handle PIN configuration. */
		if (pinctrl_it8xxx2_set(&pins[i])) {
			LOG_ERR("Pin configuration is invalid.");
			return -EINVAL;
		}

		/*
		 * If pincfg is input, we don't need to handle
		 * alternate function.
		 */
		if (IT8XXX2_DT_PINCFG_INPUT(pins[i].pincfg)) {
			*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_INPUT) &
				     ~GPCR_PORT_PIN_MODE_OUTPUT;
			continue;
		}

		/*
		 * Handle alternate function.
		 */
		/* Common settings for alternate function. */
		*reg_gpcr &= ~(GPCR_PORT_PIN_MODE_INPUT |
			       GPCR_PORT_PIN_MODE_OUTPUT);
		switch (pins[i].alt_func) {
		case IT8XXX2_ALT_FUNC_1:
			/* Func1: Alternate function has been set above. */
			break;
		case IT8XXX2_ALT_FUNC_2:
			/* Func2: WUI function: turn the pin into an input */
			*reg_gpcr |= GPCR_PORT_PIN_MODE_INPUT;
			break;
		case IT8XXX2_ALT_FUNC_3:
			/*
			 * Func3: In addition to the alternate setting above,
			 *        Func3 also need to set the general control.
			 */
			*reg_func3_gcr |= pinctrl_config->func3_en_mask[pin];
			break;
		case IT8XXX2_ALT_FUNC_4:
			/*
			 * Func4: In addition to the alternate setting above,
			 *        Func4 also need to set the general control.
			 */
			*reg_func4_gcr |= pinctrl_config->func4_en_mask[pin];
			break;
		case IT8XXX2_ALT_DEFAULT:
			*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_INPUT) &
				     ~GPCR_PORT_PIN_MODE_OUTPUT;
			*reg_func3_gcr &= ~pinctrl_config->func3_en_mask[pin];
			*reg_func4_gcr &= ~pinctrl_config->func4_en_mask[pin];
			break;
		default:
			LOG_ERR("This function is not supported.");
			return -EINVAL;
		}

	}

	return 0;
}

static int pinctrl_it8xxx2_init(const struct device *dev)
{
	struct gpio_it8xxx2_regs *const gpio_base = GPIO_IT8XXX2_REG_BASE;

	/*
	 * The default value of LPCRSTEN is bit2:1 = 10b(GPD2) in GCR.
	 * If LPC reset is enabled on GPB7, we have to clear bit2:1
	 * to 00b.
	 */
	gpio_base->GPIO_GCR &= ~IT8XXX2_GPIO_LPCRSTEN;

	/*
	 * TODO: If UART2 swaps from bit2:1 to bit6:5 in H group, we
	 * have to set UART1PSEL = 1 in UART1PMR register.
	 */

	return 0;
}

#define PINCTRL_ITE_INIT(inst)                                                          \
	static const struct pinctrl_it8xxx2_config pinctrl_it8xxx2_cfg_##inst = {       \
		.reg_gpcr = (uint8_t *)DT_INST_REG_ADDR(inst),                          \
		.func3_gcr = DT_INST_PROP(inst, func3_gcr),                             \
		.func3_en_mask = DT_INST_PROP(inst, func3_en_mask),                     \
		.func4_gcr = DT_INST_PROP(inst, func4_gcr),                             \
		.func4_en_mask = DT_INST_PROP(inst, func4_en_mask),                     \
		.volt_sel = DT_INST_PROP(inst, volt_sel),                               \
		.volt_sel_mask = DT_INST_PROP(inst, volt_sel_mask),                     \
	};                                                                              \
											\
	DEVICE_DT_INST_DEFINE(inst, &pinctrl_it8xxx2_init,                              \
			      NULL,                                                     \
			      NULL,                                                     \
			      &pinctrl_it8xxx2_cfg_##inst,                              \
			      PRE_KERNEL_1,                                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_ITE_INIT)
