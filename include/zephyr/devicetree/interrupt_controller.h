/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt controller devicetree macro public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_INTERRUPT_CONTROLLER_H_
#define ZEPHYR_INCLUDE_DEVICETREE_INTERRUPT_CONTROLLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

/**
 * @defgroup devicetree-interrupt_controller Devicetree Interrupt Controller API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the aggregator level of an interrupt controller
 *
 * @note Aggregator level is equivalent to IRQ_LEVEL + 1 (a 2nd level aggregator has Zephyr level 1
 * IRQ encoding)
 *
 * @param node_id node identifier of an interrupt controller
 *
 * @return Level of the interrupt controller
 */
#define DT_INTC_GET_AGGREGATOR_LEVEL(node_id) UTIL_INC(DT_IRQ_LEVEL(node_id))

/**
 * @brief Get the aggregator level of a `DT_DRV_COMPAT` interrupt controller
 *
 * @note Aggregator level is equivalent to IRQ_LEVEL + 1 (a 2nd level aggregator has Zephyr level 1
 * IRQ encoding)
 *
 * @param inst instance of an interrupt controller
 *
 * @return Level of the interrupt controller
 */
#define DT_INST_INTC_GET_AGGREGATOR_LEVEL(inst) DT_INTC_GET_AGGREGATOR_LEVEL(DT_DRV_INST(inst))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_INTERRUPT_CONTROLLER_H_ */
