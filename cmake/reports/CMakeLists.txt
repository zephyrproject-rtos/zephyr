# SPDX-License-Identifier: Apache-2.0

set(flag_for_ram_report ram)
set(flag_for_rom_report rom)

foreach(report ram_report rom_report)
  add_custom_target(
    ${report}
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/footprint/size_report
    -k ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
    -z ${ZEPHYR_BASE}
    -o ${CMAKE_BINARY_DIR}
    --json ${CMAKE_BINARY_DIR}/${flag_for_${report}}.json
    ${flag_for_${report}}
    DEPENDS ${logical_target_for_zephyr_elf}
            $<TARGET_PROPERTY:zephyr_property_target,${report}_DEPENDENCIES>
    USES_TERMINAL
    )
endforeach()

find_program(PUNCOVER puncover)

if(NOT ${PUNCOVER} STREQUAL PUNCOVER-NOTFOUND)
  add_custom_target(
    puncover
    ${PUNCOVER}
    --elf_file       ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
    --gcc_tools_base ${CROSS_COMPILE}
    --src_root       ${ZEPHYR_BASE}
    --build_dir      ${CMAKE_BINARY_DIR}
    DEPENDS ${logical_target_for_zephyr_elf}
            $<TARGET_PROPERTY:zephyr_property_target,${report}_DEPENDENCIES>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
    )
endif()

find_program(PAHOLE pahole)

if(NOT ${PAHOLE} STREQUAL PAHOLE-NOTFOUND)
  add_custom_target(
    pahole
    ${PAHOLE}
    --anon_include
    --nested_anon_include
    --show_decl_info
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
    DEPENDS ${logical_target_for_zephyr_elf}
            $<TARGET_PROPERTY:zephyr_property_target,${report}_DEPENDENCIES>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
    )
endif()
