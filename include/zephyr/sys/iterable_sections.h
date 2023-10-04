/*
 * Copyright (C) 2020, Intel Corporation
 * Copyright (C) 2023, Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_SYS_ITERABLE_SECTIONS_H_
#define INCLUDE_ZEPHYR_SYS_ITERABLE_SECTIONS_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Iterable Sections APIs
 * @defgroup iterable_section_apis Iterable Sections APIs
 * @ingroup os_services
 * @{
 */

/**
 * @brief Defines a new element for an iterable section for a generic type.
 *
 * @details
 * Convenience helper combining __in_section() and Z_DECL_ALIGN().
 * The section name will be '.[SECNAME].static.[SECTION_POSTFIX]'
 *
 * In the linker script, create output sections for these using
 * ITERABLE_SECTION_ROM() or ITERABLE_SECTION_RAM().
 *
 * @note In order to store the element in ROM, a const specifier has to
 * be added to the declaration: const TYPE_SECTION_ITERABLE(...);
 *
 * @param[in]  type data type of variable
 * @param[in]  varname name of variable to place in section
 * @param[in]  secname type name of iterable section.
 * @param[in]  section_postfix postfix to use in section name
 */
#define TYPE_SECTION_ITERABLE(type, varname, secname, section_postfix) \
	Z_DECL_ALIGN(type) varname \
	__in_section(_##secname, static, _CONCAT(section_postfix, _)) __used __noasan

/**
 * @brief iterable section start symbol for a generic type
 *
 * will return '_[OUT_TYPE]_list_start'.
 *
 * @param[in]  secname type name of iterable section.  For 'struct foobar' this
 * would be TYPE_SECTION_START(foobar)
 *
 */
#define TYPE_SECTION_START(secname) _CONCAT(_##secname, _list_start)

/**
 * @brief iterable section end symbol for a generic type
 *
 * will return '_<SECNAME>_list_end'.
 *
 * @param[in]  secname type name of iterable section.  For 'struct foobar' this
 * would be TYPE_SECTION_START(foobar)
 */
#define TYPE_SECTION_END(secname) _CONCAT(_##secname, _list_end)

/**
 * @brief iterable section extern for start symbol for a generic type
 *
 * Helper macro to give extern for start of iterable section.  The macro
 * typically will be called TYPE_SECTION_START_EXTERN(struct foobar, foobar).
 * This allows the macro to hand different types as well as cases where the
 * type and section name may differ.
 *
 * @param[in]  type data type of section
 * @param[in]  secname name of output section
 */
#define TYPE_SECTION_START_EXTERN(type, secname) \
	extern type TYPE_SECTION_START(secname)[]

/**
 * @brief iterable section extern for end symbol for a generic type
 *
 * Helper macro to give extern for end of iterable section.  The macro
 * typically will be called TYPE_SECTION_END_EXTERN(struct foobar, foobar).
 * This allows the macro to hand different types as well as cases where the
 * type and section name may differ.
 *
 * @param[in]  type data type of section
 * @param[in]  secname name of output section
 */
#define TYPE_SECTION_END_EXTERN(type, secname) \
	extern type TYPE_SECTION_END(secname)[]

/**
 * @brief Iterate over a specified iterable section for a generic type
 *
 * @details
 * Iterator for structure instances gathered by TYPE_SECTION_ITERABLE().
 * The linker must provide a _<SECNAME>_list_start symbol and a
 * _<SECNAME>_list_end symbol to mark the start and the end of the
 * list of struct objects to iterate over. This is normally done using
 * ITERABLE_SECTION_ROM() or ITERABLE_SECTION_RAM() in the linker script.
 */
#define TYPE_SECTION_FOREACH(type, secname, iterator)		\
	TYPE_SECTION_START_EXTERN(type, secname);		\
	TYPE_SECTION_END_EXTERN(type, secname);		\
	for (type * iterator = TYPE_SECTION_START(secname); ({	\
		__ASSERT(iterator <= TYPE_SECTION_END(secname),\
			      "unexpected list end location");	\
		     iterator < TYPE_SECTION_END(secname);	\
	     });						\
	     iterator++)

/**
 * @brief Get element from section for a generic type.
 *
 * @note There is no protection against reading beyond the section.
 *
 * @param[in]  type type of element
 * @param[in]  secname name of output section
 * @param[in]  i Index.
 * @param[out] dst Pointer to location where pointer to element is written.
 */
#define TYPE_SECTION_GET(type, secname, i, dst) do { \
	TYPE_SECTION_START_EXTERN(type, secname); \
	*(dst) = &TYPE_SECTION_START(secname)[i]; \
} while (0)

/**
 * @brief Count elements in a section for a generic type.
 *
 * @param[in]  type type of element
 * @param[in]  secname name of output section
 * @param[out] dst Pointer to location where result is written.
 */
#define TYPE_SECTION_COUNT(type, secname, dst) do { \
	TYPE_SECTION_START_EXTERN(type, secname); \
	TYPE_SECTION_END_EXTERN(type, secname); \
	*(dst) = ((uintptr_t)TYPE_SECTION_END(secname) - \
		  (uintptr_t)TYPE_SECTION_START(secname)) / sizeof(type); \
} while (0)

/**
 * @brief iterable section start symbol for a struct type
 *
 * @param[in]  struct_type data type of section
 */
#define STRUCT_SECTION_START(struct_type) \
	TYPE_SECTION_START(struct_type)

/**
 * @brief iterable section extern for start symbol for a struct
 *
 * Helper macro to give extern for start of iterable section.
 *
 * @param[in]  struct_type data type of section
 */
#define STRUCT_SECTION_START_EXTERN(struct_type) \
	TYPE_SECTION_START_EXTERN(struct struct_type, struct_type)

/**
 * @brief iterable section end symbol for a struct type
 *
 * @param[in]  struct_type data type of section
 */
#define STRUCT_SECTION_END(struct_type) \
	TYPE_SECTION_END(struct_type)

/**
 * @brief iterable section extern for end symbol for a struct
 *
 * Helper macro to give extern for end of iterable section.
 *
 * @param[in]  struct_type data type of section
 */
#define STRUCT_SECTION_END_EXTERN(struct_type) \
	TYPE_SECTION_END_EXTERN(struct struct_type, struct_type)

/**
 * @brief Defines a new element of alternate data type for an iterable section.
 *
 * @details
 * Special variant of STRUCT_SECTION_ITERABLE(), for placing alternate
 * data types within the iterable section of a specific data type. The
 * data type sizes and semantics must be equivalent!
 */
#define STRUCT_SECTION_ITERABLE_ALTERNATE(secname, struct_type, varname) \
	TYPE_SECTION_ITERABLE(struct struct_type, varname, secname, varname)

/**
 * @brief Defines an array of elements of alternate data type for an iterable
 * section.
 *
 * @see STRUCT_SECTION_ITERABLE_ALTERNATE
 */
#define STRUCT_SECTION_ITERABLE_ARRAY_ALTERNATE(secname, struct_type, varname, \
						size)                          \
	TYPE_SECTION_ITERABLE(struct struct_type, varname[size], secname,      \
			      varname)

/**
 * @brief Defines a new element for an iterable section.
 *
 * @details
 * Convenience helper combining __in_section() and Z_DECL_ALIGN().
 * The section name is the struct type prepended with an underscore.
 * The subsection is "static" and the subsubsection is the variable name.
 *
 * In the linker script, create output sections for these using
 * ITERABLE_SECTION_ROM() or ITERABLE_SECTION_RAM().
 *
 * @note In order to store the element in ROM, a const specifier has to
 * be added to the declaration: const STRUCT_SECTION_ITERABLE(...);
 */
#define STRUCT_SECTION_ITERABLE(struct_type, varname) \
	STRUCT_SECTION_ITERABLE_ALTERNATE(struct_type, struct_type, varname)

/**
 * @brief Defines an array of elements for an iterable section.
 *
 * @see STRUCT_SECTION_ITERABLE
 */
#define STRUCT_SECTION_ITERABLE_ARRAY(struct_type, varname, size)              \
	STRUCT_SECTION_ITERABLE_ARRAY_ALTERNATE(struct_type, struct_type,      \
						varname, size)

/**
 * @brief Defines a new element for an iterable section with a custom name.
 *
 * The name can be used to customize how iterable section entries are sorted.
 * @see STRUCT_SECTION_ITERABLE()
 */
#define STRUCT_SECTION_ITERABLE_NAMED(struct_type, name, varname) \
	TYPE_SECTION_ITERABLE(struct struct_type, varname, struct_type, name)

/**
 * @brief Defines a new element for an iterable section with a custom name,
 * placed in a custom section.
 *
 * The name can be used to customize how iterable section entries are sorted.
 * @see STRUCT_SECTION_ITERABLE_NAMED()
 */
#define STRUCT_SECTION_ITERABLE_NAMED_ALTERNATE(struct_type, secname, name, varname) \
	TYPE_SECTION_ITERABLE(struct struct_type, varname, secname, name)

/**
 * @brief Iterate over a specified iterable section (alternate).
 *
 * @details
 * Iterator for structure instances gathered by STRUCT_SECTION_ITERABLE().
 * The linker must provide a _<SECNAME>_list_start symbol and a
 * _<SECNAME>_list_end symbol to mark the start and the end of the
 * list of struct objects to iterate over. This is normally done using
 * ITERABLE_SECTION_ROM() or ITERABLE_SECTION_RAM() in the linker script.
 */
#define STRUCT_SECTION_FOREACH_ALTERNATE(secname, struct_type, iterator) \
	TYPE_SECTION_FOREACH(struct struct_type, secname, iterator)

/**
 * @brief Iterate over a specified iterable section.
 *
 * @details
 * Iterator for structure instances gathered by STRUCT_SECTION_ITERABLE().
 * The linker must provide a _<struct_type>_list_start symbol and a
 * _<struct_type>_list_end symbol to mark the start and the end of the
 * list of struct objects to iterate over. This is normally done using
 * ITERABLE_SECTION_ROM() or ITERABLE_SECTION_RAM() in the linker script.
 */
#define STRUCT_SECTION_FOREACH(struct_type, iterator) \
	STRUCT_SECTION_FOREACH_ALTERNATE(struct_type, struct_type, iterator)

/**
 * @brief Get element from section.
 *
 * @note There is no protection against reading beyond the section.
 *
 * @param[in]  struct_type Struct type.
 * @param[in]  i Index.
 * @param[out] dst Pointer to location where pointer to element is written.
 */
#define STRUCT_SECTION_GET(struct_type, i, dst) \
	TYPE_SECTION_GET(struct struct_type, struct_type, i, dst)

/**
 * @brief Count elements in a section.
 *
 * @param[in]  struct_type Struct type
 * @param[out] dst Pointer to location where result is written.
 */
#define STRUCT_SECTION_COUNT(struct_type, dst) \
	TYPE_SECTION_COUNT(struct struct_type, struct_type, dst);

/**
 * @}
 */ /* end of struct_section_apis */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZEPHYR_SYS_ITERABLE_SECTIONS_H_ */
