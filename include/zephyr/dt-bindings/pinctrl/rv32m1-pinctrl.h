/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright (c) 2022 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RV32M1_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RV32M1_PINCTRL_H_

/**
 * @brief Specify PORTx->PCR register MUX field
 *
 * @param port Port name ('A' to 'E')
 * @param pin Port pin number (0 to 31)
 * @param mux Alternate function number (0 to 7)
 */
#define RV32M1_MUX(port, pin, mux)              \
	(((((port) - 'A') & 0xF) << 28) |       \
	(((pin) & 0x3F) << 22) |                \
	(((mux) & 0x7) << 8))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RV32M1_PINCTRL_H_ */
