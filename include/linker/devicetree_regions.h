/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions from devicetree nodes.
 */

/* Declare a memory region */
#define _REGION_DECLARE(node, attr) DT_LABEL(node)(attr) : \
	ORIGIN = DT_REG_ADDR(node),			   \
	LENGTH = DT_REG_SIZE(node)

/**
 * @brief Generate a linker memory region from a devicetree node
 *
 * @param node devicetree node with a \<reg\> property defining region location
 *             and size and a \<label\> property defining the region name
 * @param attr region attributes to use (rx, rw, ...)
 */
#define DT_REGION_FROM_NODE_STATUS_OKAY(node, attr) \
	COND_CODE_1(DT_NODE_HAS_STATUS(node, okay), \
		    (_REGION_DECLARE(node, attr)),  \
		    ())
