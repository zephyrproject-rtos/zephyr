# Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
# Copyright (c) 2026 Mohan Nagaraj
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_UDOO_KEY_ESP32_PROCPU OR CONFIG_BOARD_UDOO_KEY_ESP32_APPCPU)

  if(NOT "${OPENOCD}" MATCHES "^${ESPRESSIF_TOOLCHAIN_PATH}/.*")
    set(OPENOCD OPENOCD-NOTFOUND)
  endif()

  find_program(OPENOCD
    openocd
    PATHS ${ESPRESSIF_TOOLCHAIN_PATH}/openocd-esp32/bin
    NO_DEFAULT_PATH
  )

elseif(CONFIG_BOARD_UDOO_KEY_RP2040)

  if("${UDOO_KEY_RP2040_DEBUG_ADAPTER}" STREQUAL "")
    set(UDOO_KEY_RP2040_DEBUG_ADAPTER "cmsis-dap")
  endif()

  board_runner_args(openocd
    --cmd-pre-init
    "source [find interface/${UDOO_KEY_RP2040_DEBUG_ADAPTER}.cfg]")

  board_runner_args(openocd "--cmd-pre-init=transport select swd")

  board_runner_args(openocd "--cmd-pre-init=source [find target/rp2040.cfg]")

  board_runner_args(probe-rs "--chip=RP2040")
  board_runner_args(jlink "--device=RP2040_M0_0")
  board_runner_args(uf2 "--board-id=RPI-RP2")
  board_runner_args(pyocd "--target=rp2040")

  board_set_debugger_ifnset(openocd)
  board_set_flasher_ifnset(openocd)

endif()

if(CONFIG_BOARD_UDOO_KEY_RP2040)
  include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/probe-rs.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
endif()

if(CONFIG_BOARD_UDOO_KEY_ESP32_PROCPU OR CONFIG_BOARD_UDOO_KEY_ESP32_APPCPU)
  include(${ZEPHYR_BASE}/boards/common/esp32.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
endif()