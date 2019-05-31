# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(esp32)

if(NOT DEFINED ESP_IDF_PATH)
  if(DEFINED ENV{ESP_IDF_PATH})
    message(WARNING "Setting ESP_IDF_PATH in the environment is deprecated. Use cmake -DESP_IDF_PATH=... instead.")
    set(ESP_IDF_PATH $ENV{ESP_IDF_PATH})
  endif()
endif()

assert(ESP_IDF_PATH "ESP_IDF_PATH is not set")

board_finalize_runner_args(esp32 "--esp-idf-path=${ESP_IDF_PATH}")
