/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <mips/regdef.h>

/* Imports */
GTEXT(__initialize)

/* Exports */
GTEXT(__start)

SECTION_FUNC(vectors, __start)
	/* Call into Zephyr initialization. */
	la v0, __initialize
	jal v0
