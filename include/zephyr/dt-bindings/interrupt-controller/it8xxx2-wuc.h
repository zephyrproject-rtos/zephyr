/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ITE IT8XXX2 wake-up controller devicetree macros
 * @ingroup dt_ite_it8xxx2_wuc
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup dt_ite_it8xxx2_wuc ITE IT8XXX2 wake-up controller
 * @brief Devicetree macros for the ITE IT8XXX2 Wake-Up Controller (WUC).
 * @ingroup devicetree-interrupt_controller
 *
 * Macros for describing wake-up controller cells used with the <tt>ite,it8xxx2-wuc</tt> and
 * <tt>ite,it8xxx2-wuc-map</tt> compatible controllers. A WUC mapping references a WUC group node
 * and the bit mask of the wake-up input within that group; @ref IT8XXX2_WUC_UNUSED_REG is used
 * where a group has no associated edge-trigger register. The @c WUC_TYPE_EDGE_* flags select the
 * edge trigger mode of a wake-up input.
 *
 * @code{.dts}
 * wuc_wu20: wu20 {
 *         wucs = <&wuc2 BIT(0)>;
 * };
 * @endcode
 * @{
 */

/** WUC reserved register of reg property */
#define IT8XXX2_WUC_UNUSED_REG	0

/**
 * @name wakeup controller flags
 * @{
 */
/** WUC rising edge trigger mode */
#define WUC_TYPE_EDGE_RISING	BIT(0)
/** WUC falling edge trigger mode */
#define WUC_TYPE_EDGE_FALLING	BIT(1)
/** WUC both edge trigger mode */
#define WUC_TYPE_EDGE_BOTH	(WUC_TYPE_EDGE_RISING | WUC_TYPE_EDGE_FALLING)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_ */
