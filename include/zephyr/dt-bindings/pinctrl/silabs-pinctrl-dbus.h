/*
 * Copyright (c) 2024 Silicon Labs
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SILABS_PINCTRL_DBUS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SILABS_PINCTRL_DBUS_H_

#include <zephyr/dt-bindings/dt-util.h>

/*
 * Silabs Series 2 DBUS configuration is encoded in a 32-bit bitfield organized as follows:
 *
 * 31    : Whether the configuration represents an analog pin
 * If digital (bit 31 == 0):
 * 30..29: Reserved
 * 28..24: Route register offset in words from peripheral config (offset of <fun>ROUTE
 *         register in GPIO_<periph>ROUTE_TypeDef)
 * 23..19: Enable bit (offset into ROUTEEN register for given function)
 * 18    : Enable bit presence (some inputs are auto-enabled)
 * 17..8 : Peripheral config offset in words from DBUS base within GPIO (offset of <periph>ROUTE[n]
 *         register in GPIO_TypeDef minus offset of first route register [DBGROUTEPEN, 0x440])
 *  7..4 : GPIO pin
 *  3..0 : GPIO port
 * If analog (bit 31 == 1):
 * 15..14: Bus selection (A, B, CD)
 * 13..12: Bus selection (EVEN0, EVEN1, ODD0, ODD1)
 * 11..8 : Peripheral selection (bit in GPIO_nBUSALLOC bitfield)
 * 7 ..0 : Reserved
 */

#define SILABS_PINCTRL_GPIO_PORT_MASK   0x0000000FUL
#define SILABS_PINCTRL_GPIO_PIN_MASK    0x000000F0UL
#define SILABS_PINCTRL_PERIPH_BASE_MASK 0x0003FF00UL
#define SILABS_PINCTRL_HAVE_EN_MASK     0x00040000UL
#define SILABS_PINCTRL_EN_BIT_MASK      0x00F80000UL
#define SILABS_PINCTRL_ROUTE_MASK       0x1F000000UL

#define SILABS_PINCTRL_ANALOG_MASK      0x80000000UL
#define SILABS_PINCTRL_ABUS_BUS_MASK    0x0000C000UL
#define SILABS_PINCTRL_ABUS_PARITY_MASK 0x00003000UL
#define SILABS_PINCTRL_ABUS_PERIPH_MASK 0x00000F00UL

#define SILABS_PINCTRL_UNUSED 0xFF
#define SILABS_PINCTRL_ANALOG 0xAA

#define SILABS_DBUS(port, pin, periph_base, en_present, en_bit, route)                             \
	(FIELD_PREP(SILABS_PINCTRL_GPIO_PORT_MASK, port) |                                         \
	 FIELD_PREP(SILABS_PINCTRL_GPIO_PIN_MASK, pin) |                                           \
	 FIELD_PREP(SILABS_PINCTRL_PERIPH_BASE_MASK, periph_base) |                                \
	 FIELD_PREP(SILABS_PINCTRL_HAVE_EN_MASK, en_present) |                                     \
	 FIELD_PREP(SILABS_PINCTRL_EN_BIT_MASK, en_bit) |                                          \
	 FIELD_PREP(SILABS_PINCTRL_ROUTE_MASK, route))

#define SILABS_ABUS(bus, parity, peripheral)                                                       \
	(FIELD_PREP(SILABS_PINCTRL_ANALOG_MASK, 1) |                                               \
	 FIELD_PREP(SILABS_PINCTRL_ABUS_BUS_MASK, bus) |                                           \
	 FIELD_PREP(SILABS_PINCTRL_ABUS_PARITY_MASK, parity) |                                     \
	 FIELD_PREP(SILABS_PINCTRL_ABUS_PERIPH_MASK, peripheral))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SILABS_PINCTRL_DBUS_H_ */
