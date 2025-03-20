/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions from devicetree nodes.
 */

#ifndef ZEPHYR_INCLUDE_LINKER_DEVICETREE_REGIONS_H_
#define ZEPHYR_INCLUDE_LINKER_DEVICETREE_REGIONS_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

/**
 * @brief Get the linker memory-region name in a token form
 *
 * This attempts to use the zephyr,memory-region property (with
 * non-alphanumeric characters replaced with underscores) returning a token.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     sram1: memory@2000000 {
 *                         zephyr,memory-region = "MY_NAME";
 *                     };
 *                     sram2: memory@2001000 {
 *                         zephyr,memory-region = "MY@OTHER@NAME";
 *                     };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    LINKER_DT_NODE_REGION_NAME_TOKEN(DT_NODELABEL(sram1)) // MY_NAME
 *    LINKER_DT_NODE_REGION_NAME_TOKEN(DT_NODELABEL(sram2)) // MY_OTHER_NAME
 * @endcode
 *
 * @param node_id node identifier
 * @return the name of the memory memory region the node will generate
 */
#define LINKER_DT_NODE_REGION_NAME_TOKEN(node_id) \
	DT_STRING_TOKEN(node_id, zephyr_memory_region)

/**
 * @brief Get the linker memory-region name
 *
 * This attempts to use the zephyr,memory-region property (with
 * non-alphanumeric characters replaced with underscores).
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     sram1: memory@2000000 {
 *                         zephyr,memory-region = "MY_NAME";
 *                     };
 *                     sram2: memory@2001000 {
 *                         zephyr,memory-region = "MY@OTHER@NAME";
 *                     };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(sram1)) // "MY_NAME"
 *    LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(sram2)) // "MY_OTHER_NAME"
 * @endcode
 *
 * @param node_id node identifier
 * @return the name of the memory memory region the node will generate
 */
#define LINKER_DT_NODE_REGION_NAME(node_id) \
	STRINGIFY(LINKER_DT_NODE_REGION_NAME_TOKEN(node_id))

#define _DT_MEMORY_REGION_FLAGS_TOKEN(n)    DT_STRING_TOKEN(n, zephyr_memory_region_flags)
#define _DT_MEMORY_REGION_FLAGS_UNQUOTED(n) DT_STRING_UNQUOTED(n, zephyr_memory_region_flags)

#define _LINKER_L_PAREN (
#define _LINKER_R_PAREN )
#define _LINKER_ENCLOSE_PAREN(x) _LINKER_L_PAREN x _LINKER_R_PAREN

#define _LINKER_IS_EMPTY_TOKEN_          1
#define _LINKER_IS_EMPTY_TOKEN_EXPAND(x) _LINKER_IS_EMPTY_TOKEN_##x
#define _LINKER_IS_EMPTY_TOKEN(x)        _LINKER_IS_EMPTY_TOKEN_EXPAND(x)

/**
 * @brief Get the linker memory-region flags with parentheses.
 *
 * This attempts to return the zephyr,memory-region-flags property
 * with parentheses.
 * Return empty string if not set the property.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     rx: memory@2000000 {
 *                             zephyr,memory-region = "READ_EXEC";
 *                             zephyr,memory-region-flags = "rx";
 *                     };
 *                     rx_not_w: memory@2001000 {
 *                             zephyr,memory-region = "READ_EXEC_NOT_WRITE";
 *                             zephyr,memory-region-flags = "rx!w";
 *                     };
 *                     no_flags: memory@2001000 {
 *                             zephyr,memory-region = "NO_FLAGS";
 *                     };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    LINKER_DT_NODE_REGION_FLAGS(DT_NODELABEL(rx))       // (rx)
 *    LINKER_DT_NODE_REGION_FLAGS(DT_NODELABEL(rx_not_w)) // (rx!w)
 *    LINKER_DT_NODE_REGION_FLAGS(DT_NODELABEL(no_flags)) // [flags will not be specified]
 * @endcode
 *
 * @param node_id node identifier
 * @return the value of the memory region flag specified in the device tree
 *         enclosed in parentheses.
 */

#define LINKER_DT_NODE_REGION_FLAGS(node_id)                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, zephyr_memory_region_flags),                         \
		    (COND_CODE_1(_LINKER_IS_EMPTY_TOKEN(_DT_MEMORY_REGION_FLAGS_TOKEN(node_id)),   \
				 (),                                                               \
				 (_LINKER_ENCLOSE_PAREN(                                           \
					_DT_MEMORY_REGION_FLAGS_UNQUOTED(node_id))                 \
				 ))),                                                              \
		    (_LINKER_ENCLOSE_PAREN(rw)))

/** @cond INTERNAL_HIDDEN */

#define _DT_COMPATIBLE	zephyr_memory_region

#define _DT_SECTION_PREFIX(node_id)	UTIL_CAT(__, LINKER_DT_NODE_REGION_NAME_TOKEN(node_id))
#define _DT_SECTION_START(node_id)	UTIL_CAT(_DT_SECTION_PREFIX(node_id), _start)
#define _DT_SECTION_END(node_id)	UTIL_CAT(_DT_SECTION_PREFIX(node_id), _end)
#define _DT_SECTION_SIZE(node_id)	UTIL_CAT(_DT_SECTION_PREFIX(node_id), _size)
#define _DT_SECTION_LOAD(node_id)	UTIL_CAT(_DT_SECTION_PREFIX(node_id), _load_start)

/**
 * @brief Declare a memory region
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *    test_sram: sram@20010000 {
 *        compatible = "zephyr,memory-region", "mmio-sram";
 *        reg = < 0x20010000 0x1000 >;
 *        zephyr,memory-region = "FOOBAR";
 *        zephyr,memory-region-flags = "rw";
 *    };
 * @endcode
 *
 * will result in:
 *
 * @code{.unparsed}
 *    FOOBAR (rw) : ORIGIN = (0x20010000), LENGTH = (0x1000)
 * @endcode
 *
 * @param node_id devicetree node identifier
 * @param attr region attributes
 */
#define _REGION_DECLARE(node_id)                                                                   \
	LINKER_DT_NODE_REGION_NAME_TOKEN(node_id)                                                  \
	LINKER_DT_NODE_REGION_FLAGS(node_id)                                                       \
		: ORIGIN = DT_REG_ADDR(node_id),                                                   \
		  LENGTH = DT_REG_SIZE(node_id)

/**
 * @brief Declare a memory section from the device tree nodes with
 *	  compatible 'zephyr,memory-region'
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *    test_sram: sram@20010000 {
 *        compatible = "zephyr,memory-region", "mmio-sram";
 *        reg = < 0x20010000 0x1000 >;
 *        zephyr,memory-region = "FOOBAR";
 *    };
 * @endcode
 *
 * will result in:
 *
 * @code{.unparsed}
 *    FOOBAR (NOLOAD) :
 *    {
 *        __FOOBAR_start = .;
 *        KEEP(*(FOOBAR))
 *        KEEP(*(FOOBAR.*))
 *        __FOOBAR_end = .;
 *    } > FOOBAR
 *    __FOOBAR_size = __FOOBAR_end - __FOOBAR_start;
 *    __FOOBAR_load_start = LOADADDR(FOOBAR);
 * @endcode
 *
 * @param node_id devicetree node identifier
 */
/* clang-format off */
#define _SECTION_DECLARE(node_id)								\
	LINKER_DT_NODE_REGION_NAME_TOKEN(node_id) (NOLOAD) :					\
	{											\
		_DT_SECTION_START(node_id) = .;							\
		KEEP(*(LINKER_DT_NODE_REGION_NAME_TOKEN(node_id)))				\
		KEEP(*(LINKER_DT_NODE_REGION_NAME_TOKEN(node_id).*))				\
		_DT_SECTION_END(node_id) = .;							\
	} > LINKER_DT_NODE_REGION_NAME_TOKEN(node_id)						\
	_DT_SECTION_SIZE(node_id) = _DT_SECTION_END(node_id) - _DT_SECTION_START(node_id);	\
	_DT_SECTION_LOAD(node_id) = LOADADDR(LINKER_DT_NODE_REGION_NAME_TOKEN(node_id));
/* clang-format on */

/** @endcond */

/**
 * @brief Generate linker memory regions from the device tree nodes with
 *        compatible 'zephyr,memory-region'
 *
 * Note: for now we do not deal with MEMORY attributes since those are
 * optional, not actually used by Zephyr and they will likely conflict with the
 * MPU configuration.
 */
#define LINKER_DT_REGIONS() \
	DT_FOREACH_STATUS_OKAY(_DT_COMPATIBLE, _REGION_DECLARE)

/**
 * @brief Generate linker memory sections from the device tree nodes with
 *        compatible 'zephyr,memory-region'
 */
#define LINKER_DT_SECTIONS() \
	DT_FOREACH_STATUS_OKAY(_DT_COMPATIBLE, _SECTION_DECLARE)

#endif /* ZEPHYR_INCLUDE_LINKER_DEVICETREE_REGIONS_H_ */
