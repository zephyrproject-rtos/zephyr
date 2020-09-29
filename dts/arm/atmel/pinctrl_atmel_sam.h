/*
 * Copyright (c) 2020 Linaro Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINCTRL_ATMEL_SAM_H_
#define PINCTRL_ATMEL_SAM_H_

#define PERIPH_a	0
#define PERIPH_b	1
#define PERIPH_c	2
#define PERIPH_d	3
#define PERIPH_e	4
#define PERIPH_f	5
#define PERIPH_g	6
#define PERIPH_h	7

/* Create a pincfg device tree node:
 *
 * The node name and nodelabel will be of the form:
 *
 * NODE = p<port><pin><periph>_<inst>_<signal>
 *
 * NODE: NODE {
 *     atmel,pins = < &pio<port> <pin> PERIPH_<perip> >;
 * }
 *
 * So for example:
 *
 * DT_ATMEL_PIN(uart, urxd, a, 8, a);
 *
 * Will become:
 *
 * pa8a_uart_urxd: pa8a_uart_urxd {
 *    atmel,pins = <&pioa 8 PERIPH_a>;
 * }
 *
 */
#define DT_ATMEL_PIN(inst, signal, port, pin, periph) \
	p##port##pin##periph##_##inst##_##signal: \
	p##port##pin##periph##_##inst##_##signal { \
	atmel,pins = < &pio##port pin PERIPH_##periph >; }

#define DT_ATMEL_GPIO(inst, signal, port, pin, periph) \
	p##port##pin##periph##_##inst##_##signal: \
	p##port##pin##periph##_##inst##_##signal { \
	atmel,pins = < &gpio##port pin PERIPH_##periph >; }

#endif /* PINCTRL_ATMEL_SAM_H_ */
