/*
 * Copyright (c) 2023 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* imports */
GTEXT(__start)

/* OpenTitan manifest consists of 896 bytes (224 words) containing signature,
 * device ID, version info, etc. The test ROM ignores all of these fields
 * except for entry point (final word in manifest).
 */
SECTION_FUNC(rom_header, __rom_header)
	.rept(223)
	.word 0
	.endr
	/* Entry point is relative to the beginning of manifest. */
	.word(__start - __rom_header)
