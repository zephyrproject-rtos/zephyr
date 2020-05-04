# SPDX-License-Identifier: Apache-2.0

set(flag_for_ram_report -r)
set(flag_for_rom_report -F)

foreach(report ram_report rom_report)
  add_custom_target(
    ${report}
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/footprint/size_report
    ${flag_for_${report}}
    --objdump ${CMAKE_OBJDUMP}
    --objcopy ${CMAKE_OBJCOPY}
    --nm      ${CMAKE_NM}
    -o ${PROJECT_BINARY_DIR}
    DEPENDS ${logical_target_for_zephyr_elf}
            $<TARGET_PROPERTY:zephyr_property_target,${report}_DEPENDENCIES>
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
