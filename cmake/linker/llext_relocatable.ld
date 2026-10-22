/*
 * Grouping linker script for the LLEXT partial link (-r / --relocatable) step.
 *
 * The LLEXT loader computes one contiguous file-offset region per llext_mem
 * type (TEXT, RODATA, DATA, BSS ...) by spanning the min->max offsets of all
 * sections of that type.  When lld is used without an explicit script, it
 * places sections in encounter order without grouping by type, which can put
 * a .data section between two .text sections in the file.  The resulting
 * overlapping computed regions cause the loader to reject the ELF.
 *
 * This script groups all sections of each type together so the loader's
 * region spans are contiguous and non-overlapping.  Sections not listed here
 * (RELA tables, debug info, attributes, etc.) are placed as orphans after
 * the grouped sections; the LLEXT loader ignores them.
 *
 * The loader classifies each input section into a region by its ELF flags
 * (executable -> TEXT, writable -> DATA, otherwise -> RODATA), not by name.
 * Custom-named sections created via Z_GENERIC_SECTION() therefore have to be
 * placed here next to the other sections of matching flags, otherwise the
 * linker emits them as orphans whose file offsets fall outside the region span
 * the loader computes, making a region appear to overlap another.
 *
 * Custom sections that the LLEXT tests inspect by name (e.g. .my_rodata, or
 * .detach looked up via llext_get_section_header()) are kept as their own
 * output sections rather than merged into the standard one, so the section
 * identity survives in the linked object; they are still placed adjacent to
 * the matching type group to keep the region file-offset spans contiguous.
 */

SECTIONS
{
    .text          : { *(.text .text.* .TEXT .TEXT.* .literal .literal.*) }
    .detach        : { *(.detach) }
    .rodata        : { *(.rodata .rodata.* .rodata.str*.* *_sect_*) }
    .my_rodata     : { *(.my_rodata) }
    .llext.rodata.noreloc : { *(.llext.rodata.noreloc .llext.rodata.noreloc.*) }
    .data.rel.ro   : { *(.data.rel.ro .data.rel.ro.*) }
    .data          : { *(.data .data.* .sdata .sdata.*) }
    .bss           : { *(.bss .bss.* .sbss .sbss.* COMMON) }
    .preinit_array : { *(.preinit_array .preinit_array.*) }
    .init_array    : { *(.init_array .init_array.*) }
    .fini_array    : { *(.fini_array .fini_array.*) }
    .exported_sym  : { *(.exported_sym) }
}
