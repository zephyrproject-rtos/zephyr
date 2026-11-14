/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Pin control driver for the Microchip PIO (group g1).
 *
 * Pins are selected with the pinmux encoding of mchp_pinctrl_pinmux_pic32c.h:
 * peripheral functions A to D are routed with PIO_ABCDSR and PIO_PDR, GPIO
 * pins are handed to the PIO with PIO_PER, and extra functions are left to
 * the peripheral that owns them (analog inputs, wake-up inputs, crystal and
 * debug pins).
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <soc.h>

/* clang-format off */
#define MCHP_PIO_ADDR_OR_NULL(nodelabel)                                                           \
	COND_CODE_1(DT_NODE_EXISTS(DT_NODELABEL(nodelabel)),                                       \
		    ((pio_registers_t *)DT_REG_ADDR(DT_NODELABEL(nodelabel))), (NULL))

static pio_registers_t *const mchp_pio_regs[] = {
	MCHP_PIO_ADDR_OR_NULL(pioa),
	MCHP_PIO_ADDR_OR_NULL(piob),
	MCHP_PIO_ADDR_OR_NULL(pioc),
	MCHP_PIO_ADDR_OR_NULL(piod),
	MCHP_PIO_ADDR_OR_NULL(pioe),
};
/* clang-format on */

#define MCHP_PIO_PORT_B 1U

/*
 * PB4, PB5, PB6, PB7 and PB12 default to system functions (JTAG/SWD, TRACESWO
 * and ERASE). Setting the matching CCFG_SYSIO bit gives the pin to the PIO.
 */
#define MCHP_PIO_SYSIO_PINS (BIT(4) | BIT(5) | BIT(6) | BIT(7) | BIT(12))

static void pinctrl_sysio(uint8_t port, uint8_t pin, bool system_function)
{
	if (port != MCHP_PIO_PORT_B || (BIT(pin) & MCHP_PIO_SYSIO_PINS) == 0U) {
		return;
	}

	if (system_function) {
		MATRIX_REGS->CCFG_SYSIO &= ~BIT(pin);
	} else {
		MATRIX_REGS->CCFG_SYSIO |= BIT(pin);
	}
}

static void pinctrl_set_flags(pio_registers_t *pio, uint32_t mask, uint16_t pinflag)
{
	/* Pull-up and pull-down are exclusive: disable both before enabling one. */
	pio->PIO_PUDR = mask;
	pio->PIO_PPDDR = mask;
	if ((pinflag & MCHP_PINCTRL_PULLUP) != 0U) {
		pio->PIO_PUER = mask;
	} else if ((pinflag & MCHP_PINCTRL_PULLDOWN) != 0U) {
		pio->PIO_PPDER = mask;
	}

	if ((pinflag & MCHP_PINCTRL_OPENDRAIN) != 0U) {
		pio->PIO_MDER = mask;
	} else {
		pio->PIO_MDDR = mask;
	}
}

static void pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint8_t port = MCHP_PINMUX_PORT_GET(pin->pinmux);
	uint8_t pin_num = MCHP_PINMUX_PIN_GET(pin->pinmux);
	uint8_t func = MCHP_PINMUX_FUNC_GET(pin->pinmux);
	uint8_t periph = MCHP_PINMUX_PERIPH_GET(pin->pinmux);
	uint32_t mask = BIT(pin_num);
	pio_registers_t *pio;

	if (port >= ARRAY_SIZE(mchp_pio_regs) || mchp_pio_regs[port] == NULL) {
		return;
	}
	pio = mchp_pio_regs[port];

	if (func == MCHP_PINMUX_FUNC_extra) {
		pinctrl_sysio(port, pin_num, true);
		return;
	}

	pinctrl_sysio(port, pin_num, false);
	pinctrl_set_flags(pio, mask, pin->pinflag);

	if (func == MCHP_PINMUX_FUNC_periph) {
		/* PIO_ABCDSR[1:0] selects peripheral A (0) to D (3). */
		if ((periph & BIT(0)) != 0U) {
			pio->PIO_ABCDSR[0] |= mask;
		} else {
			pio->PIO_ABCDSR[0] &= ~mask;
		}
		if ((periph & BIT(1)) != 0U) {
			pio->PIO_ABCDSR[1] |= mask;
		} else {
			pio->PIO_ABCDSR[1] &= ~mask;
		}
		pio->PIO_PDR = mask;
	} else {
		if ((pin->pinflag & MCHP_PINCTRL_OUTPUTENABLE) != 0U) {
			pio->PIO_OER = mask;
		} else {
			pio->PIO_ODR = mask;
		}
		pio->PIO_PER = mask;
	}
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		pinctrl_configure_pin(&pins[i]);
	}

	return 0;
}
