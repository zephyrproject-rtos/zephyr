#
# SPDX-FileCopyrightText: Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_NXP_SPECIFIC_MPU_SETTINGS)
  zephyr_sources(${ZEPHYR_BASE}/boards/nxp/common/mpu/rt11xx_cm7_mpu_regions.c)
endif()

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library()
  if(NOT DEFINED CONFIG_BOARD_FRDM_IMXRT1152)
    message(WARNING "It appears you are using the board definition for "
      "the FRDM-IMXRT1152, but targeting a custom board. You may need to "
      "update your flash configuration block"
    )
  endif()
  if(CONFIG_BOOT_FLEXSPI_NOR)
    # This flash configuration block may need modification if another
    # flash chip is used on your custom board. See NXP AN12238 for more
    # information.
    zephyr_library_sources(xip/frdm_imxrt1152_flexspi_nor_config.c)
    zephyr_library_include_directories(xip)
  endif()
  if(CONFIG_EXTERNAL_MEM_CONFIG_DATA AND CONFIG_NXP_IMX_EXTERNAL_HYPERRAM)
    # This external memory configuration data block configures the
    # FlexSPI2 HyperRAM through the boot ROM.
    zephyr_library_compile_definitions(XIP_BOOT_HEADER_XMCD_ENABLE=1)
    zephyr_library_sources(xmcd/xmcd.c)
  else()
    dt_chosen(chosen_sram_path PROPERTY "zephyr,sram")
    dt_reg_addr(RAM_ADDR PATH "${chosen_sram_path}")

    if(${RAM_ADDR} EQUAL 0x60000000)
      message(WARNING "You are using HyperRAM as RAM but no external memory"
        "configuration data (XMCD) is included. This configuration may not boot")
    endif()
  endif()
endif()

if(CONFIG_MCUX_GPT_TIMER)
  message(WARNING "You appear to be using the GPT hardware timer. "
    "This timer will enable lower power modes, but at the cost of reduced "
    "hardware timer resolution")
endif()
