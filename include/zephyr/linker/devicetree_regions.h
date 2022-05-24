/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions from devicetree nodes.
 */

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

/** @cond INTERNAL_HIDDEN */

#define _DT_COMPATIBLE	zephyr_memory_region

#define _DT_SECTION_PREFIX(node_id)	UTIL_CAT(__, LINKER_DT_NODE_REGION_NAME_TOKEN(node_id))
#define _DT_SECTION_START(node_id)	UTIL_CAT(_DT_SECTION_PREFIX(node_id), _start)
#define _DT_SECTION_END(node_id)	UTIL_CAT(_DT_SECTION_PREFIX(node_id), _end)
#define _DT_SECTION_SIZE(node_id)	UTIL_CAT(_DT_SECTION_PREFIX(node_id), _size)
#define _DT_SECTION_LOAD(node_id)	UTIL_CAT(_DT_SECTION_PREFIX(node_id), _load_start)

#define _DT_ATTR(token)			UTIL_CAT(UTIL_CAT(REGION_, token), _ATTR)

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
#define _REGION_DECLARE(node_id)			\
	LINKER_DT_NODE_REGION_NAME_TOKEN(node_id) :	\
	ORIGIN = DT_REG_ADDR(node_id),			\
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
 *    FOOBAR 0x20010000 (NOLOAD) :
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
#define _SECTION_DECLARE(node_id)								\
	LINKER_DT_NODE_REGION_NAME_TOKEN(node_id) DT_REG_ADDR(node_id) (NOLOAD) :		\
	{											\
		_DT_SECTION_START(node_id) = .;							\
		KEEP(*(LINKER_DT_NODE_REGION_NAME_TOKEN(node_id)))				\
		KEEP(*(LINKER_DT_NODE_REGION_NAME_TOKEN(node_id).*))				\
		_DT_SECTION_END(node_id) = .;							\
	} > LINKER_DT_NODE_REGION_NAME_TOKEN(node_id)						\
	_DT_SECTION_SIZE(node_id) = _DT_SECTION_END(node_id) - _DT_SECTION_START(node_id);	\
	_DT_SECTION_LOAD(node_id) = LOADADDR(LINKER_DT_NODE_REGION_NAME_TOKEN(node_id));

/**
 * Call the user-provided MPU_FN() macro passing the expected arguments
 */
#define _EXPAND_MPU_FN(node_id, MPU_FN, ...)					\
	MPU_FN(LINKER_DT_NODE_REGION_NAME(node_id),				\
	       DT_REG_ADDR(node_id),						\
	       DT_REG_SIZE(node_id),						\
	       _DT_ATTR(DT_STRING_TOKEN(node_id, zephyr_memory_region_mpu))),

/**
 * Check that the node_id has both properties:
 *  - zephyr,memory-region-mpu
 *  - zephyr,memory-region
 *
 * and call the EXPAND_MPU_FN() macro
 */
#define _CHECK_ATTR_FN(node_id, EXPAND_MPU_FN, ...)					\
	COND_CODE_1(UTIL_AND(DT_NODE_HAS_PROP(node_id, zephyr_memory_region_mpu),	\
			     DT_NODE_HAS_PROP(node_id, zephyr_memory_region)),		\
		   (EXPAND_MPU_FN(node_id, __VA_ARGS__)),				\
		   ())

/**
 * Call _CHECK_ATTR_FN() for each enabled node passing EXPAND_MPU_FN() as
 * explicit argument and the user-provided MPU_FN() macro in __VA_ARGS__
 */
#define _CHECK_APPLY_FN(compat, EXPAND_MPU_FN, ...)					\
	DT_FOREACH_STATUS_OKAY_VARGS(compat, _CHECK_ATTR_FN, EXPAND_MPU_FN, __VA_ARGS__)

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

/**
 * @brief Generate MPU regions from the device tree nodes with compatible
 *        'zephyr,memory-region' and 'zephyr,memory-region-mpu' attribute.
 *
 * Helper macro to apply an MPU_FN macro to all the memory regions declared
 * using the 'zephyr,memory-region-mpu' property and the 'zephyr,memory-region'
 * compatible.
 *
 * @p MPU_FN must take the form:
 *
 * @code{.c}
 *   #define MPU_FN(name, base, size, attr) ...
 * @endcode
 *
 * The 'name', 'base' and 'size' parameters are taken from the DT node.
 *
 * The 'zephyr,memory-region-mpu' enum property is passed as an extended token
 * to the MPU_FN macro using the 'attr' parameter, in the form
 * REGION_{attr}_ATTR.
 *
 * The following enums are supported for the 'zephyr,memory-region-mpu'
 * property:
 *
 *  - RAM
 *  - RAM_NOCACHE
 *  - FLASH
 *  - PPB
 *  - IO
 *
 * This means that usually the arch code would provide some macros or defines
 * with the same name of the extended property, that is:
 *
 *  - REGION_RAM_ATTR
 *  - REGION_RAM_NOCACHE_ATTR
 *  - REGION_FLASH_ATTR
 *  - REGION_PPB_ATTR
 *  - REGION_IO_ATTR
 *
 * Example devicetree fragment:
 *
 *     / {
 *             soc {
 *                     sram1: memory@2000000 {
 *                         zephyr,memory-region = "MY_NAME";
 *                         zephyr,memory-region-mpu = "RAM_NOCACHE";
 *                     };
 *             };
 *     };
 *
 * For detailed information about MPU region attribute define configuration refer
 * to the specific architecture MPU header.
 * For example: include/zephyr/arch/arm/aarch32/mpu/arm_mpu_v7m.h.
 *
 * The 'attr' parameter of the MPU_FN function will be the extended
 * 'REGION_RAM_NOCACHE_ATTR' token and the arch code will be usually
 * implementing a macro with the same name.
 *
 * Example:
 *
 * @code{.c}
 *
 *   #define REGION_RAM_NOCACHE_ATTR 0xAAAA
 *   #define REGION_RAM_ATTR         0xBBBB
 *   #define REGION_FLASH_ATTR       0xCCCC
 *
 *   #define MPU_FN(p_name, p_base, p_size, p_attr) \
 *       {                                          \
 *           .name = p_name,                        \
 *           .base = p_base,                        \
 *           .size = p_size,                        \
 *           .attr = p_attr,                        \
 *       }
 *
 *   static const struct arm_mpu_region mpu_regions[] = {
 *       ...
 *       LINKER_DT_REGION_MPU(MPU_FN)
 *       ...
 *   };
 * @endcode
 *
 */
#define LINKER_DT_REGION_MPU(mpu_fn) _CHECK_APPLY_FN(_DT_COMPATIBLE, _EXPAND_MPU_FN, mpu_fn)
