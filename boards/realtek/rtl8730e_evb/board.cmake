# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

dt_chosen(shelluart PROPERTY "zephyr,shell-uart")
if(shelluart)
  dt_prop(shelluart_baudrate PATH ${shelluart} PROPERTY "current-speed")
  board_runner_args(amebaflash "--baudrate=${shelluart_baudrate}")
endif()

board_runner_args(amebaflash "--image-dir=${ZEPHYR_BINARY_DIR}/../images" "--device=${CONFIG_SOC_SERIES}")
board_set_flasher_ifnset(amebaflash)
board_finalize_runner_args(amebaflash)

# Each cluster has its own CoreSight access-port script.
if(CONFIG_SOC_RTL8730E_CA32)
  board_runner_args(jlink "--device=Cortex-A32" "--speed=4000")
  board_runner_args(jlink "--tool-opt=-scriptfile ${CMAKE_CURRENT_LIST_DIR}/support/AP3_CA32_Core0.JLinkScript")
elseif(CONFIG_SOC_RTL8730E_KM4)
  board_runner_args(jlink "--device=Cortex-M55" "--speed=4000")
  board_runner_args(jlink "--tool-opt=-scriptfile ${CMAKE_CURRENT_LIST_DIR}/support/AP1_KM4.JLinkScript")
elseif(CONFIG_SOC_RTL8730E_KM0)
  board_runner_args(jlink "--device=Cortex-M23" "--speed=4000")
  board_runner_args(jlink "--tool-opt=-scriptfile ${CMAKE_CURRENT_LIST_DIR}/support/AP0_KM0.JLinkScript")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
