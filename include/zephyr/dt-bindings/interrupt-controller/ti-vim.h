/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TI VIM interrupt controller devicetree macros
 * @ingroup dt_ti_vim
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_VIM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_VIM_H_

#include <zephyr/sys/util_macro.h>

/**
 * @defgroup dt_ti_vim TI VIM interrupt controller
 * @brief Devicetree flags for the Texas Instruments Vectored Interrupt Manager (VIM).
 * @ingroup devicetree-interrupt_controller
 *
 * Trigger-type and priority flags for the <tt>ti,vim</tt> compatible interrupt controller. An
 * interrupt is described by four cells: the interrupt group, the interrupt number, the trigger type
 * (@ref IRQ_TYPE_LEVEL or @ref IRQ_TYPE_EDGE) and the priority.
 *
 * @code{.dts}
 * &some_device {
 *         interrupts = <0 64 IRQ_TYPE_EDGE IRQ_DEFAULT_PRIORITY>;
 * };
 * @endcode
 * @{
 */

#define IRQ_TYPE_LEVEL  BIT(1) /**< Level-triggered interrupt */
#define IRQ_TYPE_EDGE   BIT(2) /**< Edge-triggered interrupt */

#define IRQ_DEFAULT_PRIORITY  0xf /**< Default interrupt priority */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_VIM_H_ */
