ENTRY(efi_entry)

SECTIONS {

    /* Pick a reasonable base address, EFI won't load us there anyway */
    . = 0x4000000;
    .text : {
        *(.text)
        *(.rodata)
    }

    /* Must be a separately visible section to the EFI loader, doesn't
     * need to be page-aligned and can be immediately after text/rodata */
    .reloc :  { *(.reloc) }

    /* Must be page-aligned or EFI balks */
    . = ALIGN(4096);
    .data : {
        *(.data)
        *(COMMON)
        *(.bss)

        *(.runtime_data_end) /* Must be last.  Obviously. */
    }

    /* Because binutils ld really likes to "helpfully" ignore the section
     * directives and drop junk in weird places.
     */
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

} /* SECTIONS */
