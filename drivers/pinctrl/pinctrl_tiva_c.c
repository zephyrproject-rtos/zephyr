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
#include <inc/hw_gpio.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>

/*
 * TivaWare GPIOPinConfigure() value from pinmux encoding as per the expectation.
 */
#define TIVA_C_TO_PINCFG(pm) \
	((TIVA_C_PINMUX_PORT(pm) << TIVA_C_PINCFG_PORT_SHIFT) | \
	 (TIVA_C_PINMUX_PIN(pm) << TIVA_C_PINCFG_PIN_SHIFT) | \
	 TIVA_C_PINMUX_MUX(pm))

/*
 * Bounded retry count while waiting for a GPIO port peripheral to become
 * ready.
 */
#define TIVA_C_PERIPH_READY_RETRIES 10000U

static const uint32_t gpio_port_base[] = {
	GPIO_PORTA_BASE, GPIO_PORTB_BASE, GPIO_PORTC_BASE,
	GPIO_PORTD_BASE, GPIO_PORTE_BASE, GPIO_PORTF_BASE,
};

static const uint32_t gpio_port_periph[] = {
	SYSCTL_PERIPH_GPIOA, SYSCTL_PERIPH_GPIOB, SYSCTL_PERIPH_GPIOC,
	SYSCTL_PERIPH_GPIOD, SYSCTL_PERIPH_GPIOE, SYSCTL_PERIPH_GPIOF,
};

/*
 * Map the bias and drive bits encoded in the pinmux word to a TivaWare pad
 * type. Only one pull direction can be active at a time; open-drain takes
 * precedence as it changes the output stage rather than only the pull.
 */
static uint32_t tiva_c_pad_type(uint32_t pm)
{
	if (TIVA_C_PINMUX_OPEN_DRAIN(pm) != 0U) {
		return GPIO_PIN_TYPE_OD;
	}

	if (TIVA_C_PINMUX_PULL_UP(pm) != 0U) {
		return GPIO_PIN_TYPE_STD_WPU;
	}

	if (TIVA_C_PINMUX_PULL_DOWN(pm) != 0U) {
		return GPIO_PIN_TYPE_STD_WPD;
	}

	return GPIO_PIN_TYPE_STD;
}

/* Commit-locked NMI pins (PD7, PF0) that must be unlocked before remuxing. */
static bool tiva_c_pin_is_locked(uint8_t port_idx, uint8_t pin_mask)
{
	/* PD7 (NMI) */
	if (port_idx == TIVA_C_PORT_D && pin_mask == BIT(7)) {
		return true;
	}

	/* PF0 (NMI) */
	if (port_idx == TIVA_C_PORT_F && pin_mask == BIT(0)) {
		return true;
	}

	return false;
}

/* JTAG/SWD pins (PC0-PC3) are never unlocked to preserve debug access. */
static bool tiva_c_pin_is_jtag(uint8_t port_idx, uint8_t pin_mask)
{
	return (port_idx == TIVA_C_PORT_C) && ((pin_mask & 0x0FU) != 0U);
}

/* GPIOLOCK/GPIOCR unlock-commit-relock sequence (no DriverLib API exists). */
static void tiva_c_pin_commit_unlock(uint32_t base, uint8_t pin_mask)
{
	HWREG(base + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(base + GPIO_O_CR) |= pin_mask;
	HWREG(base + GPIO_O_LOCK) = 0;
}

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

		/* Refuse to remux JTAG/SWD pins (PC0-PC3) to keep debug access */
		if (tiva_c_pin_is_jtag(port_idx, pin_mask)) {
			return -EINVAL;
		}

		/* Enable the GPIO port clock once per port and wait until it is ready */
		if (!(enabled_ports & BIT(port_idx))) {
			uint32_t retries = TIVA_C_PERIPH_READY_RETRIES;

			SysCtlPeripheralEnable(gpio_port_periph[port_idx]);
			while (!SysCtlPeripheralReady(gpio_port_periph[port_idx])) {
				if (retries-- == 0U) {
					return -ETIMEDOUT;
				}
			}
			enabled_ports |= BIT(port_idx);
		}

		/* Commit-unlock NMI pins (PD7, PF0) before remuxing */
		if (tiva_c_pin_is_locked(port_idx, pin_mask)) {
			tiva_c_pin_commit_unlock(base, pin_mask);
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
					 tiva_c_pad_type(pm));
			break;
		}
	}

	return 0;
}
