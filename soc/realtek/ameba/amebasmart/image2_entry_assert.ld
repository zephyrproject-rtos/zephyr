/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The vendor bootloader finds Img2EntryFun0 only at the first byte of the
 * loaded image. Injected at SECTIONS scope, where a bare ASSERT is legal.
 */

ASSERT(__image2_entry_func__ == ORIGIN(RAM),
       "Img2EntryFun0 must sit at the start of RAM (image2 load address)")
