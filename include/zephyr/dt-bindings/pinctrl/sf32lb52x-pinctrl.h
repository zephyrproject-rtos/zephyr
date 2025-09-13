/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB52X_PINCTRL_H_
#define _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB52X_PINCTRL_H_

#include "sf32lb-common-pinctrl.h"

/* ports */
#define SF32LB_PORT_SA 0U
#define SF32LB_PORT_PA 1U

/* valid configurations */
#define PA18_USART1_RXD    SF32LB_PINMUX(PA, 18U, 4U, 0x58U, 1U)
#define PA19_USART1_TXD    SF32LB_PINMUX(PA, 19U, 4U, 0x58U, 0U)

#endif /* _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB52X_PINCTRL_H_ */
