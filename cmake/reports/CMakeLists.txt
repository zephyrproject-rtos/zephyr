set(flag_for_ram_report -r)
set(flag_for_rom_report -F)

foreach(report ram_report rom_report)
  add_custom_target(
    ${report}
    ${PYTHON_EXECUTABLE}
    $ENV{ZEPHYR_BASE}/scripts/footprint/size_report
    ${flag_for_${report}}
    --objdump ${CMAKE_OBJDUMP}
    --nm      ${CMAKE_NM}
    -o ${PROJECT_BINARY_DIR}
    DEPENDS ${logical_target_for_zephyr_elf}
    )
endforeach()
