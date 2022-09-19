/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_pinctrl

#include <zephyr/drivers/pinctrl.h>

/** @brief All GPIO register as arrays of registers */
struct gpio_regs {
	volatile uint32_t  CTRL[174];
	uint32_t  RESERVED[18];
	volatile uint32_t  PARIN[6];
	uint32_t  RESERVED1[26];
	volatile uint32_t  PAROUT[6];
	uint32_t  RESERVED2[20];
	volatile uint32_t  LOCK[6];
	uint32_t  RESERVED3[64];
	volatile uint32_t  CTRL2[174];
};

#define NUM_MCHP_GPIO_PORTS	6u

/* GPIO Control register field definitions. */

/* bits[1:0] internal pull up/down selection */
#define MCHP_GPIO_CTRL_PUD_POS		0
#define MCHP_GPIO_CTRL_PUD_MASK0	0x03u
#define MCHP_GPIO_CTRL_PUD_MASK		0x03u
#define MCHP_GPIO_CTRL_PUD_NONE		0x00u
#define MCHP_GPIO_CTRL_PUD_PU		0x01u
#define MCHP_GPIO_CTRL_PUD_PD		0x02u
/* Repeater(keeper) mode */
#define MCHP_GPIO_CTRL_PUD_RPT		0x03u

/* bits[3:2] power gating */
#define MCHP_GPIO_CTRL_PWRG_POS		2
#define MCHP_GPIO_CTRL_PWRG_MASK0	0x03u
#define MCHP_GPIO_CTRL_PWRG_VTR_IO	0
#define MCHP_GPIO_CTRL_PWRG_VCC_IO	SHLU32(1, MCHP_GPIO_CTRL_PWRG_POS)
#define MCHP_GPIO_CTRL_PWRG_OFF		SHLU32(2, MCHP_GPIO_CTRL_PWRG_POS)
#define MCHP_GPIO_CTRL_PWRG_RSVD	SHLU32(3, MCHP_GPIO_CTRL_PWRG_POS)
#define MCHP_GPIO_CTRL_PWRG_MASK	SHLU32(3, MCHP_GPIO_CTRL_PWRG_POS)

/* bit[8] output buffer type: push-pull or open-drain */
#define MCHP_GPIO_CTRL_BUFT_POS		8
#define MCHP_GPIO_CTRL_BUFT_MASK	BIT(MCHP_GPIO_CTRL_BUFT_POS)
#define MCHP_GPIO_CTRL_BUFT_OPENDRAIN	BIT(MCHP_GPIO_CTRL_BUFT_POS)
#define MCHP_GPIO_CTRL_BUFT_PUSHPULL	0

/* bit[9] direction */
#define MCHP_GPIO_CTRL_DIR_POS		9
#define MCHP_GPIO_CTRL_DIR_MASK		BIT(MCHP_GPIO_CTRL_DIR_POS)
#define MCHP_GPIO_CTRL_DIR_OUTPUT	BIT(MCHP_GPIO_CTRL_DIR_POS)
#define MCHP_GPIO_CTRL_DIR_INPUT	0

/*
 * bit[10] Alternate output disable. Default==0(alternate output enabled)
 * GPIO output value is controlled by bit[16] of this register.
 * Set bit[10]=1 if you wish to control pin output using the parallel
 * GPIO output register bit for this pin.
 */
#define MCHP_GPIO_CTRL_AOD_POS		10
#define MCHP_GPIO_CTRL_AOD_MASK		BIT(MCHP_GPIO_CTRL_AOD_POS)
#define MCHP_GPIO_CTRL_AOD_DIS		BIT(MCHP_GPIO_CTRL_AOD_POS)

/* bit[11] GPIO function output polarity */
#define MCHP_GPIO_CTRL_POL_POS		11
#define MCHP_GPIO_CTRL_POL_INVERT	BIT(MCHP_GPIO_CTRL_POL_POS)

/* bits[14:12] pin mux (function) */
#define MCHP_GPIO_CTRL_MUX_POS		12
#define MCHP_GPIO_CTRL_MUX_MASK0	0x07u
#define MCHP_GPIO_CTRL_MUX_MASK		SHLU32(7, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F0		0
#define MCHP_GPIO_CTRL_MUX_GPIO		MCHP_GPIO_CTRL_MUX_F0
#define MCHP_GPIO_CTRL_MUX_F1		SHLU32(1, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F2		SHLU32(2, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F3		SHLU32(3, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F4		SHLU32(4, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F5		SHLU32(5, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F6		SHLU32(6, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX_F7		SHLU32(7, MCHP_GPIO_CTRL_MUX_POS)
#define MCHP_GPIO_CTRL_MUX(n) SHLU32(((n) & 0x7u), MCHP_GPIO_CTRL_MUX_POS)

/*
 * bit[15] Disables input pad leaving output pad enabled
 * Useful for reducing power consumption of output only pins.
 */
#define MCHP_GPIO_CTRL_INPAD_DIS_POS	15
#define MCHP_GPIO_CTRL_INPAD_DIS_MASK	BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS)
#define MCHP_GPIO_CTRL_INPAD_DIS	BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS)

/* bit[16]: Alternate output pin value. Enabled when bit[10]==0(default) */
#define MCHP_GPIO_CTRL_OUTVAL_POS	16
#define MCHP_GPIO_CTRL_OUTV_HI		BIT(MCHP_GPIO_CTRL_OUTVAL_POS)

/* bit[24] Input pad value. Always live unless input pad is powered down */
#define MCHP_GPIO_CTRL_INPAD_VAL_POS	24
#define MCHP_GPIO_CTRL_INPAD_VAL_HI	BIT(MCHP_GPIO_CTRL_INPAD_VAL_POS)

#define MCHP_GPIO_CTRL_DRIVE_OD_HI				     \
	(MCHP_GPIO_CTRL_BUFT_OPENDRAIN + MCHP_GPIO_CTRL_DIR_OUTPUT + \
	 MCHP_GPIO_CTRL_MUX_GPIO + MCHP_GPIO_CTRL_OUTV_HI)

/*
 * Each GPIO pin implements a second control register.
 * GPIO Control 2 register selects pin drive strength and slew rate.
 * bit[0] = slew rate: 0=slow, 1=fast
 * bits[5:4] = drive strength
 * 00b = 2mA (default)
 * 01b = 4mA
 * 10b = 8mA
 * 11b = 12mA
 */
#define MCHP_GPIO_CTRL2_OFFSET		0x0500u
#define MCHP_GPIO_CTRL2_SLEW_POS	0
#define MCHP_GPIO_CTRL2_SLEW_MASK	0x01u
#define MCHP_GPIO_CTRL2_SLEW_SLOW	0
#define MCHP_GPIO_CTRL2_SLEW_FAST	BIT(MCHP_GPIO_CTRL2_SLEW_POS)
#define MCHP_GPIO_CTRL2_DRV_STR_POS	4
#define MCHP_GPIO_CTRL2_DRV_STR_MASK	0x30u
#define MCHP_GPIO_CTRL2_DRV_STR_2MA	0
#define MCHP_GPIO_CTRL2_DRV_STR_4MA	0x10u
#define MCHP_GPIO_CTRL2_DRV_STR_8MA	0x20u
#define MCHP_GPIO_CTRL2_DRV_STR_12MA	0x30u

#define MCHP_XEC_NO_PULL	0x0
#define MCHP_XEC_PULL_UP	0x1
#define MCHP_XEC_PULL_DOWN	0x2
#define MCHP_XEC_REPEATER	0x3
#define MCHP_XEC_PUSH_PULL	0x0
#define MCHP_XEC_OPEN_DRAIN	0x1
#define MCHP_XEC_NO_OVAL	0x0
#define MCHP_XEC_OVAL_LOW	0x1
#define MCHP_XEC_OVAL_HIGH	0x2
#define MCHP_XEC_DRVSTR_NONE	0x0
#define MCHP_XEC_DRVSTR_2MA	0x1
#define MCHP_XEC_DRVSTR_4MA	0x2
#define MCHP_XEC_DRVSTR_8MA	0x3
#define MCHP_XEC_DRVSTR_12MA	0x4

#define SHLU32(v, n)			((uint32_t)(v) << (n))

/* Microchip XEC: each GPIO pin has two 32-bit control register.
 * The first 32-bit register contains all pin features except
 * slew rate and driver strength in the second control register.
 * We compute the register index from the beginning of the GPIO
 * control address space which is the same range of the PINCTRL
 * parent node.
 */

static void config_drive_slew(struct gpio_regs * const regs, uint32_t idx, uint32_t conf)
{
	uint32_t slew = conf & (MCHP_XEC_OSPEEDR_MASK << MCHP_XEC_OSPEEDR_POS);
	uint32_t drvstr = conf & (MCHP_XEC_ODRVSTR_MASK << MCHP_XEC_ODRVSTR_POS);
	uint32_t val = 0;
	uint32_t mask = 0;

	if (slew != MCHP_XEC_OSPEEDR_NO_CHG) {
		mask |= MCHP_GPIO_CTRL2_SLEW_MASK;
		if (slew == MCHP_XEC_OSPEEDR_FAST) {
			val |= MCHP_GPIO_CTRL2_SLEW_FAST;
		}
	}
	if (drvstr != MCHP_XEC_ODRVSTR_NO_CHG) {
		mask |= MCHP_GPIO_CTRL2_DRV_STR_MASK;
		val |= (drvstr << MCHP_GPIO_CTRL2_DRV_STR_POS);
	}

	if (!mask) {
		return;
	}

	regs->CTRL2[idx] = (regs->CTRL2[idx] & ~mask) | (val & mask);
}

/* Configure pin by writing GPIO Control and Control2 registers.
 * NOTE: Disable alternate output feature since the GPIO driver does.
 * While alternate output is enabled (default state of pin) HW does not
 * ignores writes to the parallel output bit for the pin. To set parallel
 * output value we must keep pin direction as input, set alternate output
 * disable, program pin value to parallel output bit, and then disable
 * alternate output mode.
 */
static int xec_config_pin(uint32_t portpin, uint32_t conf, uint32_t altf)
{
	struct gpio_regs * const regs = (struct gpio_regs * const)DT_INST_REG_ADDR(0);
	uint32_t port = MCHP_XEC_PINMUX_PORT(portpin);
	uint32_t pin = (uint32_t)MCHP_XEC_PINMUX_PIN(portpin);
	uint32_t msk = MCHP_GPIO_CTRL_AOD_MASK;
	uint32_t val = MCHP_GPIO_CTRL_AOD_DIS;
	uint32_t idx = 0u;
	uint32_t temp = 0u;

	if (port >= NUM_MCHP_GPIO_PORTS) {
		return -EINVAL;
	}

	/* MCHP XEC family is 32 pins per port */
	idx = (port * 32U) + pin;

	config_drive_slew(regs, idx, conf);

	/* default input pad enabled, buffer type push-pull, no internal pulls */
	msk |= (BIT(MCHP_GPIO_CTRL_INPAD_DIS_POS) | MCHP_GPIO_CTRL_BUFT_MASK |
			MCHP_GPIO_CTRL_PUD_MASK |
			MCHP_GPIO_CTRL_MUX_MASK);

	if (conf & BIT(MCHP_XEC_PIN_LOW_POWER_POS)) {
		msk |= MCHP_GPIO_CTRL_PWRG_MASK;
		val |= MCHP_GPIO_CTRL_PWRG_OFF;
	}

	temp = (conf & MCHP_XEC_PUPDR_MASK) >> MCHP_XEC_PUPDR_POS;
	switch (temp) {
	case MCHP_XEC_PULL_UP:
		val |= MCHP_GPIO_CTRL_PUD_PU;
		break;
	case MCHP_XEC_PULL_DOWN:
		val |= MCHP_GPIO_CTRL_PUD_PD;
		break;
	case MCHP_XEC_REPEATER:
		val |= MCHP_GPIO_CTRL_PUD_RPT;
		break;
	default:
		val |= MCHP_GPIO_CTRL_PUD_NONE;
		break;
	}

	if ((conf >> MCHP_XEC_OTYPER_POS) & MCHP_XEC_OTYPER_MASK) {
		val |= MCHP_GPIO_CTRL_BUFT_OPENDRAIN;
	}

	regs->CTRL[idx] = (regs->CTRL[idx] & ~msk) | val;

	temp = (conf >> MCHP_XEC_OVAL_POS) & MCHP_XEC_OVAL_MASK;
	if (temp) {
		if (temp == MCHP_XEC_OVAL_DRV_HIGH) {
			regs->PAROUT[port] |= BIT(pin);
		} else {
			regs->PAROUT[port] &= ~BIT(pin);
		}

		regs->CTRL[idx] |= MCHP_GPIO_CTRL_DIR_OUTPUT;
	}

	val = (uint32_t)((altf & MCHP_GPIO_CTRL_MUX_MASK0) << MCHP_GPIO_CTRL_MUX_POS);
	regs->CTRL[idx] |= val;

	return 0;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	uint32_t portpin, mux, cfg, func;
	int ret;

	ARG_UNUSED(reg);

	for (uint8_t i = 0U; i < pin_cnt; i++) {
		mux = pins[i].pinmux;

		func = MCHP_XEC_PINMUX_FUNC(mux);
		if (func >= MCHP_AFMAX) {
			return -EINVAL;
		}

		cfg = pins[i].pincfg;
		portpin = MEC_XEC_PINMUX_PORT_PIN(mux);

		ret = xec_config_pin(portpin, cfg, func);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
