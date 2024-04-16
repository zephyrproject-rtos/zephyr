/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_ORDINALS_H_
#define ZEPHYR_INCLUDE_DEVICETREE_ORDINALS_H_

/**
 * @file
 * @brief Devicetree node dependency ordinals
 */

/**
 * @defgroup devicetree-dep-ord Dependency tracking
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a node's dependency ordinal
 * @param node_id Node identifier
 * @return the node's dependency ordinal as an integer literal
 */
#define DT_DEP_ORD(node_id) DT_CAT(node_id, _ORD)

/**
 * @brief Get a node's dependency ordinal in string sortable form
 * @param node_id Node identifier
 * @return the node's dependency ordinal as a zero-padded integer literal
 */
#define DT_DEP_ORD_STR_SORTABLE(node_id) DT_CAT(node_id, _ORD_STR_SORTABLE)

/**
 * @brief Get a list of dependency ordinals of a node's direct dependencies
 *
 * There is a comma after each ordinal in the expansion, **including**
 * the last one:
 *
 *     DT_REQUIRES_DEP_ORDS(my_node) // required_ord_1, ..., required_ord_n,
 *
 * The one case DT_REQUIRES_DEP_ORDS() expands to nothing is when
 * given the root node identifier @p DT_ROOT as argument. The root has
 * no direct dependencies; every other node at least depends on its
 * parent.
 *
 * @param node_id Node identifier
 * @return a list of dependency ordinals, with each ordinal followed
 *         by a comma (<tt>,</tt>), or an empty expansion
 */
#define DT_REQUIRES_DEP_ORDS(node_id) DT_CAT(node_id, _REQUIRES_ORDS)

/**
 * @brief Get a list of dependency ordinals of what depends directly on a node
 *
 * There is a comma after each ordinal in the expansion, **including**
 * the last one:
 *
 *     DT_SUPPORTS_DEP_ORDS(my_node) // supported_ord_1, ..., supported_ord_n,
 *
 * DT_SUPPORTS_DEP_ORDS() may expand to nothing. This happens when @p node_id
 * refers to a leaf node that nothing else depends on.
 *
 * @param node_id Node identifier
 * @return a list of dependency ordinals, with each ordinal followed
 *         by a comma (<tt>,</tt>), or an empty expansion
 */
#define DT_SUPPORTS_DEP_ORDS(node_id) DT_CAT(node_id, _SUPPORTS_ORDS)

/**
 * @brief Get a DT_DRV_COMPAT instance's dependency ordinal
 *
 * Equivalent to DT_DEP_ORD(DT_DRV_INST(inst)).
 *
 * @param inst instance number
 * @return The instance's dependency ordinal
 */
#define DT_INST_DEP_ORD(inst) DT_DEP_ORD(DT_DRV_INST(inst))

/**
 * @brief Get a list of dependency ordinals of a DT_DRV_COMPAT instance's
 *        direct dependencies
 *
 * Equivalent to DT_REQUIRES_DEP_ORDS(DT_DRV_INST(inst)).
 *
 * @param inst instance number
 * @return a list of dependency ordinals for the nodes the instance depends
 *         on directly
 */
#define DT_INST_REQUIRES_DEP_ORDS(inst) DT_REQUIRES_DEP_ORDS(DT_DRV_INST(inst))

/**
 * @brief Get a list of dependency ordinals of what depends directly on a
 *        DT_DRV_COMPAT instance
 *
 * Equivalent to DT_SUPPORTS_DEP_ORDS(DT_DRV_INST(inst)).
 *
 * @param inst instance number
 * @return a list of node identifiers for the nodes that depend directly
 *         on the instance
 */
#define DT_INST_SUPPORTS_DEP_ORDS(inst) DT_SUPPORTS_DEP_ORDS(DT_DRV_INST(inst))

/**
 * @}
 */

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_ORDINALS_H_ */
