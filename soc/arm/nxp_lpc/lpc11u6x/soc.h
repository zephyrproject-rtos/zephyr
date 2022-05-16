/*
 * Copyright (c) 2020, Seagate
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the nxp_lpc11u6x platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'nxp_lpc11u6x' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>


#endif /* !_ASMLANGUAGE */

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

#define IOCON_PIO_FUNC(x) (((x) & 0x7))
#define IOCON_PIO_FUNC_MASK IOCON_PIO_FUNC(0x7)
#define IOCON_PIO_MODE(x) (((x) & 0x3) << 3)
#define IOCON_PIO_MODE_MASK IOCON_PIO_MODE(0x3)
#define IOCON_PIO_HYS(x) (((x) & 0x1) << 5)
#define IOCON_PIO_HYS_MASK IOCON_PIO_HYS(0x1)
#define IOCON_PIO_INVERT(x) (((x) & 0x1) << 2)
#define IOCON_PIO_INVERT_MASK IOCON_PIO_INVERT(0x1)
#define IOCON_PIO_OD(x) (((x) & 0x1) << 10)
#define IOCON_PIO_OD_MASK IOCON_PIO_OD(0x1)
#define IOCON_PIO_SMODE(x) (((x) & 0x3) << 11)
#define IOCON_PIO_SMODE_MASK IOCON_PIO_SMODE(0x3)
#define IOCON_PIO_CLKDIV(x) (((x) & 0x7) << 13)
#define IOCON_PIO_CLKDIV_MASK IOCON_PIO_CLKDIV(0x7)


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

#define IOCON_PIO_ADMODE(x) (((x) & 0x1) << 7)
#define IOCON_PIO_ADMODE_MASK IOCON_PIO_ADMODE(0x1)
#define IOCON_PIO_FILTER(x) (((x) & 0x1) << 8)
#define IOCON_PIO_FILTER_MASK IOCON_PIO_FILTER(0x1)



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

#define IOCON_PIO_I2CMODE(x) (((x) & 0x3) << 8)
#define IOCON_PIO_I2CMODE_MASK IOCON_PIO_I2CMODE(0x3)

#define IOCON_FUNC0 0
#define IOCON_FUNC1 1
#define IOCON_FUNC2 2
#define IOCON_FUNC3 3
#define IOCON_FUNC4 4
#define IOCON_FUNC5 5

#endif /* _SOC__H_ */
