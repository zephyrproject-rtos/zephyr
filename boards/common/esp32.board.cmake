# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(esp32)

set(ESP_IDF_PATH ${ZEPHYR_ESPRESSIF_MODULE_DIR})
assert(ESP_IDF_PATH "ESP_IDF_PATH is not set")

board_finalize_runner_args(esp32 "--esp-idf-path=${ESP_IDF_PATH}")
