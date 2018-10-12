/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_USERSPACE
	/* We need to reserve room for the gperf generated hash functions.
	 * Fortunately, unlike the data tables, the size of the code is
	 * reasonably predictable.
	 *
	 * The linker will error out complaining that the location pointer
	 * is moving backwards if the reserved room isn't large enough.
	 */
	_priv_stacks_text_area_start = .;
	*(".priv_stacks.text*")
	_priv_stacks_text_area_end = .;
#ifndef LINKER_PASS2
	PROVIDE(_k_priv_stack_find = .);
#endif
	. += CONFIG_PRIVILEGED_STACK_TEXT_AREA - (_priv_stacks_text_area_end - _priv_stacks_text_area_start);
#endif /* CONFIG_USERSPACE */

