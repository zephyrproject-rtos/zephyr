zephyr_linker_section(NAME .last_ram_section VMA RAM LMA RAM_REGION TYPE BSS)
zephyr_linker_section_configure(
  SECTION .last_ram_section
  INPUT ""
  SYMBOLS _end z_mapped_end
  KEEP
  )
