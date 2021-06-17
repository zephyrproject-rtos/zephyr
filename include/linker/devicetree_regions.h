/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions from devicetree nodes.
 */

/* Declare a memory region */
#define _REGION_DECLARE(node, attr) DT_REGION_NAME(node)(attr) : \
	ORIGIN = DT_REG_ADDR(node),				 \
	LENGTH = DT_REG_SIZE(node)

/**
 * @brief Generate a linker memory region from a devicetree node
 *
 * @param name name of the generated memory region
 * @param attr region attributes to use (rx, rw, ...)
 * @param node devicetree node with a \<reg\> property defining region location
 *             and size.
 */
#define DT_REGION_FROM_NODE_STATUS_OKAY(name, attr, node) \
	COND_CODE_1(DT_NODE_HAS_STATUS(node, okay),	  \
		    (_REGION_DECLARE(name, attr, node)),  \
		    ())

/**
 * @brief Get a memory region name from a devicetree node
 *
 * @param node_id node identifier
 * @retval unquoted node label
 */
#define DT_REGION_NAME(node_id)		  \
	DT_NODE_NODELABEL_BY_IDX(node_id, \
				 UTIL_DECR(DT_NODE_NUM_NODELABELS(node_id)))
