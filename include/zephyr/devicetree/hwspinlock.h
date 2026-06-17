/*
 * Copyright (c) 2025 Sequans Communications
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief HW spinlock Devicetree macro public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_HWSPINLOCK_H_
#define ZEPHYR_INCLUDE_DEVICETREE_HWSPINLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-hwspinlock Devicetree HW spinlock API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the node identifier for the hardware spinlock controller from a <tt>hwlocks</tt>
 * property by index
 *
 * Example devicetree fragment:
 *
 *     hwlock1: hwspinlock-controller@... { ... };
 *     hwlock2: hwspinlock-controller@... { ... };
 *
 *     n: node {
 *             hwlocks = <&hwlock1 8>,
 *                       <&hwlock2 1>;
 *             hwlock-names = "rd", "wr";
 *     };
 *
 * Example usage:
 *
 *     DT_HWSPINLOCK_CTRL_BY_IDX(DT_NODELABEL(n), 0) // DT_NODELABEL(hwlock1)
 *     DT_HWSPINLOCK_CTRL_BY_IDX(DT_NODELABEL(n), 1) // DT_NODELABEL(hwlock2)
 *
 * @param node_id node identifier for a node with a <tt>hwlocks</tt> property
 * @param idx index of an element in the <tt>hwlocks</tt> property
 *
 * @return the node identifier for the hardware spinlock controller in the named element
 *
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_HWSPINLOCK_CTRL_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, hwlocks, idx)

/**
 * @brief Get the node identifier for the hardware spinlock controller from a <tt>hwlocks</tt>
 *        property by name
 *
 * Example devicetree fragment:
 *
 *     hwlock1: hwspinlock-controller@... { ... };
 *     hwlock2: hwspinlock-controller@... { ... };
 *
 *     n: node {
 *             hwlocks = <&hwlock1 8>,
 *                       <&hwlock2 1>;
 *             hwlock-names = "rd", "wr";
 *     };
 *
 * Example usage:
 *
 *     DT_HWSPINLOCK_CTRL_BY_NAME(DT_NODELABEL(n), rd) // DT_NODELABEL(hwlock1)
 *     DT_HWSPINLOCK_CTRL_BY_NAME(DT_NODELABEL(n), wr) // DT_NODELABEL(hwlock2)
 *
 * @param node_id node identifier for a node with a <tt>hwlocks</tt> property
 * @param name lowercase-and-underscores name of a <tt>hwlocks</tt> element
 *             as defined by the node's <tt>hwlocks-names</tt> property
 *
 * @return the node identifier for the hardware spinlock controller in the named element
 *
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_HWSPINLOCK_CTRL_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, hwlocks, name)

/**
 * @brief Get a hardware spinlock <tt>id</tt> field by name
 *
 * Example devicetree fragment:
 *
 *     hwlock1: hwspinlock-controller@... {
 *             #hwlock-cells = <1>;
 *     };
 *
 *     n: node {
 *             hwlocks = <&hwlock1 1>,
 *                       <&hwlock1 6>;
 *             hwlock-names = "rd", "wr";
 *     };
 *
 * Bindings fragment for the hwspinlock compatible:
 *
 *     hwlock-cells:
 *       - id
 *
 * Example usage:
 *
 *     DT_HWSPINLOCK_ID_BY_NAME(DT_NODELABEL(n), rd) // 1
 *     DT_HWSPINLOCK_ID_BY_NAME(DT_NODELABEL(n), wr) // 6
 *
 * @param node_id node identifier for a node with a <tt>hwlocks</tt> property
 * @param name lowercase-and-underscores name of a <tt>hwlocks</tt> element
 *             as defined by the node's <tt>hwlock-names</tt> property
 *
 * @return the channel value in the specifier at the named element or 0 if no
 *         named hardware spinlockis found
 *
 * @see DT_PHA_BY_NAME()
 */
#define DT_HWSPINLOCK_ID_BY_NAME(node_id, name) \
	DT_PHA_BY_NAME(node_id, hwlocks, name, id)

/**
 * @brief Get a hardware spinlock id by index
 *
 * Example devicetree fragment:
 *
 *     hwlock1: hwspinlock-controller@... {
 *             #hwlock-cells = <1>;
 *     };
 *
 *     n: node {
 *             hwlocks = <&hwlock1 1>,
 *                       <&hwlock1 6>;
 *     };
 *
 * Example usage:
 *
 *     DT_HWSPINLOCK_ID_BY_IDX(DT_NODELABEL(n), 0) // 1
 *     DT_HWSPINLOCK_ID_BY_IDX(DT_NODELABEL(n), 1) // 6
 *
 * @param node_id node identifier for a node with a <tt>hwlocks</tt> property
 * @param idx index of a <tt>hwlocks</tt> element in the <tt>hwlocks</tt>
 *
 * @return the channel value in the specifier at the indexed element or 0 if no
 *         hardware spinlock is found is @p idx index
 *
 * @see DT_PHA_BY_IDX()
 */
#define DT_HWSPINLOCK_ID_BY_IDX(node_id, idx) \
	DT_PHA_BY_IDX(node_id, hwlocks, idx, id)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_HWSPINLOCK_H_ */
