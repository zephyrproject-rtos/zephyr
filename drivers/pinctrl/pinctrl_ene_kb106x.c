/*
 * Copyright (c) ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_pinctrl

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/ene-kb106x-pinctrl.h>
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
 *  GPIO0B          ESBCLK          SCL5
 *  GPIO0C          ESBDAT          SDA5
 *  GPIO0D          RLC_TX2         SDA4
 *  GPIO16          SER_TXD         UART_SOUT       SBUD_CLK
 *  GPIO17          SER_RXD         UART_SIN        SBUD_DAT
 *  GPIO19          PWM3            PWMLED0
 *  GPIO30          SER_TXD         NKROKSI0
 *  GPIO48          KSO16           UART_SOUT2
 *  GPIO4C          PSCLK2          SCL3
 *  GPIO4D          SDAT2           SDA3
 *  GPIO4E          PSCLK3          KSO18
 *  GPIO4F          PSDAT3          KSO19
 *  GPIO4A          PSCLK1          SCL2            USBDM
 *  GPIO4B          PSDAT1          SDA2            USBDP
 *  GPIO01          ESPI_ALERT
 *  GPIO03          ESPI_CS
 *  GPIO07          ESPI_RST
 */

/*
 * f is function number
 * b[7:5] = pin bank
 * b[4:0] = pin position in bank
 * b[11:8] = function
 */

#define ENE_KB106X_PINMUX_PIN(p)      FIELD_GET(GENMASK(4, 0), p)
#define ENE_KB106X_PINMUX_PORT(p)     FIELD_GET(GENMASK(7, 5), p)
#define ENE_KB106X_PINMUX_FUNC(p)     FIELD_GET(GENMASK(11, 8), p)
#define ENE_KB106X_PINMUX_PORT_PIN(p) FIELD_GET(GENMASK(7, 0), p)

static const uint32_t gcfg_reg_addr = DT_REG_ADDR_BY_NAME(DT_NODELABEL(gcfg), gcfg);
static const uint32_t gpio_reg_bases[] = {
	DT_REG_ADDR(DT_NODELABEL(gpio0x1x)),
	DT_REG_ADDR(DT_NODELABEL(gpio2x3x)),
	DT_REG_ADDR(DT_NODELABEL(gpio4x5x)),
	DT_REG_ADDR(DT_NODELABEL(gpio6x7x)),
	DT_REG_ADDR(DT_NODELABEL(egpio0x1x)),
};

static int kb106x_config_pin(uint32_t gpio, uint32_t conf, uint32_t func)
{
	uint32_t port = ENE_KB106X_PINMUX_PORT(gpio);
	uint32_t pin = (uint32_t)ENE_KB106X_PINMUX_PIN(gpio);
	struct gpio_regs *gpio_regs = (struct gpio_regs *)gpio_reg_bases[port];
	struct gcfg_regs *gcfg_regs = (struct gcfg_regs *)gcfg_reg_addr;

	if (port >= ARRAY_SIZE(gpio_reg_bases)) {
		return -EINVAL;
	}

	if (func == PINMUX_FUNC_GPIO) {
		/* only GPIO function */
		WRITE_BIT(gpio_regs->GPIOFS, pin, 0);
	} else {
		func -= 1;
		/*for change to GPIOALT setting value*/
		switch (gpio) {
		case GPIO0B_ESBCLK_SCL5:
			WRITE_BIT(gcfg_regs->GPIOALT, 0, func);
			break;
		case GPIO0C_ESBDAT_SDA5:
			WRITE_BIT(gcfg_regs->GPIOALT, 1, func);
			break;
		case GPIO0D_RLCTX2_SDA4:
			WRITE_BIT(gcfg_regs->GPIOALT, 2, func);
			break;
		case GPIO16_SERTXD_UARTSOUT_SBUDCLK:
		case GPIO17_SERRXD_UARTSIN_SBUDDAT:
			gcfg_regs->GPIOMUX = (gcfg_regs->GPIOMUX & ~(3 << 9)) | (func << 9);
			break;
		case GPIO19_PWM3_PWMLED0:
			WRITE_BIT(gcfg_regs->GPIOALT, 3, func);
			break;
		case GPIO30_SERTXD_NKROKSI0:
			WRITE_BIT(gcfg_regs->GPIOALT, 5, func);
			break;
		case GPIO48_KSO16_UART_SOUT2:
			WRITE_BIT(gcfg_regs->GPIOALT, 6, func);
			break;
		case GPIO4C_PSCLK2_SCL3:
			WRITE_BIT(gcfg_regs->GPIOALT, 7, func);
			break;
		case GPIO4D_SDAT2_SDA3:
			WRITE_BIT(gcfg_regs->GPIOALT, 8, func);
			break;
		case GPIO4E_PSCLK3_KSO18:
			WRITE_BIT(gcfg_regs->GPIOALT, 9, func);
			break;
		case GPIO4F_PSDAT3_KSO19:
			WRITE_BIT(gcfg_regs->GPIOALT, 10, func);
			break;
		case GPIO4A_PSCLK1_SCL2_USBDM:
			gcfg_regs->GPIOALT = (gcfg_regs->GPIOALT & ~(3 << 24)) | (func << 24);
			break;
		case GPIO4B_PSDAT1_SDA2_USBDP:
			gcfg_regs->GPIOALT = (gcfg_regs->GPIOALT & ~(3 << 26)) | (func << 26);
			break;
		case GPIO60_SHICS:
		case GPIO61_SHICLK:
		case GPIO62_SHIDO:
		case GPIO78_SHIDI:
			gcfg_regs->GPIOMUX = (gcfg_regs->GPIOMUX & ~(3 << 0)) | (3 << 0);
			break;
		case GPIO5A_SHR_SPICS:
		case GPIO58_SHR_SPICLK:
		case GPIO5C_SHR_MOSI:
		case GPIO5B_SHR_MISO:
			gcfg_regs->GPIOMUX = (gcfg_regs->GPIOMUX & ~(3 << 0)) | (2 << 0);
			break;
		case GPIO01_ESPI_ALERT:
		case GPIO03_ESPI_CS:
		case GPIO07_ESPI_RST:
			WRITE_BIT(gcfg_regs->GPIOALT, 3, func);
			break;
		default:
			break;
		}
		WRITE_BIT(gpio_regs->GPIOFS, pin, 1);
#ifdef CONFIG_PINCTRL_ENE_KB106X_ALT_OUTPUT_LOOKBACK
		/* default Input always enable for loopback */
		WRITE_BIT(gpio_regs->GPIOIE, pin, 1);
#endif /* CONFIG_PINCTRL_ENE_KB106X_ALT_OUTPUT_LOOKBACK */
	}

	/* pull-up/pull-down function */
	if (conf & BIT(ENE_KB106X_NO_PUD_POS)) {
		WRITE_BIT(gpio_regs->GPIOPU, pin, 0);
	}
	if (conf & BIT(ENE_KB106X_PU_POS)) {
		WRITE_BIT(gpio_regs->GPIOPU, pin, 1);
	}
	if (conf & BIT(ENE_KB106X_PD_POS)) {
		/* KB106x not support */
	}
	/* output high/low, output type function */
	if (conf & BIT(ENE_KB106X_OUT_LO_POS)) {
		WRITE_BIT(gpio_regs->GPIOD, pin, 0);
	}
	if (conf & BIT(ENE_KB106X_OUT_HI_POS)) {
		WRITE_BIT(gpio_regs->GPIOD, pin, 1);
	}
	if (conf & BIT(ENE_KB106X_PUSH_PULL_POS)) {
		WRITE_BIT(gpio_regs->GPIOOD, pin, 0);
	}
	if (conf & BIT(ENE_KB106X_OUT_DIS_POS)) {
		WRITE_BIT(gpio_regs->GPIOOE, pin, 0);
		WRITE_BIT(gpio_regs->GPIOOD, pin, 0);
	}
	if (conf & BIT(ENE_KB106X_OUT_EN_POS)) {
		if (conf & BIT(ENE_KB106X_OPEN_DRAIN_POS)) {
			WRITE_BIT(gpio_regs->GPIOOD, pin, 1);
		}
		WRITE_BIT(gpio_regs->GPIOOE, pin, 1);
	}
	/* low voltage mode(support 1.8v Vih/Vil) */
	if (conf & BIT(ENE_KB106X_PIN_LOW_POWER_POS)) {
		WRITE_BIT(gpio_regs->GPIOLV, pin, 1);
	}
	/* input function */
	if (conf & BIT(ENE_KB106X_IN_DIS_POS)) {
		WRITE_BIT(gpio_regs->GPIOIE, pin, 0);
	}
	if (conf & BIT(ENE_KB106X_IN_EN_POS)) {
		WRITE_BIT(gpio_regs->GPIOIE, pin, 1);
	}
	/* drive strength function(4mA/16mA) */
	if (conf & BIT(ENE_KB106X_DRIVING_POS)) {
		WRITE_BIT(gpio_regs->GPIODC, pin, 1);
	} else {
		WRITE_BIT(gpio_regs->GPIODC, pin, 0);
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
		func = ENE_KB106X_PINMUX_FUNC(pinmux);
		if (func >= PINMUX_FUNC_MAX) {
			return -EINVAL;
		}

		portpin = ENE_KB106X_PINMUX_PORT_PIN(pinmux);
		ret = kb106x_config_pin(portpin, pinmux, func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
