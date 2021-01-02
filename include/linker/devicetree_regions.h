/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions from devicetree partitions.
 * Partitions are only considered if they exist under chosen/zephyr,flash.
 */

/* Partitions under chosen/zephyr,flash */
#define _CHOSEN_PARTITIONS(p) UTIL_CAT(DT_CHOSEN(p), _S_partitions)

/* Declare a memory region */
#define _REGION_DECLARE(node) DT_LABEL(node) (rx) : \
	ORIGIN = DT_REG_ADDR(node),		    \
	LENGTH = DT_REG_SIZE(node)

/* Apply a macro to a partition list, if it exists */
#define _FLASH_PARITIONS_APPLY(p, f) \
	COND_CODE_1(DT_NODE_EXISTS(p), (DT_FOREACH_CHILD(p, f)), ())

/* Apply a macro to a node, if it is 'okay' */
#define _OKAY_NODE_APPLY(n, f) \
	COND_CODE_1(DT_NODE_HAS_STATUS(n, okay), (f(n)), ())

/* Execution partition */
#define DT_CODE_PARITION() DT_LABEL(DT_CHOSEN(zephyr_code_partition))

/**
 * @brief Generate region definitions for all devicetree linker regions
 */
#define DT_REGIONS_FROM_CHOSEN_FLASH(chosen_flash)		 \
	_FLASH_PARITIONS_APPLY(_CHOSEN_PARTITIONS(chosen_flash), \
			       _REGION_DECLARE)

/**
 * @brief Generate a region definition from an enabled devicetree node
 */
#define DT_REGION_FROM_NODE(node)			       \
	COND_CODE_1(DT_NODE_EXISTS(node),		       \
		    (_OKAY_NODE_APPLY(node, _REGION_DECLARE)), \
		    ())
