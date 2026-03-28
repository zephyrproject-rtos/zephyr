/*
 * Copyright (c) 2022 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NUMICRO_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NUMICRO_PINCTRL_H_

#define NUMICRO_MFP_SHIFT 0U
#define NUMICRO_MFP_MASK  0xFU
#define NUMICRO_PIN_SHIFT 4U
#define NUMICRO_PIN_MASK  0xFU
#define NUMICRO_PORT_SHIFT 8U
#define NUMICRO_PORT_MASK  0xFU

/**
 * @brief Pin configuration configuration bit field.
 *
 * Fields:
 *
 * - mfp [ 0 : 3 ]
 * - pin [ 4 : 7 ]
 * - port [ 8 : 11 ]
 *
 * @param port Port ('A'..'H')
 * @param pin  Pin (0..15)
 * @param mfp  Multi-function value (0..15)
 */
#define NUMICRO_PINMUX(port, pin, mfp)					\
	(((((port) - 'A') & NUMICRO_PORT_MASK) << NUMICRO_PORT_SHIFT) |	\
	(((pin) & NUMICRO_PIN_MASK) << NUMICRO_PIN_SHIFT) |		\
	(((mfp) & NUMICRO_MFP_MASK) << NUMICRO_MFP_SHIFT))

#define NUMICRO_PORT(pinmux) \
	(((pinmux) >> NUMICRO_PORT_SHIFT) & NUMICRO_PORT_MASK)
#define NUMICRO_PIN(pinmux) \
	(((pinmux) >> NUMICRO_PIN_SHIFT) & NUMICRO_PIN_MASK)
#define NUMICRO_MFP(pinmux) \
	(((pinmux) >> NUMICRO_MFP_SHIFT) & NUMICRO_MFP_MASK)

#endif	/* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_NUMICRO_PINCTRL_H_ */
