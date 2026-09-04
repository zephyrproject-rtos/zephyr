# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R9A07G066M04")

if(CONFIG_BUILD_WITH_TFA)
  set(TFA_PLAT "rza3m_ek_nor")
  if(CONFIG_TFA_MAKE_BUILD_TYPE_DEBUG)
    set(BUILD_FOLDER "debug")
  else()
    set(BUILD_FOLDER "release")
  endif()
  set(TFA_BL2_PATH ${PROJECT_BINARY_DIR}/../tfa/${TFA_PLAT}/${BUILD_FOLDER}/${TFA_PLAT}_ipl.srec)

  board_runner_args(jlink "--pre-script-cmd=r")
  board_runner_args(jlink "--pre-script-cmd=loadfile ${TFA_BL2_PATH}")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
