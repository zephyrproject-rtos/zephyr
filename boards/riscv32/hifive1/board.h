/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2017 Palmer Dabbelt <palmer@dabbelt.com>
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
#define uart_sifive_port_0_clk_freq    16000000
#define uart_sifive_port_1_clk_freq    16000000

/* LEDS configuration */
#define LED0_GPIO_PORT        "gpio0"
#define LED1_GPIO_PORT        "gpio0"
#define LED2_GPIO_PORT        "gpio0"

#define LED0_GPIO_PIN         19
#define LED1_GPIO_PIN         21
#define LED2_GPIO_PIN         22

#endif /* __INC_BOARD_H */
