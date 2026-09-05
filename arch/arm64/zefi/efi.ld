/*
 * Copyright (c) 2020 Intel Corp.
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

ENTRY(efi_entry)

SECTIONS {

    /* Keep the link base low: AArch64 UEFI derives SizeOfImage from section
     * VMAs and will refuse to load multi-megabyte images on small VMs.
     */
    . = 0x1000;
    .text : {
        *(.text)
        *(.rodata)
    }

    .reloc :  { *(.reloc) }

    . = ALIGN(4096);
    .data : {
        *(.data)
        *(COMMON)
        *(.bss)
        *(.runtime_data_end)
    }

    .trash_bin : {
        *(.comment)
        *(.data.rel*)
        *(.dynamic)
        *(.dynbss)
        *(.dynstr)
        *(.dynsym)
        *(.eh_frame)
        *(.gnu.hash)
        *(.gnu.version*)
        *(.got)
        *(.got.plt)
        *(.hash)
        *(.note.*)
        *(.plt)
        *(.plt.*)
        *(.rela.*)
        *(.rel.local)
    }

}
