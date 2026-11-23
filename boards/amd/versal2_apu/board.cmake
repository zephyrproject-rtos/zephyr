#
# Copyright (c) 2025-2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BUILD_WITH_TFA)
  if(CONFIG_SRAM_DEPRECATED_KCONFIG_SET)
    set(RAM_ADDR ${CONFIG_SRAM_BASE_ADDRESS})
  else()
    dt_chosen(chosen_sram_path PROPERTY "zephyr,sram")
    dt_reg_addr(RAM_ADDR PATH "${chosen_sram_path}")
  endif()

  set(TFA_PLAT "versal2")
  # Add Versal Gen 2 specific TF-A build parameters
  set(TFA_EXTRA_ARGS "RESET_TO_BL31=1;PRELOADED_BL33_BASE=${RAM_ADDR};TFA_NO_PM=1;")
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(BUILD_FOLDER "debug")
  else()
    set(BUILD_FOLDER "release")
  endif()
  set(XSDB_BL31_PATH ${PROJECT_BINARY_DIR}/../tfa/versal2/${BUILD_FOLDER}/bl31/bl31.elf)
  board_runner_args(xsdb "--bl31=${XSDB_BL31_PATH}")
endif()

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)
