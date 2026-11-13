# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(soc.c)

zephyr_include_directories(.)

if(CONFIG_BUILD_OUTPUT_BIN)
  set(bk7258_bin ${PROJECT_BINARY_DIR}/${KERNEL_NAME}.bk7258.bin)

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/beken_crc.py
            ${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME} ${bk7258_bin}
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${bk7258_bin})
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
