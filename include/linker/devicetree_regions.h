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

/* Automatically generate a region from a devicetree node */
#define _REGION_FROM_NODE(attr, node)				   \
	COND_CODE_1(DT_NODE_HAS_PROP(node, label),		   \
		    (_REGION_DECLARE(DT_LABEL(node), attr, node)), \
		    (_REGION_DECLARE(DT_NODE_FULL_NAME(node), attr, node)))

/* Read-Write memory region from a 'linkable-memory' compatible */
#define _LINKABLE_MEMORY_DECLARE(inst) \
	_REGION_FROM_NODE(rw, DT_INST(inst, linkable_memory))

/**
 * @brief Generate a linker memory region from a devicetree node
 *
 * @param name name of the generated memory region
 * @param attr region attributes to use (rx, rw, ...)
 * @param node devicetree node with a <reg> property defining region location
 *             and size.
 */
#define DT_REGION_FROM_NODE_STATUS_OKAY(name, attr, node) \
	COND_CODE_1(DT_NODE_HAS_STATUS(node, okay),	  \
		    (_REGION_DECLARE(name, attr, node)),  \
		    ())

/**
 * @brief Generate a linker memory region for each 'linkable-memory' compatible
 */
#define DT_LINKABLE_MEMORY_REGIONS() \
	DT_COMPAT_FOREACH_STATUS_OKAY(linkable_memory, _LINKABLE_MEMORY_DECLARE)
