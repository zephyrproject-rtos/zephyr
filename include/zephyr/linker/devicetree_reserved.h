/*
 * Copyright (c) 2021, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions and sections from reserved-memory nodes.
 */

#include <zephyr/devicetree.h>

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
#define LINKER_DT_RESERVED_MEM_REGIONS() _RESERVED_REGION_APPLY(_RESERVED_REGION_DECLARE)

/**
 * @brief Generate section definitions for all the reserved memory regions
 */
#define LINKER_DT_RESERVED_MEM_SECTIONS() _RESERVED_REGION_APPLY(_RESERVED_SECTION_DECLARE)

/**
 * @brief Generate linker script symbols for all the reserved memory regions
 */
#define LINKER_DT_RESERVED_MEM_SYMBOLS() _RESERVED_REGION_APPLY(_RESERVED_SYMBOL_DECLARE)

/**
 * @brief Get the pointer to the reserved-memory region
 *
 * Example devicetree fragment:
 *
 *     reserved: reserved-memory {
 *         compatible = "reserved-memory";
 *         ...
 *         n: node {
 *             reg = <0x42000000 0x1000>;
 *         };
 *      };
 *
 * Example usage:
 *
 *     LINKER_DT_RESERVED_MEM_GET_PTR(DT_NODELABEL(n)) // (uint8_t *) 0x42000000
 *
 * @param node_id node identifier
 * @return pointer to the beginning of the reserved-memory region
 */
#define LINKER_DT_RESERVED_MEM_GET_PTR(node_id) _DT_RESERVED_START(node_id)

/**
 * @brief Get the size of the reserved-memory region
 *
 * Example devicetree fragment:
 *
 *     reserved: reserved-memory {
 *         compatible = "reserved-memory";
 *         ...
 *         n: node {
 *             reg = <0x42000000 0x1000>;
 *         };
 *     };
 *
 * Example usage:
 *
 *     LINKER_DT_RESERVED_MEM_GET_SIZE(DT_NODELABEL(n)) // 0x1000
 *
 * @param node_id node identifier
 * @return the size of the reserved-memory region
 */
#define LINKER_DT_RESERVED_MEM_GET_SIZE(node_id) DT_REG_SIZE(node_id)

/**
 * @brief Get the pointer to the reserved-memory region from a memory-reserved
 *        phandle
 *
 * Example devicetree fragment:
 *
 *     reserved: reserved-memory {
 *         compatible = "reserved-memory";
 *         ...
 *         res0: res {
 *             reg = <0x42000000 0x1000>;
 *             label = "res0";
 *         };
 *     };
 *
 *     n: node {
 *         memory-region = <&res0>;
 *     };
 *
 * Example usage:
 *
 *     LINKER_DT_RESERVED_MEM_GET_PTR_BY_PHANDLE(DT_NODELABEL(n), \
 *						 memory_region) // (uint8_t *) 0x42000000
 *
 * @param node_id node identifier
 * @param ph phandle to reserved-memory region
 *
 * @return pointer to the beginning of the reserved-memory region
 */
#define LINKER_DT_RESERVED_MEM_GET_PTR_BY_PHANDLE(node_id, ph) \
	LINKER_DT_RESERVED_MEM_GET_PTR(DT_PHANDLE(node_id, ph))

/**
 * @brief Get the size of the reserved-memory region from a memory-reserved
 *        phandle
 *
 * Example devicetree fragment:
 *
 *     reserved: reserved-memory {
 *         compatible = "reserved-memory";
 *         ...
 *         res0: res {
 *             reg = <0x42000000 0x1000>;
 *             label = "res0";
 *         };
 *     };
 *
 *     n: node {
 *         memory-region = <&res0>;
 *     };
 *
 * Example usage:
 *
 *     LINKER_DT_RESERVED_MEM_GET_SIZE_BY_PHANDLE(DT_NODELABEL(n), \
 *						  memory_region) // (uint8_t *) 0x42000000
 *
 * @param node_id node identifier
 * @param ph phandle to reserved-memory region
 *
 * @return size of the reserved-memory region
 */
#define LINKER_DT_RESERVED_MEM_GET_SIZE_BY_PHANDLE(node_id, ph) \
	LINKER_DT_RESERVED_MEM_GET_SIZE(DT_PHANDLE(node_id, ph))
