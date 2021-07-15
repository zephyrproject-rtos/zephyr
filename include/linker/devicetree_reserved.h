/*
 * Copyright (c) 2021, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions and sections from reserved-memory nodes.
 */

#include <devicetree.h>

/* Reserved memory node */
#define _NODE_RESERVED DT_INST(0, reserved_memory)

/* Unquoted region label */
#define _DT_LABEL_TOKEN(res) DT_STRING_TOKEN(res, label)

/* _start and _end section symbols */
#define _DT_RESERVED_PREFIX(res) UTIL_CAT(__, _DT_LABEL_TOKEN(res))
#define _DT_RESERVED_START(res)  UTIL_CAT(_DT_RESERVED_PREFIX(res), _start)
#define _DT_RESERVED_END(res)    UTIL_CAT(_DT_RESERVED_PREFIX(res), _end)

/* Declare a reserved memory region */
#define _RESERVED_REGION_DECLARE(res) DT_STRING_TOKEN(res, label) (rw) :	\
				      ORIGIN = DT_REG_ADDR(res),		\
				      LENGTH = DT_REG_SIZE(res)

/* Declare a reserved memory section */
#define _RESERVED_SECTION_DECLARE(res) SECTION_DATA_PROLOGUE(_DT_LABEL_TOKEN(res), ,)	\
				       {						\
					  _DT_RESERVED_START(res) = .;			\
					  KEEP(*(._DT_LABEL_TOKEN(res)))		\
					  KEEP(*(._DT_LABEL_TOKEN(res).*))		\
					  _DT_RESERVED_END(res) =			\
					  _DT_RESERVED_START(res) + DT_REG_SIZE(res);	\
				       } GROUP_LINK_IN(_DT_LABEL_TOKEN(res))

/* Declare reserved memory linker symbols */
#define _RESERVED_SYMBOL_DECLARE(res) extern char _DT_RESERVED_START(res)[];	\
				      extern char _DT_RESERVED_END(res)[];

/* Apply a macro to a reserved memory region */
#define _RESERVED_REGION_APPLY(f)						\
	COND_CODE_1(IS_ENABLED(UTIL_CAT(_NODE_RESERVED, _EXISTS)),		\
		    (DT_FOREACH_CHILD(_NODE_RESERVED, f)), ())

/**
 * @brief Generate region definitions for all the reserved memory regions
 */
#define DT_RESERVED_MEM_REGIONS() _RESERVED_REGION_APPLY(_RESERVED_REGION_DECLARE)

/**
 * @brief Generate section definitions for all the reserved memory regions
 */
#define DT_RESERVED_MEM_SECTIONS() _RESERVED_REGION_APPLY(_RESERVED_SECTION_DECLARE)

/**
 * @brief Generate linker script symbols for all the reserved memory regions
 */
#define DT_RESERVED_MEM_SYMBOLS() _RESERVED_REGION_APPLY(_RESERVED_SYMBOL_DECLARE)
