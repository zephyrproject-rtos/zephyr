/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/*
 * UART clock configurations
 *
 * Define them here so that it can be replaced by global variables
 * on other boards where the uart clock is determined dynamically
 * following the PLL configuration
 */
#define uart_fe310_port_0_clk_freq    16000000
#define uart_fe310_port_1_clk_freq    16000000

#endif /* __INC_BOARD_H */
