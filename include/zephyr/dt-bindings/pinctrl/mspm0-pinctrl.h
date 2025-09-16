/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MSPM0_DT_BINDINGS_PINCTRL_H_
#define _MSPM0_DT_BINDINGS_PINCTRL_H_

#define MSP_PORT_INDEX_BY_NAME(x) ((x == "PORTA") ? 0 : 1)

#define MSPM0_PIN_FUNCTION_ANALOG (0x00000000)
#define MSPM0_PIN_FUNCTION_GPIO   (0x00000001)
#define MSPM0_PIN_FUNCTION_2      (0x00000002)
#define MSPM0_PIN_FUNCTION_3      (0x00000003)
#define MSPM0_PIN_FUNCTION_4      (0x00000004)
#define MSPM0_PIN_FUNCTION_5      (0x00000005)
#define MSPM0_PIN_FUNCTION_6      (0x00000006)
#define MSPM0_PIN_FUNCTION_7      (0x00000007)
#define MSPM0_PIN_FUNCTION_8      (0x00000008)
#define MSPM0_PIN_FUNCTION_9      (0x00000009)
#define MSPM0_PIN_FUNCTION_10     (0x0000000A)
#define MSPM0_PIN_FUNCTION_11     (0x0000000B)
#define MSPM0_PIN_FUNCTION_12     (0x0000000C)
#define MSPM0_PIN_FUNCTION_13     (0x0000000D)
#define MSPM0_PIN_FUNCTION_14     (0x0000000E)
#define MSPM0_PIN_FUNCTION_15     (0x0000000F)

/* Creates a concatenation of the correct pin function based on the pin control
 * management register offset and the function suffix.
 */
#define MSP_PINMUX(pincm, function) (((pincm - 1) << 0x10) | function)

#endif
