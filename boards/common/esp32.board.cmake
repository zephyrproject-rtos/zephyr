# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(esp32)
board_set_debugger_ifnset(openocd)

board_runner_args(openocd  --no-halt --no-targets --no-load --target-handle _TARGETNAME_0)
board_runner_args(openocd  --gdb-init "set remote hardware-watchpoint-limit 2")

board_runner_args(openocd  --gdb-init "maintenance flush register-cache")
board_runner_args(openocd  --gdb-init "mon reset halt")
board_runner_args(openocd  --gdb-init "thb main")

set(ESP_IDF_PATH ${ZEPHYR_HAL_ESPRESSIF_MODULE_DIR})
assert(ESP_IDF_PATH "ESP_IDF_PATH is not set")

board_runner_args(esp32 "--esp-idf-path=${ESP_IDF_PATH}")
