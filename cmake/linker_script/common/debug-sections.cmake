# Content of this file originates from include/linker/debug-sections.ld
# Following sections are obtained via 'ld --verbose'

# Stabs debugging sections.
zephyr_linker_section(NAME  .stab ADDRESS 0)
zephyr_linker_section(NAME  .stabstr ADDRESS 0)
zephyr_linker_section(NAME  .stab.excl ADDRESS 0)
zephyr_linker_section(NAME  .stab.exclstr ADDRESS 0)
zephyr_linker_section(NAME  .stab.index ADDRESS 0)
zephyr_linker_section(NAME  .stab.indexstr ADDRESS 0)
zephyr_linker_section(NAME  .gnu.build.attributes ADDRESS 0)
zephyr_linker_section(NAME  .comment ADDRESS 0)

# DWARF debug sections.
# Symbols in the DWARF debugging sections are relative to the beginning
# of the section so we begin them at 0.
# DWARF 1 */
zephyr_linker_section(NAME  .debug ADDRESS 0)
zephyr_linker_section(NAME  .line ADDRESS 0)

# GNU DWARF 1 extensions
zephyr_linker_section(NAME  .debug_srcinfo ADDRESS 0)
zephyr_linker_section(NAME  .debug_sfnames ADDRESS 0)

# DWARF 1.1 and DWARF 2
zephyr_linker_section(NAME  .debug_aranges ADDRESS 0)
zephyr_linker_section(NAME  .debug_pubnames ADDRESS 0)

# DWARF 2
zephyr_linker_section(NAME  .debug_info ADDRESS 0)
zephyr_linker_section_configure(SECTION .debug_info INPUT ".gnu.linkonce.wi.*")
zephyr_linker_section(NAME  .debug_abbrev ADDRESS 0)
zephyr_linker_section(NAME  .debug_line ADDRESS 0)
zephyr_linker_section_configure(SECTION .debug_line INPUT ".debug_line_end")
zephyr_linker_section(NAME  .debug_frame ADDRESS 0)
zephyr_linker_section(NAME  .debug_str ADDRESS 0)
zephyr_linker_section(NAME  .debug_loc ADDRESS 0)
zephyr_linker_section(NAME  .debug_macinfo ADDRESS 0)

 #SGI/MIPS DWARF 2 extensions
zephyr_linker_section(NAME  .debug_weaknames ADDRESS 0)
zephyr_linker_section(NAME  .debug_funcnames ADDRESS 0)
zephyr_linker_section(NAME  .debug_typenames ADDRESS 0)
zephyr_linker_section(NAME  .debug_varnames ADDRESS 0)

# DWARF 3
zephyr_linker_section(NAME  .debug_pubtypes ADDRESS 0)
zephyr_linker_section(NAME  .debug_ranges ADDRESS 0)
# DWARF Extension.
zephyr_linker_section(NAME  .debug_macro ADDRESS 0)
