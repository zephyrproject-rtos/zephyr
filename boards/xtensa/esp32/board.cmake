# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd  --cmd-pre-init "set ESP_RTOS none")
board_runner_args(openocd  --cmd-pre-init "set ESP32_ONLYCPU 1")

# select FT2232 as default interface (as in ESP-WROVER-KIT or ESP-Prog)
board_runner_args(openocd  --config "interface/ftdi/esp32_devkitj_v1.cfg")
board_runner_args(openocd  --config "target/esp32.cfg")

set (ESPRESSIF_OPENOCD_TOOL ${ESPRESSIF_TOOLCHAIN_PATH}/openocd-esp32/bin/openocd)
set (ESPRESSIF_OPENOCD_SCRIPTS ${ESPRESSIF_TOOLCHAIN_PATH}/openocd-esp32/share/openocd/scripts)

set(OPENOCD ${ESPRESSIF_OPENOCD_TOOL})
set(OPENOCD_DEFAULT_PATH ${ESPRESSIF_OPENOCD_SCRIPTS})

include(${ZEPHYR_BASE}/boards/common/esp32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
