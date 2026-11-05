# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# All modes use the same registration header and records, in this order.
function(isr_tables_metadata keep)
  zephyr_linker_section(NAME .intList ${ARGN} NOINPUT)
  zephyr_linker_section_configure(SECTION .intList ${keep} INPUT ".irq_info" FIRST)
  zephyr_linker_section_configure(SECTION .intList ${keep} INPUT ".intList")
endfunction()

function(isr_tables_placeholder target)
  add_library(${target} ${ARGN} isr_tables.c)
  add_dependencies(${target} zephyr_generated_headers)
  target_link_libraries(${target} zephyr_interface)
endfunction()

function(isr_tables_local)
  set(metadata_placement ADDRESS 0 TYPE INFO NOSYMBOLS)
  set(metadata_keep KEEP)
  set(entry_type BUILD_ONLY)
  set(refs_discard_pass "")
  if(LINKER STREQUAL "iar")
    # __no_alloc records remain in the ELF without runtime storage or KEEP.
    set(metadata_keep "")
  elseif(LINKER STREQUAL "armlink")
    # The native post-link finalizer detaches the isolated metadata overlay.
    zephyr_compile_definitions(Z_ISR_LOCAL_METADATA_NONALLOC=1)
  else()
    set(entry_type INFO)
    list(APPEND metadata_placement PASS NOT LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL)
    set(refs_discard_pass PASS LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL)
  endif()

  isr_tables_metadata("${metadata_keep}" ${metadata_placement})
  if(entry_type STREQUAL INFO)
    zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".irq_info" ".intList"
      PASS LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
    )
  endif()
  zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".isr_vector_owners")
  zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".isr_vector_refs"
    ${refs_discard_pass}
  )
  zephyr_linker_section(NAME isr_declarations ADDRESS 0 TYPE ${entry_type} NOINPUT NOSYMBOLS
    PASS NOT LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
  )
  if(entry_type STREQUAL INFO)
    zephyr_linker_section_configure(SECTION isr_declarations INPUT ".isr_vector_refs" KEEP)
  endif()

  # GNU-style CODE discovery keeps typed roots, not range-limited instructions.
  set(collect_vector_entries TRUE)
  if(entry_type STREQUAL INFO AND CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE)
    set(collect_vector_entries FALSE)
    zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".isr.*"
      PASS NOT LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
    )
  endif()
  if(collect_vector_entries)
    zephyr_linker_section(NAME .isr_pre0.entries ADDRESS 0 TYPE ${entry_type} NOINPUT NOSYMBOLS
      PASS NOT LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
    )
  endif()
  zephyr_linker_section_configure(SECTION isr_declarations INPUT ".irq.*" KEEP)
  if(collect_vector_entries)
    zephyr_linker_section_configure(SECTION .isr_pre0.entries INPUT ".isr.*" KEEP)
  endif()
  isr_tables_placeholder(isr_tables_discovery OBJECT)
endfunction()

function(isr_tables_legacy)
  if(ZEPHYR_TOOLCHAIN_VARIANT STREQUAL "iar")
    # Legacy IAR cannot discard these records; keep its existing RODATA placement.
    isr_tables_metadata(KEEP GROUP RODATA_REGION)
  else()
    isr_tables_metadata(KEEP VMA IDT_LIST LMA IDT_LIST PASS NOT LINKER_ZEPHYR_FINAL)
    foreach(input .irq_info .intList)
      zephyr_linker_section_configure(SECTION /DISCARD/ KEEP INPUT ${input}
        PASS LINKER_ZEPHYR_FINAL
      )
    endforeach()
  endif()
  # This normal library must not be whole-archived, unlike the local pre0 object.
  isr_tables_placeholder(isr_tables)
  zephyr_library_link_libraries(isr_tables)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,undefined>_sw_isr_table)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,undefined>_irq_vector_table)
endfunction()

if(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
  isr_tables_local()
else()
  isr_tables_legacy()
endif()
