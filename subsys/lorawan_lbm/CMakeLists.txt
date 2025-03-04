# Copyright (c) 2024 Semtech Corporation
# SPDX-License-Identifier: Apache-2.0


if(CONFIG_LORA_BASICS_MODEM)


if(CONFIG_LORA_BASICS_MODEM_MAIN_THREAD)
  zephyr_library()
  zephyr_library_sources(
    ${CMAKE_CURRENT_LIST_DIR}/lbm_main_thread.c
  )
endif()

endif()
