/*
 * Copyright (c) 2022-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* OFS configuration registers start at address 0x00000400/0x2400 */
. = __rom_start_address + 0x400;
__ofs_regs = .;
    KEEP(*(.ofs_mem.iwdt))
    FILL(0xFF)
. = __ofs_regs + 2;
    KEEP(*(.ofs_mem.wdt))
    FILL(0xFF)
. = __ofs_regs + 4;
    KEEP(*(.ofs_mem.ofs1))
    FILL(0xFF)
. = __ofs_regs + 8;
    KEEP(*(.ofs_mem*))
    FILL(0xFF)
. = __rom_start_address + 0x43c;
