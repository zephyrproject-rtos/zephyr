/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_LPC11U6X_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_LPC11U6X_PINCTRL_H_

/**
 * @brief Pin control register for standard digital I/O pins:
 *
 * [0:2]   function.
 * [3:4]   mode.
 * [5]     hysteresis.
 * [6]     invert input.
 * [7:9]   reserved.
 * [10]    open-drain mode.
 * [11:12] digital filter sample mode.
 * [13:15] clock divisor.
 * [16:31] reserved.
 */

/**
 * @brief Control registers for digital/analog I/O pins:
 *
 * [0:2]   function.
 * [3:4]   mode.
 * [5]     hysteresis.
 * [6]     invert input.
 * [7]     analog mode.
 * [8]     input glitch filter.
 * [9]     reserved.
 * [10]    open-drain mode.
 * [11:12] digital filter sample mode.
 * [13:15] clock divisor.
 * [16:31] reserved.
 */

/**
 * @brief Control registers for open-drain I/O pins (I2C):
 *
 * [0:2]   function.
 * [3:7]   reserved.
 * [8:9]   I2C mode.
 * [10]    reserved.
 * [11:12] digital filter sample mode.
 * [13:15] clock divisor.
 * [16:31] reserved.
 */

#define IOCON_FUNC0 0
#define IOCON_FUNC1 1
#define IOCON_FUNC2 2
#define IOCON_FUNC3 3
#define IOCON_FUNC4 4
#define IOCON_FUNC5 5

#define IOCON_MODE_INACT	(0 << 3) /* No pull resistor. */
#define IOCON_MODE_PULLDOWN	(1 << 3) /* Enable pull-down resistor. */
#define IOCON_MODE_PULLUP	(2 << 3) /* Enable Pull-up resistor. */
#define IOCON_MODE_REPEATER	(3 << 3) /* Repeater mode. */

#define IOCON_HYS_EN		(1 << 5) /* Enable hysteresis. */

#define IOCON_INV_EN		(1 << 6) /* Invert input polarity. */

/* Only for analog pins. */
#define IOCON_ADMODE_EN		(0 << 7) /* Enable analog input mode. */
#define IOCON_DIGMODE_EN	(1 << 7) /* Enable digital I/O mode. */
#define IOCON_FILTR_DIS		(1 << 8) /* Disable noise filtering. */

/* Only for open-drain pins (I2C). */
#define IOCON_SFI2C_EN		(0 << 8) /* I2C standard mode / Fast-mode. */
#define IOCON_STDI2C_EN		(1 << 8) /* GPIO functionality. */
#define IOCON_FASTI2C_EN	(2 << 8) /* I2C Fast-mode Plus. */

#define IOCON_OPENDRAIN_EN	(1 << 10) /* Enable open-drain mode. */

/*
 * The digital filter mode allows to discard input pulses shorter than
 * 1, 2 or 3 clock cycles.
 */
#define IOCON_S_MODE_0CLK       (0 << 11) /* No input filter. */
#define IOCON_S_MODE_1CLK       (1 << 11)
#define IOCON_S_MODE_2CLK       (2 << 11)
#define IOCON_S_MODE_3CLK       (3 << 11)

/*
 * Clock divisor.
 */
#define IOCON_CLKDIV0		(0 << 13)
#define IOCON_CLKDIV1		(1 << 13)
#define IOCON_CLKDIV2		(2 << 13)
#define IOCON_CLKDIV3		(3 << 13)
#define IOCON_CLKDIV4		(4 << 13)
#define IOCON_CLKDIV5		(5 << 13)
#define IOCON_CLKDIV6		(6 << 13)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_LPC11U6X_PINCTRL_H_ */
