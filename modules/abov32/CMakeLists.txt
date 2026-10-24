# Copyright (c) 2026 ABOV Semiconductor Co.,Ltd.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_FAMILY_ABOV32)

zephyr_include_directories(
  ${ZEPHYR_CURRENT_MODULE_DIR}/audk32/Platform/HAL/Include
  ${ZEPHYR_CURRENT_MODULE_DIR}/audk32/Platform/HAL/HPL/Include
  ${ZEPHYR_CURRENT_MODULE_DIR}/audk32/Framework/CMSIS/Core/Include
)

file(GLOB_RECURSE AUDK32_HAL_SOURCES ${ZEPHYR_CURRENT_MODULE_DIR}/audk32/Platform/HAL/*.c)

zephyr_library()
zephyr_library_sources(
  ${AUDK32_HAL_SOURCES}
  ${ZEPHYR_CURRENT_MODULE_DIR}/audk32/Framework/CMSIS/Device/ABOV/Source/system_a31xxxx.c
)

if(CONFIG_SOC_PRODUCT_LINE_A31XXXX)
  if(CONFIG_SOC_SERIES_A31C15X)
    zephyr_include_directories(
      ${ZEPHYR_CURRENT_MODULE_DIR}/audk32/ProductConfig/A31xxxx/Config
      ${ZEPHYR_CURRENT_MODULE_DIR}/audk32/ProductConfig/A31xxxx/A31C15x/Config
      ${ZEPHYR_CURRENT_MODULE_DIR}/audk32/Framework/CMSIS/Device/ABOV/Include
    )
  endif()
endif()

endif()
