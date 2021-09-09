/*
 * Copyright (c) 2020 Linaro Limited
 * Copyright (c) 2021 Gerson Fernando Budke
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINCTRL_ATMEL_SAM_H_
#define PINCTRL_ATMEL_SAM_H_

#include <dt-bindings/dt-util.h>

#define PERIPH_a	0
#define PERIPH_b	1
#define PERIPH_c	2
#define PERIPH_d	3
#define PERIPH_e	4
#define PERIPH_f	5
#define PERIPH_g	6
#define PERIPH_h	7
#define PERIPH_i	8
#define PERIPH_j	9
#define PERIPH_k	10
#define PERIPH_l	11
#define PERIPH_m	12
#define PERIPH_n	13

/* Create a pincfg device tree node:
 *
 * The node name and nodelabel will be of the form:
 *
 * NODE = p<port><pin><periph>_<inst>_<signal>
 *
 * NODE: NODE {
 *      atmel,pins = < &pio<port> <pin> PERIPH_<perip> >;
 *      flags_1;
 *       ...
 *      flags_N;
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
 * Flags are optional and should be pass one by one as arguments:
 *
 * DT_ATMEL_PORT(sercom0, pad0, a,  4, d, pinmux-enable);
 *
 * Will become:
 *
 * pa4d_sercom0_pad0: pa4d_sercom0_pad0 {
 *    atmel,pins = <&porta 0x4 0x3 >;
 *    pinmux-enable;
 * }
 *
 * For the complete list of flags see atmel,sam[0]-pinctrl.yaml
 */
#define DT_ATMEL_PINCTRL_FLAG(flag) flag;
#define DT_ATMEL_PINCTRL_FLAGS(...) \
	MACRO_MAP_CAT(DT_ATMEL_PINCTRL_FLAG __VA_OPT__(,) __VA_ARGS__)

#define DT_ATMEL_PIN(inst, signal, port, pin, periph, ...) \
	p##port##pin##periph##_##inst##_##signal: \
	p##port##pin##periph##_##inst##_##signal { \
	atmel,pins = < &pio##port pin PERIPH_##periph >; \
		DT_ATMEL_PINCTRL_FLAGS(__VA_ARGS__) \
	}

#define DT_ATMEL_GPIO(inst, signal, port, pin, periph, ...) \
	p##port##pin##periph##_##inst##_##signal: \
	p##port##pin##periph##_##inst##_##signal { \
	atmel,pins = < &gpio##port pin PERIPH_##periph >; \
		DT_ATMEL_PINCTRL_FLAGS(__VA_ARGS__) \
	}

#define DT_ATMEL_PORT(inst, signal, grouport, pin, periph, ...) \
	p##grouport##pin##periph##_##inst##_##signal: \
	p##grouport##pin##periph##_##inst##_##signal { \
	atmel,pins = < &port##grouport pin PERIPH_##periph >; \
		DT_ATMEL_PINCTRL_FLAGS(__VA_ARGS__) \
	}

#endif /* PINCTRL_ATMEL_SAM_H_ */
