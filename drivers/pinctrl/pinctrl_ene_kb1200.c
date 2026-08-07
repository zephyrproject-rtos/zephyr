/*
 * Copyright (c) ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_pinctrl

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/ene-kb1200-pinctrl.h>
#include <zephyr/sys/util.h>
#include <reg/gcfg.h>
#include <reg/gpio.h>

/*
 *  PINMUX_FUNC_A : GPIO        Function
 *  PINMUX_FUNC_B : AltOutput 1 Function
 *  PINMUX_FUNC_C : AltOutput 2 Function
 *  PINMUX_FUNC_D : AltOutput 3 Function
 *  PINMUX_FUNC_E : AltOutput 4 Function
 *
 *  GPIO Alternate Output Function Selection
 * (PINMUX_FUNC_A) (PINMUX_FUNC_B) (PINMUX_FUNC_C) (PINMUX_FUNC_D) (PINMUX_FUNC_E)
 *  GPIO00          PWMLED0         PWM8
 *  GPIO01          SER_RXD1        UART_SIN        SBUD_DAT
 *  GPIO03          SER_TXD1        UART_SOUT       SBUD_CLK
 *  GPIO22          ESBDAT          PWM9
 *  GPIO28          32KOUT          SERCLK2
 *  GPIO36          UARTSOUT        SERTXD2
 *  GPIO5C          KSO6            P80DAT
 *  GPIO5D          KSO7            P80CLK
 *  GPIO5E          KSO8            SERRXD1
 *  GPIO5F          KSO9            SERTXD1
 *  GPIO71          SDA8            UARTRTS
 *  GPIO38          SCL4            PWM1
 */

/*
 * f is function number
 * b[7:5] = pin bank
 * b[4:0] = pin position in bank
 * b[11:8] = function
 */

#define ENE_KB1200_PINMUX_PIN(p)       FIELD_GET(GENMASK(4, 0), p)
#define ENE_KB1200_PINMUX_PORT(p)      FIELD_GET(GENMASK(7, 5), p)
#define ENE_KB1200_PINMUX_FUNC(p)      FIELD_GET(GENMASK(11, 8), p)
#define ENE_KB1200_PINMUX_PORT_PIN(p)  FIELD_GET(GENMASK(7, 0), p)

static const uint32_t gcfg_reg_addr = DT_REG_ADDR(DT_NODELABEL(gcfg));
static const uint32_t gpio_reg_bases[] = {
	DT_REG_ADDR(DT_NODELABEL(gpio0x1x)),
	DT_REG_ADDR(DT_NODELABEL(gpio2x3x)),
	DT_REG_ADDR(DT_NODELABEL(gpio4x5x)),
	DT_REG_ADDR(DT_NODELABEL(gpio6x7x)),
};

static int kb1200_config_pin(uint32_t gpio, uint32_t conf, uint32_t func)
{
	uint32_t port = ENE_KB1200_PINMUX_PORT(gpio);
	uint32_t pin = (uint32_t)ENE_KB1200_PINMUX_PIN(gpio);
	struct gpio_regs *gpio_regs = (struct gpio_regs *)gpio_reg_bases[port];
	struct gcfg_regs *gcfg_regs = (struct gcfg_regs *)gcfg_reg_addr;

	if (port >= NUM_KB1200_GPIO_PORTS) {
		return -EINVAL;
	}

	if (func == PINMUX_FUNC_GPIO) { /* only GPIO function */
		WRITE_BIT(gpio_regs->GPIOFS, pin, 0);
	} else {
		func -= 1; /*for change to GPIOALT setting value*/
		switch (gpio) {
		case GPIO00_PWMLED0_PWM8:
			WRITE_BIT(gcfg_regs->GPIOALT, 0, func);
			break;
		case GPIO01_SERRXD1_UARTSIN:
			gcfg_regs->GPIOMUX = (gcfg_regs->GPIOMUX & ~(3 << 9)) | (func << 9);
			break;
		case GPIO03_SERTXD1_UARTSOUT:
			gcfg_regs->GPIOMUX = (gcfg_regs->GPIOMUX & ~(3 << 9)) | (func << 9);
			break;
		case GPIO22_ESBDAT_PWM9:
			WRITE_BIT(gcfg_regs->GPIOALT, 1, func);
			break;
		case GPIO28_32KOUT_SERCLK2:
			WRITE_BIT(gcfg_regs->GPIOALT, 2, func);
			break;
		case GPIO36_UARTSOUT_SERTXD2:
			WRITE_BIT(gcfg_regs->GPIOALT, 3, func);
			break;
		case GPIO5C_KSO6_P80DAT:
			WRITE_BIT(gcfg_regs->GPIOALT, 4, func);
			break;
		case GPIO5D_KSO7_P80CLK:
			WRITE_BIT(gcfg_regs->GPIOALT, 5, func);
			break;
		case GPIO5E_KSO8_SERRXD1:
			WRITE_BIT(gcfg_regs->GPIOALT, 6, func);
			break;
		case GPIO5F_KSO9_SERTXD1:
			WRITE_BIT(gcfg_regs->GPIOALT, 7, func);
			break;
		case GPIO71_SDA8_UARTRTS:
			WRITE_BIT(gcfg_regs->GPIOALT, 8, func);
			break;
		case GPIO38_SCL4_PWM1:
			WRITE_BIT(gcfg_regs->GPIOALT, 9, func);
			break;
		}
		WRITE_BIT(gpio_regs->GPIOFS, pin, 1);
	}
	/*Input always enable for loopback*/
	WRITE_BIT(gpio_regs->GPIOIE, pin, 1);

	if (conf & BIT(ENE_KB1200_NO_PUD_POS)) {
		WRITE_BIT(gpio_regs->GPIOPU, pin, 0);
		WRITE_BIT(gpio_regs->GPIOPD, pin, 0);
	}
	if (conf & BIT(ENE_KB1200_PU_POS)) {
		WRITE_BIT(gpio_regs->GPIOPU, pin, 1);
	}
	if (conf & BIT(ENE_KB1200_PD_POS)) {
		WRITE_BIT(gpio_regs->GPIOPD, pin, 1);
	}

	if (conf & BIT(ENE_KB1200_OUT_DIS_POS)) {
		WRITE_BIT(gpio_regs->GPIOOE, pin, 0);
	}
	if (conf & BIT(ENE_KB1200_OUT_EN_POS)) {
		WRITE_BIT(gpio_regs->GPIOOE, pin, 1);
	}

	if (conf & BIT(ENE_KB1200_OUT_LO_POS)) {
		WRITE_BIT(gpio_regs->GPIOD, pin, 0);
	}
	if (conf & BIT(ENE_KB1200_OUT_HI_POS)) {
		WRITE_BIT(gpio_regs->GPIOD, pin, 1);
	}

	if (conf & BIT(ENE_KB1200_PUSH_PULL_POS)) {
		WRITE_BIT(gpio_regs->GPIOOD, pin, 0);
	}
	if (conf & BIT(ENE_KB1200_OPEN_DRAIN_POS)) {
		WRITE_BIT(gpio_regs->GPIOOD, pin, 1);
	}

	if (conf & BIT(ENE_KB1200_PIN_LOW_POWER_POS)) {
		WRITE_BIT(gpio_regs->GPIOLV, pin, 1);
	}

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	uint32_t portpin, pinmux, func;
	int ret;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinmux = pins[i];

		func = ENE_KB1200_PINMUX_FUNC(pinmux);
		if (func >= PINMUX_FUNC_MAX) {
			return -EINVAL;
		}

		portpin = ENE_KB1200_PINMUX_PORT_PIN(pinmux);

		ret = kb1200_config_pin(portpin, pinmux, func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
