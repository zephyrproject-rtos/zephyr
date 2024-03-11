/*
 * Copyright (c) 2023 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* exports */
GTEXT(__rom_header)

/* OpenTitan manifest consists of 964 bytes (226 words, plus 15 words
 * of manifest extenstion) containing signature, device ID, version
 * info, etc. The test ROM ignores all of these fields except for entry
 * point (final word in the base manifest).
 */
SECTION_FUNC(rom_header, __rom_header)
        .rept(225)
        .word 0
        .endr
        /* Entry point is relative to the beginning of manifest. */
        .word(__reset - __rom_header)
        /* TODO: extensions entry table goes here. */
