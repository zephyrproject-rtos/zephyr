# Keep in sync with include/zephyr/linker/sw-isr-table-switch-tail.ld, which
# the ld scripts include here for the same reason: the generated table must be
# the last thing in the region so the final link cannot shift anything.
if(CONFIG_GEN_SW_ISR_TABLE AND NOT CONFIG_SRAM_SW_ISR_TABLE
   AND CONFIG_GEN_SW_ISR_TABLE_SWITCH AND NOT CONFIG_XIP)
  zephyr_linker_section(NAME sw_isr_table GROUP DATA_REGION NOINPUT)
  zephyr_linker_section_configure(
    SECTION sw_isr_table
    INPUT ".gnu.linkonce.sw_isr_table*"
  )
endif()

zephyr_linker_section(NAME .last_ram_section VMA RAM LMA RAM_REGION TYPE BSS)
zephyr_linker_section_configure(
  SECTION .last_ram_section
  INPUT ""
  SYMBOLS _end z_mapped_end
  KEEP
  )
