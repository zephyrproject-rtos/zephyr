/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ brief architecture specific support for the mem_probe tests
 */

#include <arch/x86/asm.h>

	GDATA(foo)
	GTEXT(add_ten_to_foo)

SECTION_FUNC(TEXT, add_ten_to_foo)
	addl $10, foo
	ret
