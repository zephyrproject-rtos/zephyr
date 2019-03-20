/*
 * SPDX-License-Identifier: Apache-2.0
 * Extracted from:
 * ext/hal/cypress/.../devices/psoc6/linker/gcc/cy8c6xx6_cm0plus.ld
 *
 * Size of sections are calculated in the startup scripts,
 * so they don't have to be specified here.
 */

. = ALIGN(8);
KEEP(*(.ram_vectors))

. = ALIGN(4);
KEEP(*(.heap))

. = ALIGN(4);
KEEP(*(.stack))
