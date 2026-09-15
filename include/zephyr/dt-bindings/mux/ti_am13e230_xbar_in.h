/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MUX_TI_AM13E230_XBAR_IN_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MUX_TI_AM13E230_XBAR_IN_H_

/*
 * Packed mux-state trailing cell for the AM13E230 input X-BAR: the GPIO
 * number to route, plus whether that GPIO's RAW_INPUTSELECT_Pn bit should
 * select the raw pad signal (1) or the signal after the GPIO peripheral
 * module processes it (0, TRM reset default). The bit lives per-GPIO, not
 * per-channel - routing two channels to the same GPIO with conflicting
 * raw settings is a devicetree authoring error, not something the driver
 * arbitrates.
 */
#define AM13E230_XBAR_IN_PIN_MASK  0xFFU
#define AM13E230_XBAR_IN_RAW_SHIFT 8U
#define AM13E230_XBAR_IN_RAW_MASK  0x1U

#define AM13E230_XBAR_IN_STATE(pin, raw)                                                           \
	(((pin) & AM13E230_XBAR_IN_PIN_MASK) |                                                     \
	 (((raw) & AM13E230_XBAR_IN_RAW_MASK) << AM13E230_XBAR_IN_RAW_SHIFT))

#define AM13E230_XBAR_IN_STATE_PIN(state) ((state) & AM13E230_XBAR_IN_PIN_MASK)
#define AM13E230_XBAR_IN_STATE_RAW(state)                                                          \
	(((state) >> AM13E230_XBAR_IN_RAW_SHIFT) & AM13E230_XBAR_IN_RAW_MASK)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MUX_TI_AM13E230_XBAR_IN_H_ */
