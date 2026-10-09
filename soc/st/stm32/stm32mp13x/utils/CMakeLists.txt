# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_SERIES_STM32MP13X_BOOTROM_HEADER)
  dt_chosen(stm32mp13_sram_path PROPERTY "zephyr,sram")
  dt_reg_addr(stm32mp13_load_address PATH "${stm32mp13_sram_path}")
  math(EXPR stm32mp13_entry_point
    "${stm32mp13_load_address} + ${CONFIG_ROM_START_OFFSET}"
    OUTPUT_FORMAT HEXADECIMAL
  )

  set(stm32mp13_image ${CONFIG_KERNEL_BIN_NAME}.stm32)
  set(stm32mp13_image_script ${CMAKE_CURRENT_SOURCE_DIR}/stm32mp13_image.py)

  # Relink and rerun the post-build command when the image generator changes.
  set_property(TARGET zephyr_interface APPEND PROPERTY INTERFACE_LINK_DEPENDS
    ${stm32mp13_image_script}
  )

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${stm32mp13_image_script}
      --entry-point ${stm32mp13_entry_point}
      --load-address ${stm32mp13_load_address}
      --binary-type ${CONFIG_SOC_SERIES_STM32MP13X_BOOTROM_HEADER_BINARY_TYPE}
      ${KERNEL_BIN_NAME}
      ${stm32mp13_image}
  )
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts ${stm32mp13_image})
  set_property(TARGET runners_yaml_props_target PROPERTY bin_file ${stm32mp13_image})
endif()
