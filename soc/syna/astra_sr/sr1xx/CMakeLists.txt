# SPDX-FileCopyrightText: Copyright (c) 2025 Synaptics Incorporated
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

if(CONFIG_SOC_SR100_M55)
  find_path(SRSDK_IMGGEN
    srsdk_image_generator.py
    HINTS $ENV{SRSDK_IMGGEN}
    OPTIONAL
  )

  if(SRSDK_IMGGEN)
    message("-- Found SRSDK Image generator directory: ${SRSDK_IMGGEN}")

    zephyr_get(SR100_M4_ELF SYSBUILD GLOBAL)

    add_custom_target(srsdk_generate_image ALL
      COMMAND ${CMAKE_COMMAND}
        -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
        -DSRSDK_IMGGEN:PATH=${SRSDK_IMGGEN}
        -DM4_ELF:FILEPATH=${SR100_M4_ELF}
        -DM55_ELF:FILEPATH=${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}.elf
        -DOUTPUT_BIN:FILEPATH=${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}_flash.bin
        -P ${CMAKE_CURRENT_LIST_DIR}/srsdk_generate_image.cmake
      DEPENDS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
    )

    set_property(TARGET runners_yaml_props_target PROPERTY bin_file ${KERNEL_NAME}_flash.bin)
  endif()
endif()
