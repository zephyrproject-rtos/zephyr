/* pinmux_board_arduino_due.c - Arduino Due pinmux driver */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <pinmux.h>
#include <soc.h>
#include <sys_io.h>

#include "pinmux/pinmux.h"

/**
 * @brief Pinmux driver for Arduino due
 *
 * The SAM3X8E on Arduion Due has 4 PIO controllers. These controllers
 * are responsible for pin muxing, input/output, pull-up, etc.
 *
 * All PIO controller pins are flatten into sequentially incrementing
 * pin numbers:
 *   Pins  0 -  31 are for PIOA
 *   Pins 32 -  63 are for PIOB
 *   Pins 64 -  95 are for PIOC
 *   Pins 96 - 127 are for PIOD
 *
 * For all the pin descriptions, refer to the Atmel datasheet, and
 * the Arduino Due schematics.
 */

/*
 * These is the mapping from the board pins to PIO controllers.
 * This mapping is created from the Arduino Due schematics.
 * Refer to the official schematics for the actual mapping,
 * as the following may not be accurate.
 *
 * IO_0  : PA8
 * IO_1  : PA9
 * IO_2  : PB25
 * IO_3  : PC28
 * IO_4  : PA29
 * IO_5  : PC25
 * IO_6  : PC24
 * IO_7  : PC23
 *
 * IO_8  : PC22
 * IO_9  : PC21
 * IO_10 : PA28 and PC29
 * IO_11 : PD7
 * IO_12 : PD8
 * IO_13 : PB27
 * SDA1  : PA17
 * SCL1  : PA18
 *
 * IO_14 : PD4
 * IO_15 : PD5
 * IO_16 : PA13
 * IO_17 : PA12
 * IO_18 : PA11
 * IO_19 : PA10
 * IO_20 : PB12
 * IO_21 : PB13
 *
 * A_0   : PA16
 * A_1   : PA24
 * A_2   : PA23
 * A_3   : PA22
 * A_4   : PA6
 * A_5   : PA4
 * A_6   : PA3
 * A_7   : PA2
 *
 * A_8   : PB17
 * A_9   : PB18
 * A_10  : PB19
 * A_11  : PB20
 * DAC0  : PB15
 * DAC1  : PB16
 * CANRX : PA1
 * CANTX : PA0
 *
 * IO_22 : PB26
 * IO_23 : PA14
 * IO_24 : PA15
 * IO_25 : PD0
 * IO_26 : PD1
 * IO_27 : PD2
 * IO_28 : PD3
 * IO_29 : PD6
 * IO_30 : PD9
 * IO_31 : PA7
 * IO_32 : PD10
 * IO_33 : PC1
 * IO_34 : PC2
 * IO_35 : PC3
 * IO_36 : PC4
 * IO_37 : PC5
 * IO_38 : PC6
 * IO_39 : PC7
 * IO_40 : PC8
 * IO_41 : PC9
 * IO_42 : PA19
 * IO_43 : PA20
 * IO_44 : PC19
 * IO_45 : PC18
 * IO_46 : PC17
 * IO_47 : PC16
 * IO_48 : PC15
 * IO_49 : PC14
 * IO_50 : PC13
 * IO_51 : PC12
 */

#define N_PIOA 0
#define N_PIOB 1
#define N_PIOC 2
#define N_PIOD 3

/*
 * This function sets the default for the following:
 * - Pin mux (peripheral A or B)
 * - Set pin as input or output
 * - Enable pull-up for pins
 *
 * At boot, all pins are outputs with pull-up enabled, and are set to be
 * peripheral A (with value 0). So only the peripherals that need to be
 * set to B (value 1) will be declared explicitly below.
 *
 * Note that all pins are set to be controlled by the PIO controllers
 * by default. For peripherals to work (e.g. UART), the PIO has to
 * be disabled for that pin (e.g. UART to take over those pins).
 */
static void __pinmux_defaults(void)
{
	uint32_t ab_select[4];	/* A/B selection   */
	uint32_t output_en[4];	/* output enabled  */
	uint32_t pull_up[4];	/* pull-up enabled */
	uint32_t pio_ctrl[4];	/* PIO enable      */
	uint32_t tmp;

	/* Read defaults at boot, as the bootloader may have already
	 * configured some pins.
	 */
	ab_select[N_PIOA] = __PIOA->absr;
	ab_select[N_PIOB] = __PIOB->absr;
	ab_select[N_PIOC] = __PIOC->absr;
	ab_select[N_PIOD] = __PIOD->absr;

	output_en[N_PIOA] = __PIOA->osr;
	output_en[N_PIOB] = __PIOB->osr;
	output_en[N_PIOC] = __PIOC->osr;
	output_en[N_PIOD] = __PIOD->osr;

	pio_ctrl[N_PIOA] = __PIOA->psr;
	pio_ctrl[N_PIOB] = __PIOB->psr;
	pio_ctrl[N_PIOC] = __PIOC->psr;
	pio_ctrl[N_PIOD] = __PIOD->psr;

	/* value 1 means pull-up disabled, so need to invert */
	pull_up[N_PIOA] = ~(__PIOA->pusr);
	pull_up[N_PIOB] = ~(__PIOB->pusr);
	pull_up[N_PIOC] = ~(__PIOC->pusr);
	pull_up[N_PIOD] = ~(__PIOD->pusr);

	/*
	 * Now modify as we wish
	 */

	/* Make sure JTAG pins are used for JTAG */
	pio_ctrl[N_PIOB] &= ~(BIT(28) | BIT(29) | BIT(30) | BIT(31));

	/* UART console:
	 * IO_0: PA8 (RX)
	 * IO_1: PA9 (TX)
	 */
	pio_ctrl[N_PIOA] &= ~(BIT(8) | BIT(9));

	/* I2C pins on TWI controller #0
	 *
	 * SDA1: PA17
	 * SCL1: PA18
	 *
	 * Note that these need external pull-up resistors.
	 */
	pio_ctrl[N_PIOA] &= ~(BIT(17) | BIT(18));

	/* I2C pins on TWI controller #1
	 *
	 * IO_20: PB12 (SDA)
	 * IO_21: PB13 (SCL)
	 *
	 * Board already have pull-up resistors.
	 */
	pio_ctrl[N_PIOB] &= ~(BIT(12) | BIT(13));

	/*
	 * Setup ADC pins.
	 *
	 * Note that the ADC is considered extra function
	 * for the pins (other than A or B). This extra
	 * pin function is enabled by enabling the ADC
	 * controller. Therefore, the following code
	 * only sets these pins as input, with pull-up
	 * disabled. This does not detach the PIO
	 * controller from the pins so the peripherals
	 * won't take over.
	 *
	 * A_0 : PA16
	 * A_1 : PA24
	 * A_2 : PA23
	 * A_3 : PA22
	 * A_4 : PA6
	 * A_5 : PA4
	 * A_6 : PA3
	 * A_7 : PA2
	 *
	 * A_8 : PB17
	 * A_9 : PB18
	 * A_10: PB19
	 * A_11: PB20
	 */
	tmp = BIT(16) | BIT(24) | BIT(23) | BIT(22)
	      | BIT(6) | BIT(4) | BIT(3) | BIT(2);

	pio_ctrl[N_PIOA] |= tmp;
	output_en[N_PIOA] &= ~(tmp);
	pull_up[N_PIOA] &= ~(tmp);

	tmp = BIT(17) | BIT(18) | BIT(19) | BIT(20);

	pio_ctrl[N_PIOB] |= tmp;
	output_en[N_PIOB] &= ~(tmp);
	pull_up[N_PIOB] &= ~(tmp);

	/*
	 * Write modifications back to those registers
	 */

	__PIOA->absr = ab_select[N_PIOA];
	__PIOB->absr = ab_select[N_PIOB];
	__PIOC->absr = ab_select[N_PIOC];
	__PIOD->absr = ab_select[N_PIOD];

	/* set output enable */
	__PIOA->oer = output_en[N_PIOA];
	__PIOB->oer = output_en[N_PIOB];
	__PIOC->oer = output_en[N_PIOC];
	__PIOD->oer = output_en[N_PIOD];

	/* set output disable */
	__PIOA->odr = ~(output_en[N_PIOA]);
	__PIOB->odr = ~(output_en[N_PIOB]);
	__PIOC->odr = ~(output_en[N_PIOC]);
	__PIOD->odr = ~(output_en[N_PIOD]);

	/* set PIO enable */
	__PIOA->per = pio_ctrl[N_PIOA];
	__PIOB->per = pio_ctrl[N_PIOB];
	__PIOC->per = pio_ctrl[N_PIOC];
	__PIOD->per = pio_ctrl[N_PIOD];

	/* set PIO disable */
	__PIOA->pdr = ~(pio_ctrl[N_PIOA]);
	__PIOB->pdr = ~(pio_ctrl[N_PIOB]);
	__PIOC->pdr = ~(pio_ctrl[N_PIOC]);
	__PIOD->pdr = ~(pio_ctrl[N_PIOD]);

	/* set pull-up enable */
	__PIOA->puer = pull_up[N_PIOA];
	__PIOB->puer = pull_up[N_PIOB];
	__PIOC->puer = pull_up[N_PIOC];
	__PIOD->puer = pull_up[N_PIOD];

	/* set pull-up disable */
	__PIOA->pudr = ~(pull_up[N_PIOA]);
	__PIOB->pudr = ~(pull_up[N_PIOB]);
	__PIOC->pudr = ~(pull_up[N_PIOC]);
	__PIOD->pudr = ~(pull_up[N_PIOD]);
}

static int pinmux_init(struct device *port)
{
	ARG_UNUSED(port);

	__pinmux_defaults();

	return 0;
}

SYS_INIT(pinmux_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
