/*
 * Copyright (c) 2021 Nicolai Glud <kludentwo@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINMUX_NXP_K6X_H_
#define PINMUX_NXP_K6X_H_

#define K_PORT_PinDisabledOrAnalog		0
#define K_PORT_MuxAsGpio			1
#define K_PORT_MuxAlt2				2
#define K_PORT_MuxAlt3				3
#define K_PORT_MuxAlt4				4
#define K_PORT_MuxAlt5				5
#define K_PORT_MuxAlt6				6
#define K_PORT_MuxAlt7				7
#define K_PORT_MuxAlt8				8
#define K_PORT_MuxAlt9				9
#define K_PORT_MuxAlt10				10
#define K_PORT_MuxAlt11				11
#define K_PORT_MuxAlt12				12
#define K_PORT_MuxAlt13				13
#define K_PORT_MuxAlt14				14
#define K_PORT_MuxAlt15				15

#define DT_NXP_PINMUX_DEFINE(k_name, k_port, k_pin, k_mux) \
	k_name##_##pt##k_port##k_pin: k_name##_##pt##k_port##k_pin { \
		pin = <k_pin>; \
		mux = <k_mux>; \
	}

#endif /* PINMUX_NXP_K6X_H_ */
