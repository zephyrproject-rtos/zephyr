# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

# LOCAL generates once from discovery; legacy generates from the prebuilt image.
function(zephyr_isr_tables_generate prebuilt_target)
  set(ISR_TABLES_DISCOVERY_SOURCES "" PARENT_SCOPE)
  set(ISR_TABLES_GENERATED_SOURCES "" PARENT_SCOPE)
  set(ISR_TABLES_LINKER_DEPENDENCIES "" PARENT_SCOPE)
  if(NOT CONFIG_GEN_ISR_TABLES)
    return()
  endif()

  set(outputs
    "${PROJECT_BINARY_DIR}/isr_tables.c"
    "${PROJECT_BINARY_DIR}/isr_tables_vt.ld"
    "${PROJECT_BINARY_DIR}/isr_tables_swi.ld"
  )
  set(generator_args "${GEN_ISR_TABLE_EXTRA_ARG}")
  if(CONFIG_GEN_SW_ISR_TABLE)
    list(APPEND generator_args --sw-isr-table)
  endif()
  if(CONFIG_GEN_IRQ_VECTOR_TABLE)
    list(APPEND generator_args --vector-table)
  endif()

  set(generator "${ZEPHYR_BASE}/scripts/build/gen_isr_tables.py")
  if(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
    set(kernel_target zephyr_pre0)
    set(parser "${ZEPHYR_BASE}/scripts/build/gen_isr_tables_parser_local.py")
    set(layout "${ZEPHYR_BASE}/scripts/build/isr_table_layout.py")
    list(APPEND outputs
      "${PROJECT_BINARY_DIR}/isr_tables_shared.ld"
      "${PROJECT_BINARY_DIR}/isr_tables_entries.ld"
      "${PROJECT_BINARY_DIR}/isr_tables_layout.cmake"
      "${PROJECT_BINARY_DIR}/isr_tables_layout.json"
    )
    list(APPEND generator_args
      --shared-linker-output "${PROJECT_BINARY_DIR}/isr_tables_shared.ld"
      --entries-linker-output "${PROJECT_BINARY_DIR}/isr_tables_entries.ld"
      --cmake-output "${PROJECT_BINARY_DIR}/isr_tables_layout.cmake"
      --layout-output "${PROJECT_BINARY_DIR}/isr_tables_layout.json"
    )
  else()
    set(kernel_target ${prebuilt_target})
    set(parser "${ZEPHYR_BASE}/scripts/build/gen_isr_tables_parser_carrays.py")
    set(layout "")
    list(APPEND generator_args
      $<$<BOOL:${CONFIG_BIG_ENDIAN}>:--big-endian>
      $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--debug>
    )
  endif()

  add_custom_command(
    OUTPUT ${outputs}
    COMMAND "${PYTHON_EXECUTABLE}" "${generator}"
      --kernel $<TARGET_FILE:${kernel_target}>
      --output-source "${PROJECT_BINARY_DIR}/isr_tables.c"
      --linker-output-files
        "${PROJECT_BINARY_DIR}/isr_tables_vt.ld"
        "${PROJECT_BINARY_DIR}/isr_tables_swi.ld"
      --intlist-section .intList --intlist-section intList ${generator_args}
    DEPENDS ${kernel_target} "${generator}" "${parser}" ${layout}
    COMMAND_EXPAND_LISTS
    VERBATIM
  )

  if(NOT CONFIG_ISR_TABLES_LOCAL_DECLARATION)
    set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)
    return()
  endif()

  # Compile once so both fixed-layout links use the same object.
  add_custom_target(isr_tables_generated_files DEPENDS ${outputs})
  add_library(isr_tables_generated OBJECT "${PROJECT_BINARY_DIR}/isr_tables.c")
  target_link_libraries(isr_tables_generated zephyr_interface)
  add_dependencies(isr_tables_generated isr_tables_generated_files zephyr_generated_headers)
  set(ISR_TABLES_LINKER_DEPENDENCIES isr_tables_generated_files ${outputs} PARENT_SCOPE)
  set(ISR_TABLES_DISCOVERY_SOURCES $<TARGET_OBJECTS:isr_tables_discovery> PARENT_SCOPE)
  set(ISR_TABLES_GENERATED_SOURCES $<TARGET_OBJECTS:isr_tables_generated> PARENT_SCOPE)
  if(CONFIG_CMAKE_LINKER_GENERATOR)
    zephyr_linker_include_generated(
      CMAKE "${PROJECT_BINARY_DIR}/isr_tables_layout.cmake"
      PASS LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
    )
  endif()
endfunction()

# Validate resolved entries after the final link, including pre1 layout stability.
function(zephyr_isr_tables_check target)
  if(NOT CONFIG_ISR_TABLES_LOCAL_DECLARATION)
    return()
  endif()

  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
    "${ZEPHYR_BASE}/scripts/build/check_isr_tables.py"
  )
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "${PYTHON_EXECUTABLE}" "${ZEPHYR_BASE}/scripts/build/check_isr_tables.py"
      --elf $<TARGET_FILE:${target}>
      --manifest "${PROJECT_BINARY_DIR}/isr_tables_layout.json"
      --prelinked $<TARGET_FILE:zephyr_pre1>
      --report "${PROJECT_BINARY_DIR}/isr_tables_check.json"
    VERBATIM
  )
endfunction()
