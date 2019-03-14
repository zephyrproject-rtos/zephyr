/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_USERSPACE
	/* We need to reserve room for the gperf generated hash functions.
	 * Fortunately, unlike the data tables, the size of the code is
	 * reasonably predictable.
	 */
	_priv_stacks_text_area_start = .;
	*(".priv_stacks.text*")
	_priv_stacks_text_area_end = .;

	_priv_stacks_text_area_used = _priv_stacks_text_area_end - _priv_stacks_text_area_start;

#ifndef LINKER_PASS2
	PROVIDE(z_priv_stack_find = .);
#endif

	/* In a valid build the MAX function will always evaluate to the
	second argument below, but to give the user a good error message
	when the area overflows we need to temporarily corrupt the
	location counter, and then detect the overflow with an assertion
	later on. */

	. = MAX(., _priv_stacks_text_area_start + CONFIG_PRIVILEGED_STACK_TEXT_AREA);

	ASSERT(
		CONFIG_PRIVILEGED_STACK_TEXT_AREA >= _priv_stacks_text_area_used,
"The configuration system has incorrectly set
'CONFIG_PRIVILEGED_STACK_TEXT_AREA' to
CONFIG_PRIVILEGED_STACK_TEXT_AREA, which is not big enough. You must
through Kconfig either disable 'CONFIG_USERSPACE', or set
'CONFIG_PRIVILEGED_STACK_TEXT_AREA' to a value larger than
CONFIG_PRIVILEGED_STACK_TEXT_AREA."
		);
#endif /* CONFIG_USERSPACE */
