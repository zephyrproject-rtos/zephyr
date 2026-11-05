# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

# ROM and RAM callers supply placement and the legacy-only section attributes.
function(zephyr_linker_sw_isr_tables)
  cmake_parse_arguments(TABLE "" "" "PLACEMENT;LEGACY_ATTRIBUTES" ${ARGN})
  if(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
    if(CONFIG_GEN_SW_ISR_TABLE_ARRAY)
      zephyr_linker_section(NAME sw_isr_table ${TABLE_PLACEMENT} NOINPUT
        ALIGN ${CONFIG_ARCH_SW_ISR_TABLE_ALIGN}
      )
      zephyr_linker_section_configure(SECTION sw_isr_table
        INPUT ".isr_pre0.sw" KEEP
        PASS NOT LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
      )
    else()
      zephyr_linker_section(NAME isr_entries ${TABLE_PLACEMENT} NOINPUT NOSYMBOLS
        ALIGN ${CONFIG_ARCH_SW_ISR_TABLE_ALIGN}
      )
    endif()
    if(CONFIG_SHARED_INTERRUPTS)
      zephyr_linker_section(NAME shared_sw_isr_table ${TABLE_PLACEMENT} NOINPUT)
      zephyr_linker_section_configure(SECTION shared_sw_isr_table
        INPUT ".isr_pre0.shared" KEEP
        PASS NOT LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
      )
    endif()
  else()
    zephyr_linker_section(NAME sw_isr_table ${TABLE_PLACEMENT} ${TABLE_LEGACY_ATTRIBUTES})
    zephyr_linker_section_configure(SECTION sw_isr_table
      INPUT ".gnu.linkonce.sw_isr_table*"
    )
    if(CONFIG_SHARED_INTERRUPTS)
      zephyr_linker_section_configure(SECTION sw_isr_table
        INPUT ".gnu.linkonce.shared_sw_isr_table*"
      )
    endif()
  endif()
endfunction()
