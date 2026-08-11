/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ITE IT51XXX wake-up controller devicetree macros
 * @ingroup dt_ite_it51xxx_wuc
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_IT51XXX_WUC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_IT51XXX_WUC_H_

/**
 * @defgroup dt_ite_it51xxx_wuc ITE IT51XXX wake-up controller
 * @brief Devicetree macros for the ITE IT51XXX Wake-Up Controller (WUC).
 * @ingroup devicetree-interrupt_controller
 *
 * Macros for describing wake-up controller cells used with the <tt>ite,it51xxx-wuc</tt> and
 * <tt>ite,it51xxx-wuc-map</tt> compatible controllers. A WUC mapping references a WUC group node
 * and the bit mask of the wake-up input within that group; @ref IT51XXX_WUC_UNUSED_REG is used
 * where a group has no associated edge-trigger register.
 *
 * @code{.dts}
 * wuc_wu20: wu20 {
 *         wucs = <&wuc2 BIT(0)>;
 * };
 * @endcode
 * @{
 */

/** WUC reserved register of reg property */
#define IT51XXX_WUC_UNUSED_REG 0

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ITE_IT51XXX_WUC_H_ */
