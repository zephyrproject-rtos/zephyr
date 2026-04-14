/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Linumiz
 * Author: Sri Surya <srisurya@linumiz.com>
 */

/* pinctrl_tiva_c.c - TI Tiva C Series pin controller driver */

#define DT_DRV_COMPAT ti_tiva_c_pinctrl

#include <zephyr/drivers/pinctrl.h>

/* TivaWare HAL */
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <inc/hw_memmap.h>

/*
 * TivaWare GPIOPinConfigure() value from pinmux encoding as per the expectation.
 */
#define TIVA_C_TO_PINCFG(pm) \
	((TIVA_C_PINMUX_PORT(pm) << TIVA_C_PINCFG_PORT_SHIFT) | \
	 (TIVA_C_PINMUX_PIN(pm) << TIVA_C_PINCFG_PIN_SHIFT) | \
	 TIVA_C_PINMUX_MUX(pm))

static const uint32_t gpio_port_base[] = {
	GPIO_PORTA_BASE, GPIO_PORTB_BASE, GPIO_PORTC_BASE,
	GPIO_PORTD_BASE, GPIO_PORTE_BASE, GPIO_PORTF_BASE,
};

static const uint32_t gpio_port_periph[] = {
	SYSCTL_PERIPH_GPIOA, SYSCTL_PERIPH_GPIOB, SYSCTL_PERIPH_GPIOC,
	SYSCTL_PERIPH_GPIOD, SYSCTL_PERIPH_GPIOE, SYSCTL_PERIPH_GPIOF,
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins,
			   uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint32_t pm;
	uint8_t port_idx;
	uint8_t pin_mask;
	uint32_t base;
	uint8_t enabled_ports = 0;

	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		pm = pins[i].pinmux;
		port_idx = TIVA_C_PINMUX_PORT(pm);
		if (port_idx >= ARRAY_SIZE(gpio_port_base)) {
			return -EINVAL;
		}
		pin_mask = BIT(TIVA_C_PINMUX_PIN(pm));
		base = gpio_port_base[port_idx];

		/* Enable the GPIO port clock once per port and wait until it is ready */
		if (!(enabled_ports & BIT(port_idx))) {
			SysCtlPeripheralEnable(gpio_port_periph[port_idx]);
			while (!SysCtlPeripheralReady(gpio_port_periph[port_idx])) {
			}
			enabled_ports |= BIT(port_idx);
		}

		/* Set alternate-function mux */
		GPIOPinConfigure(TIVA_C_TO_PINCFG(pm));

		/* Configure pin type */
		switch (TIVA_C_PINMUX_TYPE(pm)) {
		case TIVA_C_TYPE_UART:
			GPIOPinTypeUART(base, pin_mask);
			break;
		case TIVA_C_TYPE_I2C:
			GPIOPinTypeI2C(base, pin_mask);
			break;
		case TIVA_C_TYPE_I2C_SCL:
			GPIOPinTypeI2CSCL(base, pin_mask);
			break;
		case TIVA_C_TYPE_SSI:
			GPIOPinTypeSSI(base, pin_mask);
			break;
		case TIVA_C_TYPE_CAN:
			GPIOPinTypeCAN(base, pin_mask);
			break;
		case TIVA_C_TYPE_PWM:
			GPIOPinTypePWM(base, pin_mask);
			break;
		default:
			GPIOPadConfigSet(base, pin_mask,
					 GPIO_STRENGTH_2MA,
					 GPIO_PIN_TYPE_STD);
			break;
		}
	}

	return 0;
}
