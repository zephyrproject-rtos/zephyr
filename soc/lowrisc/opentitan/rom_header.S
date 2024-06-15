/*
 * Copyright (c) 2023 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__rom_header)

/* OpenTitan manifest consists of 1024 bytes (256 words) of manifest
 * containing signature, device ID, version info, etc. The test ROM
 * ignores all of these fields except for entry point.
 *
 * The layout below conforms to manifest version: 0x71c3 (major),
 * 0x6c47 (minor).  The manifest format is documented here:
 * https://github.com/lowRISC/opentitan/blob/689a163294e1791bd30cfe096decf7f9233abad4/sw/host/opentitanlib/src/image/manifest.rs#L205
 */
SECTION_FUNC(rom_header, __rom_header)
	.rept(225)
	.word 0
	.endr
	/* Entry point is relative to the beginning of manifest. */
	.word(__start - __rom_header)
	/* Extensions entry table goes here. */
	.rept(30)
	.word 0
	.endr
