# Copyright (c) 2026 Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFA)
  dt_chosen(chosen_sram_path PROPERTY "zephyr,sram")
  dt_reg_addr(RAM_ADDR PATH "${chosen_sram_path}")

  set(TFA_PLAT "zynqmp")
  set(TFA_EXTRA_ARGS "PRELOADED_BL33_BASE=${RAM_ADDR}")
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(BUILD_FOLDER "debug")
  else()
    set(BUILD_FOLDER "release")
  endif()
  set(XSDB_BL31_PATH ${PROJECT_BINARY_DIR}/../tfa/zynqmp/${BUILD_FOLDER}/bl31/bl31.elf)
  board_runner_args(xsdb "--bl31=${XSDB_BL31_PATH}")
endif()

include(${ZEPHYR_BASE}/boards/common/xsdb.board.cmake)
