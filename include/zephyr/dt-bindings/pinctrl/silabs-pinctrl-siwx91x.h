/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SILABS_PINCTRL_SIWX91X_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SILABS_PINCTRL_SIWX91X_H_

#include <zephyr/dt-bindings/dt-util.h>

#if !defined(FIELD_PREP)
/* Upstream does not make these macros available to DeviceTree */
#define LSB_GET(value)          ((value) & -(value))
#define FIELD_GET(mask, value)  (((value) & (mask)) / LSB_GET(mask))
#define FIELD_PREP(mask, value) (((value) * LSB_GET(mask)) & (mask))
#endif

#define SIWX91X_PINCTRL_PORT_MASK    0x0000000FUL
#define SIWX91X_PINCTRL_PIN_MASK     0x000000F0UL
#define SIWX91X_PINCTRL_ULPPIN_MASK  0x00000F00UL
#define SIWX91X_PINCTRL_MODE_MASK    0x0003F000UL
#define SIWX91X_PINCTRL_ULPMODE_MASK 0x00FC0000UL
#define SIWX91X_PINCTRL_PAD_MASK     0xFF000000UL

/* Declare an integer representing the pinctrl/gpio state of a signal on SiWx91x.
 * @param mode          HP GPIO mode, 0xFF if HP GPIO mux isn't used
 * @param ulpmode       ULP GPIO mode, 0xFF if ULP GPIO mux isn't used
 * @param pad           HP pad number, 0xFF if HP pad isn't used, 0 if host pad
 * @param port          GPIO port number (0-4)
 * @param pin           HP GPIO pin number, value is unused if mode is 0xFF
 * @param ulppin        ULP GPIO pin number, value is unused if ulpmode is 0xFF
 */
#define SIWX91X_GPIO(mode, ulpmode, pad, port, pin, ulppin)                                        \
	(FIELD_PREP(SIWX91X_PINCTRL_PORT_MASK, port) | FIELD_PREP(SIWX91X_PINCTRL_PIN_MASK, pin) | \
	 FIELD_PREP(SIWX91X_PINCTRL_ULPPIN_MASK, ulppin) |                                         \
	 FIELD_PREP(SIWX91X_PINCTRL_MODE_MASK, mode) |                                             \
	 FIELD_PREP(SIWX91X_PINCTRL_ULPMODE_MASK, ulpmode) |                                       \
	 FIELD_PREP(SIWX91X_PINCTRL_PAD_MASK, pad))

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SILABS_PINCTRL_SIWX91X_H_ */
