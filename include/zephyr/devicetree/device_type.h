/*
 * Copyright (c) 2024 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree device type
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_DEVICE_TYPE_H_
#define ZEPHYR_INCLUDE_DEVICETREE_DEVICE_TYPE_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-device-type Devicetree device type
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Does a devicetree node have a zephyr,device-type property?
 *
 * Tests whether a devicetree node has a zephyr,device-type property defined.
 *
 * @param node_id node identifier
 * @return 1 if the node has the zephyr,device-type property, 0 otherwise.
 */
#define DT_NODE_HAS_DEVICE_TYPE(node_id) DT_NODE_HAS_PROP(node_id, zephyr_device_type)

/**
 * @brief Does a devicetree node have a matching zephyr,device-type property?
 *
 * Tests whether a devicetree node has a zephyr,device-type property matching @p device_type.
 *
 * @param node_id node identifier
 * @param device_type lowercase-and-underscores device type
 * @return 1 if the node has a matching zephyr,device-type property, 0 otherwise.
 */
#define DT_NODE_IS_DEVICE_TYPE(node_id, device_type)                                               \
	IS_ENABLED(DT_CAT3(node_id, _ZEPHYR_DEVICE_TYPE_, device_type))

/**
 * @brief Does a devicetree node have a matching zephyr,device-type property and status `okay`?
 *
 * Tests whether a devicetree node has a zephyr,device-type property matching @p device_type and
 * status `okay` (as usual, a missing status property is treated as status `okay`).
 *
 * @param node_id node identifier
 * @param device_type lowercase-and-underscores device type
 * @return 1 if the node has a matching zephyr,device-type property and status `okay`, 0 otherwise.
 */
#define DT_NODE_IS_DEVICE_TYPE_STATUS_OKAY(node_id, device_type)                                   \
	UTIL_AND(DT_NODE_IS_DEVICE_TYPE(node_id, device_type), DT_NODE_HAS_STATUS(node_id, okay))

/**
 * @cond INTERNAL_HIDDEN
 */

#define DT_FOREACH_DEVICE_TYPE_NODE_INTERNAL(node_id, device_type, fn)                             \
	COND_CODE_1(DT_NODE_IS_DEVICE_TYPE(node_id, device_type), (fn(node_id)), ())

#define DT_FOREACH_DEVICE_TYPE_NODE_VARGS_INTERNAL(node_id, device_type, fn, ...)                  \
	COND_CODE_1(DT_NODE_IS_DEVICE_TYPE(node_id, device_type), (fn(node_id, __VA_ARGS__)), ())

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Invokes @p fn for every node in the tree with a ``zephyr,device-type`` property matching
 * @p device_type.
 *
 * The macro @p fn must take one parameter, which will be a node identifier. The macro is expanded
 * once for each node in the tree with a ``zephyr,device-type`` property matching @p
 * device_type. The order that nodes are visited in is not specified.
 *
 * @param device_type lowercase-and-underscores device type
 * @param fn macro to invoke
 */
#define DT_FOREACH_DEVICE_TYPE_NODE(device_type, fn)                                               \
	DT_FOREACH_NODE_VARGS(DT_FOREACH_DEVICE_TYPE_NODE_INTERNAL, device_type, fn)

/**
 * @brief Invokes @p fn for every node in the tree with a ``zephyr,device-type`` property matching
 * @p device_type with multiple arguments.
 *
 * The macro @p fn takes multiple arguments. The first should be the node identifier for the
 * node. The macro is expanded once for each node in the tree with a ``zephyr,device-type`` property
 * matching @p device_type. The order that nodes are visited in is not specified.
 *
 * @param device_type lowercase-and-underscores device type
 * @param fn macro to invoke
 */
#define DT_FOREACH_DEVICE_TYPE_NODE_VARGS(device_type, fn, ...)                                    \
	DT_FOREACH_NODE_VARGS(DT_FOREACH_DEVICE_TYPE_NODE_VARGS_INTERNAL, device_type, fn,         \
			      __VA_ARGS__)

/**
 * @brief Invokes @p fn for every node in the tree with a ``zephyr,device-type`` property matching
 * @p device_type.
 *
 * The macro @p fn must take one parameter, which will be a node identifier. The macro is expanded
 * once for each node in the tree with a ``zephyr,device-type`` property matching @p device_type and
 * with status `okay` (as usual, a missing status property is treated as status `okay`). The order
 * that nodes are visited in is not specified.
 *
 * @param device_type lowercase-and-underscores device type
 * @param fn macro to invoke
 */
#define DT_FOREACH_DEVICE_TYPE_STATUS_OKAY_NODE(device_type, fn)                                   \
	DT_FOREACH_STATUS_OKAY_NODE_VARGS(DT_FOREACH_DEVICE_TYPE_NODE_INTERNAL, device_type, fn)

/**
 * @brief Invokes @p fn for every node in the tree with a ``zephyr,device-type`` property matching
 * @p device_type with multiple arguments.
 *
 * The macro @p fn takes multiple arguments. The first should be the node identifier for the
 * node. The macro is expanded once for each node in the tree with a ``zephyr,device-type`` property
 * matching @p device_type and with status `okay` (as usual, a missing status property is treated as
 * status `okay`). The order that nodes are visited in is not specified.
 *
 * @param device_type lowercase-and-underscores device type
 * @param fn macro to invoke
 */
#define DT_FOREACH_DEVICE_TYPE_STATUS_OKAY_NODE_VARGS(device_type, fn, ...)                        \
	DT_FOREACH_STATUS_OKAY_NODE_VARGS(DT_FOREACH_DEVICE_TYPE_NODE_VARGS_INTERNAL, device_type, \
					  fn, __VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_DEVICE_TYPE_H_ */
