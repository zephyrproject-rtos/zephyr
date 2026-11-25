/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MANAGEMENT_CLOCK_MANAGEMENT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MANAGEMENT_CLOCK_MANAGEMENT_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @name Clock state selection masks
 *
 * Used in the clock-request-<n> properties of clock consumers to select the
 * clock states of a clock domain that are allowed for a request. CLK_STATE_n
 * selects the nth clock-state child of the clock domain, counted in the order
 * the children are declared in devicetree. Masks can be combined, e.g.
 * (CLK_STATE_0 | CLK_STATE_2).
 * @{
 */
#define CLK_STATE_0 BIT(0)
#define CLK_STATE_1 BIT(1)
#define CLK_STATE_2 BIT(2)
#define CLK_STATE_3 BIT(3)
#define CLK_STATE_4 BIT(4)
#define CLK_STATE_5 BIT(5)
#define CLK_STATE_6 BIT(6)
#define CLK_STATE_7 BIT(7)
#define CLK_STATE_8 BIT(8)
#define CLK_STATE_9 BIT(9)
#define CLK_STATE_10 BIT(10)
#define CLK_STATE_11 BIT(11)
#define CLK_STATE_12 BIT(12)
#define CLK_STATE_13 BIT(13)
#define CLK_STATE_14 BIT(14)
#define CLK_STATE_15 BIT(15)
#define CLK_STATE_16 BIT(16)
#define CLK_STATE_17 BIT(17)
#define CLK_STATE_18 BIT(18)
#define CLK_STATE_19 BIT(19)
#define CLK_STATE_20 BIT(20)
#define CLK_STATE_21 BIT(21)
#define CLK_STATE_22 BIT(22)
#define CLK_STATE_23 BIT(23)
#define CLK_STATE_24 BIT(24)
#define CLK_STATE_25 BIT(25)
#define CLK_STATE_26 BIT(26)
#define CLK_STATE_27 BIT(27)
#define CLK_STATE_28 BIT(28)
#define CLK_STATE_29 BIT(29)
#define CLK_STATE_30 BIT(30)
#define CLK_STATE_31 BIT(31)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MANAGEMENT_CLOCK_MANAGEMENT_H_ */
