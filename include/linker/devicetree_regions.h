/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions from devicetree nodes.
 */

/* Declare a memory region */
#define _REGION_DECLARE(name, attr, node) name(attr) : \
	ORIGIN = DT_REG_ADDR(node),		       \
	LENGTH = DT_REG_SIZE(node)

/**
 * @brief Generate a linker memory region from a devicetree node
 *
 * If @p node_id refers to a node with status "okay", then this declares
 * a linker memory region named @p name with attributes from @p attr.
 *
 * Otherwise, it doesn't expand to anything.
 *
 * @param name name of the generated memory region
 * @param attr region attributes to use (rx, rw, ...)
 * @param node_id devicetree node identifier with a \<reg\> property
 *                defining region location and size.
 */
#define LINKER_DT_REGION_FROM_NODE(name, attr, node_id)		\
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),		\
		    (_REGION_DECLARE(name, attr, node_id)),	\
		    ())
