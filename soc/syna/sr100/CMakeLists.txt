# Copyright (c) 2025 Synaptics Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

find_path(SRSDK_IMGGEN
  srsdk_image_generator.py
  OPTIONAL
)

if(SRSDK_IMGGEN)
  message("-- Found SRSDK Image generator directory: ${SRSDK_IMGGEN}")
  add_custom_target(srsdk_generate_image ALL
    COMMAND ${PYTHON_EXECUTABLE} srsdk_image_generator.py
      -B0
      -flash_image
      -sdk_secured
      -spk B0_Input_examples/spk_rc3_0_secure_otpk_0605.bin
      -apbl B0_Input_examples/sr100_b0_bootloader_ver_0x0134_ASIC_Release.axf
      -m55_image ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}.elf
      -model ''
      -flash_type GD25LE128
      -flash_freq 67
    COMMAND ${CMAKE_COMMAND} -E copy
      Output/B0_Flash/B0_flash_full_image_GD25LE128_67Mhz_secured.bin
      ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_NAME}_flash.bin
    WORKING_DIRECTORY ${SRSDK_IMGGEN}
    DEPENDS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  )

  set_property(TARGET runners_yaml_props_target PROPERTY bin_file ${KERNEL_NAME}_flash.bin)
endif()
