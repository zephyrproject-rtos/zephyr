/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_ARM_ZIMAGE_HEADER)
KEEP(*(.image_header))
KEEP(*(.".image_header.*"))
/*
 * The zImage header at offset 0x2c must hold the end-of-image LMA so that
 * (end - start) equals the file size of the produced binary, matching the
 * Linux ARM zImage convention.  '.' here is only just past the 48-byte
 * header (rom_start is one of the first sections emitted), so use the
 * .last_section LMA which the linker resolves lazily at final link time.
 */
__end = LOADADDR(.last_section) + SIZEOF(.last_section);
#endif
