/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions from devicetree partitions.
 * Partitions are only considered if they exist under chosen/zephyr,flash.
 */

/* Partition under chosen/zephyr,flash */
#define _CHOSEN_PARTITIONS UTIL_CAT(DT_CHOSEN(zephyr_flash), _S_partitions)

/* Declare a memory region definition if `linker-region` is set */
#define _REGION_DECLARE(part) COND_CODE_1(DT_PROP(part, linker_region),	\
					  (DT_LABEL(part) (rx) :	\
					   ORIGIN = DT_REG_ADDR(part),	\
					   LENGTH = DT_REG_SIZE(part)),	\
					  ())

/* Return a memory region origin if `linker-region` is set*/
#define _REGION_ADDR(part) COND_CODE_1(DT_PROP(part, linker_region), \
				       (DT_REG_ADDR(part),),	     \
				       ())

/* Apply a macro to chosen partition, if it exists */
#define _REGION_APPLY(f)					       \
	COND_CODE_1(IS_ENABLED(UTIL_CAT(_CHOSEN_PARTITIONS, _EXISTS)), \
		    (DT_FOREACH_CHILD(_CHOSEN_PARTITIONS, f)), ())

/**
 * @brief Find the lowest start address of all devicetree linker regions
 *
 * The MIN symbol passed to `FOR_EACH_NESTED` does not resolve to the standard
 * ternery operation macro. Instead it is pasted as a symbol into the final
 * linker file, where MIN is a builtin function in ld.
 *     https://sourceware.org/binutils/docs/ld/Builtin-Functions.html
 *
 * @param def default return value if no custom partitions are present
 */
#define DT_PARTITION_REGIONS_START(def)	\
	FOR_EACH_NESTED(MIN, def, _REGION_APPLY(_REGION_ADDR))

/**
 * @brief Generate region definitions for all devicetree linker regions
 */
#define DT_PARTITION_REGIONS() _REGION_APPLY(_REGION_DECLARE)
