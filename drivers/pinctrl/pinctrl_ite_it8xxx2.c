/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_pinctrl_func

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <chip_chipregs.h>

LOG_MODULE_REGISTER(pinctrl_ite_it8xxx2, LOG_LEVEL_ERR);

#define GPIO_IT8XXX2_REG_BASE \
	((struct gpio_it8xxx2_regs *)DT_REG_ADDR(DT_NODELABEL(gpiogcr)))
#define GPIO_GROUP_MEMBERS  8

struct pinctrl_it8xxx2_gpio {
	/* gpio port control register (byte mapping to pin) */
	uint8_t *reg_gpcr;
	/* port driving select control */
	uint8_t *reg_pdsc;
	/* function 3 general control register */
	uintptr_t func3_gcr[GPIO_GROUP_MEMBERS];
	/* function 3 enable mask */
	uint8_t func3_en_mask[GPIO_GROUP_MEMBERS];
	/* function 3 external control register */
	uintptr_t func3_ext[GPIO_GROUP_MEMBERS];
	/* function 3 external mask */
	uint8_t func3_ext_mask[GPIO_GROUP_MEMBERS];
	/* function 4 general control register */
	uintptr_t func4_gcr[GPIO_GROUP_MEMBERS];
	/* function 4 enable mask */
	uint8_t func4_en_mask[GPIO_GROUP_MEMBERS];
	/* Input voltage selection */
	uintptr_t volt_sel[GPIO_GROUP_MEMBERS];
	/* Input voltage selection mask */
	uint8_t volt_sel_mask[GPIO_GROUP_MEMBERS];
};

struct pinctrl_it8xxx2_ksi_kso {
	/*
	 * KSI[7:0]/KSO[15:8]/KSO[7:0] port gpio control register
	 * (bit mapping to pin)
	 */
	uint8_t *reg_gctrl;
	/* KSI[7:0]/KSO[15:8]/KSO[7:0] port control register */
	uint8_t *reg_ctrl;
	/*
	 * KSO push-pull/open-drain bit of KSO[15:0] control register
	 * (this bit apply to all pins)
	 */
	int pp_od_mask;
	/*
	 * KSI/KSO pullup bit of KSI[7:0]/KSO[15:0] control register
	 * (this bit apply to all pins)
	 */
	int pullup_mask;
};

struct pinctrl_it8xxx2_config {
	bool gpio_group;
	union {
		struct pinctrl_it8xxx2_gpio gpio;
		struct pinctrl_it8xxx2_ksi_kso ksi_kso;
	};
};

static int pinctrl_it8xxx2_set(const pinctrl_soc_pin_t *pins)
{
	const struct pinctrl_it8xxx2_config *pinctrl_config = pins->pinctrls->config;
	const struct pinctrl_it8xxx2_gpio *gpio = &(pinctrl_config->gpio);
	uint32_t pincfg = pins->pincfg;
	uint8_t pin = pins->pin;
	volatile uint8_t *reg_gpcr = (uint8_t *)gpio->reg_gpcr + pin;
	volatile uint8_t *reg_volt_sel = (uint8_t *)(gpio->volt_sel[pin]);
	volatile uint8_t *reg_pdsc = (uint8_t *)gpio->reg_pdsc;

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

	/*
	 * Since not all GPIOs support voltage selection, configure voltage
	 * selection register only if it is present.
	 */
	if (reg_volt_sel != NULL) {
		/* Setting voltage 3.3V or 1.8V. */
		switch (IT8XXX2_DT_PINCFG_VOLTAGE(pincfg)) {
		case IT8XXX2_VOLTAGE_3V3:
			/* Input voltage selection 3.3V. */
			*reg_volt_sel &= ~gpio->volt_sel_mask[pin];
			break;
		case IT8XXX2_VOLTAGE_1V8:
			__ASSERT(!(IT8XXX2_DT_PINCFG_PUPDR(pincfg)
				   == IT8XXX2_PULL_UP),
			"Don't enable internal pullup if 1.8V voltage is used");
			/* Input voltage selection 1.8V. */
			*reg_volt_sel |= gpio->volt_sel_mask[pin];
			break;
		default:
			LOG_ERR("The voltage selection is not supported");
			return -EINVAL;
		}
	}

	/* Setting tri-state mode. */
	if (IT8XXX2_DT_PINCFG_IMPEDANCE(pincfg)) {
		*reg_gpcr |= (GPCR_PORT_PIN_MODE_PULLUP |
			      GPCR_PORT_PIN_MODE_PULLDOWN);
	}

	/* Driving current selection. */
	if (reg_pdsc != NULL &&
		IT8XXX2_DT_PINCFG_DRIVE_CURRENT(pincfg) != IT8XXX2_DRIVE_DEFAULT) {
		if (IT8XXX2_DT_PINCFG_DRIVE_CURRENT(pincfg) & IT8XXX2_PDSCX_MASK) {
			/* Driving current selects low. */
			*reg_pdsc |= BIT(pin);
		} else {
			/* Driving current selects high. */
			*reg_pdsc &= ~BIT(pin);
		}
	}

	return 0;
}

static int pinctrl_gpio_it8xxx2_configure_pins(const pinctrl_soc_pin_t *pins)
{
	const struct pinctrl_it8xxx2_config *pinctrl_config = pins->pinctrls->config;
	const struct pinctrl_it8xxx2_gpio *gpio = &(pinctrl_config->gpio);
	uint8_t pin = pins->pin;
	volatile uint8_t *reg_gpcr = (uint8_t *)gpio->reg_gpcr + pin;
	volatile uint8_t *reg_func3_gcr = (uint8_t *)(gpio->func3_gcr[pin]);
	volatile uint8_t *reg_func4_gcr = (uint8_t *)(gpio->func4_gcr[pin]);
	volatile uint8_t *reg_func3_ext = (uint8_t *)(gpio->func3_ext[pin]);

	/* Handle PIN configuration. */
	if (pinctrl_it8xxx2_set(pins)) {
		LOG_ERR("Pin configuration is invalid.");
		return -EINVAL;
	}

	/*
	 * Default input mode prevents leakage during changes to extended
	 * setting (e.g. enabling i2c functionality on GPIO E1/E2 on IT82002)
	 */
	*reg_gpcr = (*reg_gpcr | GPCR_PORT_PIN_MODE_INPUT) &
		     ~GPCR_PORT_PIN_MODE_OUTPUT;

	/*
	 * If pincfg is input, we don't need to handle
	 * alternate function.
	 */
	if (IT8XXX2_DT_PINCFG_INPUT(pins->pincfg)) {
		return 0;
	}

	/*
	 * Handle alternate function.
	 */
	if (reg_func3_gcr != NULL) {
		*reg_func3_gcr &= ~gpio->func3_en_mask[pin];
	}
	/* Ensure that func3-ext setting is in default state. */
	if (reg_func3_ext != NULL) {
		*reg_func3_ext &= ~gpio->func3_ext_mask[pin];
	}

	switch (pins->alt_func) {
	case IT8XXX2_ALT_FUNC_1:
		/* Func1: Alternate function will be set below. */
		break;
	case IT8XXX2_ALT_FUNC_2:
		/* Func2: WUI function: pin has been set as input above.*/
		return 0;
	case IT8XXX2_ALT_FUNC_3:
		/*
		 * Func3: In addition to the alternate setting above,
		 *        Func3 also need to set the general control.
		 */
		if (reg_func3_gcr != NULL) {
			*reg_func3_gcr |= gpio->func3_en_mask[pin];
		}
		/* Func3-external: Some pins require external setting. */
		if (reg_func3_ext != NULL) {
			*reg_func3_ext |= gpio->func3_ext_mask[pin];
		}
		break;
	case IT8XXX2_ALT_FUNC_4:
		/*
		 * Func4: In addition to the alternate setting above,
		 *        Func4 also need to set the general control.
		 */
		*reg_func4_gcr |= gpio->func4_en_mask[pin];
		break;
	case IT8XXX2_ALT_DEFAULT:
		*reg_func3_gcr &= ~gpio->func3_en_mask[pin];
		*reg_func4_gcr &= ~gpio->func4_en_mask[pin];
		return 0;
	default:
		LOG_ERR("This function is not supported.");
		return -EINVAL;
	}

	/* Common settings for alternate function. */
	*reg_gpcr &= ~(GPCR_PORT_PIN_MODE_INPUT |
		       GPCR_PORT_PIN_MODE_OUTPUT);

	return 0;
}

static int pinctrl_kscan_it8xxx2_set(const pinctrl_soc_pin_t *pins)
{
	const struct pinctrl_it8xxx2_config *pinctrl_config = pins->pinctrls->config;
	const struct pinctrl_it8xxx2_ksi_kso *ksi_kso = &(pinctrl_config->ksi_kso);
	volatile uint8_t *reg_ctrl = ksi_kso->reg_ctrl;
	uint8_t pullup_mask = ksi_kso->pullup_mask;
	uint8_t pp_od_mask = ksi_kso->pp_od_mask;
	uint32_t pincfg = pins->pincfg;

	/*
	 * Enable or disable internal pull-up (this bit apply to all pins):
	 * If KSI[7:0]/KSO[15:0] is in KBS mode , setting 1 enables the internal
	 * pull-up (KSO[17:16] setting internal pull-up by GPIO port GPCR register).
	 * If KSI[7:0]/KSO[15:0] is in GPIO mode, then this bit is always disabled.
	 */
	switch (IT8XXX2_DT_PINCFG_PULLUP(pincfg)) {
	case IT8XXX2_PULL_PIN_DEFAULT:
		/* Disable internal pulll-up */
		*reg_ctrl &= ~pullup_mask;
		break;
	case IT8XXX2_PULL_UP:
		*reg_ctrl |= pullup_mask;
		break;
	default:
		LOG_ERR("This pull level is not supported.");
		return -EINVAL;
	}

	/*
	 * Set push-pull or open-drain mode (this bit apply to all pins):
	 * KSI[7:0] doesn't support push-pull and open-drain settings in kbs mode.
	 * If KSO[17:0] is in KBS mode, setting 1 selects open-drain mode,
	 * setting 0 selects push-pull mode.
	 * If KSO[15:0] is in GPIO mode, then this bit is always disabled.
	 */
	if (pp_od_mask != NO_FUNC) {
		switch (IT8XXX2_DT_PINCFG_PP_OD(pincfg)) {
		case IT8XXX2_PUSH_PULL:
			*reg_ctrl &= ~pp_od_mask;
			break;
		case IT8XXX2_OPEN_DRAIN:
			*reg_ctrl |= pp_od_mask;
			break;
		default:
			LOG_ERR("This pull mode is not supported.");
			return -EINVAL;
		}
	}

	return 0;
}

static int pinctrl_kscan_it8xxx2_configure_pins(const pinctrl_soc_pin_t *pins)
{
	const struct pinctrl_it8xxx2_config *pinctrl_config = pins->pinctrls->config;
	const struct pinctrl_it8xxx2_ksi_kso *ksi_kso = &(pinctrl_config->ksi_kso);

	/* Set a pin of KSI[7:0]/KSO[15:0] to pullup, push-pull/open-drain */
	if (pinctrl_kscan_it8xxx2_set(pins)) {
		return -EINVAL;
	}

#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
	uint8_t pin_mask = BIT(pins->pin);
	volatile uint8_t *reg_gctrl = ksi_kso->reg_gctrl;

	switch (pins->alt_func) {
	case IT8XXX2_ALT_FUNC_1:
		/* Set a pin of KSI[7:0]/KSO[15:0] to kbs mode */
		*reg_gctrl &= ~pin_mask;
		break;
	case IT8XXX2_ALT_DEFAULT:
		/* Set a pin of KSI[7:0]/KSO[15:0] to gpio mode */
		*reg_gctrl |= pin_mask;
		break;
#elif CONFIG_SOC_IT8XXX2_REG_SET_V2
	uint8_t pin = pins->pin;
	volatile uint8_t *reg_gctrl = ksi_kso->reg_gctrl + pin;

	switch (pins->alt_func) {
	case IT8XXX2_ALT_FUNC_1:
		/* Set a pin of KSI[7:0]/KSO[15:0] to kbs mode */
		*reg_gctrl &= ~(GPCR_PORT_PIN_MODE_INPUT |
				GPCR_PORT_PIN_MODE_OUTPUT);
		break;
	case IT8XXX2_ALT_DEFAULT:
		/* Set a pin of KSI[7:0]/KSO[15:0] to gpio mode */
		*reg_gctrl = (*reg_gctrl | GPCR_PORT_PIN_MODE_INPUT) &
			      ~GPCR_PORT_PIN_MODE_OUTPUT;
		break;
#endif
	default:
		LOG_ERR("Alternate function not supported");
		return -ENOTSUP;
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);
	const struct pinctrl_it8xxx2_config *pinctrl_config;
	int status;

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_config = pins[i].pinctrls->config;

		if (pinctrl_config->gpio_group) {
			status = pinctrl_gpio_it8xxx2_configure_pins(&pins[i]);
		} else {
			status = pinctrl_kscan_it8xxx2_configure_pins(&pins[i]);
		}

		if (status < 0) {
			LOG_ERR("%s pin%d configuration is invalid.",
				pins[i].pinctrls->name, pins[i].pin);
			return status;
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

#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V2
#if defined(CONFIG_I2C_ITE_ENHANCE) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i2c5))
	const struct gpio_dt_spec scl_gpios = GPIO_DT_SPEC_GET(DT_NODELABEL(i2c5), scl_gpios);
	const struct gpio_dt_spec sda_gpios = GPIO_DT_SPEC_GET(DT_NODELABEL(i2c5), sda_gpios);

	/*
	 * When setting these pins as I2C alternate mode and then setting
	 * GCR7 or func3-ext of GPIO extended, it will cause leakage.
	 * In order to prevent leakage, it must be set to GPIO INPUT mode.
	 */
	/* Set I2C5 SCL as GPIO input to prevent leakage */
	gpio_pin_configure_dt(&scl_gpios, GPIO_INPUT);
	/* Set I2C5 SDA as GPIO input to prevent leakage */
	gpio_pin_configure_dt(&sda_gpios, GPIO_INPUT);
#endif
	/*
	 * Swap the default I2C2 SMCLK2/SMDAT2 pins from GPC7/GPD0 to GPF6/GPF7,
	 * and I2C3 SMCLK3/SMDAT3 pins from GPB2/GPB5 to GPH1/GPH2,
	 * and I2C5 SMCLK5/SMDAT5 pins from GPE1/GPE2 to GPA4/GPA5,
	 */
	gpio_base->GPIO_GCR7 &= ~(IT8XXX2_GPIO_SMB2PS |
				  IT8XXX2_GPIO_SMB3PS |
				  IT8XXX2_GPIO_SMB5PS);
#endif
	return 0;
}

#define INIT_UNION_CONFIG(inst)                                                        \
	COND_CODE_1(DT_INST_PROP(inst, gpio_group),                                    \
		(.gpio = {                                                             \
			 .reg_gpcr = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 0),      \
			 .reg_pdsc = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 1),      \
			 .func3_gcr = DT_INST_PROP(inst, func3_gcr),                   \
			 .func3_en_mask = DT_INST_PROP(inst, func3_en_mask),           \
			 .func3_ext = DT_INST_PROP_OR(inst, func3_ext, {0}),           \
			 .func3_ext_mask = DT_INST_PROP_OR(inst, func3_ext_mask, {0}), \
			 .func4_gcr = DT_INST_PROP(inst, func4_gcr),                   \
			 .func4_en_mask = DT_INST_PROP(inst, func4_en_mask),           \
			 .volt_sel = DT_INST_PROP(inst, volt_sel),                     \
			 .volt_sel_mask = DT_INST_PROP(inst, volt_sel_mask),           \
		}),                                                                    \
		(.ksi_kso = {                                                          \
			 .reg_gctrl = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 0),     \
			 .reg_ctrl = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 1),      \
			 .pp_od_mask = (uint8_t)DT_INST_PROP(inst, pp_od_mask),        \
			 .pullup_mask = (uint8_t)DT_INST_PROP(inst, pullup_mask),      \
		})                                                                     \
	)

#define PINCTRL_ITE_INIT(inst)                                                     \
	static const struct pinctrl_it8xxx2_config pinctrl_it8xxx2_cfg_##inst = {  \
		.gpio_group = DT_INST_PROP(inst, gpio_group),                      \
		{                                                                  \
			INIT_UNION_CONFIG(inst)                                    \
		}                                                                  \
	};                                                                         \
										   \
	DEVICE_DT_INST_DEFINE(inst, &pinctrl_it8xxx2_init,                         \
			      NULL,                                                \
			      NULL,                                                \
			      &pinctrl_it8xxx2_cfg_##inst,                         \
			      PRE_KERNEL_1,                                        \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                 \
			      NULL);
DT_INST_FOREACH_STATUS_OKAY(PINCTRL_ITE_INIT)
