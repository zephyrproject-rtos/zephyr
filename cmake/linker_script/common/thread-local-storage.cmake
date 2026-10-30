# The contents of this file is based on include/zephyr/linker/thread-local-storage.ld
# Please keep in sync

if(CONFIG_THREAD_LOCAL_STORAGE)
  if(CONFIG_STACK_CANARIES_TLS_PREPEND)
    # Stack canary section placed before tdata so the canary is at a
    # fixed offset (0) from the TLS base pointer.
    zephyr_linker_section(NAME .stack_chk LMA FLASH NOINPUT)
    zephyr_linker_section_configure(SECTION .stack_chk INPUT ".stack_chk.guard" KEEP)
  endif()

  zephyr_linker_section(NAME .tdata LMA FLASH NOINPUT)
  zephyr_linker_section_configure(SECTION .tdata INPUT ".tdata")
  zephyr_linker_section_configure(SECTION .tdata INPUT ".tdata.*")
  zephyr_linker_section_configure(SECTION .tdata INPUT ".gnu.linkonce.td.*")
  # GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

  zephyr_linker_section(NAME .tbss LMA FLASH NOINPUT)
  zephyr_linker_section_configure(SECTION .tbss INPUT ".tbss")
  zephyr_linker_section_configure(SECTION .tbss INPUT ".tbss.*")
  zephyr_linker_section_configure(SECTION .tbss INPUT ".gnu.linkonce.tb.*")
  zephyr_linker_section_configure(SECTION .tbss INPUT ".tcommon")
  # GROUP_ROM_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

  # The generator emits per-section start, end, size and alignment symbols.
  # Non-IAR TLS initialization also needs the size and alignment of the full
  # ELF TLS image, including padding between its sections.
  if(NOT CMAKE_C_COMPILER_ID STREQUAL "IAR")
    if(CONFIG_STACK_CANARIES_TLS_PREPEND)
      zephyr_linker_symbol(SYMBOL __tdata_offset
        EXPR "(@__tdata_start@ - @__stack_chk_start@)")
      zephyr_linker_symbol(SYMBOL __tls_size
        EXPR "(@__tbss_end@ - @__stack_chk_start@)")
      zephyr_linker_symbol(SYMBOL __tls_align
        EXPR "MAX(@__stack_chk_align@, MAX(@__tdata_align@, @__tbss_align@))")
    else()
      zephyr_linker_symbol(SYMBOL __tls_size EXPR "(@__tbss_end@ - @__tdata_start@)")
      zephyr_linker_symbol(SYMBOL __tls_align EXPR "MAX(@__tdata_align@, @__tbss_align@)")
    endif()
  endif()
endif()
