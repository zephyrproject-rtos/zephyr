#
# Copyright 2025 PHYTEC America, LLC
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library()
  set(RT1170_BOARD_NAME "phycore_rt1170")

  if(CONFIG_BOOT_FLEXSPI_NOR)
    zephyr_library_sources(xip/${RT1170_BOARD_NAME}_flexspi_nor_config.c)
    zephyr_library_include_directories(xip)
  endif()

  if(CONFIG_EXTERNAL_MEM_CONFIG_DATA)
    zephyr_library_compile_definitions(XIP_BOOT_HEADER_XMCD_ENABLE=1)
    zephyr_library_sources(xmcd/xmcd.c)
  else()
    if(CONFIG_SRAM_DEPRECATED_KCONFIG_SET)
      set(RAM_ADDR ${CONFIG_SRAM_BASE_ADDRESS})
    else()
      dt_chosen(chosen_sram_path PROPERTY "zephyr,sram")
      dt_reg_addr(RAM_ADDR PATH "${chosen_sram_path}")
    endif()

    if(${RAM_ADDR} EQUAL 0x80000000)
      message(WARNING "You are using SDRAM as RAM but no external memory"
        "configuration data (XMCD) is included. This configuration may not boot")
    endif()
  endif()

endif()

if(CONFIG_MCUX_GPT_TIMER)
  message(WARNING "You appear to be using the GPT hardware timer. "
    "This timer will enable lower power modes, but at the cost of reduced "
    "hardware timer resolution")
endif()
