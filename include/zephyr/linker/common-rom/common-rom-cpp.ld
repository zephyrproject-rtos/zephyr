/* SPDX-License-Identifier: Apache-2.0 */

#ifdef CONFIG_CPP_STATIC_INIT_GNU
	SECTION_PROLOGUE(_CTOR_SECTION_NAME,,)
	{
		/*
		 * The compiler fills the constructor pointers table below,
		 * hence symbol __CTOR_LIST__ must be aligned on word
		 * boundary.  To align with the C++ standard, the first element
		 * of the array contains the number of actual constructors. The
		 * last element is NULL.
		 */
#ifdef CONFIG_64BIT
		. = ALIGN(8);
		__CTOR_LIST__ = .;
		QUAD((__CTOR_END__ - __CTOR_LIST__) / 8 - 2)
		KEEP(*(SORT_BY_NAME(".ctors*")))
		QUAD(0)
		__CTOR_END__ = .;
#else
		. = ALIGN(4);
		__CTOR_LIST__ = .;
		LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
		KEEP(*(SORT_BY_NAME(".ctors*")))
		LONG(0)
		__CTOR_END__ = .;
#endif
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(init_array,,)
	{
		. = ALIGN(4);
		__init_array_start = .;
		KEEP(*(SORT_BY_NAME(".init_array*")))
		__init_array_end = .;
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif
