# Copyright (c) 2021 Nordic Semiconductor ASA.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP OR CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP_NS)
  zephyr_library()
  zephyr_library_sources(nrf5340_audio_dk_config.c)

  if(CONFIG_BUILD_WITH_TFM)
    zephyr_library_include_directories(
      $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
    )
  endif()
endif()
