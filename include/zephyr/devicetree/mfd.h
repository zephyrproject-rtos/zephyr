/*
 * Copyright (c) 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Devicetree macros for multi-functional devices.
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MFD_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MFD_H_

/**
 * @brief Multi-functional device devicetree macros
 * @defgroup multi_functional_devices Multi-functional devices
 * @ingroup multi_functional_devices
 * @{
 */

/**
 * @def MFD_DT_PARENT_GET
 *
 * @brief Validate and get parent devicetree node
 *
 * @details Expands to the parent devicetree node if the parent node
 * is enabled and its compatible property value matches the compat
 * parameter. Otherwise expands to ()
 *
 * @param node_id The child's devicetree node identifier
 * @param compat The parent node's expected compatible property value
 * @return Parent struct device
 */
#define MFD_DT_PARENT_GET(node_id, compat) \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(DT_PARENT(node_id), compat), \
			(DEVICE_DT_GET(DT_PARENT(node_id))), ())

/**
 * @def MFD_DT_CHILD_GET
 *
 * @brief Validate and get child devicetree node
 *
 * @details Expands to the child devicetree node if the child node's
 * compatible property value matches the compat parameter. Otherwise
 * expands to ()
 *
 * @param node_id The parent's devicetree node identifier
 * @param child The child's devicetree node identifier
 * @param compat The child node's expected compatible property value
 * @return Child struct device
 */
#define MFD_DT_CHILD_GET(node_id, child, compat) \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(DT_CHILD(node_id, child), compat), \
			(DEVICE_DT_GET(DT_CHILD(node_id, child))), ())

/**
 * @def MFD_DT_CHILD_GET_OR_NULL
 */
#define MFD_DT_CHILD_GET_OR_NULL(node_id, child, compat) \
	COND_CODE_1( DT_NODE_EXISTS(DT_CHILD(node_id, child)), \
	(DT_HAS_COMPAT_STATUS_OKAY(DT_CHILD(node_id, child), compat), \
	DT_CHILD(node_id, child), (NULL)), (NULL))

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DEVICETREE_MFD_H_ */
