/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  * @brief SF32LB58X Pin Multiplexer device tree pinctrl bindings.
  *
  */
#ifndef _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB58X_PINCTRL_H_
#define _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB58X_PINCTRL_H_

#include "sf32lb-common-pinctrl.h"

/**
 * @name Pin Multiplexer definitions for SF32LB58X
 * @{
 */

/** port for SA */
#define SF32LB_PORT_SA 0U
/** port for PA */
#define SF32LB_PORT_PA 1U

/** @} */

/**
 * @name Pin Multiplexer definitions for SF32LB58X
 * @{
 */

/** UART2_TXD on PA16 */
#define PA16_UART2_TXD          SF32LB_PINMUX(PA, 16U, 1U, 0U, 0U)

/** UART2_RXD on PA17 */
#define PA17_UART2_RXD          SF32LB_PINMUX(PA, 17U, 1U, 0U, 0U)

/** UART1_TXD on PA31 */
#define PA31_UART1_TXD          SF32LB_PINMUX(PA, 31U, 1U, 0U, 0U)

/** UART1_RXD on PA32 */
#define PA32_UART1_RXD          SF32LB_PINMUX(PA, 32U, 1U, 0U, 0U)



/** @} */

#endif /* _INCLUDE_ZEPHYR_DT_BINDINGS_PINCTRL_SF32LB58X_PINCTRL_H_ */
