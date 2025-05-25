zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    CACHE INTERNAL "SoC Linker script ${SOC_NAME}"
)
