# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0

set(serial_memory_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/serial-memory)

zephyr_include_directories(${serial_memory_dir}/include)
zephyr_include_directories(${serial_memory_dir}/source)

zephyr_library_sources_ifdef(CONFIG_USE_INFINEON_SMIF ${serial_memory_dir}/source/mtb_serial_memory.c)

if(CONFIG_USE_INFINEON_SMIF AND CONFIG_XIP)

  if(CONFIG_CPU_CORTEX_M33 AND CONFIG_TRUSTED_EXECUTION_SECURE)
    set(sram_code M33SCODE)
  elseif(CONFIG_CPU_CORTEX_M33)
    set(sram_code M33CODE)
  else()
    set(sram_code ITCM)
  endif()

  zephyr_code_relocate(FILES ${serial_memory_dir}/source/mtb_serial_memory.c LOCATION ${sram_code})
endif()
