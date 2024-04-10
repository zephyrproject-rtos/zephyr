/*
 * Copyright (C) 2020, Intel Corporation
 * Copyright (C) 2023, Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_LINKER_ITERABLE_SECTIONS_H_
#define INCLUDE_ZEPHYR_LINKER_ITERABLE_SECTIONS_H_

/**
 * @addtogroup iterable_section_apis
 * @{
 */

#define Z_LINK_ITERABLE(struct_type) \
	_CONCAT(_##struct_type, _list_start) = .; \
	KEEP(*(SORT_BY_NAME(._##struct_type.static.*))); \
	_CONCAT(_##struct_type, _list_end) = .

#define Z_LINK_ITERABLE_NUMERIC(struct_type) \
	_CONCAT(_##struct_type, _list_start) = .; \
	KEEP(*(SORT(._##struct_type.static.*_?_*))); \
	KEEP(*(SORT(._##struct_type.static.*_??_*))); \
	_CONCAT(_##struct_type, _list_end) = .

#define Z_LINK_ITERABLE_ALIGNED(struct_type, align) \
	. = ALIGN(align); \
	Z_LINK_ITERABLE(struct_type);

#define Z_LINK_ITERABLE_GC_ALLOWED(struct_type) \
	_CONCAT(_##struct_type, _list_start) = .; \
	*(SORT_BY_NAME(._##struct_type.static.*)); \
	_CONCAT(_##struct_type, _list_end) = .

#ifdef CONFIG_64BIT
#define Z_LINK_ITERABLE_SUBALIGN  8
#else
#define Z_LINK_ITERABLE_SUBALIGN  4
#endif

/**
 * @brief Define a read-only iterable section output.
 *
 * @details
 * Define an output section which will set up an iterable area
 * of equally-sized data structures. For use with STRUCT_SECTION_ITERABLE().
 * Input sections will be sorted by name, per ld's SORT_BY_NAME.
 *
 * This macro should be used for read-only data.
 *
 * Note that this keeps the symbols in the image even though
 * they are not being directly referenced. Use this when symbols
 * are indirectly referenced by iterating through the section.
 */
#define ITERABLE_SECTION_ROM(struct_type, subalign) \
	SECTION_PROLOGUE(struct_type##_area,,SUBALIGN(subalign)) \
	{ \
		Z_LINK_ITERABLE(struct_type); \
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

/**
 * @brief Define a read-only iterable section output, sorted numerically.
 *
 * This version of ITERABLE_SECTION_ROM() sorts the entries numerically, that
 * is, `SECNAME_10` will come after `SECNAME_2`. `_` separator is required, and
 * up to 2 numeric digits are handled (0-99).
 *
 * @see ITERABLE_SECTION_ROM()
 */
#define ITERABLE_SECTION_ROM_NUMERIC(struct_type, subalign) \
	SECTION_PROLOGUE(struct_type##_area, EMPTY, SUBALIGN(subalign)) \
	{ \
		Z_LINK_ITERABLE_NUMERIC(struct_type); \
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

/**
 * @brief Define a garbage collectable read-only iterable section output.
 *
 * @details
 * Define an output section which will set up an iterable area
 * of equally-sized data structures. For use with STRUCT_SECTION_ITERABLE().
 * Input sections will be sorted by name, per ld's SORT_BY_NAME.
 *
 * This macro should be used for read-only data.
 *
 * Note that the symbols within the section can be garbage collected.
 */
#define ITERABLE_SECTION_ROM_GC_ALLOWED(struct_type, subalign) \
	SECTION_PROLOGUE(struct_type##_area,,SUBALIGN(subalign)) \
	{ \
		Z_LINK_ITERABLE_GC_ALLOWED(struct_type); \
	} GROUP_LINK_IN(ROMABLE_REGION)

/**
 * @brief Define a read-write iterable section output.
 *
 * @details
 * Define an output section which will set up an iterable area
 * of equally-sized data structures. For use with STRUCT_SECTION_ITERABLE().
 * Input sections will be sorted by name, per ld's SORT_BY_NAME.
 *
 * This macro should be used for read-write data that is modified at runtime.
 *
 * Note that this keeps the symbols in the image even though
 * they are not being directly referenced. Use this when symbols
 * are indirectly referenced by iterating through the section.
 */
#define ITERABLE_SECTION_RAM(struct_type, subalign) \
	SECTION_DATA_PROLOGUE(struct_type##_area,,SUBALIGN(subalign)) \
	{ \
		Z_LINK_ITERABLE(struct_type); \
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

/**
 * @brief Define a read-write iterable section output, sorted numerically.
 *
 * This version of ITERABLE_SECTION_RAM() sorts the entries numerically, that
 * is, `SECNAME10` will come after `SECNAME2`. Up to 2 numeric digits are
 * handled (0-99).
 *
 * @see ITERABLE_SECTION_RAM()
 */
#define ITERABLE_SECTION_RAM_NUMERIC(struct_type, subalign) \
	SECTION_PROLOGUE(struct_type##_area, EMPTY, SUBALIGN(subalign)) \
	{ \
		Z_LINK_ITERABLE_NUMERIC(struct_type); \
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

/**
 * @brief Define a garbage collectable read-write iterable section output.
 *
 * @details
 * Define an output section which will set up an iterable area
 * of equally-sized data structures. For use with STRUCT_SECTION_ITERABLE().
 * Input sections will be sorted by name, per ld's SORT_BY_NAME.
 *
 * This macro should be used for read-write data that is modified at runtime.
 *
 * Note that the symbols within the section can be garbage collected.
 */
#define ITERABLE_SECTION_RAM_GC_ALLOWED(struct_type, subalign) \
	SECTION_DATA_PROLOGUE(struct_type##_area,,SUBALIGN(subalign)) \
	{ \
		Z_LINK_ITERABLE_GC_ALLOWED(struct_type); \
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

/**
 * @}
 */ /* end of struct_section_apis */

#endif /* INCLUDE_ZEPHYR_LINKER_ITERABLE_SECTIONS_H_ */
